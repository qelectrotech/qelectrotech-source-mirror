/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef TITLEBLOCK_SLASH_MOVE_TEMPLATES_HANDLER_H
#define TITLEBLOCK_SLASH_MOVE_TEMPLATES_HANDLER_H
#include <QtCore>
#include "qet.h"
#include "templatelocation.h"

/**
	This class defines the minimal interface required to implement an object
	able to handle a title block template move or copy.
	It is a Strategy pattern that embeds the copy/move error handling instead
	of the whole process.
*/
class MoveTitleBlockTemplatesHandler : public QObject {
	Q_OBJECT
	
	// Constructors, destructor
	public:
	MoveTitleBlockTemplatesHandler(QObject * = 0) {}
	virtual ~MoveTitleBlockTemplatesHandler() {}
	private:
	MoveTitleBlockTemplatesHandler(const MoveTitleBlockTemplatesHandler &);
	
	// methods
	public:
	virtual QET::Action templateAlreadyExists(const TitleBlockTemplateLocation &src, const TitleBlockTemplateLocation &dst) = 0;
	virtual QET::Action errorWithATemplate(const TitleBlockTemplateLocation &, const QString &) = 0;
	virtual QString nameForRenamingOperation() = 0;
};
#endif
