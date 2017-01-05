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
#include "numerotationcontextcommands.h"
#include "qetdiagrameditor.h"
#include "conductor.h"
#include "diagram.h"
#include "qet.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "potentialselectordialog.h"
#include "assignvariables.h"

/**
 * @brief ConductorAutoNumerotation::ConductorAutoNumerotation
 * Constructor of autonum, after create a class, call numerate to apply the autonum.
 * When autonum is applyed, they do with an undo command added to the stack of diagram.
 * If you give a parent_undo at constructor, the undo command create in this class have parent_undo for parent,
 * and wasn't added to the stack of diagram (it's the responsabillty of the parent_undo)
 * @param conductor   : the conductor to apply automatic numerotation
 * @param diagram     : the diagram of conductor
 * @param parent_undo : parent undo command
 */
ConductorAutoNumerotation::ConductorAutoNumerotation(Conductor *conductor, Diagram *diagram, QUndoCommand *parent_undo) :
	m_diagram      (diagram),
	m_conductor     (conductor),
	conductor_list (conductor -> relatedPotentialConductors().toList()),
	m_parent_undo  (parent_undo)
{}

/**
 * @brief ConductorAutoNumerotation::numerate
 * execute the automatic numerotation
 */
void ConductorAutoNumerotation::numerate() {
	if (!m_conductor) return;
	if (conductor_list.size() >= 1 ) numeratePotential();
	else if (m_conductor -> properties().type == ConductorProperties::Multi) numerateNewConductor();
}

/**
 * @brief ConductorAutoNumerotation::applyText
 * apply the text @t to @conductor_ and all conductors at the same potential
 */
void ConductorAutoNumerotation::applyText(QString t)
{
	if (!m_conductor) return;

	QVariant old_value, new_value;
	ConductorProperties cp = m_conductor -> properties();
	old_value.setValue(cp);
	cp.text = t;
	new_value.setValue(cp);

	QUndoCommand *undo = nullptr;

	if (m_parent_undo)
	{
		new QPropertyUndoCommand(m_conductor, "properties", old_value, new_value, m_parent_undo);
		undo = m_parent_undo;
	}
	else
	{
		undo = new QUndoCommand();
		new QPropertyUndoCommand(m_conductor, "properties", old_value, new_value, undo);
		undo->setText(QObject::tr("Modifier les propriétés d'un conducteur", "undo caption"));
	}

	if (!conductor_list.isEmpty())
	{
		if (!m_parent_undo)
			undo->setText(QObject::tr("Modifier les propriétés de plusieurs conducteurs", "undo caption"));

		foreach (Conductor *cond, conductor_list)
		{
			ConductorProperties cp2 = cond -> properties();
			old_value.setValue(cp2);
			cp2.text = t;
			new_value.setValue(cp2);
			new QPropertyUndoCommand(cond, "properties", old_value, new_value, undo);
		}
	}

	if (!m_parent_undo)
		m_diagram->undoStack().push(undo);
}

/**
 * @brief ConductorAutoNumerotation::numeratePotential
 * Numerate a conductor on an existing potential
 */
void ConductorAutoNumerotation::numeratePotential()
{
	ConductorProperties cp = conductor_list.first()->properties();
	bool properties_equal = true;
	foreach (const Conductor *conductor, conductor_list)
	{
		if (conductor->properties() != cp)
			properties_equal = false;
	}
		//Every properties of the potential is equal, so we apply it to m_conductor
	if (properties_equal)
	{
		m_conductor->setProperties(cp);
		m_conductor->rSequenceNum() = conductor_list.first()->sequenceNum();
		return;
	}

	QStringList text_list;
	QStringList formula_list;
	foreach (const Conductor *cc, conductor_list)
	{
		ConductorProperties cp = cc->properties();
		text_list << cp.text;
		formula_list << cp.m_formula;
	}


		//the texts is identicals
	if (QET::eachStrIsEqual(text_list) && QET::eachStrIsEqual(formula_list))
	{
		QList<ConductorProperties> cp_list;
		foreach(Conductor *c, conductor_list)
			cp_list<<c->properties();

		ConductorProperties cp = m_conductor->properties();
		cp.applyForEqualAttributes(cp_list);
		m_conductor->rSequenceNum() = conductor_list.first()->sequenceNum();
		m_conductor->setProperties(cp);
	}
		//the texts isn't identicals
	else
	{
		PotentialSelectorDialog psd(m_conductor, m_parent_undo, m_conductor->diagramEditor());
		psd.exec();
	}
}

/**
 * @brief ConductorAutoNumerotation::numerateNewConductor
 * create and apply a new numerotation to @m_conductor
 */
void ConductorAutoNumerotation::numerateNewConductor()
{
	if (!m_conductor || m_diagram->conductorsAutonumName().isEmpty())
		return;

	NumerotationContext context = m_diagram->project()->conductorAutoNum(m_diagram -> conductorsAutonumName());
	if (context.isEmpty())
		return;

	QString autoNum_name = m_diagram->project()->conductorCurrentAutoNum();
	QString formula = autonum::numerotationContextToFormula(context);

	ConductorProperties cp = m_conductor -> properties();
	cp.m_formula = formula;
	m_conductor->setProperties(cp);

	autonum::setSequential(formula, m_conductor->rSequenceNum(), context, m_diagram, autoNum_name);

	NumerotationContextCommands ncc (context, m_diagram);
	m_diagram->project()->addConductorAutoNum(autoNum_name, ncc.next());

	applyText(autonum::AssignVariables::formulaToLabel(formula, m_conductor->rSequenceNum(), m_diagram));
}
