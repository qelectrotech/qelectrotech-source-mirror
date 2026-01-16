/*
		Copyright 2006-2026 The QElectroTech Team
		This file is part of QElectroTech.

		QElectroTech is free software: you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation, either version 2 of the License, or
		(at your option) any later version.

		QElectroTech is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		GNU General Public License for more details.

		You should have received a copy of the GNU General Public License
		along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef USERPROPERTIES_H
#define USERPROPERTIES_H

#include "propertiesinterface.h"

/**
 * @brief The UserProperties class
 * This class store user properties.
 * User properties are properties set by user
 * who are don't know by QElectroTech.
 * An user property is defined by a name and a value
 * of arbitrarie type (QString, char*, int, double, bool, QUuid, QColor)
 */
class UserProperties : public PropertiesInterface
{
	public:
		UserProperties(const QString tag_name = "userProperties");

		virtual void toSettings (QSettings &,
								 const QString = QString()) const override {}

		virtual void fromSettings (const QSettings &,
								   const QString = QString()) override {}

		virtual QDomElement toXml (QDomDocument &xml_document) const override;
		virtual bool fromXml (const QDomElement &xml_element) override;

		void deleteUserProperties();
		int userPropertiesCount() const;
		void setUserProperty(const QString& key, const QVariant& value);
		bool existUserProperty(const QString& key) const;
		QVariant userPropertyValue(const QString& key);

		void setTagName(const QString& tag_name);
		QString tagName() const;

	private:
		QHash<QString, QVariant> m_properties;
		QString m_tag_name;
};

#endif // USERPROPERTIES_H
