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
#ifndef QETXML_H
#define QETXML_H

#include <QDomElement>
#include <QPen>

class QDomDocument;
class QDir;
class QFile;
class QAbstractItemModel;

/**
 *This namespace contain some function to use xml with QET.
*/
namespace QETXML
{
	QDomElement penToXml(QDomDocument &parent_document, const QPen& pen);
	QPen penFromXml (const QDomElement &element);

	QDomElement brushToXml (
			QDomDocument &parent_document,
			const QBrush& brush);

	QBrush brushFromXml (const QDomElement &element);

	QDomElement fileSystemDirToXmlCollectionDir (
			QDomDocument &document,
			const QDir &dir,
			const QString& rename = QString());

	QDomElement fileSystemElementToXmlCollectionElement (
			QDomDocument &document,
			QFile &file,
			const QString& rename = QString());

	bool writeXmlFile(
			const QDomDocument &xml_document,
			const QString &file_path,
			QString *error_message = nullptr);

	QDomElement textToDomElement (
			QDomDocument &document,
			const QString& tag_name,
			const QString& value);

	QVector <QDomElement> directChild(
			const QDomElement &element,
			const QString &tag_name);

	QVector <QDomElement> subChild(
			const QDomElement &element,
			const QString parent_tag_name,
			const QString &children_tag_name);

	QDomElement marginsToXml (
			QDomDocument &parent_document,
			const QMargins &margins);

	QMargins marginsFromXml(const QDomElement &element);

	QDomElement modelHeaderDataToXml(
			QDomDocument &parent_document,
			const QAbstractItemModel *model,
			QHash<int,
			QList<int>> horizontal_section_role,
			QHash<int,
			QList<int>> vertical_section_role);

	void modelHeaderDataFromXml(
			const QDomElement &element,
			QAbstractItemModel *model);
}

#endif // QETXML_H
