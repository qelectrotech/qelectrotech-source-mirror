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
#include "elementprovider.h"

#include "diagram.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminalelement.h"
#include "qetproject.h"

#include <QAbstractItemModel>

/**
	@brief ElementProvider::ElementProvider Constructor
	@param prj the project where we must find element
	@param diagram the diagram to exclude from the search
*/
ElementProvider::ElementProvider(QETProject *prj, Diagram *diagram) :
	m_project(prj),
	m_excluded(diagram)
{}

/**
	@brief ElementProvider::ElementProvider Constructor
	@param diag Diagram to search
*/
ElementProvider::ElementProvider(Diagram *diag) {
	m_diagram_list << diag;
}

/**
	@brief ElementProvider::ElementProvider Constructor
	@param diagrams the diagrams to search, e.g. QETProject::builtFolios()
*/
ElementProvider::ElementProvider(const QList<Diagram *> &diagrams) :
	m_diagram_list(diagrams)
{}

/**
	@brief ElementProvider::folios
	@return the folios to search for symbols of the kinds @p kinds (free
	ones only if @p free_only). For a project, the folios not built yet
	(QET_LAZY_FOLIOS) holding such a symbol are built first, and only them
	(QETProject::buildFoliosHolding()).
*/
QList<Diagram *> ElementProvider::folios(ElementData::Types kinds, bool free_only) const
{
	if (!m_project) {
		return m_diagram_list;
	}
	m_project->buildFoliosHolding(kinds, free_only);
	QList<Diagram *> list = m_project->builtFolios();
	list.removeOne(m_excluded);
	return list;
}

/**
	@brief ElementProvider::allFolios
	@return the folios to search for anything else: every one, built
*/
QList<Diagram *> ElementProvider::allFolios() const
{
	if (!m_project) {
		return m_diagram_list;
	}
	QList<Diagram *> list = m_project->diagrams();
	list.removeOne(m_excluded);
	return list;
}

/**
	@brief ElementProvider::FreeElement
	Search and return the asked element corresponding with the given filter
	All returned element are free,
	ie element aren't connected with another element
	@param filter
	the filter for search element
	@return
*/
QVector <QPointer<Element>> ElementProvider::freeElement(ElementData::Types filter) const
{
	QVector<QPointer<Element>> free_elmt;
	QList<Element *> elmt_list;
	const QList<Diagram *> searched = folios(filter, true);
		//A symbol whose saved partner is on a folio not built yet
		//(QET_LAZY_FOLIOS) is not linked yet, but is not free: it will be
	const QSet<QUuid> waiting = m_project ? m_project->symbolsOnUnbuiltFolios()
										  : QSet<QUuid>();

		//search in all diagram
	for (const auto &diagram_ : std::as_const(searched))
	{
			//get all element in diagram d
		elmt_list = diagram_->elements();
		for (const auto &elmt_ : std::as_const(elmt_list))
		{
			if (filter & elmt_->elementData().m_type &&
				elmt_->isFree() &&
				!elmt_->pendingLinks().intersects(waiting))
			{
				free_elmt << elmt_;
			}
		}
			//Clear for next loop
		elmt_list.clear();
	}

	return free_elmt;
}
/**
	@brief ElementProvider::fromUuids
	@param uuid_list list of uuid must be found
	@return all elements with uuid corresponding to uuid in uuid_list
*/
QList <Element *> ElementProvider::fromUuids(QList<QUuid> uuid_list) const
{
	QList <Element *> found_element;

	foreach (Diagram *d, allFolios()) {
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
 * @brief ElementProvider::find
 * Search and return the element with the type given in parameter
 * @param elmt_type
 * @return
 */
QVector<QPointer<Element>> ElementProvider::find(ElementData::Types elmt_type) const
{
	QVector<QPointer<Element>> returned_vector;
	const QList<Diagram *> searched = folios(elmt_type, false);
	for (const auto &diagram_ : std::as_const(searched))
	{
		const auto elmt_list = diagram_->elements();
		for (const auto &elmt_ : elmt_list)
		{
			if (elmt_type & elmt_->elementData().m_type) {
				returned_vector << QPointer<Element>(elmt_);
			}
		}
	}

	return returned_vector;
}

/**
	@brief ElementProvider::table
	@param table
	@param model
	@return All tables which display the derivated class of model (if set)
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

	for (auto d : allFolios()) {
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

/**
 * @brief ElementProvider::freeTerminal
 * @return a vector of every terminals element who doesn't
 * belong to a terminal strip.
 */
QVector<TerminalElement *> ElementProvider::freeTerminal() const
{
	QVector<TerminalElement *> vector_;
	const QList<Diagram *> searched = folios(ElementData::Terminal, false);

	for (const auto &diagram : std::as_const(searched))
	{
		const auto elmt_list{diagram->elements()};

		for (const auto element : elmt_list)
		{
			if (element->elementData().m_type == ElementData::Terminal)
			{
				const auto te{static_cast<TerminalElement *>(element)};
				if (!te->parentTerminalStrip()) {
					vector_.append(te);
				}
			}
		}
	}

	return vector_;
}
