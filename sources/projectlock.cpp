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

#include "qetversion.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QSysInfo>
#include <QUuid>

#ifdef Q_OS_WIN
#	include <windows.h>
#else
#	include <cerrno>
#	include <signal.h>
#endif

namespace {
	bool g_headless = false;
	int  g_heartbeat_ms = 60000;
		//A lock file that cannot be read yet may be one just created and
		//not written: it counts as held for this long
	constexpr int unreadable_grace_s = 10;

	bool processAlive(qint64 pid)
	{
		if (pid <= 0) return false;
#ifdef Q_OS_WIN
		HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
		if (!h) return GetLastError() == ERROR_ACCESS_DENIED;
		DWORD code = 0;
		const bool alive = GetExitCodeProcess(h, &code) && code == STILL_ACTIVE;
		CloseHandle(h);
		return alive;
#else
		return ::kill(static_cast<pid_t>(pid), 0) == 0 || errno == EPERM;
#endif
	}

	QString isoUtc(const QDateTime &t)
	{
		return t.toUTC().toString(Qt::ISODate);
	}
}

/**
	@brief ProjectLock::ProjectLock
	@param parent
*/
ProjectLock::ProjectLock(QObject *parent) :
	QObject(parent),
	m_session(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
	m_timer.setInterval(g_heartbeat_ms);
	connect(&m_timer, &QTimer::timeout, this, &ProjectLock::refresh);
}

/**
	@brief ProjectLock::~ProjectLock
	Deletes the lock file if it is still ours.
*/
ProjectLock::~ProjectLock()
{
	release();
}

/**
	@brief ProjectLock::lockPathFor
	@return the lock file of the project @p project_path:
	".~plant.qetz" gives ".~plant.qetz.lock" in the same folder
*/
QString ProjectLock::lockPathFor(const QString &project_path)
{
	const QFileInfo info(project_path);
	return info.absoluteDir().filePath(QStringLiteral(".~") + info.fileName() + QStringLiteral(".lock"));
}

/**
	@brief ProjectLock::enabled
	@return false when the user turned locking off (Settings, General,
	Projects); on by default
*/
bool ProjectLock::enabled()
{
	QSettings settings;
	return settings.value(QStringLiteral("projectlock/enabled"), true).toBool();
}

void ProjectLock::setEnabled(bool enabled)
{
	QSettings settings;
	settings.setValue(QStringLiteral("projectlock/enabled"), enabled);
}

/**
	@brief ProjectLock::mayTake
	@return true when this process should take locks: locking is on and this
	is not a headless run (those only read locks, to refuse a write)
*/
bool ProjectLock::mayTake()
{
	return !g_headless && enabled();
}

void ProjectLock::setHeadless(bool headless)
{
	g_headless = headless;
}

/**
	@brief ProjectLock::setHeartbeatInterval
	For tests. Applies to locks made afterwards.
*/
void ProjectLock::setHeartbeatInterval(int msec)
{
	g_heartbeat_ms = msec;
}

QString ProjectLock::thisHost()
{
	return QSysInfo::machineHostName();
}

QString ProjectLock::thisUser()
{
	for (const char *name : {"USER", "USERNAME", "LOGNAME"}) {
		const QString user = qEnvironmentVariable(name);
		if (!user.isEmpty()) return user;
	}
	return QStringLiteral("unknown");
}

