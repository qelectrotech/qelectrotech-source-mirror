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
#include "qetlogger.h"

#include "crashhandler.h"
#include "../qetapp.h"
#include "../qetversion.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSysInfo>
#include <cctype>
#include <cstdio>

namespace {

/**
	@brief legacyStderrOutput
	The QET_LOG_DISABLE=1 escape hatch. Deliberately independent of
	every other function in this file -- including sanitize()/
	formatLine(), which are exactly the new code a problem might be in
	-- so this path stays usable even if the rest of the rework
	misbehaves. No ring, no file, no rotation, no mutex.
*/
void legacyStderrOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	const QByteArray local_msg = msg.toLocal8Bit();
	const char *file = context.file ? context.file : "";
	const char *function = context.function ? context.function : "";

	const char *level = "Unknown";
	switch (type) {
		case QtDebugMsg:    level = "Debug";    break;
		case QtInfoMsg:     level = "Info";     break;
		case QtWarningMsg:  level = "Warning";  break;
		case QtCriticalMsg: level = "Critical"; break;
		case QtFatalMsg:    level = "Fatal";    break;
	}

	fprintf(stderr, "%s: %s (%s:%u, %s)\n",
		level, local_msg.constData(), file, context.line, function);
}

/**
	@brief ReentrancyGuard
	Sets the referenced flag on construction, clears it on destruction
	(including via early return / exception unwinding). Used as the
	per-thread guard against the logger recursing into itself.
*/
struct ReentrancyGuard
{
	bool &flag;
	explicit ReentrancyGuard(bool &f) : flag(f) {flag = true;}
	~ReentrancyGuard() {flag = false;}
};

} // namespace

/**
	@brief QetLogger::instance
	Function-local static: guaranteed constructed exactly once, in a
	thread-safe way, on first use -- but the *meaningful* initialisation
	(log path resolution, opening the file) happens in init(), called
	explicitly from main() at a defined point, not implicitly on
	whichever thread happens to log first.
*/
QetLogger &QetLogger::instance()
{
	static QetLogger logger;
	return logger;
}

void QetLogger::init()
{
	m_disabled = (qgetenv("QET_LOG_DISABLE") == "1");
	if (m_disabled) {
		return;
	}

	m_log_dir = QETApp::dataDir();
	m_base_name = QDate::currentDate().toString(QStringLiteral("yyyyMMdd"));

	QMutexLocker locker(&m_file_mutex);
	m_file_output_ok = ensureFileOpenLocked();
}

void QetLogger::installCrashHandler()
{
	if (m_disabled) {
		return;
	}
		//Both run here, in normal startup context, before this run's own
		//dump path is fixed: a dump left by a pre-#905 version is moved
		//in so it can still be offered, and any backlog is trimmed.
	migrateLegacyCrashDump();
	pruneCrashDumps();

		//Fixed for the life of the process: the handler copies it into a
		//preallocated buffer, and pendingCrashDumpFiles() needs to know
		//which file is this run's own so it doesn't offer it back.
	m_crash_dump_path = buildCrashDumpPath();
	CrashHandler::install(&m_ring, m_crash_dump_path);
}

/**
	@brief QetLogger::crashDumpDir
	@return the directory holding crash dumps.

	A directory rather than a single file, because dumps are per-run and
	several can be waiting at once. Creates nothing: a getter that made a
	directory as a side effect surprised a reviewer on #905, and the
	readers here (listing, pruning) have no business creating it.
*/
QString QetLogger::crashDumpDir() const
{
	return m_log_dir % QStringLiteral("/crashes");
}

/**
	@brief QetLogger::ensureCrashDumpDir
	@return crashDumpDir(), created if missing.

	For the callers that are about to write into it.
*/
QString QetLogger::ensureCrashDumpDir() const
{
	const QString dir = crashDumpDir();
	QDir().mkpath(dir);
	return dir;
}

