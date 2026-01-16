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
#include <QFontDialog>

#include "terminalstriplayouteditor.h"
#include "ui_terminalstriplayouteditor.h"
#include "../GraphicsItem/properties/terminalstriplayoutpattern.h"

TerminalStripLayoutEditor::TerminalStripLayoutEditor(QSharedPointer<TerminalStripLayoutPattern> layout,
													 QWidget *parent) :
	QWidget{ parent },
	ui{ new Ui::TerminalStripLayoutEditor },
	m_layout{ layout }
{
	ui->setupUi(this);
	ui->m_graphics_view->setScene(new QGraphicsScene{ this });
	ui->m_graphics_view->scene()->addItem(&m_preview_strip_item);
	updateUi();
}

TerminalStripLayoutEditor::~TerminalStripLayoutEditor()
{
	delete ui;
}

void TerminalStripLayoutEditor::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	updatePreview();
}

void TerminalStripLayoutEditor::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	updatePreview();
}

void TerminalStripLayoutEditor::valueEdited()
{
	if (!m_layout || m_ui_updating) {
		return;
	}

    m_layout.data()->m_header_rect.setRect(0,
                                           ui->m_y_header_sb->value(),
                                           ui->m_width_header_sb->value(),
                                           ui->m_height_header_sb->value());

	m_layout.data()->m_spacer_rect.setRect(0,
										   ui->m_y_spacer_sb->value(),
										   ui->m_width_spacer_sb->value(),
										   ui->m_height_spacer_sb->value());

	m_layout.data()->m_terminal_rect[0].setRect(0,
												ui->m_y_terminal_0_sb->value(),
												ui->m_width_terminal_0_sb->value(),
												ui->m_height_terminal_0_sb->value());

	m_layout.data()->m_terminal_rect[1].setRect(0,
												ui->m_y_terminal_1_sb->value(),
												ui->m_width_terminal_1_sb->value(),
												ui->m_height_terminal_1_sb->value());

	m_layout.data()->m_terminal_rect[2].setRect(0,
												ui->m_y_terminal_2_sb->value(),
												ui->m_width_terminal_2_sb->value(),
												ui->m_height_terminal_2_sb->value());

	m_layout.data()->m_terminal_rect[3].setRect(0,
												ui->m_y_terminal_3_sb->value(),
												ui->m_width_terminal_3_sb->value(),
												ui->m_height_terminal_3_sb->value());

	m_layout.data()->m_bridge_point_y_offset[0] = ui->m_bridge_point_0_sb->value();
	m_layout.data()->m_bridge_point_y_offset[1] = ui->m_bridge_point_1_sb->value();
	m_layout.data()->m_bridge_point_y_offset[2] = ui->m_bridge_point_2_sb->value();
	m_layout.data()->m_bridge_point_y_offset[3] = ui->m_bridge_point_3_sb->value();

    auto font_ = ui->m_font_cb->currentFont();
    font_.setPixelSize(ui->m_font_size_sb->value());
    m_layout->setFont(font_);

    m_layout.data()->m_header_text_orientation = ui->m_header_text_orientation_cb->currentIndex() == 0 ?
                                                     Qt::Horizontal :
                                                     Qt::Vertical;

	switch (ui->m_header_text_alignment_cb->currentIndex()) {
		case 0:
			m_layout.data()->setHeaderTextAlignment(Qt::AlignLeft | Qt::AlignVCenter); break;
		case 1:
			m_layout.data()->setHeaderTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); break;
		default:
			m_layout.data()->setHeaderTextAlignment(Qt::AlignRight | Qt::AlignVCenter); break;
	}

		//Terminal text
    m_layout.data()->m_terminals_text_orientation = ui->m_terminal_text_orientation_cb->currentIndex() == 0 ?
                                                        Qt::Horizontal :
                                                        Qt::Vertical;

    switch (ui->m_terminal_text_alignment_cb->currentIndex()) {
        case 0:
            m_layout.data()->setTerminalsTextAlignment(Qt::Alignment {Qt::AlignLeft | Qt::AlignVCenter});
            break;
        case 1:
            m_layout.data()->setTerminalsTextAlignment(Qt::Alignment { Qt::AlignHCenter | Qt::AlignVCenter});
            break;
        default:
            m_layout.data()->setTerminalsTextAlignment(Qt::Alignment { Qt::AlignRight | Qt::AlignVCenter});
            break;
    }

    m_layout.data()->m_terminals_text_y = ui->m_terminal_text_y_sb->value();
    m_layout.data()->m_terminals_text_height = ui->m_terminal_text_height_sb->value();

		//Xref text
	m_layout.data()->m_xref_text_orientation = ui->m_xref_orientation_cb->currentIndex() == 0 ?
														Qt::Horizontal :
														Qt::Vertical;

	switch (ui->m_xref_alignment_cb->currentIndex()) {
	case 0:
		m_layout.data()->setXrefTextAlignment(Qt::Alignment {Qt::AlignLeft | Qt::AlignVCenter});
		break;
	case 1:
		m_layout.data()->setXrefTextAlignment(Qt::Alignment { Qt::AlignHCenter | Qt::AlignVCenter});
		break;
	default:
		m_layout.data()->setXrefTextAlignment(Qt::Alignment { Qt::AlignRight | Qt::AlignVCenter});
		break;
	}

	m_layout.data()->m_xref_text_y = ui->m_xref_y_sb->value();
	m_layout.data()->m_xref_text_height = ui->m_xref_height_sb->value();

	updateUi();
	m_preview_strip_item.update();
}

