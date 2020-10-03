/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "qetxml.h"
#include "nameslist.h"

#include <QPen>
#include <QDir>
#include <QFont>

/**
	@brief QETXML::penToXml
	Write attribute of a QPen in xml element
	@param parent_document : parent document for create the QDomElement
	@param pen : the pen to store
	@return : A QDomElement with the attribute stored.
	The tagName of QDomeElement is "pen".
*/
QDomElement QETXML::penToXml(QDomDocument &parent_document,const QPen& pen)
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
		case Qt::CustomDashLine : style = "CustomDashLine"; break;
		default                 : style = "Unknown";         break;
	}

	element.setAttribute("style", style);
	element.setAttribute("color", pen.color().name());
	element.setAttribute("widthF", QString::number(pen.widthF()));
	return element;
}

/**
	@brief QETXML::penFromXml
	Build a QPen from a xml description
	@param element : The QDomElement that describe the pen
	@return the created pen. If element is null or tagName isn't "pen"
	return a default constructed QPen
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
	else if (style == "CustomDashLine") {
		pen.setStyle(Qt::CustomDashLine);
		pen.setDashPattern( QVector<qreal>() << 10 << 10 );
	}
	else                                pen.setStyle(Qt::DashLine);

	pen.setColor(QColor(element.attribute("color", "#000000")));
	pen.setWidthF(element.attribute("widthF", "1").toDouble());
	return pen;
}

/**
	@brief QETXML::brushToXml
	Write attribute of a QBrush in xml element
	@param parent_document : parent document for create the QDomElement
	@param brush : the brush to store
	@return A QDomElement with the attribute stored.
	The tagName of QDomeElement is "brush".
*/
QDomElement QETXML::brushToXml(
		QDomDocument &parent_document, const QBrush& brush)
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
		default                   : style = "Unknown";          break;
	}

	element.setAttribute("style", style);
	element.setAttribute("color", brush.color().name());
	return element;
}

