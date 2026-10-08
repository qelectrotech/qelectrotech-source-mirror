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
#include <csignal>
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
	// QET_CRASH_BACKTRACE is defined by CMake, via find_package(Backtrace),
	// not by probing for the header here. <execinfo.h> exists on FreeBSD as
	// well, but backtrace() is in a separate libexecinfo there, so a header
	// probe compiles and then fails to link.
#ifdef QET_CRASH_BACKTRACE
#include <execinfo.h>
#endif
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

#ifdef QET_CRASH_BACKTRACE
// Preallocated here for the same reason as everything else in this block:
// backtrace() fills a caller-supplied array, so it needs no heap of its
// own, and backtrace_symbols_fd() writes straight to the fd (unlike
// backtrace_symbols(), which mallocs and is therefore unusable here).
void *g_backtrace_frames[64];
#endif

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

	// open/write/close, and backtrace_symbols_fd, are all on the POSIX
	// async-signal-safe function list; nothing else is called here.
	//
	// Async-signal-safe is not the same as lock-free, which is why the
	// order below matters. backtrace() unwinds through libgcc, which calls
	// dl_iterate_phdr and takes the loader lock. Warming it in install()
	// removes the allocation, not the lock -- so a crash that happens
	// inside dlopen() (Qt plugin loading), or on a corrupted heap or
	// stack, can leave this handler deadlocked or faulting a second time
	// at the backtrace. Everything cheaper and more valuable is therefore
	// written and flushed first: header, signal, then the log ring. If the
	// backtrace never completes, the dump is still there and still useful.
	const int fd = ::open(g_dump_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd >= 0) {
		if (g_header_len > 0) {
			::write(fd, g_header, static_cast<size_t>(g_header_len));
		}

		// Which signal killed it. The header is built once at install()
		// and is therefore identical for every crash, so without this the
		// dump never said what actually happened -- SIGSEGV and SIGABRT
		// point at very different bugs.
		char line[64];
		int len = 0;
		const char kSignalLabel[] = "Signal: ";
		for (unsigned i = 0 ; i < sizeof(kSignalLabel) - 1 ; ++i) {
			line[len++] = kSignalLabel[i];
		}
		len += CrashHandler::formatInt(line + len, static_cast<int>(sizeof(line)) - len - 1, sig);
		line[len++] = '\n';
		::write(fd, line, static_cast<size_t>(len));

		// The ring first: it is the part that says what the program was
		// doing, it costs one write, and it takes no lock.
		const char kRingLabel[] = "--- log ---\n";
		::write(fd, kRingLabel, sizeof(kRingLabel) - 1);
		if (g_ring) {
			g_ring->dumpToFd(fd);
		}

#ifdef QET_CRASH_BACKTRACE
		// Then where it was when it died. Last, deliberately: see the
		// note above about the loader lock. backtrace() is warmed in
		// install() so its first-call lazy resolution cannot allocate
		// here, and backtrace_symbols_fd() writes to the fd without
		// allocating -- unlike backtrace_symbols(), which mallocs and
		// must not be used.
		const char kBacktraceLabel[] = "--- backtrace ---\n";
		::write(fd, kBacktraceLabel, sizeof(kBacktraceLabel) - 1);
		const int frames = ::backtrace(g_backtrace_frames,
					       static_cast<int>(sizeof(g_backtrace_frames)
								/ sizeof(g_backtrace_frames[0])));
		if (frames > 0) {
			::backtrace_symbols_fd(g_backtrace_frames, frames, fd);
		}
#endif

		::close(fd);
	}

	restoreDefaultAndReraise(sig);
}

#else // Q_OS_WIN

// The dump is not written on the crashing thread but on a reporter thread
// started by install(). The crashing thread may have no stack left at all
// (a stack overflow, EXCEPTION_STACK_OVERFLOW), and even when it has some,
// the module lookup below takes the loader lock, which the crashing thread
// may be holding. The exception filter therefore only records what
// happened, wakes the reporter and waits for it, for a bounded time: a
// reporter that blocks costs the dump, never a hung process.
HANDLE g_reporter_go = nullptr;
HANDLE g_reporter_done = nullptr;
const DWORD kReporterTimeoutMs = 10000;

// What the crashing thread hands over. Filled in before g_reporter_go is
// signalled and only read after it, so no lock is needed.
enum class CrashKind { Exception, Abort, Fatal };
CrashKind g_crash_kind = CrashKind::Exception;
DWORD g_crash_code = 0;			// the exception code, for CrashKind::Exception
const EXCEPTION_RECORD *g_crash_record = nullptr;
CONTEXT g_crash_context;		// copied: the walk below modifies it
DWORD64 g_crash_stack_low = 0;	// the crashing thread's stack, from its TIB,
DWORD64 g_crash_stack_high = 0;	// so the walk never reads outside it

