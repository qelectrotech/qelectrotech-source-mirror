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
#include "ellipseeditor.h"
#include "ui_ellipseeditor.h"
#include "partellipse.h"
#include "styleeditor.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"

/**
 * @brief EllipseEditor::EllipseEditor
 * @param editor
 * @param part
 * @param parent
 */
EllipseEditor::EllipseEditor(QETElementEditor *editor, PartEllipse *part, QWidget *parent) :
	ElementItemEditor(editor, parent),
	ui(new Ui::EllipseEditor),
	m_part(part)
{
    ui->setupUi(this);
	m_style = new StyleEditor(editor);
	ui->m_main_layout->insertWidget(0, m_style);
	updateForm();
}

/**
 * @brief EllipseEditor::~EllipseEditor
 */
EllipseEditor::~EllipseEditor()
{
	delete ui;
}

/**
 * @brief EllipseEditor::setPart
 * @param part
 * @return
 */
bool EllipseEditor::setPart(CustomElementPart *part)
{
	disconnectChangeConnections();
	if (!part)
	{
		m_part = nullptr;
		return true;
	}
	if (PartEllipse *part_ellipse = dynamic_cast<PartEllipse *>(part))
	{
		if (m_part == part_ellipse) {
			return true;
		}

		m_part = part_ellipse;
		setUpChangeConnections();
		updateForm();
		return true;
	}
	return false;
}

/**
 * @brief EllipseEditor::setParts
 * @param parts
 * @return
 */
bool EllipseEditor::setParts(QList<CustomElementPart *> parts)
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
 * @brief EllipseEditor::currentPart
 * @return
 */
CustomElementPart *EllipseEditor::currentPart() const {
	return m_part;
}

/**
 * @brief EllipseEditor::currentParts
 * @return
 */
QList<CustomElementPart *> EllipseEditor::currentParts() const {
	return m_style->currentParts();
}

/**
 * @brief EllipseEditor::updateForm
 */
void EllipseEditor::updateForm()
{
	if (!m_part) {
		return;
	}

	QRectF rect = m_part->property("rect").toRectF();
	ui->m_x_sb->setValue(m_part->mapToScene(rect.topLeft()).x() + (rect.width()/2));
	ui->m_y_sb->setValue(m_part->mapToScene(rect.topLeft()).y() + (rect.height()/2));
	ui->m_horizontal_diameter_sb->setValue(rect.width());
	ui->m_vertical_diameter_sb->setValue(rect.height());
}

void EllipseEditor::on_m_x_sb_editingFinished()                   { updateRect(); }
void EllipseEditor::on_m_y_sb_editingFinished()                   { updateRect(); }
void EllipseEditor::on_m_horizontal_diameter_sb_editingFinished() { updateRect(); }
void EllipseEditor::on_m_vertical_diameter_sb_editingFinished()   { updateRect(); }

/**
 * @brief EllipseEditor::updateRect
 */
void EllipseEditor::updateRect()
{
	QRectF rect;

	auto pos_ = m_part->mapFromScene(QPointF(ui->m_x_sb->value(), ui->m_y_sb->value()));
	rect.setX(pos_.x() - ui->m_horizontal_diameter_sb->value()/2);
	rect.setY(pos_.y() - ui->m_vertical_diameter_sb->value()/2);
	rect.setWidth(ui->m_horizontal_diameter_sb->value());
	rect.setHeight(ui->m_vertical_diameter_sb->value());

	for (auto part: m_style->currentParts())
	{
		auto ellipse = static_cast<PartEllipse*>(part);
		if (rect != part->property("rect"))
		{
			auto undo= new QPropertyUndoCommand(ellipse, "rect", ellipse->property("rect"), rect);
			undo->setText("Modifier un arc");
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}
}

/**
 * @brief EllipseEditor::setUpChangeConnections
 */
void EllipseEditor::setUpChangeConnections()
{
	m_change_connections << connect(m_part, &PartEllipse::xChanged, this, &EllipseEditor::updateForm);
	m_change_connections << connect(m_part, &PartEllipse::yChanged, this, &EllipseEditor::updateForm);
	m_change_connections << connect(m_part, &PartEllipse::rectChanged, this, &EllipseEditor::updateForm);
}

/**
 * @brief EllipseEditor::disconnectChangeConnections
 */
void EllipseEditor::disconnectChangeConnections()
{
	for (QMetaObject::Connection c : m_change_connections) {
		disconnect(c);
	}
	m_change_connections.clear();
}
