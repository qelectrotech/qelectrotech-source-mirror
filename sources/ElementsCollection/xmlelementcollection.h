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
#ifndef XMLELEMENTCOLLECTION_H
#define XMLELEMENTCOLLECTION_H

#include <QObject>
#include <QDomElement>
#include "elementslocation.h"

class QDomElement;
class QFile;

/**
 * @brief The XmlElementCollection class
 * This class represent a collection of elements stored to xml
 */
class XmlElementCollection : public QObject
{
		Q_OBJECT
	public:
		XmlElementCollection (QObject *parent = nullptr);
		XmlElementCollection (const QDomElement &dom_element, QObject *parent = nullptr);
		QDomElement root() const;
		QDomElement importCategory() const;
		QDomNodeList childs(const QDomElement &parent_element) const;
		QDomElement child(const QDomElement &parent_element, const QString &child_name) const;
		QDomElement child(const QString &path) const;
		QList<QDomElement> directories(const QDomElement &parent_element);
		QStringList directoriesNames(const QDomElement &parent_element);
		QList<QDomElement> elements(const QDomElement &parent_element);
		QStringList elementsNames(const QDomElement &parent_element);
		QDomElement element(const QString &path);
		QDomElement directory(const QString &path);
		QString addElement (const QString &path);
		ElementsLocation copy (ElementsLocation &source, ElementsLocation &destination, QString rename = QString(), bool deep_copy = true);
		bool exist (const QString &path);

	private:
		ElementsLocation copyDirectory(ElementsLocation &source, ElementsLocation &destination, QString rename = QString(), bool deep_copy = true);
		ElementsLocation copyElement(ElementsLocation &source, ElementsLocation &destination, QString rename = QString());

	signals:
			/**
			 * @brief elementAdded
			 * This signal is emited when a element is added to this collection
			 * @param collection_path, the path of element in this collection
			 */
		void elementAdded(QString collection_path);

	public slots:

	private:
		QDomDocument m_dom_document;
};

#endif // XMLELEMENTCOLLECTION_H
