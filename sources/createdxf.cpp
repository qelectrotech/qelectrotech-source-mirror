/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "exportdialog.h"


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
void  Createdxf::dxfBegin (const QString& fileName)
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
void  Createdxf::dxfEnd (const QString& fileName)
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
void Createdxf::drawCircle (const QString& fileName, double radius, double x, double y, int colour)
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
void Createdxf::drawLine (const QString &fileName, double x1, double y1, double x2, double y2,const int &colour)
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

long Createdxf::RGBcodeTable[255]{
    0x000000,    0xff0000,    0xffff00,    0x00ff00,    0x00ffff,
    0x0000ff,    0xff00ff,    0xffffff,    0x414141,    0x808080,
    0xff0000,    0xffaaaa,    0xbd0000,    0xbd7e7e,    0x810000,
    0x815656,    0x680000,    0x684545,    0x4f0000,    0x4f3535,
    0xff3f00,    0xffbfaa,    0xbd2e00,    0xbd8d7e,    0x811f00,
    0x816056,    0x681900,    0x684e45,    0x4f1300,    0x4f3b35,
    0xff7f00,    0xffd4aa,    0xbd5e00,    0xbd9d7e,    0x814000,
    0x816b56,    0x683400,    0x685645,    0x4f2700,    0x4f4235,
    0xffbf00,    0xffeaaa,    0xbd8d00,    0xbdad7e,    0x816000,
    0x817656,    0x684e00,    0x685f45,    0x4f3b00,    0x4f4935,
    0xffff00,    0xffffaa,    0xbdbd00,    0xbdbd7e,    0x818100,
    0x818156,    0x686800,    0x686845,    0x4f4f00,    0x4f4f35,
    0xbfff00,    0xeaffaa,    0x8dbd00,    0xadbd7e,    0x608100,
    0x768156,    0x4e6800,    0x5f6845,    0x3b4f00,    0x494f35,
    0x7fff00,    0xd4ffaa,    0x5ebd00,    0x9dbd7e,    0x408100,
    0x6b8156,    0x346800,    0x566845,    0x274f00,    0x424f35,
    0x3fff00,    0xbfffaa,    0x2ebd00,    0x8dbd7e,    0x1f8100,
    0x608156,    0x196800,    0x4e6845,    0x134f00,    0x3b4f35,
    0x00ff00,    0xaaffaa,    0x00bd00,    0x7ebd7e,    0x008100,
    0x568156,    0x006800,    0x456845,    0x004f00,    0x354f35,
    0x00ff3f,    0xaaffbf,    0x00bd2e,    0x7ebd8d,    0x00811f,
    0x568160,    0x006819,    0x45684e,    0x004f13,    0x354f3b,
    0x00ff7f,    0xaaffd4,    0x00bd5e,    0x7ebd9d,    0x008140,
    0x56816b,    0x006834,    0x456856,    0x004f27,    0x354f42,
    0x00ffbf,    0xaaffea,    0x00bd8d,    0x7ebdad,    0x008160,
    0x568176,    0x00684e,    0x45685f,    0x004f3b,    0x354f49,
    0x00ffff,    0xaaffff,    0x00bdbd,    0x7ebdbd,    0x008181,
    0x568181,    0x006868,    0x456868,    0x004f4f,    0x354f4f,
    0x00bfff,    0xaaeaff,    0x008dbd,    0x7eadbd,    0x006081,
    0x567681,    0x004e68,    0x455f68,    0x003b4f,    0x35494f,
    0x007fff,    0xaad4ff,    0x005ebd,    0x7e9dbd,    0x004081,
    0x566b81,    0x003468,    0x455668,    0x00274f,    0x35424f,
    0x003fff,    0xaabfff,    0x002ebd,    0x7e8dbd,    0x001f81,
    0x566081,    0x001968,    0x454e68,    0x00134f,    0x353b4f,
    0x0000ff,    0xaaaaff,    0x0000bd,    0x7e7ebd,    0x000081,
    0x565681,    0x000068,    0x454568,    0x00004f,    0x35354f,
    0x3f00ff,    0xbfaaff,    0x2e00bd,    0x8d7ebd,    0x1f0081,
    0x605681,    0x190068,    0x4e4568,    0x13004f,    0x3b354f,
    0x7f00ff,    0xd4aaff,    0x5e00bd,    0x9d7ebd,    0x400081,
    0x6b5681,    0x340068,    0x564568,    0x27004f,    0x42354f,
    0xbf00ff,    0xeeaaff,    0x8d00bd,    0xad7ebd,    0x600081,
    0x765681,    0x4e0068,    0x5f4568,    0x3b004f,    0x49354f,
    0xff00ff,    0xffaaff,    0xbd00bd,    0xbd7ebd,    0x810081,
    0x815681,    0x680068,    0x684568,    0x4f004f,    0x4f354f,
    0xff00bf,    0xffaaea,    0xbd008d,    0xbd7ead,    0x810060,
    0x815676,    0x68004e,    0x68455f,    0x4f003b,    0x4f3549,
    0xff007f,    0xffaad4,    0xbd005e,    0xbd7e9d,    0x810040,
    0x81566b,    0x680034,    0x684556,    0x4f0027,    0x4f3542,
    0xff003f,    0xffaabf,    0xbd002e,    0xbd7e8d,    0x81001f,
    0x815660,    0x680019,    0x68454e,    0x4f0013,    0x4f353b,
    0x333333,    0x505050,    0x696969,    0x828282,    0xbebebe
};

