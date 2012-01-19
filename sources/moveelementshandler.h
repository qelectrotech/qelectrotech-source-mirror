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
#ifndef MOVE_ELEMENTS_HANDLER_H
#define MOVE_ELEMENTS_HANDLER_H
#include <QObject>
#include "qet.h"
class ElementDefinition;
class ElementsCategory;
/**
	Cette classe definit l'interface minimale pour implementer un objet capable
	de prendre en main une operation de deplacement ou de copie d'elements.
	Ce type d'objet est typiquement utilise dans les methodes move et copy des
	classes ElementDefinition et ElementsCategory. Ces methodes font appel a cet
	objet pour qu'il leur indique comment gerer les eventuels problemes
	rencontres lors du deplacement / de la copie.
	Exemple : lors de la recopie d'un element dans une categorie, il se peut que
	cet element existe deja. Il est alors possible d'ecraser l'element cible ou
	d'abandonner l'operation. Cette decision est a la charge d'une classe fille
	de MoveElementsHandler.
	Cet objet peut effectuer des interactions avec l'utilisateur ou non.
	Cet aspect, ainsi que la politique de gestion des elements, est laisse aux
	bons soins de l'implementation.
	Il s'agit d'un pattern Strategie qui encapsule non pas l'algorithme de
	deplacement / copie des categories / elements mais la gestion des erreurs
	durant l'execution de cet algorithme.
	@see ElementsCategory
	@see ElementDefinition
*/
class MoveElementsHandler : public QObject {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	MoveElementsHandler(QObject * = 0) {};
	virtual ~MoveElementsHandler() {};
	private:
	MoveElementsHandler(const MoveElementsHandler &);
	
	// methodes
	public:
	/**
		@return l'action a effectuer si la categorie cible existe deja
	*/
	virtual QET::Action categoryAlreadyExists(ElementsCategory *src, ElementsCategory  *dst) = 0;
	/**
		@return l'action a effectuer si l'element cible existe deja
	*/
	virtual QET::Action elementAlreadyExists(ElementDefinition *src, ElementDefinition *dst) = 0;
	
	/**
		@return l'action a effectuer si la categorie existe deja
	*/
	virtual QET::Action categoryIsNotReadable(ElementsCategory *) = 0;
	/**
		@return l'action a effectuer si l'element existe deja
	*/
	virtual QET::Action elementIsNotReadable(ElementDefinition *) = 0;
	
	/**
		@return l'action a effectuer si la categorie cible n'est pas accessible
		en ecriture
	*/
	virtual QET::Action categoryIsNotWritable(ElementsCategory *) = 0;
	/**
		@return l'action a effectuer si l'element cible n'est pas accessible
		en ecriture
	*/
	virtual QET::Action elementIsNotWritable(ElementDefinition *) = 0;
	
	/**
		@return l'action a effectuer lorsque l'erreur decrite dans la QString
		s'est produite avec la categorie indiquee
	*/
	virtual QET::Action errorWithACategory(ElementsCategory *, const QString &) = 0;
	/**
		@return l'action a effectuer lorsque l'erreur decrite dans la QString
		s'est produite avec l'element indique
	*/
	virtual QET::Action errorWithAnElement(ElementDefinition *, const QString &) = 0;
	
	/**
		@return le nom a utiliser pour le renommage si une methode de cet objet
		a precedemment renvoye QET::Rename.
	*/
	virtual QString nameForRenamingOperation() = 0;
};
#endif
