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
#include "potentialselectordialog.h"
#include "ui_potentialselectordialog.h"
#include "conductor.h"
#include "terminal.h"
#include <QRadioButton>
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "diagram.h"
#include "element.h"
#include "reportelement.h"

//### BEGIN PRIVATE CLASS ###//

/**
 * @brief The NewConductorPotentialSelector class
 * Use for get the conductor propertie when two potentials is linked by a conductor
 */
class NewConductorPotentialSelector : public AbstractPotentialSelector
{
	public:
		NewConductorPotentialSelector(Conductor *conductor) :
			m_is_valid(false)
		{
			Terminal *terminal_1 = conductor->terminal1;
			Terminal *terminal_2 = conductor->terminal2;
				//We temporarily remove the conductor of his two terminals, to get the two existing potential
			terminal_1->removeConductor(conductor);
			terminal_2->removeConductor(conductor);

			getPotential(terminal_1, m_properties_1, m_conductor_number_1);
			getPotential(terminal_2, m_properties_2, m_conductor_number_2);

				//There isn't a potential at terminal 1 or 2.
			if (m_conductor_number_1 == 0 && m_conductor_number_2 == 0) return;

				//Re-add conductor to his terminals.
			terminal_1->addConductor(conductor);
			terminal_2->addConductor(conductor);
			m_is_valid = true;
		}

		bool isValid() const {return m_is_valid;}

		/**
		 * @brief getPotential
		 * Get the conductor propertie of the potential at terminal, and the number of wire in this potential.
		 * @param terminal
		 * @param properties
		 * @param number
		 */
		void getPotential(Terminal *terminal, ConductorProperties &properties, int &number)
		{
			Conductor *conductor_in_potential = nullptr;

				//Terminal have conductor linked to, we get it.
			if (!terminal->conductors().isEmpty())
				conductor_in_potential = terminal->conductors().first();

				//Terminal haven't got a conductor, but if parent element is a folio report or a terminal element, we search a potential
				//through the report or in another terminal of the terminal element.
			else if (terminal->parentElement()->linkType() & (Element::Terminale | Element::AllReport))
			{
				Element *elmt_ = terminal->parentElement();

				if ((elmt_->linkType() & Element::Terminale) && !elmt_->terminals().isEmpty())
				{
					foreach(Terminal *t, elmt_->terminals())
					{
						if (t->conductors().isEmpty()) continue;
						conductor_in_potential = t->conductors().first();
						break;
					}
				}
				else if ((elmt_->linkType() & Element::AllReport) && !elmt_->isFree())
				{
					Element *other_report = elmt_->linkedElements().first();
					if (other_report->terminals().isEmpty()) return;
					Terminal *t = other_report->terminals().first();
					if (t->conductors().isEmpty()) return;
					conductor_in_potential= t->conductors().first();
				}
			}

			if (!conductor_in_potential) return;
			properties = conductor_in_potential->properties();
			number = conductor_in_potential->relatedPotentialConductors().size()+1; //We add +1 because conductor_in_potential isn't count by relatedPotentialConductors
		}

		~NewConductorPotentialSelector() {}

	private :
		bool m_is_valid;
};

/**
 * @brief The LinkReportPotentialSelector class
 * Use for get the conductor propertie when two potentials is linked with a folio report
 */
class LinkReportPotentialSelector : public AbstractPotentialSelector
{
	public:
		LinkReportPotentialSelector(Element *report) :
			m_is_valid(false)
		{
			if ((report->linkType() & Element::AllReport) && !report->isFree())
			{
					//We temporarily unlink report to get the two existing potential
				Element *other_report = report->linkedElements().first();
				report->unlinkAllElements();

				if (report->conductors().isEmpty() || other_report->conductors().isEmpty()) return;

				m_properties_1 = report->conductors().first()->properties();
				m_conductor_number_1 = report->conductors().first()->relatedPotentialConductors().size() + 1;
				m_properties_2 = other_report->conductors().first()->properties();
				m_conductor_number_2 = other_report->conductors().first()->relatedPotentialConductors().size() + 1;

					//We relink the report
				report->linkToElement(other_report);
				m_is_valid = true;
			}
		}

		~LinkReportPotentialSelector() {}

		bool isValid() const {return m_is_valid;}

	private:
		bool m_is_valid;
};

//### END PRIVATE CLASS ###//

/**
 * @brief PotentialSelectorDialog::PotentialSelectorDialog
 * Constructor when we link two potentiels together, with a conductor
 * @param conductor : the new conductor who connect to existing potential
 * @param parent_undo : undo parent to use.
 * @param parent : parent widget.
 */
