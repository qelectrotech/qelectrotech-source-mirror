/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "../../ui/alignmenttextdialog.h"

#include <QColorDialog>
#include <QFontDialog>

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

#ifdef BUILD_WITHOUT_KF5
	m_color_pb = new QPushButton(this);
	m_color_pb->setMinimumSize(40, 24);
	connect(m_color_pb, &QPushButton::clicked, this, &TerminalEditor::labelColorClicked);
#else
	m_color_pb = new KColorButton(this);
	m_color_pb->setMinimumSize(40, 24);
	connect(m_color_pb, &KColorButton::changed, this, &TerminalEditor::labelColorClicked);
#endif

	QLayout *layout = ui->m_color_widget->parentWidget()->layout();
	layout->replaceWidget(ui->m_color_widget, m_color_pb);
	delete ui->m_color_widget;
	ui->m_color_widget = nullptr;

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
	ui->m_type_cb->setCurrentIndex(ui->m_type_cb->findData(m_part->terminalType()));

	ui->m_show_name_cb->setChecked(m_part->showName());
	ui->m_label_x_dsb->setValue(m_part->labelPos().x());
	ui->m_label_y_dsb->setValue(m_part->labelPos().y());
	ui->m_font_pb->setText(m_part->labelFont().family());
	ui->m_label_size_sb->setValue(m_part->labelFont().pointSize());
	ui->m_label_rotation_sb->setValue(static_cast<int>(m_part->labelRotation()));
	ui->m_label_frame_cb->setChecked(m_part->labelFrame());

#ifdef BUILD_WITHOUT_KF5
	QPixmap px(16, 16);
	px.fill(m_part->labelColor());
	m_color_pb->setIcon(QIcon(px));
#else
	m_color_pb->setColor(m_part->labelColor());
#endif

	ui->m_text_props_gb->setEnabled(m_part->showName());

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
	ui->m_type_cb->addItem(tr("NO (contact SW)"),   TerminalData::No);
	ui->m_type_cb->addItem(tr("NC (contact SW)"),   TerminalData::Nc);
	ui->m_type_cb->addItem(tr("Commun (contact SW)"), TerminalData::Common);

	ui->m_text_props_gb->setEnabled(false);
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

void TerminalEditor::showNameEdited()
{
	if (m_locked) return;
	m_locked = true;

	bool show = ui->m_show_name_cb->isChecked();
	if (m_part->showName() != show) {
		auto undo = new QPropertyUndoCommand(m_part, "show_name", m_part->showName(), show);
		undo->setText(tr("Afficher/cacher le nom du terminal"));
		undoStack().push(undo);
	}
	ui->m_text_props_gb->setEnabled(show);
	m_locked = false;
}

void TerminalEditor::labelPosEdited()
{
	if (m_locked) return;
	m_locked = true;

	QPointF new_pos(ui->m_label_x_dsb->value(), ui->m_label_y_dsb->value());
	if (m_part->labelPos() != new_pos) {
		auto undo = new QPropertyUndoCommand(m_part, "label_pos", m_part->labelPos(), new_pos);
		undo->setText(tr("Modifier la position du label"));
		undoStack().push(undo);
	}
	m_locked = false;
}

void TerminalEditor::labelFontClicked()
{
	if (m_locked) return;
	m_locked = true;

	bool ok;
	QFont font = QFontDialog::getFont(&ok, m_part->labelFont(), this);
	if (ok && font != m_part->labelFont()) {
		ui->m_font_pb->setText(font.family());
		ui->m_label_size_sb->blockSignals(true);
		ui->m_label_size_sb->setValue(font.pointSize());
		ui->m_label_size_sb->blockSignals(false);

		auto undo = new QPropertyUndoCommand(m_part, "label_font", m_part->labelFont(), font);
		undo->setText(tr("Modifier la police du label"));
		undoStack().push(undo);
	}
	m_locked = false;
}

void TerminalEditor::labelSizeEdited()
{
	if (m_locked) return;
	m_locked = true;

	QFont new_font = m_part->labelFont();
	new_font.setPointSize(ui->m_label_size_sb->value());
	if (m_part->labelFont() != new_font) {
		auto undo = new QPropertyUndoCommand(m_part, "label_font", m_part->labelFont(), new_font);
		undo->setText(tr("Modifier la taille de police du label"));
		undoStack().push(undo);
	}
	m_locked = false;
}

