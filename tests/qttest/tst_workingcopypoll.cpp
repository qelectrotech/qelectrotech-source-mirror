// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "qetapp.h"
#include "qetproject.h"
#include "dataBase/projectdatabase.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QTemporaryDir>

// The working copy in the editor (QET-SQL-FORMAT S3z2): a project opened
// with backups on has one in the data folder; a change made outside the
// undo stack reaches it within 2 s (the 1 s poll); closing the project
// deletes it.
class tst_workingcopypoll : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	// The project's title as the working copy, copied as a crash would
	// leave it, gives it back
	QString titleInCopy(const QString &working_copy, int n)
	{
		const QString dir = m_dir.filePath(QStringLiteral("copy%1").arg(n));
		QDir().mkpath(dir);
		const QString copy = dir + QStringLiteral("/copy.sqlite");
		if (!QFile::copy(working_copy, copy)) return QStringLiteral("no copy");
		if (QFile::exists(working_copy + QStringLiteral("-wal")))
			QFile::copy(working_copy + QStringLiteral("-wal"), copy + QStringLiteral("-wal"));
		const QString written = dir + QStringLiteral("/written.qet");
		QString error;
		if (!projectDataBase::writeWorkingCopy(copy, written, &error)) return error;
		QFile file(written);
		QDomDocument document;
		if (!file.open(QIODevice::ReadOnly) || !document.setContent(&file)) return QStringLiteral("no project");
		return document.documentElement().attribute(QStringLiteral("title"));
	}

private slots:
	void changeOutsideUndoStackIsCommitted()
	{
		auto *project = new QETProject(QStringLiteral(QET_EXAMPLES_DIR "/tremie_vibrante.qet"));
		QCOMPARE(project->state(), QETProject::Ok);
		projectDataBase *db = project->dataBase();
		QVERIFY(db->hasWorkingCopy());
		const QString working_copy = db->workingCopyPath();
		QVERIFY2(working_copy.startsWith(QETApp::dataDir() + QStringLiteral("/working-copies/")),
				 qPrintable(working_copy));
		QVERIFY(QFile::exists(working_copy));

		const QString title = titleInCopy(working_copy, 0);
		project->setTitle(QStringLiteral("Changed outside the undo stack"));
			//No event loop has run: nothing committed it yet
		QCOMPARE(titleInCopy(working_copy, 1), title);
		QTest::qWait(2000);
		QCOMPARE(titleInCopy(working_copy, 2), QStringLiteral("Changed outside the undo stack"));

		const QString base = working_copy.chopped(QStringLiteral(".sqlite").size());
		delete project;
		for (const QString &suffix : {QStringLiteral(".sqlite"), QStringLiteral(".sqlite-wal"),
									  QStringLiteral(".lock")})
			QVERIFY2(!QFile::exists(base + suffix), qPrintable(base + suffix));
	}
};

QET_APP_TEST_MAIN(tst_workingcopypoll)
#include "tst_workingcopypoll.moc"
