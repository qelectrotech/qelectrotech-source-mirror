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
#include "qetapptest.h"
#include "qetgraphicsitem/diagramimageitem.h"

#include <QBuffer>
#include <QDomDocument>
#include <QPicture>
#include <QProcess>
#include <QRegularExpression>
#include <QSvgGenerator>
#include <QTemporaryDir>

/**
	SVG export with "smaller pictures" (ExportProperties::svg_small_pictures,
	--svg-small-pictures) stores each picture at four times the size it is
	drawn at when that is smaller than the picture, and at full resolution
	otherwise. fixtures/svg_small_pictures.qet has an 800x600 picture drawn
	at a tenth of its size and a 40x30 one at full size.
*/
class tst_svgsmallpictures : public QObject
{
	Q_OBJECT

	QString m_fixture;

		/// Pixel size of each picture embedded in @p svg, smallest first
	static QList<QSize> embeddedSizes(const QByteArray &svg)
	{
		static const QRegularExpression data(
					QStringLiteral("data:image/png;base64,([A-Za-z0-9+/=\\s]+)"));
		QList<QSize> sizes;
		auto it = data.globalMatch(QString::fromLatin1(svg));
		while (it.hasNext())
			sizes << QImage::fromData(QByteArray::fromBase64(
										  it.next().captured(1).toLatin1())).size();
		std::sort(sizes.begin(), sizes.end(), [](const QSize &a, const QSize &b) {
			return a.width() < b.width();
		});
		return sizes;
	}

	QByteArray exportWithCli(const QStringList &extra)
	{
		QTemporaryDir dir;
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   QStringList{QStringLiteral("--export-svg"), m_fixture, dir.path()} + extra);
		if (!proc.waitForFinished(60000) || proc.exitCode() != 0)
			return {};
		const QStringList files = QDir(dir.path()).entryList({QStringLiteral("*.svg")});
		QFile svg(QDir(dir.path()).filePath(files.value(0)));
		return svg.open(QIODevice::ReadOnly) ? svg.readAll() : QByteArray();
	}

		/// As ExportDialog::generateSvg(): the folio drawn into a QPicture,
		/// played into the SVG generator
	static QByteArray exportLikeTheDialog(Diagram *diagram, bool small)
	{
		const QSize size = diagram->imageSize();
		QPicture picture;
		diagram->setSmallPictures(small);
		diagram->toPaintDevice(picture, size.width(), size.height());
		diagram->setSmallPictures(false);

		QByteArray svg;
		QBuffer buffer(&svg);
		buffer.open(QIODevice::WriteOnly);
		QSvgGenerator generator;
		generator.setSize(size);
		generator.setOutputDevice(&buffer);
		QPainter painter(&generator);
		picture.play(&painter);
		painter.end();
		return svg;
	}

private slots:
	void initTestCase()
	{
		m_fixture = QFINDTESTDATA("fixtures/svg_small_pictures.qet");
		QVERIFY2(!m_fixture.isEmpty(), "fixture project not found");
		QVERIFY(QetAppTest::editor());
	}

	void cliFullResolutionByDefault()
	{
		const QByteArray svg = exportWithCli({});
		QVERIFY2(!svg.isEmpty(), "--export-svg failed");
		QCOMPARE(embeddedSizes(svg), (QList<QSize>{QSize(40, 30), QSize(800, 600)}));
	}

	void cliSmallPictures()
	{
		const QByteArray svg = exportWithCli({QStringLiteral("--svg-small-pictures")});
		QVERIFY2(!svg.isEmpty(), "--export-svg --svg-small-pictures failed");
			// 800 x 600 drawn at 0.1, times 4; the 40 x 30 one is kept whole
		QCOMPARE(embeddedSizes(svg), (QList<QSize>{QSize(40, 30), QSize(320, 240)}));
	}

	void dialogSmallPicturesThenFullAgain()
	{
		QFile file(m_fixture);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QDomDocument document;
		QVERIFY(document.setContent(&file));
		const QDomElement image_xml = document.elementsByTagName(
					QStringLiteral("image")).at(0).toElement();
		Diagram *diagram = QetAppTest::diagram();
		auto *picture = new DiagramImageItem();
		QVERIFY(picture->fromXml(image_xml));
		diagram->addItem(picture);

		QCOMPARE(embeddedSizes(exportLikeTheDialog(diagram, true)),
				 QList<QSize>{QSize(320, 240)});
			// The option lasts for one export only
		QVERIFY(!diagram->smallPictures());
		QCOMPARE(embeddedSizes(exportLikeTheDialog(diagram, false)),
				 QList<QSize>{QSize(800, 600)});
		delete picture;
	}
};

QET_APP_TEST_MAIN(tst_svgsmallpictures)
#include "tst_svgsmallpictures.moc"
