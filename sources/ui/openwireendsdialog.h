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
#ifndef OPENWIREENDSDIALOG_H
#define OPENWIREENDSDIALOG_H

#include <projectcheck/openwireends.h>

#include <QDialog>
#include <QHash>
#include <QPointer>

class QLabel;
class QETProject;
class QTreeWidget;
class QTreeWidgetItem;

/**
	@brief The OpenWireEndsDialog class
	Lists the wires that lead nowhere (OpenWireEnds::check()) and, when a
	row is clicked, selects the wires and the symbol of that row on their
	folio.
*/
class OpenWireEndsDialog : public QDialog
{
	Q_OBJECT
	public:
		explicit OpenWireEndsDialog(QETProject *project, QWidget *parent = nullptr);

		static QString problemText(const OpenWireEnds::Finding &finding);

	public slots:
		void refresh();
		void showFinding(QTreeWidgetItem *row);

	private:
		QPointer<QETProject> m_project;
		QTreeWidget *m_tree;
		QLabel *m_summary;
		QHash<QTreeWidgetItem *, OpenWireEnds::Finding> m_findings;
};

#endif
