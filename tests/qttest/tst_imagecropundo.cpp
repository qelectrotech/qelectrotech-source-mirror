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

#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSize>
#include <QTemporaryDir>

/**
	Undoing a crop restores the crop rectangle too, not only the pixels
	shown: before, a project saved after Ctrl+Z still recorded the undone
	crop, and the picture came back cropped once reopened. Runs the real
	binary on a script: add a picture, crop it, undo, save; redo, save.

	It also restores whether the pivot was placed by hand: a crop moves
	the pivot to the centre of the kept region and marks it as default,
	and before, Ctrl+Z brought the hand-placed pivot back without its
	mark, so the next save dropped it.
*/
class tst_imagecropundo : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QJsonObject run(const QString &script, const QString &project)
	{
		const QString path = m_dir.filePath(QStringLiteral("probe.js"));
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly)) return {};
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
		if (!proc.waitForFinished(120000)) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput()
											  + proc.readAllStandardError());
		const QString mark = QStringLiteral("PROBE ");
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(mark);
			if (i >= 0)
				return QJsonDocument::fromJson(line.mid(i + mark.size()).toUtf8()).object();
		}
		return {};
	}

	static QByteArray read(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}
	// The size of the picture as shown, from the <image> element's own
	// text (its first text node: a cropped picture carries <image_base>
	// as a child too).
	static QSize shownSize(const QByteArray &xml)
	{
		const QRegularExpression image(QStringLiteral("<image\\b[^>]*>\\s*([A-Za-z0-9+/=]+)"));
		const QRegularExpressionMatch m = image.match(QString::fromUtf8(xml));
		if (!m.hasMatch()) return {};
		return QImage::fromData(QByteArray::fromBase64(m.captured(1).toLatin1())).size();
	}
	// qet.imageCrop()'s {x, y, width, height}, as logged by the script.
	static QRect rect(const QJsonValue &crop)
	{
		const QJsonObject o = crop.toObject();
		return QRect(o.value("x").toInt(-1), o.value("y").toInt(-1),
					 o.value("width").toInt(-1), o.value("height").toInt(-1));
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		QImage img(40, 30, QImage::Format_RGB32);
		img.fill(Qt::darkGreen);
		QVERIFY(img.save(m_dir.filePath(QStringLiteral("pic.png"))));
	}

	void undoneCropIsNotSaved()
	{
		const QString undone = m_dir.filePath(QStringLiteral("undone.qet"));
		const QString redone = m_dir.filePath(QStringLiteral("redone.qet"));
		const QString script = QStringLiteral(R"JS(
var i = qet.addImage(0, '%1', 100, 100);
var r = {full: qet.imageCrop(0, i)};
r.cropped_ok = qet.cropImage(0, i, 10, 5, 20, 10);
r.cropped = qet.imageCrop(0, i);
r.same_ok = qet.cropImage(0, i, 10, 5, 20, 10);
r.empty_ok = qet.cropImage(0, i, 10, 5, 0, 10);
r.outside_ok = qet.cropImage(0, i, 100, 100, 20, 10);
qet.undo();
r.undone = qet.imageCrop(0, i);
qet.save('%2');
qet.redo();
r.redone = qet.imageCrop(0, i);
qet.save('%3');
qet.log('PROBE ' + JSON.stringify(r));
)JS").arg(m_dir.filePath(QStringLiteral("pic.png")), undone, redone);

		const QJsonObject r = run(script, QStringLiteral(QET_EXAMPLES_DIR "/741.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QCOMPARE(rect(r.value("full")), QRect(0, 0, 40, 30));
		QVERIFY(r.value("cropped_ok").toBool());
		QCOMPARE(rect(r.value("cropped")), QRect(10, 5, 20, 10));
		// Crops that change nothing report it, and push no undo step:
		// the undo below still undoes the real crop.
		QCOMPARE(r.value("same_ok").toBool(true), false);
		QCOMPARE(r.value("empty_ok").toBool(true), false);
		QCOMPARE(r.value("outside_ok").toBool(true), false);
		QCOMPARE(rect(r.value("undone")), QRect(0, 0, 40, 30));
		QCOMPARE(rect(r.value("redone")), QRect(10, 5, 20, 10));

		const QByteArray undone_xml = read(undone);
		QVERIFY(undone_xml.contains("<image "));
		QVERIFY2(!undone_xml.contains("<crop "), "an undone crop was saved");
		// The picture shown follows: whole after undo, the kept region after redo.
		QCOMPARE(shownSize(undone_xml), QSize(40, 30));
		QCOMPARE(shownSize(read(redone)), QSize(20, 10));
		const QRegularExpressionMatch crop =
				QRegularExpression(QStringLiteral("<crop ([^>]*)/>")).match(QString::fromUtf8(read(redone)));
		QVERIFY2(crop.hasMatch(), "the redone crop was not saved");
		for (const char *attribute : {R"(x="10")", R"(y="5")", R"(w="20")", R"(h="10")"})
			QVERIFY2(crop.captured(1).contains(QLatin1String(attribute)), attribute);
	}

	void undoneCropKeepsHandPlacedPivot()
	{
		// A picture whose pivot was placed by hand: saved as <transform>
		// with pivotX/pivotY, which no script call writes, so the test
		// adds it to a saved project itself.
		const QString plain = m_dir.filePath(QStringLiteral("plain.qet"));
		const QString pivot = m_dir.filePath(QStringLiteral("pivot.qet"));
		QJsonObject r = run(QStringLiteral(R"JS(
var i = qet.addImage(0, '%1', 100, 100);
qet.save('%2');
qet.log('PROBE ' + JSON.stringify({added: i >= 0}));
)JS").arg(m_dir.filePath(QStringLiteral("pic.png")), plain),
							 QStringLiteral(QET_EXAMPLES_DIR "/741.qet"));
		QVERIFY(r.value("added").toBool());
		QByteArray xml = read(plain);
		QCOMPARE(xml.count("</image>"), 1);
		xml.replace("</image>", "<transform rotation=\"0\" skewX=\"0\" skewY=\"0\" "
								"scaleX=\"1\" scaleY=\"1\" pivotX=\"5\" pivotY=\"7\"/></image>");
		QFile f(pivot);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(xml);
		f.close();

		const QString before = m_dir.filePath(QStringLiteral("before.qet"));
		const QString undone = m_dir.filePath(QStringLiteral("pivot-undone.qet"));
		r = run(QStringLiteral(R"JS(
var r = {};
qet.save('%1');
r.cropped_ok = qet.cropImage(0, 0, 10, 5, 20, 10);
qet.undo();
qet.save('%2');
qet.log('PROBE ' + JSON.stringify(r));
)JS").arg(before, undone), pivot);
		QVERIFY(r.value("cropped_ok").toBool());
		// The fixture is read as intended: saved untouched, it keeps the pivot.
		const QByteArray saved = read(before);
		QVERIFY(saved.contains("pivotX=\"5\"") && saved.contains("pivotY=\"7\""));
		const QByteArray afterUndo = read(undone);
		QVERIFY2(afterUndo.contains("pivotX=\"5\"") && afterUndo.contains("pivotY=\"7\""),
				 "the hand-placed pivot was dropped after undoing the crop");
	}
};

QTEST_GUILESS_MAIN(tst_imagecropundo)
#include "tst_imagecropundo.moc"
