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
#ifndef TITLEBLOCK_SLASH_LOCATION_CHOOSER_H
#define TITLEBLOCK_SLASH_LOCATION_CHOOSER_H
#include <QtGui>
#include "templatelocation.h"
class TitleBlockTemplateCollection;

/**
	This class is a widget that allows the user to choose a target title block
	template.
*/
class TitleBlockTemplateLocationChooser : public QWidget {
	Q_OBJECT
	// Constructor, destructor
	public:
	TitleBlockTemplateLocationChooser(const TitleBlockTemplateLocation &, QWidget * = 0);
	~TitleBlockTemplateLocationChooser();
	private:
	TitleBlockTemplateLocationChooser(const TitleBlockTemplateLocationChooser &);
	
	// methods
	public:
	TitleBlockTemplateLocation location() const;
	TitleBlockTemplatesCollection *collection() const;
	QString name() const;
	void setLocation(const TitleBlockTemplateLocation &);
	private:
	void init();
	
	// slots
	private slots:
	void updateCollections();
	void updateTemplates();
	void updateNewName();
	
	// attributes
	private:
	QComboBox *collections_;  ///< Collections combo box
	/// Collections index within the combo box
	QHash<int, TitleBlockTemplatesCollection *> collections_index_;
	QComboBox *templates_;    ///< Existing templates combo box
	QLineEdit *new_name_;     ///< New template name textfield
};
#endif
