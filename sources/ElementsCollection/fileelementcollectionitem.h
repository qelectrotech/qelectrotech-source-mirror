/*
		Copyright 2006-2017 The QElectroTech Team
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
		int type() const override { return Type;}

		bool setRootPath(const QString& path, bool set_data = true, bool hide_element = false);
		QString fileSystemPath() const;
		QString dirPath() const;

		bool isDir() const override;
		bool isElement() const override;
		QString localName() override;
		QString name() const override;
		QString collectionPath() const override;
		bool isCollectionRoot() const override;
		bool isCommonCollection() const;
		bool isCustomCollection() const;
		void addChildAtPath(const QString &collection_name) override;

		void setUpData() override;
		void setUpIcon() override;

		void hire();


	private:
		void setPathName(const QString& path_name, bool set_data = true, bool hide_element = false);
		void populate(bool set_data = true, bool hide_element = false);

	private:
		QString m_path;
};

#endif // FILEELEMENTCOLLECTIONITEM2_H
