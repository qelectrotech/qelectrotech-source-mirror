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
#ifndef CABLENUMBERINGDIALOG_H
#define CABLENUMBERINGDIALOG_H

#include <QDialog>

class QDialogButtonBox;
class QETProject;
class QPushButton;
class QRadioButton;
class SelectAutonumW;

/**
	@brief The window behind the "Numérotation des câbles" entry of the
	numbering menu: the one numbering rule the project numbers its
	cables with, the axis those cables are laid out along when the whole
	project is numbered again, and the button which does that numbering.

	The rule is the very editor the project properties hold on their
	cable tab, read from the project when the window opens and written
	back into the project by that editor's own Apply button -- and by
	the numbering button, which writes what the window holds before it
	numbers, so numbering always happens with the rule on show: a rule
	which is changed here is the rule the project keeps, not a rule of
	its own. There is no confirm button of this window's own: what the
	window holds is either applied by hand or left behind when it is
	closed.

	Renumbering works every cable of the project out from the beginning
	of that rule, so the hole a deleted cable leaves in the numbering
	closes -- and it only happens when he asks for it.
*/
class CableNumberingDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit CableNumberingDialog(QETProject *project, QWidget *parent = nullptr);

	private slots:
			///Put the rule and the axis into the project
		void applyRule();
			///Take the cable numbering rule out of the project
		void removeRule();
			///Number every cable of the project with the rule on show
		void renumber();
			///Let the button say whether there is a rule to number with
		void updateEnabling();

	private:
			///Write what the window holds into the project
		bool save();
			/**
				Ask what to do with the names he typed in himself.
				@param count how many such names the project holds
				@return 1 to number them too, 0 to leave them alone,
				-1 to do nothing at all
			*/
		int askAboutHandWritten(int count);

	private:
		QETProject *m_project = nullptr;
		SelectAutonumW *m_rule = nullptr;
		QRadioButton *m_axis_x = nullptr;
		QRadioButton *m_axis_y = nullptr;
		QPushButton *m_renumber_pb = nullptr;
		QDialogButtonBox *m_box = nullptr;
};

#endif // CABLENUMBERINGDIALOG_H
