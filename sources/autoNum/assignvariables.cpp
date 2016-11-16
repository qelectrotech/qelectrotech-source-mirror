/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "assignvariables.h"
#include "diagram.h"
#include "element.h"
#include "diagramposition.h"
#include <QVariant>

namespace autonum
{
	/**
	 * @brief AssignVariables::formulaToLabel
	 * Return the @formula with variable assigned (ready to be displayed)
	 * @param formula - the formula to work
	 * @param seqStruct - struct where is stocked int values (struct is passed as a reference and modified by this static method)
	 * @param diagram - the diagram where occure the formula.
	 * @param elmt - parent element (if any) of the formula
	 * @return the string with variable assigned.
	 */
	QString AssignVariables::formulaToLabel(QString formula, sequenceStruct &seqStruct, Diagram *diagram, const Element *elmt)
	{
		AssignVariables av(formula, seqStruct, diagram, elmt);
		seqStruct = av.m_seq_struct;
		return av.m_assigned_label;
	}

	AssignVariables::AssignVariables(QString formula, sequenceStruct seqStruct , Diagram *diagram, const Element *elmt):
	m_diagram(diagram),
	m_arg_formula(formula),
	m_assigned_label(formula),
	m_seq_struct(seqStruct),
	m_element(elmt)
	{
		if (m_diagram)
		{
			m_assigned_label.replace("%F",  m_diagram -> border_and_titleblock.folio());
			m_assigned_label.replace("%f",     QString::number(m_diagram->folioIndex()+1));
			m_assigned_label.replace("%id",    QString::number(m_diagram->folioIndex()+1));
			m_assigned_label.replace("%total", QString::number(m_diagram->border_and_titleblock.folioTotal()));
			m_assigned_label.replace("%M",  m_diagram -> border_and_titleblock.machine());
			m_assigned_label.replace("%LM", m_diagram -> border_and_titleblock.locmach());


			if (m_element)
			{
				m_assigned_label.replace("%c", QString::number(m_diagram->convertPosition(m_element->scenePos()).number()));
				m_assigned_label.replace("%l", m_diagram->convertPosition(m_element->scenePos()).letter());
				m_assigned_label.replace("%prefix", m_element->getPrefix());
			}

			assignTitleBlockVar();
			assignProjectVar();
			assignSequence();
		}
	}

	void AssignVariables::assignTitleBlockVar()
	{
		for (int i = 0; i < m_diagram->border_and_titleblock.additionalFields().count(); i++)
		{
			QString folio_variable = m_diagram->border_and_titleblock.additionalFields().keys().at(i);
			QVariant folio_value = m_diagram->border_and_titleblock.additionalFields().operator [](folio_variable);

			if (m_assigned_label.contains(folio_variable)) {
				m_assigned_label.replace("%{" + folio_variable + "}", folio_value.toString());
				m_assigned_label.replace("%"  + folio_variable      , folio_value.toString());
			}
		}
	}

	void AssignVariables::assignProjectVar()
	{
		for (int i = 0; i < m_diagram->project()->projectProperties().count(); i++)
		{
			QString folio_variable = m_diagram->project()->projectProperties().keys().at(i);
			QVariant folio_value = m_diagram->project()->projectProperties().operator [](folio_variable);

			if (m_assigned_label.contains(folio_variable)) {
				m_assigned_label.replace("%{" + folio_variable + "}", folio_value.toString());
				m_assigned_label.replace("%"  + folio_variable      , folio_value.toString());
			}
		}
	}

	void AssignVariables::assignSequence()
	{
		int max = qMax(
						qMax(
							qMax(m_seq_struct.unit_folio.size(),
								 m_seq_struct.ten_folio.size()),
							qMax(m_seq_struct.hundred_folio.size(),
								 m_seq_struct.unit.size())),
						qMax(m_seq_struct.hundred.size(),
							 m_seq_struct.ten.size())
					);

		for (int i=1; i<=max ; i++)
		{
			if (m_assigned_label.contains("%sequ_" + QString::number(i)) && !m_seq_struct.unit.isEmpty()) {
				m_assigned_label.replace("%sequ_" + QString::number(i),m_seq_struct.unit.at(i-1));
			}
			if (m_assigned_label.contains("%seqt_" + QString::number(i)) && !m_seq_struct.ten.isEmpty()) {
				m_assigned_label.replace("%seqt_" + QString::number(i),m_seq_struct.ten.at(i-1));
			}
			if (m_assigned_label.contains("%seqh_" + QString::number(i)) && !m_seq_struct.hundred.isEmpty()) {
				m_assigned_label.replace("%seqh_" + QString::number(i),m_seq_struct.hundred.at(i-1));
			}
			if (m_assigned_label.contains("%sequf_" + QString::number(i)) && !m_seq_struct.unit_folio.isEmpty()) {
				m_assigned_label.replace("%sequf_" + QString::number(i),m_seq_struct.unit_folio.at(i-1));
			}
			if (m_assigned_label.contains("%seqtf_" + QString::number(i)) && !m_seq_struct.ten_folio.isEmpty()) {
				m_assigned_label.replace("%seqtf_" + QString::number(i),m_seq_struct.ten_folio.at(i-1));
			}
			if (m_assigned_label.contains("%seqhf_" + QString::number(i)) && !m_seq_struct.hundred_folio.isEmpty()) {
				m_assigned_label.replace("%seqhf_" + QString::number(i),m_seq_struct.hundred_folio.at(i-1));
			}
		}
	}

}
