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
#include "terminaleditor.h"
#include "partterminal.h"
#include "qeticons.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param term La borne a editer
	@param parent QWidget parent de ce widget
*/
TerminalEditor::TerminalEditor(QETElementEditor *editor, PartTerminal *term, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(term),
	m_locked(false)
{
	qle_x = new QDoubleSpinBox();
	qle_y = new QDoubleSpinBox();
	
	qle_x -> setRange(-5000, 5000);
	qle_y -> setRange(-5000, 5000);
	
	orientation = new QComboBox();
	orientation -> addItem(QET::Icons::North, tr("Nord"),  Qet::North);
	orientation -> addItem(QET::Icons::East,  tr("Est"),   Qet::East);
	orientation -> addItem(QET::Icons::South, tr("Sud"),   Qet::South);
	orientation -> addItem(QET::Icons::West,  tr("Ouest"), Qet::West);
	
	QVBoxLayout *main_layout = new QVBoxLayout();
	main_layout -> addWidget(new QLabel(tr("Position : ")));
	
	QHBoxLayout *position = new QHBoxLayout();
	position -> addWidget(new QLabel(tr("x : ")));
	position -> addWidget(qle_x                 );
	position -> addWidget(new QLabel(tr("y : ")));
	position -> addWidget(qle_y                 );
	main_layout -> addLayout(position);
	
	QHBoxLayout *ori = new QHBoxLayout();
	ori -> addWidget(new QLabel(tr("Orientation : ")));
	ori -> addWidget(orientation                     );
	main_layout -> addLayout(ori);
	
	main_layout -> addStretch();
	setLayout(main_layout);
	
	activeConnections(true);
	updateForm();
}

/// Destructeur
TerminalEditor::~TerminalEditor() {
};

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur de borne acceptera d'editer la primitive new_part s'il s'agit d'un
	objet de la classe PartTerminal.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool TerminalEditor::setPart(CustomElementPart *new_part)
{
	if (!new_part)
	{
		if (part)
			disconnect(part, &PartTerminal::orientationChanged, this, &TerminalEditor::updateForm);
		part = 0;
		return(true);
	}
	if (PartTerminal *part_terminal = dynamic_cast<PartTerminal *>(new_part))
	{
		if(part == part_terminal) return true;
		if (part)
			disconnect(part, &PartTerminal::orientationChanged, this, &TerminalEditor::updateForm);
		part = part_terminal;
		updateForm();
		connect(part, &PartTerminal::orientationChanged, this, &TerminalEditor::updateForm);
		return(true);
	}
	return(false);
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *TerminalEditor::currentPart() const {
	return(part);
}

/// Met a jour l'orientation de la borne et cree un objet d'annulation
void TerminalEditor::updateTerminalO()
{
	if (m_locked) return;
	m_locked = true;
	QVariant var(orientation -> itemData(orientation -> currentIndex()));
	if (var != part->property("orientation"))
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "orientation", part->property("orientation"), var);
		undo->setText(tr("Modifier l'orientation d'une borne"));
		undoStack().push(undo);
	}
	m_locked = false;
}

void TerminalEditor::updatePos()
{
	if (m_locked) return;
	m_locked = true;
	QPointF new_pos(qle_x->value(), qle_y->value());
	if (new_pos != part->pos())
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "pos", part->property("pos"), new_pos);
		undo->setText(tr("Déplacer une borne"));
		undo->enableAnimation();
		undoStack().push(undo);
	}
	m_locked=false;
}
/// update Number and name, create cancel object

/**
	Met a jour le formulaire d'edition
*/
void TerminalEditor::updateForm() {
	if (!part) return;
	activeConnections(false);
	qle_x -> setValue(part->property("x").toReal());
	qle_y -> setValue(part->property("y").toReal());
	orientation -> setCurrentIndex(orientation->findData(part->property("orientation")));
	activeConnections(true);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void TerminalEditor::activeConnections(bool active)
{
	if (active)
	{
		connect(qle_x,       SIGNAL(editingFinished()), this, SLOT(updatePos()));
		connect(qle_y,       SIGNAL(editingFinished()), this, SLOT(updatePos()));
		connect(orientation, SIGNAL(activated(int)),    this, SLOT(updateTerminalO()));
	}
	else
	{
		disconnect(qle_x,       SIGNAL(editingFinished()), this, SLOT(updatePos()));
		disconnect(qle_y,       SIGNAL(editingFinished()), this, SLOT(updatePos()));
		disconnect(orientation, SIGNAL(activated(int)),    this, SLOT(updateTerminalO()));
	}
}
