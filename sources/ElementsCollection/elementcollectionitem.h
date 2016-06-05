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
#ifndef ELEMENTCOLLECTIONITEM2_H
#define ELEMENTCOLLECTIONITEM2_H

#include <QStandardItem>

/**
 * @brief The ElementCollectionItem class
 * This class represent a item (a directory or an element) in a element collection.
 * This class must be herited for specialisation.
 * This item is used by ElementsCollectionModel for manage the elements collection
 */
class ElementCollectionItem : public QStandardItem
{
	public:
		ElementCollectionItem();

		enum {Type = UserType+1};
		virtual int type() const { return Type; }

		virtual bool isDir() const = 0;
		virtual bool isElement() const = 0;
		virtual QString localName() = 0;
		virtual QString name() const = 0;
		virtual QString collectionPath() const = 0;
		virtual bool isCollectionRoot() const = 0;
		virtual void addChildAtPath(const QString &collection_name) = 0;
		virtual void setUpData() = 0;
		virtual void setUpIcon() = 0;
		virtual void clearData();

		ElementCollectionItem *lastItemForPath(const QString &path, QString &no_found_path);
		ElementCollectionItem *childWithCollectionName(QString name) const;
		QList<QStandardItem *> directChilds() const;
		int rowForInsertItem(const QString &name);
		ElementCollectionItem *itemAtPath(const QString &path);

		QList<ElementCollectionItem *> elementsDirectChild() const;
		QList<ElementCollectionItem *> directoriesDirectChild() const;
		QList<ElementCollectionItem *> items() const;
};

#endif // ELEMENTCOLLECTIONITEM2_H
