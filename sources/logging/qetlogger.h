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

	Deliberately NOT included in this step (see discussion #644): no
	signal handler / crash-flush (step 4), no diagnostics export UI
	(step 5), no log categories, no session header, no repeat collapsing
	or rate limiting. Those are independent, separately-scoped follow-ups.

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

		/// The function installed via qInstallMessageHandler() forwards here.
		void handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

		/// Replaces the old delete_old_log_files(): same call shape, fixed
		/// to use lastModified() (not lastRead()) and to also match rotated
		/// file names.
		void pruneOldLogFiles(int days);

		/// Snapshot of the in-memory ring, oldest first. For future use
		/// (e.g. a diagnostics export action) -- not wired to any UI here.
		QVector<QByteArray> ringSnapshot() const {return m_ring.snapshot();}

	private:
		QetLogger() = default;
		QetLogger(const QetLogger &) = delete;

		bool ensureFileOpenLocked();
		void rotateLocked();
		void writeToFile(const QByteArray &line, QtMsgType type);
		QString rotatedPath(int index) const;

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
