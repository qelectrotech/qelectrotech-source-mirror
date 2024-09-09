/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "ui_terminaleditor.h"
#include "../../qeticons.h"
#include "../../qet.h"
#include "../graphicspart/partterminal.h"
#include "../../QPropertyUndoCommand/qpropertyundocommand.h"

/**
 * @brief TerminalEditor::TerminalEditor
 * Default constructor
 * @param editor : element editor of which this terminal editor belong
 * @param parent : parent widget
 */
TerminalEditor::TerminalEditor(QETElementEditor *editor, QWidget *parent) :
	ElementItemEditor(editor, parent),
	ui(new Ui::TerminalEditor)
{
	ui->setupUi(this);
	init();
}

/**
 * @brief TerminalEditor::~TerminalEditor
 * Destructor
 */
TerminalEditor::~TerminalEditor()
{
	delete ui;
}

/**
 * @brief TerminalEditor::updateForm
 * Reimplemented from ElementItemEditor
 * Update the content of this widget
 */
void TerminalEditor::updateForm()
{
	if (!m_part) {
		return;
	}
	activeConnections(false);

	ui->m_x_dsb->setValue(m_part->property("x").toReal());
	ui->m_y_dsb->setValue(m_part->property("y").toReal());
	ui->m_orientation_cb->setCurrentIndex(ui->m_orientation_cb->findData(m_part->property("orientation")));
	ui->m_name_le->setText(m_part->terminalName());
	ui->m_type_cb->setCurrentIndex(ui->m_orientation_cb->findData(m_part->terminalType()));

	activeConnections(true);
}

/**
 * @brief TerminalEditor::setPart
 * Set the part to edit.
 * The part must be a PartTerminal, in other case return false.
 * @param new_part : the part to edit
 * @return true if the part can be edited.
 */
bool TerminalEditor::setPart(CustomElementPart *new_part)
{
	if (m_part == new_part) {
		return true;
	}

	activeChangeConnections(false);

	if (!new_part)
	{
		m_part = nullptr;
		return(true);
	}

	if (PartTerminal *part_terminal = dynamic_cast<PartTerminal *>(new_part))
	{
		m_part = part_terminal;
		updateForm();
		activeChangeConnections(true);
		return(true);
	}
	return(false);
}

/**
 * @brief TerminalEditor::currentPart
 * @return the current edited part
 * or nullptr if there is no part or several part
 * @see QList<CustomElementPart *> TerminalEditor::currentParts() const
 */
CustomElementPart *TerminalEditor::currentPart() const
{
	return m_part;
}

/**
 * @brief TerminalEditor::init
 * Some init about this class
 */
void TerminalEditor::init()
{
	ui->m_orientation_cb->addItem(QET::Icons::North, tr("Nord"),  Qet::North);
	ui->m_orientation_cb->addItem(QET::Icons::East,  tr("Est"),   Qet::East);
	ui->m_orientation_cb->addItem(QET::Icons::South, tr("Sud"),   Qet::South);
	ui->m_orientation_cb->addItem(QET::Icons::West,  tr("Ouest"), Qet::West);

	ui->m_type_cb->addItem(tr("Générique"),         TerminalData::Generic);
	ui->m_type_cb->addItem(tr("Bornier intérieur"), TerminalData::Inner);
	ui->m_type_cb->addItem(tr("Bornier extérieur"), TerminalData::Outer);
}

/**
 * @brief TerminalEditor::posEdited
 */
void TerminalEditor::posEdited()
{
	if (m_locked) {
		return;
	}
	m_locked = true;

	QPointF new_pos(ui->m_x_dsb->value(),
					ui->m_y_dsb->value());

	if (m_part->pos() != new_pos)
	{
		auto undo = new QPropertyUndoCommand(m_part, "pos", m_part->property("pos"), new_pos);
		undo->setText(tr("Déplacer une borne"));
		undo->setAnimated(true, false);
		undoStack().push(undo);
	}

	m_locked = false;
}

/**
 * @brief TerminalEditor::orientationEdited
 */
void TerminalEditor::orientationEdited()
{
	if (m_locked) {
		return;
	}
	m_locked = true;

	auto ori_ = ui->m_orientation_cb->currentData();
	if (m_part->orientation() != ori_)
	{
		auto undo = new QPropertyUndoCommand(m_part, "orientation", m_part->property("orientation"), ori_);
		undo->setText(tr("Modifier l'orientation d'une borne"));
		undoStack().push(undo);
	}

	m_locked = false;
}

/**
 * @brief TerminalEditor::nameEdited
 */
void TerminalEditor::nameEdited()
{
	if (m_locked) {
		return;
	}

	m_locked = true;
	QString name_(ui->m_name_le->text());

	if (m_part->terminalName() != name_)
	{
		auto undo = new QPropertyUndoCommand(m_part, "terminal_name", m_part->property("terminal_name"), name_);
		undo->setText(tr("Modifier le nom du terminal"));
		undoStack().push(undo);
	}
	m_locked=false;
}

/**
 * @brief TerminalEditor::typeEdited
 */
void TerminalEditor::typeEdited()
{
	if (m_locked) {
		return;
	}
	m_locked = true;

	auto type = ui->m_type_cb->currentData();
	if (type != m_part->terminalType()) {
		auto undo = new QPropertyUndoCommand(m_part, "terminal_type", m_part->terminalType(), type);
		undo->setText(tr("Modifier le type d'une borne"));
		undoStack().push(undo);
	}
	m_locked = false;
}

/**
 * @brief TerminalEditor::activeConnections
 * Active connection between the widgets used in this editor
 * and method of this class.
 * @param active
 */
void TerminalEditor::activeConnections(bool active)
{
	if (active) {
		m_editor_connections << connect(ui->m_x_dsb, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged),
										[this]() { TerminalEditor::posEdited(); ui->m_x_dsb->setFocus();} ) ;
		m_editor_connections << connect(ui->m_y_dsb, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged),
										[this]() { TerminalEditor::posEdited(); ui->m_y_dsb->setFocus(); } ) ;
		m_editor_connections << connect(ui->m_orientation_cb,  QOverload<int>::of(&QComboBox::activated),
										this, &TerminalEditor::orientationEdited);
		m_editor_connections << connect(ui->m_name_le, &QLineEdit::editingFinished,
										this, &TerminalEditor::nameEdited);
		m_editor_connections << connect(ui->m_type_cb, QOverload<int>::of(&QComboBox::activated),
										this, &TerminalEditor::typeEdited);
	} else {
		for (auto const & con : std::as_const(m_editor_connections)) {
			QObject::disconnect(con);
		}
		m_editor_connections.clear();
	}
}

void TerminalEditor::activeChangeConnections(bool active)
{
	if (active)
	{
		m_change_connections << connect(m_part, &PartTerminal::xChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::yChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::orientationChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::nameChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::terminalTypeChanged, this, &TerminalEditor::updateForm);
	} else {
		for (auto &con : m_change_connections) {
			QObject::disconnect(con);
		}
		m_change_connections.clear();
	}
}
