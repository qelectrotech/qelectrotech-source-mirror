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
#include "createdxf.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QString>


const double Createdxf::sheetWidth = 4000;
const double Createdxf::sheetHeight = 2700;

double Createdxf::xScale = 1;
double Createdxf::yScale = 1;

Createdxf::Createdxf()
{
}


Createdxf::~Createdxf()
{
}

/* Header section of every DXF file.*/
void  Createdxf::dxfBegin (QString fileName)
{

    // Creation of an output stream object in text mode.
    // Header section of every dxf file.
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::WriteOnly)) {
            // error message
            QMessageBox errorFileOpen;
             errorFileOpen.setIcon(QMessageBox::Warning);
             errorFileOpen.setText("Error: "+fileName+" Could Not be Opened.");
             errorFileOpen.setInformativeText("Close all Files and Try Again.");
             errorFileOpen.exec();
             exit(0);
        } else {
            QTextStream To_Dxf(&file);
            To_Dxf << 999           << "\r\n";
            To_Dxf << "QET"         << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "SECTION"     << "\r\n";
            To_Dxf << 2             << "\r\n";
            To_Dxf << "HEADER"      << "\r\n";
            To_Dxf << 9             << "\r\n";
            To_Dxf << "$ACADVER"    << "\r\n";
            To_Dxf << 1             << "\r\n";
            To_Dxf << "AC1006"      << "\r\n";
            To_Dxf << 9             << "\r\n";
            To_Dxf << "$INSBASE"    << "\r\n";
            To_Dxf << 10            << "\r\n";
            To_Dxf << "0.0"         << "\r\n";
            To_Dxf << 20            << "\r\n";
            To_Dxf << "0.0"         << "\r\n";
            To_Dxf << 30            << "\r\n";
            To_Dxf << "0.0"         << "\r\n";
            To_Dxf << 9             << "\r\n";

            To_Dxf << "$EXTMIN"     << "\r\n";
            To_Dxf << 10            << "\r\n";
            To_Dxf << "0.0"         << "\r\n";
            To_Dxf << 20            << "\r\n";
            To_Dxf << "0.0"         << "\r\n";
            To_Dxf << 9             << "\r\n";
            To_Dxf << "$EXTMAX"     << "\r\n";
            To_Dxf << 10            << "\r\n";
            To_Dxf << "4000.0"      << "\r\n";
            To_Dxf << 20            << "\r\n";
            To_Dxf << "4000.0"      << "\r\n";

            To_Dxf << 9             << "\r\n";
            To_Dxf << "$LIMMIN"     << "\r\n";
            To_Dxf << 10            << "\r\n";
            To_Dxf << "0.0"         << "\r\n";
            To_Dxf << 20            << "\r\n";
            To_Dxf << "0.0"         << "\r\n";
            To_Dxf << 9             << "\r\n";
            To_Dxf << "$LIMMAX"     << "\r\n";
            To_Dxf << 10            << "\r\n";
            To_Dxf << "4000.0"      << "\r\n";
            To_Dxf << 20            << "\r\n";
            To_Dxf << "4000.0"      << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "ENDSEC"      << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "SECTION"     << "\r\n";
            To_Dxf << 2             << "\r\n";
            To_Dxf << "TABLES"      << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "TABLE"       << "\r\n";
            To_Dxf << 2             << "\r\n";

            To_Dxf << "VPORT"       << "\r\n";
            To_Dxf << 70            << "\r\n";
            To_Dxf << 1             << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "VPORT"       << "\r\n";
            To_Dxf << 2             << "\r\n";
            To_Dxf << "*ACTIVE"     << "\r\n";
            To_Dxf << 70            << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << 10            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 20            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 11            << "\r\n";
            To_Dxf << 1.0           << "\r\n";
            To_Dxf << 21            << "\r\n";
            To_Dxf << 1.0           << "\r\n";
            To_Dxf << 12            << "\r\n";
            To_Dxf << 2000          << "\r\n";
            To_Dxf << 22            << "\r\n";
            To_Dxf << 1350          << "\r\n";
            To_Dxf << 13            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 23            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 14            << "\r\n";
            To_Dxf << 1.0           << "\r\n";
            To_Dxf << 24            << "\r\n";
            To_Dxf << 1.0           << "\r\n";
            To_Dxf << 15            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 25            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 16            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 26            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 36            << "\r\n";
            To_Dxf << 1.0           << "\r\n";
            To_Dxf << 17            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 27            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 37            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 40            << "\r\n";
            To_Dxf << 2732.5        << "\r\n";
            To_Dxf << 41            << "\r\n";
            To_Dxf << 2.558         << "\r\n";
            To_Dxf << 42            << "\r\n";
            To_Dxf << 50.0          << "\r\n";
            To_Dxf << 43            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 44            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 50            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 51            << "\r\n";
            To_Dxf << 0.0           << "\r\n";
            To_Dxf << 71            << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << 72            << "\r\n";
            To_Dxf << 100           << "\r\n";
            To_Dxf << 73            << "\r\n";
            To_Dxf << 1             << "\r\n";
            To_Dxf << 74            << "\r\n";
            To_Dxf << 1             << "\r\n";
            To_Dxf << 75            << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << 76            << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << 77            << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << 78            << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "ENDTAB"      << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "TABLE"       << "\r\n";
            To_Dxf << 2             << "\r\n";

            To_Dxf << "LTYPE"       << "\r\n";
            To_Dxf << 70            << "\r\n";
            To_Dxf << 1             << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "LTYPE"       << "\r\n";
            To_Dxf << 2             << "\r\n";
            To_Dxf << "CONTINUOUS"  << "\r\n";
            To_Dxf << 70            << "\r\n";
            To_Dxf << 64            << "\r\n";
            To_Dxf << 3             << "\r\n";
            To_Dxf << "Solid Line"  << "\r\n";
            To_Dxf << 72            << "\r\n";
            To_Dxf << 65            << "\r\n";
            To_Dxf << 73            << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << 40            << "\r\n";
            To_Dxf << 0.00          << "\r\n";
            To_Dxf << 0             << "\r\n";

            To_Dxf << "ENDTAB"      << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "ENDSEC"      << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "SECTION"     << "\r\n";
            To_Dxf << 2             << "\r\n";
            To_Dxf << "BLOCKS"      << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "ENDSEC"      << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "SECTION"     << "\r\n";
            To_Dxf << 2             << "\r\n";
            To_Dxf << "ENTITIES"    << "\r\n";
            file.close();
        }
    }
}

