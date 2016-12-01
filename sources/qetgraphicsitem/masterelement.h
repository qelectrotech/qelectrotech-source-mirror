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
#ifndef MASTERELEMENT_H
#define MASTERELEMENT_H

#include "customelement.h"

class CrossRefItem;

/**
 * @brief The MasterElement class
 * This class is a custom element, with extended behavior
 * to be a master element. Master element can be linked with slave element
 * and display a cross ref item for know with what other element he is linked
 */
class MasterElement : public CustomElement
{
	Q_OBJECT
	
	public:
		explicit MasterElement(const ElementsLocation &, QGraphicsItem * = 0, int * = 0);
		~MasterElement();

		virtual void linkToElement     (Element *elmt);
		virtual void unlinkAllElements ();
		virtual void unlinkElement     (Element *elmt);
		virtual void initLink          (QETProject *project);

	public slots:
		void updateLabel(DiagramContext old_info, DiagramContext new_info);

	private:
		bool aboutDeleteXref ();

	private:
		CrossRefItem *m_Xref_item;
};

#endif // MASTERELEMENT_H
