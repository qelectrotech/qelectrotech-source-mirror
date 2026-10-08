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
#include <QtTest>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>

/**
	Before #1310, a project saved after a crop was undone kept the undone
	<crop>, with the uncropped picture shown. Loading such a file has to
	drop that crop: the picture shown is always exactly the crop's size,
	so a picture of another size means the crop is stale. Otherwise the
	next edit of the picture applies the crop again.

	Runs the real binary: a script adds and crops a picture and saves;
	the test then puts another picture where the shown one is, and lets
	the binary load and save that file again.
*/
class tst_imagestalecrop : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	// Runs the binary on @p project with a script that saves it to @p out.
	bool run(const QString &script, const QString &project)
	{
		const QString path = m_dir.filePath(QStringLiteral("probe.js"));
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly)) return false;
		f.write(script.toUtf8());
		f.close();
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), path, project});
		return proc.waitForFinished(120000) && proc.exitStatus() == QProcess::NormalExit;
	}

	// Opens @p in and saves it to @p out.
	bool resave(const QString &in, const QString &out)
	{
		return run(QStringLiteral("qet.save('%1');\n").arg(out), in) && QFile::exists(out);
	}

	static QByteArray read(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

	static QByteArray png(const QSize &size)
	{
		QImage img(size, QImage::Format_RGB32);
		img.fill(Qt::darkGreen);
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		img.save(&buffer, "PNG");
		return bytes.toBase64();
	}

	// The size of the saved original (<image_base>), or an empty size.
	static QSize baseSize(const QByteArray &xml)
	{
		const QRegularExpression base(QStringLiteral("<image_base>\\s*([A-Za-z0-9+/=]+)"));
		const QRegularExpressionMatch m = base.match(QString::fromLatin1(xml));
		if (!m.hasMatch()) return {};
		return QImage::fromData(QByteArray::fromBase64(m.captured(1).toLatin1())).size();
	}

	// @p xml with the shown picture (the <image> element's first text)
	// replaced by a PNG of @p size.
	static QByteArray withShown(QByteArray xml, const QSize &size)
	{
		const QRegularExpression shown(QStringLiteral("(<image\\b[^>]*>\\s*)([A-Za-z0-9+/=]+)"));
		const QRegularExpressionMatch m = shown.match(QString::fromLatin1(xml));
		if (!m.hasMatch()) return {};
		return xml.replace(m.capturedStart(2), m.capturedLength(2), png(size));
	}

	QString m_cropped;   // a picture of 40 x 30, cropped to 10,5,20,10

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		QImage img(40, 30, QImage::Format_RGB32);
		img.fill(Qt::darkGreen);
		const QString pic = m_dir.filePath(QStringLiteral("pic.png"));
		QVERIFY(img.save(pic));

		m_cropped = m_dir.filePath(QStringLiteral("cropped.qet"));
		QVERIFY(run(QStringLiteral(R"JS(
var i = qet.addImage(0, '%1', 100, 100);
qet.cropImage(0, i, 10, 5, 20, 10);
qet.save('%2');
)JS").arg(pic, m_cropped), QStringLiteral(QET_EXAMPLES_DIR "/741.qet")));
		const QByteArray xml = read(m_cropped);
		QCOMPARE(xml.count("<image "), 1);
		QVERIFY(xml.contains("<crop "));
		QVERIFY(xml.contains("<image_base>"));
	}

	// A valid crop is kept, byte for byte.
	void validCropIsKept()
	{
		const QString out = m_dir.filePath(QStringLiteral("valid-out.qet"));
		QVERIFY(resave(m_cropped, out));
		const QByteArray xml = read(out);
		// Attributes in any order: Qt before 6.5 writes them unordered.
		const QRegularExpression crop(QStringLiteral("<crop (?=[^>]*\\bx=\"10\")(?=[^>]*\\by=\"5\")"
													 "(?=[^>]*\\bw=\"20\")(?=[^>]*\\bh=\"10\")"));
		QVERIFY2(crop.match(QString::fromLatin1(xml)).hasMatch(), "the valid crop was lost");
		QVERIFY(xml.contains("<image_base>"));
	}

	// The shown picture is the whole original (40 x 30): the crop was
	// undone before the file was saved. It is dropped, and with nothing
	// else to remember the original is not saved twice any more.
	void staleCropOfWholeOriginalIsDropped()
	{
		const QString stale = m_dir.filePath(QStringLiteral("stale.qet"));
		const QByteArray xml = withShown(read(m_cropped), QSize(40, 30));
		QVERIFY(!xml.isEmpty());
		QFile f(stale);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(xml);
		f.close();

		const QString out = m_dir.filePath(QStringLiteral("stale-out.qet"));
		QVERIFY(resave(stale, out));
		const QByteArray saved = read(out);
		QCOMPARE(saved.count("<image "), 1);
		QVERIFY2(!saved.contains("<crop "), "the stale crop was kept");
		QVERIFY(!saved.contains("<image_base>"));
	}

	// The shown picture matches neither the crop nor the original: it is
	// what the user saw, so it becomes the original -- a crop made after
	// loading cuts it, not the old original.
	void shownPictureOfOtherSizeBecomesTheOriginal()
	{
		const QString odd = m_dir.filePath(QStringLiteral("odd.qet"));
		const QByteArray xml = withShown(read(m_cropped), QSize(25, 25));
		QVERIFY(!xml.isEmpty());
		QFile f(odd);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(xml);
		f.close();

		const QString out = m_dir.filePath(QStringLiteral("odd-out.qet"));
		QVERIFY(resave(odd, out));
		const QByteArray saved = read(out);
		QVERIFY2(!saved.contains("<crop "), "the stale crop was kept");
		QVERIFY(!saved.contains("<image_base>"));

		const QString recropped = m_dir.filePath(QStringLiteral("odd-recropped.qet"));
		QVERIFY(run(QStringLiteral("qet.cropImage(0, 0, 0, 0, 10, 10);\nqet.save('%1');\n")
					.arg(recropped), odd));
		QCOMPARE(baseSize(read(recropped)), QSize(25, 25));
	}
};

QTEST_MAIN(tst_imagestalecrop)
#include "tst_imagestalecrop.moc"