/**
 * @brief Createdxf::getcolorCode
 * This function returns the ACI color which is the "nearest" color to
 * the color defined by the red, green and blue (RGB) values passed
 * in argument.
 * @param red
 * @param green
 * @param blue
 */
int Createdxf::getcolorCode (const long red, const long green, const long blue)
{
   long acirgb, r,g,b;
   long mindst = 2147483647L;
   long dst = 0;
   int minndx = 0;
   for ( int i = 0; i < 254; i++ )
   {
      acirgb = RGBcodeTable[i];
      b = ( acirgb & 0xffL );
      g = ( acirgb & 0xff00L ) >> 8;
      r = acirgb >> 16;
      dst = abs ( r-red) + abs ( g -green) + abs (b-blue);
      if ( dst < mindst )
      {
         minndx = i;
         mindst = dst;
      }
   }
   return minndx;
}

/**
 * @brief Createdxf::drawLine
 * Conveniance function to draw line
 * @param filepath
 * @param line
 * @param colorcode
 */
void Createdxf::drawLine(const QString &filepath, const QLineF &line, const int &colorcode) {
	drawLine(filepath, line.p1().x() * xScale,
					   sheetHeight - (line.p1().y() * yScale),
					   line.p2().x() * xScale,
					   sheetHeight - (line.p2().y() * yScale),
					   colorcode);
}

