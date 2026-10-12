/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "cablelistmodel.h"

#include "cablelistrows.h"

#include "../cable/cable.h"
#include "../diagram.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"
#include "../qetxml.h"
#include "../utils/qetutils.h"

#include <QFont>
#include <QMetaEnum>

#include <algorithm>
#include <utility>

/**
	@brief CableListModel::CableListModel
	@param project : project whose cables this model lists
	@param parent : parent QObject
*/
CableListModel::CableListModel(QETProject *project, QObject *parent) :
	QAbstractTableModel(parent),
	m_project(project)
{
		//Left is what a table starts with and stays with: the factory
		//overwrites it with the alignment chosen in the dialog, and a
		//table which never got that far (no column selected at
		//placement) still draws its cells left rather than from an
		//empty role
	m_index_0_0_data.insert(Qt::TextAlignmentRole, int(Qt::AlignLeft));
		//The same gap between the text and the cell lines the creation
		//dialog offers for the material list, already in place for a
		//table which starts out empty and gets its columns later --
		//the factory overwrites it with the margins chosen in the
		//dialog as soon as the table has a cell to configure
	m_index_0_0_data.insert(Qt::UserRole+1,
							QETUtils::marginsToString(QMargins(5,5,10,5)));
	connectProject();
	setFields(CableList::defaultKeys());
}

/**
	@brief CableListModel::CableListModel
	@param other_model
*/
CableListModel::CableListModel(const CableListModel &other_model) :
	QAbstractTableModel(other_model.parent())
{
	setParent(other_model.parent());
	m_project = other_model.m_project;
	m_identifier = other_model.m_identifier;
	connectProject();
	setFields(other_model.m_fields);
	m_header_data = other_model.m_header_data;
	m_index_0_0_data = other_model.m_index_0_0_data;
}

/**
	@brief CableListModel::rowCount
	Reimplemented for QAbstractTableModel
	@param parent
	@return
*/
int CableListModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

		//A table built without a single column shows no rows either:
		//the empty frame a nomenclature table makes when it is
		//created without entries, not a list of rows nobody can see
	if (m_fields.isEmpty()) {
		return 0;
	}

	return m_rows.count();
}

/**
	@brief CableListModel::columnCount
	Reimplemented for QAbstractTableModel
	@param parent
	@return
*/
int CableListModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return m_fields.count();
}

/**
	@brief CableListModel::setHeaderData
	Reimplemented from QAbstractTableModel.
	Only horizontal orientation is accepted.
	@param section
	@param orientation
	@param value
	@param role
	@return
*/
bool CableListModel::setHeaderData(int section, Qt::Orientation orientation,
								   const QVariant &value, int role)
{
	if (orientation == Qt::Vertical) {
		return false;
	}
	auto hash_ = m_header_data.value(section);
	hash_.insert(role, value);
	m_header_data.insert(section, hash_);
	emit headerDataChanged(orientation, section, section);
	return true;
}

/**
	@brief CableListModel::headerData
	Reimplemented for QAbstractTableModel
	@param section
	@param orientation
	@param role
	@return
*/
QVariant CableListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Vertical) {
		return QVariant();
	}

	if (m_header_data.contains(section))
	{
		auto hash_ = m_header_data.value(section);
		if (role == Qt::DisplayRole && !hash_.contains(Qt::DisplayRole)) { //special case to have the same behavior as Qt
			return hash_.value(Qt::EditRole);
		}
		return m_header_data.value(section).value(role);
	}
	return QVariant();
}

/**
	@brief CableListModel::setData
	Only store the data for the index 0.0. An empty table has no cells
	yet -- index(0,0) does not exist without a single column -- but
	its look is still settable: the factory configuring a freshly made
	table must not be thrown away just because the table was placed
	without entries and gains its columns afterwards. The data is read
	as soon as cells exist.
	@param index
	@param value
	@param role
	@return
*/
bool CableListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid() && (index.row() != 0 || index.column() != 0)) {
		return false;
	}
	m_index_0_0_data.insert(role, value);
	if (index.isValid()) {
		emit dataChanged(index, index, {role});
	}
	return true;
}

