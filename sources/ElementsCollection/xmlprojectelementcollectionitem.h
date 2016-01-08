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
#ifndef XMLPROJECTELEMENTCOLLECTIONITEM_H
#define XMLPROJECTELEMENTCOLLECTIONITEM_H

#include "elementcollectionitem.h"

#include <QDomElement>
#include <QIcon>

class QETProject;

/**
 * @brief The XmlProjectElementCollectionItem class
 * This class specialise ElementCollectionItem for manage an xml collection embedded in a project.
 */
class XmlProjectElementCollectionItem : public ElementCollectionItem
{
    public:
        XmlProjectElementCollectionItem(QETProject *project, ElementCollectionItem *parent = nullptr);
	private:
		XmlProjectElementCollectionItem (QETProject *project, const QDomElement &dom_element, ElementCollectionItem *parent = nullptr);
	public:
        ~XmlProjectElementCollectionItem();

        enum {Type = UserType + 2};
        virtual int type() const {return Type;}

        virtual QVariant data(int column, int role);
		virtual QMimeData *mimeData();
		virtual Qt::ItemFlags flags();

        virtual bool isCollectionRoot() const;
        virtual QString name();
        virtual bool isValid() const;
		QETProject *project() const;
		virtual bool isDir() const;
		virtual bool isElement() const;
		QString collectionPath() const;
		QString embeddedPath() const;
		virtual QString collectionName() const;

		XmlProjectElementCollectionItem *lastItemForPath(const QString &path, QString &next_item);
		int rowForInsertItem(const QString &collection_name);
		void insertNewItem(const QString &collection_name);


	private:
		void populate();

	private:
		QETProject *m_project;
		QDomElement m_dom_element;
		QIcon m_icon;
};

#endif // XMLPROJECTELEMENTCOLLECTIONITEM_H
