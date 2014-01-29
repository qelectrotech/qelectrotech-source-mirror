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
#include "rectangleeditor.h"
#include "partrectangle.h"

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
	
	x = new QLineEdit();
	y = new QLineEdit();
	w = new QLineEdit();
	h = new QLineEdit();
	
	x -> setValidator(new QDoubleValidator(x));
	y -> setValidator(new QDoubleValidator(y));
	w -> setValidator(new QDoubleValidator(w));
	h -> setValidator(new QDoubleValidator(h));
	
	QVBoxLayout *v_layout = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	grid -> addWidget(new QLabel(tr("Coin sup\351rieur gauche\240: ")), 0, 0, 1, 4);
	grid -> addWidget(new QLabel("x"),                                  1, 0, Qt::AlignRight);
	grid -> addWidget(x,                                                1, 1);
	grid -> addWidget(new QLabel("y"),                                  1, 2);
	grid -> addWidget(y,                                                1, 3);
	grid -> addWidget(new QLabel(tr("Dimensions\240: ")),               2, 0, 1, 4);
	grid -> addWidget(new QLabel(tr("Largeur\240:")),                   3, 0);
	grid -> addWidget(w,                                                3, 1);
	grid -> addWidget(new QLabel(tr("Hauteur\240:")),                   4, 0);
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
	Met a jour le rectangle a partir des donnees du formulaire
*/
void RectangleEditor::updateRectangle() {
	if (!part) return;
	part -> setProperty("x",      x -> text().toDouble());
	part -> setProperty("y",      y -> text().toDouble());
	part -> setProperty("width",  w -> text().toDouble());
	part -> setProperty("height", h -> text().toDouble());
}

/// Met a jour l'abscisse du coin superieur gauche du rectangle et cree un objet d'annulation
void RectangleEditor::updateRectangleX() { addChangePartCommand(tr("abscisse"),               part, "x",           x -> text().toDouble());       }
/// Met a jour l'ordonnee du coin superieur gauche du rectangle et cree un objet d'annulation
void RectangleEditor::updateRectangleY() { addChangePartCommand(tr("ordonn\351e"),            part, "y",           y -> text().toDouble());       }
/// Met a jour la largeur du rectangle et cree un objet d'annulation
void RectangleEditor::updateRectangleW() { addChangePartCommand(tr("largeur"),                part, "width",       w -> text().toDouble());       }
/// Met a jour la hauteur du rectangle et cree un objet d'annulation
void RectangleEditor::updateRectangleH() { addChangePartCommand(tr("hauteur"),                part, "height",      h -> text().toDouble());       }

/**
	Met a jour le formulaire d'edition
*/
void RectangleEditor::updateForm() {
	if (!part) return;
	activeConnections(false);
	x -> setText(part -> property("x").toString());
	y -> setText(part -> property("y").toString());
	w -> setText(part -> property("width").toString());
	h -> setText(part -> property("height").toString());
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
