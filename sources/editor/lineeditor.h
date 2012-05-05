/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartLine;
class StyleEditor;
/**
	Cette classe represente le widget d'edition d'une ligne dans l'editeur
	d'element.
*/
class LineEditor : public ElementItemEditor {
	Q_OBJECT
	//constructeurs, destructeur
	public:
	LineEditor(QETElementEditor *, PartLine * = 0, QWidget * = 0);
	virtual ~LineEditor();
	private:
	LineEditor(const LineEditor &);
	
	// attributs
	private:
	PartLine *part;
	StyleEditor *style_;
	QLineEdit *x1, *y1, *x2, *y2;
	QComboBox *end1_type, *end2_type;
	QLineEdit *end1_length, *end2_length;
	
	// methodes
	public:
	virtual bool setPart(CustomElementPart *);
	virtual CustomElementPart *currentPart() const;
	
	public slots:
	void updateLine();
	void updateLineX1();
	void updateLineY1();
	void updateLineX2();
	void updateLineY2();
	void updateLineEndType1();
	void updateLineEndLength1();
	void updateLineEndType2();
	void updateLineEndLength2();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
