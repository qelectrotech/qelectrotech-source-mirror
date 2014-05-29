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
#include "ellipseeditor.h"
#include "styleeditor.h"
#include "partellipse.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param ellipse L'ellipse a editer
	@param parent le Widget parent
*/
EllipseEditor::EllipseEditor(QETElementEditor *editor, PartEllipse *ellipse, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(ellipse)
{
	style_ = new StyleEditor(editor);
	
	x = new QDoubleSpinBox();
	y = new QDoubleSpinBox();
	h = new QDoubleSpinBox();
	v = new QDoubleSpinBox();
	
	x->setRange(-1000, 1000);
	y->setRange(-1000, 1000);
	h->setRange(-1000, 1000);
	v->setRange(-1000, 1000);
	
	QVBoxLayout *v_layout = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	grid -> addWidget(new QLabel(tr("Centre : ")),       0, 0);
	grid -> addWidget(new QLabel("x"),                   1, 0, Qt::AlignRight);
	grid -> addWidget(x,                                 1, 1);
	grid -> addWidget(new QLabel("y"),                   1, 2);
	grid -> addWidget(y,                                 1, 3);
	grid -> addWidget(new QLabel(tr("Diam\350tres : ")), 2, 0);
	grid -> addWidget(new QLabel(tr("horizontal :")),    3, 0);
	grid -> addWidget(h,                                 3, 1);
	grid -> addWidget(new QLabel(tr("vertical :")),      4, 0);
	grid -> addWidget(v,                                 4, 1);
	
	v_layout -> addWidget(style_);
	v_layout -> addLayout(grid);
	
	activeConnections(true);
	updateForm();
}

/// Destructeur
EllipseEditor::~EllipseEditor() {
}

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur d'ellipse acceptera d'editer la primitive new_part s'il s'agit
	d'un objet de la classe PartEllipse.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool EllipseEditor::setPart(CustomElementPart *new_part) {
	if (!new_part) {
		part = 0;
		style_ -> setPart(0);
		return(true);
	}
	if (PartEllipse *part_ellipse = dynamic_cast<PartEllipse *>(new_part)) {
		part = part_ellipse;
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
CustomElementPart *EllipseEditor::currentPart() const {
	return(part);
}

/**
	Met a jour l'ellipse a partir des donnees du formulaire
*/
void EllipseEditor::updateEllipse() {
	if (!part) return;
	part -> setProperty("x",           x  -> value());
	part -> setProperty("y",           y  -> value());
	part -> setProperty("diameter_h",  h  -> value());
	part -> setProperty("diameter_v",  v  -> value());
}

/// Met a jour l'abscisse du centre de l'ellipse et cree un objet d'annulation
void EllipseEditor::updateEllipseX() { addChangePartCommand(tr("abscisse"),               part, "x",           x -> value());       }
/// Met a jour l'ordonnee du centre de l'ellipse et cree un objet d'annulation
void EllipseEditor::updateEllipseY() { addChangePartCommand(tr("ordonn\351e"),            part, "y",           y -> value());       }
/// Met a jour le diametre horizontal de l'ellipse et cree un objet d'annulation
void EllipseEditor::updateEllipseH() { addChangePartCommand(tr("diam\350tre horizontal"), part, "diameter_h",  h -> value());       }
/// Met a jour le diametre vertical de l'ellipse et cree un objet d'annulation
void EllipseEditor::updateEllipseV() { addChangePartCommand(tr("diam\350tre vertical"),   part, "diameter_v",  v -> value());       }

/**
	Met a jour le formulaire d'edition
*/
void EllipseEditor::updateForm() {
	if (!part) return;
	activeConnections(false);
	x->setValue(part->property("x").toReal());
	y->setValue(part->property("y").toReal());
	h->setValue(part->property("diameter_h").toReal());
	v->setValue(part->property("diameter_v").toReal());
	activeConnections(true);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void EllipseEditor::activeConnections(bool active) {
	if (active) {
		connect(x, SIGNAL(editingFinished()), this, SLOT(updateEllipseX()));
		connect(y, SIGNAL(editingFinished()), this, SLOT(updateEllipseY()));
		connect(h, SIGNAL(editingFinished()), this, SLOT(updateEllipseH()));
		connect(v, SIGNAL(editingFinished()), this, SLOT(updateEllipseV()));
	} else {
		disconnect(x, SIGNAL(editingFinished()), this, SLOT(updateEllipseX()));
		disconnect(y, SIGNAL(editingFinished()), this, SLOT(updateEllipseY()));
		disconnect(h, SIGNAL(editingFinished()), this, SLOT(updateEllipseH()));
		disconnect(v, SIGNAL(editingFinished()), this, SLOT(updateEllipseV()));
	}
}
