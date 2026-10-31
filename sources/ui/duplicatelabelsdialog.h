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
#ifndef DUPLICATELABELSDIALOG_H
#define DUPLICATELABELSDIALOG_H

#include <duplicatelabels.h>

#include <QDialog>
#include <QHash>

class QCheckBox;
class QLabel;
class QETProject;
class QTreeWidget;
class QTreeWidgetItem;

class DuplicateLabelsDialog : public QDialog
{
	Q_OBJECT
  public:
	explicit DuplicateLabelsDialog(
		QETProject* project, QWidget* parent = nullptr);

  public slots:
	void refresh();
	void showOccurrence(QTreeWidgetItem* row);

  private:
	void buildFolios();

	QPointer<QETProject>							   m_project;
	QTreeWidget*									   m_tree;
	QCheckBox*										   m_case_sensitive;
	QLabel*											   m_summary;
	QHash<QTreeWidgetItem*, QPointer<QGraphicsObject>> m_targets;
};

#endif
