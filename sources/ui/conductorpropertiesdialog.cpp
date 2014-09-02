/*
	Copyright 2006-2014 The QElectroTech Team
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

#include "conductorpropertiesdialog.h"
#include "ui_conductorpropertiesdialog.h"

#include "conductor.h"
#include "conductorpropertieswidget.h"
#include "diagramcommands.h"
#include "diagram.h"

/**
 * @brief ConductorPropertiesDialog::ConductorPropertiesDialog
 * Constructor
 * @param conductor, conductor to edit propertie
 * @param parent, parent widget
 */
ConductorPropertiesDialog::ConductorPropertiesDialog(Conductor *conductor, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConductorPropertiesDialog)
{
	ui->setupUi(this);
	m_cpw = new ConductorPropertiesWidget(conductor->properties());
	ui -> main_layout -> insertWidget(1, m_cpw);
}

/**
 * @brief ConductorPropertiesDialog::~ConductorPropertiesDialog
 */
ConductorPropertiesDialog::~ConductorPropertiesDialog()
{
	delete ui;
}

/**
 * @brief ConductorPropertiesDialog::PropertiesDialog
 * Static method for open and apply properties.
 * @param conductor, conductor to edit propertie
 * @param parent, parent widget
 */
void ConductorPropertiesDialog::PropertiesDialog(Conductor *conductor, QWidget *parent) {
	ConductorPropertiesDialog cpd (conductor, parent);

	if (cpd.exec() == QDialog::Accepted && cpd.properties() != conductor->properties()) {

		if (cpd.applyAll()) {
			QList <Conductor *> conductorslist = conductor -> relatedPotentialConductors().toList();
			conductorslist << conductor;
			QList <ConductorProperties> old_properties_list;

			foreach (Conductor *c, conductorslist) {
				if (c == conductor) {
					old_properties_list << conductor -> properties();
				} else {
					old_properties_list << c -> properties();
					c -> setProperties( cpd.properties() );
				}
			}
			//initialize the corresponding UndoCommand object
			ChangeSeveralConductorsPropertiesCommand *cscpc = new ChangeSeveralConductorsPropertiesCommand(conductorslist);
			cscpc -> setOldSettings(old_properties_list);
			cscpc -> setNewSettings(cpd.properties());
			conductor -> diagram() -> undoStack().push(cscpc);
		} else {
			// initialize the corresponding UndoCommand object
			ChangeConductorPropertiesCommand *ccpc = new ChangeConductorPropertiesCommand(conductor);
			ccpc -> setOldSettings(conductor -> properties());
			ccpc -> setNewSettings(cpd.properties());
			conductor -> diagram() -> undoStack().push(ccpc);
		}
	}
}

/**
 * @brief ConductorPropertiesDialog::properties
 * @return the edited properties
 */
ConductorProperties ConductorPropertiesDialog::properties() const {
	return m_cpw -> properties();
}

/**
 * @brief ConductorPropertiesDialog::applyAll
 * @return
 * true -> must apply the propertie to all conductor at the same potential
 * false -> must apply properties only for the edited conductor
 */
bool ConductorPropertiesDialog::applyAll() const {
	return ui -> m_apply_all_cb -> isChecked();
}
