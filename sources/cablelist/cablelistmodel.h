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
#ifndef CABLELISTMODEL_H
#define CABLELISTMODEL_H

#include <QAbstractTableModel>
#include <QDomElement>
#include <QHash>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QStringList>
#include <QVector>

class Cable;
class Diagram;
class Element;
class QETProject;

/**
	@brief The CableListModel class

	The rows of a cable list, shown by QetGraphicsTableItem the way the
	project database model shows a nomenclature -- and shaped after it
	on purpose: same roles, same header behaviour, same way of saving
	itself to the project file, so that the table on the sheet treats
	this model exactly like any other.

	Whereas ProjectDBModel asks the project database, this model asks
	CableList::rows(), straight from the cables of the project. It
	refreshes itself whenever anything those rows are worked out from
	changes: a cable edited, a cable added or removed, a folio added,
	removed or re-informed (its title block carries the installation and
	the place), a label typed over on a piece a cable lands on.
*/
class CableListModel : public QAbstractTableModel
{
	Q_OBJECT

	public:
		explicit CableListModel(QETProject *project, QObject *parent = nullptr);
		CableListModel(const CableListModel &other_model);

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

			/// The columns this table shows, in order: keys of CableList
		void setFields(const QStringList &fields);
		QStringList fields() const {return m_fields;}
			/// The same, joined the way the project file stores it
		QString queryString() const;
		void setIdentifier(const QString &identifier) {m_identifier = identifier;}
		QString identifier() const {return m_identifier;}
		QETProject *project() const;

		QDomElement toXml(QDomDocument &document) const;
		void fromXml(const QDomElement &element);
		static QString xmlTagName() {return QStringLiteral("cable_list_model");}

	public slots:
			/// Work the rows out again and tell whoever shows them
		void refresh();

	private:
			/// Watch everything the rows are worked out from
		void connectProject();

		QPointer<QETProject> m_project;
		QStringList m_fields;
		QVector<QMap<QString, QString>> m_rows;
		//First int = section, second int = Qt::role, QVariant = value
		QHash<int, QHash<int, QVariant>> m_header_data;
		QHash<int, QVariant> m_index_0_0_data;
		QString m_identifier = QStringLiteral("cable_list");
		/// The current watchers of this model, dropped and taken
		/// again on every refresh
		QList<QMetaObject::Connection> m_watchers;
};

#endif // CABLELISTMODEL_H
