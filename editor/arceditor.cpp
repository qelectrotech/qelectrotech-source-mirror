/*
	Copyright 2006-2007 Xavier Guerrin
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
#include "arceditor.h"
#include "partarc.h"

ArcEditor::ArcEditor(QETElementEditor *editor, PartArc *arc, QWidget *parent) : ElementItemEditor(editor, parent) {
	
	part = arc;
	
	x = new QLineEdit();
	y = new QLineEdit();
	h = new QLineEdit();
	v = new QLineEdit();
	start_angle = new QSpinBox();
	angle       = new QSpinBox();
	start_angle -> setRange(-360, 360);
	angle       -> setRange(-360, 360);
	
	QGridLayout *grid = new QGridLayout(this);
	grid -> addWidget(new QLabel(tr("Centre : ")),            0, 0);
	grid -> addWidget(new QLabel("x"),                        1, 0);
	grid -> addWidget(x,                                      1, 1);
	grid -> addWidget(new QLabel("y"),                        1, 2);
	grid -> addWidget(y,                                      1, 3);
	grid -> addWidget(new QLabel(tr("Diam\350tres : ")),      2, 0);
	grid -> addWidget(new QLabel(tr("horizontal :")),         3, 0);
	grid -> addWidget(h,                                      3, 1);
	grid -> addWidget(new QLabel(tr("vertical :")),           4, 0);
	grid -> addWidget(v,                                      4, 1);
	grid -> addWidget(new QLabel(tr("Angle de d\351part :")), 5, 0);
	grid -> addWidget(start_angle,                            5, 1);
	grid -> addWidget(new QLabel(tr("Angle :")),              6, 0);
	grid -> addWidget(angle,                                  6, 1);
	updateForm();
	
	activeConnections(true);
}

ArcEditor::~ArcEditor() {
}

void ArcEditor::updateArc() {
	part -> setProperty("x",          x -> text().toDouble());
	part -> setProperty("y",          y -> text().toDouble());
	part -> setProperty("diameter_h", h -> text().toDouble());
	part -> setProperty("diameter_v", v -> text().toDouble());
	part -> setStartAngle(-start_angle -> value() + 90);
	part -> setAngle(-angle -> value());
}

void ArcEditor::updateArcX() { addChangePartCommand(tr("abscisse"),               part, "x",           x -> text().toDouble());       }
void ArcEditor::updateArcY() { addChangePartCommand(tr("ordonn\351e"),            part, "y",           y -> text().toDouble());       }
void ArcEditor::updateArcH() { addChangePartCommand(tr("diam\350tre horizontal"), part, "diameter_h",  h -> text().toDouble());       }
void ArcEditor::updateArcV() { addChangePartCommand(tr("diam\350tre vertical"),   part, "diameter_v",  v -> text().toDouble());       }
void ArcEditor::updateArcS() { addChangePartCommand(tr("angle de d\351part"),     part, "start_angle", -start_angle -> value() + 90); }
void ArcEditor::updateArcA() { addChangePartCommand(tr("angle"),                  part, "angle",       -angle -> value());            }

void ArcEditor::updateForm() {
	activeConnections(false);
	x -> setText(part -> property("x").toString());
	y -> setText(part -> property("y").toString());
	h -> setText(part -> property("diameter_h").toString());
	v -> setText(part -> property("diameter_v").toString());
	start_angle -> setValue(-part -> startAngle() + 90);
	angle -> setValue(-part -> angle());
	activeConnections(true);
}

void ArcEditor::activeConnections(bool active) {
	if (active) {
		connect(x,           SIGNAL(editingFinished()), this, SLOT(updateArcX()));
		connect(y,           SIGNAL(editingFinished()), this, SLOT(updateArcY()));
		connect(h,           SIGNAL(editingFinished()), this, SLOT(updateArcH()));
		connect(v,           SIGNAL(editingFinished()), this, SLOT(updateArcV()));
		connect(start_angle, SIGNAL(editingFinished()), this, SLOT(updateArcS()));
		connect(angle,       SIGNAL(editingFinished()), this, SLOT(updateArcA()));
	} else {
		disconnect(x,           SIGNAL(editingFinished()), this, SLOT(updateArcX()));
		disconnect(y,           SIGNAL(editingFinished()), this, SLOT(updateArcY()));
		disconnect(h,           SIGNAL(editingFinished()), this, SLOT(updateArcH()));
		disconnect(v,           SIGNAL(editingFinished()), this, SLOT(updateArcV()));
		disconnect(start_angle, SIGNAL(editingFinished()), this, SLOT(updateArcS()));
		disconnect(angle,       SIGNAL(editingFinished()), this, SLOT(updateArcA()));
	}
}
