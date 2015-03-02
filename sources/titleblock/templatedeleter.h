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
#ifndef TITLEBLOCK_SLASH_TEMPLATE_DELETER_H
#define TITLEBLOCK_SLASH_TEMPLATE_DELETER_H
#include "templatelocation.h"
#include <QtWidgets>
/**
	This class is an abstraction layer to delete an existing title block
	template.
*/
class TitleBlockTemplateDeleter : public QWidget {
	Q_OBJECT
	// Constructors, destructor
	public:
	TitleBlockTemplateDeleter(const TitleBlockTemplateLocation &, QWidget * = 0);
	virtual ~TitleBlockTemplateDeleter();
	private:
	TitleBlockTemplateDeleter(const TitleBlockTemplateDeleter &);
	
	// methods
	public slots:
	bool exec();
	
	// attributes
	private:
	TitleBlockTemplateLocation template_location_;
};
#endif