/**
	@brief ProjectLock::readHolder
	@return what the lock file of @p project_path says; not valid when there
	is none or it is not a lock
*/
ProjectLock::Holder ProjectLock::readHolder(const QString &project_path)
{
	Holder holder;
	QFile file(lockPathFor(project_path));
	if (!file.open(QIODevice::ReadOnly)) return holder;
	const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	if (!doc.isObject()) return holder;
	const QJsonObject o = doc.object();
	if (o.value(QStringLiteral("format")).toInt() < 1 || !o.contains(QStringLiteral("session"))) return holder;
	holder.session   = o.value(QStringLiteral("session")).toString();
	holder.user      = o.value(QStringLiteral("user")).toString();
	holder.host      = o.value(QStringLiteral("host")).toString();
	holder.pid       = static_cast<qint64>(o.value(QStringLiteral("pid")).toDouble());
	holder.app       = o.value(QStringLiteral("app")).toString();
	holder.since     = QDateTime::fromString(o.value(QStringLiteral("since")).toString(), Qt::ISODate);
	holder.heartbeat = QDateTime::fromString(o.value(QStringLiteral("heartbeat")).toString(), Qt::ISODate);
	holder.valid     = !holder.session.isEmpty();
	return holder;
}

/**
	@brief ProjectLock::isStale
	A lock is known to be dead only when it was made on this machine by a
	process that no longer exists. Of a lock from another machine nothing is
	known, whatever its age.
*/
bool ProjectLock::isStale(const Holder &holder)
{
	return holder.valid
			&& holder.host.compare(thisHost(), Qt::CaseInsensitive) == 0
			&& !processAlive(holder.pid);
}

/**
	@brief ProjectLock::liveHolder
	@param own_session the session of the caller, whose lock is not a conflict
	@return who holds @p project_path now: not valid when nobody does, the
	lock is stale or it is @p own_session's. A lock file that cannot be read
	and is younger than a few seconds is a lock being written: held, by an
	unknown holder.
*/
ProjectLock::Holder ProjectLock::liveHolder(const QString &project_path, const QString &own_session)
{
	const QString lock_path = lockPathFor(project_path);
	if (!QFileInfo::exists(lock_path)) return Holder();
	Holder holder = readHolder(project_path);
	if (!holder.valid) {
		const QDateTime modified = QFileInfo(lock_path).lastModified();
		if (modified.secsTo(QDateTime::currentDateTime()) < unreadable_grace_s) {
			holder.valid = true;
			holder.session = QStringLiteral("?");
			holder.user = holder.host = QStringLiteral("?");
			holder.since = holder.heartbeat = modified;
			return holder;
		}
		return Holder();
	}
	if (!own_session.isEmpty() && holder.session == own_session) return Holder();
	if (isStale(holder)) return Holder();
	return holder;
}

QByteArray ProjectLock::content(const QDateTime &since, const QDateTime &heartbeat) const
{
	QJsonObject scope;
	scope.insert(QStringLiteral("kind"), QStringLiteral("project"));
	QJsonObject o;
	o.insert(QStringLiteral("format"), 1);
	o.insert(QStringLiteral("session"), m_session);
	o.insert(QStringLiteral("user"), thisUser());
	o.insert(QStringLiteral("host"), thisHost());
	o.insert(QStringLiteral("pid"), static_cast<double>(QCoreApplication::applicationPid()));
	o.insert(QStringLiteral("app"), QStringLiteral("QElectroTech %1").arg(QetVersion::currentVersion().toString()));
	o.insert(QStringLiteral("since"), isoUtc(since));
	o.insert(QStringLiteral("heartbeat"), isoUtc(heartbeat));
	o.insert(QStringLiteral("scope"), QJsonArray{scope});
	return QJsonDocument(o).toJson(QJsonDocument::Indented);
}

