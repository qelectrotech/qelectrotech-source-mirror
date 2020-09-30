/*
	Copyright 2006-2020 The QElectroTech Team
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
	@brief TerminalEditor::TerminalEditor
	@param editor
	@param parent
*/
TerminalEditor::TerminalEditor(QETElementEditor* editor, QWidget* parent):
	ElementItemEditor(editor, parent) {
	m_part = nullptr;
	m_terminals.clear();
	init();
}

/**
	@brief TerminalEditor::TerminalEditor
	Constructeur
	@param editor :
	L'editeur d'element concerne
	@param terms :
	La borne a editer
	@param parent :
	QWidget parent de ce widget
*/
TerminalEditor::TerminalEditor(
		QETElementEditor *editor,
		QList<PartTerminal *> &terms,
		QWidget *parent) :
	ElementItemEditor(editor, parent),
	m_terminals(terms),
	m_part(terms.first()) {
	init();
}

/**
	@brief TerminalEditor::init
*/
void TerminalEditor::init()
{
	qle_x = new QDoubleSpinBox();
	qle_y = new QDoubleSpinBox();
	name = new QLineEdit();

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

	QHBoxLayout *lay_name = new QHBoxLayout();
	lay_name -> addWidget(new QLabel(tr("Name : ")));
	lay_name -> addWidget(name);
	main_layout -> addLayout(lay_name);

	main_layout -> addStretch();
	setLayout(main_layout);

	activeConnections(true);
	updateForm();
}

/**
	@brief TerminalEditor::~TerminalEditor
	Destructeur
*/
TerminalEditor::~TerminalEditor()
{
}

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur de borne acceptera d'editer la primitive new_part s'il s'agit d'un
	objet de la classe PartTerminal.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool TerminalEditor::setPart(CustomElementPart* new_part) {
	m_terminals.clear();
	if (!new_part) {
		if (m_part) {
			disconnect(m_part, &PartTerminal::orientationChanged, this, &TerminalEditor::updateForm);
		}
		m_part = nullptr;
		return(true);
	}
	if (PartTerminal *part_terminal = static_cast<PartTerminal *>(new_part)) {
		if(m_part == part_terminal) return true;
		if (m_part) {
			disconnect(m_part, &PartTerminal::orientationChanged, this, &TerminalEditor::updateForm);
		}
		m_part = part_terminal;
		updateForm();
		connect(m_part, &PartTerminal::orientationChanged, this, &TerminalEditor::updateForm);
		return(true);
	}
	return(false);
}

bool TerminalEditor::setParts(QList<CustomElementPart *> parts) {
	if (parts.isEmpty()) {
		m_terminals.clear();
		if (m_part) {
			disconnect(m_part, &PartTerminal::orientationChanged, this, &TerminalEditor::updateForm);
		}
		m_part = nullptr;
		return(true);
	}

	if (PartTerminal *part_terminal = static_cast<PartTerminal *>(parts.first())) {
		if (m_part) {
			disconnect(m_part, &PartTerminal::orientationChanged, this, &TerminalEditor::updateForm);
		}
		m_part = part_terminal;
		m_terminals.clear();
		m_terminals.append(part_terminal);
		for (int i=1; i < parts.length(); i++) {
			m_terminals.append(static_cast<PartTerminal*>(parts[i]));
		}
		updateForm();
		connect(m_part, &PartTerminal::orientationChanged, this, &TerminalEditor::updateForm);
		return(true);
	}
	return(false);
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *TerminalEditor::currentPart() const
{
	return(m_part);
}

QList<CustomElementPart*> TerminalEditor::currentParts() const
{
	QList<CustomElementPart*> parts;
	for (auto term: m_terminals) {
		parts.append(static_cast<CustomElementPart*>(term));
	}
	return parts;
}

/// Met a jour l'orientation de la borne et cree un objet d'annulation
void TerminalEditor::updateTerminalO()
{
	if (m_locked) return;
	m_locked = true;
	QVariant var(orientation -> itemData(orientation -> currentIndex()));

	for (int i=0; i < m_terminals.length(); i++) {
		PartTerminal* term = m_terminals[i];
		if (var != term->property("orientation"))
		{
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(term, "orientation", term->property("orientation"), var);
			undo->setText(tr("Modifier l'orientation d'une borne"));
			undoStack().push(undo);
		}
	}
	m_locked = false;
}

/**
	@brief TerminalEditor::updateXPos
*/
void TerminalEditor::updateXPos()
{
	if (m_locked) return;
	m_locked = true;
	QPointF new_pos(qle_x->value(), 0);

	for (int i=0; i < m_terminals.length(); i++) {
		PartTerminal* term = m_terminals[i];
		new_pos.setY(term->pos().y()); // change only x value
		if (term->pos() != new_pos) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(term, "pos", term->property("pos"), new_pos);
			undo->setText(tr("Déplacer une borne"));
			undo->enableAnimation();
			undoStack().push(undo);
		}
	}
	m_locked=false;
}

