/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CABLELISTMODELPROPERTIESWIDGET_H
#define CABLELISTMODELPROPERTIESWIDGET_H

#include "../../PropertiesEditor/propertieseditorwidget.h"

class CableListModel;
class QPushButton;

/**
	@brief The CableListModelPropertiesWidget class
	The editor shown when a cable list table is selected: change which
	columns the table shows, or work its rows out again. Shaped after
	ProjectDBModelPropertiesWidget, which changes its query the same
	immediate, not-undoable way -- the table itself is undoable to move
	or delete, its content always follows the cables.
*/
class CableListModelPropertiesWidget : public PropertiesEditorWidget
{
	Q_OBJECT

	public:
		explicit CableListModelPropertiesWidget(CableListModel *model = nullptr,
												QWidget *parent = nullptr);
		~CableListModelPropertiesWidget() override = default;

		void setModel(CableListModel *model);

	private slots:
		void editColumns();
		void refreshRows();

	private:
		CableListModel *m_model = nullptr;
		QPushButton *m_edit_pb = nullptr;
		QPushButton *m_refresh_pb = nullptr;
};

#endif // CABLELISTMODELPROPERTIESWIDGET_H
