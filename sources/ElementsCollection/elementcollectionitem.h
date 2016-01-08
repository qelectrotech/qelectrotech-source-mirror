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
#ifndef ELEMENTCOLLECTIONITEM_H
#define ELEMENTCOLLECTIONITEM_H

#include <QVariant>

class QMimeData;
class ElementCollectionItem;
class QList<ElementCollectionItem>;

/**
 * @brief The ElementCollectionItem class
 * This class represent a item (a directory or an element) in a element collection.
 * This class must be herited for specialisation.
 * This item is used by ElementsCollectionModel for manage the elements collection
 */
class ElementCollectionItem
{
    public:
        ElementCollectionItem(ElementCollectionItem *parent = nullptr);
		virtual ~ElementCollectionItem();

		enum {Type = 1 , UserType = 100};
		virtual int type() const {return Type;}

        void appendChild (ElementCollectionItem *item);
		bool removeChild (int row, int count);
		bool insertChild (int row, ElementCollectionItem *item);
        ElementCollectionItem *child(int row);
		ElementCollectionItem *childWithCollectionName(QString name) const;
        int childCount() const;
        int columnCount() const;
		virtual QVariant data(int column, int role);
		virtual QMimeData *mimeData();
		virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int column) const;
		virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int column);
		virtual Qt::ItemFlags flags();
        ElementCollectionItem *parent();
        int row() const;
		virtual QString name();
		virtual QString collectionName() const;

		virtual bool isDir() const;
		virtual bool isElement() const;
		virtual bool isValid() const;
		virtual QList <ElementCollectionItem *> items() const;
		QList<ElementCollectionItem *> elementsChild() const;
		QList<ElementCollectionItem *> directoriesChild() const;
		int indexOfChild(ElementCollectionItem *child) const;


		virtual bool canRemoveContent();
		virtual bool removeContent();

	protected:
        ElementCollectionItem *m_parent_item;
        QList <ElementCollectionItem *> m_child_items;
		QString m_name;
};

#endif // ELEMENTCOLLECTIONITEM_H
