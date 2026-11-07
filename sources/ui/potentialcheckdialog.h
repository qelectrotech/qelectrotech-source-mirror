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
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef POTENTIALCHECKDIALOG_H
#define POTENTIALCHECKDIALOG_H

#include <dataBase/projectdatabase.h>

#include <QDialog>
#include <QPointer>

class QLabel;
class QETProject;
class QTreeWidget;
class QTreeWidgetItem;

/**
	@brief The PotentialCheckDialog class
	Lists the potentials that carry wires of different potential types
	and selects those wires on their sheets when a row is clicked.
*/
class PotentialCheckDialog : public QDialog
{
	Q_OBJECT
	public:
		explicit PotentialCheckDialog(QETProject *project, QWidget *parent = nullptr);

	public slots:
		void refresh();
		void showWires(QTreeWidgetItem *row);

	private:
		QPointer<QETProject> m_project;
		QTreeWidget *m_tree;
		QLabel *m_summary;
		QHash<QTreeWidgetItem *, QList<QUuid>> m_wires;
};

#endif
