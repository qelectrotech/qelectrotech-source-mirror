/*
	Copyright 2006-2014 The QElectroTech team
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
#include "conductorautonumerotation.h"
#include "diagramcommands.h"
#include "numerotationcontextcommands.h"
#include "qetdiagrameditor.h"
#include "conductor.h"
#include "diagram.h"
#include "potentialtextsdialog.h"

/**
 *Constructor
 * @param c the conductor to apply automatic numerotation
 */
ConductorAutoNumerotation::ConductorAutoNumerotation(Conductor *c) :
	m_diagram      (c -> diagram()),
	conductor_     (c),
	conductor_list (c -> relatedPotentialConductors())
{}

/**
 * @brief ConductorAutoNumerotation::numerate
 * execute the automatic numerotation
 */
void ConductorAutoNumerotation::numerate() {
	if (!conductor_) return;
	if (conductor_list.size() >= 1 ) numeratePotential();
	else if (conductor_ -> properties().type == ConductorProperties::Multi) numerateNewConductor();
}

/**
 * @brief ConductorAutoNumerotation::checkPotential
 * Check if eah texts of this potential is identical.
 * If not, ask user how to numerate
 * @param conductor
 * A conductor of the potential to check.
 */
void ConductorAutoNumerotation::checkPotential(Conductor *conductor) {
	//fill list of potential
	QSet <Conductor *> c_list = conductor->relatedPotentialConductors();
	c_list << conductor;
	//fill list of text
	QStringList strl;
	foreach (const Conductor *c, c_list) strl<<(c->text());

	//check text list, isn't same in potential, ask user what to do
	if (!eachIsEqual(strl)) {
		PotentialTextsDialog ptd(conductor, conductor->diagramEditor());
		if ( ptd.exec() == QDialog::Accepted ) {
			ConductorAutoNumerotation can(conductor);
			can.applyText(ptd.selectedText());
		}
	}
}

/**
 * @brief ConductorAutoNumerotation::applyText
 * apply the text @t to @conductor_ and all conductors at the same potential
 */
void ConductorAutoNumerotation::applyText(QString t) {
	if (!conductor_) return;
	if (conductor_list.empty()) {
		//initialize the corresponding UndoCommand object
		ChangeConductorPropertiesCommand *ccpc = new ChangeConductorPropertiesCommand (conductor_);
		ccpc -> setOldSettings (conductor_ -> properties());
		ConductorProperties cp = conductor_ -> properties();
		cp.text = t;
		ccpc -> setNewSettings(cp);
		m_diagram -> undoStack().push(ccpc);
	}
	else {
		QList <Conductor *> clist = conductor_list.toList();
		clist << conductor_;
		QList <ConductorProperties> old_properties, new_properties;
		ConductorProperties cp;

		foreach (Conductor *c, clist) {
			old_properties << c -> properties();
			cp = c -> properties();
			cp.text = t;
			new_properties << cp;
		}
		//initialize the corresponding UndoCommand object
		ChangeSeveralConductorsPropertiesCommand *cscpc = new ChangeSeveralConductorsPropertiesCommand(clist);
		cscpc -> setOldSettings(old_properties);
		cscpc -> setNewSettings(new_properties);
		m_diagram -> undoStack().push(cscpc);
	}
}

/**
 * @brief ConductorAutoNumerotation::numeratePotential
 * Numerate a conductor on an existing potential
 */
void ConductorAutoNumerotation::numeratePotential() {
	QStringList strl;
	foreach (const Conductor *cc, conductor_list) strl<<(cc->text());
	//the texts is identicals
	if (eachIsEqual(strl)) {
		ConductorProperties cp = conductor_ -> properties();
		cp.text = strl.at(0);
		conductor_ -> setProperties(cp);
		conductor_ -> setText(strl.at(0));
	}
	//the texts isn't identicals
	else {
		PotentialTextsDialog ptd (conductor_, conductor_ -> diagramEditor());
		if (ptd.exec() == QDialog::Accepted) {
			applyText(ptd.selectedText());
		}
	}
}

/**
 * @brief ConductorAutoNumerotation::numerateNewConductor
 * create and apply a new numerotation to @conductor_
 */
void ConductorAutoNumerotation::numerateNewConductor() {
	if (!conductor_ || m_diagram->conductorsAutonumName().isEmpty()) return;

	QString name = m_diagram -> conductorsAutonumName();
	NumerotationContextCommands ncc (m_diagram, m_diagram->project()->conductorAutoNum(name));
	applyText(ncc.toRepresentedString());
	m_diagram->project()->addConductorAutoNum(name, ncc.next());
}

/**
 * @return true if every text of qsl is identical, else false.
 */
bool eachIsEqual (const QStringList &qsl) {
	foreach (const QString t, qsl) {
		if (qsl.at(0) != t) return false;
	}
	return true;
}
