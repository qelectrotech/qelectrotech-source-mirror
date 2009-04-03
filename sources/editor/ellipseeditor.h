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
#ifndef ELLIPSE_EDITOR_H
#define ELLIPSE_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartEllipse;
/**
	Cette classe represente le widget d'edition d'une ellipse dans l'editeur
	d'element.
*/
class EllipseEditor : public ElementItemEditor {
	Q_OBJECT
	//constructeurs, destructeur
	public:
	EllipseEditor(QETElementEditor *, PartEllipse *, QWidget * = 0);
	~EllipseEditor();
	private:
	EllipseEditor(const EllipseEditor &);
	
	// attributs
	private:
	PartEllipse *part;
	QLineEdit *x, *y, *h, *v;
	
	// methodes
	public slots:
	void updateEllipse();
	void updateEllipseX();
	void updateEllipseY();
	void updateEllipseH();
	void updateEllipseV();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
