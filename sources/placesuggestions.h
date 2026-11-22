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
#ifndef PLACESUGGESTIONS_H
#define PLACESUGGESTIONS_H

#include <QString>
#include <QStringList>

class QETProject;
class QLineEdit;

/**
	@brief The PlaceSuggestions namespace
	The plants and locations a project already has, offered as a drop-down
	list while a plant or a location is typed. They are the project's places
	as its database views work them out, so a plant that only a folio or a
	structure box holds is in the list too. A list only suggests: any other
	name can still be typed.
*/
namespace PlaceSuggestions
{
	QStringList plants(QETProject *project);
	QStringList locations(QETProject *project, const QString &plant = QString());
	QStringList merged(const QStringList &first, const QStringList &second);

	void setSuggestions(QLineEdit *edit, const QStringList &suggestions);
	void attach(QLineEdit *plant_edit, QLineEdit *location_edit, QETProject *project);
}

#endif // PLACESUGGESTIONS_H
