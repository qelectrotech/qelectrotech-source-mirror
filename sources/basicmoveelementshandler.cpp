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
#include "basicmoveelementshandler.h"

/**
	Constructeur
	@param parent QObject parent
*/
BasicMoveElementsHandler::BasicMoveElementsHandler(QObject *parent) :
	MoveElementsHandler(parent),
	already_exists_(QET::Erase),
	not_readable_(QET::Ignore),
	not_writable_(QET::Ignore),
	error_(QET::Ignore),
	rename_("renamed")
{
}

/**
	Destructeur
*/
BasicMoveElementsHandler::~BasicMoveElementsHandler() {
}

/**
	@param action Action a renvoyer si un item existe deja
*/
void BasicMoveElementsHandler::setActionIfItemAlreadyExists(QET::Action action) {
	already_exists_ = action;
}

/**
	@param action Action a renvoyer si un item n'est pas lisible
*/
void BasicMoveElementsHandler::setActionIfItemIsNotReadable(QET::Action action) {
	not_readable_ = action;
}

/**
	@param action Action a renvoyer si un item n'est pas accessible en ecriture
*/
void BasicMoveElementsHandler::setActionIfItemIsNotWritable(QET::Action action) {
	not_writable_ = action;
}

/**
	@param action Action a renvoyer si un item provoque une erreur
*/
void BasicMoveElementsHandler::setActionIfItemTriggersAnError(QET::Action action) {
	error_ = action;
}

/**
	@param name Nom a renvoyer pour une eventuelle operation de renommage
	Il est toutefois deconseille de proceder a un renommage systematique, vu que
	cette propriete est invariable.
*/
void BasicMoveElementsHandler::setNameForRenamingOperation(const QString &name) {
	rename_ = name;
}

/**
	@return l'action a effectuer si la categorie cible existe deja
*/
QET::Action BasicMoveElementsHandler::categoryAlreadyExists(ElementsCategory *, ElementsCategory  *) {
	return(already_exists_);
}

/**
	@return l'action a effectuer si l'element cible existe deja
*/
QET::Action BasicMoveElementsHandler::elementAlreadyExists(ElementDefinition *, ElementDefinition *) {
	return(already_exists_);
}

/**
	@return l'action a effectuer si la categorie existe deja
*/
QET::Action BasicMoveElementsHandler::categoryIsNotReadable(ElementsCategory *) {
	return(not_readable_);
}

/**
	@return l'action a effectuer si l'element existe deja
*/
QET::Action BasicMoveElementsHandler::elementIsNotReadable(ElementDefinition *) {
	return(not_readable_);
}

/**
	@return l'action a effectuer si la categorie cible n'est pas accessible
	en ecriture
*/
QET::Action BasicMoveElementsHandler::categoryIsNotWritable(ElementsCategory *) {
	return(not_writable_);
}

/**
	@return l'action a effectuer si l'element cible n'est pas accessible
	en ecriture
*/
QET::Action BasicMoveElementsHandler::elementIsNotWritable(ElementDefinition *) {
	return(not_writable_);
}

/**
	@return l'action a effectuer lorsque l'erreur decrite dans la QString
	s'est produite avec la categorie indiquee
*/
QET::Action BasicMoveElementsHandler::errorWithACategory(ElementsCategory *, const QString &) {
	return(error_);
}

/**
	@return l'action a effectuer lorsque l'erreur decrite dans la QString
	s'est produite avec l'element indique
*/
QET::Action BasicMoveElementsHandler::errorWithAnElement(ElementDefinition *, const QString &) {
	return(error_);
}

/**
	@return le nom a utiliser pour le renommage si une methode de cet objet
	a precedemment renvoye QET::Rename.
*/
QString BasicMoveElementsHandler::nameForRenamingOperation() {
	return(rename_);
}
