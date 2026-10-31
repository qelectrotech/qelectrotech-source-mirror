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

#include "projectlock.h"

#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <QJsonArray>
#ifndef Q_OS_WIN
#	include <unistd.h>
#endif

/**
	ProjectLock -- the sidecar file saying who has a project open.
*/
class tst_projectlock : public QObject
{
	Q_OBJECT

	private:
		QTemporaryDir m_dir;
		QString       m_project;

		static QJsonObject readJson(const QString &project)
		{
			QFile f(ProjectLock::lockPathFor(project));
			if (!f.open(QIODevice::ReadOnly)) return QJsonObject();
			return QJsonDocument::fromJson(f.readAll()).object();
		}

			//A lock file as another session would have made it
		static void writeLock(const QString &project, const QString &host, qint64 pid,
							  const QDateTime &heartbeat, const QString &user = QStringLiteral("alice"))
		{
			QJsonObject o;
			o["format"] = 1;
			o["session"] = QStringLiteral("other-session");
			o["user"] = user;
			o["host"] = host;
			o["pid"] = static_cast<double>(pid);
			o["app"] = QStringLiteral("QElectroTech test");
			o["since"] = heartbeat.addSecs(-600).toUTC().toString(Qt::ISODate);
			o["heartbeat"] = heartbeat.toUTC().toString(Qt::ISODate);
			QFile f(ProjectLock::lockPathFor(project));
			QVERIFY(f.open(QIODevice::WriteOnly));
			f.write(QJsonDocument(o).toJson());
		}

		static qint64 deadPid()
		{
			QProcess p;
			p.start(QStringLiteral("true"));
			p.waitForFinished();
			return p.processId() ? p.processId() : 999999;
		}

	private slots:
		void initTestCase()
		{
			QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech-tst_projectlock"));
			QCoreApplication::setApplicationName(QStringLiteral("tst_projectlock"));
			QSettings().clear();
			QVERIFY(m_dir.isValid());
		}

		void init()
		{
			m_project = m_dir.filePath(QStringLiteral("plant.qetz"));
			QFile::remove(ProjectLock::lockPathFor(m_project));
			ProjectLock::setHeadless(false);
			ProjectLock::setHeartbeatInterval(60000);
		}

