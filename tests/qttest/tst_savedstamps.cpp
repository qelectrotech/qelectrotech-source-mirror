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
#include <QDate>
#include <QDomDocument>
#include <QProcess>
#include <QTemporaryDir>

/**
	The title block variables of a save (saveddate, savedtime,
	savedfilename, savedfilepath...) are set when the project is saved
	with something new in it, and go into the file that save writes. They
	used to be set after writing, so each save wrote the time of the one
	before: a project saved twice without a change gave two different
	files, and a changed project was saved with the previous date.

	Runs the real binary on fixtures/unlinked_contact_label.qet, whose
	stamps say 2026-10-02 18:39 in a temporary folder:
	- saved twice in its own file without a change: byte-identical;
	- one symbol moved, then saved: today's date and the file's own path.
*/
class tst_savedstamps : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QString m_base;

	QString run(const QStringList &arguments)
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
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.setProcessChannelMode(QProcess::MergedChannels);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), arguments);
		if (!proc.waitForFinished(120000) || proc.exitStatus() != QProcess::NormalExit
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

		/// The saved-* properties of @p qetz, read back through --resave
	QMap<QString, QString> stamps(const QString &qetz)
	{
		const QString xml = qetz + QStringLiteral(".qet");
		if (run({QStringLiteral("--resave"), qetz, xml}).isEmpty()) return {};
		QFile file(xml);
		QDomDocument doc;
		if (!file.open(QIODevice::ReadOnly) || !doc.setContent(&file)) return {};
		QMap<QString, QString> out;
		const QDomNodeList props = doc.documentElement().firstChildElement(
					QStringLiteral("properties")).elementsByTagName(QStringLiteral("property"));
		for (int i = 0; i < props.size(); ++i) {
			const QDomElement p = props.at(i).toElement();
			if (p.attribute(QStringLiteral("name")).startsWith(QLatin1String("saved")))
				out.insert(p.attribute(QStringLiteral("name")), p.text());
		}
		return out;
	}

	static QByteArray bytes(const QString &path)
	{
		QFile file(path);
		return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
	}

private slots:
	void initTestCase()
	{
		QVERIFY2(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)), "qelectrotech binary not found");
		QVERIFY(m_dir.isValid());
		const QString fixture = QFINDTESTDATA("fixtures/unlinked_contact_label.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");
			//The fixture as a .qetz, its stamps as they were: a copy
			//written by --resave is not a save of the project
		m_base = m_dir.filePath(QStringLiteral("base.qetz"));
		QVERIFY(!run({QStringLiteral("--resave"), fixture, m_base}).isEmpty());
		QCOMPARE(stamps(m_base).value(QStringLiteral("saveddate-us")), QStringLiteral("2026-10-02"));
	}

	void unchangedSavesAreIdentical()
	{
		const QString project = m_dir.filePath(QStringLiteral("unchanged.qetz"));
		QVERIFY(QFile::copy(m_base, project));
		const QString js = script(QStringLiteral("unchanged.js"),
			QStringLiteral("qet.save('');\nqet.save('');\nqet.log('SAVED twice');\n"));
		QVERIFY(run({QStringLiteral("--run"), js, project}).contains(QLatin1String("SAVED twice")));
		QCOMPARE(bytes(project), bytes(m_base));
	}

	void changedSaveIsStampedNow()
	{
		const QString project = m_dir.filePath(QStringLiteral("changed.qetz"));
		QVERIFY(QFile::copy(m_base, project));
		const QString js = script(QStringLiteral("changed.js"), QStringLiteral(
			"var u = qet.elementUuids(0)[0];\n"
			"qet.log('MOVED ' + qet.moveElement(0, u, 10, 0));\n"
			"qet.save('');\n"));
		QVERIFY(run({QStringLiteral("--run"), js, project}).contains(QLatin1String("MOVED true")));
		const QMap<QString, QString> s = stamps(project);
		QCOMPARE(s.value(QStringLiteral("saveddate-us")), QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
		QCOMPARE(s.value(QStringLiteral("savedfilepath")), QFileInfo(project).absoluteFilePath());
		QCOMPARE(s.value(QStringLiteral("savedfilename")), QStringLiteral("changed"));
	}
};

QTEST_GUILESS_MAIN(tst_savedstamps)
#include "tst_savedstamps.moc"
