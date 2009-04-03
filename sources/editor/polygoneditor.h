/*
	Copyright 2006-2009 Xavier Guerrin
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
#ifndef POLYGON_EDITOR_H
#define POLYGON_EDITOR_H
#include "elementitemeditor.h"
class PartPolygon;
/**
	Cette classe represente le widget d'edition d'un polygone dans l'editeur
	d'element.
*/
class PolygonEditor : public ElementItemEditor {
	
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	PolygonEditor(QETElementEditor *, PartPolygon *, QWidget * = 0);
	virtual ~PolygonEditor();
	
	private:
	PolygonEditor(const PolygonEditor &);
	
	// attributs
	private:
	PartPolygon *part;
	QTreeWidget points_list;
	QCheckBox close_polygon;
	
	// methodes
	private:
	QVector<QPointF> getPointsFromTree();
	
	public slots:
	void updatePolygon();
	void updatePolygonPoints();
	void updatePolygonClosedState();
	void updateForm();
	void validColumn(QTreeWidgetItem *qtwi, int column);
	
	private:
	void activeConnections(bool);
};
#endif
