/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef SIMPLEELEMENT_H
#define SIMPLEELEMENT_H

#include "customelement.h"

/**
 * @brief The SimpleElement class
 *this class represente a simple element with no specific attribute
 */
class SimpleElement : public CustomElement {

	Q_OBJECT

	public  :
	explicit SimpleElement(const ElementsLocation &, QGraphicsItem * = 0, Diagram * = 0, int * = 0);

	signals:

	public slots:

};

#endif // SIMPLEELEMENT_H
