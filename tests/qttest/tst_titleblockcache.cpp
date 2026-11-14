// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "diagram.h"
#include "qetproject.h"
#include "titleblockproperties.h"

#include <QGraphicsView>
#include <QImage>
#include <QPainter>
#include <QtTest>

/*
	The title block a view shows is recorded once and played at every
	paint (TitleBlockTemplateRenderer). Checked here: what the screen shows
	is what a direct drawing of the folio shows, and a change of the
	folio's data reaches the screen.
*/
class tst_titleblockcache : public QObject
{
	Q_OBJECT

	static double differingShare(const QImage &a, const QImage &b)
	{
		if (a.size() != b.size()) {
			return 1;
		}
		qint64 differing = 0;
		for (int y = 0; y < a.height(); ++y) {
			for (int x = 0; x < a.width(); ++x) {
				const QRgb p = a.pixel(x, y), q = b.pixel(x, y);
				if (qAbs(qRed(p) - qRed(q)) + qAbs(qGreen(p) - qGreen(q))
						+ qAbs(qBlue(p) - qBlue(q)) > 60) {
					++differing;
				}
			}
		}
		return double(differing) / (a.width() * a.height());
	}

	static QImage direct(Diagram *diagram, QGraphicsView *view)
	{
		QImage image(view->viewport()->size(), QImage::Format_RGB32);
		image.fill(Qt::white);
		QPainter painter(&image);
		painter.setRenderHints(view->renderHints());
		diagram->render(&painter, QRectF(image.rect()),
						view->mapToScene(view->viewport()->rect()).boundingRect());
		return image;
	}

	static QImage screen(QGraphicsView *view)
	{
		return view->viewport()->grab().toImage().convertToFormat(QImage::Format_RGB32);
	}

private slots:
	void screenShowsTheTitleBlockAsDrawn()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		Diagram *diagram = project.diagrams().first();
		QGraphicsView view(diagram);
		view.resize(900, 650);
		view.show();
		QVERIFY(QTest::qWaitForWindowExposed(&view));
		const QRectF block = diagram->border_and_titleblock.titleBlockRect();
		view.fitInView(block, Qt::KeepAspectRatio);
		QCoreApplication::processEvents();

			//Twice on screen: recorded, then played
		screen(&view);
		const QImage played = screen(&view);
		QVERIFY2(differingShare(played, direct(diagram, &view)) < 0.01,
				 "the title block on screen differs from the one drawn");

			//New data on the folio: the screen shows it
		TitleBlockProperties properties = diagram->border_and_titleblock.exportTitleBlock();
		properties.title = QStringLiteral("CHANGED TITLE 12345");
		diagram->border_and_titleblock.importTitleBlock(properties);
		QCoreApplication::processEvents();
		const QImage changed = screen(&view);
		QVERIFY2(differingShare(changed, direct(diagram, &view)) < 0.01,
				 "the title block on screen kept the former data");
		QVERIFY(differingShare(changed, played) > 0);
	}
};

QET_APP_TEST_MAIN(tst_titleblockcache)
#include "tst_titleblockcache.moc"