/**
	@brief QetLogger::migrateLegacyCrashDump

	Before #905 the handler wrote to a single m_log_dir/crash_dump.log.
	After upgrading, nothing looks at that path any more: the dump of the
	crash that quite possibly prompted the upgrade would sit there unseen
	and undeleted forever. Move it into crashes/ under a name the
	crash_*.log filter matches, so it is offered exactly once like any
	other. Named from its own mtime, so it sorts by when it was written
	rather than when it was moved.
*/
void QetLogger::migrateLegacyCrashDump() const
{
	const QFileInfo legacy(m_log_dir % QStringLiteral("/crash_dump.log"));
	if (!legacy.exists() || !legacy.isFile() || legacy.size() <= 0) {
		return;
	}

	const QString target = ensureCrashDumpDir()
		% QStringLiteral("/crash_")
		% legacy.lastModified().toString(QStringLiteral("yyyyMMdd-hhmmss"))
		% QStringLiteral("_legacy.log");

	if (QFile::exists(target)) {
			//Migrated already by an earlier run of this version.
		QFile::remove(legacy.absoluteFilePath());
		return;
	}
	QFile::rename(legacy.absoluteFilePath(), target);
}

/**
	@brief QetLogger::pruneCrashDumps

	A crash that repeats on startup would otherwise write one dump per
	attempt without limit, since nothing is deleted until a dialog is
	actually shown and answered. Keep the newest kMaxPendingCrashDumps --
	enough to see a pattern, bounded however long the loop runs.
*/
void QetLogger::pruneCrashDumps() const
{
	QDir dir(crashDumpDir());
	if (!dir.exists()) {
		return;
	}
	dir.setNameFilters({QStringLiteral("crash_*.log")});
	dir.setFilter(QDir::Files);
	dir.setSorting(QDir::Time);

	const QFileInfoList entries = dir.entryInfoList();
	for (int i = kMaxPendingCrashDumps ; i < entries.size() ; ++i) {
		QFile::remove(entries.at(i).absoluteFilePath());
	}
}

/**
	@brief QetLogger::buildCrashDumpPath
	@return where this run would write a crash dump.

	One file per run, rather than a single fixed crash_dump.log. That old
	scheme opened one path with O_TRUNC, so a second crash overwrote the
	first: someone who crashed ten times still ended up with exactly one
	dump, the most recent. Reported on #898 -- "the report appeared only
	once despite there being 10 or more crashes" -- where losing the
	earlier dumps mattered as much as never being shown them.

	Built here in normal context and handed to CrashHandler::install(),
	which copies it into a preallocated buffer, so the handler still
	writes to one fixed path and its no-allocation invariant is untouched.
*/
QString QetLogger::buildCrashDumpPath() const
{
	return ensureCrashDumpDir()
		% QStringLiteral("/crash_")
		% QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"))
		% QStringLiteral("_")
		% QString::number(QCoreApplication::applicationPid())
		% QStringLiteral(".log");
}

/**
	@brief QetLogger::pendingCrashDumpFiles
	@return dumps left by previous runs, newest first, at most
	kMaxPendingCrashDumps of them.

	This run's own path is excluded: it does not exist yet unless this run
	is itself crashing, and a handler mid-crash is in no position to be
	offered a dialog.

	Callers take this list once and pass it on to
	pendingCrashDumpContents() and clearPendingCrashDump(), rather than
	each of those re-reading the directory. See clearPendingCrashDump().
*/
QStringList QetLogger::pendingCrashDumpFiles() const
{
	QDir dir(crashDumpDir());
	if (!dir.exists()) {
		return QStringList();
	}
	dir.setNameFilters({QStringLiteral("crash_*.log")});
	dir.setFilter(QDir::Files);
	dir.setSorting(QDir::Time);

	QStringList files;
	const QFileInfoList entries = dir.entryInfoList();
	for (const QFileInfo &info : entries)
	{
		if (info.size() <= 0) {
			continue;
		}
		if (!m_crash_dump_path.isEmpty()
				&& info.absoluteFilePath() == QFileInfo(m_crash_dump_path).absoluteFilePath()) {
			continue;
		}
		files << info.absoluteFilePath();
		if (files.size() >= kMaxPendingCrashDumps) {
			break;
		}
	}
	return files;
}

