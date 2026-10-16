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

#ifndef PROJECTLOCK_H
#define PROJECTLOCK_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>

/**
	@brief The ProjectLock class
	A sidecar file beside a project that says who has it open, so that a second
	person (or a second QElectroTech) opening the same file on a shared drive
	is told, instead of both editing and the last save wiping the other's work.

	The file is ".~<filename>.lock" in the project's folder, a small versioned
	JSON object. It is not the project file itself: a project is saved by
	writing a temporary file and renaming it over the old one, which would
	replace a lock kept in the project at every save.

	Taking the lock is an exclusive create (QFile NewOnly: O_EXCL, or
	CREATE_NEW on Windows), the one operation shared drives do reliably. This is
	not a QLockFile: its stale time defaults to 30 s, and for a lock made on
	another machine it decides by age alone, so another PC would take a lock
	held for over 30 s. Here a lock is stale only when it was made on this
	machine by a process that is gone; any other lock is shown to the user, with
	the age of its heartbeat, to judge.
*/
class ProjectLock : public QObject
{
	Q_OBJECT

	public:
			/// What a lock file says
		struct Holder
		{
			bool      valid = false;  ///< false: the file could not be read as a lock
			QString   session;
			QString   user;
			QString   host;
			qint64    pid = 0;
			QString   app;
			QDateTime since;
			QDateTime heartbeat;
		};

		enum class Result {
			Acquired,     ///< the lock is ours
			Held,         ///< a live lock of another session is there
			Unavailable   ///< the lock file cannot be made (read-only folder, ...)
		};

		explicit ProjectLock(QObject *parent = nullptr);
		~ProjectLock() override;

		Result acquire(const QString &project_path, bool take_over = false);
		bool   moveTo(const QString &project_path);
		void   release();
		bool   refresh();

		bool    isHeld() const {return m_held;}
		QString projectPath() const {return m_project_path;}
		QString session() const {return m_session;}
		Holder  blockingHolder() const {return m_blocking;}

		static QString lockPathFor(const QString &project_path);
		static Holder  readHolder(const QString &project_path);
		static bool    isStale(const Holder &holder);
		static Holder  liveHolder(const QString &project_path, const QString &own_session = QString());

		static bool enabled();
		static void setEnabled(bool);
		static bool mayTake();
		static void setHeadless(bool);
		static void setHeartbeatInterval(int msec);

		static QString thisHost();
		static QString thisUser();

	signals:
			/// Another session took the lock over (its "Open anyway")
		void lockTaken(const ProjectLock::Holder &by);

	private:
		QByteArray content(const QDateTime &since, const QDateTime &heartbeat) const;
		bool       createExclusive(const QString &lock_path);
		static void hide(const QString &lock_path);

		QString   m_project_path;
		QString   m_session;
		QDateTime m_since;
		bool      m_held = false;
		Holder    m_blocking;
		QTimer    m_timer;
};

Q_DECLARE_METATYPE(ProjectLock::Holder)

#endif
