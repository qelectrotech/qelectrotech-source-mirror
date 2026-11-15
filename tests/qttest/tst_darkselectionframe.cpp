// SPDX-License-Identifier: GPL-2.0-or-later
	//The selection frame stands out on a dark sheet: drawn after the
	//lightness is inverted or the items tinted, which would otherwise make
	//it nearly the sheet's color
#include <QtTest>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include "palettegraphicsview.h"
#include "qetpalette.h"

namespace {
	class Framed : public QGraphicsItem {
	public:
		QRectF boundingRect() const override { return QRectF(0, 0, 60, 40); }
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override {
			if (!PaletteGraphicsView::deferSelectionFrame(painter, boundingRect(), false))
				PaletteGraphicsView::drawSelectionFrame(painter, boundingRect(), false, false);
		}
	};
}

class tst_darkselectionframe : public QObject {
	Q_OBJECT
		/// The lightest pixel on the frame's left edge
	static int lightestOnEdge(bool dark) {
		QApplication::setPalette(dark ? QET::Palette::fusionDark() : QET::Palette::fusionLight());
		QGraphicsScene scene(0, 0, 100, 80);
		scene.setBackgroundBrush(Qt::white);
		auto *item = new Framed;
		item->setPos(20, 20);
		scene.addItem(item);
		PaletteGraphicsView view(&scene);
		view.setFrameShape(QFrame::NoFrame);
		view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
		view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		view.resize(100, 80);
		view.show();
		if (!QTest::qWaitForWindowExposed(&view))
			return -1;
		const QImage shown = view.viewport()->grab().toImage();
		const qreal dpr = shown.devicePixelRatio();
		int lightest = 0, darkest = 255;
		for (int y = 32; y < 48; ++y) {
			const QPoint p = view.mapFromScene(QPointF(20, y));
			const int l = shown.pixelColor(qRound(p.x() * dpr), qRound(p.y() * dpr)).lightness();
			lightest = qMax(lightest, l);
			darkest = qMin(darkest, l);
		}
		return dark ? lightest : darkest;
	}
private slots:
	void cleanup() { QApplication::setPalette(QET::Palette::fusionLight()); }
		//On a light sheet the grey dashes as before
	void light() { QCOMPARE(lightestOnEdge(false), QColor(Qt::gray).lightness()); }
		//On a dark one light dashes, not the inverted grey
	void dark() { QVERIFY(lightestOnEdge(true) >= 190); }
};

QTEST_MAIN(tst_darkselectionframe)
#include "tst_darkselectionframe.moc"
