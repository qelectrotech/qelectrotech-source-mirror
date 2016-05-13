/*
	Copyright 2006-2016 The QElectroTech Team
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
	@param action Action return if an item already exists
*/
void BasicMoveElementsHandler::setActionIfItemAlreadyExists(QET::Action action) {
	already_exists_ = action;
}

/**
	@param action Action return if an item is not readable
*/
void BasicMoveElementsHandler::setActionIfItemIsNotReadable(QET::Action action) {
	not_readable_ = action;
}

/**
	@param action Action return if an item is not writable
*/
void BasicMoveElementsHandler::setActionIfItemIsNotWritable(QET::Action action) {
	not_writable_ = action;
}

/**
	@param action Action if a return item causes an error
*/
void BasicMoveElementsHandler::setActionIfItemTriggersAnError(QET::Action action) {
	error_ = action;
}

/**
	@param name Name refer to a possible renaming operation
	However, it is not recommended to proceed to a systematic renaming, as
	this property is invariable.
*/
void BasicMoveElementsHandler::setNameForRenamingOperation(const QString &name) {
	rename_ = name;
}

/**
	@return the action to be performed if the target category already exists
*/
QET::Action BasicMoveElementsHandler::categoryAlreadyExists(ElementsCategory *, ElementsCategory  *) {
	return(already_exists_);
}

/**
	@return the action performed if the target element already exists
*/
QET::Action BasicMoveElementsHandler::elementAlreadyExists(ElementDefinition *, ElementDefinition *) {
	return(already_exists_);
}

/**
	@return the action to take if the category already exists
*/
QET::Action BasicMoveElementsHandler::categoryIsNotReadable(ElementsCategory *) {
	return(not_readable_);
}

/**
	@return the action to take if the element already exists
*/
QET::Action BasicMoveElementsHandler::elementIsNotReadable(ElementDefinition *) {
	return(not_readable_);
}

/**
	@return the action to be performed if the target category is not accessible
	in writing
*/
QET::Action BasicMoveElementsHandler::categoryIsNotWritable(ElementsCategory *) {
	return(not_writable_);
}

/**
	@return the action performed if the target element is not accessible
	in writing
*/
QET::Action BasicMoveElementsHandler::elementIsNotWritable(ElementDefinition *) {
	return(not_writable_);
}

/**
	@return the action to be performed when the error described in the QString
	occurred with dieters category
*/
QET::Action BasicMoveElementsHandler::errorWithACategory(ElementsCategory *, const QString &) {
	return(error_);
}

/**
	@return the action to be performed when the error described in the QString
	occurred with the element indicates
*/
QET::Action BasicMoveElementsHandler::errorWithAnElement(ElementDefinition *, const QString &) {
	return(error_);
}

/**
	@return the name to use for renaming if a method of this object
	has previously sent back QET::Rename.
*/
QString BasicMoveElementsHandler::nameForRenamingOperation() {
	return(rename_);
}
