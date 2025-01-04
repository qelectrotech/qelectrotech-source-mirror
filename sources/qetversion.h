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
#ifndef QETVERSION_H
#define QETVERSION_H

#include <QVersionNumber>
#include <QDomElement>

namespace QetVersion
{
	enum VersionType {
		dev = 0,
		alpha1 = 1,
		alpha2 = 2,
		alpha3 = 3,
		stable = 4
	};

	QVersionNumber currentVersion();
	QString displayedVersion();

	QVersionNumber versionZeroDotSix();

	void toXmlAttribute(QDomElement &xml);
	QVersionNumber fromXmlAttribute(const QDomElement &xml);
}

#endif // QETVERSION_H
