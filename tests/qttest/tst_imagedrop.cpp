/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "imagedrop.h"

#include <QtEndian>
#include <QtTest>

#include <QImage>
#include <QMimeData>
#include <QTemporaryDir>
#include <QUrl>

/**
	Picture files dropped on a folio: which files are taken, the limits a
	file must meet before it is embedded in the project, and where the
	pictures of one drop land.
*/
class tst_imagedrop : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QString path(const QString &name) const { return m_dir.filePath(name); }

	static QMimeData *urls(const QStringList &paths)
	{
		auto *mime = new QMimeData();
		QList<QUrl> list;
		for (const QString &p : paths)
			list << (p.startsWith("http") ? QUrl(p) : QUrl::fromLocalFile(p));
		mime->setUrls(list);
		return mime;
	}

	// A BMP header announcing width x height pixels, without the pixels.
	static QByteArray bmpHeader(qint32 width, qint32 height)
	{
		QByteArray b(54, '\0');
		b[0] = 'B'; b[1] = 'M';
		qToLittleEndian<quint32>(54, b.data() + 2);
		qToLittleEndian<quint32>(54, b.data() + 10);
		qToLittleEndian<quint32>(40, b.data() + 14);
		qToLittleEndian<qint32>(width, b.data() + 18);
		qToLittleEndian<qint32>(height, b.data() + 22);
		qToLittleEndian<quint16>(1, b.data() + 26);
		qToLittleEndian<quint16>(24, b.data() + 28);
		return b;
	}

	static void write(const QString &p, const QByteArray &data)
	{
		QFile f(p);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(data);
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QImage img(40, 20, QImage::Format_ARGB32);
		img.fill(Qt::red);
		QVERIFY(img.save(path("a.png")));
		QVERIFY(img.save(path("b.JPG"), "JPG"));
		write(path("plan.qet"), "<project/>");
		write(path("fake.png"), "this is not a picture");
		write(path("bomb.bmp"), bmpHeader(10000, 10000));
		QFile big(path("big.png"));
		QVERIFY(big.open(QIODevice::WriteOnly));
		QVERIFY(big.resize(ImageDrop::maxFileBytes + 1));
	}

	// Only local files with a picture suffix are taken, in drop order and
	// whatever the case of the suffix.
	void picturesAreTakenInOrder()
	{
		std::unique_ptr<QMimeData> mime(urls({path("b.JPG"), path("plan.qet"), path("a.png"),
											   "https://example.org/remote.png"}));
		QCOMPARE(ImageDrop::imageFiles(mime.get()), QStringList({path("b.JPG"), path("a.png")}));
		QVERIFY(!ImageDrop::hasOnlyOtherUrls(mime.get()));
	}

	// A drop holding only a project is left to the main window, which
	// opens it; plain text is not a URL drop at all.
	void otherFilesAreLeftAlone()
	{
		std::unique_ptr<QMimeData> project(urls({path("plan.qet")}));
		QVERIFY(ImageDrop::imageFiles(project.get()).isEmpty());
		QVERIFY(ImageDrop::hasOnlyOtherUrls(project.get()));

		QMimeData text;
		text.setText("K1");
		QVERIFY(!ImageDrop::hasOnlyOtherUrls(&text));
		QVERIFY(ImageDrop::imageFiles(&text).isEmpty());
		QVERIFY(ImageDrop::imageFiles(nullptr).isEmpty());
	}

	void validPictureLoads()
	{
		QString error;
		const QImage image = ImageDrop::load(path("a.png"), &error);
		QCOMPARE(image.size(), QSize(40, 20));
		QVERIFY(error.isEmpty());
	}

	void unusableFilesAreRefusedWithAReason_data()
	{
		QTest::addColumn<QString>("file");
		QTest::addColumn<QString>("reason");
		QTest::newRow("missing") << path("missing.png") << "file";
		QTest::newRow("directory") << m_dir.path() << "file";
		QTest::newRow("not a picture") << path("fake.png") << "read";
		QTest::newRow("over 10 MB") << path("big.png") << "10 MB";
		// refused on its header, before the 300 MB of pixels are allocated
		QTest::newRow("too many pixels") << path("bomb.bmp") << "pixels";
	}

	void unusableFilesAreRefusedWithAReason()
	{
		QFETCH(QString, file);
		QFETCH(QString, reason);
		QString error;
		QVERIFY(ImageDrop::load(file, &error).isNull());
		QVERIFY2(error.contains(reason), qPrintable(error));
	}

	// A picture too large for the folio is scaled down until it leaves 20 %
	// of the frame free on every side, i.e. into 60 % of its width and
	// height; a picture that already fits keeps its size.
	void largePicturesLeaveAMarginToTheFrame_data()
	{
		QTest::addColumn<QSizeF>("size");
		QTest::addColumn<qreal>("scale");
		// frame: 1000 x 800, so the inner frame is 600 x 480
		QTest::newRow("small, kept") << QSizeF(100, 50) << 1.0;
		QTest::newRow("exactly the inner frame") << QSizeF(600, 480) << 1.0;
		QTest::newRow("wide") << QSizeF(3000, 1000) << 0.2;
		QTest::newRow("tall photo") << QSizeF(3024, 4032) << 480.0 / 4032;
	}

	void largePicturesLeaveAMarginToTheFrame()
	{
		QFETCH(QSizeF, size);
		QFETCH(qreal, scale);
		const QRectF frame(50, 30, 1000, 800);
		QCOMPARE(ImageDrop::innerFrame(frame), QRectF(250, 190, 600, 480));
		QCOMPARE(ImageDrop::fitScale(size, frame), scale);
		const QSizeF scaled = size * ImageDrop::fitScale(size, frame);
		QVERIFY(scaled.width() <= 600.0001 && scaled.height() <= 480.0001);
	}

	void noFrameMeansNoScaling()
	{
		QCOMPARE(ImageDrop::fitScale(QSizeF(4000, 3000), QRectF()), 1.0);
	}

	// A picture dropped near an edge is moved back inside, by the least
	// amount; one larger than the area is centred on it.
	void picturesAreKeptInsideTheArea_data()
	{
		QTest::addColumn<QRectF>("picture");
		QTest::addColumn<QRectF>("expected");
		const QRectF inside(300, 300, 100, 50);
		QTest::newRow("already inside") << inside << inside;
		QTest::newRow("over the top edge") << QRectF(300, -20, 100, 50) << QRectF(300, 0, 100, 50);
		QTest::newRow("past the right and bottom") << QRectF(980, 790, 100, 50)
												  << QRectF(900, 750, 100, 50);
		QTest::newRow("wider than the area") << QRectF(-50, 100, 1200, 50)
											<< QRectF(-100, 100, 1200, 50);
	}

	void picturesAreKeptInsideTheArea()
	{
		QFETCH(QRectF, picture);
		QFETCH(QRectF, expected);
		QCOMPARE(ImageDrop::keepInside(picture, QRectF(0, 0, 1000, 800)), expected);
	}

	void picturesOfOneDropCascade()
	{
		QCOMPARE(ImageDrop::cascadeOffset(0), QPointF(0, 0));
		QCOMPARE(ImageDrop::cascadeOffset(2), QPointF(2 * ImageDrop::cascadeStep,
													  2 * ImageDrop::cascadeStep));
	}
};

QTEST_MAIN(tst_imagedrop)
#include "tst_imagedrop.moc"
