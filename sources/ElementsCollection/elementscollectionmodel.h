/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef ELEMENTSCOLLECTIONMODEL_H
#define ELEMENTSCOLLECTIONMODEL_H

#include <QAbstractItemModel>

class ElementCollectionItem;
class QETProject;
class QList<QETProject>;
class XmlProjectElementCollectionItem;

/**
 * @brief The ElementsCollectionModel class
 * Provide a data model for co;llection of elements.
 */
class ElementsCollectionModel : public QAbstractItemModel
{
		Q_OBJECT

	public:
		ElementsCollectionModel(QObject *parent = nullptr);
		~ElementsCollectionModel();

		virtual QModelIndex index (int row, int column, const QModelIndex &parent = QModelIndex()) const;
		virtual QModelIndex parent (const QModelIndex &child) const;

		virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
		virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
		virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

		virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

		virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
		virtual Qt::ItemFlags flags(const QModelIndex &index) const;
		virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;
		virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
		QStringList mimeTypes() const;
		QList <ElementCollectionItem *> items() const;

		void addCommonCollection();
		void addCustomCollection();
		bool addProject(QETProject *project);
		bool removeProject(QETProject *project);
		QList<QETProject *> project() const;

	private:
		XmlProjectElementCollectionItem *itemForProject(QETProject *project);
		void elementIntegratedToCollection (QString path);
		void updateItem (QString path);
			//Use as slot in method drop mime data
		void bir(ElementCollectionItem *eci, int first, int last);
		void brr(ElementCollectionItem *eci, int first, int last);

	private:
		ElementCollectionItem *m_root_item;
		QList <QETProject *> m_project_list;
		QModelIndex m_parent_at_drop;
};

#endif // ELEMENTSCOLLECTIONMODEL_H