/* End Section of every DXF File*/
void  Createdxf::dxfEnd (QString fileName)
{
    // Creation of an output stream object in text mode.
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::Append)) {
            // error message
            QMessageBox errorFileOpen;
             errorFileOpen.setText("Error: File "+fileName+" was not written correctly.");
             errorFileOpen.setInformativeText("Close all Files and Re-Run");
             errorFileOpen.exec();
        } else {
            QTextStream To_Dxf(&file);
            To_Dxf << 0             << "\r\n";
            To_Dxf << "ENDSEC"      << "\r\n";
            To_Dxf << 0             << "\r\n";
            To_Dxf << "EOF";
            file.close();
        }
    }
}


/* draw circle in dxf format*/
void Createdxf::drawCircle (QString fileName, double radius, double x, double y, int colour)
{
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::Append)) {
            // error message
            QMessageBox errorFileOpen;
            errorFileOpen.setText("Error: File "+fileName+" was not written correctly.");
            errorFileOpen.setInformativeText("Close all Files and Re-Run");
            errorFileOpen.exec();
        } else {
            QTextStream To_Dxf(&file);
            // Draw the circle
            To_Dxf << 0         << "\r\n";
            To_Dxf << "CIRCLE"  << "\r\n";
            To_Dxf << 8         << "\r\n";
            To_Dxf << 0         << "\r\n";    // Layer number (default layer in autocad)
            To_Dxf << 62        << "\r\n";
            To_Dxf << colour    << "\r\n";    // Colour Code
            To_Dxf << 10        << "\r\n";    // XYZ is the Center point of circle
            To_Dxf << x         << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 20        << "\r\n";
            To_Dxf << y         << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 30        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 40        << "\r\n";
            To_Dxf << radius    << "\r\n";    // radius of circle
            file.close();
        }
    }
}


/* draw line in DXF Format*/
void Createdxf::drawLine (QString fileName, double x1, double y1, double x2, double y2, int colour)
{
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::Append)) {
            // error message
            QMessageBox errorFileOpen;
            errorFileOpen.setText("Error: File "+fileName+" was not written correctly.");
            errorFileOpen.setInformativeText("Close all Files and Re-Run");
            errorFileOpen.exec();
        } else {
            QTextStream To_Dxf(&file);
            // Draw the Line
            To_Dxf << 0         << "\r\n";
            To_Dxf << "LINE"    << "\r\n";
            To_Dxf << 8         << "\r\n";
            To_Dxf << 0         << "\r\n";    // Layer number (default layer in autocad)
            To_Dxf << 62        << "\r\n";
            To_Dxf << colour    << "\r\n";    // Colour Code
            To_Dxf << 10        << "\r\n";
            To_Dxf << x1        << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 20        << "\r\n";
            To_Dxf << y1        << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 30        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 11        << "\r\n";
            To_Dxf << x2        << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 21        << "\r\n";
            To_Dxf << y2        << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 31        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            file.close();
        }
    }
}

