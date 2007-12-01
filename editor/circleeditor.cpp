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
#include "circleeditor.h"
#include "partcircle.h"

CircleEditor::CircleEditor(QETElementEditor *editor, PartCircle *circle, QWidget *parent) : ElementItemEditor(editor, parent) {
	
	part = circle;
	
	x = new QLineEdit();
	y = new QLineEdit();
	r = new QLineEdit();
	
// 	QDoubleValidator *format = new QDoubleValidator(-1000.0, -1000.0, 4, this);
// 	x -> setValidator(new QDoubleValidator(-1000.0, 1000.0, 4, this));
// 	y -> setValidator(new QDoubleValidator(-1000.0, 1000.0, 4, this));
// 	h -> setValidator(new QDoubleValidator(0.0, 1000.0, 4, this));
	
	
	QGridLayout *grid = new QGridLayout(this);
	grid -> addWidget(new QLabel(tr("Centre : ")),       0, 0);
	grid -> addWidget(new QLabel("x"),                   1, 0);
	grid -> addWidget(x,                                 1, 1);
	grid -> addWidget(new QLabel("y"),                   1, 2);
	grid -> addWidget(y,                                 1, 3);
	grid -> addWidget(new QLabel(tr("Diam\350tre : ")),  2, 0);
	grid -> addWidget(r,                                 2, 1);
	
	activeConnections(true);
	updateForm();
}

CircleEditor::~CircleEditor() {
}

void CircleEditor::updateCircle() {
	part -> setProperty("x", x -> text().toDouble());
	part -> setProperty("y", y -> text().toDouble());
	part -> setProperty("diameter", r -> text().toDouble());
}

void CircleEditor::updateCircleX() { addChangePartCommand(tr("abscisse"),    part, "x",        x -> text().toDouble()); }
void CircleEditor::updateCircleY() { addChangePartCommand(tr("ordonn\351e"), part, "y",        y -> text().toDouble()); }
void CircleEditor::updateCircleD() { addChangePartCommand(tr("diam\350tre"), part, "diameter", r -> text().toDouble()); }

void CircleEditor::updateForm() {
	activeConnections(false);
	x -> setText(part -> property("x").toString());
	y -> setText(part -> property("y").toString());
	r -> setText(part -> property("diameter").toString());
	activeConnections(true);
}

void CircleEditor::activeConnections(bool active) {
	if (active) {
		connect(x, SIGNAL(editingFinished()), this, SLOT(updateCircleX()));
		connect(y, SIGNAL(editingFinished()), this, SLOT(updateCircleY()));
		connect(r, SIGNAL(editingFinished()), this, SLOT(updateCircleD()));
	} else {
		disconnect(x, SIGNAL(editingFinished()), this, SLOT(updateCircleX()));
		disconnect(y, SIGNAL(editingFinished()), this, SLOT(updateCircleY()));
		disconnect(r, SIGNAL(editingFinished()), this, SLOT(updateCircleD()));
	}
}
