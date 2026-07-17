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
#ifndef CONTACTGROUPSELECTIONDIALOG_H
#define CONTACTGROUPSELECTIONDIALOG_H

#include <QDialog>
#include <QVector>
#include <QSet>

#include "../properties/elementdata.h"

class QTableWidget;
class QTableWidgetItem;
class QPushButton;

/**
	@brief The ContactGroupSelectionDialog class
	A dialog that displays all slave contact groups defined by a master element
	in a table format. The user can select one group to assign to a slave element.
	Columns: #, Type, Subtype, Contacts, Terminals, T1, T2, ..., TN
*/
class ContactGroupSelectionDialog : public QDialog
{
	Q_OBJECT

	public:
		ContactGroupSelectionDialog(
			const QVector<ElementData::SlaveContactGroup> &groups,
			const QSet<int> &usedGroupIndices,
			const ElementData &slaveData,
			QWidget *parent = nullptr);

		int selectedIndex() const;

	private:
		QTableWidget *m_table = nullptr;
		QPushButton *m_ok_button = nullptr;
		int m_selected_index = -1;
		QSet<int> m_used_indices;
		QSet<int> m_disabled_rows;

		static QString typeToString(ElementData::SlaveState type);
		static QString subtypeToString(ElementData::SlaveType subtype);
};

#endif // CONTACTGROUPSELECTIONDIALOG_H
