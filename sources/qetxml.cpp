/*
        Copyright 2006-2015 The QElectroTech Team
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
#include "qetxml.h"
#include "nameslist.h"
#include <QPen>
#include <QDir>

/**
 * @brief QETXML::penToXml
 * Write attribute of a QPen in xml element
 * @param parent_document : parent document for create the QDomElement
 * @param pen : the pen to store
 * @return : A QDomElement with the attribute stored. The tagName of QDomeElement is "pen".
 */
QDomElement QETXML::penToXml(QDomDocument &parent_document,QPen pen)
{
	QDomElement element = parent_document.createElement("pen");

	QString style;
	switch(pen.style())
	{
		case Qt::SolidLine      : style = "SolidLine";      break;
		case Qt::DashLine       : style = "DashLine";       break;
		case Qt::DotLine        : style = "DotLine";        break;
		case Qt::DashDotLine    : style = "DashDotLine";    break;
		case Qt::DashDotDotLine : style = "DashDotDotLine"; break;
		default                 : style = "Unknow";         break;
	}

	element.setAttribute("style", style);
	element.setAttribute("color", pen.color().name());
	element.setAttribute("widthF", QString::number(pen.widthF()));
    return element;
}

/**
 * @brief QETXML::penFromXml
 * Build a QPen from a xml description
 * @param element, The QDomElement that describe the pen
 * @return the created pen. If @element is null or tagName isn't "pen"
 * return a default constructed QPen
 */
QPen QETXML::penFromXml(const QDomElement &element)
{
    QPen pen;

	if (!(!element.isNull() && element.tagName() == "pen"))
	{
		pen.setStyle(Qt::DashLine);
		return pen;
	}

	QString style = element.attribute("style", "DashLine");
	if      (style == "SolidLine")      pen.setStyle(Qt::SolidLine);
	else if (style == "DashLine")       pen.setStyle(Qt::DashLine);
	else if (style == "DotLine")        pen.setStyle(Qt::DotLine);
	else if (style == "DashDotLine")    pen.setStyle(Qt::DashDotLine);
	else if (style == "DashDotDotLine") pen.setStyle(Qt::DashDotDotLine);
	else                                pen.setStyle(Qt::DashLine);

	pen.setColor(QColor(element.attribute("color", "#000000")));
	pen.setWidthF(element.attribute("widthF", "1").toDouble());
    return pen;
}

/**
 * @brief QETXML::brushToXml
 * Write attribute of a QBrush in xml element
 * @param parent_document : parent document for create the QDomElement
 * @param brush : the brush to store
 * @return A QDomElement with the attribute stored. The tagName of QDomeElement is "brush".
 */
QDomElement QETXML::brushToXml(QDomDocument &parent_document, QBrush brush)
{
	QDomElement element = parent_document.createElement("brush");

	QString style;
	switch (brush.style())
	{
		case Qt::NoBrush          : style = "NoBrush";          break;
		case Qt::SolidPattern     : style = "SolidPattern";     break;
		case Qt::Dense1Pattern    : style = "Dense1Pattern";    break;
		case Qt::Dense2Pattern    : style = "Dense2Pattern";    break;
		case Qt::Dense3Pattern    : style = "Dense3Pattern";    break;
		case Qt::Dense4Pattern    : style = "Dense4Pattern";    break;
		case Qt::Dense5Pattern    : style = "Dense5Pattern";    break;
		case Qt::Dense6Pattern    : style = "Dense6Pattern";    break;
		case Qt::Dense7Pattern    : style = "Dense7Pattern";    break;
		case Qt::HorPattern       : style = "HorPattern";       break;
		case Qt::VerPattern       : style = "VerPattern";       break;
		case Qt::CrossPattern     : style = "CrossPattern";     break;
		case Qt::BDiagPattern     : style = "BDiagPattern";     break;
		case Qt::FDiagPattern     : style = "FDiagPattern";     break;
		case Qt::DiagCrossPattern : style = "DiagCrossPattern"; break;
		default                   : style = "Unknow";           break;
	}

	element.setAttribute("style", style);
	element.setAttribute("color", brush.color().name());
	return element;
}

