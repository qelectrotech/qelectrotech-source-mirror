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
#ifndef ABSTRACTELEMENTPROPERTIESEDITORWIDGET_H
#define ABSTRACTELEMENTPROPERTIESEDITORWIDGET_H

#include "PropertiesEditor/propertieseditorwidget.h"

class Element;

/**
 * @brief The AbstractElementPropertiesEditorWidget class
 * This class provide common method for all widget used to edit some properties of an element
 */
class AbstractElementPropertiesEditorWidget : public PropertiesEditorWidget
{
		Q_OBJECT
	public:
		explicit AbstractElementPropertiesEditorWidget(QWidget *parent = 0);
		virtual void setElement(Element *element) =0;

	protected:
		Element *m_element;


};

#endif // ABSTRACTELEMENTPROPERTIESEDITORWIDGET_H
