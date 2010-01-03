/*
	Copyright 2006-2010 Xavier Guerrin
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
#ifndef RECTANGLE_EDITOR_H
#define RECTANGLE_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartRectangle;
/**
	Cette classe represente le widget d'edition d'un rectangle dans l'editeur
	d'element.
*/
class RectangleEditor : public ElementItemEditor {
	Q_OBJECT
	//constructeurs, destructeur
	public:
	RectangleEditor(QETElementEditor *, PartRectangle *, QWidget * = 0);
	~RectangleEditor();
	private:
	RectangleEditor(const RectangleEditor &);
	
	// attributs
	private:
	PartRectangle *part;
	QLineEdit *x, *y, *w, *h;
	
	// methodes
	public slots:
	void updateRectangle();
	void updateRectangleX();
	void updateRectangleY();
	void updateRectangleW();
	void updateRectangleH();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
