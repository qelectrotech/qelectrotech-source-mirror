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
#ifndef CREATEDXF_H
#define CREATEDXF_H
#include <QString>
#include <QtCore>
#include <QtWidgets>

/**
	@brief The Createdxf class
	This class exports the project to DXF Format
*/
class Createdxf
{
	public:
		Createdxf();
		~Createdxf();
		static void dxfBegin (const QString&);
		static void dxfEnd(const QString&);
		// you can add more functions to create more drawings.
		static void drawCircle(
				const QString&,
				double,
				double,
				double,
				int);

		static void drawCircle(
				const QString& ,
				QPointF,
				double,
				int );

		static void drawArc(
				const QString&,
				double x,
				double y,
				double rad,
				double startAngle,
				double endAngle,
				int color);
		static void drawDonut(QString,double,double,double,int);

		static void drawArcEllipse (
				const QString &file_path,
				qreal x,
				qreal y,
				qreal w,
				qreal h,
				qreal startAngle,
				qreal spanAngle,
				qreal hotspot_x,
				qreal hotspot_y,
				qreal rotation_angle,
				const int &colorcode);

	static void drawArcEllipse(
		const QString &file_path,
		QRectF rect,
		qreal startAngle,
		qreal spanAngle,
		QPointF hotspot,
		qreal rotation_angle,
		const int &colorcode);

		static void drawEllipse (const QString &filepath,
					 const QRectF &rect,
					 const int &colorcode);

		static void drawRectangle(
				const QString &filepath,
				double,
				double,
				double,
				double,
				const int &colorcode);
		static void drawRectangle(
				const QString &filepath,
				const QRectF &rect,
				const int &colorcode);

		static void drawPolygon(
				const QString &filepath,
				const QPolygonF &poly,
				const int &colorcode);

		static void drawLine(
				const QString &filapath,
				double,
				double,
				double,
				double,
				const int &clorcode);
		static void drawLine(
				const QString &filepath,
				const QLineF &line,
				const int &colorcode);

		static void drawText(
				const QString&,
				const QString&,
				double,double,
				double,
				double,
		int,
		double xScale=1.0);
	static void drawText(
		const QString&,
		const QString&,
		QPointF,
		double,
		double,
		int,
		double xScale=1.0);
		static void drawTextAligned(
				const QString& fileName,
				const QString& text,
				double x,
				double y,
				double height,
				double rotation,
				double oblique,
				int hAlign,
				int vAlign,
				double xAlign,
				double xScale,
				int colour);

	static void drawPolyline(
		const QString &filepath,
		const QPolygonF &poly,
		const int &colorcode,
		bool preScaled = false);

		static int getcolorCode (
				const long red,
				const long green,
				const long blue);
		static long RGBcodeTable[];
	static int dxfColor(QColor color);
	static int dxfColor(QPen pen);

		static const double sheetWidth;
		static const double sheetHeight;
		static double		xScale;
		static double		yScale;
};

#endif // CREATEDXF_H
