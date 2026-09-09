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
#ifndef LOGRING_H
#define LOGRING_H

#include <QByteArray>
#include <QVector>
#include <atomic>
#include <vector>

/**
	@brief The LogRing class
	Fixed-capacity, always-on in-memory ring of the most recent log
	lines, preallocated once at construction -- append() never
	allocates.

	Lock-free by construction, not just "thread-safe": step 4 (see
	crashhandler.h) reads this ring from inside a POSIX signal handler,
	where taking any lock is unsafe -- if the crashing thread happens to
	be the one that already holds it (or any other thread does and never
	gets scheduled again), the handler hangs forever, and you lose both
	the ring dump *and* the core dump. So there is no mutex here at all:
	append() claims a slot with a single atomic fetch-add, and
	dumpToFd()/snapshot() read the preallocated entries directly.

	Accepted tradeoff: if dumpToFd() runs while another thread is
	mid-append into the exact slot being read (only possible in the
	crash-handler case, and only for at most one slot), that one entry
	may be read torn -- part old content, part new. Every other entry is
	unaffected. This is deliberate: the alternative (a seqlock or similar
	to detect and retry torn reads) adds real complexity for a window
	that, per discussion #644, is not worth trading "the handler must
	never block" against.
*/
class LogRing
{
	public:
		static constexpr int kCapacityEntries = 4096;
		static constexpr int kEntryBytes = 512; // 4096 * 512 = 2 MiB total

		LogRing();

		/// Append one already-formatted, already-truncated log line.
		/// Bytes beyond kEntryBytes are dropped with a truncation marker.
		/// Never allocates, never blocks. Safe to call from any normal
		/// (non-signal) thread concurrently.
		void append(const QByteArray &line) noexcept;

		/// Snapshot of the entries currently held, oldest first. Normal
		/// (non-signal) context only.
		QVector<QByteArray> snapshot() const;

		/// Async-signal-safe: writes every entry currently held to fd via
		/// write(2) only -- no allocation, no Qt, no locks. May write a
		/// torn entry under the rare race described above; never blocks.
		void dumpToFd(int fd) const noexcept;

		void clear();

	private:
		struct Entry {
			char data[kEntryBytes];
			std::atomic<int> length{0}; // 0 = not yet written this lap
		};

		std::vector<Entry> m_entries; // preallocated once, capacity fixed
		std::atomic<quint64> m_write_cursor{0}; // monotonically increasing
};

#endif // LOGRING_H
