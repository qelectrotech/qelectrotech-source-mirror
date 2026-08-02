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
#ifndef SPACEMOUSECONFIGPAGE_H
#define SPACEMOUSECONFIGPAGE_H

#include "configpage.h"

class QTableWidget;

/**
	@brief The SpaceMouseConfigPage class
	Configuration page listing every device-button-to-action binding
	(SpaceMouseButtonMap), letting the user add, edit and remove them.
	Modelled directly on ShortcutsConfigPage, one row per binding instead
	of one row per shortcut, since the two are close cousins: both end in
	the same place -- ShortcutManager::trigger()/registerAction() -- just
	from a different input source. Bindings are only persisted (via
	SpaceMouseButtonMap::setActionId()) when applyConf() runs, i.e. when
	the user validates the surrounding ConfigDialog.

	Only compiled in, and only added to the Configuration dialog, when
	QET_SPACEMOUSE_SUPPORT is defined -- see qetapp.cpp.

	Button numbering is intentionally not fixed to any particular device:
	it is exactly whatever integer SpaceMouseBackend::buttonPressed()
	reports for the button pressed, which is backend- and device-specific.
	Rows are added/removed freely rather than the page assuming a button
	count, since that varies from 2 (SpaceMouse Compact) to over 30
	(SpacePilot Pro).
*/
class SpaceMouseConfigPage : public ConfigPage
{
		Q_OBJECT

	public:
		explicit SpaceMouseConfigPage(QWidget *parent = nullptr);
		~SpaceMouseConfigPage() override;

		void applyConf() override;
		QString title() const override;
		QIcon icon() const override;

	private slots:
		void addRow();
		void removeSelectedRow();

	private:
		void populateTable();
		void appendRow(int button, const QString &action_id);

		QTableWidget *m_table;
};

#endif // SPACEMOUSECONFIGPAGE_H
