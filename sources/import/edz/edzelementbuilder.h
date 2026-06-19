/*
	Copyright 2006 The QElectroTech Team
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
#ifndef EDZELEMENTBUILDER_H
#define EDZELEMENTBUILDER_H

#include <QDomDocument>

class EdzPart;

/**
	@brief Builds a QElectroTech element (.elmt) from an EdzPart.

	Generates a generic symbol: a body rectangle with one west-facing terminal
	per pin (stacked on the left edge, on the 10px wiring grid), per-pin labels,
	localized names and the element-information fields QET uses for the BOM.
	Faithful port of the edz2qet.py prototype's builder.
*/
class EdzElementBuilder
{
	public:
		static QDomDocument build(const EdzPart &part);
		static QString toElmtString(const QDomDocument &doc);
};

#endif // EDZELEMENTBUILDER_H
