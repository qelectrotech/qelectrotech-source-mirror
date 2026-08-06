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
#include <QMutex>
#include <QVector>
#include <vector>

/**
	@brief The LogRing class
	Fixed-capacity, always-on in-memory ring of the most recent log
	lines. Discussion #644 (step 3): the ring exists as forward-compatible
	infrastructure for a future crash-flush (step 4, not implemented
	here) as well as an on-demand "what just happened" snapshot, so its
	entries are stored pre-formatted as plain bytes in storage
	preallocated once at construction -- append() never allocates.

	Entries are fixed-size slots rather than a byte-packed ring: with
	kCapacityEntries * kEntryBytes chosen to land exactly on the 2 MiB
	budget, this keeps wraparound trivial (whole-slot overwrite, so a
	slot is always either fully the old entry or fully the new one --
	no torn entries) at the cost of truncating any single line to
	kEntryBytes, independently of the logger's own (larger) per-message
	truncation.

	Thread-safe via a plain QMutex. This is *not* the lock-free design
	discussion #644 specifies for a signal-handler crash path (step 4)
	-- no signal handler is installed by this code, so nothing calls
	into the ring from inside a signal context.
*/
class LogRing
{
	public:
		static constexpr int kCapacityEntries = 4096;
		static constexpr int kEntryBytes = 512; // 4096 * 512 = 2 MiB total

		LogRing();

		/// Append one already-formatted, already-truncated log line.
		/// Bytes beyond kEntryBytes - 1 are dropped with a truncation marker.
		void append(const QByteArray &line);

		/// Snapshot of the entries currently held, oldest first.
		QVector<QByteArray> snapshot() const;

		void clear();

	private:
		struct Entry {
			char data[kEntryBytes] = {};
			int length = 0;
		};

		mutable QMutex m_mutex;
		std::vector<Entry> m_entries; // preallocated once, capacity fixed
		int m_next_index = 0;
		int m_count = 0;
};

#endif // LOGRING_H