/* draw rectangle in dxf format */
void Createdxf::drawRectangle (QString fileName, double x1, double y1, double width, double height, int colour)
{
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::Append)) {
            // error message
            QMessageBox errorFileOpen;
            errorFileOpen.setText("Error: File "+fileName+" was not written correctly.");
            errorFileOpen.setInformativeText("Close all Files and Re-Run");
            errorFileOpen.exec();
        } else {
            QTextStream To_Dxf(&file);
            // Draw the Rectangle
            To_Dxf << 0         << "\r\n";
            To_Dxf << "LINE"    << "\r\n";
            To_Dxf << 8         << "\r\n";
            To_Dxf << 0         << "\r\n";    // Layer number (default layer in autocad)
            To_Dxf << 62        << "\r\n";
            To_Dxf << colour    << "\r\n";    // Colour Code
            To_Dxf << 10        << "\r\n";
            To_Dxf << x1        << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 20        << "\r\n";
            To_Dxf << y1        << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 30        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 11        << "\r\n";
            To_Dxf << x1+width  << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 21        << "\r\n";
            To_Dxf << y1        << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 31        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 0         << "\r\n";
            To_Dxf << "LINE"    << "\r\n";
            To_Dxf << 8         << "\r\n";
            To_Dxf << 0         << "\r\n";    // Layer number (default layer in autocad)
            To_Dxf << 62        << "\r\n";
            To_Dxf << colour    << "\r\n";    // Colour Code
            To_Dxf << 10        << "\r\n";
            To_Dxf << x1        << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 20        << "\r\n";
            To_Dxf << y1        << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 30        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 11        << "\r\n";
            To_Dxf << x1        << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 21        << "\r\n";
            To_Dxf << y1+height << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 31        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 0         << "\r\n";
            To_Dxf << "LINE"    << "\r\n";
            To_Dxf << 8         << "\r\n";
            To_Dxf << 0         << "\r\n";    // Layer number (default layer in autocad)
            To_Dxf << 62        << "\r\n";
            To_Dxf << colour    << "\r\n";    // Colour Code
            To_Dxf << 10        << "\r\n";
            To_Dxf << x1+width  << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 20        << "\r\n";
            To_Dxf << y1        << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 30        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 11        << "\r\n";
            To_Dxf << x1+width  << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 21        << "\r\n";
            To_Dxf << y1+height << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 31        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 0         << "\r\n";
            To_Dxf << "LINE"    << "\r\n";
            To_Dxf << 8         << "\r\n";
            To_Dxf << 0         << "\r\n";    // Layer number (default layer in autocad)
            To_Dxf << 62        << "\r\n";
            To_Dxf << colour    << "\r\n";    // Colour Code
            To_Dxf << 10        << "\r\n";
            To_Dxf << x1        << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 20        << "\r\n";
            To_Dxf << y1+height << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 30        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 11        << "\r\n";
            To_Dxf << x1+width  << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 21        << "\r\n";
            To_Dxf << y1+height << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 31        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            file.close();
        }
    }
}

/* draw arc in dx format */
void Createdxf::drawArc(QString fileName,double x,double y,double rad,double startAngle,double endAngle,int color)
{
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::Append)) {
            // error message
            QMessageBox errorFileOpen;
            errorFileOpen.setText("Error: File "+fileName+" was not written correctly.");
            errorFileOpen.setInformativeText("Close all Files and Re-Run");
            errorFileOpen.exec();
        } else {
            QTextStream To_Dxf(&file);
            // Draw the arc
            To_Dxf << 0         << "\r\n";
            To_Dxf << "ARC"     << "\r\n";
            To_Dxf << 8         << "\r\n";
            To_Dxf << 0         << "\r\n";    // Layer number (default layer in autocad)
            To_Dxf << 62        << "\r\n";
            To_Dxf << color     << "\r\n";    // Colour Code
            To_Dxf << 10        << "\r\n";    // XYZ is the Center point of circle
            To_Dxf << x         << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 20        << "\r\n";
            To_Dxf << y         << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 30        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 40        << "\r\n";
            To_Dxf << rad       << "\r\n";    // radius of arc
            To_Dxf << 50        << "\r\n";
            To_Dxf << startAngle<< "\r\n";    // start angle
            To_Dxf << 51        << "\r\n";
            To_Dxf << endAngle  << "\r\n";    // end angle
            file.close();
        }
    }
}

