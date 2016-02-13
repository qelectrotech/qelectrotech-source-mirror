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
#ifndef FILEELEMENTCOLLECTIONITEM_H
#define FILEELEMENTCOLLECTIONITEM_H

#include "elementcollectionitem.h"
#include "elementlocation.h"
#include <QString>
#include <QDir>

/**
 * @brief The FileElementCollectionItem class
 * This class specialise ElementCollectionItem for manage a collection in
 * a file system. They represente a directory or an element.
 */
class FileElementCollectionItem : public ElementCollectionItem
{
    public:
		FileElementCollectionItem(ElementCollectionItem *parent = nullptr);
        ~FileElementCollectionItem();

        enum {Type = UserType + 1};
        virtual int type() const {return Type;}

        bool setRootPath(QString path);
		QString fileSystemPath() const;
		QString dirPath() const;

		QString collectionPath() const;
		QString collectionName() const;

		virtual QVariant data(int column, int role);
		virtual QMimeData *mimeData();
		virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column) const;
		virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column);
		virtual Qt::ItemFlags flags();

		virtual bool isDir() const;
		virtual bool isElement() const;
		virtual bool isCollectionRoot() const;
		bool isCommonCollection() const;
		virtual bool isValid() const;
		virtual QString name();

		virtual bool canRemoveContent();
		virtual bool removeContent();
		virtual void insertNewItem(const QString &collection_name);

    private:
		void setPathName(QString path_name);
		void populate();
//		bool handleElementDrop (const QMimeData *data);
//		bool handleDirectoryDrop (const QMimeData *data);
//		bool createSubDir (QDir dir_to_copy, QDir destination);

    private:
		QString m_path;
		ElementLocation m_location;
		QIcon m_icon;
};

#endif // FILEELEMENTCOLLECTIONITEM_H
