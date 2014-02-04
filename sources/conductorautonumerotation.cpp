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
#include "conductorautonumerotationwidget.h"
#include "diagramcommands.h"
#include "numerotationcontextcommands.h"

/**
 *Constructor
 * @param c the conductor to apply automatic numerotation
 */
ConductorAutoNumerotation::ConductorAutoNumerotation(Conductor *c) :
	AutoNumerotation (c -> diagram()),
	conductor_ (c),
	conductor_list(c -> relatedPotentialConductors())
{
	num_context = diagram_ -> getNumerotation(Diagram::Conductors);
}

/**
 * Constructor
 * @param d a diagram to apply automatic numerotation
 */
ConductorAutoNumerotation::ConductorAutoNumerotation(Diagram *d) :
	AutoNumerotation (d),
	conductor_ (NULL)
{}

/**
 * @param c the conductor to apply automatic numerotation
 */
void ConductorAutoNumerotation::setConductor(Conductor *c) {
	conductor_ = c;
	diagram_ = c -> diagram();
	conductor_list = c -> relatedPotentialConductors();
	num_context = diagram_ -> getNumerotation(Diagram::Conductors);
}

/**
 * @brief ConductorAutoNumerotation::numerate
 * execute the automatic numerotation
 */
void ConductorAutoNumerotation::numerate() {
	if (!conductor_) return;
	if (conductor_list.size() >= 1 ) numeratePotential();
	else if (conductor_ -> properties().type == ConductorProperties::Multi) numerateNewConductor();
	else return;
}

/**
 * @brief ConductorAutoNumerotation::numerateDiagram
 * Numerate all conductor in diagram
 */
void ConductorAutoNumerotation::numerateDiagram() {
	if (!diagram_) return;
	//Get all potentials presents in diagram
	QList <QSet <Conductor *> > potential_list = diagram_ -> potentials();
	//Browse all potentials and set new numerotation
	for (int i=0; i < potential_list.size(); ++i) {
		setConductor (potential_list.at(i).toList().first());
		NumerotationContextCommands ncc(diagram_, num_context);
		applyText(ncc.toRepresentedString());
		diagram_ -> setNumerotation(Diagram::Conductors, ncc.next());
	}
}

/**
 * @brief ConductorAutoNumerotation::checkPotential
 * Check if text of this potential is identical.
 * If not, ask user how to numerate
 * @param conductor
 * One conductor of this potential.
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
		ConductorAutoNumerotationWidget *canw = new ConductorAutoNumerotationWidget(conductor, c_list, conductor -> diagramEditor());
		ConductorAutoNumerotation can(conductor);
		connect(canw, SIGNAL(textIsSelected(QString)), &can, SLOT(applyText(QString)));
		canw -> exec();
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
		diagram_ -> undoStack().push(ccpc);
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
		diagram_ -> undoStack().push(cscpc);
	}
}

/**
 * @brief Set the default text to all potentials of the diagram
 */
void ConductorAutoNumerotation::removeNumOfDiagram() {
	if (!diagram_) return;
	//Get all potentials presents in diagram
	QList <QSet <Conductor *> > potential_list = diagram_ -> potentials();
	//Browse all potentials and set the default text
	for (int i=0; i < potential_list.size(); i++) {
		setConductor (potential_list.at(i).toList().first());
		applyText (diagram_ -> defaultConductorProperties.text);
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
		ConductorAutoNumerotationWidget *canw = new ConductorAutoNumerotationWidget(conductor_, conductor_list, conductor_ -> diagramEditor());
		connect(canw, SIGNAL(textIsSelected(QString)),
				this, SLOT(applyText(QString)));
		canw -> exec();
	}
}

/**
 * @brief ConductorAutoNumerotation::numerateNewConductor
 * create and apply a new numerotation to @conductor_
 */
void ConductorAutoNumerotation::numerateNewConductor() {
	if (!conductor_ || num_context.isEmpty()) return;

	NumerotationContextCommands ncc (diagram_, num_context);
	applyText(ncc.toRepresentedString());
	diagram_-> setNumerotation(Diagram::Conductors, ncc.next());
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
