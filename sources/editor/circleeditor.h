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
#ifndef CIRCLE_EDITOR_H
#define CIRCLE_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartCircle;
class StyleEditor;
/**
	Cette classe represente un editeur de cercle.
	Elle permet d'editer a travers une interface graphique les
	proprietes d'une cercle composant le dessin d'un element.
*/
class CircleEditor : public ElementItemEditor {
	Q_OBJECT
	// Constructeurs, destructeur
	public:
	CircleEditor(QETElementEditor *, PartCircle * = 0, QWidget * = 0);
	virtual ~CircleEditor();
	private:
	CircleEditor(const CircleEditor &);
	
	// attributs
	private:
	PartCircle *part;
	StyleEditor *style_;
	QLineEdit *x, *y, *r;
	
	// methodes
	public:
	virtual bool setPart(CustomElementPart *);
	virtual CustomElementPart *currentPart() const;
	
	public slots:
	void updateCircle();
	void updateCircleX();
	void updateCircleY();
	void updateCircleD();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
