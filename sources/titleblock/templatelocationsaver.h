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
#ifndef TITLEBLOCK_SLASH_LOCATION_SAVER_H
#define TITLEBLOCK_SLASH_LOCATION_SAVER_H
#include <QtGui>
#include "templatelocationchooser.h"
class TitleBlockTemplateCollection;

/**
	This class is a widget that allows the user to choose a target title block
	template, with the intention to save it. Therefore, compared to a
	TitleBlockTemplateLocationChooser, it includes an extra field for the user to
	set the name of the new template if needed.
*/
class TitleBlockTemplateLocationSaver : public TitleBlockTemplateLocationChooser {
	Q_OBJECT
	// Constructor, destructor
	public:
	TitleBlockTemplateLocationSaver(const TitleBlockTemplateLocation &, QWidget * = 0);
	~TitleBlockTemplateLocationSaver();
	private:
	TitleBlockTemplateLocationSaver(const TitleBlockTemplateLocationSaver &);
	
	// methods
	virtual QString name() const;
	virtual void setLocation(const TitleBlockTemplateLocation &);
	
	private:
	void init();
	
	// slots
	protected slots:
	virtual void updateTemplates();
	virtual void updateNewName();
	
	// attributes
	protected:
	QLineEdit *new_name_;     ///< New template name textfield
};
#endif
