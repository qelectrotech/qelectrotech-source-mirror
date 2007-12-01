/*
	Copyright 2006-2007 Xavier Guerrin
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
#ifndef CUSTOM_ELEMENT_PART_H
#define CUSTOM_ELEMENT_PART_H
#include <QtGui>
#include <QtXml>
#include <QImage>
class CustomElement;
class QETElementEditor;
class ElementScene;
/**
	Cette classe abstraite represente une partie de la representation graphique
	d'un element de schema electrique. Les attributs et methodes qu'elle
	encapsule ne sont pas integres directement dans la classe CustomElement
	afin de ne pas alourdir celle-ci. Il est en effet inutile pour cette classe
	de retenir sa conception graphique autrement que sous la forme d'une
	QImage.
*/
class CustomElementPart {
	// constructeurs, destructeur
	public:
	CustomElementPart(QETElementEditor *editor) : element_editor(editor) {}
	virtual ~CustomElementPart() {}
	
	private:
	CustomElementPart(const CustomElementPart &);
	
	// attributs
	private:
	QETElementEditor *element_editor;
	
	// methodes
	public:
	virtual void fromXml(const QDomElement &) = 0;
	virtual const QDomElement toXml(QDomDocument &) const = 0;
	virtual QWidget *elementInformations() = 0;
	virtual void setProperty(const QString &, const QVariant &) = 0;
	virtual QVariant property(const QString &) = 0;
	virtual QETElementEditor *elementEditor() const;
	virtual ElementScene *elementScene() const;
	virtual QUndoStack &undoStack() const;
	virtual QString name() const = 0;
};
#endif
