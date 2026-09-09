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
#include "eventloopwatchdog.h"

#include <QDebug>
#include <QProcessEnvironment>

EventLoopWatchdog::EventLoopWatchdog(QObject *parent) :
	QObject(parent)
{
	m_disabled = QProcessEnvironment::systemEnvironment()
			.value(QStringLiteral("QET_WATCHDOG_DISABLE")) == QStringLiteral("1");

	// Precise, not the default Coarse: Coarse explicitly trades timing
	// accuracy for power/scheduling efficiency (platform-dependent, but
	// commonly +/- a double-digit percentage), which would show up as
	// noise indistinguishable from a real stall in exactly the
	// measurement this class exists to make trustworthy.
	m_timer.setTimerType(Qt::PreciseTimer);
	connect(&m_timer, &QTimer::timeout, this, &EventLoopWatchdog::tick);
}

void EventLoopWatchdog::start()
{
	if (m_disabled)
		return;

	m_elapsed.start();
	m_timer.start(kTickIntervalMs);
}

void EventLoopWatchdog::tick()
{
	// restart() returns the elapsed time and resets the clock in one
	// call, so this tick's own cost is never counted against the next.
	const qint64 actual_ms = m_elapsed.restart();

	if (actual_ms > kStallThresholdMs) {
		qWarning() << "EventLoopWatchdog: main thread stalled for"
			   << actual_ms << "ms (expected a tick every"
			   << kTickIntervalMs << "ms)";
	}
}
