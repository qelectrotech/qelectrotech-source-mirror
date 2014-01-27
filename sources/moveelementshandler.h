/*
	Copyright 2006-2013 The QElectroTech Team
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
	This class defines the minimum interface required to implement an object able
	to handle a move/copy operation among elements collections. This kind of
	objects is typically useful in the move() and copy() method of the
	ElementDefinition and ElementsCategory classes. These methods simply rely on
	answers provided by an instance of this class as soon as they encounter
	typical move/copy problems, such as conflicts or permission issues.
	
	For instance, when copying a particular element to a given category, that
	element may already exist in the target category. It is then possible either
	to erase the target element, change the target name or cancel the operation.
	There is no systematic or calculable answer to this kind of questions, hence
	the rational to delegate the decision process to a dedicated "Strategy" object.
	
	All we know from this object is the fact it implements the interface defined
	below. It is then free to create complicated dialogs to ask a user what to do
	or even to pick a random decision.
	
	@see ElementsCategory
	@see ElementDefinition
*/
class MoveElementsHandler : public QObject {
	Q_OBJECT
	// constructors, destructor
	public:
	MoveElementsHandler(QObject * = 0) {};
	virtual ~MoveElementsHandler() {};
	private:
	MoveElementsHandler(const MoveElementsHandler &);
	
	// methods
	public:
	/**
		@return what to do if the target category already exists
	*/
	virtual QET::Action categoryAlreadyExists(ElementsCategory *src, ElementsCategory  *dst) = 0;
	/**
		@return what to do if the target element already exists
	*/
	virtual QET::Action elementAlreadyExists(ElementDefinition *src, ElementDefinition *dst) = 0;
	
	/**
		@return what to do if a category is not readable
	*/
	virtual QET::Action categoryIsNotReadable(ElementsCategory *) = 0;
	/**
		@return what to do if an element is not readable
	*/
	virtual QET::Action elementIsNotReadable(ElementDefinition *) = 0;
	
	/**
		@return what to do if the target category is not writable
	*/
	virtual QET::Action categoryIsNotWritable(ElementsCategory *) = 0;
	/**
		@return what to do if the target element is not writable
	*/
	virtual QET::Action elementIsNotWritable(ElementDefinition *) = 0;
	
	/**
		@return what to do when an error, described by the provided QString, occurs in relation with the given elements category.
	*/
	virtual QET::Action errorWithACategory(ElementsCategory *, const QString &) = 0;
	/**
		@return what to do when an error, described by the provided QString, occurs in relation with the given element.
	*/
	virtual QET::Action errorWithAnElement(ElementDefinition *, const QString &) = 0;
	
	/**
		@return the name to be used along with the latest QET::Rename answer
	*/
	virtual QString nameForRenamingOperation() = 0;
};
#endif
