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
class ElementCollectionItem : public QObject
{
		Q_OBJECT

    public:
        ElementCollectionItem(ElementCollectionItem *parent = nullptr);
		virtual ~ElementCollectionItem();

		enum {Type = 1 , UserType = 100};
		virtual int type() const {return Type;}

        void appendChild (ElementCollectionItem *item);
		bool removeChild (int row, int count);
		bool insertChild (int row, ElementCollectionItem *item);
		ElementCollectionItem *child(int row) const;
		ElementCollectionItem *childWithCollectionName(QString name) const;
		ElementCollectionItem *lastItemForPath(const QString &path, QString &newt_item);
		ElementCollectionItem *itemAtPath(const QString &path);
		int rowForInsertItem(const QString &collection_name);
		virtual void insertNewItem(const QString &collection_name);
        int childCount() const;
        int columnCount() const;
		virtual QVariant data(int column, int role);
		virtual void clearData();
		virtual QMimeData *mimeData();
		virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column) const;
		virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column);
		virtual Qt::ItemFlags flags();
        ElementCollectionItem *parent();
        int row() const;
		virtual QString name();
		virtual QString collectionName() const;
		virtual QString collectionPath() const {return QString();}

		virtual bool isDir() const;
		virtual bool isElement() const;
		virtual bool isValid() const;
		virtual QList <ElementCollectionItem *> items() const;
		QList<ElementCollectionItem *> elementsChild() const;
		QList<ElementCollectionItem *> directoriesChild() const;
		int indexOfChild(ElementCollectionItem *child) const;
		void setBackgroundColor(Qt::GlobalColor color, bool show);

	signals:
		void beginInsertRows(ElementCollectionItem *parent, int first, int last);
		void endInsertRows();
		void beginRemoveRows(ElementCollectionItem *parent, int first, int last);
		void endRemoveRows();

	protected:
        ElementCollectionItem *m_parent_item;
        QList <ElementCollectionItem *> m_child_items;
		QString m_name;
		Qt::GlobalColor m_bg_color;
		bool m_show_bg_color = false;
};

#endif // ELEMENTCOLLECTIONITEM_H
