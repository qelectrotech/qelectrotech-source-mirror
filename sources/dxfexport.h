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
#ifndef DXFEXPORT_H
#define DXFEXPORT_H

#include <QPointF>
#include <QSize>
#include <QString>
#include <QStringList>

class Diagram;
class ExportProperties;

/**
	Writes a folio as a DXF file. Used by the export dialog and by
	--export-dxf (discussion #1072), so both write the same file for the
	same options.
*/
namespace DxfExport
{
		/// The layers of an exported folio (discussion #1071). Fixed and
		/// untranslated, so a CAD user's layer filters and scripts work
		/// whatever language QElectroTech runs in; prefixed so they sort
		/// together and do not clash with the recipient's own layers.
	namespace Layer
	{
		inline const QString Border      = QStringLiteral("QET_BORDER");
		inline const QString TitleBlock  = QStringLiteral("QET_TITLEBLOCK");
		inline const QString Symbols     = QStringLiteral("QET_SYMBOLS");
		inline const QString SymbolTexts = QStringLiteral("QET_SYMBOL_TEXTS");
		inline const QString Terminals   = QStringLiteral("QET_TERMINALS");
		inline const QString Wires       = QStringLiteral("QET_WIRES");
		inline const QString WireNumbers = QStringLiteral("QET_WIRE_NUMBERS");
		inline const QString Junctions   = QStringLiteral("QET_JUNCTIONS");
		inline const QString Texts       = QStringLiteral("QET_TEXTS");
		inline const QString Xrefs       = QStringLiteral("QET_XREFS");
		inline const QString Shapes      = QStringLiteral("QET_SHAPES");
		inline const QString Tables      = QStringLiteral("QET_TABLES");
		inline const QString Images      = QStringLiteral("QET_IMAGES");

		inline QStringList all()
		{
			return {Border, TitleBlock, Symbols, SymbolTexts, Terminals,
					Wires, WireNumbers, Junctions, Texts, Xrefs, Shapes,
					Tables, Images};
		}
	}

	QSize folioSize(Diagram *diagram, const ExportProperties &properties);
	void write(Diagram *diagram, int width, int height,
			   const QString &path, const ExportProperties &properties);
	QPointF rotation_transformed(qreal px, qreal py,
								 qreal origin_x, qreal origin_y, qreal angle);
}

#endif // DXFEXPORT_H