QString QetLogger::currentLogFilePath() const
{
	return m_log_dir % QStringLiteral("/") % m_base_name % QStringLiteral(".log");
}

/**
	@brief QetLogger::ensureFileOpenLocked
	Caller must hold m_file_mutex. Opens the current session's log file
	if not already open. Refuses to follow a pre-existing symlink at
	that path, and creates the file owner-read/write only.
*/
bool QetLogger::ensureFileOpenLocked()
{
	if (m_file.isOpen()) {
		return true;
	}

	QDir().mkpath(m_log_dir);

	const QString path = currentLogFilePath();

	const QFileInfo info(path);
	if (info.exists() && info.isSymLink()) {
		// Filesystem hardening: refuse a pre-planted symlink rather than
		// silently appending to whatever it points at.
		return false;
	}

	m_file.setFileName(path);
	if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
		return false;
	}
	m_file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
	m_bytes_written_current_file = m_file.size();
	return true;
}

QString QetLogger::rotatedPath(int index) const
{
	return m_log_dir % QStringLiteral("/") % m_base_name % QStringLiteral(".") % QString::number(index) % QStringLiteral(".log");
}

/**
	@brief QetLogger::rotateLocked
	Caller must hold m_file_mutex. Shifts .3.log -> .4.log (dropping the
	previous .4.log), .2.log -> .3.log, .1.log -> .2.log, .log -> .1.log,
	then opens a fresh, empty current file.
*/
void QetLogger::rotateLocked()
{
	m_file.close();

	const QString base_path = currentLogFilePath();

	for (int i = kRotationKeep; i >= 1; --i) {
		const QString from = (i == 1) ? base_path : rotatedPath(i - 1);
		const QString to = rotatedPath(i);

		if (QFile::exists(to)) {
			QFile::remove(to);
		}
		if (QFile::exists(from)) {
			QFile::rename(from, to);
		}
	}

	m_bytes_written_current_file = 0;
	m_file_output_ok = ensureFileOpenLocked();
}

void QetLogger::writeToFile(const QByteArray &line, QtMsgType type)
{
	QMutexLocker locker(&m_file_mutex);

	if (!m_file_output_ok) {
		// Write-failure policy: once file output has failed, stop
		// attempting it rather than spin-retrying every message. The
		// ring keeps running regardless.
		return;
	}

	const qint64 written = m_file.write(line);
	if (written != line.size()) {
		m_file_output_ok = false;
		m_file.close();
		return;
	}
	m_bytes_written_current_file += written;

	if (type >= QtWarningMsg) {
		m_file.flush();
	}

	if (m_bytes_written_current_file >= kMaxFileBytes) {
		rotateLocked();
	}
}

/**
	@brief QetLogger::sanitize
	Escapes newlines, carriage returns and other control characters.
	Much of what QET logs is externally controlled (file paths, element
	names, font strings read out of a .qet file); left unescaped, a
	crafted string containing '\n' can forge additional log lines.
	Operates on already-UTF-8-encoded bytes: this is safe because UTF-8
	continuation bytes are always >= 0x80, so any byte < 0x20 found here
	is a genuine ASCII control character, never part of a multi-byte
	sequence.
*/
QByteArray QetLogger::sanitize(const QByteArray &input)
{
	QByteArray out;
	out.reserve(input.size());

	for (unsigned char c : input) {
		if (c == '\n') {
			out += "\\n";
		} else if (c == '\r') {
			out += "\\r";
		} else if (c == '\t') {
			out += static_cast<char>(c);
		} else if (c < 0x20 || c == 0x7F) {
			out += "\\x";
			out += QByteArray::number(c, 16).rightJustified(2, '0');
		} else {
			out += static_cast<char>(c);
		}
	}

	return out;
}

