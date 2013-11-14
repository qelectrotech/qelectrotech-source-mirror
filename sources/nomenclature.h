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
#ifndef NOMENCLATURE_H
#define NOMENCLATURE_H

#include <QtGui>

#include "qetproject.h"
#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "diagramcontent.h"
#include "qetgraphicsitem/customelement.h"

class QETProject;
class Diagram;
class Element;
class DiagramContent;
class CustomeElement;

/**
		This class represents a nomenclature...
*/
class nomenclature : public QObject {
	Q_OBJECT
	
	private:       
	QETProject *m_project;
	QList<Diagram *> m_list_diagram;
	QWidget *m_parent;
	
	// constructors, destructor
	public:
	nomenclature(QETProject *project =0, QWidget *parent =0);
	virtual ~nomenclature();
	
	// attributes
	public:
	
	// methods
	public:
	bool saveToCSVFile();

	
	private:
	QStringList getRows(Diagram *schema);
	
};

#endif

