/*
		Copyright 2006-2020 QElectroTech Team
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
#include "propertieseditorfactory.h"
#include "QGraphicsItem"
#include "element.h"
#include "PropertiesEditor/propertieseditorwidget.h"
#include "elementpropertieswidget.h"
#include "element.h"
#include "independenttextitem.h"
#include "inditextpropertieswidget.h"
#include "diagramimageitem.h"
#include "imagepropertieswidget.h"
#include "qetshapeitem.h"
#include "shapegraphicsitempropertieswidget.h"
#include "dynamicelementtextitem.h"
#include "dynamicelementtextitemeditor.h"
#include "elementtextitemgroup.h"
#include "qetgraphicstableitem.h"
#include "graphicstablepropertieseditor.h"
#include "projectdbmodelpropertieswidget.h"
#include "projectdbmodel.h"

/**
	@brief PropertiesEditorFactory::propertiesEditor
	@param model : the model to be edited
	@param editor :
	if the properties editor to be created is the same class as editor,
	the this function set item as edited item of editor and return editor
	@param parent : parent widget of the returned editor
	@return an editor or nullptr
*/
PropertiesEditorWidget *PropertiesEditorFactory::propertiesEditor(
		QAbstractItemModel *model,
		PropertiesEditorWidget *editor,
		QWidget *parent)
{
	if (auto m = static_cast<ProjectDBModel *>(model))
	{
		if (editor &&
			editor->metaObject()->className()
				== ProjectDBModelPropertiesWidget::staticMetaObject.className())
		{
			static_cast<ProjectDBModelPropertiesWidget *>(editor)->setModel(m);
			return editor;
		}
		return new ProjectDBModelPropertiesWidget(m, parent);
	}
	return nullptr;
}

/**
	@brief propertiesEditor
	@param items : The items to be edited
	@param editor :
	If the properties editor to be created is the same class as editor,
	then this function set item as edited item of editor and return editor
	@param parent : parent widget of the returned editor
	@return : an editor or nullptr;
*/
PropertiesEditorWidget *PropertiesEditorFactory::propertiesEditor(
		QList<QGraphicsItem *> items,
		PropertiesEditorWidget *editor,
		QWidget *parent)
{
	const int count_ = items.size();
	if (count_ == 0) {
		return nullptr;
	}
	QGraphicsItem *item = items.first();
	const int type_ = item->type();

		//The editor widget can only edit one item
		//or several items of the same type
	for (auto qgi : items) {
		if (qgi->type() != type_) {
			return nullptr;
		}
	}

	QString class_name;
	if (editor) {
		class_name = editor->metaObject()->className();
	}

	switch (type_)
	{
		case Element::Type: //1000
		{
			if (count_ > 1) {
				return nullptr;
			}
			auto elmt = static_cast<Element*>(item);
			//auto created_editor = new ElementPropertiesWidget(elmt, parent);

				//We already edit an element, just update the editor with a new element
			if (class_name == ElementPropertiesWidget::staticMetaObject.className())
			{
				static_cast<ElementPropertiesWidget*>(editor)->setElement(elmt);
				return  editor;
			}
			return  new ElementPropertiesWidget(elmt, parent);
		}
		case IndependentTextItem::Type: //1005
		{
			QList<IndependentTextItem *> text_list;
			for (QGraphicsItem *qgi : items) {
				text_list.append(static_cast<IndependentTextItem*>(qgi));
			}

			if (class_name == IndiTextPropertiesWidget::staticMetaObject.className())
			{
				static_cast<IndiTextPropertiesWidget*>(editor)->setText(text_list);
				return  editor;
			}

			return new IndiTextPropertiesWidget(text_list, parent);
		}
		case DiagramImageItem::Type: //1007
		{
			if (count_ > 1) {
				return nullptr;
			}
			return new ImagePropertiesWidget(static_cast<DiagramImageItem*>(item), parent);
		}
		case QetShapeItem::Type: //1008
		{
			QList<QetShapeItem *> shapes_list;
			for (QGraphicsItem *qgi : items) {
				shapes_list.append(static_cast<QetShapeItem*>(qgi));
			}

			if (class_name == ShapeGraphicsItemPropertiesWidget::staticMetaObject.className())
			{
				static_cast<ShapeGraphicsItemPropertiesWidget*>(editor)->setItems(shapes_list);
				return editor;
			}

			return new ShapeGraphicsItemPropertiesWidget(shapes_list, parent);
		}
		case DynamicElementTextItem::Type: //1010
		{
			if (count_ > 1) {
				return nullptr;
			}

			DynamicElementTextItem *deti = static_cast<DynamicElementTextItem *>(item); 
				//For dynamic element text, we open the element editor to edit it
				//If we already edit an element, just update the editor with a new element
			if (class_name == ElementPropertiesWidget::staticMetaObject.className())
			{
				static_cast<ElementPropertiesWidget*>(editor)->setDynamicText(deti);
				return editor;
			}
			return new ElementPropertiesWidget(deti, parent);
		}
		case QGraphicsItemGroup::Type:
		{
			if (count_ > 1) {
				return nullptr;
			}

			if(ElementTextItemGroup *group = dynamic_cast<ElementTextItemGroup *>(item))
			{
					//For element text item group, we open the element editor to edit it
					//If we already edit an element, just update the editor with a new element
				if(class_name == ElementPropertiesWidget::staticMetaObject.className())
				{
					static_cast<ElementPropertiesWidget *>(editor)->setTextsGroup(group);
					return editor;
				}
				return new ElementPropertiesWidget(group, parent);
			}
			break;
		}
		case QetGraphicsTableItem::Type:
		{
			if (count_ > 1) {
				return nullptr;
			}

			auto table = static_cast<QetGraphicsTableItem*>(item);
			if (class_name == GraphicsTablePropertiesEditor::staticMetaObject.className())
			{
				static_cast<GraphicsTablePropertiesEditor*>(editor)->setTable(table);
				return editor;
			}
			return new GraphicsTablePropertiesEditor(table, parent);
		}
		default:
			return nullptr;
	}

	return nullptr;
}