/**
	@brief QetLogger::truncateMessage
	Caps a single message at max_bytes, appending a marker stating how
	many bytes were dropped, so one pathological caller (e.g. dumping an
	entire XML document to qDebug()) can't consume an unbounded amount
	of the ring's or file's byte budget.
*/
QByteArray QetLogger::truncateMessage(const QByteArray &input, int max_bytes)
{
	if (input.size() <= max_bytes) {
		return input;
	}

	const int dropped = input.size() - max_bytes;
	QByteArray out = input.left(max_bytes);
	out += " ...[truncated ";
	out += QByteArray::number(dropped);
	out += " bytes]";
	return out;
}

QByteArray QetLogger::formatLine(QtMsgType type, const QMessageLogContext &context, const QByteArray &sanitized_msg)
{
	// Includes the date (not just the time) so that a session crossing
	// midnight -- now kept in a single file -- doesn't read as ambiguous.
	const QByteArray timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz")).toUtf8();

	const char *level = "Unknown";
	switch (type) {
		case QtDebugMsg:    level = "Debug";    break;
		case QtInfoMsg:     level = "Info";     break;
		case QtWarningMsg:  level = "Warning";  break;
		case QtCriticalMsg: level = "Critical"; break;
		case QtFatalMsg:    level = "Fatal";    break;
	}

	const char *file = context.file ? context.file : "";
	const char *function = context.function ? context.function : "";

	QByteArray line = timestamp;
	line += ' ';
	line += level;
	line += ": ";
	line += sanitized_msg;

	if (type == QtInfoMsg) {
		line += " \n";
	} else {
		line += " (";
		line += file;
		line += ":";
		line += QByteArray::number(context.line ? context.line : 0);
		line += ", ";
		line += function;
		line += ")\n";
	}

	return line;
}

void QetLogger::handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	if (m_disabled) {
		legacyStderrOutput(type, context, msg);
		return;
	}

	static thread_local bool in_handler = false;
	if (in_handler) {
		// The logger itself triggered a message (e.g. from inside a Qt
		// call it made) -- drop it rather than recurse.
		return;
	}
	ReentrancyGuard guard(in_handler);

	const QByteArray sanitized = truncateMessage(sanitize(msg.toUtf8()), kMaxMessageBytes);
	const QByteArray line = formatLine(type, context, sanitized);

	fwrite(line.constData(), 1, static_cast<size_t>(line.size()), stderr);

	m_ring.append(line);
	writeToFile(line, type);
}

void QetLogger::pruneOldLogFiles(int days)
{
	if (m_disabled) {
		return;
	}

	const QDate today = QDate::currentDate();
	const QStringList filters = {
		QStringLiteral("????????.log"),   // base files, e.g. 20260803.log
		QStringLiteral("????????.?.log"), // rotated files, e.g. 20260803.1.log
	};

	const QDir dir(m_log_dir);
	const auto entries = dir.entryInfoList(filters, QDir::Files);
	for (const QFileInfo &file_info : entries) {
		if (!file_info.isFile()) {
			continue;
		}
		// lastModified(), not lastRead(): reading the log (opening it to
		// attach to a bug report, a backup job, an indexer) must not
		// reset the retention clock and keep it alive indefinitely.
		if (file_info.lastModified().date().daysTo(today) > days) {
			QFile::remove(file_info.absoluteFilePath());
		}
	}
}

// --- Step 5: getting the data back out ----------------------------------

bool QetLogger::hasPendingCrashDump() const
{
	if (m_disabled) {
		return false;
	}
	return !pendingCrashDumpFiles().isEmpty();
}

