// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include "qettesttimeout.h"

#include "projectorigin.h"

// The corner a project counts its columns and rows from. The names and
// directions are tested on their own; the saved setting through the real
// binary: --resave keeps it, and adds nothing to a project that never had
// it, so existing projects save exactly as before.
class tst_projectorigin : public QObject
{
	Q_OBJECT

	using Mode = ProjectOrigin::Mode;
	QTemporaryDir m_dir;
	int m_run = 0;

		// Runs --resave on @p in, returns the saved file's text (empty on failure)
	QString resave(const QString &in)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run));
		const QString tmp = m_dir.filePath(QStringLiteral("tmp%1").arg(m_run));
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run++));
		QDir().mkpath(home);
		QDir().mkpath(tmp);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/.config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/.local/share"));
		env.insert(QStringLiteral("TMPDIR"), tmp);

		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), in, out});
		if (!proc.waitForFinished(QetTest::timeout(120000)) || proc.exitCode() != 0)
			return {};
		QFile file(out);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return {};
		return QString::fromUtf8(file.readAll());
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
	}

	void modeNames()
	{
		for (Mode mode : {Mode::TopLeft, Mode::TopRight, Mode::BottomLeft, Mode::BottomRight})
			QCOMPARE(int(ProjectOrigin::fromString(ProjectOrigin::toString(mode))), int(mode));
			// An unknown or missing value is the standard order
		QCOMPARE(int(ProjectOrigin::fromString(QStringLiteral("centre"))), int(Mode::TopLeft));
		QCOMPARE(int(ProjectOrigin::fromString(QString())), int(Mode::TopLeft));
	}

	void directions()
	{
		QVERIFY(!ProjectOrigin::reversedIsColumns(Mode::TopLeft));
		QVERIFY(!ProjectOrigin::reversedIsRows(Mode::TopLeft));
		QVERIFY(ProjectOrigin::reversedIsColumns(Mode::TopRight));
		QVERIFY(!ProjectOrigin::reversedIsRows(Mode::TopRight));
		QVERIFY(!ProjectOrigin::reversedIsColumns(Mode::BottomLeft));
		QVERIFY(ProjectOrigin::reversedIsRows(Mode::BottomLeft));
		QVERIFY(ProjectOrigin::reversedIsColumns(Mode::BottomRight));
		QVERIFY(ProjectOrigin::reversedIsRows(Mode::BottomRight));
	}

	void savedSettingSurvivesResave()
	{
		const QString fixture = QFINDTESTDATA("fixtures/wiring_list_arrows.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");

			// The fixture has no setting: a resave must not add one
		const QString plain = resave(fixture);
		QVERIFY2(!plain.isEmpty(), "--resave failed");
		QVERIFY(!plain.contains(QLatin1String("corner=")));

			// With the setting, a resave keeps it
		QFile source(fixture);
		QVERIFY(source.open(QIODevice::ReadOnly | QIODevice::Text));
		QString text = QString::fromUtf8(source.readAll());
		const int newdiagrams = text.indexOf(QLatin1String("<newdiagrams"));
		QVERIFY(newdiagrams > 0);
		text.insert(newdiagrams, QStringLiteral("<cell_origin corner=\"bottom-right\"/>\n    "));
		const QString with_setting = m_dir.filePath(QStringLiteral("with_setting.qet"));
		QFile out(with_setting);
		QVERIFY(out.open(QIODevice::WriteOnly | QIODevice::Text));
		out.write(text.toUtf8());
		out.close();

		const QString saved = resave(with_setting);
		QVERIFY2(!saved.isEmpty(), "--resave failed");
		QVERIFY2(saved.contains(QLatin1String("<cell_origin corner=\"bottom-right\"/>")),
				 "the setting was lost on save");
	}
};

QTEST_GUILESS_MAIN(tst_projectorigin)

#include "tst_projectorigin.moc"