/**
	@brief QETXML::brushFromXml
	Build a QBrush from a xml description
	@param element : the QDomElement that describe the pen
	@return the created brush. If element is null or tagName isn't "brush"
	return a default constructed QBrush
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
	else if (style == "Unknown")          brush.setStyle(Qt::NoBrush);

	brush.setColor(QColor(element.attribute("color", "#000000")));
	return brush;
}

/**
	@brief QETXML::fileSystemDirToXmlCollectionDir
	@param document : owner document of returned QDomElement,
	use to create the QDomElement.
	@param dir : file system direcory to convert to QDomElement directory
	@param rename : by default the attribute "name" of the returned
	QDomElement is the same name of @dir
	but we can override itwith rename
	@return A file system directory converted to a QDomElement directory
	ready to be inserted into a XmlElementCollection.
	If the QDomElement can't be created, return a null QDomElement.
*/
QDomElement QETXML::fileSystemDirToXmlCollectionDir(
		QDomDocument &document, const QDir &dir, const QString& rename)
{
	if (!dir.exists()) return QDomElement();

	QDomElement dir_element = document.createElement("category");
	dir_element.setAttribute("name", rename.isNull()? dir.dirName() : rename);

		//Get the traduction of this directory
	QFile qet_dir(dir.filePath("qet_directory"));
	if (qet_dir.exists() && qet_dir.open(
				QIODevice::ReadOnly
				| QIODevice::Text))
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
	@brief QETXML::fileSystemElementToXmlCollectionElement
	@param document : owner document of returned QDomElement,
	use to create the QDomElement.
	@param file : file system element file to convert to QDomElement;
	@param rename : by default the attribute "name" of
	the returned QDomElement is the same name of @file
	but we can override itwith rename
	@return A file system element converted to a QDomElement
	ready to be inserted into a XmlElementCollection
	If the QDomElement can't be created, return a null QDomElement
*/
QDomElement QETXML::fileSystemElementToXmlCollectionElement(
		QDomDocument &document, QFile &file, const QString& rename)
{
	if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QDomDocument docu;
		if (docu.setContent(&file))
		{
			QFileInfo fi(file);
			QDomElement dom_element = document.createElement("element");
			dom_element.setAttribute("name", rename.isEmpty()? fi.fileName() : rename);
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

/**
	@brief QETXML::writeXmlFile
	Export an XML document to an UTF-8 text file indented with 4 spaces,
	with LF end of lines and no BOM.
	@param xml_document : An XML document to be exported
	@param file_path : Path to the file to be written
	@param error_message : If non-zero, will contain an error message
	explaining what happened when this function returns false.
	@return false if an error occurred, true otherwise
*/
bool QETXML::writeXmlFile(
		const QDomDocument &xml_document,
		const QString &file_path,
		QString *error_message)
{
	QFile file(file_path);

	// Note: we do not set QIODevice::Text to avoid generating CRLF end of lines
	bool file_opening = file.open(QIODevice::WriteOnly);
	if (!file_opening)
	{
		if (error_message)
		{
			*error_message = QString(
						QObject::tr(
							"Impossible d'ouvrir le fichier %1 en écriture, erreur %2 rencontrée.",
							 "error message when attempting to write an XML file")
							 ).arg(file_path).arg(file.error());
		}
		return(false);
	}

	QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)	// ### Qt 6: remove
	out.setCodec("UTF-8");
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
	out.setEncoding(QStringConverter::Utf8);
#endif
	out.setGenerateByteOrderMark(false);
	out << xml_document.toString(4);
	file.close();

	return(true);
}

/**
	@brief QETXML::textToDomElement
	Return a QDomElement, created from document,
	with tag name tag_name and text value.
	@param document
	@param tag_name
	@param value
	@return a QDomElement, created from document
*/
QDomElement QETXML::textToDomElement(
		QDomDocument &document,
		const QString& tag_name,
		const QString& value)
{
	QDomElement element = document.createElement(tag_name);
	QDomText text = document.createTextNode(value);

	element.appendChild(text);
	return element;
}

/**
	@brief QETXML::directChild
	@param element
	@param tag_name
	@return All direct child of element  with the tag name tag_name
*/
QVector<QDomElement> QETXML::directChild(
		const QDomElement &element, const QString &tag_name)
{
	QVector<QDomElement> return_list;
	for (
		 QDomNode node = element.firstChild() ;
		 !node.isNull() ;
		 node = node.nextSibling())
	{
		if (!node.isElement()) continue;
		QDomElement element = node.toElement();
		if (element.isNull() || element.tagName() != tag_name) continue;
		return_list << element;
	}

	return(return_list);
}

/**
	@brief QETXML::subChild
	@param element
	@param parent_tag_name
	@param children_tag_name
	@return When given an xml dom element element,
	returns a vector of all children dom_elements tagged children_tag_name
	nested in the parent dom elements tagged parent_tag_name,
	themselves children of the dom element element.
*/
QVector<QDomElement> QETXML::subChild(
		const QDomElement &element,
		const QString parent_tag_name,
		const QString &children_tag_name)
{
	QVector<QDomElement> return_list;

	for (
		 QDomNode child = element.firstChild() ;
		 !child.isNull() ;
		 child = child.nextSibling())
	{
		QDomElement parents = child.toElement();
		if (parents.isNull() || parents.tagName() != parent_tag_name)
			continue;

		for (
			 QDomNode node_children = parents.firstChild() ;
			 !node_children.isNull() ;
			 node_children = node_children.nextSibling())
		{
			QDomElement n_children = node_children.toElement();
			if (!n_children.isNull() && n_children.tagName() == children_tag_name)
				return_list.append(n_children);
		}
	}

	return(return_list);
}

/**
	@brief QETXML::marginsToXml
	Save a QMargins to xml. the xml tag name is 'margins'
	@param parent_document
	@param margins
	@return
*/
QDomElement QETXML::marginsToXml(QDomDocument &parent_document,
				 const QMargins &margins)
{
	auto dom_ = parent_document.createElement("margins");
	auto text_ = parent_document.createTextNode(
				QString::number(margins.left())
				+ QString(";")
				+ QString::number(margins.top())
				+ QString(";")
				+ QString::number(margins.right())
				+ QString(";")
				+ QString::number(margins.bottom()));
	dom_.appendChild(text_);
	return  dom_;
}

/**
	@brief QETXML::marginsFromXml
	@param element
	@return a QMargins from an xml description.
	The tag name must ne 'margins'
*/
QMargins QETXML::marginsFromXml(const QDomElement &element)
{
	if (element.tagName() != "margins") {
		return QMargins();
	}

	auto margins_ = element.text().split(";");
	if (margins_.size() == 4) {
		return QMargins(
					margins_.at(0).toInt(),
					margins_.at(1).toInt(),
					margins_.at(2).toInt(),
					margins_.at(3).toInt());
	} else {
		return QMargins();
	}
}

/**
	@brief QETXML::modelHeaderDataToXml
	Save to xml element all header data specified
	by horizontal_section_role and vertical_section_role
	@param parent_document
	@param model
	@param horizontal_section_role : key as header section and value
	as list of roles to save in xml
	@param vertical_section_role :key as header section and value
	as list of roles to save in xml
	@return
*/
QDomElement QETXML::modelHeaderDataToXml(
		QDomDocument &parent_document,
		const QAbstractItemModel *model,
		QHash<int,
		QList<int>> horizontal_section_role,
		QHash<int,
		QList<int>> vertical_section_role)
{
	auto dom_element = parent_document.createElement("header_data");

	auto orientation_ = Qt::Horizontal;
	auto data_hash = horizontal_section_role;
	auto meta_enum_ori = QMetaEnum::fromType<Qt::Orientation>();
	auto meta_enum_role = QMetaEnum::fromType<Qt::ItemDataRole>();

		//Iterate twice, first for horizontal header and second to vertical header
	while (true)
	{
		for (auto section : data_hash.keys())
		{
			for (auto role : data_hash.value(section))
			{
				auto variant = model->headerData(section, orientation_, role);
				if (variant.isValid())
				{
					auto dom_data = parent_document.createElement("data");
					dom_data.setAttribute("section", QString::number(section));
					dom_data.setAttribute("orientation", meta_enum_ori.valueToKey(orientation_));
					dom_data.setAttribute("role", meta_enum_role.valueToKey(role));

					auto text_node = parent_document.createTextNode("");
					if (role == Qt::DisplayRole
						|| role == Qt::EditRole
						|| role == Qt::ToolTipRole
						|| role == Qt::StatusTipRole
						|| role == Qt::WhatsThisRole)
					{
						text_node.setData(variant.toString());
					}
					else if (role == Qt::FontRole)
					{
						auto font = variant.value<QFont>();
						text_node.setData(font.toString());
					}
					else if (role == Qt::TextAlignmentRole)
					{
						auto me = QMetaEnum::fromType<Qt::Alignment>();
						text_node.setData(me.valueToKey(variant.toInt()));
					}
					dom_data.appendChild(text_node);
					dom_element.appendChild(dom_data);
				}
			}
		}

		if(orientation_ == Qt::Vertical) {
			break;
		} else {
			data_hash = vertical_section_role;
			orientation_ = Qt::Vertical;
		}
	}

	return dom_element;
}

/**
	@brief QETXML::modelHeaderDataFromXml
	Restore from xml modele header data
	@param element
	@param model
*/
void QETXML::modelHeaderDataFromXml(
		const QDomElement &element, QAbstractItemModel *model)
{
	if (element.tagName() != "header_data")
		return;

	auto meta_enum_orientation = QMetaEnum::fromType<Qt::Orientations>();
	auto meta_enum_role        = QMetaEnum::fromType<Qt::ItemDataRole>();

	for (auto child : QETXML::directChild(element, "data"))
	{
		auto section_ = child.attribute("section", "-1").toInt();
		auto orientation_ = Qt::Orientation(
					meta_enum_orientation.keyToValue(
						child.attribute(
							"orientation",
							"Horizontal"
							).toStdString().data()));
		auto role_ = meta_enum_role.keyToValue(
					child.attribute(
						"role",
						"DisplayRole"
						).toStdString().data());
		auto text_ = child.text();
		QVariant data_;

		if (role_ == Qt::DisplayRole
				|| role_ == Qt::EditRole
				|| role_ == Qt::ToolTipRole
				|| role_ == Qt::StatusTipRole
				|| role_ == Qt::WhatsThisRole) {
			data_ = text_;
		}
		else if (role_ == Qt::FontRole)
		{
			QFont font;
			font.fromString(text_);
			data_ = font;
		}
		else if (role_ == Qt::TextAlignmentRole)
		{
			auto me = QMetaEnum::fromType<Qt::Alignment>();
			data_ = me.keyToValue(text_.toStdString().data());
		}

		model->setHeaderData(section_, orientation_, data_, role_);
	}
}
