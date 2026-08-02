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
#include "spacemouselistener.h"

#include "../diagramview.h"
#include "../projectview.h"
#include "../qetdiagrameditor.h"

#include <QApplication>
#include <QScrollBar>
#include <QSocketNotifier>

#include <spnav.h>

namespace {
		//A spnav motion delta is an integer on roughly the same order of
		//magnitude as a QWheelEvent::angleDelta() tick (about +-120 per
		//detent, more under a hard push/twist -- exact range depends on the
		//user's spacenavd sensitivity setting, which is configured once
		//outside QET and out of scope here). DiagramView::wheelEvent()
		//already turns such a tick into a small per-event zoom step via
		//zoom(1 + value/1000); reused as a starting point.
		//
		//Neither this divisor nor PAN_SCALE below has been calibrated
		//against real hardware -- there is none in the environment this was
		//built in. Both are named constants specifically so that is a
		//one-line fix once someone with a device tries it.
	constexpr qreal ZOOM_DIVISOR = 1000.0;
	constexpr qreal PAN_SCALE = 1.0;
}

/**
	@brief SpaceMouseListener::SpaceMouseListener
	Try to connect to spacenavd. Failure -- no daemon running, no device
	attached -- is left silent: it is the expected state for most users and
	must never surface as an error dialog or a log warning on every
	ordinary startup.
	@param parent
*/
SpaceMouseListener::SpaceMouseListener(QObject *parent) :
	QObject(parent)
{
	if (spnav_open() == -1) {
		return;
	}

	m_available = true;
	m_notifier = new QSocketNotifier(spnav_fd(), QSocketNotifier::Read, this);
	connect(m_notifier, &QSocketNotifier::activated,
		this, &SpaceMouseListener::readEvents);
}

/**
	@brief SpaceMouseListener::~SpaceMouseListener
*/
SpaceMouseListener::~SpaceMouseListener()
{
	if (m_available) {
		spnav_close();
	}
}

/**
	@brief SpaceMouseListener::zoomFactorForZAxis
	@param z : raw Z-axis (push/pull) delta from a spnav motion event
	@return the multiplicative factor DiagramView::zoom() expects
*/
qreal SpaceMouseListener::zoomFactorForZAxis(int z)
{
	return 1.0 + (static_cast<qreal>(z) / ZOOM_DIVISOR);
}

/**
	@brief SpaceMouseListener::readEvents
	Called when the spacenavd socket has data available. Drains every event
	currently queued -- spnav_poll_event() returns 0 once the queue is
	empty -- rather than handling just one per activation, so events cannot
	silently back up if several arrive between two Qt event loop turns.
*/
void SpaceMouseListener::readEvents()
{
	spnav_event event;
	while (spnav_poll_event(&event))
	{
		if (event.type == SPNAV_EVENT_MOTION) {
			dispatchMotion(event.motion);
		}
			//Button events (SPNAV_EVENT_BUTTON) are deliberately not
			//handled: mapping device buttons to QET actions is the "Related,
			//not proposed here" follow-up in discussion #599, not this
			//phase.
	}
}

/**
	@brief SpaceMouseListener::dispatchMotion
	Apply one motion event to whichever DiagramView is currently active.
	X/Y translation pans it, Z translation zooms it -- the same two
	primitives (scrollbars, DiagramView::zoom()) DiagramView::wheelEvent()
	already drives from a physical wheel, so there is no new navigation
	logic here, only a new input source feeding the existing one.

	A 6-DOF device also reports rotation (rx, ry, rz); QET's view has
	nothing rotation maps to, so those three axes are read by nothing here.

	Which of the three translation axes is "left/right" vs "forward/back"
	vs "up/down" on the physical device, and their sign, is a hardware
	convention this could not be checked against real hardware while
	writing it -- see the PR description.
	@param motion
*/
void SpaceMouseListener::dispatchMotion(const spnav_event_motion &motion)
{
	auto *editor = qobject_cast<QETDiagramEditor *>(qApp->activeWindow());
	if (!editor) {
		return;
	}

	ProjectView *project_view = editor->currentProjectView();
	if (!project_view) {
		return;
	}

	DiagramView *view = project_view->currentDiagram();
	if (!view) {
		return;
	}

	if (motion.x || motion.y)
	{
		view->horizontalScrollBar()->setValue(
			view->horizontalScrollBar()->value() - qRound(motion.x * PAN_SCALE));
		view->verticalScrollBar()->setValue(
			view->verticalScrollBar()->value() - qRound(motion.y * PAN_SCALE));
	}

	if (motion.z) {
		view->zoom(zoomFactorForZAxis(motion.z));
	}
}