/* draw simple text in dxf format without any alignment specified */
void Createdxf::drawText(QString fileName, QString text,double x, double y, double height, double rotation, int colour)
{
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::Append)) {
            // error message
            QMessageBox errorFileOpen;
            errorFileOpen.setText("Error: File "+fileName+" was not written correctly.");
            errorFileOpen.setInformativeText("Close all Files and Re-Run");
            errorFileOpen.exec();
        } else {
            QTextStream To_Dxf(&file);
            // Draw the circle
            To_Dxf << 0         << "\r\n";
            To_Dxf << "TEXT"    << "\r\n";
            To_Dxf << 8         << "\r\n";
            To_Dxf << 0         << "\r\n";    // Layer number (default layer in autocad)
            To_Dxf << 62        << "\r\n";
            To_Dxf << colour    << "\r\n";    // Colour Code
            To_Dxf << 10        << "\r\n";    // XYZ
            To_Dxf << x         << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 20        << "\r\n";
            To_Dxf << y         << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 30        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 40        << "\r\n";
            To_Dxf << height    << "\r\n";    // Text Height
            To_Dxf << 1         << "\r\n";
            To_Dxf << text      << "\r\n";    // Text Value
            To_Dxf << 50        << "\r\n";
            To_Dxf << rotation  << "\r\n";    // Text Rotation
            file.close();
        }
    }
}

/* draw aligned text in DXF Format */
// leftAlign flag added. If the alignment requested is 'fit to width' and the text length is very small,
// then the text is either centered or left-aligned, depnding on the value of leftAlign.
void Createdxf::drawTextAligned(QString fileName, QString text,double x, double y, double height, double rotation, double oblique,int hAlign, int vAlign, double xAlign,int colour,
							bool leftAlign, float scale)
{
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::Append)) {
            // error message
            QMessageBox errorFileOpen;
            errorFileOpen.setText("Error: File "+fileName+" was not written correctly.");
            errorFileOpen.setInformativeText("Close all Files and Re-Run");
            errorFileOpen.exec();
        } else {
            QTextStream To_Dxf(&file);
            // Draw the circle
            To_Dxf << 0         << "\r\n";
            To_Dxf << "TEXT"    << "\r\n";
            To_Dxf << 8         << "\r\n";
            To_Dxf << 0         << "\r\n";    // Layer number (default layer in autocad)
            To_Dxf << 62        << "\r\n";
            To_Dxf << colour    << "\r\n";    // Colour Code
            To_Dxf << 10        << "\r\n";    // XYZ
            To_Dxf << x         << "\r\n";    // X in UCS (User Coordinate System)coordinates
            To_Dxf << 20        << "\r\n";
            To_Dxf << y         << "\r\n";    // Y in UCS (User Coordinate System)coordinates
            To_Dxf << 30        << "\r\n";
            To_Dxf << 0.0       << "\r\n";    // Z in UCS (User Coordinate System)coordinates
            To_Dxf << 40        << "\r\n";
            To_Dxf << height    << "\r\n";    // Text Height
            To_Dxf << 1         << "\r\n";
            To_Dxf << text      << "\r\n";    // Text Value
            To_Dxf << 50        << "\r\n";
            To_Dxf << rotation  << "\r\n";    // Text Rotation
			// If "Fit to width", then check if width of text < width specified then change it "center align or left align"
			if (hAlign == 5) {
				int xDiff = xAlign - x;
				if (text.length() < xDiff/height && !leftAlign) {
					hAlign = 1;
					xAlign = (x+xAlign) / 2;
				} else if (text.length() < xDiff/height && leftAlign) {
					file.close();
					return;
				}
			}

            To_Dxf << 51        << "\r\n";
            To_Dxf << oblique   << "\r\n";    // Text Obliqueness
            To_Dxf << 72        << "\r\n";            
            To_Dxf << hAlign    << "\r\n";    // Text Horizontal Alignment
            To_Dxf << 73        << "\r\n";
            To_Dxf << vAlign    << "\r\n";    // Text Vertical Alignment

            if ((hAlign) || (vAlign)) { // Enter Second Point
                To_Dxf << 11       << "\r\n"; // XYZ
                To_Dxf << xAlign   << "\r\n"; // X in UCS (User Coordinate System)coordinates
                To_Dxf << 21       << "\r\n";
                To_Dxf << y        << "\r\n"; // Y in UCS (User Coordinate System)coordinates
                To_Dxf << 31       << "\r\n";
                To_Dxf << 0.0      << "\r\n"; // Z in UCS (User Coordinate System)coordinates
            }
            file.close();
        }
    }
}
