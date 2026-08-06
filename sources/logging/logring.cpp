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

#include <algorithm>
#include <cstring>

/**
	@brief LogRing::LogRing
	Preallocates all kCapacityEntries slots up front -- the only
	allocation this class ever does.
*/
LogRing::LogRing()
{
	m_entries.resize(kCapacityEntries);
}

/**
	@brief LogRing::append
	@param line one already-formatted log line (no further formatting
	is done here). Truncated to kEntryBytes - 1 bytes if longer, with a
	trailing marker, so the stored entry is always a complete,
	independently-readable line.
*/
void LogRing::append(const QByteArray &line)
{
	static const char kMarker[] = "...[ring-truncated]\n";
	const int marker_len = static_cast<int>(sizeof(kMarker)) - 1;

	QMutexLocker locker(&m_mutex);

	Entry &slot = m_entries[static_cast<size_t>(m_next_index)];

	if (line.size() < kEntryBytes) {
		std::memcpy(slot.data, line.constData(), static_cast<size_t>(line.size()));
		slot.length = line.size();
	} else {
		const int keep = kEntryBytes - marker_len;
		std::memcpy(slot.data, line.constData(), static_cast<size_t>(keep));
		std::memcpy(slot.data + keep, kMarker, static_cast<size_t>(marker_len));
		slot.length = kEntryBytes;
	}

	m_next_index = (m_next_index + 1) % kCapacityEntries;
	if (m_count < kCapacityEntries) {
		++m_count;
	}
}

/**
	@brief LogRing::snapshot
	@return the entries currently held, oldest first. Safe to call from
	normal (non-signal) code only.
*/
QVector<QByteArray> LogRing::snapshot() const
{
	QMutexLocker locker(&m_mutex);

	QVector<QByteArray> result;
	result.reserve(m_count);

	const int start = (m_count < kCapacityEntries) ? 0 : m_next_index;
	for (int i = 0; i < m_count; ++i) {
		const Entry &slot = m_entries[static_cast<size_t>((start + i) % kCapacityEntries)];
		result.append(QByteArray(slot.data, slot.length));
	}

	return result;
}

void LogRing::clear()
{
	QMutexLocker locker(&m_mutex);
	m_next_index = 0;
	m_count = 0;
}
