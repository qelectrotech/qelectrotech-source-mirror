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
#include "crashhandler.h"

#include "logring.h"
#include "../qetversion.h"

#include <QByteArray>
#include <QSysInfo>
#include <atomic>
#include <cstring>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {

// Everything the handler touches is preallocated here and filled in by
// install() (normal context, runs once at startup) -- nothing under the
// actual signal/exception path may allocate or touch QString/Qt.
const LogRing *g_ring = nullptr;
char g_dump_path[1024] = {};
char g_header[1024] = {};
int g_header_len = 0;

// Guards against two threads crashing at once, or the handler itself
// faulting while dumping: only the first crash writes a dump. See
// crashhandler.h invariant 4.
std::atomic<bool> g_already_dumped{false};

#ifndef Q_OS_WIN

// A stack-overflow SIGSEGV leaves no usable stack for a handler to run
// on at all, hence the alternate signal stack (invariant: sized well
// above any known SIGSTKSZ so this doesn't depend on
// sysconf(_SC_SIGSTKSZ), which some libc versions require at runtime
// rather than offering as a compile-time constant).
char g_altstack[65536];

const int kHandledSignals[] = {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL};

void restoreDefaultAndReraise(int sig)
{
	struct sigaction sa {};
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(sig, &sa, nullptr);
	raise(sig);
}

void signalHandler(int sig)
{
	if (g_already_dumped.exchange(true, std::memory_order_acq_rel)) {
		// Not the first crash (concurrent fault on another thread, or
		// this handler faulting while dumping): skip straight to
		// restore-and-re-raise rather than risk a second, interleaved
		// write to the same file.
		restoreDefaultAndReraise(sig);
		return;
	}

	// open/write/close are all on the POSIX async-signal-safe function
	// list; nothing else is called here.
	const int fd = ::open(g_dump_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd >= 0) {
		if (g_header_len > 0) {
			::write(fd, g_header, static_cast<size_t>(g_header_len));
		}
		if (g_ring) {
			g_ring->dumpToFd(fd);
		}
		::close(fd);
	}

	restoreDefaultAndReraise(sig);
}

#else // Q_OS_WIN

LONG WINAPI windowsExceptionFilter(EXCEPTION_POINTERS *)
{
	bool expected = false;
	if (!g_already_dumped.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	int fd = -1;
	errno_t err = _sopen_s(&fd, g_dump_path,
				_O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
				_SH_DENYWR, _S_IREAD | _S_IWRITE);
	if (err == 0 && fd >= 0) {
		if (g_header_len > 0) {
			_write(fd, g_header, g_header_len);
		}
		if (g_ring) {
			g_ring->dumpToFd(fd);
		}
		_close(fd);
	}

	// Do not suppress Windows Error Reporting / an attached debugger --
	// same invariant as re-raising on POSIX (see crashhandler.h,
	// invariant 3).
	return EXCEPTION_CONTINUE_SEARCH;
}

#endif

} // namespace

void CrashHandler::install(const LogRing *ring, const QString &dump_path)
{
	g_ring = ring;

	const QByteArray path_utf8 = dump_path.toUtf8();
	std::strncpy(g_dump_path, path_utf8.constData(), sizeof(g_dump_path) - 1);

	const QByteArray header = QByteArray("QET crash dump\n")
		+ "Version: " + QetVersion::displayedVersion().toUtf8() + "\n"
		+ "Git: " GIT_COMMIT_SHA "\n"
		+ "OS: " + QSysInfo::prettyProductName().toUtf8() + " (" + QSysInfo::currentCpuArchitecture().toUtf8() + ")\n"
		+ "Qt: " QT_VERSION_STR "\n"
		+ "---\n";
	g_header_len = qMin<int>(header.size(), static_cast<int>(sizeof(g_header)) - 1);
	std::memcpy(g_header, header.constData(), static_cast<size_t>(g_header_len));

#ifdef Q_OS_WIN
	SetUnhandledExceptionFilter(windowsExceptionFilter);
#else
	stack_t ss;
	ss.ss_sp = g_altstack;
	ss.ss_size = sizeof(g_altstack);
	ss.ss_flags = 0;
	sigaltstack(&ss, nullptr);

	struct sigaction sa {};
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_ONSTACK;

	for (int sig : kHandledSignals) {
		sigaction(sig, &sa, nullptr);
	}
#endif
}