void writeText(int fd, const char *text)
{
	_write(fd, text, static_cast<unsigned int>(std::strlen(text)));
}

void writeHex(int fd, unsigned long long value)
{
	char buffer[24];
	_write(fd, buffer, static_cast<unsigned int>(
		       CrashHandler::formatHex(buffer, sizeof(buffer), value)));
}

/**
	The exception codes worth a name, so a report reads "access violation"
	rather than a number to look up. Anything else is still printed as
	its code.
*/
const char *exceptionName(DWORD code)
{
	switch (code) {
		case EXCEPTION_ACCESS_VIOLATION:       return "access violation";
		case EXCEPTION_STACK_OVERFLOW:         return "stack overflow";
		case EXCEPTION_ILLEGAL_INSTRUCTION:    return "illegal instruction";
		case EXCEPTION_PRIV_INSTRUCTION:       return "privileged instruction";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:     return "integer divide by zero";
		case EXCEPTION_INT_OVERFLOW:           return "integer overflow";
		case EXCEPTION_IN_PAGE_ERROR:          return "in-page error";
		case EXCEPTION_DATATYPE_MISALIGNMENT:  return "datatype misalignment";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:  return "array bounds exceeded";
		case EXCEPTION_BREAKPOINT:             return "breakpoint";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:     return "floating-point divide by zero";
		case EXCEPTION_FLT_INVALID_OPERATION:  return "floating-point invalid operation";
		case 0xC0000374:                       return "heap corruption";
		case 0xC0000409:                       return "stack buffer overrun";
		case 0x20474343:                       return "uncaught C++ exception (GCC)";
		case 0xE06D7363:                       return "uncaught C++ exception (MSVC)";
		default:                               return nullptr;
	}
}

/**
	Writes @p address as "module.dll+0x1234", the form a symbolizer needs:
	modules load at a different address on every run, the offset inside
	the module does not. Returns false, writing nothing, when the address
	is in no loaded module.
*/
bool writeModuleOffset(int fd, DWORD64 address)
{
	PVOID base = nullptr;
	if (!RtlPcToFileHeader(reinterpret_cast<PVOID>(address), &base) || !base) {
		return false;
	}

	wchar_t path[MAX_PATH];
	const DWORD length = GetModuleFileNameW(static_cast<HMODULE>(base), path, MAX_PATH);
	const wchar_t *name = path;
	for (DWORD i = 0 ; i < length ; ++i) {
		if (path[i] == L'\\' || path[i] == L'/') {
			name = path + i + 1;
		}
	}
	char name_utf8[MAX_PATH * 3];
	const int written = length
			? WideCharToMultiByte(CP_UTF8, 0, name, -1,
					      name_utf8, sizeof(name_utf8), nullptr, nullptr)
			: 0;
	writeText(fd, written > 0 ? name_utf8 : "?");
	writeText(fd, "+");
	writeHex(fd, address - reinterpret_cast<DWORD64>(base));
	return true;
}

#if defined(_M_X64) || defined(__x86_64__)
/**
	Walks the crashed thread's stack from @p context, one "#NN module+offset"
	line per frame. Uses the unwind tables every x64 image carries (GCC
	emits them too), through RtlLookupFunctionEntry/RtlVirtualUnwind: no
	dbghelp, no symbols, no allocation. Turning the offsets into function
	names and lines is left to a symbolizer run later against the same
	build, e.g. addr2line.
*/
void writeBacktrace(int fd, CONTEXT *context)
{
	const DWORD64 low = g_crash_stack_low;
	const DWORD64 high = g_crash_stack_high;
	int printed = 0;

	for (int i = 0 ; i < 64 ; ++i)
	{
		const DWORD64 pc = context->Rip;

			//Frame 0 is always written, even outside any module: a call
			//through a null pointer shows up exactly that way. Deeper
			//frames outside every module are values the leaf rule below
			//picked up from a helper such as __chkstk_ms that pushed
			//registers without unwind data; skipping them keeps the list
			//readable, and the walk re-synchronises on its own.
		PVOID module_base = nullptr;
		const bool in_module = RtlPcToFileHeader(reinterpret_cast<PVOID>(pc), &module_base)
				       && module_base;
		if (i == 0 || in_module) {
			char frame[8] = {'#', 0, 0, ' ', 0};
			frame[1] = static_cast<char>('0' + (printed / 10) % 10);
			frame[2] = static_cast<char>('0' + printed % 10);
			writeText(fd, frame);
			if (!writeModuleOffset(fd, pc)) {
				writeHex(fd, pc);
			}
			writeText(fd, "\n");
			++printed;
		}

		DWORD64 image_base = 0;
		PRUNTIME_FUNCTION function = pc
				? RtlLookupFunctionEntry(pc, &image_base, nullptr)
				: nullptr;
		if (function) {
			PVOID handler_data = nullptr;
			DWORD64 establisher_frame = 0;
			RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, pc, function,
					 context, &handler_data, &establisher_frame, nullptr);
		} else {
				//A leaf function (no unwind entry), or a call to a bad
				//address: the return address is on top of the stack.
			if (context->Rsp < low || context->Rsp + 8 > high) {
				break;
			}
			context->Rip = *reinterpret_cast<const DWORD64 *>(context->Rsp);
			context->Rsp += 8;
		}
		if (!context->Rip || context->Rsp < low || context->Rsp >= high) {
			break;
		}
	}
}
#endif

