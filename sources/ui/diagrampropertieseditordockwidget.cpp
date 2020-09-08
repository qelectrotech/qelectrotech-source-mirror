/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "diagrampropertieseditordockwidget.h"
#include "diagram.h"
#include "PropertiesEditor/propertieseditorwidget.h"
#include "propertieseditorfactory.h"

/**
	@brief DiagramPropertiesEditorDockWidget::DiagramPropertiesEditorDockWidget
	Constructor
	@param parent : parent widget
*/
DiagramPropertiesEditorDockWidget::DiagramPropertiesEditorDockWidget(QWidget *parent) :
	PropertiesEditorDockWidget(parent),
	m_diagram(nullptr),
	m_edited_qgi_type (-1)
{}

/**
	@brief DiagramPropertiesEditorDockWidget::setDiagram
	Set the diagram to edit the selection.
	Connect the diagram signal selectionChanged() to this slot selectionChanged();
	If diagram = nullptr, we just disconnect all signal and remove editor.
	@param diagram
*/
void DiagramPropertiesEditorDockWidget::setDiagram(Diagram *diagram)
{
	if (m_diagram == diagram) return;

	if (m_diagram)
	{
		disconnect(m_diagram, SIGNAL(selectionChanged()),
			   this, SLOT(selectionChanged()));
		disconnect(m_diagram, SIGNAL(destroyed()),
			   this, SLOT(diagramWasDeleted()));
	}

	if (diagram)
	{
		m_diagram = diagram;
		connect(m_diagram, SIGNAL(selectionChanged()),
			this, SLOT(selectionChanged()), Qt::QueuedConnection);
		connect(m_diagram, SIGNAL(destroyed()),
			this, SLOT(diagramWasDeleted()));
		selectionChanged();
	}
	else
	{
		m_diagram = nullptr;
		m_edited_qgi_type = -1;
		clear();
	}
}

/**
	@brief DiagramPropertiesEditorDockWidget::selectionChanged
	The current selection of diagram was changed.
	We fill the dock with the appropriate ElementPropertiesWidget of the current selection.
*/
void DiagramPropertiesEditorDockWidget::selectionChanged()
{
	if (!m_diagram) {
		return;
	}

	auto editor_ = PropertiesEditorFactory::propertiesEditor(
				m_diagram->selectedItems(),
				editors().count() ? editors().first() : nullptr,
				this);
	if (!editor_) {
		clear();
		return;
	}
	if (editors().count() &&
		editors().first() != editor_) {
		clear();
	}

	addEditor(editor_);
	for (PropertiesEditorWidget *pew : editors()) {
		pew->setLiveEdit(true);
	}
}

/**
	@brief DiagramPropertiesEditorDockWidget::diagramWasDeleted
	Remove current editor and set m_diagram to nullptr.
*/
void DiagramPropertiesEditorDockWidget::diagramWasDeleted()
{
	m_diagram = nullptr;
	m_edited_qgi_type = -1;
	clear();
}
