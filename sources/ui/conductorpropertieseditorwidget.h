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
#ifndef CONDUCTORPROPERTIESEDITORWIDGET_H
#define CONDUCTORPROPERTIESEDITORWIDGET_H

#include "../PropertiesEditor/propertieseditorwidget.h"
#include "../conductorproperties.h"

#include <QList>
#include <QMetaObject>

class Conductor;
class ConductorPropertiesWidget;
class QCheckBox;

/**
	@brief The ConductorPropertiesEditorWidget class
	Hosts the existing ConductorPropertiesWidget in the dockable selection-
	properties panel, so a selected conductor can be edited in place like the
	other item types, instead of only through the modal dialog (issue #500).
	A pinned "apply to all conductors of the potential" checkbox (persisted)
	mirrors the modal dialog's option to propagate edits to the whole potential.
*/
class ConductorPropertiesEditorWidget : public PropertiesEditorWidget
{
		Q_OBJECT

	public:
		explicit ConductorPropertiesEditorWidget(
			Conductor *conductor = nullptr, QWidget *parent = nullptr);
		~ConductorPropertiesEditorWidget() override;

		void setConductor(Conductor *conductor);

		void apply() override;
		void reset() override;
		void updateUi() override;
		QUndoCommand *associatedUndo() const override;
		QString title() const override;
		bool setLiveEdit(bool live_edit) override;

	private:
		void connectChangeSignals();
		void disconnectChangeSignals();

	private:
		ConductorPropertiesWidget *m_cpw = nullptr;
		QCheckBox *m_apply_all_cb = nullptr;
		Conductor *m_conductor = nullptr;
		ConductorProperties m_initial;
		QList<QMetaObject::Connection> m_live_connections;
		bool m_updating = false;
};

#endif // CONDUCTORPROPERTIESEDITORWIDGET_H
