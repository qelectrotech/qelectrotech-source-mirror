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
#include "elementprovider.h"
#include "qetproject.h"
#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicstableitem.h"
#include <QAbstractItemModel>

/**
	@brief ElementProvider::ElementProvider Constructor
	@param prj the project where we must find element
	@param diagram the diagram to exclude from the search
*/
ElementProvider::ElementProvider(QETProject *prj, Diagram *diagram)
{
	m_diagram_list = prj->diagrams();
	m_diagram_list.removeOne(diagram);
}

/**
	@brief ElementProvider::ElementProvider Constructor
	@param diag Diagram to search
*/
ElementProvider::ElementProvider(Diagram *diag) {
	m_diagram_list << diag;
}

/**
	@brief ElementProvider::FreeElement
	Search and return the asked element corresponding with the given filter
	All returned element are free,
	ie element aren't connected with another element
	@param filter
	the filter for search element
	(You can find all filter with the define in Element.h)
	@return
*/
QList <Element *> ElementProvider::freeElement(const int filter) const{
	QList <Element *> free_elmt;

	//serch in all diagram
	foreach (Diagram *d, m_diagram_list) {
		//get all element in diagram d
		QList <Element *> elmt_list;
		elmt_list = d->elements();
		foreach (Element *elmt, elmt_list) {
			if (filter & elmt->linkType())
				if (elmt->isFree()) free_elmt << elmt;
		}
	}
	return (free_elmt);
}

/**
	@brief ElementProvider::fromUuids
	@param uuid_list list of uuid must be found
	@return all elements with uuid corresponding to uuid in uuid_list
*/
QList <Element *> ElementProvider::fromUuids(QList<QUuid> uuid_list) const
{
	QList <Element *> found_element;

	foreach (Diagram *d, m_diagram_list) {
		foreach(Element *elmt, d->elements()) {
			if (uuid_list.contains(elmt->uuid())) {
				found_element << elmt;
				uuid_list.removeAll(elmt->uuid());
			}
		}
	}
	return found_element;
}

/**
	@brief ElementProvider::find
	Search and return the asked element corresponding  with the given filter
	@param filter
	the filter for search element
	(You can find all filter with the define in Element.h)
*/
QList <Element *> ElementProvider::find(const int filter) const
{
	QList <Element *> elmt_;

	//serch in all diagram
	foreach (Diagram *d, m_diagram_list) {
		//get all element in diagram d
		QList <Element *> elmt_list;
		elmt_list = d->elements();
		foreach (Element *elmt, elmt_list) {
			if (filter & elmt->linkType())
				elmt_ << elmt;
		}
	}
	return (elmt_);
}

/**
	@brief ElementProvider::table
	@param table
	@param model
	@return All tables wich display the derivated class of model (if set)
	and not already in all the chain of next/previous table of table (if set)
	If table and model are nullptr, return every tables
*/
QVector<QetGraphicsTableItem *> ElementProvider::table(
		QetGraphicsTableItem *table,
		QAbstractItemModel *model)
{
	QVector<QetGraphicsTableItem *> v_;
	QVector<QetGraphicsTableItem *> linked_vector;

	if (table)
	{
		 //table can be inside a chain, at the head of a chain or alone
		auto linked_table = table->previousTable()
				? table->previousTable()
				: table->nextTable();
		while (linked_table) { //Go to the first table
			if (linked_table->previousTable())
				linked_table = linked_table->previousTable();
			else
				break;
		}
		while (linked_table) { //Store each linked table in linked_vector
			linked_vector.append(linked_table);
			linked_table = linked_table->nextTable();
		}
	}

	for (auto d : m_diagram_list) {
		for (auto item_ : d->items())
		{
			if(item_->type() == QetGraphicsTableItem::Type)
			{
				auto found_table = static_cast<QetGraphicsTableItem *>(item_);

				if (linked_vector.contains(found_table)) {
					continue;
				}

				if (!model ||
					(found_table->model() &&
					 model->metaObject()->className() == found_table->model()->metaObject()->className()))
				{v_.append(found_table);}
			}
		}
	}

	return v_;
}

/**
	@brief ElementProvider::tableFromUuid
	@param uuid
	@return the table with uuid uuid or nullptr if not found
*/
QetGraphicsTableItem *ElementProvider::tableFromUuid(const QUuid &uuid)
{
	for (auto table : this->table())
		if (table->uuid() == uuid)
			return table;

	return nullptr;
}
