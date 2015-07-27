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
#include "textfieldeditor.h"
#include "parttextfield.h"
#include "qtextorientationspinboxwidget.h"
#include "qetapp.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param textfield Le champ de texte a editer
	@param parent QWidget parent
*/
TextFieldEditor::TextFieldEditor(QETElementEditor *editor, PartTextField *textfield, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(textfield),
	m_locked(false)
{
	qle_x     = new QDoubleSpinBox();
	qle_y     = new QDoubleSpinBox();
	qle_text  = new QLineEdit();
	qle_text  ->setClearButtonEnabled(true);
	font_size = new QSpinBox();
	font_size -> setRange(0, 144);
	rotate    = new QCheckBox(tr("Ne pas subir les rotations de l'élément parent"));
	rotate -> setChecked(true);
	QLabel *rotation_angle_label = new QLabel(tr("Angle de rotation par défaut : "));
	rotation_angle_label -> setWordWrap(true);
	rotation_angle_ = QETApp::createTextOrientationSpinBoxWidget();
	
	qle_x -> setRange (-1000, 1000);
	qle_y -> setRange (-1000, 1000);
	
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
	t -> addWidget(new QLabel(tr("Texte par défaut : ")));
	t -> addWidget(qle_text);
	main_layout -> addLayout(t);

	//add the tagg combobox
	QHBoxLayout *tagg_layout = new QHBoxLayout();
	tagg_layout -> addWidget(new QLabel(tr("tagg :")));
	tagg_layout -> addWidget(m_tagg_cb = new QComboBox());
	m_tagg_cb -> addItem(tr("Aucun"), QVariant("none"));
	m_tagg_cb -> addItem(tr("label"), QVariant("label"));
	main_layout -> addLayout(tagg_layout);
	
	QHBoxLayout *rotation_angle_layout = new QHBoxLayout();
	rotation_angle_layout -> addWidget(rotation_angle_label);
	rotation_angle_layout -> addWidget(rotation_angle_);
	main_layout -> addLayout(rotation_angle_layout);
	
	QHBoxLayout *r = new QHBoxLayout();
	r -> addWidget(rotate);
	main_layout -> addLayout(r);
	
	main_layout -> addStretch();
	setLayout(main_layout);
	updateForm();
}

/// Destructeur
TextFieldEditor::~TextFieldEditor() {
}

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur de texte dynamique acceptera d'editer la primitive new_part s'il
	s'agit d'un objet de la classe PartTextField.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool TextFieldEditor::setPart(CustomElementPart *new_part)
{
	if (!new_part)
	{
		part = 0;
		return(true);
	}
	if (PartTextField *part_textfield = dynamic_cast<PartTextField *>(new_part))
	{
		if(part == part_textfield) return true;
		part = part_textfield;
		updateForm();
		return(true);
	}
	return(false);
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *TextFieldEditor::currentPart() const {
	return(part);
}

/// Met a jour le texte du champ de texte et cree un objet d'annulation
void TextFieldEditor::updateTextFieldT()
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

/// Met a jour la taille du champ de texte et cree un objet d'annulation
void TextFieldEditor::updateTextFieldS()
{
	if(m_locked) return;
	m_locked = true;
	int size = font_size->value();
	if (size != part->property("size"))
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "size", part->property("size"), size);
		undo->setText(tr("Modifier la taille d'un champ texte"));
		undoStack().push(undo);
	}
	m_locked= false;
}

/// Met a jour la taille du champ de texte et cree un objet d'annulation
void TextFieldEditor::updateTextFieldR()
{
	if(m_locked) return;
	m_locked = true;
	bool rot = !rotate -> isChecked();
	if (rot != part->property("rotate"))
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "rotate", part->property("rotate"), rot);
		undo->setText(tr("Modifier les propriétés d'un champ texte"));
		undoStack().push(undo);
	}
	m_locked= false;
}

/// Met a jour l'angle de rotation du champ de texte et cree un objet d'annulation
void TextFieldEditor::updateTextFieldRotationAngle()
{
	if(m_locked) return;
	m_locked = true;
	double rot = rotation_angle_ -> value();
	if (rot != part->property("rotation_angle"))
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "rotation_angle", part->property("rotation_angle"), rot);
		undo->setText(tr("Modifier l'angle de rotation d'un champ texte"));
		undo->enableAnimation();
		undoStack().push(undo);
	}
	m_locked= false;
}
void TextFieldEditor::updateTagg()
{
	if(m_locked) return;
	m_locked = true;
	QVariant var(m_tagg_cb->itemData(m_tagg_cb->currentIndex()).toString());
	if (var != part->property("tagg"))
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "tagg", part->property("tagg"), var);
		undo->setText(tr("Modifier le tagg d'un champ texte"));
		undoStack().push(undo);
	}
	m_locked= false;
}

void TextFieldEditor::updatePos()
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
 * @brief TextFieldEditor::updateForm
 * Update the value of editor widget
 */
void TextFieldEditor::updateForm() {
	if (!part) return;
	activeConnections(false);

	qle_x           -> setValue        (part      -> property ("x").toReal());
	qle_y           -> setValue        (part      -> property ("y").toReal());
	qle_text        -> setText         (part      -> property ("text").toString());
	font_size       -> setValue        (part      -> property ("size").toInt());
	rotate	        -> setChecked      (!part     -> property ("rotate").toBool());
	rotation_angle_ -> setValue        (part      -> property ("rotation").toDouble());
	m_tagg_cb       -> setCurrentIndex (m_tagg_cb -> findData (part -> property("tagg")));

	activeConnections(true);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void TextFieldEditor::activeConnections(bool active)
{
	if (active)
	{
		connect(qle_x,			 SIGNAL(editingFinished()),		   this, SLOT(updatePos()));
		connect(qle_y,			 SIGNAL(editingFinished()),		   this, SLOT(updatePos()));
		connect(qle_text,		 SIGNAL(editingFinished()),		   this, SLOT(updateTextFieldT()));
		connect(font_size,		 SIGNAL(editingFinished()),		   this, SLOT(updateTextFieldS()));
		connect(rotate,			 SIGNAL(stateChanged(int)),		   this, SLOT(updateTextFieldR()));
		connect(rotation_angle_, SIGNAL(editingFinished()),		   this, SLOT(updateTextFieldRotationAngle()));
		connect(m_tagg_cb,		 SIGNAL(currentIndexChanged(int)), this, SLOT(updateTagg()));
	}
	else
	{
		disconnect(qle_x,			SIGNAL(editingFinished()),		  this, SLOT(updatePos()));
		disconnect(qle_y,			SIGNAL(editingFinished()),		  this, SLOT(updatePos()));
		disconnect(qle_text,		SIGNAL(editingFinished()),		  this, SLOT(updateTextFieldT()));
		disconnect(font_size,		SIGNAL(editingFinished()),		  this, SLOT(updateTextFieldS()));
		disconnect(rotate,			SIGNAL(stateChanged(int)),		  this, SLOT(updateTextFieldR()));
		disconnect(rotation_angle_, SIGNAL(editingFinished()),		  this, SLOT(updateTextFieldRotationAngle()));
		connect(m_tagg_cb,			SIGNAL(currentIndexChanged(int)), this, SLOT(updateTagg()));
	}
}