/**
 * @brief QETXML::brushFromXml
 * Build a QBrush from a xml description
 * @param element, the QDomElement that describe the pen
 * @return the created brush. If @element is null or tagName isn't "brush"
 * return a default constructed QBrush
 */
QBrush QETXML::brushFromXml(const QDomElement &element)
{
	QBrush brush;

	if (!(!element.isNull() && element.tagName() == "brush")) return brush;

	QString style = element.attribute("style", "NoBrush");
	if      (style == "NoBrush")          brush.setStyle(Qt::NoBrush);
	else if (style == "SolidPattern")     brush.setStyle(Qt::SolidPattern);
	else if (style == "Dense1Pattern")    brush.setStyle(Qt::Dense1Pattern);
	else if (style == "Dense2Pattern")    brush.setStyle(Qt::Dense2Pattern);
	else if (style == "Dense3Pattern")    brush.setStyle(Qt::Dense3Pattern);
	else if (style == "Dense4Pattern")    brush.setStyle(Qt::Dense4Pattern);
	else if (style == "Dense5Pattern")    brush.setStyle(Qt::Dense5Pattern);
	else if (style == "Dense6Pattern")    brush.setStyle(Qt::Dense6Pattern);
	else if (style == "Dense7Pattern")    brush.setStyle(Qt::Dense7Pattern);
	else if (style == "HorPattern")       brush.setStyle(Qt::HorPattern);
	else if (style == "VerPattern")       brush.setStyle(Qt::VerPattern);
	else if (style == "CrossPattern")     brush.setStyle(Qt::CrossPattern);
	else if (style == "BDiagPattern")     brush.setStyle(Qt::BDiagPattern);
	else if (style == "FDiagPattern")     brush.setStyle(Qt::FDiagPattern);
	else if (style == "DiagCrossPattern") brush.setStyle(Qt::DiagCrossPattern);
	else if (style == "Unknow")           brush.setStyle(Qt::NoBrush);

	brush.setColor(QColor(element.attribute("color", "#000000")));
	return brush;
}

/**
 * @brief QETXML::fileSystemDirToXmlCollectionDir
 * @param document : owner document of returned QDomElement, use to create the QDomElement.
 * @param dir : file system direcory to convert to QDomElement directory
 * @return A file system directory converted to a QDomElement directory ready to be inserted into a XmlElementCollection.
 * If the QDomElement can't be created, return a null QDomElement.
 */
QDomElement QETXML::fileSystemDirToXmlCollectionDir(QDomDocument &document, const QDir &dir)
{
	if (!dir.exists()) return QDomElement();

	QDomElement dir_element = document.createElement("category");
	dir_element.setAttribute("name", dir.dirName());

		//Get the traduction of this directory
	QFile qet_dir(dir.filePath("qet_directory"));
	if (qet_dir.exists() && qet_dir.open(QIODevice::ReadOnly | QIODevice::Text))
	{
			//Get the content of the file
		QDomDocument trad_document;
		if (trad_document.setContent(&qet_dir))
		{
			QDomElement root = trad_document.documentElement();
			if (root.tagName() == "qet-directory")
			{
				NamesList nl;
				nl.fromXml(root);
				dir_element.appendChild(nl.toXml(document));
			}
		}
		qet_dir.close();
	}

	return dir_element;
}

/**
 * @brief QETXML::fileSystemElementToXmlCollectionElement
 * @param document : owner document of returned QDomElement, use to create the QDomElement.
 * @param file : file system element file to convert to QDomElement;
 * @return A file system element converted to a QDomElement ready to be inserted into a XmlElementCollection
 * If the QDomElement can't be created, return a null QDomElement
 */
QDomElement QETXML::fileSystemElementToXmlCollectionElement(QDomDocument &document, QFile &file)
{
	if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QDomDocument docu;
		if (docu.setContent(&file))
		{
			QFileInfo fi(file);
			QDomElement dom_element = document.createElement("element");
			dom_element.setAttribute("name", fi.fileName());
			dom_element.appendChild(docu.documentElement());
			file.close();
			return dom_element;
		}
		else
		{
			file.close();
			return QDomElement();
		}
	}
	else
		return QDomElement();
}
