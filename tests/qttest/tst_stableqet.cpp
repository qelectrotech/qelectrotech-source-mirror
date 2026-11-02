// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>
#include "qettesttimeout.h"

/**
	A .qet kept in git changes only where the project did (scorpio810 on
	#1512; QET-SQL-FORMAT-SCOPE.md 7.4):
	- an unchanged project saved in another language, in another time
	  zone or from another folder is the same file, byte for byte;
	- one symbol moved changes that symbol's line, the wires drawn again
	  and the saved date, time and path, and no other line.
	Runs the real binary on fixtures/every_feature.qet, a project using
	every feature QElectroTech saves.
*/
class tst_stableqet : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QString m_base;

	QString run(const QStringList &arguments, const QMap<QString, QString> &extra = {})
	{
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
			//Its own settings and single-instance socket: another test's
			//QElectroTech running at the same time would otherwise be
			//handed this run (SingleApplication)
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		for (auto it = extra.constBegin() ; it != extra.constEnd() ; ++it)
			env.insert(it.key(), it.value());
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.setProcessChannelMode(QProcess::MergedChannels);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), arguments);
		if (!proc.waitForFinished(QetTest::timeout(120000)) || proc.exitStatus() != QProcess::NormalExit
				|| proc.exitCode() != 0)
			return QString();
		return QString::fromUtf8(proc.readAll()) + QStringLiteral("\nEXIT 0");
	}

	QString script(const QString &name, const QString &body)
	{
		const QString path = m_dir.filePath(name);
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly)) return QString();
		file.write(body.toUtf8());
		return path;
	}

	static QByteArray bytes(const QString &path)
	{
		QFile file(path);
		return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
	}

		/// The lines of @p xml a move of the symbol @p uuid may change: its
		/// own, the wires' (each <conductor> and what it holds) and the
		/// saved date, time and path
	static QStringList unexpected(const QString &xml, const QString &uuid, const QSet<QString> &others)
	{
		static const QRegularExpression stamp(QStringLiteral(
			"<property [^>]*name=\"saved(filename|filepath|filenamedir|date|date-us|date-eu|time)\""));
		QStringList out;
		bool in_wire = false;
		for (const QString &line : xml.split(QLatin1Char('\n'))) {
			const QString text = line.trimmed();
			const bool wire_starts = text.startsWith(QLatin1String("<conductor "));
			if (wire_starts) in_wire = !text.endsWith(QLatin1String("/>"));
			const bool allowed = wire_starts || in_wire
					|| (text.startsWith(QLatin1String("<element ")) && text.contains(uuid))
					|| stamp.match(text).hasMatch();
			if (text == QLatin1String("</conductor>")) in_wire = false;
			if (!allowed && !others.contains(line)) out << text.left(160);
		}
		return out;
	}

private slots:
	void initTestCase()
	{
		QVERIFY2(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)), "qelectrotech binary not found");
		QVERIFY(m_dir.isValid());
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");
			//The fixture as QElectroTech writes it, its stamps as they were:
			//a copy written by --resave is not a save of the project
		m_base = m_dir.filePath(QStringLiteral("base.qet"));
		QVERIFY(!run({QStringLiteral("--resave"), fixture, m_base}).isEmpty());
	}

	void unchangedSaveIsIdentical_data()
	{
		QTest::addColumn<QString>("folder");
		QTest::addColumn<QString>("language");
		QTest::addColumn<QString>("zone");
		QTest::newRow("same folder") << QString() << QString() << QString();
		QTest::newRow("another language") << QString() << QStringLiteral("fr_FR.UTF-8") << QString();
		QTest::newRow("another time zone") << QString() << QString() << QStringLiteral("Pacific/Chatham");
		QTest::newRow("another folder") << QStringLiteral("elsewhere/deeper") << QString() << QString();
	}
	void unchangedSaveIsIdentical()
	{
		QFETCH(QString, folder);
		QFETCH(QString, language);
		QFETCH(QString, zone);
		const QString dir = m_dir.filePath(folder.isEmpty() ? QStringLiteral("unchanged") : folder);
		QVERIFY(QDir().mkpath(dir));
		const QString project = dir + QStringLiteral("/project.qet");
		QFile::remove(project);
		QVERIFY(QFile::copy(m_base, project));
		QMap<QString, QString> env;
		if (!language.isEmpty()) {
			env.insert(QStringLiteral("LANG"), language);
			env.insert(QStringLiteral("LC_ALL"), language);
			env.insert(QStringLiteral("LANGUAGE"), language.left(2));
		}
		if (!zone.isEmpty()) env.insert(QStringLiteral("TZ"), zone);
		const QString js = script(QStringLiteral("unchanged.js"),
			QStringLiteral("qet.save('');\nqet.log('SAVED');\n"));
		QVERIFY(run({QStringLiteral("--run"), js, project}, env).contains(QLatin1String("SAVED")));
		QCOMPARE(bytes(project), bytes(m_base));
	}

	void movedSymbolChangesItsLinesOnly()
	{
		const QString project = m_dir.filePath(QStringLiteral("moved.qet"));
		QVERIFY(QFile::copy(m_base, project));
		const QString js = script(QStringLiteral("moved.js"), QStringLiteral(
			"var u = qet.elementUuids(0)[0];\n"
			"qet.log('MOVED ' + u + ' ' + qet.moveElement(0, u, 10, 0));\n"
			"qet.save('');\n"));
		const QString out = run({QStringLiteral("--run"), js, project});
		const QRegularExpression moved(QStringLiteral("MOVED (\\{[0-9a-f-]+\\}) true"));
		const QRegularExpressionMatch match = moved.match(out);
		QVERIFY2(match.hasMatch(), qPrintable(out.right(400)));
		const QString uuid = match.captured(1);

		const QString before = QString::fromUtf8(bytes(m_base));
		const QString after = QString::fromUtf8(bytes(project));
		QVERIFY2(before != after, "moving a symbol changed nothing");
		const QStringList before_lines = before.split(QLatin1Char('\n'));
		const QStringList after_lines = after.split(QLatin1Char('\n'));
			//The lines the other file has too are not changes
		const QSet<QString> in_before(before_lines.cbegin(), before_lines.cend());
		const QSet<QString> in_after(after_lines.cbegin(), after_lines.cend());
		const QStringList added = unexpected(after, uuid, in_before);
		const QStringList removed = unexpected(before, uuid, in_after);
		QVERIFY2(added.isEmpty() && removed.isEmpty(),
				 qPrintable(QStringLiteral("other lines changed: +%1 -%2")
							.arg(added.join(QStringLiteral(" | ")), removed.join(QStringLiteral(" | ")))));
		QVERIFY2(after.contains(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"))),
				 "the changed project was not stamped with today's date");
	}
};

QTEST_GUILESS_MAIN(tst_stableqet)
#include "tst_stableqet.moc"
