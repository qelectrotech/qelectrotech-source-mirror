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
#ifndef XMLPROJECTELEMENTCOLLECTIONITEM2_H
#define XMLPROJECTELEMENTCOLLECTIONITEM2_H

#include "elementcollectionitem.h"
#include <QDomElement>

class QETProject;

/**
 * @brief The XmlProjectElementCollectionItem class
 * This class specialise ElementCollectionItem for manage an xml collection embedded in a project.
 */
class XmlProjectElementCollectionItem : public ElementCollectionItem
{
	public:
		XmlProjectElementCollectionItem();

		enum {Type = UserType+3};
		int type() const override { return Type; }

		bool isDir() const override;
		bool isElement() const override;
		QString localName() override;
		QString name() const override;
		QString collectionPath() const override;
		virtual QString embeddedPath() const;
		bool isCollectionRoot() const override;
		void addChildAtPath(const QString &collection_name) override;
		QETProject * project() const;

		void setProject (QETProject *project, bool set_data = true, bool hide_element = false);
		void setUpData() override;
		void setUpIcon() override;

	private:
		void populate(bool set_data = true, bool hide_element = false);
		void setXmlElement(QDomElement element, QETProject *project, bool set_data = true, bool hide_element = false);

	private:
		QETProject *m_project = nullptr;
		QDomElement m_dom_element;
};

#endif // XMLPROJECTELEMENTCOLLECTIONITEM2_H
