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
#ifndef DIAGRAMFOLIOLIST_H
#define DIAGRAMFOLIOLIST_H

#include "diagram.h"

class DiagramFolioList : public Diagram
{
	public:
	DiagramFolioList(QObject *parent = 0) : Diagram(parent) {}
	virtual ~DiagramFolioList() {}

	protected:
	void drawBackground(QPainter *, const QRectF &);

	private:
	void fillRow(QPainter *, const QRectF &, QString, QString, QString, QString);

};

#endif // DIAGRAMFOLIOLIST_H
