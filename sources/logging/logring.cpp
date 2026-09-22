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
#include "logring.h"

#include <cstring>

#ifdef Q_OS_WIN
#include <io.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

static_assert(std::atomic<int>::is_always_lock_free,
	      "LogRing::Entry::length must be a lock-free atomic<int> -- "
	      "dumpToFd() reads it from a signal handler and must never block.");

namespace {

/**
	@brief writeAllSignalSafe
	Loops until length bytes have been written to fd or an unrecoverable
	error occurs. write(2) may write fewer bytes than requested and may
	return EINTR -- both are *more* likely from inside a signal handler
	than in normal code, so a single write() call is not enough here.
	Async-signal-safe: only calls write(2)/errno, nothing else.
*/
void writeAllSignalSafe(int fd, const char *data, int length) noexcept
{
	int remaining = length;
	const char *p = data;

	while (remaining > 0) {
#ifdef Q_OS_WIN
		const int n = _write(fd, p, static_cast<unsigned int>(remaining));
		if (n <= 0) {
			return;
		}
#else
		const ssize_t n = ::write(fd, p, static_cast<size_t>(remaining));
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			return; // unrecoverable -- give up silently, never block/throw
		}
		if (n == 0) {
			return;
		}
#endif
		p += n;
		remaining -= static_cast<int>(n);
	}
}

} // namespace

LogRing::LogRing() :
	// The sized constructor value-initialises each Entry in place; unlike
	// resize(), it doesn't require Entry to be move/copy-constructible,
	// which std::atomic<int> deliberately never is. The only allocation
	// this class ever does.
	m_entries(kCapacityEntries)
{
}

void LogRing::append(const QByteArray &line) noexcept
{
	static const char kMarker[] = "...[ring-truncated]\n";
	const int marker_len = static_cast<int>(sizeof(kMarker)) - 1;

	const quint64 idx = m_write_cursor.fetch_add(1, std::memory_order_relaxed);
	Entry &slot = m_entries[static_cast<size_t>(idx % static_cast<quint64>(kCapacityEntries))];

	// Zero the length first so a concurrent reader landing on this exact
	// slot mid-copy sees "not ready" rather than the previous lap's
	// (now-being-overwritten) content at a stale length.
	slot.length.store(0, std::memory_order_relaxed);

	int len;
	if (line.size() < kEntryBytes) {
		std::memcpy(slot.data, line.constData(), static_cast<size_t>(line.size()));
		len = line.size();
	} else {
		const int keep = kEntryBytes - marker_len;
		std::memcpy(slot.data, line.constData(), static_cast<size_t>(keep));
		std::memcpy(slot.data + keep, kMarker, static_cast<size_t>(marker_len));
		len = kEntryBytes;
	}

	slot.length.store(len, std::memory_order_release);
}

QVector<QByteArray> LogRing::snapshot() const
{
	const quint64 cursor = m_write_cursor.load(std::memory_order_acquire);
	const quint64 cap = static_cast<quint64>(kCapacityEntries);
	const quint64 count = (cursor < cap) ? cursor : cap;
	const quint64 start = (cursor < cap) ? 0 : (cursor - cap);

	QVector<QByteArray> result;
	result.reserve(static_cast<int>(count));

	for (quint64 i = 0; i < count; ++i) {
		const Entry &slot = m_entries[static_cast<size_t>((start + i) % cap)];
		const int len = slot.length.load(std::memory_order_acquire);
		if (len > 0) {
			result.append(QByteArray(slot.data, len));
		}
	}

	return result;
}

void LogRing::dumpToFd(int fd) const noexcept
{
	const quint64 cursor = m_write_cursor.load(std::memory_order_acquire);
	const quint64 cap = static_cast<quint64>(kCapacityEntries);
	const quint64 count = (cursor < cap) ? cursor : cap;
	const quint64 start = (cursor < cap) ? 0 : (cursor - cap);

	for (quint64 i = 0; i < count; ++i) {
		const Entry &slot = m_entries[static_cast<size_t>((start + i) % cap)];
		const int len = slot.length.load(std::memory_order_acquire);
		if (len > 0) {
			writeAllSignalSafe(fd, slot.data, len);
		}
	}
}

void LogRing::clear()
{
	m_write_cursor.store(0, std::memory_order_relaxed);
	for (auto &entry : m_entries) {
		entry.length.store(0, std::memory_order_relaxed);
	}
}