		void sidecarNameAndContent()
		{
			QCOMPARE(QFileInfo(ProjectLock::lockPathFor(m_project)).fileName(),
					 QStringLiteral(".~plant.qetz.lock"));
			ProjectLock lock;
			QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Acquired);
			QVERIFY(lock.isHeld());
			const QJsonObject o = readJson(m_project);
			QCOMPARE(o["format"].toInt(), 1);
			QCOMPARE(o["session"].toString(), lock.session());
			QCOMPARE(o["user"].toString(), ProjectLock::thisUser());
			QCOMPARE(o["host"].toString(), ProjectLock::thisHost());
			QCOMPARE(static_cast<qint64>(o["pid"].toDouble()), QCoreApplication::applicationPid());
			QVERIFY(o["app"].toString().startsWith(QStringLiteral("QElectroTech")));
			QVERIFY(QDateTime::fromString(o["since"].toString(), Qt::ISODate).isValid());
			QVERIFY(QDateTime::fromString(o["heartbeat"].toString(), Qt::ISODate).isValid());
			QCOMPARE(o["scope"].toArray().at(0).toObject()["kind"].toString(), QStringLiteral("project"));
		}

		void removedOnRelease()
		{
			{
				ProjectLock lock;
				QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Acquired);
				QVERIFY(QFileInfo::exists(ProjectLock::lockPathFor(m_project)));
			}
			QVERIFY(!QFileInfo::exists(ProjectLock::lockPathFor(m_project)));
		}

		void secondSessionSeesItHeld()
		{
			ProjectLock first;
			QCOMPARE(first.acquire(m_project), ProjectLock::Result::Acquired);
			ProjectLock second;
			QCOMPARE(second.acquire(m_project), ProjectLock::Result::Held);
			QVERIFY(!second.isHeld());
			QCOMPARE(second.blockingHolder().session, first.session());
			QCOMPARE(second.blockingHolder().user, ProjectLock::thisUser());
				//and it did not disturb the first one's file
			QCOMPARE(readJson(m_project)["session"].toString(), first.session());
				//a released session does not remove another's lock
			second.release();
			QVERIFY(QFileInfo::exists(ProjectLock::lockPathFor(m_project)));
		}

		void takeOverReplacesAndTheOldOneIsTold()
		{
			ProjectLock first;
			QCOMPARE(first.acquire(m_project), ProjectLock::Result::Acquired);
			ProjectLock second;
			QCOMPARE(second.acquire(m_project, true), ProjectLock::Result::Acquired);
			QCOMPARE(readJson(m_project)["session"].toString(), second.session());
			QSignalSpy taken(&first, &ProjectLock::lockTaken);
			QVERIFY(!first.refresh());
			QCOMPARE(taken.count(), 1);
				//the old one must not delete the new one's file
			first.release();
			QCOMPARE(readJson(m_project)["session"].toString(), second.session());
		}

		void staleLockOfDeadProcessOnThisHostIsTaken()
		{
			writeLock(m_project, ProjectLock::thisHost(), deadPid(), QDateTime::currentDateTimeUtc());
			QVERIFY(ProjectLock::isStale(ProjectLock::readHolder(m_project)));
			QVERIFY(!ProjectLock::liveHolder(m_project).valid);
			ProjectLock lock;
			QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Acquired);
			QCOMPARE(readJson(m_project)["session"].toString(), lock.session());
		}

		void liveProcessOnThisHostIsHeldHoweverOld()
		{
			writeLock(m_project, ProjectLock::thisHost(), QCoreApplication::applicationPid(),
					  QDateTime::currentDateTimeUtc().addDays(-3));
			ProjectLock lock;
			QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Held);
		}

		void otherHostIsHeldHoweverOldAndShowsHeartbeat()
		{
			const QDateTime old_beat = QDateTime::currentDateTimeUtc().addSecs(-3 * 3600);
			writeLock(m_project, QStringLiteral("WORKSHOP-PC"), 4120, old_beat);
			ProjectLock lock;
			QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Held);
			QCOMPARE(lock.blockingHolder().host, QStringLiteral("WORKSHOP-PC"));
			QCOMPARE(lock.blockingHolder().user, QStringLiteral("alice"));
			QVERIFY(qAbs(lock.blockingHolder().heartbeat.secsTo(old_beat)) <= 1);
				//the 30 s age that makes QLockFile decide "stale" does not apply
			QVERIFY(ProjectLock::liveHolder(m_project).valid);
		}

		void heartbeatIsRefreshed()
		{
			ProjectLock lock;
			QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Acquired);
			const QDateTime first = ProjectLock::readHolder(m_project).heartbeat;
			const QString since = readJson(m_project)["since"].toString();
			QTest::qWait(1100);
			QVERIFY(lock.refresh());
			QVERIFY(ProjectLock::readHolder(m_project).heartbeat > first);
				//since stays what it was
			QCOMPARE(readJson(m_project)["since"].toString(), since);
		}

		void heartbeatTimerRuns()
		{
			ProjectLock::setHeartbeatInterval(200);
			ProjectLock lock;
			QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Acquired);
			const QDateTime first = ProjectLock::readHolder(m_project).heartbeat;
			QTRY_VERIFY_WITH_TIMEOUT(ProjectLock::readHolder(m_project).heartbeat > first, 4000);
		}

		void refreshMakesAgainAFileTheShareDropped()
		{
			ProjectLock lock;
			QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Acquired);
			QFile::remove(ProjectLock::lockPathFor(m_project));
			QVERIFY(lock.refresh());
			QCOMPARE(readJson(m_project)["session"].toString(), lock.session());
		}

		void readOnlyFolderDoesNotBlock()
		{
#ifdef Q_OS_WIN
			QSKIP("a read-only folder still takes new files on Windows");
#else
			if (geteuid() == 0)
				QSKIP("root can write anywhere");
#endif
			QTemporaryDir ro;
			QVERIFY(ro.isValid());
			const QString project = ro.filePath(QStringLiteral("plant.qet"));
			QVERIFY(QFile::setPermissions(ro.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner));
			ProjectLock lock;
			QCOMPARE(lock.acquire(project), ProjectLock::Result::Unavailable);
			QVERIFY(!lock.isHeld());
			QFile::setPermissions(ro.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
		}

		void moveToFollowsSaveAs()
		{
			const QString other = m_dir.filePath(QStringLiteral("copy.qetz"));
			QFile::remove(ProjectLock::lockPathFor(other));
			ProjectLock lock;
			QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Acquired);
			QVERIFY(lock.moveTo(other));
			QVERIFY(!QFileInfo::exists(ProjectLock::lockPathFor(m_project)));
			QCOMPARE(readJson(other)["session"].toString(), lock.session());
			QCOMPARE(lock.projectPath(), other);
		}

		void moveToRefusedOntoAFileSomeoneElseHas()
		{
			const QString other = m_dir.filePath(QStringLiteral("theirs.qetz"));
			writeLock(other, QStringLiteral("WORKSHOP-PC"), 4120, QDateTime::currentDateTimeUtc());
			QVERIFY(ProjectLock::liveHolder(other, QStringLiteral("my-session")).valid);
			ProjectLock lock;
			QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Acquired);
			QVERIFY(!lock.moveTo(other));
				//still holding the old file
			QVERIFY(lock.isHeld());
			QCOMPARE(readJson(m_project)["session"].toString(), lock.session());
			QCOMPARE(readJson(other)["user"].toString(), QStringLiteral("alice"));
			QFile::remove(ProjectLock::lockPathFor(other));
		}

		void ownLockIsNoConflict()
		{
			ProjectLock lock;
			QCOMPARE(lock.acquire(m_project), ProjectLock::Result::Acquired);
			QVERIFY(!ProjectLock::liveHolder(m_project, lock.session()).valid);
			QVERIFY(ProjectLock::liveHolder(m_project).valid);   // a headless caller has no session
		}

		void settingTurnsItOff()
		{
			QVERIFY(ProjectLock::enabled());
			QVERIFY(ProjectLock::mayTake());
			ProjectLock::setEnabled(false);
			QVERIFY(!ProjectLock::mayTake());
			ProjectLock::setEnabled(true);
			ProjectLock::setHeadless(true);
			QVERIFY(!ProjectLock::mayTake());
		}
};

QTEST_GUILESS_MAIN(tst_projectlock)
#include "tst_projectlock.moc"
