/*
	Copyright 2006-2026 The QElectroTech Team
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
#ifndef EPLANELEMENTBUILDER_H
#define EPLANELEMENTBUILDER_H

#include "eplandxfreader.h"

#include <QDomDocument>
#include <QMap>
#include <QStringList>

/**
	@brief The drawing of one symbol, in EPLAN drawing coordinates (mm, y up).
*/
struct EplanSymbolData
{
	QVector<QLineF> lines;
	QVector<QPair<QPointF, double>> circles;
	QVector<EplanDxfText> texts;
	QVector<EplanDxfPin> pins;       ///< sorted top-left to bottom-right; may be empty
	QVector<EplanDxfField> fields;   ///< empty: one function field per terminal
	QVector<QRectF> filled;          ///< solid black areas (transformer windings)
	bool pin_fields = true;          ///< no fields: a function field per terminal
	double scale = 2.5;              ///< px per mm: EPLAN 8 mm grid -> 20 px; 2.0 for a 5 mm grid (WSCAD)
	QString source;                  ///< note for the element, empty: from DXF data
};

/**
	@brief Builds a QElectroTech element (.elmt) from an EPLAN symbol drawing.

	The geometry is kept (8 mm EPLAN grid -> 20 px), terminals sit on the
	10 px wiring grid and face the nearest side of the device frame. Every
	empty EPLAN field (PLC address, symbolic address, function text) becomes
	a user text of its own that can be filled in per placement; symbols
	without such fields get one function text per terminal.
*/
class EplanElementBuilder
{
	public:
		/// @param info element information in display order (name, value)
		static QDomDocument build(const EplanSymbolData &data,
					  const QMap<QString, QString> &names,
					  const QVector<QPair<QString, QString>> &info);
		/// the names build() gives the terminals of @p pins, in their order:
		/// repeated names numbered "1.1", "1.2"
		static QStringList terminalNames(const QVector<EplanDxfPin> &pins);
};

#endif // EPLANELEMENTBUILDER_H
