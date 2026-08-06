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

#include "../qetapp.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
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

	const QString path = m_log_dir % QStringLiteral("/") % m_base_name % QStringLiteral(".log");

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

	const QString base_path = m_log_dir % QStringLiteral("/") % m_base_name % QStringLiteral(".log");

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