/**
	Runs on the reporter thread. Same order as the POSIX handler, and for
	the same reason: what crashed, then the log ring, then the backtrace,
	so that whatever the walk runs into, the cheaper parts are already on
	disk.
*/
void writeWindowsDump()
{
	int fd = -1;
	const errno_t err = _sopen_s(&fd, g_dump_path,
				     _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
				     _SH_DENYWR, _S_IREAD | _S_IWRITE);
	if (err != 0 || fd < 0) {
		return;
	}

	if (g_header_len > 0) {
		_write(fd, g_header, g_header_len);
	}

	if (g_crash_kind == CrashKind::Exception && g_crash_record) {
		writeText(fd, "Exception: ");
		writeHex(fd, g_crash_code);
		if (const char *name = exceptionName(g_crash_code)) {
			writeText(fd, " (");
			writeText(fd, name);
			writeText(fd, ")");
		}
		writeText(fd, "\nAt: ");
		const DWORD64 address = reinterpret_cast<DWORD64>(g_crash_record->ExceptionAddress);
		if (!writeModuleOffset(fd, address)) {
			writeHex(fd, address);
		}
		writeText(fd, "\n");
			//For an access violation, what was accessed: a small address
			//such as 0x10 is a null pointer's member, a large one a
			//dangling or corrupted pointer.
		if (g_crash_code == EXCEPTION_ACCESS_VIOLATION
				&& g_crash_record->NumberParameters >= 2) {
			const ULONG_PTR kind = g_crash_record->ExceptionInformation[0];
			writeText(fd, kind == 0 ? "Reading: "
					: kind == 1 ? "Writing: "
					: "Executing: ");
			writeHex(fd, g_crash_record->ExceptionInformation[1]);
			writeText(fd, "\n");
		}
	} else if (g_crash_kind == CrashKind::Fatal) {
		writeText(fd, "Fatal: qFatal() (its message is the last Fatal line of the log)\n");
	} else {
		writeText(fd, "Signal: SIGABRT (abort)\n");
	}

	writeText(fd, "--- log ---\n");
	if (g_ring) {
		g_ring->dumpToFd(fd);
	}

#if defined(_M_X64) || defined(__x86_64__)
	writeText(fd, "--- backtrace ---\n");
	writeBacktrace(fd, &g_crash_context);
#endif

	_close(fd);
}

DWORD WINAPI reporterThread(LPVOID)
{
	WaitForSingleObject(g_reporter_go, INFINITE);
	writeWindowsDump();
	SetEvent(g_reporter_done);
	return 0;
}

/**
	Hands the crash to the reporter thread and waits for it. Uses almost no
	stack of its own, which is what lets a stack overflow be reported.
*/
void reportFromCrashingThread()
{
	const NT_TIB *tib = reinterpret_cast<const NT_TIB *>(NtCurrentTeb());
	g_crash_stack_low = reinterpret_cast<DWORD64>(tib->StackLimit);
	g_crash_stack_high = reinterpret_cast<DWORD64>(tib->StackBase);

	if (g_reporter_go && g_reporter_done) {
		SetEvent(g_reporter_go);
		WaitForSingleObject(g_reporter_done, kReporterTimeoutMs);
	}
}

