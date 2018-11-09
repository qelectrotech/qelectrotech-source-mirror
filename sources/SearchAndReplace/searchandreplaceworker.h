/*
	Copyright 2006-2018 The QElectroTech Team
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
#ifndef SEARCHANDREPLACEWORKER_H
#define SEARCHANDREPLACEWORKER_H

#include <QDate>

#include "titleblockproperties.h"

class Diagram;
class Element;

/**
 * @brief The SearchAndReplaceWorker class
 * This class is the worker use to change properties when use the search and replace function of QET
 */
class SearchAndReplaceWorker
{
	public:
		SearchAndReplaceWorker();
		
		void clear();
		void replaceDiagram(QList <Diagram *> diagram_list);
		void replaceDiagram(Diagram *diagram);
		void replaceElement(QList <Element *> list);
		void replaceElement(Element *element);
		
		static QString eraseText() {return QString("XXXXXXXXXXXXXXXXXXX");}
		static QDate eraseDate() {return QDate(1900, 1, 1);}
		
	private:
		TitleBlockProperties m_titleblock_properties;
		DiagramContext m_element_context;
		
		friend class SearchAndReplaceWidget;
};

#endif // SEARCHANDREPLACEWORKER_H
