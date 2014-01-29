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
#ifndef ARC_EDITOR_H
#define ARC_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartArc;
class StyleEditor;
/**
	This class provides a widget to edit elliptical arcs within the element editor.
*/
class ArcEditor : public ElementItemEditor {
	Q_OBJECT
	// constructors, destructor
	public:
	ArcEditor(QETElementEditor *, PartArc * = 0, QWidget * = 0);
	virtual ~ArcEditor();
	private:
	ArcEditor(const ArcEditor &);
	
	// attributes
	private:
	PartArc *part;
	StyleEditor *style_;
	QLineEdit *x, *y, *h, *v;
	QSpinBox *angle, *start_angle;
	
	// methods
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
