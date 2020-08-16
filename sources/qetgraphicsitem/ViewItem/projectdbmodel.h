/*
	Copyright 2006-2020 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef PROJECTDBMODEL_H
#define PROJECTDBMODEL_H

#include <QAbstractTableModel>
#include <QPointer>
#include <QDomElement>

class QETProject;

/**
	@brief The ProjectDBModel class
	This model is intended to be use with the class projectDataBase
	and is designed to be displayed by the class QetGraphicsTableItem
	(but can be use by other view class since it inherit from QAbstractTableModel).
	This class should be sufficient to display the content of
	the project data base from a query set by the method
	void ProjectDBModel::setQuery(const QString &query).
	The indentifier method is used by widget editor to retrieve
	the good widget for edit the query.
	By defaut identifer return the string 'unknow'.
	You should use setIdentfier method to set your custom identifier.
	At the time this sentence is written, there is two identifier :
	nomenclature
	summary
*/
class ProjectDBModel : public QAbstractTableModel
{
	Q_OBJECT

	public:
		explicit ProjectDBModel(QETProject *project, QObject *parent = nullptr);
		explicit ProjectDBModel (const ProjectDBModel &other_model);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		int columnCount(const QModelIndex &parent = QModelIndex()) const override;
		bool setHeaderData(int section,
				   Qt::Orientation orientation,
				   const QVariant &value,
				   int role = Qt::EditRole) override;
		QVariant headerData(int section,
				    Qt::Orientation orientation,
				    int role = Qt::DisplayRole) const override;
		bool setData(const QModelIndex &index,
			     const QVariant &value,
			     int role = Qt::EditRole) override;
		QVariant data(const QModelIndex &index,
			      int role = Qt::DisplayRole) const override;
		void setQuery(const QString &setQuery);
		QString queryString() const;
		QETProject *project() const;

		QDomElement toXml(QDomDocument &document) const;
		void fromXml(const QDomElement &element);
		void setIdentifier(const QString &identifier);
		QString identifier() const {return m_identifier;}
		static QString xmlTagName() {return QString("project_data_base_model");}

	private:
		void dataBaseUpdated();
		void setHeaderString();
		void fillValue();

	private:
		QPointer<QETProject> m_project;
		QString m_query;
		QVector<QStringList> m_record;
		//First int = section, second int = Qt::role, QVariant = value
		QHash<int, QHash<int, QVariant>> m_header_data;
		QHash<int, QVariant> m_index_0_0_data;
		QString m_identifier = "unknow";
};

#endif // PROJECTDBMODEL_H
