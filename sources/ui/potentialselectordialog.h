/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef POTENTIALSELECTORDIALOG_H
#define POTENTIALSELECTORDIALOG_H

#include <QDialog>
#include "conductorproperties.h"
class Conductor;
class QUndoCommand;
class Element;

class AbstractPotentialSelector
{
	public:
		virtual ~AbstractPotentialSelector() {}
		virtual bool isValid() const = 0;

		ConductorProperties m_properties_1, m_properties_2;
		int m_conductor_number_1, m_conductor_number_2;
};

namespace Ui {
	class PotentialSelectorDialog;
}

/**
 * @brief The PotentialSelectorDialog class
 * This dialog is used when user try to connect two existing potential together.
 * The dialog ask to user to make a choice between the properties of the two existing potential,
 * to apply it for the new potential.
 *
 * Each constructor have a QUndoCommand @parent_undo for parameter
 * If @parent_undo isn't null, when user click on OK button, the dialog will use the parent-undo
 * as parent of the undo command that describe the changes.
 * If @parent_undo is null, the created undo-command is push to the undo stack of the parent diagram of a conductor in potential.
 * else we apply the change without a QUndoCommand.
 */
class PotentialSelectorDialog : public QDialog
{
        Q_OBJECT

    public:
		explicit PotentialSelectorDialog(Conductor *conductor, QUndoCommand *parent_undo = nullptr, QWidget *parent = nullptr);
		explicit PotentialSelectorDialog(Element *report, QUndoCommand *parent_undo = nullptr, QWidget *parent = nullptr);
        ~PotentialSelectorDialog();

	private slots:
		void on_buttonBox_accepted();

	private:
        void buildWidget();

    private:
        Ui::PotentialSelectorDialog *ui;
        Conductor *m_conductor;
		Element *m_report;
		QUndoCommand *m_parent_undo;
		ConductorProperties m_selected_properties;
		AbstractPotentialSelector *m_potential_selector;
};
#endif // POTENTIALSELECTORDIALOG_H