LONG WINAPI windowsExceptionFilter(EXCEPTION_POINTERS *pointers)
{
	bool expected = false;
	if (!g_already_dumped.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	g_crash_kind = CrashKind::Exception;
	g_crash_record = pointers->ExceptionRecord;
	g_crash_code = pointers->ExceptionRecord->ExceptionCode;
	g_crash_context = *pointers->ContextRecord;
	reportFromCrashingThread();

	// Do not suppress Windows Error Reporting / an attached debugger --
	// same invariant as re-raising on POSIX (see crashhandler.h,
	// invariant 3).
	return EXCEPTION_CONTINUE_SEARCH;
}

/**
	abort() -- which std::terminate() and a failed assert end in -- does
	not raise an SEH exception on Windows, so the filter above never sees
	it. The C runtime raises SIGABRT first, though. (qFatal() does not
	get here: see CrashHandler::reportFatal().)
*/
void windowsAbortHandler(int)
{
	bool expected = false;
	if (g_already_dumped.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
		g_crash_kind = CrashKind::Abort;
		RtlCaptureContext(&g_crash_context);
		reportFromCrashingThread();
	}
		//Let abort() carry on to its default end, so Windows Error
		//Reporting still sees the crash (invariant 3).
	signal(SIGABRT, SIG_DFL);
}

#endif

} // namespace

// Async-signal-safe decimal formatting: write() takes a buffer, and there
// is no snprintf on the POSIX async-signal-safe list. Writes into a
// caller-owned buffer (stack, not heap) and returns the length used.
//
// Defined as CrashHandler::formatInt rather than a file-local helper only
// so tst_crashhandler can reach it; it is not called anywhere else.
int CrashHandler::formatInt(char *buffer, int size, int value)
{
	if (size <= 0) return 0;
	if (value == 0) {
		buffer[0] = '0';
		return 1;
	}
	char scratch[16];
	int n = 0;
	bool negative = value < 0;
	unsigned int v = negative ? static_cast<unsigned int>(-(value + 1)) + 1u
				  : static_cast<unsigned int>(value);
	while (v > 0 && n < static_cast<int>(sizeof(scratch))) {
		scratch[n++] = static_cast<char>('0' + (v % 10));
		v /= 10;
	}
	int len = 0;
	if (negative && len < size) buffer[len++] = '-';
	while (n > 0 && len < size) buffer[len++] = scratch[--n];
	return len;
}

void CrashHandler::reportFatal()
{
#ifdef Q_OS_WIN
	bool expected = false;
	if (g_already_dumped.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
		g_crash_kind = CrashKind::Fatal;
		RtlCaptureContext(&g_crash_context);
		reportFromCrashingThread();
	}
#endif
}

// Same constraints as formatInt(): caller-owned buffer, no allocation.
// Always writes the "0x" prefix and at least one digit when there is room.
int CrashHandler::formatHex(char *buffer, int size, unsigned long long value)
{
	if (size <= 0) return 0;
	char scratch[16];
	int n = 0;
	do {
		scratch[n++] = "0123456789abcdef"[value & 0xf];
		value >>= 4;
	} while (value != 0 && n < static_cast<int>(sizeof(scratch)));

	int len = 0;
	const char kPrefix[] = "0x";
	for (unsigned i = 0 ; i < sizeof(kPrefix) - 1 && len < size ; ++i) {
		buffer[len++] = kPrefix[i];
	}
	while (n > 0 && len < size) buffer[len++] = scratch[--n];
	return len;
}

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
		//The reporter thread and its two events are made here, in normal
		//context, because nothing can be created once the crash happens.
		//The thread only waits; its 64 KiB is reserved, not committed.
	g_reporter_go = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	g_reporter_done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (g_reporter_go && g_reporter_done) {
		HANDLE thread = CreateThread(nullptr, 65536, reporterThread, nullptr,
					     STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
		if (thread) {
			CloseHandle(thread);
		} else {
			CloseHandle(g_reporter_go);
			CloseHandle(g_reporter_done);
			g_reporter_go = g_reporter_done = nullptr;
		}
	}

		//After a stack overflow the filter still needs a little stack to
		//hand over to the reporter. Keep some in reserve on the main
		//thread, where the GUI's deep recursions happen. install() runs on
		//it, and the guarantee only applies to the calling thread.
	ULONG stack_guarantee = 32768;
	SetThreadStackGuarantee(&stack_guarantee);

	SetUnhandledExceptionFilter(windowsExceptionFilter);
	signal(SIGABRT, windowsAbortHandler);
#else
	stack_t ss;
	ss.ss_sp = g_altstack;
	ss.ss_size = sizeof(g_altstack);
	ss.ss_flags = 0;
	sigaltstack(&ss, nullptr);

#ifdef QET_CRASH_BACKTRACE
	// Warm the unwinder. backtrace()'s *first* call resolves dynamic
	// linker state and may allocate; every call after that does not. Doing
	// it here, in normal context, is what lets the handler call it without
	// breaking invariant 2. The result is deliberately discarded.
	void *warmup[4];
	(void) ::backtrace(warmup, 4);
#endif

	struct sigaction sa {};
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_ONSTACK;

	for (int sig : kHandledSignals) {
		sigaction(sig, &sa, nullptr);
	}
#endif
}