void TerminalEditor::labelRotationEdited()
{
	if (m_locked) return;
	m_locked = true;

	qreal rot = static_cast<qreal>(ui->m_label_rotation_sb->value());
	if (!qFuzzyCompare(m_part->labelRotation(), rot)) {
		auto undo = new QPropertyUndoCommand(m_part, "label_rotation", m_part->labelRotation(), rot);
		undo->setText(tr("Modifier la rotation du label"));
		undoStack().push(undo);
	}
	m_locked = false;
}

void TerminalEditor::labelAlignClicked()
{
	Qt::Alignment align = m_part->labelHAlignment() | m_part->labelVAlignment();
	AlignmentTextDialog dialog(align, this);
	if (dialog.exec() == QDialog::Accepted) {
		Qt::Alignment new_align = dialog.alignment();
		Qt::Alignment new_h = new_align & Qt::AlignHorizontal_Mask;
		Qt::Alignment new_v = new_align & Qt::AlignVertical_Mask;

		if (new_h != m_part->labelHAlignment()) {
			auto undo = new QPropertyUndoCommand(m_part, "label_halignment",
				QVariant::fromValue(m_part->labelHAlignment()), QVariant::fromValue(new_h));
			undo->setText(tr("Modifier l'alignement du label"));
			undoStack().push(undo);
		}
		if (new_v != m_part->labelVAlignment()) {
			auto undo = new QPropertyUndoCommand(m_part, "label_valignment",
				QVariant::fromValue(m_part->labelVAlignment()), QVariant::fromValue(new_v));
			undo->setText(tr("Modifier l'alignement du label"));
			undoStack().push(undo);
		}
	}
}

void TerminalEditor::labelFrameEdited()
{
	if (m_locked) return;
	m_locked = true;

	bool frame = ui->m_label_frame_cb->isChecked();
	if (m_part->labelFrame() != frame) {
		auto undo = new QPropertyUndoCommand(m_part, "label_frame", m_part->labelFrame(), frame);
		undo->setText(tr("Afficher/cacher le cadre du label"));
		undoStack().push(undo);
	}
	m_locked = false;
}

void TerminalEditor::labelColorClicked()
{
	if (m_locked) return;
	m_locked = true;

#ifdef BUILD_WITHOUT_KF5
	QColor new_color = QColorDialog::getColor(m_part->labelColor(), this);
	if (new_color.isValid() && m_part->labelColor() != new_color) {
		auto undo = new QPropertyUndoCommand(m_part, "label_color", m_part->labelColor(), new_color);
		undo->setText(tr("Modifier la couleur du label"));
		undoStack().push(undo);
	}
#else
	QColor new_color = m_color_pb->color();
	if (new_color.isValid() && m_part->labelColor() != new_color) {
		auto undo = new QPropertyUndoCommand(m_part, "label_color", m_part->labelColor(), new_color);
		undo->setText(tr("Modifier la couleur du label"));
		undoStack().push(undo);
	}
#endif
	m_locked = false;
}

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
		m_editor_connections << connect(ui->m_show_name_cb, &QCheckBox::toggled,
										this, &TerminalEditor::showNameEdited);
		m_editor_connections << connect(ui->m_label_x_dsb, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged),
										[this]() { TerminalEditor::labelPosEdited(); ui->m_label_x_dsb->setFocus(); });
		m_editor_connections << connect(ui->m_label_y_dsb, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged),
										[this]() { TerminalEditor::labelPosEdited(); ui->m_label_y_dsb->setFocus(); });
		m_editor_connections << connect(ui->m_font_pb, &QPushButton::clicked,
										this, &TerminalEditor::labelFontClicked);
		m_editor_connections << connect(ui->m_label_size_sb, QOverload<int>::of(&QSpinBox::valueChanged),
										[this]() { TerminalEditor::labelSizeEdited(); ui->m_label_size_sb->setFocus(); });
		m_editor_connections << connect(ui->m_label_rotation_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
										[this]() { TerminalEditor::labelRotationEdited(); ui->m_label_rotation_sb->setFocus(); });
		m_editor_connections << connect(ui->m_align_pb, &QPushButton::clicked,
										this, &TerminalEditor::labelAlignClicked);
		m_editor_connections << connect(ui->m_label_frame_cb, &QCheckBox::toggled,
										this, &TerminalEditor::labelFrameEdited);
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
		m_change_connections << connect(m_part, &PartTerminal::showNameChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::labelPosChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::labelFontChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::labelRotationChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::labelHAlignmentChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::labelVAlignmentChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::labelFrameChanged, this, &TerminalEditor::updateForm);
		m_change_connections << connect(m_part, &PartTerminal::labelColorChanged, this, &TerminalEditor::updateForm);
	} else {
		for (auto &con : m_change_connections) {
			QObject::disconnect(con);
		}
		m_change_connections.clear();
	}
}
