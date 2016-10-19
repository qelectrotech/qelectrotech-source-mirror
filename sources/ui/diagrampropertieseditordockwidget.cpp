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
#include "diagrampropertieseditordockwidget.h"
#include "elementpropertieswidget.h"
#include "diagram.h"
#include "element.h"
#include "diagramimageitem.h"
#include "imagepropertieswidget.h"
#include "qetshapeitem.h"
#include "shapegraphicsitempropertieswidget.h"

/**
 * @brief DiagramPropertiesEditorDockWidget::DiagramPropertiesEditorDockWidget
 * Constructor
 * @param parent : parent widget
 */
DiagramPropertiesEditorDockWidget::DiagramPropertiesEditorDockWidget(QWidget *parent) :
	PropertiesEditorDockWidget(parent),
	m_diagram(nullptr),
	m_edited_qgi_type (-1)
{}

/**
 * @brief DiagramPropertiesEditorDockWidget::setDiagram
 * Set the diagram to edit the selection.
 * Connect the diagram signal selectionChanged() to this slot selectionChanged();
 * If diagram = nullptr, we just disconnect all signal and remove editor.
 * @param diagram
 * @param diagram
 */
void DiagramPropertiesEditorDockWidget::setDiagram(Diagram *diagram)
{
	if (m_diagram == diagram) return;

	if (m_diagram)
	{
		disconnect(m_diagram, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
		disconnect(m_diagram, SIGNAL(destroyed()),        this, SLOT(diagramWasDeleted()));
	}

	if (diagram)
	{
		m_diagram = diagram;
		connect(m_diagram, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()), Qt::QueuedConnection);
		connect(m_diagram, SIGNAL(destroyed()),        this, SLOT(diagramWasDeleted()));
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
 * @brief DiagramPropertiesEditorDockWidget::selectionChanged
 * The current selection of diagram was changed.
 * We fill the dock with the appropriate ElementPropertiesWidget of the current selection.
 */
void DiagramPropertiesEditorDockWidget::selectionChanged()
{
	if (!m_diagram) return;
		//This editor can edit only one item.
	if (m_diagram->selectedItems().size() != 1)
	{
		clear();
		m_edited_qgi_type = -1;
		return;
	}

	QGraphicsItem *item = m_diagram->selectedItems().first();
	const int type_ = item->type();

	switch (type_)
	{
		case Element::Type: {
				//We already edit an element, just update the editor with a new element
			if (m_edited_qgi_type == type_)
			{
				static_cast<ElementPropertiesWidget*>(editors().first())->setElement(static_cast<Element*>(item));
				return;
			}

			clear();
			m_edited_qgi_type = type_;
			addEditor(new ElementPropertiesWidget(static_cast<Element*>(item), this));
			break; }

		case DiagramImageItem::Type: {
			clear();
			m_edited_qgi_type = type_;
			addEditor(new ImagePropertiesWidget(static_cast<DiagramImageItem*>(item), this));
			break; }

		case QetShapeItem::Type: {
			if (m_edited_qgi_type == type_)
			{
				static_cast<ShapeGraphicsItemPropertiesWidget*>(editors().first())->setItem(static_cast<QetShapeItem*>(item));
				return;
			}

			clear();
			m_edited_qgi_type = type_;
			addEditor(new ShapeGraphicsItemPropertiesWidget(static_cast<QetShapeItem*>(item), this));
			break; }

		default:
			m_edited_qgi_type = -1;
			clear();
	}

	foreach (PropertiesEditorWidget *pew, editors())
		pew->setLiveEdit(true);
}

/**
 * @brief DiagramPropertiesEditorDockWidget::diagramWasDeleted
 * Remove current editor and set m_diagram to nullptr.
 */
void DiagramPropertiesEditorDockWidget::diagramWasDeleted()
{
	m_diagram = nullptr;
	m_edited_qgi_type = -1;
	clear();
}
