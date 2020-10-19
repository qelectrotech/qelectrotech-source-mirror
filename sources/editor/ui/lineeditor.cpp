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
#include "lineeditor.h"
#include "ui_lineeditor.h"
#include "elementitemeditor.h"
#include "styleeditor.h"
#include "partline.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"
#include "qeticons.h"

/**
	@brief LineEditor::LineEditor
	@param editor : Element editor who belong this editor
	@param part : part line to edit
	@param parent : parent widget
*/
LineEditor::LineEditor(
		QETElementEditor *editor, PartLine *part, QWidget *parent) :
	ElementItemEditor(editor, parent),ui(new Ui::LineEditor)
{
	ui->setupUi(this);

	ui->m_end1_cb->addItem(
				QET::Icons::EndLineNone,
				tr("Normale",
				   "type of the 1st end of a line"),
				Qet::None    );
	ui->m_end1_cb->addItem(
				QET::Icons::EndLineSimple,
				tr("Flèche simple",
				   "type of the 1st end of a line"),
				Qet::Simple  );
	ui->m_end1_cb->addItem(
				QET::Icons::EndLineTriangle,
				tr("Flèche triangulaire",
				   "type of the 1st end of a line"),
				Qet::Triangle);
	ui->m_end1_cb->addItem(
				QET::Icons::EndLineCircle,
				tr("Cercle",
				   "type of the 1st end of a line"),
				Qet::Circle  );
	ui->m_end1_cb->addItem(
				QET::Icons::EndLineDiamond,
				tr("Carré",
				   "type of the 1st end of a line"),
				Qet::Diamond );

	ui->m_end2_cb->addItem(
				QET::Icons::EndLineNone,
				tr("Normale",
				   "type of the 1st end of a line"),
				Qet::None    );
	ui->m_end2_cb->addItem(
				QET::Icons::EndLineSimple,
				tr("Flèche simple",
				   "type of the 1st end of a line"),
				Qet::Simple  );
	ui->m_end2_cb->addItem(
				QET::Icons::EndLineTriangle,
				tr("Flèche triangulaire",
				   "type of the 1st end of a line"),
				Qet::Triangle);
	ui->m_end2_cb->addItem(
				QET::Icons::EndLineCircle,
				tr("Cercle",
				   "type of the 1st end of a line"),
				Qet::Circle  );
	ui->m_end2_cb->addItem(
				QET::Icons::EndLineDiamond,
				tr("Carré",
				   "type of the 1st end of a line"),
				Qet::Diamond );

	m_style = new StyleEditor(editor);
	ui->m_main_layout->insertWidget(0, m_style);
	if (part) {
		setPart(part);
	}
}

/**
	@brief LineEditor::~LineEditor
*/
LineEditor::~LineEditor()
{
	delete ui;
}

/**
	@brief LineEditor::setPart
	Reimplemented from ElementItemEditor
	@param part : part line to edit
	@return : true if the given part can be edited
*/
bool LineEditor::setPart(CustomElementPart *part)
{
	if (m_part == part) {
		return true;
	}

	if (!part)
	{
		if (m_part) {
			disconnectChangeConnections();
		}

		m_part = nullptr;
		return true;
	}

	if (PartLine *part_line = dynamic_cast<PartLine *>(part))
	{
		if (m_part) {
			disconnectChangeConnections();
		}
		m_part = part_line;
		updateForm();
		setUpChangeConnections();
		return true;
	}

	return false;
}

/**
 * @brief LineEditor::setParts
 * Reimplemented from ElementItemEditor
 * @param parts : parts to edit
 * @return true if the parts can be edited
 */
bool LineEditor::setParts(QList<CustomElementPart *> parts)
{
	if (parts.isEmpty()) {
		return false;
	}

	if (!setPart(parts.first())) {
		return false;
	}

	return m_style->setParts(parts);
}

/**
 * @brief LineEditor::currentPart
 * @return the current edited part or nullptr
 */
CustomElementPart *LineEditor::currentPart() const {
	return m_part;
}

/**
 * @brief LineEditor::currentParts
 * @return A QList of edited parts
 */
QList<CustomElementPart *> LineEditor::currentParts() const {
	return m_style->currentParts();
}

/**
	@brief LineEditor::updateForm
	Reimplemented from ElementItemEditor
*/
void LineEditor::updateForm()
{
	if (!m_part) {
		return;
	}

	activeConnection(false);

	QLineF line(m_part->sceneP1(),
				m_part->sceneP2());
	ui->m_x1_sb->setValue(line.x1());
	ui->m_y1_sb->setValue(line.y1());
	ui->m_x2_sb->setValue(line.x2());
	ui->m_y2_sb->setValue(line.y2());
	ui->m_end1_cb->setCurrentIndex(ui->m_end1_cb->findData(m_part->firstEndType()));
	ui->m_end2_cb->setCurrentIndex(ui->m_end2_cb->findData(m_part->secondEndType()));
	ui->m_lenght1_sb->setValue(m_part->firstEndLength());
	ui->m_lenght2_sb->setValue(m_part->secondEndLength());

	activeConnection(true);
}

/**
 * @brief LineEditor::setUpChangeConnections
 * Make the connection use for update the form when line is edited out of this editor
 */
