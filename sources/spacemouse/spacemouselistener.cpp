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
#ifdef QET_SPACEMOUSE_BACKEND_HID
#	include "hidbackend.h"
#endif
#ifdef QET_SPACEMOUSE_BACKEND_CONNEXION
#	include "connexionbackend.h"
#endif

#include "../diagramview.h"
#include "../editor/elementview.h"
#include "../editor/ui/qetelementeditor.h"
#include "../projectview.h"
#include "../qetdiagrameditor.h"
#include "../shortcutmanager.h"

#include <QApplication>
#include <QScrollBar>

/**
	@brief SpaceMouseListener::SpaceMouseListener
	Construct whichever backend is available for this platform and connect
	its motion() signal. If none is compiled in, or the one that is can't
	reach a device, isAvailable() simply stays false -- see the class
	comment.
	@param parent
*/
SpaceMouseListener::SpaceMouseListener(QObject *parent) :
	QObject(parent),
	m_settings(SpaceMouseSettings::load())
{
#if defined(QET_SPACEMOUSE_BACKEND_CONNEXION)
		//When 3DxWare is installed and running it has the device to
		//itself, so ask it first; otherwise read the device directly.
	auto *connexion = new ConnexionBackend(this);
	if (connexion->isAvailable()) {
		m_backend = connexion;
	} else {
		delete connexion;
	}
#endif
#if defined(QET_SPACEMOUSE_BACKEND_SPNAV)
	if (!m_backend) {
		m_backend = new SpnavBackend(this);
	}
#elif defined(QET_SPACEMOUSE_BACKEND_HID)
	if (!m_backend) {
		m_backend = new HidBackend(this);
	}
#endif

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
	@brief SpaceMouseListener::reloadSettings
*/
void SpaceMouseListener::reloadSettings()
{
	m_settings = SpaceMouseSettings::load();
}

/**
	@brief SpaceMouseListener::applyMotion
	Apply one motion sample to the view of the active window: the current
	folio of a diagram editor, or the drawing of an element editor.
	Translation pans it and push/pull (or twist, per the user's settings)
	zooms it -- the same two primitives (scrollbars, zoom()) each view's
	wheelEvent() already drives from a physical wheel, so there is no new
	navigation logic here, only a new input source feeding the existing one.
	@param sample
*/
void SpaceMouseListener::applyMotion(const SpaceMouseSample &sample)
{
	const qint64 elapsed_ms = m_since_last_sample.isValid()
			? m_since_last_sample.restart()
			: -1;
	if (!m_since_last_sample.isValid()) {
		m_since_last_sample.start();
	}
	if (elapsed_ms < 0 || elapsed_ms > SpaceMouseMotion::MAX_PERIOD_MS) {
			//the device was at rest: nothing left over to carry on with
		m_scroll_remainder = QPointF();
	}

	const SpaceMouseViewMotion motion =
			SpaceMouseMotion::map(sample, elapsed_ms, m_settings);
	const bool pans = motion.scroll_x != 0 || motion.scroll_y != 0;
	const bool zooms = motion.zoom_factor != 1.0;
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

		if (pans) {
			scrollView(view, motion.scroll_x, motion.scroll_y);
		}
		if (zooms) {
			view->zoom(motion.zoom_factor);
		}
	}
	else if (auto *element_editor = qobject_cast<QETElementEditor *>(window))
	{
		ElementView *view = element_editor->elementView();
		if (!view) {
			return;
		}

		if (pans)
		{
				//The element editor's scene rect only just covers what is
				//on screen, so grow it before each sample, as its own
				//middle-button pan does on release -- otherwise the
				//scrollbars have no range and the pan does nothing.
			view->adjustSceneRect();
			scrollView(view, motion.scroll_x, motion.scroll_y);
		}
		if (zooms) {
			view->zoom(motion.zoom_factor);
		}
	}
}

/**
	@brief SpaceMouseListener::scrollView
	Move a view's scrollbars by (\a dx, \a dy) pixels -- the same way both
	editors' own middle-button drag pans them -- keeping the fractional part
	for the next sample.
	@param view
	@param dx
	@param dy
*/
void SpaceMouseListener::scrollView(QGraphicsView *view, qreal dx, qreal dy)
{
	m_scroll_remainder += QPointF(dx, dy);
	const int whole_x = qRound(m_scroll_remainder.x());
	const int whole_y = qRound(m_scroll_remainder.y());
	m_scroll_remainder -= QPointF(whole_x, whole_y);

	view->horizontalScrollBar()->setValue(view->horizontalScrollBar()->value() + whole_x);
	view->verticalScrollBar()->setValue(view->verticalScrollBar()->value() + whole_y);
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
