/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef ELEMENTSCOLLECTIONMODEL2_H
#define ELEMENTSCOLLECTIONMODEL2_H

#include <QStandardItemModel>
#include "elementslocation.h"

class XmlProjectElementCollectionItem;
class ElementCollectionItem;
template<> class QList<QETProject>;
template<> class QHash<QETProject, XmlProjectElementCollectionItem>;
template<> class QList<ElementCollectionItem>;


class ElementsCollectionModel : public QStandardItemModel
{
	Q_OBJECT

	public:
		ElementsCollectionModel(QObject *parent = Q_NULLPTR);

		virtual QVariant data(const QModelIndex &index, int role) const;
		virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
		virtual QStringList mimeTypes() const;
		virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;
		virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

		void loadCollections(bool common_collection, bool custom_collection, QList<QETProject *> projects);

		void addCommonCollection(bool set_data = true);
		void addCustomCollection(bool set_data = true);
		void addLocation(ElementsLocation location);

		void addProject(QETProject *project, bool set_data = true);
		void removeProject(QETProject *project);
		QList<QETProject *> project() const;
		void highlightUnusedElement();


		QList <ElementCollectionItem *> items() const;
		void hideElement();
		bool isHideElement() {return m_hide_element;}
		QModelIndex indexFromLocation(const ElementsLocation &location);

	signals:
		void loadingMaxValue(int);
		void loadingProgressValue(int);

	private:
		void elementIntegratedToCollection (QString path);
		void itemRemovedFromCollection (QString path);
		void updateItem (QString path);

	private:
		QList <QETProject *> m_project_list;
		QHash <QETProject *, XmlProjectElementCollectionItem *> m_project_hash;
		bool m_hide_element = false;
};

#endif // ELEMENTSCOLLECTIONMODEL2_H
