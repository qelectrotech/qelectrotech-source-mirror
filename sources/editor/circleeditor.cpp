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
#include "circleeditor.h"
#include "styleeditor.h"
#include "partcircle.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param circle Le cercle a editer
	@param parent le Widget parent
*/
CircleEditor::CircleEditor(QETElementEditor *editor, PartCircle *circle, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(circle)
{
	style_ = new StyleEditor(editor);
	
	x = new QLineEdit();
	y = new QLineEdit();
	r = new QLineEdit();
	
	x -> setValidator(new QDoubleValidator(x));
	y -> setValidator(new QDoubleValidator(y));
	r -> setValidator(new QDoubleValidator(r));
	
	QVBoxLayout *v_layout = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	grid -> addWidget(new QLabel(tr("Centre : ")),       0, 0);
	grid -> addWidget(new QLabel("x"),                   1, 0, Qt::AlignRight);
	grid -> addWidget(x,                                 1, 1);
	grid -> addWidget(new QLabel("y"),                   1, 2);
	grid -> addWidget(y,                                 1, 3);
	grid -> addWidget(new QLabel(tr("Diam\350tre : ")),  2, 0);
	grid -> addWidget(r,                                 2, 1);
	
	v_layout -> addWidget(style_);
	v_layout -> addLayout(grid);
	
	activeConnections(true);
	updateForm();
}

/// Destructeur
CircleEditor::~CircleEditor() {
}

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur de cercle acceptera d'editer la primitive new_part s'il s'agit
	d'un objet de la classe PartCircle.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool CircleEditor::setPart(CustomElementPart *new_part) {
	if (!new_part) {
		part = 0;
		style_ -> setPart(0);
		return(true);
	}
	if (PartCircle *part_circle = dynamic_cast<PartCircle *>(new_part)) {
		part = part_circle;
		style_ -> setPart(part);
		updateForm();
		return(true);
	} else {
		return(false);
	}
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *CircleEditor::currentPart() const {
	return(part);
}

/**
	met a jour le cercle a partir des donnees du formulaire
*/
void CircleEditor::updateCircle() {
	if (!part) return;
	part -> setProperty("x", x -> text().toDouble());
	part -> setProperty("y", y -> text().toDouble());
	part -> setProperty("diameter", r -> text().toDouble());
}

/// Met a jour l'abscisse du cercle et cree un objet d'annulation
void CircleEditor::updateCircleX() { addChangePartCommand(tr("abscisse"),    part, "x",        x -> text().toDouble()); }

/// Met a jour l'ordonnee du cercle et cree un objet d'annulation
void CircleEditor::updateCircleY() { addChangePartCommand(tr("ordonn\351e"), part, "y",        y -> text().toDouble()); }

/// Met a jour le diametre du cercle et cree un objet d'annulation
void CircleEditor::updateCircleD() { addChangePartCommand(tr("diam\350tre"), part, "diameter", r -> text().toDouble()); }

/**
	Met a jour le formulaire d'edition
*/
void CircleEditor::updateForm() {
	if (!part) return;
	activeConnections(false);
	x -> setText(part -> property("x").toString());
	y -> setText(part -> property("y").toString());
	r -> setText(part -> property("diameter").toString());
	activeConnections(true);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
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
