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
#include "projectdbmodel.h"
#include "projectdatabase.h"
#include "qetproject.h"
#include "qetxml.h"
#include "qetapp.h"

#include <QSqlError>
#include <QSqlRecord>

/**
	@brief ProjectDBModel::ProjectDBModel
	@param project :project of this nomenclature
	@param parent : parent QObject
*/
ProjectDBModel::ProjectDBModel(QETProject *project, QObject *parent) :
	QAbstractTableModel(parent),
	m_project(project)
{
	connect(m_project->dataBase(), &projectDataBase::dataBaseUpdated, this, &ProjectDBModel::dataBaseUpdated);
}

/**
	@brief ProjectDBModel::ProjectDBModel
	@param other_model
*/
ProjectDBModel::ProjectDBModel(const ProjectDBModel &other_model) :
	QAbstractTableModel(other_model.parent())
{
	this->setParent(other_model.parent());
	m_project = other_model.m_project;
	connect(m_project->dataBase(), &projectDataBase::dataBaseUpdated, this, &ProjectDBModel::dataBaseUpdated);
	m_index_0_0_data = other_model.m_index_0_0_data;
	setQuery(other_model.queryString());
}

/**
	@brief ProjectDBModel::rowCount
	Reimplemented for QAbstractTableModel
	@param parent
	@return
*/
int ProjectDBModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	
	return  m_record.count();
}

/**
	@brief ProjectDBModel::columnCount
	Reimplemented for QAbstractTableModel
	@param parent
	@return
*/
int ProjectDBModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	
	if (m_record.count()) {
		return m_record.first().count();
	}
	
	return 0;
}

/**
	@brief ProjectDBModel::setHeaderData
	Reimplemented from QAbstractTableModel.
	Only horizontal orientation is accepted.
	@param section
	@param orientation
	@param value
	@param role
	@return
*/
bool ProjectDBModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
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
	@brief ProjectDBModel::headerData
	Reimplemented from QAbstractTableModel.
	@param section
	@param orientation
	@param role
	@return
*/
QVariant ProjectDBModel::headerData(int section, Qt::Orientation orientation, int role) const
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
	@brief ProjectDBModel::setData
	Only store the data for the index 0.0
	@param index
	@param value
	@param role
	@return
*/
bool ProjectDBModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || index.row() != 0 || index.column() != 0) {
		return false;
	}
	m_index_0_0_data.insert(role, value);
	emit dataChanged(index, index, QVector<int>(role));
	return true;
}

/**
	@brief ProjectDBModel::data
	Reimplemented for QAbstractTableModel
	@param index
	@param role
	@return
*/
QVariant ProjectDBModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	
	if (index.row() == 0 &&
		index.column() == 0 &&
		role != Qt::DisplayRole) {
		return m_index_0_0_data.value(role);
	}
	
	if (role == Qt::DisplayRole) {
		QVariant v(m_record.at(index.row()).at(index.column()));
		return v;
	}
	
	return QVariant();
}

/**
	@brief ProjectDBModel::setQuery
	Query the internall bd with query.
	@param query
*/
void ProjectDBModel::setQuery(const QString &query)
{
	auto rm_ = m_query != query;
	if (rm_) {
		emit beginResetModel();
	}

	m_query = query;
	
	if (m_project)
	{
		if (rm_) {
			disconnect(m_project->dataBase(),
				   &projectDataBase::dataBaseUpdated,
				   this,
				   &ProjectDBModel::dataBaseUpdated);
		}
		m_project->dataBase()->updateDB();
		if (rm_) {
			setHeaderString();
			fillValue();
			connect(m_project->dataBase(),
				&projectDataBase::dataBaseUpdated,
				this,
				&ProjectDBModel::dataBaseUpdated);
		}
	}
	
	if (rm_) {
		emit endResetModel();
	}
}

/**
	@brief ProjectDBModel::queryString
	@return the current query used by this model
*/
QString ProjectDBModel::queryString() const
{
	return m_query;
}

QETProject *ProjectDBModel::project() const
{
	return m_project.data();
}

