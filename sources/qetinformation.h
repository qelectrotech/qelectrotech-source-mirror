/*
	Copyright 2006-2020 The QElectroTech Team
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
#ifndef QETINFORMATION_H
#define QETINFORMATION_H

#include <QStringList>
#include <QHash>

/**
 * Inside this namespace you will fin all information used in QElectrotech for
 * element, conductor and diagram.
 * Each information have 3 values :
 * #1 the info key = the key of an information as a QString used in the code (exemple : label)
 * #2 the info key to variable = the key in form of a variable.
 * This is used by the user to replace a variable by the string of this variable (exemple : %{label})
 * #3 the info key translated to the current local (exemple label in dutch = Betriebsmittelkennzeichen)
 */
namespace QETInformation
{
	QStringList titleblockInfoKeys();
	QString titleblockInfoKeysToVar(const QString &info);
	QHash <QString, QString> titleblockTranslatedKeyHashVar();
	QStringList folioReportInfoKeys();
	QHash <QString, QString> folioReportInfoKeyToVar();
	QStringList conductorInfoKeys();

	QString translatedInfoKey(const QString &info);
}

#endif // QETINFORMATION_H
