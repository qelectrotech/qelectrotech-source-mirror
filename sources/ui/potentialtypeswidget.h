// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef POTENTIALTYPESWIDGET_H
#define POTENTIALTYPESWIDGET_H

#include <properties/potentialtypes.h>

#include <QWidget>

class QComboBox;
class QListWidget;
class QTableWidget;

/**
	@brief The PotentialTypesWidget class
	Edits the potential types of a project: whether its conductors are
	drawn from them, the name, colour, line style and width of each, and
	the pairs of types joined on purpose.
*/
class PotentialTypesWidget : public QWidget
{
		Q_OBJECT

	public:
		explicit PotentialTypesWidget(QWidget *parent = nullptr);

		void setPotentialTypes(const PotentialTypes &types);
		PotentialTypes potentialTypes() const;

	private:
		void addRow(const PotentialType &type);
		void chooseColor(QWidget *button);
		void setTypes(const QList<PotentialType> &types);
		void setBonds(const QList<PotentialBond> &bonds);
		void addBondItem(const PotentialBond &bond);
		void refreshBondChoices();

		QComboBox *m_mode_cb = nullptr;
		QTableWidget *m_table = nullptr;
		QComboBox *m_bond_a_cb = nullptr;
		QComboBox *m_bond_b_cb = nullptr;
		QListWidget *m_bond_list = nullptr;
};

#endif // POTENTIALTYPESWIDGET_H