/**
	@brief TerminalEditor::updateYPos
*/
void TerminalEditor::updateYPos()
{
	if (m_locked) return;
	m_locked = true;
	QPointF new_pos(0, qle_y->value()); // change only y value

	for (int i=0; i < m_terminals.length(); i++) {
		PartTerminal* term = m_terminals[i];
		new_pos.setX(term->pos().x());
		if (term->pos() != new_pos) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(term, "pos", term->property("pos"), new_pos);
			undo->setText(tr("Déplacer une borne"));
			undo->enableAnimation();
			undoStack().push(undo);
		}
	}
	m_locked=false;
}

/**
	@brief TerminalEditor::updateName
	SLOT set name to Terminal
*/
void TerminalEditor::updateName() {
	if (m_locked) return;
	m_locked = true;
	QVariant var(name->text());

	for (int i=0; i < m_terminals.length(); i++) {
		PartTerminal* term = m_terminals[i];
		if (var != term->property("name"))
		{
			QPropertyUndoCommand *undo;
			undo = new QPropertyUndoCommand(term,
							"name",
							term->property("name"),
							var);
			undo->setText(tr("Modifier le nom du terminal"));
			undoStack().push(undo);
		}
	}
	m_locked=false;

}

/// update Number and name, create cancel object

/**
	Met a jour le formulaire d'edition
*/
void TerminalEditor::updateForm()
{
	if (!m_part) return;
	activeConnections(false);
	qle_x -> setValue(m_part->property("x").toReal());
	qle_y -> setValue(m_part->property("y").toReal());
	orientation -> setCurrentIndex(orientation->findData(m_part->property("orientation")));
	name -> setText(m_part->name());
	activeConnections(true);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void TerminalEditor::activeConnections(bool active) {
	if (active) {
		connect(qle_x,
			&QDoubleSpinBox::editingFinished,
			this, &TerminalEditor::updateXPos);
		connect(qle_y,
			&QDoubleSpinBox::editingFinished,
			this, &TerminalEditor::updateYPos);
		connect(orientation,
			QOverload<int>::of(&QComboBox::activated),
			this, &TerminalEditor::updateTerminalO);
		connect(name, &QLineEdit::editingFinished,
			this, &TerminalEditor::updateName);
	}
	else {
		disconnect(qle_x, &QDoubleSpinBox::editingFinished,
			   this, &TerminalEditor::updateXPos);
		disconnect(qle_y, &QDoubleSpinBox::editingFinished,
			   this, &TerminalEditor::updateYPos);
		disconnect(orientation, QOverload<int>::of(&QComboBox::activated),
			   this, &TerminalEditor::updateTerminalO);
		disconnect(name, &QLineEdit::editingFinished,
			   this, &TerminalEditor::updateName);
	}
}
