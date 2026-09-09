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
#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#include <QString>

class LogRing;

/**
	@brief The CrashHandler class
	Discussion #644, step 4: on a fatal crash, flush the in-memory
	LogRing to a fixed file before the process dies, so the last N log
	lines leading up to the crash survive it -- today they only exist in
	memory and are lost with the process.

	This is the highest-risk piece of the whole logging rework (the
	discussion's own words: "lands last, behind its own switch"), so its
	invariants are worth restating plainly:

	1. The handler must never block. It takes no locks -- LogRing itself
	   is lock-free for exactly this reason (see logring.h). A handler
	   that can hang is worse than no handler: it turns a clean crash
	   (which at least produces a core dump) into a hung process that has
	   to be force-killed, producing neither a core dump nor a ring dump.
	2. The handler must never allocate. Under heap corruption -- a
	   plausible *cause* of the very crash being handled -- malloc may
	   itself deadlock or fault. Every buffer this code touches at crash
	   time (the dump path, the header, the ring's own storage) is
	   preallocated by install(), which runs once at startup in normal
	   (non-signal) context.
	3. The handler must not swallow the crash. After writing the dump it
	   restores the default disposition for the signal and re-raises, so
	   the OS still produces a core dump (POSIX) / Windows Error
	   Reporting still sees the exception. A handler that "fixed" the
	   crash by not re-raising would destroy the post-mortem evidence a
	   core dump provides.
	4. Only the *first* crash writes a dump. An atomic test-and-set
	   guards against two threads faulting simultaneously (or the handler
	   itself faulting while dumping) producing an interleaved or
	   truncated file; every crash after the first goes straight to
	   restore-and-re-raise.

	Tested in this environment: POSIX/Linux only (sigaction, sigaltstack,
	SIGSEGV/SIGABRT/SIGBUS/SIGFPE/SIGILL). The Windows path
	(SetUnhandledExceptionFilter) and macOS-specific behaviour (signal
	handling itself is POSIX and shares the Linux code path, but sandbox
	profiles can affect where the dump file may be written) are
	implemented per the discussion's guidance but could not be exercised
	here -- there is no Windows or macOS build available in this sandbox.
	Please sanity-check both before relying on them in the field.
*/
class CrashHandler
{
	public:
		/// Installs the crash handler. Must be called from normal
		/// (non-signal) startup code, after the LogRing it will dump
		/// exists, and only once. `ring` must outlive the process (in
		/// practice: the LogRing owned by QetLogger's function-local
		/// static instance, which is never destroyed before exit).
		/// `dump_path` is resolved and copied into a fixed-size internal
		/// buffer here; nothing under the actual signal/exception path
		/// touches QString.
		static void install(const LogRing *ring, const QString &dump_path);

	private:
		CrashHandler() = delete;
};

#endif // CRASHHANDLER_H
