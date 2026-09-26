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
#ifndef SPACEMOUSEBUTTONMAP_H
#define SPACEMOUSEBUTTONMAP_H

#include <QMap>
#include <QString>

/**
	@brief The SpaceMouseButtonMap class
	Persisted mapping from a device button's raw index (backend- and
	device-specific -- see SpaceMouseBackend::buttonPressed()) to a
	ShortcutManager action id. Deliberately just a thin QSettings wrapper,
	the same weight as ShortcutManager::savedSequence(): no caching, reads
	and writes the setting directly on every call, since this is called at
	most once per button press, never in a hot loop.

	Unbound by default for every button on every device: nothing is bound
	until the user opens Configuration > 3D Mouse and binds something --
	see spacemouseconfigpage.h. A device compiled in and connected but
	never configured here does nothing on any button press, matching the
	rest of this feature's "silent until asked for" default.
*/
class SpaceMouseButtonMap
{
	public:
			/// @return the action id bound to \a button, or an empty string
			/// if nothing is bound to it.
		static QString actionId(int button);

			/// Bind \a button to \a action_id. An empty \a action_id
			/// removes the binding (equivalent to it never having been set).
		static void setActionId(int button, const QString &action_id);

			/// @return every currently bound button, for the configuration
			/// page to list.
		static QMap<int, QString> allBindings();
};

#endif // SPACEMOUSEBUTTONMAP_H
