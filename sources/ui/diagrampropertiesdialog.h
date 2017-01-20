/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef DIAGRAMPROPERTIESDIALOG_H
#define DIAGRAMPROPERTIESDIALOG_H

#include "qdialog.h"

class Diagram;
class AutonumSelectorWidget;

/**
 * @brief The DiagramPropertiesDialog class
 * This dialog open a windows for edit properties of a diagram.
 */
class DiagramPropertiesDialog : public QDialog {
		Q_OBJECT

	public:
		DiagramPropertiesDialog(Diagram *diagram, QWidget *parent = 0);

		static void diagramPropertiesDialog(Diagram *diagram, QWidget *parent = 0);

	public slots:
		void editAutonum ();
        void editAutoFolioNum ();

	private:
		Diagram               *m_diagram;
		AutonumSelectorWidget *m_asw;
};

#endif // DIAGRAMPROPERTIESDIALOG_H
