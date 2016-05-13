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
#ifndef BASIC_MOVE_ELEMENTS_HANDLER
#define BASIC_MOVE_ELEMENTS_HANDLER
#include "moveelementshandler.h"
/**
	This class implements the MoveElementsHandler Strategy class in a baasic way.
	It always returns the same (configurable) action or value for a particular
	question.
*/
class BasicMoveElementsHandler : public MoveElementsHandler {
	Q_OBJECT
	
	// constructors, destructor
	public:
	BasicMoveElementsHandler(QObject * = 0);
	virtual ~BasicMoveElementsHandler();
	private:
	BasicMoveElementsHandler(const BasicMoveElementsHandler &);
	
	// methods
	public:
	virtual void setActionIfItemAlreadyExists(QET::Action);
	virtual void setActionIfItemIsNotReadable(QET::Action);
	virtual void setActionIfItemIsNotWritable(QET::Action);
	virtual void setActionIfItemTriggersAnError(QET::Action);
	virtual void setNameForRenamingOperation(const QString &);
	
	virtual QET::Action categoryAlreadyExists(ElementsCategory *src, ElementsCategory  *dst);
	virtual QET::Action elementAlreadyExists(ElementDefinition *src, ElementDefinition *dst);
	virtual QET::Action categoryIsNotReadable(ElementsCategory *);
	virtual QET::Action elementIsNotReadable(ElementDefinition *);
	virtual QET::Action categoryIsNotWritable(ElementsCategory *);
	virtual QET::Action elementIsNotWritable(ElementDefinition *);
	virtual QET::Action errorWithACategory(ElementsCategory *, const QString &);
	virtual QET::Action errorWithAnElement(ElementDefinition *, const QString &);
	virtual QString nameForRenamingOperation();
	
	// attributes
	private:
	QET::Action already_exists_;
	QET::Action not_readable_;
	QET::Action not_writable_;
	QET::Action error_;
	QString rename_;
};
#endif
