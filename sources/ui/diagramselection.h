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
#ifndef DIAGRAMSELECTION_H
#define DIAGRAMSELECTION_H

#include <QWidget>

#include "diagram.h"
#include "qetproject.h"

namespace Ui {
	class diagramselection;
}

class diagramselection : public QWidget
{
	Q_OBJECT
	
	public:
	explicit diagramselection(QETProject *prj, QWidget *parent = 0);
	~diagramselection();
	
	QList<Diagram *> list_of_DiagramSelected();
	
	private slots:
	void on_tableDiagram_customContextMenuRequested(const QPoint &pos);
	
	private:
	Ui::diagramselection *ui;
	QETProject *prj_;
	QList<Diagram *> list_diagram_;
	
	void load_TableDiagram();
};

#endif // DIAGRAMSELECTION_H
