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
#ifndef SPACEMOUSELISTENER_H
#define SPACEMOUSELISTENER_H

#include "spacemousemotion.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPointF>

class QGraphicsView;
class SpaceMouseBackend;

/**
	@brief The SpaceMouseListener class
	https://github.com/qelectrotech/qelectrotech-source-mirror/discussions/599 :
	bridges a 3Dconnexion SpaceMouse/SpacePilot 6-DOF device to the
	existing pan/zoom primitives of DiagramView and ElementView (the same
	horizontalScrollBar()/verticalScrollBar()/zoom() calls their
	wheelEvent() already uses for a physical wheel), and its
	buttons to named QET actions via ShortcutManager -- the same registry
	keyboard shortcuts already use, so a device button can trigger anything
	in that registry (undo, redo, rotate selection, ...) without QET having
	a second, device-specific action list.

	Everything here is platform-independent: which view to apply
	motion to, the pan/zoom calls, the sample-to-motion mapping
	(SpaceMouseMotion, tuned by the user's SpaceMouseSettings), and button
	dispatch via SpaceMouseButtonMap + ShortcutManager. Talking to the
	actual device driver is a SpaceMouseBackend's job (see its class
	comment) -- this class owns one and applies whatever it reports,
	without knowing or caring which platform it came from.

	Only compiled in when QET_SPACEMOUSE_SUPPORT is defined. Even then,
	constructing one is always safe: if no backend is available for this
	platform, or the one that exists can't reach a driver/daemon (no
	device attached -- the expected state for the overwhelming majority of
	users, even of a build with the option on), this silently does nothing
	rather than failing or nagging the user. There is exactly one of
	these, owned by QETApp, because a physical 6-DOF device is a single
	ambient input source for the whole application, not something tied to
	one window.
*/
class SpaceMouseListener : public QObject
{
	Q_OBJECT

	public:
		explicit SpaceMouseListener(QObject *parent = nullptr);
		~SpaceMouseListener() override = default;

			/// True once the platform backend has a live connection to a
			/// driver/daemon. False is the common case, not an error -- see
			/// the class comment -- so callers should not warn the user
			/// when this is false.
		bool isAvailable() const;

			/// Re-read SpaceMouseSettings, after the configuration page
			/// saved new ones.
		void reloadSettings();

	private slots:
			/// Apply one motion sample -- from whichever backend is in use
			/// -- to the view of the active diagram or element editor.
		void applyMotion(const SpaceMouseSample &sample);

			/// Look up which action id, if any, SpaceMouseButtonMap binds
			/// \a button to, and trigger it via ShortcutManager. Does
			/// nothing for an unbound button -- see SpaceMouseButtonMap's
			/// class comment on the "unbound by default" contract.
		void applyButton(int button);

	private:
		void scrollView(QGraphicsView *view, qreal dx, qreal dy);

		SpaceMouseBackend *m_backend = nullptr;
		SpaceMouseSettings m_settings;
			/// Time since the previous motion sample -- see
			/// SpaceMouseMotion::stepFor().
		QElapsedTimer m_since_last_sample;
			/// Fractions of a pixel not yet scrolled. Scrollbars only take
			/// whole pixels; without this a slow push that maps to under
			/// half a pixel per sample would never move the view at all.
		QPointF m_scroll_remainder;
};

#endif // SPACEMOUSELISTENER_H
