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
#ifndef CONDUCTORCOLORRULESW_H
#define CONDUCTORCOLORRULESW_H

#include <QWidget>

class QETProject;
class QTableWidget;
class QToolButton;

/**
	@brief The ConductorColorRulesW class
	Discussion #606 configuration UI: lists every entry of the project's
	QETProject::conductorColorRules() map and lets the user add, edit and
	remove them. One row per rule: the resolved label text it applies to,
	its color, and an optional second stripe color for a dual-color rule
	(e.g. "PE").

	Modelled on SelectAutonumW, the sibling tab this sits next to in
	ProjectAutoNumConfigPage: takes the QETProject in its constructor and
	reads its own data from it, and only writes changes back to the
	project when the user presses this widget's own save button -- exactly
	like every other tab in that same page, none of which are gated by the
	surrounding ConfigDialog's own OK button (see
	ProjectAutoNumConfigPage::applyProjectConf(), deliberately empty).
*/
class ConductorColorRulesW : public QWidget
{
		Q_OBJECT

	public:
		explicit ConductorColorRulesW(QETProject *project, QWidget *parent = nullptr);

	private slots:
		void addRow();
		void removeRow();
		void save();

	private:
		void populateTable();
		void appendRow(const QString &text, const QColor &color, bool bicolor, const QColor &color_2);

		QETProject *m_project;
		QTableWidget *m_table;
};

#endif // CONDUCTORCOLORRULESW_H
