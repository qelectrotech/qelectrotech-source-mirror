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
#ifndef RECTANGLE_EDITOR_H
#define RECTANGLE_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartRectangle;
class StyleEditor;
/**
	This class provides a widget to edit rectangles within the element editor.
*/
class RectangleEditor : public ElementItemEditor {
	Q_OBJECT
	// constructors, destructor
	public:
	RectangleEditor(QETElementEditor *, PartRectangle * = 0, QWidget * = 0);
	virtual ~RectangleEditor();
	private:
	RectangleEditor(const RectangleEditor &);
	
	// attributes
	private:
	PartRectangle *part;
	StyleEditor *style_;
	QLineEdit *x, *y, *w, *h;
	
	// methods
	public:
	virtual bool setPart(CustomElementPart *);
	virtual CustomElementPart *currentPart() const;
	
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
