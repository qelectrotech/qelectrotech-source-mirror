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
#ifndef XREFDISPLAYDIALOG_H
#define XREFDISPLAYDIALOG_H

#include "../properties/elementdata.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;

/**
	@brief The XRefDisplayDialog class
	Asks how the cross reference of one master (a relay, a contactor) is
	shown: cross or contact symbols, where it sits, how thick its lines
	are. The master keeps the project's setting until the user changes one.
*/
class XRefDisplayDialog : public QDialog
{
	Q_OBJECT

	public:
		XRefDisplayDialog(const ElementData::XRefDisplay &display,
						  const QPointF &suggested_manual,
						  QWidget *parent = nullptr);

		ElementData::XRefDisplay display() const;

	private:
		void updateEnabled();

		QComboBox *m_appearance;
		QComboBox *m_position;
		QDoubleSpinBox *m_width;
		QDoubleSpinBox *m_gap;
		QDoubleSpinBox *m_x;
		QDoubleSpinBox *m_y;
		QCheckBox *m_show_unlinked;
		bool m_restore = false;
};

#endif // XREFDISPLAYDIALOG_H
