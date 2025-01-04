/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "qetversion.h"

namespace QetVersion
{
	QetVersion::VersionType VERSION_TYPE = QetVersion::dev;

	QVersionNumber currentVersion()
	{
		return QVersionNumber{ 0, 100, 0 };
	}

	QString displayedVersion()
	{
		auto str = currentVersion().toString();

		switch (VERSION_TYPE) {
			case dev:
				str.append(QStringLiteral("-dev"));
				break;
			case alpha1:
				str.append(QStringLiteral("-alpha 1 "));
				break;
			case alpha2:
				str.append(QStringLiteral("-alpha 2 "));
				break;
			case alpha3:
				str.append(QStringLiteral("-alpha 3 "));
				break;
			default:
				break;
		}

		return str;
	}

	/**
	 * @brief toXmlAttribute
	 * Write @a version as an attribute of @a xml
	 * @param xml
	 * @param version
	 */
	void toXmlAttribute(QDomElement &xml)
	{
		xml.setAttribute(QStringLiteral("version"), currentVersion().toString());
	}

	/**
	 * @brief fromXmlAttribute
	 * Read and return a QVersionNumber
	 * stored as attribute of @a xml
	 * @param xml
	 * @return the version stored in @a xml.
	 * Note that the returned version can be null
	 */
	QVersionNumber fromXmlAttribute(const QDomElement &xml)
	{
		if (xml.hasAttribute(QStringLiteral("version"))) {
			return QVersionNumber::fromString(xml.attribute(QStringLiteral("version")));
		} else {
			return QVersionNumber{};
		}
	}

	QVersionNumber versionZeroDotSix()
	{
		return QVersionNumber{ 0, 60 };
	}
}