PotentialSelectorDialog::PotentialSelectorDialog(Conductor *conductor, QUndoCommand *parent_undo, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PotentialSelectorDialog),
	m_conductor(conductor),
	m_report(nullptr),
	m_parent_undo(parent_undo)
{
	ui->setupUi(this);
	m_potential_selector = new NewConductorPotentialSelector(conductor);
	buildWidget();
}

/**
 * @brief PotentialSelectorDialog::PotentialSelectorDialog
 * Constructor when we link two potentiels together, with a folio report.
 * @param report : one of the report used to link the potentials (report must be linked to another report)
 * @param parent_undo : undo parent to use
 * @param parent : paren widget
 */
PotentialSelectorDialog::PotentialSelectorDialog(Element *report, QUndoCommand *parent_undo, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PotentialSelectorDialog),
	m_conductor(nullptr),
	m_report(report),
	m_parent_undo(parent_undo)
{
	ui->setupUi(this);
	m_potential_selector = new LinkReportPotentialSelector(report);
	buildWidget();
}

PotentialSelectorDialog::~PotentialSelectorDialog()
{
    delete ui;
	delete m_potential_selector;
}

/**
 * @brief PotentialSelectorDialog::buildWidget
 * Build the dialog
 */
void PotentialSelectorDialog::buildWidget()
{
	QRadioButton *rb1 = new QRadioButton(tr("Le potentiel avec numero de fil %1 est présent %2 fois").arg(m_potential_selector->m_properties_1.text).arg(m_potential_selector->m_conductor_number_1), this);
	QRadioButton *rb2 = new QRadioButton(tr("Le potentiel avec numero de fil %1 est présent %2 fois").arg(m_potential_selector->m_properties_2.text).arg(m_potential_selector->m_conductor_number_2), this);

	connect(rb1, &QRadioButton::toggled, [this](bool t){if(t) this->m_selected_properties = this->m_potential_selector->m_properties_1;});
	connect(rb2, &QRadioButton::toggled, [this](bool t){if(t) this->m_selected_properties = this->m_potential_selector->m_properties_2;});

		//Set the radio button of potential with the bigger number of conductors,
		//at first position, and check it
	if (m_potential_selector->m_conductor_number_1 >= m_potential_selector->m_conductor_number_2)
	{
		ui->verticalLayout->insertWidget(1, rb1);
		ui->verticalLayout->insertWidget(2, rb2);
		rb1->setChecked(true);
	}
	else
	{
		ui->verticalLayout->insertWidget(1, rb2);
		ui->verticalLayout->insertWidget(2, rb1);
		rb2->setChecked(true);
	}
}

/**
 * @brief PotentialSelectorDialog::on_buttonBox_accepted
 * Action when user click on OK button
 */
void PotentialSelectorDialog::on_buttonBox_accepted()
{
	if (!m_potential_selector->isValid()) return;

	if (!m_conductor)
		m_conductor = m_report->conductors().first();

	ConductorProperties new_properties = m_conductor->properties();
	new_properties.text = m_selected_properties.text;
	new_properties.m_function = m_selected_properties.m_function;
	new_properties.m_tension_protocol = m_selected_properties.m_tension_protocol;

	QVariant old_value, new_value;
	old_value.setValue(m_conductor->properties());
	new_value.setValue(new_properties);

		//Set the properties for the new conductor
	QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_conductor, "properties", old_value, new_value, m_parent_undo);
	undo->setText(tr("Modifier les propriétés de plusieurs conducteurs", "undo caption"));

		//Set the new properties for each conductors of the new potential
	foreach(Conductor *cond, m_conductor->relatedPotentialConductors())
	{
		new_properties = cond->properties();
		new_properties.text = m_selected_properties.text;
		new_properties.m_function = m_selected_properties.m_function;
		new_properties.m_tension_protocol = m_selected_properties.m_tension_protocol;
		old_value.setValue(cond->properties());
		new_value.setValue(new_properties);
		new QPropertyUndoCommand(cond, "properties", old_value, new_value, undo);
	}

		//There is an undo parent, we stop here, the owner of m_parent_undo will push it to an undo stack
	if (m_parent_undo) return;
		//There isn't a parent, we push the undo command to diagram undo stack.
	if (m_conductor->diagram()) m_conductor->diagram()->undoStack().push(undo);
		//We apply the change without undo command
	else
	{
		delete undo;
		m_conductor->setProperties(new_properties);
	}
}
