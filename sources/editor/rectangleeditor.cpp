/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "rectangleeditor.h"
#include "partrectangle.h"
#include "styleeditor.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param rect Le rectangle a editer
	@param parent le Widget parent
*/
RectangleEditor::RectangleEditor(QETElementEditor *editor, PartRectangle *rect, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(rect)
{
	style_ = new StyleEditor(editor);
	
	x = new QDoubleSpinBox();
	y = new QDoubleSpinBox();
	w = new QDoubleSpinBox();
	h = new QDoubleSpinBox();
	
	x->setRange(-1000, 1000);
	y->setRange(-1000, 1000);
	w->setRange(-1000, 1000);
	h->setRange(-1000, 1000);
	
	QVBoxLayout *v_layout = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	grid -> addWidget(new QLabel(tr("Coin supérieur gauche : ")), 0, 0, 1, 4);
	grid -> addWidget(new QLabel("x"),                                  1, 0, Qt::AlignRight);
	grid -> addWidget(x,                                                1, 1);
	grid -> addWidget(new QLabel("y"),                                  1, 2);
	grid -> addWidget(y,                                                1, 3);
	grid -> addWidget(new QLabel(tr("Dimensions : ")),               2, 0, 1, 4);
	grid -> addWidget(new QLabel(tr("Largeur :")),                   3, 0);
	grid -> addWidget(w,                                                3, 1);
	grid -> addWidget(new QLabel(tr("Hauteur :")),                   4, 0);
	grid -> addWidget(h,                                                4, 1);
	
	v_layout -> addWidget(style_);
	v_layout -> addLayout(grid);
	
	activeConnections(true);
	updateForm();
}

/// Destructeur
RectangleEditor::~RectangleEditor() {
}

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur de rectangle acceptera d'editer la primitive new_part s'il s'agit
	d'un objet de la classe PartRectangle.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool RectangleEditor::setPart(CustomElementPart *new_part) {
	if (!new_part) {
		part = 0;
		style_ -> setPart(0);
		return(true);
	}
	if (PartRectangle *part_rectangle = dynamic_cast<PartRectangle *>(new_part)) {
		part = part_rectangle;
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
CustomElementPart *RectangleEditor::currentPart() const {
	return(part);
}

/**
 * @brief RectangleEditor::topLeft
 * @return The edited topLeft already mapped to part coordinate
 */
QPointF RectangleEditor::editedTopLeft() const {
	return part -> mapFromScene(x->value(), y->value());
}

/**
	Met a jour le rectangle a partir des donnees du formulaire
*/
void RectangleEditor::updateRectangle() {
	if (!part) return;
	part -> setProperty("rectTopLeft", editedTopLeft());
	part -> setProperty("width",  w -> value());
	part -> setProperty("height", h -> value());
}

/// Met a jour l'abscisse du coin superieur gauche du rectangle et cree un objet d'annulation
void RectangleEditor::updateRectangleX() { addChangePartCommand(tr("abscisse"),               part, "rectTopLeft", editedTopLeft());}
/// Met a jour l'ordonnee du coin superieur gauche du rectangle et cree un objet d'annulation
void RectangleEditor::updateRectangleY() { addChangePartCommand(tr("ordonnée"),            part, "rectTopLeft", editedTopLeft());}
/// Met a jour la largeur du rectangle et cree un objet d'annulation
void RectangleEditor::updateRectangleW() { addChangePartCommand(tr("largeur"),                part, "width",       w  -> value());}
/// Met a jour la hauteur du rectangle et cree un objet d'annulation
void RectangleEditor::updateRectangleH() { addChangePartCommand(tr("hauteur"),                part, "height",      h  -> value());}

/**
	Met a jour le formulaire d'edition
*/
void RectangleEditor::updateForm() {
	if (!part) return;
	activeConnections(false);
	QPointF p = part->mapToScene(part->property("rectTopLeft").toPointF());
	x->setValue(p.x());
	y->setValue(p.y());
	w->setValue(part->property("width").toReal());
	h->setValue(part->property("height").toReal());
	activeConnections(true);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void RectangleEditor::activeConnections(bool active) {
	if (active) {
		connect(x, SIGNAL(editingFinished()), this, SLOT(updateRectangleX()));
		connect(y, SIGNAL(editingFinished()), this, SLOT(updateRectangleY()));
		connect(w, SIGNAL(editingFinished()), this, SLOT(updateRectangleW()));
		connect(h, SIGNAL(editingFinished()), this, SLOT(updateRectangleH()));
	} else {
		disconnect(x, SIGNAL(editingFinished()), this, SLOT(updateRectangleX()));
		disconnect(y, SIGNAL(editingFinished()), this, SLOT(updateRectangleY()));
		disconnect(w, SIGNAL(editingFinished()), this, SLOT(updateRectangleW()));
		disconnect(h, SIGNAL(editingFinished()), this, SLOT(updateRectangleH()));
	}
}
