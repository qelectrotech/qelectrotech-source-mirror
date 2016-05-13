/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef PROJECTPROPERTIESDIALOG_H
#define PROJECTPROPERTIESDIALOG_H
#include <QtCore>

class QETProject;
class QWidget;
class ConfigDialog;

/**
 * @brief The ProjectPropertiesDialog class
 * this class builds a dialog to edit whole properties of a project
 */
class ProjectPropertiesDialog : public QObject {
	public:
		enum Page {
			Main    = 0,
			Diagram = 1,
			Autonum = 2
		};

		ProjectPropertiesDialog(QETProject *project, QWidget *parent = 0);
		~ProjectPropertiesDialog();
		void exec();
		void setCurrentPage(ProjectPropertiesDialog::Page);
        void changeToFolio();

	private:
		ConfigDialog *m_properties_dialog;
};

#endif // PROJECTPROPERTIESDIALOG_H