/**
	@brief QetLogger::pendingCrashDumpContents
	@param files the list from pendingCrashDumpFiles()
	@return those dumps, in the order given, concatenated.

	All of them rather than only the latest: a crash that repeats is the
	case where the earlier dumps are most worth having, since the
	difference between them is the evidence. They are separated by a
	banner so a reader can tell where one ends and the next begins, and
	the whole thing is redacted as a single pass.
*/
QByteArray QetLogger::pendingCrashDumpContents(const QStringList &files) const
{
	if (files.isEmpty()) {
		return QByteArray();
	}

	QByteArray all;
	if (files.size() > 1) {
		all += QByteArray("QET: ") + QByteArray::number(files.size())
			+ " crash dumps pending, newest first.\n\n";
	}

	for (const QString &path : files)
	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		all += "===== " + QFileInfo(path).fileName().toUtf8() + " =====\n";
		all += file.readAll();
		if (!all.endsWith('\n')) {
			all += '\n';
		}
		all += '\n';
	}

	return redact(all);
}

/**
	@brief QetLogger::clearPendingCrashDump
	@param files exactly the dumps that were offered

	Deletes the list it is given rather than re-reading the directory.
	The offer sits inside a modal dialog that can stay open for as long
	as the user cares to read it, and dumps are per-run: a second
	QElectroTech -- SingleApplication keys its socket on the binary path,
	so a different build is a separate instance -- can crash and write a
	new dump while that dialog is up. Re-listing at this point would
	delete that fresh dump without anyone ever having seen it, which is
	the failure this whole change is about.
*/
void QetLogger::clearPendingCrashDump(const QStringList &files)
{
	for (const QString &path : files) {
		QFile::remove(path);
	}
}

QByteArray QetLogger::buildDiagnosticsReport() const
{
	QByteArray header;
	header += "QElectroTech diagnostics report\n";
	header += "Generated: " % QDateTime::currentDateTime().toString(Qt::ISODate) % "\n";
	header += "Version: " % QetVersion::displayedVersion() % "\n";
	header += "Git: " GIT_COMMIT_SHA "\n";
	header += "OS: " % QSysInfo::prettyProductName() % " (" % QSysInfo::currentCpuArchitecture() % ")\n";
	header += "Qt: " QT_VERSION_STR "\n";
	header += "---\n";

	QByteArray body;
	QFile file(currentLogFilePath());
	if (file.open(QIODevice::ReadOnly)) {
		body = file.readAll();
	} else {
		// Fall back to the in-memory ring if the file itself can't be
		// read (e.g. file output already failed this session).
		for (const QByteArray &line : m_ring.snapshot()) {
			body += line;
		}
	}

	return redact(header + body);
}

/**
	@brief QetLogger::redact
	Replaces the user's home directory with "~" wherever it appears.
	Applied before a crash dump or a diagnostics report is ever shown to
	the user: both are destined to be attached to a public bug tracker,
	and an absolute path under the home directory leaks the account name
	(discussion #644's privacy section: "/home/laurent/... leaks a
	username"). This is the one redaction implemented here; the
	discussion's fancier "optionally redact project filenames too" is
	not attempted -- reliably telling a project path apart from
	arbitrary log text is a much fuzzier problem than a literal prefix
	match against a known directory.
*/
QByteArray QetLogger::redact(const QByteArray &input)
{
	QByteArray out = input;

	const QByteArray home = QDir::homePath().toUtf8();
	if (!home.isEmpty()) {
		out.replace(home, QByteArrayLiteral("~"));
	}

		//backtrace_symbols_fd() writes the absolute path of each module,
		//which for an AppImage is the per-run mount point
		///tmp/.mount_QElectXXXXXX. Not identifying on its own, but it is
		//noise in a bug report and it is a path the user never typed, so
		//fold it to a stable name. Done after the home replacement above
		//because the mount point is not under $HOME.
	const QByteArray mount_prefix("/tmp/.mount_");
	int at = out.indexOf(mount_prefix);
	while (at >= 0)
	{
		int end = at + mount_prefix.size();
		while (end < out.size() && out.at(end) != '/' && !isspace(static_cast<unsigned char>(out.at(end)))) {
			++end;
		}
		out.replace(at, end - at, QByteArrayLiteral("<appimage>"));
		at = out.indexOf(mount_prefix, at + 10);
	}

	return out;
}