void TerminalStripLayoutEditor::updateUi()
{
	if (!m_layout) {
		return;
	}

	const auto data = m_layout.data();

	m_ui_updating = true;

	ui->m_y_header_sb->setValue(data->m_header_rect.y());
	ui->m_width_header_sb->setValue(data->m_header_rect.width());
	ui->m_height_header_sb->setValue(data->m_header_rect.height());

	ui->m_y_spacer_sb->setValue(data->m_spacer_rect.y());
	ui->m_width_spacer_sb->setValue(data->m_spacer_rect.width());
	ui->m_height_spacer_sb->setValue(data->m_spacer_rect.height());

	const auto terminal_0 = data->m_terminal_rect[0];
	ui->m_y_terminal_0_sb->setValue(terminal_0.y());
	ui->m_height_terminal_0_sb->setValue(terminal_0.height());
	ui->m_width_terminal_0_sb->setValue(terminal_0.width());

	const auto terminal_1 = data->m_terminal_rect[1];
	ui->m_y_terminal_1_sb->setValue(terminal_1.y());
	ui->m_height_terminal_1_sb->setValue(terminal_1.height());
	ui->m_width_terminal_1_sb->setValue(terminal_1.width());

	const auto terminal_2 = data->m_terminal_rect[2];
	ui->m_y_terminal_2_sb->setValue(terminal_2.y());
	ui->m_height_terminal_2_sb->setValue(terminal_2.height());
	ui->m_width_terminal_2_sb->setValue(terminal_2.width());

	const auto terminal_3 = data->m_terminal_rect[3];
	ui->m_y_terminal_3_sb->setValue(terminal_3.y());
	ui->m_height_terminal_3_sb->setValue(terminal_3.height());
	ui->m_width_terminal_3_sb->setValue(terminal_3.width());

	const auto bridge_point = data->m_bridge_point_y_offset;
	ui->m_bridge_point_0_sb->setValue(bridge_point[0]);
	ui->m_bridge_point_1_sb->setValue(bridge_point[1]);
	ui->m_bridge_point_2_sb->setValue(bridge_point[2]);
	ui->m_bridge_point_3_sb->setValue(bridge_point[3]);

    const auto font = m_layout->font();
    ui->m_font_size_sb->setValue(font.pixelSize());
    ui->m_font_cb->setCurrentFont(font);

    if (data->m_header_text_orientation == Qt::Horizontal) {
        ui->m_header_text_orientation_cb->setCurrentIndex(0);
    } else {
        ui->m_header_text_orientation_cb->setCurrentIndex(1);
    }

    if (data->m_terminals_text_orientation == Qt::Horizontal) {
        ui->m_terminal_text_orientation_cb->setCurrentIndex(0);
    } else {
        ui->m_terminal_text_orientation_cb->setCurrentIndex(1);
    }

	const auto header_alignment = data->headerTextAlignment();
	if (header_alignment &Qt::AlignLeft) {
		ui->m_header_text_alignment_cb->setCurrentIndex(0);
	} else if (header_alignment &Qt::AlignHCenter) {
		ui->m_header_text_alignment_cb->setCurrentIndex(1);
	} else if (header_alignment &Qt::AlignRight) {
		ui->m_header_text_alignment_cb->setCurrentIndex(2);
	}

		//Terminal text
    const auto terminal_alignment = data->terminalsTextAlignment();
    if (terminal_alignment &Qt::AlignLeft) {
        ui->m_terminal_text_alignment_cb->setCurrentIndex(0);
    } else if (terminal_alignment &Qt::AlignHCenter) {
        ui->m_terminal_text_alignment_cb->setCurrentIndex(1);
    } else if (terminal_alignment &Qt::AlignRight) {
        ui->m_terminal_text_alignment_cb->setCurrentIndex(2);
	}

    ui->m_terminal_text_y_sb->setValue(data->m_terminals_text_y);
    ui->m_terminal_text_height_sb->setValue(data->m_terminals_text_height);

		//Xref text
	if (data->m_xref_text_orientation == Qt::Horizontal) {
		ui->m_xref_orientation_cb->setCurrentIndex(0);
	} else {
		ui->m_xref_orientation_cb->setCurrentIndex(1);
	}

	const auto xref_alignment = data->xrefTextAlignment();
	if (xref_alignment &Qt::AlignLeft) {
		ui->m_xref_alignment_cb->setCurrentIndex(0);
	} else if (xref_alignment &Qt::AlignHCenter) {
		ui->m_xref_alignment_cb->setCurrentIndex(1);
	} else if (xref_alignment &Qt::AlignRight) {
		ui->m_xref_alignment_cb->setCurrentIndex(2);
	}

	ui->m_xref_y_sb->setValue(data->m_xref_text_y);
	ui->m_xref_height_sb->setValue(data->m_xref_text_height);

	m_ui_updating = false;
	updatePreview();
}

void TerminalStripLayoutEditor::updatePreview()
{
	ui->m_graphics_view->fitInView(m_preview_strip_item.boundingRect().adjusted(-5,-5,5,5),
								   Qt::KeepAspectRatio);
}

void TerminalStripLayoutEditor::on_m_display_preview_help_clicked(bool checked)
{
	m_preview_strip_item.m_drawer.setPreviewDraw(checked);
	m_preview_strip_item.update();
}
