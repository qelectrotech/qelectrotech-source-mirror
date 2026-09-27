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

class Diagram;
class ExportProperties;

/**
	Writes a folio as a DXF file. Used by the export dialog and by
	--export-dxf (discussion #1072), so both write the same file for the
	same options.
*/
namespace DxfExport
{
	QSize folioSize(Diagram *diagram, const ExportProperties &properties);
	void write(Diagram *diagram, int width, int height,
			   const QString &path, const ExportProperties &properties);
	QPointF rotation_transformed(qreal px, qreal py,
								 qreal origin_x, qreal origin_y, qreal angle);
}

#endif // DXFEXPORT_H
