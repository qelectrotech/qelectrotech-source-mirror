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
#ifndef FILEELEMENTCOLLECTIONITEM2_H
#define FILEELEMENTCOLLECTIONITEM2_H

#include "elementcollectionitem.h"

/**
 * @brief The FileElementCollectionItem class
 * This class specialise ElementCollectionItem for manage a collection in
 * a file system. They represente a directory or an element.
 */
class FileElementCollectionItem : public ElementCollectionItem
{
	public:
		FileElementCollectionItem();

		enum { Type = UserType+2 };
		virtual int type() const { return Type;}

		bool setRootPath(QString path, bool set_data = true, bool hide_element = false);
		QString fileSystemPath() const;
		QString dirPath() const;

		virtual bool isDir() const;
		virtual bool isElement() const;
		virtual QString localName();
		virtual QString name() const;
		virtual QString collectionPath() const;
		virtual bool isCollectionRoot() const;
		bool isCommonCollection() const;
		virtual void addChildAtPath(const QString &collection_name);

		void setUpData();
		void setUpIcon();

		void hire();


	private:
		void setPathName(QString path_name, bool set_data = true, bool hide_element = false);
		void populate(bool set_data = true, bool hide_element = false);

	private:
		QString m_path;
};

#endif // FILEELEMENTCOLLECTIONITEM2_H