void Createdxf::drawArcEllipse(const QString &file_path, qreal x, qreal y, qreal w, qreal h, qreal startAngle, qreal spanAngle, qreal hotspot_x, qreal hotspot_y, qreal rotation_angle, const int &colorcode) {
	// vector of parts of arc (stored as a pair of startAngle and spanAngle) for each quadrant.
	QVector< QPair<qreal,qreal> > arc_parts_vector;

	if (spanAngle > 0) {
		qreal start = startAngle;
		qreal span;
		int i;
		for ( i = startAngle; i < startAngle+spanAngle; i++ ) {
			int absolute_theta = (i > 0) ? i : -i;
			if (absolute_theta == 0 || absolute_theta == 90 ||
				absolute_theta == 180 || absolute_theta == 270 ||
				absolute_theta == 360) {
				span = i - start;
				QPair<qreal, qreal> newPart(start,span);
				arc_parts_vector.push_back(newPart);
				start = i;
			}
		}
		if (start != i) {
			span = i - start;
			QPair<qreal, qreal> newPart(start,span);
			arc_parts_vector.push_back(newPart);
		}
	} else {
		qreal start = startAngle;
		qreal span;
		int i;
		for ( i = startAngle; i > startAngle+spanAngle; i-- ) {
			int absolute_theta = (i > 0) ? i : -i;
			if (absolute_theta == 0 || absolute_theta == 90 ||
				absolute_theta == 180 || absolute_theta == 270 ||
				absolute_theta == 360) {
				span = i - start;
				QPair<qreal, qreal> newPart(start,span);
				arc_parts_vector.push_back(newPart);
				start = i;
			}
		}
		if (start != i) {
			span = i - start;
			QPair<qreal, qreal> newPart(start,span);
			arc_parts_vector.push_back(newPart);
		}
	}

	for (int i = 0; i < arc_parts_vector.size(); i++) {

		QPair<qreal,qreal> arc = arc_parts_vector[i];
		if (arc.second == 0)
			continue;
		qreal arc_startAngle = arc.first * 3.142/180;
		qreal arc_spanAngle = arc.second * 3.142/180;

		qreal a = w/2;
		qreal b = h/2;

		qreal x1 = x + w/2 + a*cos(arc_startAngle);
		qreal y1 = y - h/2 + b*sin(arc_startAngle);
		qreal x2 = x + w/2 + a*cos(arc_startAngle + arc_spanAngle);
		qreal y2 = y - h/2 + b*sin(arc_startAngle + arc_spanAngle);


		qreal mid_ellipse_x = x + w/2 + a*cos(arc_startAngle + arc_spanAngle/2);
		qreal mid_ellipse_y = y - h/2 + b*sin(arc_startAngle + arc_spanAngle/2);
		qreal mid_line_x = (x1+x2)/2;
		qreal mid_line_y = (y1+y2)/2;

		qreal x3 = (mid_ellipse_x + mid_line_x)/2;
		qreal y3 = (mid_ellipse_y + mid_line_y)/2;

		// find circumcenter of points (x1,y1), (x3,y3) and (x2,y2)
		qreal a1 = 2*x2 - 2*x1;
		qreal b1 = 2*y2 - 2*y1;
		qreal c1 = x1*x1 + y1*y1 - x2*x2 - y2*y2;

		qreal a2 = 2*x3 - 2*x1;
		qreal b2 = 2*y3 - 2*y1;
		qreal c2 = x1*x1 + y1*y1 - x3*x3 - y3*y3;

		qreal center_x = (b1*c2 - b2*c1) / (a1*b2 - a2*b1);
		qreal center_y = (a1*c2 - a2*c1) / (b1*a2 - b2*a1);

		qreal radius = sqrt( (x1-center_x)*(x1-center_x) + (y1-center_y)*(y1-center_y) );

		if ( x1 > center_x && y1 > center_y )
			arc_startAngle = asin( (y1 - center_y) / radius );
		else if ( x1 > center_x && y1 < center_y )
			arc_startAngle = 3.142*2 - asin( (center_y - y1) / radius );
		else if ( x1 < center_x && y1 < center_y )
			arc_startAngle = 3.142 + asin( (center_y - y1) / radius );
		else
			arc_startAngle = 3.142 - asin( (y1 - center_y) / radius );

		qreal arc_endAngle;

		if ( x2 > center_x && y2 > center_y )
			arc_endAngle = asin( (y2 - center_y) / radius );
		else if ( x2 > center_x && y2 < center_y )
			arc_endAngle = 3.142*2 - asin( (center_y - y2) / radius );
		else if ( x2 < center_x && y2 < center_y )
			arc_endAngle = 3.142 + asin( (center_y - y2) / radius );
		else
			arc_endAngle = 3.142 - asin( (y2 - center_y) / radius );

		if (arc_endAngle < arc_startAngle) {
			qreal temp = arc_startAngle;
			arc_startAngle = arc_endAngle;
			arc_endAngle = temp;
		}

		QPointF transformed_point = ExportDialog::rotation_transformed(center_x, center_y, hotspot_x, hotspot_y, rotation_angle);
		center_x = transformed_point.x();
		center_y = transformed_point.y();
		arc_endAngle *= 180/3.142;
		arc_startAngle *= 180/3.142;
		arc_endAngle -= rotation_angle;
		arc_startAngle -= rotation_angle;

		drawArc(file_path, center_x, center_y, radius, arc_startAngle, arc_endAngle, colorcode);
	}
}

/**
 * @brief Createdxf::drawEllipse
 * Conveniance function for draw ellipse
 * @param filepath
 * @param rect
 * @param colorcode
 */
void Createdxf::drawEllipse(const QString &filepath, const QRectF &rect, const int &colorcode) {
	drawArcEllipse(filepath, rect.topLeft().x() * xScale,
						  sheetHeight - (rect.topLeft().y() * yScale),
						  rect.width() * xScale,
						  rect.height() * yScale,
						  0, 360, 0, 0, 0, colorcode);
}

/* draw rectangle in dxf format */
void Createdxf::drawRectangle (const QString &fileName, double x1, double y1, double width, double height, const int &colour)
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

/**
 * @brief Createdxf::drawRectangle
 * Conveniance function for draw rectangle
 * @param filepath
 * @param rect
 * @param color
 */
void Createdxf::drawRectangle(const QString &filepath, const QRectF &rect, const int &colorcode) {
	drawRectangle(filepath, rect.bottomLeft().x() * xScale,
							sheetHeight - (rect.bottomLeft().y() * yScale),
							rect.width() * xScale,
							rect.height() * yScale,
							colorcode);
}

/* draw arc in dx format */
void Createdxf::drawArc(const QString& fileName,double x,double y,double rad,double startAngle,double endAngle,int color)
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
void Createdxf::drawText(const QString& fileName, const QString& text,double x, double y, double height, double rotation, int colour)
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
void Createdxf::drawTextAligned(const QString& fileName, const QString& text,double x, double y, double height, double rotation, double oblique,int hAlign, int vAlign, double xAlign,int colour,
							bool leftAlign, float scale)
{
	Q_UNUSED(scale);

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
