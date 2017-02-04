/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "diagram.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

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
	m_cpw -> setHiddenOneTextPerFolio(true);
	if (conductor -> diagram() -> defaultConductorProperties.m_one_text_per_folio == true &&
		conductor -> relatedPotentialConductors().size()) {
		m_cpw->setDisabledShowText();
	}
	ui -> main_layout -> insertWidget(0, m_cpw);
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
void ConductorPropertiesDialog::PropertiesDialog(Conductor *conductor, QWidget *parent)
{
	ConductorPropertiesDialog cpd (conductor, parent);

	if (cpd.exec() == QDialog::Rejected || cpd.properties() == conductor->properties()) return;

	QVariant old_value, new_value;
	old_value.setValue(conductor->properties());
	new_value.setValue(cpd.properties());

	QPropertyUndoCommand *undo = new QPropertyUndoCommand(conductor, "properties", old_value, new_value);
	undo->setText(tr("Modifier les propriétés d'un conducteur", "undo caption"));

	if (!conductor->relatedPotentialConductors().isEmpty() && cpd.applyAll())
	{
		undo->setText(tr("Modifier les propriétés de plusieurs conducteurs", "undo caption"));

		for (Conductor *potential_conductor: conductor->relatedPotentialConductors())
		{
			old_value.setValue(potential_conductor->properties());
			new QPropertyUndoCommand (potential_conductor, "properties", old_value, new_value, undo);
		}
	}

	conductor->diagram()->undoStack().push(undo);
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
