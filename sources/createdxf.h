/*
	Copyright 2006-2014 The QElectroTech Team
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
#include <QtGui>


/* This class exports the project to DXF Format */
class Createdxf
{    
    public:
    Createdxf();
    ~Createdxf();
	static void dxfBegin (QString);
	static void dxfEnd(QString);
    // you can add more functions to create more drawings.
	static void drawCircle(QString,double,double,double,int);
	static void drawArc(QString,double x,double y,double rad,double startAngle,double endAngle,int color);
	static void drawDonut(QString,double,double,double,int);
	static void drawRectangle(QString,double,double,double,double,int);
	static void drawLine(QString,double,double,double,double,int);
	static void drawText(QString,QString,double,double,double,double,int);
	static void drawTextAligned(QString fileName, QString text,double x, double y, double height, double rotation, double oblique,int hAlign, int vAlign, double xAlign, int colour,
					 bool leftAlign = false, float scale = 0);

	static const double sheetWidth;
	static const double sheetHeight;
	static double		xScale;
	static double		yScale;
};

#endif // CREATEDXF_H
