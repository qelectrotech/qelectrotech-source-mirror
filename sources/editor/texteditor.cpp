/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "qetapp.h"
#include "qtextorientationspinboxwidget.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param text Champ de texte a editer
	@param parent QWidget parent de ce widget
*/
TextEditor::TextEditor(QETElementEditor *editor, PartText *text, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(text),
	m_locked(false)
{
	qle_x     = new QDoubleSpinBox();
	qle_y     = new QDoubleSpinBox();
	qle_text  = new QLineEdit();
	qle_text  ->setClearButtonEnabled(true);
	font_size = new QSpinBox();
	font_size -> setRange(0, 144);
	black_color_ = new QRadioButton(tr("Noir", "element text part color"));
	white_color_ = new QRadioButton(tr("Blanc", "element text part color"));
	color_ = new QButtonGroup(this);
	color_ -> addButton(black_color_, true);
	color_ -> addButton(white_color_, false);
	connect(color_, SIGNAL(buttonClicked(int)), this, SLOT(updateTextC()));
	QLabel *rotation_angle_label = new QLabel(tr("Angle de rotation : "));
	rotation_angle_label -> setWordWrap(true);
	rotation_angle_ = QETApp::createTextOrientationSpinBoxWidget();
	
	qle_x -> setRange(-5000, 5000);
	qle_y -> setRange(-5000, 5000);
	
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
	
	QHBoxLayout *color_layout = new QHBoxLayout();
	color_layout -> addWidget(new QLabel(tr("Couleur : ")));
	color_layout -> addWidget(black_color_);
	color_layout -> addWidget(white_color_);
	color_layout -> addStretch();
	main_layout -> addLayout(color_layout);
	
	QHBoxLayout *t = new QHBoxLayout();
	t -> addWidget(new QLabel(tr("Texte : ")));
	t -> addWidget(qle_text);
	
	QHBoxLayout *rotation_angle_layout = new QHBoxLayout();
	rotation_angle_layout -> addWidget(rotation_angle_label);
	rotation_angle_layout -> addWidget(rotation_angle_);
	
	main_layout -> addLayout(t);
	main_layout -> addLayout(rotation_angle_layout);
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
bool TextEditor::setPart(CustomElementPart *new_part)
{
	if (!new_part)
	{
		part = 0;
		return(true);
	}
	if (PartText *part_text = dynamic_cast<PartText *>(new_part))
	{
		if (part == part_text) return true;
		part = part_text;
		updateForm();
		return(true);
	}
	return(false);
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *TextEditor::currentPart() const {
	return(part);
}

/// Met a jour le texte et cree un objet d'annulation
void TextEditor::updateTextT()
{
	if(m_locked) return;
	m_locked = true;
	QString text = qle_text->text();
	if (text != part->property("text"))
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "text", part->property("text"), text);
		undo->setText(tr("Modifier le contenu d'un champ texte"));
		undoStack().push(undo);
	}
	m_locked= false;
}

/// Met a jour la taille du texte et cree un objet d'annulation
void TextEditor::updateTextS()
{
	if(m_locked) return;
	m_locked = true;
	int size = font_size->value();
	if (size != part->property("size"))
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "size", part->property("size"), size);
		undo->setText(tr("Modifier la taille d'un champ texte"));
		undo->enableAnimation();
		undoStack().push(undo);
	}
	m_locked= false;
}

/// Update the text color and create an undo object
void TextEditor::updateTextC()
{
	if(m_locked) return;
	m_locked = true;
	int color = color_->checkedId();
	if (color != part->property("color"))
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "color", part->property("color"), color);
		undo->setText(tr("Modifier la couleur d'un champ texte"));
		undoStack().push(undo);
	}
	m_locked= false;
}
/// Met a jour l'angle de rotation du champ de texte et cree un objet d'annulation
void TextEditor::updateTextRotationAngle()
{
	if(m_locked) return;
	m_locked = true;
	double rot = rotation_angle_->value();
	if (rot != part->property("rotation"))
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "rotation", part->property("rotation"), rot);
		undo->setText(tr("Modifier l'angle d'un champ texte"));
		undo->enableAnimation();
		undoStack().push(undo);
	}
	m_locked= false;
}

void TextEditor::updatePos()
{
	if(m_locked) return;
	m_locked = true;
	QPointF pos(qle_x->value(), qle_y->value());
	if (pos != part->pos())
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "pos", part->pos(), pos);
		undo->setText(tr("Déplacer un champ texte"));
		undo->enableAnimation();
		undoStack().push(undo);
	}
	m_locked= false;
}

/**
	Met a jour le formulaire a partir du champ de texte
*/
void TextEditor::updateForm() {
	if (!part) return;
	activeConnections(false);
	qle_x     -> setValue(part->property("x").toReal());
	qle_y     -> setValue(part->property("y").toReal());
	qle_text  -> setText(part -> property("text").toString());
	font_size -> setValue(part -> property("size").toInt());
	if (QAbstractButton *button = color_ -> button(part -> property("color").toBool())) {
		button -> setChecked(true);
	}
	rotation_angle_ -> setValue(part -> property("rotation").toReal());
	activeConnections(true);
}

void TextEditor::activeConnections(bool active)
{
	if (active)
	{
		connect(qle_x,     SIGNAL(editingFinished()), this, SLOT(updatePos()));
		connect(qle_y,     SIGNAL(editingFinished()), this, SLOT(updatePos()));
		connect(qle_text,  SIGNAL(editingFinished()), this, SLOT(updateTextT()));
		connect(font_size, SIGNAL(editingFinished()), this, SLOT(updateTextS()));
		connect(rotation_angle_, SIGNAL(editingFinished()), this, SLOT(updateTextRotationAngle()));
	}
	else
	{
		disconnect(qle_x,     SIGNAL(editingFinished()), this, SLOT(updatePos()));
		disconnect(qle_y,     SIGNAL(editingFinished()), this, SLOT(updatePos()));
		disconnect(qle_text,  SIGNAL(editingFinished()), this, SLOT(updateTextT()));
		disconnect(font_size, SIGNAL(editingFinished()), this, SLOT(updateTextS()));
		disconnect(rotation_angle_, SIGNAL(editingFinished()), this, SLOT(updateTextRotationAngle()));
	}
}
