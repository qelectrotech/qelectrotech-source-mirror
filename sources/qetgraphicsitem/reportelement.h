/*
	Copyright 2006-2013 The QElectroTech Team
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

#include "customelement.h"

/**
 * @brief The ReportElement class
 *this class represent an element that can be linked to an other ReportElement
 * a folio report in a diagram is a element that show a wire go on an other folio
 */
class ReportElement : public CustomElement {

	Q_OBJECT

	public  :
	explicit ReportElement(const ElementsLocation &,QString link_type, QGraphicsItem * = 0, Diagram * = 0, int * = 0);
	~ReportElement();
	virtual void linkToElement(Element *);
	virtual void unlinkAllElements();
	virtual void unlinkElement(Element *elmt);
	virtual int linkType() const;

	private:
	int link_type_;
	int inverse_report;
	QString label_;

	signals:

	private slots:
	void setLabel (QString label);
	void updateLabel();
};

#endif // REPORTELEMENT_H
