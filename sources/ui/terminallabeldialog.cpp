// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "terminallabeldialog.h"
#include "ui_terminallabeldialog.h"

#include "alignmenttextdialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QFontDialog>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>

/**
	@brief TerminalLabelDialog::TerminalLabelDialog
	Show @p label in the widgets, which change it as they are used; the
	dialog itself writes nothing.
	@param label the name and the drawing the widgets start from
	@param parent
*/
TerminalLabelDialog::TerminalLabelDialog(const TerminalLabel &label, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::TerminalLabelDialog),
	m_label(label)
{
	ui->setupUi(this);

#ifdef BUILD_WITHOUT_KF
	m_color_pb = new QPushButton(this);
	m_color_pb->setMinimumSize(40, 24);
	connect(m_color_pb, &QPushButton::clicked, this, &TerminalLabelDialog::colorClicked);
#else
	m_color_pb = new KColorButton(this);
	m_color_pb->setMinimumSize(40, 24);
	connect(m_color_pb, &KColorButton::changed, this, &TerminalLabelDialog::colorClicked);
#endif

	QLayout *layout = ui->m_color_widget->parentWidget()->layout();
	layout->replaceWidget(ui->m_color_widget, m_color_pb);
	delete ui->m_color_widget;
	ui->m_color_widget = nullptr;

	ui->m_font_pb->setText(m_label.m_label_font.family());
	ui->m_label_size_sb->setValue(m_label.m_label_font.pointSize());
	ui->m_label_x_dsb->setValue(m_label.m_label_pos.x());
	ui->m_label_y_dsb->setValue(m_label.m_label_pos.y());
	ui->m_label_rotation_sb->setValue(static_cast<int>(m_label.m_label_rotation));
	ui->m_label_frame_cb->setChecked(m_label.m_label_frame);
#ifdef BUILD_WITHOUT_KF
	QPixmap swatch(16, 16);
	swatch.fill(m_label.m_label_color);
	m_color_pb->setIcon(QIcon(swatch));
#else
	m_color_pb->setColor(m_label.m_label_color);
#endif

	connect(ui->m_font_pb, &QPushButton::clicked,
			this, &TerminalLabelDialog::fontClicked);
	connect(ui->m_label_size_sb, QOverload<int>::of(&QSpinBox::valueChanged),
			this, &TerminalLabelDialog::sizeEdited);
	connect(ui->m_label_x_dsb, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged),
			this, &TerminalLabelDialog::posEdited);
	connect(ui->m_label_y_dsb, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged),
			this, &TerminalLabelDialog::posEdited);
	connect(ui->m_label_rotation_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
			this, &TerminalLabelDialog::rotationEdited);
	connect(ui->m_align_pb, &QPushButton::clicked,
			this, &TerminalLabelDialog::alignClicked);
	connect(ui->m_label_frame_cb, &QCheckBox::toggled, this,
			[this](bool frame) {m_label.m_label_frame = frame;});
	connect(ui->m_button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(ui->m_button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

TerminalLabelDialog::~TerminalLabelDialog()
{
	delete ui;
}

/**
	@brief TerminalLabelDialog::fontClicked
	Ask for a font, as the element editor does: the family changes here,
	its size follows in the spin box, which says nothing itself about it
*/
void TerminalLabelDialog::fontClicked()
{
	bool ok;
	const QFont font = QFontDialog::getFont(&ok, m_label.m_label_font, this, QString(),
											QFontDialog::DontUseNativeDialog);
	if (!ok)
		return;

	m_label.m_label_font = font;
	ui->m_font_pb->setText(font.family());
	ui->m_label_size_sb->blockSignals(true);
	ui->m_label_size_sb->setValue(font.pointSize());
	ui->m_label_size_sb->blockSignals(false);
}

void TerminalLabelDialog::sizeEdited()
{
	m_label.m_label_font.setPointSize(ui->m_label_size_sb->value());
}

void TerminalLabelDialog::posEdited()
{
	m_label.m_label_pos = QPointF(ui->m_label_x_dsb->value(),
								  ui->m_label_y_dsb->value());
}

void TerminalLabelDialog::rotationEdited()
{
	m_label.m_label_rotation = ui->m_label_rotation_sb->value();
}

void TerminalLabelDialog::alignClicked()
{
	const Qt::Alignment alignment = m_label.m_label_halignment | m_label.m_label_valignment;
	AlignmentTextDialog dialog(alignment, this);
	if (dialog.exec() != QDialog::Accepted)
		return;

	const Qt::Alignment chosen = dialog.alignment();
	m_label.m_label_halignment = chosen & Qt::AlignHorizontal_Mask;
	m_label.m_label_valignment = chosen & Qt::AlignVertical_Mask;
}

void TerminalLabelDialog::colorClicked()
{
#ifdef BUILD_WITHOUT_KF
	const QColor color = QColorDialog::getColor(m_label.m_label_color, this, QString(),
												QColorDialog::DontUseNativeDialog);
	if (!color.isValid())
		return;
	m_label.m_label_color = color;
	QPixmap swatch(16, 16);
	swatch.fill(color);
	m_color_pb->setIcon(QIcon(swatch));
#else
	m_label.m_label_color = m_color_pb->color();
#endif
}
