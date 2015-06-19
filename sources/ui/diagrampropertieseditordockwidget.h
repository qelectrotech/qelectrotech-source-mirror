/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef DIAGRAMPROPERTIESEDITORDOCKWIDGET_H
#define DIAGRAMPROPERTIESEDITORDOCKWIDGET_H

#include "PropertiesEditor/propertieseditordockwidget.h"

class Diagram;

class DiagramPropertiesEditorDockWidget : public PropertiesEditorDockWidget
{
		Q_OBJECT

	public:
		DiagramPropertiesEditorDockWidget(QWidget *parent = nullptr);

		void setDiagram(Diagram *diagram);

	private: //Make this method private because only this class manage the editor widget.
		bool addEditor(PropertiesEditorWidget *editor, int index = 0) { return PropertiesEditorDockWidget::addEditor(editor, index); }
		bool removeEditor(PropertiesEditorWidget *editor) { return PropertiesEditorDockWidget::removeEditor(editor); }

	private slots:
		void selectionChanged();
		void diagramWasDeleted();

	private:
		Diagram *m_diagram;
		int m_edited_qgi_type;
};

#endif // DIAGRAMPROPERTIESEDITORDOCKWIDGET_H