void LineEditor::setUpChangeConnections()
{
	m_change_connections << connect(m_part, &PartLine::xChanged,               this, &LineEditor::updateForm);
	m_change_connections << connect(m_part, &PartLine::yChanged,               this, &LineEditor::updateForm);
	m_change_connections << connect(m_part, &PartLine::lineChanged,            this, &LineEditor::updateForm);
	m_change_connections << connect(m_part, &PartLine::firstEndTypeChanged,  this, &LineEditor::updateForm);
	m_change_connections << connect(m_part, &PartLine::firstEndLengthChanged,  this, &LineEditor::updateForm);
	m_change_connections << connect(m_part, &PartLine::secondEndTypeChanged,   this, &LineEditor::updateForm);
	m_change_connections << connect(m_part, &PartLine::secondEndLengthChanged, this, &LineEditor::updateForm);
}

/**
 * @brief LineEditor::disconnectChangeConnections
 * disconnect the connection
 * use for update the form when line is edited out of this editor
 */
void LineEditor::disconnectChangeConnections()
{
	for (auto connection : m_change_connections) {
		disconnect(connection);
	}

	m_change_connections.clear();
}

/**
 * @brief LineEditor::activeConnection
 * Active or not the connection between the signals from widgets of this editor and the slots
 * @param active
 */
void LineEditor::activeConnection(bool active)
{
	if (active)
	{
		connect(ui->m_x1_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::lineEdited);
		connect(ui->m_y1_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::lineEdited);
		connect(ui->m_x2_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::lineEdited);
		connect(ui->m_y2_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::lineEdited);
		connect(ui->m_end1_cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LineEditor::firstEndEdited);
		connect(ui->m_end2_cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LineEditor::secondEndEdited);
		connect(ui->m_lenght1_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::firstEndLenghtEdited);
		connect(ui->m_lenght2_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::secondEndLenghtEdited);
	}
	else
	{
		disconnect(ui->m_x1_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::lineEdited);
		disconnect(ui->m_y1_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::lineEdited);
		disconnect(ui->m_x2_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::lineEdited);
		disconnect(ui->m_y2_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::lineEdited);
		disconnect(ui->m_end1_cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LineEditor::firstEndEdited);
		disconnect(ui->m_end2_cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LineEditor::secondEndEdited);
		disconnect(ui->m_lenght1_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::firstEndLenghtEdited);
		disconnect(ui->m_lenght2_sb, &QDoubleSpinBox::editingFinished, this, &LineEditor::secondEndLenghtEdited);
	}
}

/**
 * @brief LineEditor::lineEdited
 * slot
 */
void LineEditor::lineEdited()
{
	if (m_locked) {
		return;
	}
	m_locked = true;

	QPointF p1(ui->m_x1_sb->value(), ui->m_y1_sb->value());
	QPointF p2(ui->m_x2_sb->value(), ui->m_y2_sb->value());
	QLineF line(m_part->mapFromScene(p1), m_part->mapFromScene(p2));

	if (line != m_part->line()) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_part, "line", m_part->line(), line);
		undo->setAnimated();
		elementScene()->undoStack().push(undo);
	}

	m_locked = false;
}

/**
 * @brief LineEditor::firstEndEdited
 * slot
 */
void LineEditor::firstEndEdited()
{
	if (m_locked) {
		return;
	}
	m_locked = true;

	QVariant end = ui->m_end1_cb->itemData(ui->m_end1_cb->currentIndex());

	for (auto part: m_style->currentParts())
	{
		PartLine* line = static_cast<PartLine*>(part);

		if (end != line->property("end1"))
		{
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(line, "end1", line->property("end1"), end);
			undo->setText(tr("Modifier une ligne"));
			elementScene()->undoStack().push(undo);
		}
	}

	m_locked = false;
}

/**
 * @brief LineEditor::firstEndLenghtEdited
 * slot
 */
void LineEditor::firstEndLenghtEdited()
{
	if (m_locked) {
		return;
	}
	m_locked = true;

	if (!qFuzzyCompare(ui->m_lenght1_sb->value(), m_part->firstEndLength())) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_part, "length1", m_part->firstEndLength(), ui->m_lenght1_sb->value());
		undo->setAnimated();
		elementScene()->undoStack().push(undo);
	}

	m_locked = false;
}

/**
 * @brief LineEditor::secondEndEdited
 * slot
 */
void LineEditor::secondEndEdited()
{
	if (m_locked) {
		return;
	}
	m_locked = true;

	QVariant end = ui->m_end2_cb->itemData(ui->m_end2_cb->currentIndex());

	for (auto part: m_style->currentParts())
	{
		PartLine* line = static_cast<PartLine*>(part);

		if (end != line->property("end2"))
		{
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(line, "end2", line->property("end2"), end);
			undo->setText(tr("Modifier une ligne"));
			elementScene()->undoStack().push(undo);
		}
	}

	m_locked = false;
}

/**
 * @brief LineEditor::secondEndLenghtEdited
 * slot
 */
void LineEditor::secondEndLenghtEdited()
{
	if (m_locked) {
		return;
	}
	m_locked = true;

	if (!qFuzzyCompare(ui->m_lenght2_sb->value(), m_part->secondEndLength())) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_part, "length2", m_part->secondEndLength(), ui->m_lenght2_sb->value());
		undo->setAnimated();
		elementScene()->undoStack().push(undo);
	}

	m_locked = false;
}