/**
	@brief CableListModel::data
	Reimplemented for QAbstractTableModel
	@param index
	@param role
	@return
*/
QVariant CableListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() == 0 &&
		index.column() == 0 &&
		role != Qt::DisplayRole) {
		return m_index_0_0_data.value(role);
	}

	if (role == Qt::DisplayRole) {
		return m_rows.at(index.row()).value(m_fields.at(index.column()));
	}

	return QVariant();
}

/**
	@brief CableListModel::project
	@return the project whose cables this model lists
*/
QETProject *CableListModel::project() const
{
	return m_project.data();
}

/**
	@brief CableListModel::setFields
	Set which columns this table shows, in that order, and work the rows
	out along the way. Unknown keys are dropped rather than shown as
	empty columns: a file naming a column this program does not know
	would draw a blank header nobody can fill.
	@param fields the keys of CableList::allColumns()
*/
void CableListModel::setFields(const QStringList &fields)
{
	QStringList kept;
	for (const QString &field : fields)
	{
		if (kept.contains(field)) {
			continue;
		}
		if (CableList::labelOf(field).isEmpty()) {
			continue;
		}
		kept << field;
	}

	beginResetModel();
	m_fields = kept;
		//The columns themselves change, their labels are written again
		//below -- but HOW a header is drawn (font, alignment, margins)
		//belongs to the table, not to the column set, and stays as it
		//is across a column edit. Sections which no longer exist are
		//dropped, so a stale header of an old column cannot linger
	for (auto it = m_header_data.begin(); it != m_header_data.end(); ) {
		if (it.key() >= m_fields.count()) {
			it = m_header_data.erase(it);
		} else {
			++it;
		}
	}
	for (int i = 0; i < m_fields.count(); ++i) {
		m_header_data[i].insert(Qt::DisplayRole, CableList::labelOf(m_fields.at(i)));
	}
	m_rows = m_project ? CableList::rows(m_project)
					   : QVector<QMap<QString, QString>>();
	endResetModel();
}

/**
	@brief CableListModel::queryString
	@return the fields joined the way the project file stores them
*/
QString CableListModel::queryString() const
{
	return m_fields.join(QLatin1Char(','));
}

/**
	@brief CableListModel::refresh
	Work the rows out again, and watch everything they are worked out
	from: a cable of the project, a folio, a piece a cable lands on
	(its label is the BMK of an end, its Installation and Localisation
	are what those two columns of an end read).
*/
void CableListModel::refresh()
{
	for (const QMetaObject::Connection &connection : std::as_const(m_watchers)) {
		QObject::disconnect(connection);
	}
	m_watchers.clear();

	if (m_project)
	{
		const QVector<Cable *> cables = m_project->cables();
		for (Cable *cable : cables) {
			m_watchers << connect(cable, &Cable::changed,
								  this, &CableListModel::refresh);
		}

		const QList<Diagram *> diagrams = m_project->diagrams();
		for (Diagram *diagram : diagrams)
		{
			m_watchers << connect(diagram, &Diagram::diagramInformationChanged,
								  this, &CableListModel::refresh);
			const QList<Element *> elements = diagram->elements();
			for (Element *element : elements) {
				m_watchers << connect(element, &Element::elementInfoChange,
									  this, &CableListModel::refresh);
			}
		}
	}

	beginResetModel();
	m_rows = m_project ? CableList::rows(m_project)
					   : QVector<QMap<QString, QString>>();
	endResetModel();
}

