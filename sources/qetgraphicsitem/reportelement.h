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
#ifndef REPORTELEMENT_H
#define REPORTELEMENT_H

#include "element.h"

/**
 * @brief The ReportElement class
 *this class represent an element that can be linked to an other ReportElement
 * a folio report in a diagram is a element that show a wire go on an other folio
 */
class ReportElement : public Element
{
	Q_OBJECT

	public :
		explicit ReportElement(const ElementsLocation &,const QString& link_type, QGraphicsItem * = nullptr, int * = nullptr);
		~ReportElement() override;
		void linkToElement(Element *) override;
		void unlinkAllElements() override;
		void unlinkElement(Element *elmt) override;
		
	private:
		int              m_inverse_report;
};

#endif // REPORTELEMENT_H
