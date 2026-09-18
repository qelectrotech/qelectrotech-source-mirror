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
#ifndef EVENTLOOPWATCHDOG_H
#define EVENTLOOPWATCHDOG_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

/**
	@brief The EventLoopWatchdog class
	Detects when the main (GUI) thread's event loop goes unresponsive --
	QetLogger (discussion #644) can only see what an explicit qDebug()/
	qInfo()/qWarning() call already decided to report, and most of a
	session (painting, dragging, a slow synchronous operation) produces
	no log output at all, so a silent multi-second gap in the log is
	indistinguishable from the user simply not doing anything.

	This closes that gap the direct way: a repeating QTimer::PreciseTimer
	ticks on a short, fixed interval; each tick measures the *actual*
	wall-clock time elapsed since the previous one via QElapsedTimer
	(monotonic -- unaffected by system clock/NTP adjustments, unlike
	QDateTime). Qt does not queue up missed fires for a normal repeating
	timer, so if the event loop is blocked for 600ms, the timer fires
	once as soon as the loop frees up, with ~600ms measured since the
	last tick -- that gap *is* the stall, measured at its source rather
	than inferred from log silence.

	Only fires a qWarning() (and so only touches the log at all) when a
	tick is late by more than kStallThresholdMs, to stay within the
	spirit of QetLogger's bounded-log design (see its class comment) --
	a healthy session should produce zero output from this class. This
	tells you *that* a stall happened and *how long* it was, not what
	caused it; pair a reported timestamp with `docker exec`+gdb the way
	the CLI hang (PR #661) was diagnosed to go from "it lagged" to a
	root cause.

	Escape hatch: if QET_WATCHDOG_DISABLE=1 is set in the environment at
	construction time, start() does nothing.
*/
class EventLoopWatchdog : public QObject
{
		Q_OBJECT

	public:
		/// How often the watchdog checks in. Small enough to bound the
		/// measurement's own granularity, large enough that the tick
		/// itself is negligible overhead on the event loop it's watching.
		static constexpr int kTickIntervalMs = 50;

		/// A tick arriving later than this many ms after the previous one
		/// is logged as a stall. Comfortably above kTickIntervalMs so
		/// ordinary OS scheduling noise doesn't produce a warning on every
		/// tick, and in the range a user would actually notice as lag.
		static constexpr int kStallThresholdMs = 200;

		explicit EventLoopWatchdog(QObject *parent = nullptr);

		/// Starts ticking. Must be called from the main thread, after the
		/// event loop it watches is about to run (i.e. immediately before
		/// QApplication::exec()) -- constructing this class earlier is
		/// harmless, but start() before there is an event loop to tick
		/// against would just measure the time until app.exec() is
		/// reached. No-op if QET_WATCHDOG_DISABLE=1 was set at
		/// construction time.
		void start();

	private slots:
		void tick();

	private:
		QTimer m_timer;
		QElapsedTimer m_elapsed;
		bool m_disabled = false;
};

#endif // EVENTLOOPWATCHDOG_H