/**
	@brief ProjectDBModel::toXml
	Save the model to xml,since model can have unlimited data we only save few data (only these used by qelectrotech).
	The query, all header data. and some data of index::(0,0). All other data are not saved.
	@param document
	@return
*/
QDomElement ProjectDBModel::toXml(QDomDocument &document) const
{
	auto dom_element = document.createElement(xmlTagName());
	
	//Identifier
	auto dom_identifier = document.createElement("identifier");
	auto dom_identifier_text = document.createTextNode(m_identifier);
	dom_identifier.appendChild(dom_identifier_text);
	dom_element.appendChild(dom_identifier);
	
	//query
	auto dom_query = document.createElement("query");
	auto dom_query_text = document.createTextNode(m_query);
	dom_query.appendChild(dom_query_text);
	dom_element.appendChild(dom_query);
	
	//Add index 0,0 data
	auto index_00 = document.createElement("index00");
	index_00.setAttribute("font", m_index_0_0_data.value(Qt::FontRole).toString());
	auto me = QMetaEnum::fromType<Qt::Alignment>();
	index_00.setAttribute("alignment", me.valueToKey(m_index_0_0_data.value(Qt::TextAlignmentRole).toInt()));
	dom_element.appendChild(index_00);
	index_00.setAttribute("margins", m_index_0_0_data.value(Qt::UserRole+1).toString());
	
	//header data
	QHash<int, QList<int>> horizontal_;
	for (auto key : m_header_data.keys())
	{
		//We save all data except the display role, because he was generated in the fly
		auto list = m_header_data.value(key).keys();
		list.removeAll(Qt::DisplayRole);
		
		horizontal_.insert(key, list);
	}
	
	dom_element.appendChild(QETXML::modelHeaderDataToXml(document, this, horizontal_, QHash<int, QList<int>>()));
	
	return dom_element;
}

/**
	@brief ProjectDBModel::fromXml
	Restore the model from xml
	@param element
*/
void ProjectDBModel::fromXml(const QDomElement &element)
{
	if (element.tagName() != xmlTagName())
		return;
	
	setIdentifier(element.firstChildElement("identifier").text());
	setQuery(element.firstChildElement("query").text());
	
	//Index 0,0
	auto index_00 = element.firstChildElement("index00");
	QFont font_;
	font_.fromString(index_00.attribute("font"));
	m_index_0_0_data.insert(Qt::FontRole, font_);
	auto me = QMetaEnum::fromType<Qt::Alignment>();
	m_index_0_0_data.insert(Qt::TextAlignmentRole, me.keyToValue(index_00.attribute("alignment").toStdString().data()));
	m_index_0_0_data.insert(Qt::UserRole+1, index_00.attribute("margins"));
	
	QETXML::modelHeaderDataFromXml(element.firstChildElement("header_data"), this);
}

/**
	@brief ProjectDBModel::setIdentifier
	Set the identifier of this model to identifier
	@param identifier
*/
void ProjectDBModel::setIdentifier(const QString &identifier) {
	m_identifier = identifier;
}

/**
	@brief ProjectDBModel::dataBaseUpdated
	slot called when the project database is updated
*/
void ProjectDBModel::dataBaseUpdated()
{
	auto original_record = m_record;
	fillValue();
	auto new_record = m_record;
	m_record = original_record;
	
	if (new_record.size() != m_record.size())
	{
		emit beginResetModel();
		m_record = new_record;
		emit endResetModel();
	}
	else
	{
		m_record = new_record;
		auto row = m_record.size();
		auto col = row ? m_record.first().count() : 1;
		
		emit dataChanged(this->index(0,0), this->index(row-1, col-1), QVector<int>(Qt::DisplayRole));
	}
}

void ProjectDBModel::setHeaderString()
{
	auto q = m_project->dataBase()->newQuery(m_query);
	auto record = q.record();
	
	for (auto i=0 ; i<record.count() ; ++i)
	{
		auto field_name = record.fieldName(i);
		QString header_name;
		
		if (field_name == "position") {
			header_name = tr("Position");
		} else if (field_name == "diagram_position") {
			header_name = tr("Position du folio");
		} else {
			header_name = QETApp::elementTranslatedInfoKey(field_name);
			if (header_name.isEmpty()) {
				header_name = QETApp::diagramTranslatedInfoKey(field_name);
			}
			if (header_name.isEmpty()) {
				header_name = field_name;
			}
		}
		this->setHeaderData(i, Qt::Horizontal, header_name, Qt::DisplayRole);
	}
}

void ProjectDBModel::fillValue()
{
	m_record.clear();
	
	auto query_ = m_project->dataBase()->newQuery(m_query);
	if (!query_.exec()) {
		qDebug() << "Query error : " << query_.lastError();
	}
	
	while (query_.next())
	{
		QStringList record_;
		auto i=0;
		while (query_.value(i).isValid())
		{
			auto date = query_.value(i).toDate();
			if (!date.isNull()) {
				record_ << QLocale::system().toString(query_.value(i).toDate(), QLocale::ShortFormat);
			} else {
				record_ << query_.value(i).toString();
			}
			++i;
		}
		m_record << record_;
	}
}

