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

#include "conductorpropertiesdialog.h"

#include <QPropertyUndoCommand/qpropertyundocommand.h>
#include <undocommand/dissolvereportlinkscommand.h>
#include <bridge/bridgemanager.h>
#include <diagram.h>
#include <qetgraphicsitem/conductor.h>
#include <qetproject.h>
#include "conductorpropertieswidget.h"
#include "ui_conductorpropertiesdialog.h"

#include <qet.h>
/**
	@brief ConductorPropertiesDialog::ConductorPropertiesDialog
	Constructor
	@param conductor : conductor to edit properties
	@param parent : parent widget
*/
ConductorPropertiesDialog::ConductorPropertiesDialog(
		Conductor *conductor, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConductorPropertiesDialog)
{
	ui->setupUi(this);
	QET::trackDialogGeometry(this);
	m_cpw = new ConductorPropertiesWidget(conductor->properties());
	m_cpw -> setHiddenOneTextPerFolio(true);
	m_cpw->setHiddenAvailableAutonum(true);
	if (conductor->diagram() && conductor->diagram()->project()) {
		m_cpw->setPotentialTypes(conductor->diagram()->project()->potentialTypes());
		m_cpw->setProject(conductor->diagram()->project());
	}
	if (conductor -> diagram() -> defaultConductorProperties.m_one_text_per_folio == true &&
		conductor -> relatedPotentialConductors().size()) {
		m_cpw->setDisabledShowText();
	}
	ui -> main_layout -> insertWidget(0, m_cpw);
}

/**
	@brief ConductorPropertiesDialog::~ConductorPropertiesDialog
*/
ConductorPropertiesDialog::~ConductorPropertiesDialog()
{
	delete ui;
}

/**
	@brief ConductorPropertiesDialog::PropertiesDialog
	Static method for open and apply properties.
	@param conductor : conductor to edit properties
	@param parent : parent widget
*/
void ConductorPropertiesDialog::PropertiesDialog(Conductor *conductor,
						 QWidget *parent)
{
	ConductorPropertiesDialog cpd (conductor, parent);

	if (cpd.exec() == QDialog::Rejected
			|| cpd.properties() == conductor->properties()) return;

	QVariant old_value, new_value;
	old_value.setValue(conductor->properties());
	new_value.setValue(cpd.properties());

		//A name written over a potential stays on the folio it is written
		//on in a project chaining its reports by the wire function: the
		//folio report links of the potential go first, and the chaining
		//makes them again with the new name
	const bool renames = cpd.properties().m_function
			!= conductor->properties().m_function;
	QETProject *project = conductor->diagram() ? conductor->diagram()->project() : nullptr;
	const bool stop_at_report_links = renames && project
			&& project->chainsByWireFunction();
	const QSet<Conductor *> potential = conductor->relatedPotentialConductors(
				true, nullptr, false, stop_at_report_links);

	QUndoCommand *undo = new QUndoCommand();
	undo->setText(tr("Edit conductor properties", "undo caption"));
	if (stop_at_report_links) {
		new DissolveReportLinksCommand({conductor}, undo);
	}
	new QPropertyUndoCommand(conductor, "properties", old_value, new_value, undo);

	if (!potential.isEmpty() && cpd.applyAll())
	{
		undo->setText(tr("Edit the properties of several conductors", "undo caption"));

		foreach (Conductor *potential_conductor, potential)
		{
				//A core of a cable keeps its own properties: what the
				//cable says about it is not overwritten by an edit made
				//on the other side of a terminal
			if (!potential_conductor->properties().m_cable_uuid.isNull()) {
				continue;
			}
			old_value.setValue(potential_conductor->properties());
			new QPropertyUndoCommand (potential_conductor, "properties", old_value, new_value, undo);
		}
	}

		//A function separated terminal element (element information
		//potential_isolating) keeps the two sides of the terminal apart:
		//every property but what the node itself describes stays local to
		//the side it was edited on. The voltage / protocol and the drawing
		//of the wire still cross, because the node is drawn the same on
		//both sides of it.
	QSet<Conductor *> separated_reached;
	if (cpd.applyAll())
	{
			//Only an edit that really changed one of those fields passes
			//it on, so the comparison is the edit itself -- what the dialog
			//was filled with, against what the user left in it -- and never
			//the far wire's own value, which differs from it just as often
			//as not (a 24 V side against a 0 V one, say) and would then be
			//overwritten by every edit, voltage included.
		const ConductorProperties shown = conductor->properties();
		foreach (Conductor *separated_conductor, conductor->functionSeparatedConductors())
		{
			separated_reached.insert(separated_conductor);
			ConductorProperties separated_properties = separated_conductor->properties();
			separated_properties.applyNodePropertiesChange(
						shown,
						cpd.properties());
			if (separated_properties == separated_conductor->properties())
				continue;

			QVariant separated_old_value, separated_new_value;
			separated_old_value.setValue(separated_conductor->properties());
			separated_new_value.setValue(separated_properties);
			new QPropertyUndoCommand (separated_conductor, "properties",
						  separated_old_value, separated_new_value, undo);
		}
	}

		//A bridge sits across the terminals it lands on, so the voltage
		//of one of their wires reaches every wire of every terminal of it
		//-- and only the voltage crosses a bridge, never the function and
		//never the drawing: a separated function stays separated however
		//many bridges sit on it. A wire the function separation already
		//reached keeps the wider change it got from there.
	if (cpd.applyAll())
	{
		const ConductorProperties shown_for_bridge = conductor->properties();
		const auto over_bridges = BridgeManager::conductorsOverBridges(
					conductor->diagram()->project(), QList<Conductor *>{conductor});
		for (Conductor *over : over_bridges)
		{
			if (!over || over == conductor || separated_reached.contains(over)) {
				continue;
			}
			ConductorProperties over_properties = over->properties();
			over_properties.applyTensionProtocolChange(
						shown_for_bridge, cpd.properties());
			if (over_properties == over->properties()) {
				continue;
			}
			QVariant over_old_value, over_new_value;
			over_old_value.setValue(over->properties());
			over_new_value.setValue(over_properties);
			new QPropertyUndoCommand(over, "properties",
						  over_old_value, over_new_value, undo);
		}
	}

	conductor->diagram()->undoStack().push(undo);
}

/**
	@brief ConductorPropertiesDialog::properties
	@return the edited properties
*/
ConductorProperties ConductorPropertiesDialog::properties() const
{
	return m_cpw -> properties();
}

/**
	@brief ConductorPropertiesDialog::applyAll
	@return
	true -> must apply properties to all conductors at the same potential
	false -> must apply properties only for the edited conductor
*/
bool ConductorPropertiesDialog::applyAll() const
{
	return ui -> m_apply_all_cb -> isChecked();
}
