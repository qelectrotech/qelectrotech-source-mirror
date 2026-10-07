// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include "darkimagerendering.h"
#include "palettegraphicsview.h"
#include "qetpalette.h"

class Raster : public QGraphicsItem {
public:
	QPixmap pixmap;
	bool adapt = false;
	explicit Raster(QGraphicsItem *parent = nullptr) : QGraphicsItem(parent) {
		QImage image(40, 30, QImage::Format_ARGB32);
		image.fill(QColor(197, 81, 33));
		for (int y = 0; y < 30; ++y)
			for (int x = 10; x < 20; ++x) image.setPixelColor(x, y, QColor(197,81,33,128));
		for (int y = 0; y < 30; ++y)
			for (int x = 30; x < 40; ++x) image.setPixelColor(x, y, Qt::transparent);
		pixmap = QPixmap::fromImage(image);
	}
	QRectF boundingRect() const override { return QRectF(0,0,40,40); }
	void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override {
		DarkImageRendering::paintPixmap(p, pixmap, adapt);
		p->fillRect(QRectF(0,34,8,4), Qt::black); // caption/decorations remain adapted
	}
};

class tst_darkimagelayers : public QObject {
	Q_OBJECT
	static QColor sample(const QImage &image, const PaletteGraphicsView &view, QPointF scene) {
		const QPoint point = view.mapFromScene(scene);
		const qreal dpr = image.devicePixelRatio();
		return image.pixelColor(qRound(point.x()*dpr), qRound(point.y()*dpr));
	}
	static void setup(PaletteGraphicsView &view) {
		view.setFrameShape(QFrame::NoFrame);
		view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
		view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		view.resize(100,80); view.show();
	}
private slots:
	void cleanup() {
		QApplication::setPalette(QET::Palette::fusionLight());
		PaletteGraphicsView::setCustomBackgroundColor(false);
	}
	void modes_data() {
		QTest::addColumn<bool>("dark"); QTest::addColumn<bool>("adapt"); QTest::addColumn<bool>("custom");
		QTest::newRow("light-original") << false << false << false;
		QTest::newRow("light-adapt") << false << true << false;
		QTest::newRow("dark-original") << true << false << false;
		QTest::newRow("dark-adapt") << true << true << false;
		QTest::newRow("light-custom-original") << false << false << true;
		QTest::newRow("light-custom-adapt") << false << true << true;
		QTest::newRow("dark-custom-original") << true << false << true;
		QTest::newRow("dark-custom-adapt") << true << true << true;
	}
	void modes() {
		QFETCH(bool,dark); QFETCH(bool,adapt); QFETCH(bool,custom);
		PaletteGraphicsView::setCustomBackgroundColor(custom);
		const QPalette palette = dark ? QET::Palette::fusionDark() : QET::Palette::fusionLight();
		QApplication::setPalette(palette);
		const QColor background = custom ? QColor(80,90,100) : QColor(Qt::white);
		QGraphicsScene scene(0,0,100,80); scene.setBackgroundBrush(background);
		auto *raster = new Raster; raster->adapt = adapt; raster->setPos(10,10); scene.addItem(raster);
		scene.addRect(32,10,5,20,QPen(Qt::NoPen),QBrush(Qt::black))->setZValue(1);
		PaletteGraphicsView view(&scene); setup(view);
		QVERIFY(QTest::qWaitForWindowExposed(&view));
		const QImage shown = view.viewport()->grab().toImage();
		QImage expected(100,80,QImage::Format_RGB32);
		expected.fill(custom ? background : (dark && !adapt ? palette.color(QPalette::Base) : QColor(Qt::white)));
		QPainter p(&expected);
		if (dark && custom && adapt) {
			QImage adapted = raster->pixmap.toImage();
			QET::Palette::invertLightnessLayer(adapted,palette.color(QPalette::Base),palette.color(QPalette::Text));
			p.drawImage(QPoint(10,10),adapted);
		} else p.drawPixmap(QPoint(10,10),raster->pixmap);
		p.fillRect(QRect(32,10,5,20), dark && !adapt && !custom ? palette.color(QPalette::Text) : QColor(Qt::black));
		p.end();
		if (dark && adapt && !custom) QET::Palette::invertLightness(expected,palette.color(QPalette::Base),palette.color(QPalette::Text));
		for (const QPoint point : {QPoint(15,15),QPoint(25,15),QPoint(45,15),QPoint(34,15)})
			QCOMPARE(sample(shown,view,point), expected.pixelColor(point));
		QCOMPARE(sample(shown,view,QPoint(13,45)), dark && !custom ? palette.color(QPalette::Text) : QColor(Qt::black));
	}
	void parentClippingRotationOpacityAndOverlap() {
		const auto palette = QET::Palette::fusionDark(); QApplication::setPalette(palette);
		QGraphicsScene scene(0,0,100,80); scene.setBackgroundBrush(Qt::white);
		auto *parent = scene.addRect(0,0,25,40,QPen(Qt::NoPen),QBrush(Qt::NoBrush));
		parent->setFlag(QGraphicsItem::ItemClipsChildrenToShape); parent->setPos(35,15); parent->setRotation(12);
		auto *first = new Raster(parent); first->setOpacity(0.5);
		auto *second = new Raster; second->setPos(36,30); scene.addItem(second); second->setZValue(2);
		QImage blue(40,30,QImage::Format_ARGB32); blue.fill(QColor(20,80,210,128)); second->pixmap=QPixmap::fromImage(blue);
		PaletteGraphicsView view(&scene); setup(view); QVERIFY(QTest::qWaitForWindowExposed(&view));
		const auto shown = view.viewport()->grab().toImage();
		// Original pixels blend once over the dark sheet, then the upper
		// original image blends once over those pixels. No inverse transform.
		QImage reference(1,1,QImage::Format_RGB32); reference.fill(palette.color(QPalette::Base));
		QPainter p(&reference); p.setOpacity(0.5); p.fillRect(reference.rect(),QColor(197,81,33)); p.end();
		QCOMPARE(sample(shown,view,first->mapToScene(QPointF(5,5))),reference.pixelColor(0,0));
		QPainter upper(&reference); upper.fillRect(reference.rect(),QColor(20,80,210,128)); upper.end();
		QCOMPARE(sample(shown,view,first->mapToScene(QPointF(5,20))),reference.pixelColor(0,0));
		// Pixel in the child's transparent stripe / beyond the parent's clip.
		QCOMPARE(sample(shown,view,first->mapToScene(QPointF(28,5))),palette.color(QPalette::Base));
		first->setPos(3,0); scene.update(); QApplication::processEvents();
		QVERIFY(!view.viewport()->grab().isNull());
	}
	void alphaMapping() {
		QImage image(2,1,QImage::Format_ARGB32_Premultiplied);
		image.setPixelColor(0,0,QColor(0,0,0,128)); image.setPixelColor(1,0,Qt::transparent);
		QET::Palette::invertLightnessLayer(image,QColor(30,30,30),QColor(220,220,220));
		QCOMPARE(image.pixelColor(0,0),QColor(220,220,220,128));
		QCOMPARE(image.pixelColor(1,0).alpha(),0);
	}
	void exportUnaffected() {
		QApplication::setPalette(QET::Palette::fusionDark());
		QGraphicsScene scene(0,0,100,80); scene.setBackgroundBrush(Qt::white);
		auto *raster=new Raster; raster->setPos(10,10); scene.addItem(raster);
		QImage results[2];
		for(int i=0;i<2;++i) {
			raster->adapt=i; results[i]=QImage(100,80,QImage::Format_RGB32); results[i].fill(Qt::white);
			QPainter p(&results[i]); scene.render(&p,QRectF(0,0,100,80),scene.sceneRect());
		}
		QCOMPARE(results[0],results[1]);
		QCOMPARE(results[0].pixelColor(15,15),QColor(197,81,33));
	}
};
QTEST_MAIN(tst_darkimagelayers)
#include "tst_darkimagelayers.moc"
