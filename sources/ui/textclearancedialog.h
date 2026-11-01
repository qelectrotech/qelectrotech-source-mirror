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
#ifndef TEXTCLEARANCEDIALOG_H
#define TEXTCLEARANCEDIALOG_H

#include "../textclearance.h"

#include <QDialog>

class Diagram;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QLabel;

/**
	@brief The TextClearanceDialog class
	Asks how far texts must stay from wires and how far they may move, and
	says how many texts that would move before anything is changed.
*/
class TextClearanceDialog : public QDialog
{
	Q_OBJECT

	public:
		TextClearanceDialog(Diagram *diagram, QWidget *parent = nullptr);

		TextClearance::Plan plan() const {return m_plan;}

	private:
		void update();

		QPointer<Diagram> m_diagram;
		QDoubleSpinBox *m_distance;
		QDoubleSpinBox *m_max_move;
		QComboBox *m_direction;
		QComboBox *m_scope;
		QLabel *m_summary;
		QDialogButtonBox *m_buttons;
		TextClearance::Plan m_plan;
};

#endif // TEXTCLEARANCEDIALOG_H
