/*
	Copyright 2006-2010 Xavier Guerrin
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
#include "texteditor.h"
#include "parttext.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param text Champ de texte a editer
	@param parent QWidget parent de ce widget
*/
TextEditor::TextEditor(QETElementEditor *editor, PartText *text, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(text)
{
	qle_x     = new QLineEdit();
	qle_y     = new QLineEdit();
	qle_text  = new QLineEdit();
	font_size = new QSpinBox();
	font_size -> setRange(0, 144);
	
	qle_x -> setValidator(new QDoubleValidator(qle_x));
	qle_y -> setValidator(new QDoubleValidator(qle_y));
	
	QVBoxLayout *main_layout = new QVBoxLayout();
	main_layout -> addWidget(new QLabel(tr("Position : ")));
	
	QHBoxLayout *position = new QHBoxLayout();
	position -> addWidget(new QLabel(tr("x : ")));
	position -> addWidget(qle_x                 );
	position -> addWidget(new QLabel(tr("y : ")));
	position -> addWidget(qle_y                 );
	main_layout -> addLayout(position);
	
	QHBoxLayout *fs = new QHBoxLayout();
	fs -> addWidget(new QLabel(tr("Taille : ")));
	fs -> addWidget(font_size);
	main_layout -> addLayout(fs);
	
	QHBoxLayout *t = new QHBoxLayout();
	t -> addWidget(new QLabel(tr("Texte : ")));
	t -> addWidget(qle_text);
	main_layout -> addLayout(t);
	main_layout -> addStretch();
	setLayout(main_layout);
	
	updateForm();
}

/**
	Destructeur
*/
TextEditor::~TextEditor() {
}

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur de texte statique acceptera d'editer la primitive new_part s'il
	s'agit d'un objet de la classe PartText.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool TextEditor::setPart(CustomElementPart *new_part) {
	if (!new_part) {
		part = 0;
		return(true);
	}
	if (PartText *part_text = dynamic_cast<PartText *>(new_part)) {
		part = part_text;
		updateForm();
		return(true);
	} else {
		return(false);
	}
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *TextEditor::currentPart() const {
	return(part);
}

/**
	Met a jour le champ de texte a partir des donnees du formulaire
*/
void TextEditor::updateText() {
	if (!part) return;
	part -> setProperty("size", font_size -> value());
	part -> setPlainText(qle_text -> text());
	part -> setPos(qle_x -> text().toDouble(), qle_y -> text().toDouble());
}

/// Met a jour l'abscisse de la position du texte et cree un objet d'annulation
void TextEditor::updateTextX() { addChangePartCommand(tr("abscisse"),    part, "x",    qle_x -> text().toDouble()); updateForm(); }
/// Met a jour l'ordonnee de la position du texte et cree un objet d'annulation
void TextEditor::updateTextY() { addChangePartCommand(tr("ordonn\351e"), part, "y",    qle_y -> text().toDouble()); updateForm(); }
/// Met a jour le texte et cree un objet d'annulation
void TextEditor::updateTextT() { addChangePartCommand(tr("contenu"),     part, "text", qle_text -> text());         }
/// Met a jour la taille du texte et cree un objet d'annulation
void TextEditor::updateTextS() { addChangePartCommand(tr("taille"),      part, "size", font_size -> value());       }

/**
	Met a jour le formulaire a partir du champ de texte
*/
void TextEditor::updateForm() {
	if (!part) return;
	activeConnections(false);
	qle_x     -> setText(part -> property("x").toString());
	qle_y     -> setText(part -> property("y").toString());
	qle_text  -> setText(part -> property("text").toString());
	font_size -> setValue(part -> property("size").toInt());
	activeConnections(true);
}

void TextEditor::activeConnections(bool active) {
	if (active) {
		connect(qle_x,     SIGNAL(editingFinished()), this, SLOT(updateTextX()));
		connect(qle_y,     SIGNAL(editingFinished()), this, SLOT(updateTextY()));
		connect(qle_text,  SIGNAL(editingFinished()), this, SLOT(updateTextT()));
		connect(font_size, SIGNAL(editingFinished()), this, SLOT(updateTextS()));
	} else {
		disconnect(qle_x,     SIGNAL(editingFinished()), this, SLOT(updateTextX()));
		disconnect(qle_y,     SIGNAL(editingFinished()), this, SLOT(updateTextY()));
		disconnect(qle_text,  SIGNAL(editingFinished()), this, SLOT(updateTextT()));
		disconnect(font_size, SIGNAL(editingFinished()), this, SLOT(updateTextS()));
	}
}
