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
#ifndef QETLOGGER_H
#define QETLOGGER_H

#include "logring.h"

#include <QFile>
#include <QMutex>
#include <QString>
#include <QtGlobal>

/**
	@brief The QetLogger class
	Rework of QET's diagnostic logging (discussion #644, steps 1-3):

	- Step 1: one file handle held open for the session under a mutex
	  instead of opening/closing per message; the log path (including
	  the date-stamped filename) is resolved exactly once, at init(),
	  instead of being recomputed on every message -- a session that
	  crosses midnight now stays in one file; retention now uses
	  lastModified() instead of lastRead(); stderr and file output both
	  use UTF-8 explicitly (previously stderr used the local 8-bit
	  codec and the file's encoding silently differed between Qt5 and
	  Qt6).
	- Step 2: the previously-unbounded daily file is now size-capped
	  and rotated (kMaxFileBytes per file, kRotationKeep old files kept
	  beyond the current one); each message is truncated to
	  kMaxMessageBytes and control characters are escaped before being
	  written, so one pathological caller can't blow the size budget or
	  forge log lines; the log file is refused if it already exists as
	  a symlink and is created owner-read/write only.
	- Step 3: every formatted line is also appended to an in-memory
	  LogRing (see logring.h) -- always on, fixed capacity, allocation-
	  free on the hot path.
	- Step 4: installCrashHandler() wires the ring up to CrashHandler
	  (see crashhandler.h), so a SIGSEGV/SIGABRT/SIGBUS/SIGFPE/SIGILL (or,
	  on Windows, an unhandled structured exception) flushes the ring to
	  a fixed crash-dump file before the process dies.
	- Step 5: hasPendingCrashDump()/pendingCrashDumpContents()/
	  clearPendingCrashDump() let startup code (see QETApp::checkBackupFiles())
	  notice and offer an unretrieved crash dump from the *previous* run;
	  buildDiagnosticsReport() is the equivalent for a manual "save a
	  report right now" action on the *current*, still-running session.
	  Both go through redact() before ever reaching the user, since both
	  are destined for a public bug tracker.

	Deliberately NOT included: log categories, a full session header
	beyond what the crash dump/report already carry, repeat collapsing,
	rate limiting. Those are listed in discussion #644 under "best
	practices worth building in", not part of the numbered steps.

	Escape hatch: if QET_LOG_DISABLE=1 is set in the environment at
	init() time, this class does nothing beyond a minimal, independent
	stderr passthrough -- no ring, no file, no rotation -- so a problem
	in this rework can be worked around without a rebuild.
*/
class QetLogger
{
	public:
		static constexpr qint64 kMaxFileBytes = 2 * 1024 * 1024; // 2 MiB per file
		static constexpr int kRotationKeep = 4;                  // .1.log .. .4.log
		static constexpr int kMaxMessageBytes = 4096;             // per-message truncation

		static QetLogger &instance();

		/// Must be called exactly once, from main(), before
		/// qInstallMessageHandler(). Resolves the log directory and the
		/// session's log filename, and opens the file.
		void init();

		/// Step 4: installs the crash handler (see crashhandler.h). Must
		/// be called after init() (the ring and the dump path must exist
		/// first) and, like init(), only once.
		void installCrashHandler();

		/// The function installed via qInstallMessageHandler() forwards here.
		void handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

		/// Replaces the old delete_old_log_files(): same call shape, fixed
		/// to use lastModified() (not lastRead()) and to also match rotated
		/// file names.
		void pruneOldLogFiles(int days);

		/// Snapshot of the in-memory ring, oldest first.
		QVector<QByteArray> ringSnapshot() const {return m_ring.snapshot();}

		// --- Step 5: getting the data back out -------------------------

		/// True if a previous run's crash handler left an unretrieved
		/// dump behind.
		bool hasPendingCrashDump() const;

		/// Raw contents of the pending crash dump, or an empty array if
		/// there isn't one. Does not delete it -- call
		/// clearPendingCrashDump() once it has been offered to the user.
		QByteArray pendingCrashDumpContents() const;

		/// Deletes the pending crash dump file. Call after the user has
		/// been offered it (whether they chose to save it or not) so it
		/// is never offered a second time.
		void clearPendingCrashDump();

		/// Builds a redacted diagnostics bundle from the *current* session
		/// (header + this session's log file so far) for the manual
		/// "Save report" action -- as opposed to pendingCrashDumpContents(),
		/// which is about a *previous*, already-terminated session.
		QByteArray buildDiagnosticsReport() const;

		/// Replaces occurrences of the user's home directory with "~".
		/// Applied to both the crash dump and buildDiagnosticsReport()
		/// before they are ever shown to the user, since both are
		/// destined for a public bug tracker.
		static QByteArray redact(const QByteArray &input);

	private:
		QetLogger() = default;
		QetLogger(const QetLogger &) = delete;

		bool ensureFileOpenLocked();
		void rotateLocked();
		void writeToFile(const QByteArray &line, QtMsgType type);
		QString rotatedPath(int index) const;
		QString crashDumpPath() const;
		QString currentLogFilePath() const;

		static QByteArray sanitize(const QByteArray &input);
		static QByteArray truncateMessage(const QByteArray &input, int max_bytes);
		static QByteArray formatLine(QtMsgType type, const QMessageLogContext &context, const QByteArray &sanitized_msg);

		bool m_disabled = false;

		QString m_log_dir;
		QString m_base_name; // e.g. "20260803", resolved once in init()

		QMutex m_file_mutex;
		QFile m_file;
		qint64 m_bytes_written_current_file = 0;
		bool m_file_output_ok = false;

		LogRing m_ring;
};

#endif // QETLOGGER_H
