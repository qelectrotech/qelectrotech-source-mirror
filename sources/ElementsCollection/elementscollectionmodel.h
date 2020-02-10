/*
	Copyright 2006-2019 The QElectroTech Team
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

		QVariant data(const QModelIndex &index, int role) const override;
		QMimeData *mimeData(const QModelIndexList &indexes) const override;
		QStringList mimeTypes() const override;
		bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
		bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

		void loadCollections(bool common_collection, bool custom_collection, QList<QETProject *> projects);

		void addCommonCollection(bool set_data = true);
		void addCustomCollection(bool set_data = true);
		void addLocation(const ElementsLocation& location);

		void addProject(QETProject *project, bool set_data = true);
		void removeProject(QETProject *project);
		QList<QETProject *> project() const;
		void highlightUnusedElement();


		QList <ElementCollectionItem *> items() const;
		QList <ElementCollectionItem *> projectItems(QETProject *project) const;
		void hideElement();
		bool isHideElement() {return m_hide_element;}
		QModelIndex indexFromLocation(const ElementsLocation &location);

	signals:
		void loadingProgressValueChanged(int);
		void loadingProgressRangeChanged(int, int);
		void loadingFinished();

	private:
		void elementIntegratedToCollection (const QString& path);
		void itemRemovedFromCollection (const QString& path);
		void updateItem (const QString& path);

	private:
		QList <QETProject *> m_project_list;
		QHash <QETProject *, XmlProjectElementCollectionItem *> m_project_hash;
		bool m_hide_element = false;
		QFuture<void> m_future;
		QList <ElementCollectionItem *> m_items_list_to_setUp;
};

#endif // ELEMENTSCOLLECTIONMODEL2_H