/**
	@brief ProjectLock::createExclusive
	Make the lock file, failing if it exists. NewOnly is O_EXCL / CREATE_NEW:
	the check and the creation are one step on the file system, so two
	sessions cannot both succeed, also over SMB.
*/
bool ProjectLock::createExclusive(const QString &lock_path)
{
	QFile file(lock_path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return false;
	const QByteArray data = content(m_since, QDateTime::currentDateTimeUtc());
	const bool ok = file.write(data) == data.size();
	file.close();
	if (!ok) QFile::remove(lock_path);
	return ok;
}

void ProjectLock::hide(const QString &lock_path)
{
#ifdef Q_OS_WIN
	const std::wstring path = QDir::toNativeSeparators(lock_path).toStdWString();
	SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
#else
	Q_UNUSED(lock_path)
#endif
}

/**
	@brief ProjectLock::acquire
	Take the lock of @p project_path.
	@param take_over take it although another live session has it: its
	lock file is replaced, and that session learns of it at its next heartbeat
	@return Acquired; Held, with blockingHolder() saying by whom; or
	Unavailable when the lock file cannot be made (read-only folder):
	the caller opens the project as without locking
*/
ProjectLock::Result ProjectLock::acquire(const QString &project_path, bool take_over)
{
	release();
	m_blocking = Holder();
	const QString lock_path = lockPathFor(project_path);
	m_since = QDateTime::currentDateTimeUtc();

	for (int attempt = 0; attempt < 3; ++attempt)
	{
		if (createExclusive(lock_path)) {
			// Read back: if two sessions removed a stale lock and made
			// theirs at once, only the survivor's session is in the file
			if (readHolder(project_path).session != m_session) continue;
			m_project_path = project_path;
			m_held = true;
			hide(lock_path);
			m_timer.setInterval(g_heartbeat_ms);
			m_timer.start();
			return Result::Acquired;
		}
		if (!QFileInfo::exists(lock_path)) {
				//Not there and cannot be made
			if (!QFileInfo(QFileInfo(lock_path).absolutePath()).isWritable())
				return Result::Unavailable;
			continue;
		}
		const Holder holder = liveHolder(project_path);
		if (holder.valid && !take_over) {
			m_blocking = holder;
			return Result::Held;
		}
			//Stale, unreadable for good, or taken over: replace it
		if (!QFile::remove(lock_path) && QFileInfo::exists(lock_path))
			return Result::Unavailable;
	}
	return Result::Unavailable;
}

/**
	@brief ProjectLock::moveTo
	The project is now @p project_path (Save As, .qet saved as .qetz): take
	the new lock, then let the old one go. Nothing changes when the new one
	cannot be had.
	@return true when the lock is now on @p project_path
*/
bool ProjectLock::moveTo(const QString &project_path)
{
	if (m_held && QFileInfo(project_path).absoluteFilePath() == QFileInfo(m_project_path).absoluteFilePath()) return true;
	const QString old_path = m_project_path;
	const bool was_held = m_held;
	m_held = false;                       // keep release() from touching the old file
	m_timer.stop();
	const Result result = acquire(project_path);
	if (result == Result::Acquired) {
		if (was_held) {
			const QString old_lock = lockPathFor(old_path);
			if (readHolder(old_path).session == m_session) QFile::remove(old_lock);
		}
		return true;
	}
	if (was_held) {                       // go back to the old one
		m_project_path = old_path;
		m_held = true;
		m_timer.start();
	}
	return false;
}

/**
	@brief ProjectLock::release
	Delete the lock file, if it is ours: a lock another session took over
	is left to it.
*/
void ProjectLock::release()
{
	m_timer.stop();
	if (!m_held) return;
	m_held = false;
	if (readHolder(m_project_path).session == m_session)
		QFile::remove(lockPathFor(m_project_path));
}

/**
	@brief ProjectLock::refresh
	Write a fresh heartbeat. Called by the timer, every minute.
	If the file is gone (a share dropped it) it is made again; if another
	session's lock is there, ours was taken over: lockTaken() is emitted and
	the lock is let go.
	@return true when the lock is still ours
*/
bool ProjectLock::refresh()
{
	if (!m_held) return false;
	const QString lock_path = lockPathFor(m_project_path);
	if (QFileInfo::exists(lock_path)) {
		const Holder holder = readHolder(m_project_path);
		if (holder.valid && holder.session != m_session) {
			m_held = false;
			m_timer.stop();
			emit lockTaken(holder);
			return false;
		}
		QFile file(lock_path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return true;
		file.write(content(m_since, QDateTime::currentDateTimeUtc()));
		return true;
	}
	if (createExclusive(lock_path)) hide(lock_path);
	return true;
}