/**
	@brief CableListModel::connectProject
	Watch the project itself: cables and folios coming and going reach
	this model even though no refresh of ours was running to catch them
	(the rows of a project being read from its file are filled cable by
	cable, one signal at a time).
*/
void CableListModel::connectProject()
{
	if (!m_project) {
		return;
	}

	connect(m_project, &QETProject::cableAdded, this, &CableListModel::refresh);
	connect(m_project, &QETProject::cableRemoved, this, &CableListModel::refresh);
	connect(m_project, &QETProject::diagramAdded, this, &CableListModel::refresh);
	connect(m_project, &QETProject::diagramRemoved, this, &CableListModel::refresh);
}

/**
	@brief CableListModel::toXml
	Save the model to xml: which columns are shown, the look of the
	cells and of the header -- the rows themselves are not saved, they
	are worked out again from the cables whenever the project opens.
	@param document
	@return
*/
QDomElement CableListModel::toXml(QDomDocument &document) const
{
	auto dom_element = document.createElement(xmlTagName());

	//Identifier
	auto dom_identifier = document.createElement("identifier");
	auto dom_identifier_text = document.createTextNode(m_identifier);
	dom_identifier.appendChild(dom_identifier_text);
	dom_element.appendChild(dom_identifier);

	//Fields
	auto dom_fields = document.createElement("fields");
	auto dom_fields_text = document.createTextNode(queryString());
	dom_fields.appendChild(dom_fields_text);
	dom_element.appendChild(dom_fields);

	//Add index 0,0 data
	auto index_00 = document.createElement("index00");
	index_00.setAttribute("font", QETUtils::fontToString(m_index_0_0_data.value(Qt::FontRole).value<QFont>()));
	auto me = QMetaEnum::fromType<Qt::Alignment>();
	index_00.setAttribute("alignment", me.valueToKey(m_index_0_0_data.value(Qt::TextAlignmentRole).toInt()));
	dom_element.appendChild(index_00);
	index_00.setAttribute("margins", m_index_0_0_data.value(Qt::UserRole+1).toString());

	//header data
	QHash<int, QList<int>> horizontal_;
	for (auto key : m_header_data.keys())
	{
		//We save all data except the display role, because it is
		//generated from the fields above (same reason and same sorted
		//order as ProjectDBModel, so the file stays reproducible)
		auto list = m_header_data.value(key).keys();
		list.removeAll(Qt::DisplayRole);
		std::sort(list.begin(), list.end());

		horizontal_.insert(key, list);
	}

	dom_element.appendChild(QETXML::modelHeaderDataToXml(document, this, horizontal_, QHash<int, QList<int>>()));

	return dom_element;
}

/**
	@brief CableListModel::fromXml
	Restore the model from xml
	@param element
*/
void CableListModel::fromXml(const QDomElement &element)
{
	if (element.tagName() != xmlTagName())
		return;

	setIdentifier(element.firstChildElement("identifier").text());
	setFields(element.firstChildElement("fields").text()
			  .split(QLatin1Char(','), Qt::SkipEmptyParts));

	//Index 0,0
	auto index_00 = element.firstChildElement("index00");
 QFont font_;
	QETUtils::fontFromString(font_, index_00.attribute("font"));
	m_index_0_0_data.insert(Qt::FontRole, font_);
	auto me = QMetaEnum::fromType<Qt::Alignment>();
	const int alignment = me.keyToValue(index_00.attribute("alignment")
										 .toStdString().data());
		//A file written without an alignment (an empty or unknown enum
		//key) would restore as -1, whose bits draw every cell right-
		//aligned -- left is the default instead
	m_index_0_0_data.insert(Qt::TextAlignmentRole,
							alignment > 0 ? alignment : int(Qt::AlignLeft));
		//A file saved without cell margins (one written before the
		//look of an empty table was kept) restores with the same
		//default the creation dialog offers, instead of text drawn
		//against the cell lines
	const QString margins_ = index_00.attribute("margins");
	m_index_0_0_data.insert(Qt::UserRole+1,
							margins_.isEmpty()
								? QETUtils::marginsToString(QMargins(5,5,10,5))
								: margins_);

	QETXML::modelHeaderDataFromXml(element.firstChildElement("header_data"), this);
}
