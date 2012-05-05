/*
	Copyright 2006-2012 Xavier Guerrin
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
	/**
		Constructeur
		@param editor Editeur d'element auquel cette partie est rattachee
	*/
	CustomElementPart(QETElementEditor *editor) : element_editor(editor) {}
	/// Destructeur
	virtual ~CustomElementPart() {}
	
	private:
	CustomElementPart(const CustomElementPart &);
	
	// attributs
	private:
	QETElementEditor *element_editor;
	
	// methodes
	public:
	/**
		Charge la partie depuis un element XML sense le decrire
	*/
	virtual void fromXml(const QDomElement &) = 0;
	/**
		Enregistre la partie dans un document XML
	*/
	virtual const QDomElement toXml(QDomDocument &) const = 0;
	/**
		Permet de modifier une des proprietes de la partie
	*/
	virtual void setProperty(const QString &, const QVariant &) = 0;
	/**
		Permet de lire une des proprietes de la partie
	*/
	virtual QVariant property(const QString &) = 0;
	/**
		@return true si la partie n'est pas pertinente, false sinon
		Typiquement, une partie non pertinente n'est pas conservee lors de
		l'enregistrement de l'element.
	*/
	virtual bool isUseless() const = 0;
	/// @return un pointeur vers l'editeur d'element parent
	virtual QETElementEditor *elementEditor() const;
	/**
		Appelle le slot updateCurrentPartEditor de l'editeur
		@see QETElementEditor::updateCurrentPartEditor()
	*/
	virtual void updateCurrentPartEditor() const;
	/// @return un pointeur vers la scene d'edition parente
	virtual ElementScene *elementScene() const;
	/// @return la pile d'annulations a utiliser
	virtual QUndoStack &undoStack() const;
	/// @return le nom de la partie
	virtual QString name() const = 0;
	/// @return le nom qui sera utilise pour nommer l'element XML lors de l'export
	virtual QString xmlName() const = 0;
};
#endif
