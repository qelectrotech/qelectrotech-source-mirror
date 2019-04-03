/*
	Copyright 2006-2019 The QElectroTech Team
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
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"
#include "independenttextitem.h"
#include "inditextpropertieswidget.h"

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
	
	int count_ = m_diagram->selectedItems().size();
	
		//The editor widget can only edit one item
		//or several items of the same type
	if (count_ != 1)
	{
		if (count_ == 0) {
			clear();
			m_edited_qgi_type = -1;
			return;
		}
		
		const QList<QGraphicsItem *> list_ = m_diagram->selectedItems();
		int type_ = list_.first()->type();
		for (QGraphicsItem *qgi : list_)
		{
			if (qgi->type() != type_)
			{
				clear();
				m_edited_qgi_type = -1;
				return;
			}
		}
	}

	QGraphicsItem *item = m_diagram->selectedItems().first();
	const int type_ = item->type();

	switch (type_)
	{
		case Element::Type: //1000
		{
			if (count_ > 1)
			{
				clear();
				m_edited_qgi_type = -1;
				return;
			}
				//We already edit an element, just update the editor with a new element
			if (m_edited_qgi_type == type_)
			{
				static_cast<ElementPropertiesWidget*>(editors().first())->setElement(static_cast<Element*>(item));
				return;
			}

			clear();
			m_edited_qgi_type = type_;
			addEditor(new ElementPropertiesWidget(static_cast<Element*>(item), this));
			break;
		}
		case IndependentTextItem::Type: //1005
		{
			QList<IndependentTextItem *> text_list;
			for (QGraphicsItem *qgi : m_diagram->selectedItems()) {
				text_list.append(static_cast<IndependentTextItem*>(qgi));
			}
			
			if (m_edited_qgi_type == type_)
			{
				static_cast<IndiTextPropertiesWidget*>(editors().first())->setText(text_list);
				return;
			}
			
			clear();
			m_edited_qgi_type = type_;
			addEditor(new IndiTextPropertiesWidget(text_list, this));
			break;
		}
		case DiagramImageItem::Type: //1007
		{
			if (count_ > 1)
			{
				clear();
				m_edited_qgi_type = -1;
				return;
			}
			
			clear();
			m_edited_qgi_type = type_;
			addEditor(new ImagePropertiesWidget(static_cast<DiagramImageItem*>(item), this));
			break;
		}
		case QetShapeItem::Type: //1008
		{
			QList<QetShapeItem *> shapes_list;
			for (QGraphicsItem *qgi : m_diagram->selectedItems()) {
				shapes_list.append(static_cast<QetShapeItem*>(qgi));
			}

			if (m_edited_qgi_type == type_)
			{
				static_cast<ShapeGraphicsItemPropertiesWidget*>(editors().first())->setItems(shapes_list);
				return;
			}

			clear();
			m_edited_qgi_type = type_;
			addEditor(new ShapeGraphicsItemPropertiesWidget(shapes_list, this));
			break;
		}
		case DynamicElementTextItem::Type: //1010
		{
			if (count_ > 1)
			{
				clear();
				m_edited_qgi_type = -1;
				return;
			}
			
			DynamicElementTextItem *deti = static_cast<DynamicElementTextItem *>(item);
			
				//For dynamic element text, we open the element editor to edit it
				//If we already edit an element, just update the editor with a new element
			if (m_edited_qgi_type == Element::Type)
			{
				static_cast<ElementPropertiesWidget*>(editors().first())->setDynamicText(deti);
				return;
			}
			
			clear();
			m_edited_qgi_type = Element::Type;
			addEditor(new ElementPropertiesWidget(deti, this));
			break;
		}
		case QGraphicsItemGroup::Type:
		{
			if (count_ > 1)
			{
				clear();
				m_edited_qgi_type = -1;
				return;
			}
			
			if(ElementTextItemGroup *group = dynamic_cast<ElementTextItemGroup *>(item))
			{
					//For element text item group, we open the element editor to edit it
					//If we already edit an element, just update the editor with a new element
				if(m_edited_qgi_type == Element::Type)
				{
					static_cast<ElementPropertiesWidget *>(editors().first())->setTextsGroup(group);
					return;
				}
				
				clear();
				m_edited_qgi_type = Element::Type;
				addEditor(new ElementPropertiesWidget(group, this));
			}
			break;
		}
		default:
			m_edited_qgi_type = -1;
			clear();
	}

	for (PropertiesEditorWidget *pew : editors()) {
		pew->setLiveEdit(true);
	}
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
