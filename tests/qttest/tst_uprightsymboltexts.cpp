// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>

// A project can keep the texts drawn in its turned symbols horizontal
// (<symbol_texts upright="true"/>, Project properties). Through the real
// binary on tremie_vibrante.qet: the symbols of the first folio are turned
// by 90 degrees, then the folio is exported to DXF with the setting off and
// on. The "M" of each motor turns with its symbol when it is off, and stays
// horizontal when it is on.
class tst_uprightsymboltexts : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QProcessEnvironment environment()
	{
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		return env;
	}

	bool runQet(const QStringList &arguments)
	{
		QProcess proc;
		proc.setProcessEnvironment(environment());
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), arguments);
		return proc.waitForFinished(120000) && proc.exitCode() == 0;
	}

	// The rotation (DXF group code 50) of every TEXT entity of @p dxf whose
	// text (group code 1) is @p text
	static QList<double> textRotations(const QString &dxf, const QString &text)
	{
		QFile file(dxf);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
		const QStringList lines = QString::fromUtf8(file.readAll())
				.split(QLatin1Char('\n'));
		QList<double> rotations;
		bool in_text = false, match = false;
		double rotation = 0;
		for (int i = 0; i + 1 < lines.size(); i += 2) {
			const QString code = lines.at(i).trimmed();
			const QString value = lines.at(i + 1).trimmed();
			if (code == QLatin1String("0")) {
				if (in_text && match) rotations << rotation;
				in_text = value == QLatin1String("TEXT");
				match = false;
				rotation = 0;
			} else if (in_text && code == QLatin1String("1")) {
				match = value == text;
			} else if (in_text && code == QLatin1String("50")) {
				rotation = value.toDouble();
			}
		}
		return rotations;
	}

	// The DXF of the first folio of @p project, exported into @p name
	QString exportDxf(const QString &project, const QString &name)
	{
		const QString out = m_dir.filePath(name);
		QDir().mkpath(out);
		if (!runQet({QStringLiteral("--export-dxf"), project, out})) return {};
		const QStringList files = QDir(out).entryList({QStringLiteral("01*.dxf")});
		return files.isEmpty() ? QString() : QDir(out).filePath(files.first());
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void motorTextStaysHorizontal()
	{
			//The symbols of the first folio, turned by 90 degrees
		const QString off = m_dir.filePath(QStringLiteral("off.qet"));
		const QString script = m_dir.filePath(QStringLiteral("turn.js"));
		QFile js(script);
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write(QStringLiteral(
			"var els = qet.elementUuids(0);\n"
			"for (var e = 0; e < els.length; e++) qet.rotateElement(0, els[e], 90);\n"
			"qet.save(%1);\n").arg(QLatin1Char('"') + off + QLatin1Char('"')).toUtf8());
		js.close();
		QVERIFY(runQet({QStringLiteral("--run"), script,
						QStringLiteral(QET_EXAMPLES_DIR "/tremie_vibrante.qet")}));

			//An existing project is read with the setting off, and saved so
		QFile off_file(off);
		QVERIFY(off_file.open(QIODevice::ReadOnly));
		QString xml = QString::fromUtf8(off_file.readAll());
		off_file.close();
		QVERIFY(!xml.contains(QStringLiteral("symbol_texts")));

			//The same project with the setting on
		const QString on = m_dir.filePath(QStringLiteral("on.qet"));
		const int root_end = xml.indexOf(QLatin1Char('>'), xml.indexOf(QStringLiteral("<project"))) + 1;
		QVERIFY(root_end > 0);
		xml.insert(root_end, QStringLiteral("\n<symbol_texts upright=\"true\"/>"));
		QFile on_file(on);
		QVERIFY(on_file.open(QIODevice::WriteOnly));
		on_file.write(xml.toUtf8());
		on_file.close();

		const QList<double> turned = textRotations(exportDxf(off, QStringLiteral("dxf-off")),
												   QStringLiteral("M"));
		const QList<double> upright = textRotations(exportDxf(on, QStringLiteral("dxf-on")),
													QStringLiteral("M"));
			//The two motors of the folio
		QCOMPARE(turned.size(), 2);
		QCOMPARE(upright.size(), 2);
		for (double rotation : turned)
			QCOMPARE(std::fmod(rotation, 360.0), 270.0);
		for (double rotation : upright)
			QCOMPARE(std::fmod(rotation, 360.0), 0.0);
	}
};

QTEST_APPLESS_MAIN(tst_uprightsymboltexts)

#include "tst_uprightsymboltexts.moc"
