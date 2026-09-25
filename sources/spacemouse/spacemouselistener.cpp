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

#include "spacemousebackend.h"
#include "spacemousebuttonmap.h"
#ifdef QET_SPACEMOUSE_BACKEND_SPNAV
#	include "spnavbackend.h"
#endif

#include "../diagramview.h"
#include "../editor/elementview.h"
#include "../editor/ui/qetelementeditor.h"
#include "../projectview.h"
#include "../qetdiagrameditor.h"
#include "../shortcutmanager.h"

#include <QApplication>
#include <QScrollBar>

namespace {
		//A device motion delta is an integer on roughly the same order of
		//magnitude as a QWheelEvent::angleDelta() tick (about +-120 per
		//detent, more under a hard push/twist -- exact range depends on the
		//backend and the user's own driver-level sensitivity setting, which
		//is configured outside QET and out of scope here).
		//DiagramView::wheelEvent() already turns such a tick into a small
		//per-event zoom step via zoom(1 + value/1000); reused as a starting
		//point.
		//
		//Neither this divisor nor PAN_SCALE below has been calibrated
		//against real hardware -- there is none in the environment this was
		//built in. Both are named constants specifically so that is a
		//one-line fix once someone with a device tries it.
	constexpr qreal ZOOM_DIVISOR = 1000.0;
	constexpr qreal PAN_SCALE = 1.0;

		//Pan a view by one motion sample, through its scrollbars -- the same
		//way both editors' own middle-button drag pans them.
	void panView(QGraphicsView *view, int dx, int dy)
	{
		view->horizontalScrollBar()->setValue(
			view->horizontalScrollBar()->value() - qRound(dx * PAN_SCALE));
		view->verticalScrollBar()->setValue(
			view->verticalScrollBar()->value() - qRound(dy * PAN_SCALE));
	}
}

/**
	@brief SpaceMouseListener::SpaceMouseListener
	Construct whichever backend is available for this platform and connect
	its motion() signal. If none is compiled in, or the one that is can't
	reach a device, isAvailable() simply stays false -- see the class
	comment.
	@param parent
*/
SpaceMouseListener::SpaceMouseListener(QObject *parent) :
	QObject(parent)
{
#ifdef QET_SPACEMOUSE_BACKEND_SPNAV
	m_backend = new SpnavBackend(this);
#endif
		//A future Windows/macOS backend (3Dconnexion's proprietary 3DxWare
		//SDK) slots in here behind its own QET_SPACEMOUSE_BACKEND_3DXWARE
		//guard, without changing anything below this constructor.

	if (m_backend) {
		connect(m_backend, &SpaceMouseBackend::motion,
			this, &SpaceMouseListener::applyMotion);
		connect(m_backend, &SpaceMouseBackend::buttonPressed,
			this, &SpaceMouseListener::applyButton);
	}
}

/**
	@brief SpaceMouseListener::isAvailable
	@return whether the platform backend has a live device connection
*/
bool SpaceMouseListener::isAvailable() const
{
	return m_backend && m_backend->isAvailable();
}

/**
	@brief SpaceMouseListener::zoomFactorForZAxis
	@param z : raw Z-axis (push/pull) delta from a device motion sample
	@return the multiplicative factor DiagramView::zoom() expects
*/
qreal SpaceMouseListener::zoomFactorForZAxis(int z)
{
	return 1.0 + (static_cast<qreal>(z) / ZOOM_DIVISOR);
}

/**
	@brief SpaceMouseListener::applyMotion
	Apply one motion sample to the view of the active window: the current
	folio of a diagram editor, or the drawing of an element editor.
	X/Y translation pans it, Z translation zooms it -- the same two
	primitives (scrollbars, zoom()) each view's wheelEvent() already drives
	from a physical wheel, so there is no new navigation logic here, only a
	new input source feeding the existing one.

	Which of a device's three translation axes is "left/right" vs
	"forward/back" vs "up/down", and their sign, is a hardware convention
	this could not be checked against real hardware while writing it -- see
	the PR description.
	@param dx
	@param dy
	@param dz
*/
void SpaceMouseListener::applyMotion(int dx, int dy, int dz)
{
	QWidget *window = qApp->activeWindow();

	if (auto *editor = qobject_cast<QETDiagramEditor *>(window))
	{
		ProjectView *project_view = editor->currentProjectView();
		if (!project_view) {
			return;
		}

		DiagramView *view = project_view->currentDiagram();
		if (!view) {
			return;
		}

		if (dx || dy) {
			panView(view, dx, dy);
		}
		if (dz) {
			view->zoom(zoomFactorForZAxis(dz));
		}
	}
	else if (auto *element_editor = qobject_cast<QETElementEditor *>(window))
	{
		ElementView *view = element_editor->elementView();
		if (!view) {
			return;
		}

		if (dx || dy)
		{
				//The element editor's scene rect only just covers what is
				//on screen, so grow it before each sample, as its own
				//middle-button pan does on release -- otherwise the
				//scrollbars have no range and the pan does nothing.
			view->adjustSceneRect();
			panView(view, dx, dy);
		}
		if (dz) {
			view->zoom(zoomFactorForZAxis(dz));
		}
	}
}

/**
	@brief SpaceMouseListener::applyButton
	@param button
*/
void SpaceMouseListener::applyButton(int button)
{
	const QString action_id = SpaceMouseButtonMap::actionId(button);
	if (!action_id.isEmpty()) {
		ShortcutManager::instance().trigger(action_id);
	}
}
