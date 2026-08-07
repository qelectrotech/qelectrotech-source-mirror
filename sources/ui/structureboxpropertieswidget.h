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
#ifndef STRUCTUREBOXPROPERTIESWIDGET_H
#define STRUCTUREBOXPROPERTIESWIDGET_H

#include "../PropertiesEditor/propertieseditorwidget.h"

class QLineEdit;
class StructureBoxItem;

/**
	@brief The StructureBoxPropertiesWidget class
	Provide a widget to edit the plant/location/prefix identity
	of a StructureBoxItem.
*/
class StructureBoxPropertiesWidget : public PropertiesEditorWidget
{
		Q_OBJECT

	public:
		explicit StructureBoxPropertiesWidget(StructureBoxItem *item, QWidget *parent = nullptr);

		void apply() override;
		void reset() override;
		void updateUi() override;
		QString title() const override { return tr("Éditer les propriétés d'un cadre de repérage"); }

	private:
		StructureBoxItem *m_item;
		QLineEdit *m_plant_edit;
		QLineEdit *m_location_edit;
		QLineEdit *m_prefix_edit;
};

#endif // STRUCTUREBOXPROPERTIESWIDGET_H
