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
#ifndef ARC_EDITOR_H
#define ARC_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartArc;
class StyleEditor;
/**
	Cette classe represente le widget d'edition d'un arc dans l'editeur
	d'element.
*/
class ArcEditor : public ElementItemEditor {
	Q_OBJECT
	//constructeurs, destructeur
	public:
	ArcEditor(QETElementEditor *, PartArc * = 0, QWidget * = 0);
	virtual ~ArcEditor();
	private:
	ArcEditor(const ArcEditor &);
	
	// attributs
	private:
	PartArc *part;
	StyleEditor *style_;
	QLineEdit *x, *y, *h, *v;
	QSpinBox *angle, *start_angle;
	
	// methodes
	public:
	virtual bool setPart(CustomElementPart *);
	virtual CustomElementPart *currentPart() const;
	
	public slots:
	void updateArc();
	void updateArcX();
	void updateArcY();
	void updateArcH();
	void updateArcV();
	void updateArcS();
	void updateArcA();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
