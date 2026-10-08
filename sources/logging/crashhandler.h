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

	1. The handler must never block. It takes no locks of its own --
	   LogRing is lock-free for exactly this reason (see logring.h). A
	   handler that can hang is worse than no handler: it turns a clean
	   crash (which at least produces a core dump) into a hung process
	   that has to be force-killed, producing neither a core dump nor a
	   ring dump. The one exception is deliberate and comes last:
	   backtrace() unwinds through libgcc, which takes the loader lock,
	   so it is written after the ring rather than before it. A crash
	   inside dlopen() then costs the backtrace, not the whole dump.
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

	On Windows the dump says what crashed (the exception code, the
	module and offset it happened at, and for an access violation the
	address accessed) and walks the stack as "module+offset" lines, from
	the unwind tables every x64 image carries. It is written by a
	reporter thread started by install(), not by the crashing thread,
	which may have no stack left (a stack overflow) or hold the loader
	lock; the crashing thread waits for it for at most ten seconds.
	abort() -- the end of std::terminate() and a failed assert -- raises
	no SEH exception there, so SIGABRT is handled too. qFatal() is
	neither: Qt ends it with TerminateProcess(), so the message handler
	reports it (reportFatal()).

	Tested: POSIX/Linux (sigaction, sigaltstack,
	SIGSEGV/SIGABRT/SIGBUS/SIGFPE/SIGILL), and the Windows path under
	Wine 11 (a null pointer, a call through a null pointer, a stack
	overflow, abort() and qFatal()). macOS shares the POSIX code path, but sandbox
	profiles can affect where the dump file may be written; that has not
	been exercised.
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

			/// Writes the dump for a qFatal(). Called by the message
			/// handler once the fatal message is in the ring. Needed on
			/// Windows only: Qt ends a qFatal() there with
			/// TerminateProcess(), which neither the exception filter
			/// nor SIGABRT ever sees. Elsewhere it does nothing, since
			/// qFatal() ends in abort() and the SIGABRT handler writes the
			/// dump, backtrace included.
		static void reportFatal();

			/// Writes `value` as decimal into `buffer`, at most `size`
			/// bytes, and returns how many were written. The handler
			/// needs this because write() takes a buffer and snprintf()
			/// is not on the async-signal-safe list; `buffer` is caller-
			/// owned (the handler's stack), so nothing is allocated.
			/// Truncates rather than overflowing when `size` is too
			/// small, and writes nothing for `size <= 0`.
			///
			/// Public only so tests can reach it -- see
			/// tests/qttest/tst_crashhandler.cpp. Nothing else in the
			/// application calls it.
		static int formatInt(char *buffer, int size, int value);

			/// Writes `value` as "0x" followed by lower-case hex digits,
			/// with the same rules as formatInt(): caller-owned buffer,
			/// at most `size` bytes, truncating rather than overflowing.
			/// Used for addresses and exception codes in the Windows
			/// dump; public for the same reason as formatInt().
		static int formatHex(char *buffer, int size, unsigned long long value);

	private:
		CrashHandler() = delete;
};

#endif // CRASHHANDLER_H
