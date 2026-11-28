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
#include "inditextpropertieswidget.h"

#include <QPropertyUndoCommand/qpropertyundocommand.h>
#include <diagram.h>
#include <diagramcommands.h>
#include <lastusedstyle.h>
#include <qetgraphicsitem/independenttextitem.h>
#include <ui_inditextpropertieswidget.h>
#include <utils/textsizebox.h>

#include <QLineEdit>
#include <QtGlobal>

/**
	@return width as shown in the width spin box: whole pixels, 0 for the
	automatic width ("Auto"). The box starts at 0, so no number means Auto
	and the first step up is 1 px; IndependentTextItem::setTextWidth()
	treats 0 as -1, the automatic width, so the two never disagree.
*/
static int spinBoxWidth(qreal width)
{
	return width > 0 ? qRound(width) : 0;
}

/**
	@brief IndiTextPropertiesWidget::sizePxPerMm
	@return the pixels per millimetre the size box was last filled with,
	0 if it shows points. The box is read in that unit, even if the scale
	or the module of the project changed since: the number in it still
	means what it did when it was shown (updateUi() fills it again).
*/
qreal IndiTextPropertiesWidget::sizePxPerMm() const
{
	return m_size_px_per_mm;
}

/**
	@brief IndiTextPropertiesWidget::projectSizePxPerMm
	@return the pixels per millimetre of the project if the text size
	is given as a capital height in mm (the edited texts are on a folio
	of a project drawn to scale) now, else 0
*/
qreal IndiTextPropertiesWidget::projectSizePxPerMm() const
{
	const IndependentTextItem *text = m_text ? m_text.data()
		: (m_text_list.isEmpty() ? nullptr : m_text_list.first().data());
	return text && text->diagram()
		? TextSizeBox::pxPerMm(text->diagram()->project()) : 0;
}

/**
	@brief IndiTextPropertiesWidget::sizeFont
	@return the font whose cap height gives the size in mm
*/
QFont IndiTextPropertiesWidget::sizeFont() const
{
	if (m_font_is_selected)
		return m_selected_font;
	if (m_text)
		return m_text->font();
	return m_text_list.isEmpty() ? QFont() : m_text_list.first()->font();
}

/**
	@brief IndiTextPropertiesWidget::shownSize
	@return the number the size box shows for \p text
*/
qreal IndiTextPropertiesWidget::shownSize(const IndependentTextItem *text, qreal px_per_mm)
{
	return TextSizeBox::shown(px_per_mm, text->font().pointSizeF(), text->font());
}

/**
	@brief IndiTextPropertiesWidget::IndiTextPropertiesWidget
	@param text : the text to edit
	@param parent : the parent widget of this widget
*/
IndiTextPropertiesWidget::IndiTextPropertiesWidget(IndependentTextItem *text, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::IndiTextPropertiesWidget)
{
	ui->setupUi(this);
	if (text) {
		setText(text);
	}
}

/**
	@brief IndiTextPropertiesWidget::IndiTextPropertiesWidget
	@param text_list : a list of texts to edit
	@param parent : the parent widget of this widget
*/
IndiTextPropertiesWidget::IndiTextPropertiesWidget(
		QList<IndependentTextItem *> text_list,
		QWidget *parent) :
	PropertiesEditorWidget (parent),
	ui(new Ui::IndiTextPropertiesWidget)
{
	ui->setupUi(this);
	setText(text_list);
}

/**
	@brief IndiTextPropertiesWidget::~IndiTextPropertiesWidget
*/
IndiTextPropertiesWidget::~IndiTextPropertiesWidget()
{
	delete  ui;
}

/**
	@brief IndiTextPropertiesWidget::setText
	@param text : set text as edited text
*/
void IndiTextPropertiesWidget::setText(IndependentTextItem *text)
{
	if (m_text) {
		for (QMetaObject::Connection c : m_connect_list) {
			disconnect(c);
		}
	}
	
	m_text = text;
	m_connect_list.clear();
	m_connect_list << connect(m_text.data(), &IndependentTextItem::xChanged, this, &IndiTextPropertiesWidget::updateUi);
	m_connect_list << connect(m_text.data(), &IndependentTextItem::yChanged, this, &IndiTextPropertiesWidget::updateUi);
	m_connect_list << connect(m_text.data(), &IndependentTextItem::rotationChanged, this, &IndiTextPropertiesWidget::updateUi);
	m_connect_list << connect(m_text.data(), &IndependentTextItem::fontChanged, this, &IndiTextPropertiesWidget::updateUi);
	m_connect_list << connect(m_text.data(), &IndependentTextItem::textEdited, this, &IndiTextPropertiesWidget::updateUi);
	m_connect_list << connect(m_text.data(), &IndependentTextItem::textWidthChanged, this, &IndiTextPropertiesWidget::updateUi);

	updateUi();
}

void IndiTextPropertiesWidget::setText(QList<IndependentTextItem *> text_list)
{
	for (QMetaObject::Connection c : m_connect_list) {
		disconnect(c);
	}
	m_connect_list.clear();
	m_text_list.clear();
	m_text = nullptr;
	
	if (text_list.size() == 0) {
		updateUi();
	}
	else if (text_list.size() == 1) 
	{
		setText(text_list.first());
		m_text_list.clear();
	}
	else
	{
		for (IndependentTextItem *iti : text_list) {
			m_text_list.append(QPointer<IndependentTextItem>(iti));
		}
		updateUi();
	}
}

/**
	@brief IndiTextPropertiesWidget::apply
	Apply the current edition through a QUndoCommand pushed
	to the undo stack of text's diagram.
*/
void IndiTextPropertiesWidget::apply()
{
	Diagram *d = nullptr;
	
	if (m_text && m_text->diagram()) {
		d = m_text->diagram();
	} else if (!m_text_list.isEmpty()) {
		for (QPointer<IndependentTextItem> piti : m_text_list) {
			if (piti->diagram()) {
				d = piti->diagram();
				break;
			}
		}
	}
	
	if (d)
	{
		QUndoCommand *undo = associatedUndo();
		if (undo) {
			d->undoStack().push(undo);
		}
	}
}

/**
	@brief IndiTextPropertiesWidget::setLiveEdit
	@param live_edit
	@return 
*/
bool IndiTextPropertiesWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) {
		return true;
	}
	m_live_edit = live_edit;
	
	if (m_live_edit) {
		setUpEditConnection();
	}
	else {
		for (QMetaObject::Connection c : m_edit_connection) {
			disconnect(c);
		}
		m_edit_connection.clear();
	}
	return true;
}

/**
	@brief IndiTextPropertiesWidget::associatedUndo
	@return 
*/
QUndoCommand *IndiTextPropertiesWidget::associatedUndo() const
{
	if (m_live_edit)
	{
		QPropertyUndoCommand *undo = nullptr;
			//One text is edited
		if (m_text_list.isEmpty())
		{
			if(ui->m_x_sb->value() != m_text->pos().x()) {
				undo = new QPropertyUndoCommand(m_text.data(), "x", QVariant(m_text->pos().x()), QVariant(ui->m_x_sb->value()));
				undo->setAnimated(true, false);
				undo->setText(tr("Move a text field"));
			}
			if(ui->m_y_sb->value() != m_text->pos().y()) {
				undo = new QPropertyUndoCommand(m_text.data(), "y", QVariant(m_text->pos().y()), QVariant(ui->m_y_sb->value()));
				undo->setAnimated(true, false);
				undo->setText(tr("Move a text field"));
			}
			if(ui->m_angle_sb->value() != m_text->rotation()) {
				undo = new QPropertyUndoCommand(m_text.data(), "rotation", QVariant(m_text->rotation()), QVariant(ui->m_angle_sb->value()));
				undo->setAnimated(true, false);
				undo->setText(tr("Rotate a text field"));
			}
			if (ui->m_line_edit->text() != m_text->toPlainText()) {
				undo = new QPropertyUndoCommand(m_text.data(), "plainText", m_text->toPlainText(), ui->m_line_edit->text());
				undo->setText(tr("Edit the text field"));
			}
			if (!TextSizeBox::isUnchanged(ui->m_size_sb, sizePxPerMm(), m_text->font().pointSizeF(), m_text->font())) {
				QFont font = m_text->font();
				font.setPointSizeF(TextSizeBox::points(ui->m_size_sb, sizePxPerMm(), font));
				undo = new QPropertyUndoCommand(m_text.data(), "font", m_text->font(), font);
				undo->setText(tr("Change the size of a text field"));
			}
			if (m_font_is_selected &&
				m_selected_font != m_text->font()) {
				undo = new QPropertyUndoCommand(m_text.data(), "font", m_text->font(), m_selected_font);
				undo->setText(tr("Change the font of a text field"));
			}
			if (ui->m_width_sb->value() != spinBoxWidth(m_text->textWidth())) {
				undo = new QPropertyUndoCommand(m_text.data(), "textWidth", m_text->textWidth(), qreal(ui->m_width_sb->value()));
				undo->setText(tr("Change the width of a text field"));
			}
			
			return undo;
		}
		else //several text are edited, only size and rotation is available for edition
		{
			QUndoCommand *parent_undo = nullptr;
			bool size_equal = true;
			bool angle_equal = true;
			bool font_equal = true;
			bool width_equal = true;
			qreal rotation_ = m_text_list.first()->rotation();
			const qreal mm_ = sizePxPerMm();
			qreal size_ = shownSize(m_text_list.first(), mm_);
			QFont font_ = m_text_list.first()->font();
			int width_ = spinBoxWidth(m_text_list.first()->textWidth());
			for (QPointer<IndependentTextItem> piti : m_text_list)
			{
				if (piti->rotation() != rotation_) {
					angle_equal = false;
				}
				if (shownSize(piti, mm_) != size_) {
					size_equal = false;
				}
				if (piti->font() != font_) {
					font_equal = false;
				}
				if (spinBoxWidth(piti->textWidth()) != width_) {
					width_equal = false;
				}
			}
				
			if ((angle_equal && (ui->m_angle_sb->value() != rotation_)) ||
				(!angle_equal && (ui->m_angle_sb->value() != ui->m_angle_sb->minimum())))
			{
				for (QPointer<IndependentTextItem> piti : m_text_list)
				{
					if (piti)
					{
						if (!parent_undo) {
							parent_undo = new QUndoCommand(tr("Rotate multiple text fields"));
						}
						QPropertyUndoCommand *qpuc = new QPropertyUndoCommand(piti.data(), "rotation", QVariant(piti->rotation()), QVariant(ui->m_angle_sb->value()), parent_undo);
						qpuc->setAnimated(true, false);
					}
				}
			}
			else if ((size_equal && (qAbs(ui->m_size_sb->value() - size_) > 0.001)) ||
					 (!size_equal && (ui->m_size_sb->value() != ui->m_size_sb->minimum())))
			{
				for (QPointer<IndependentTextItem> piti : m_text_list)
				{
					if (piti)
					{
						if (!parent_undo) {
							parent_undo = new QUndoCommand(tr("Change the size of multiple text fields"));
						}
						QFont font = piti->font();
						font.setPointSizeF(TextSizeBox::points(ui->m_size_sb, mm_, font));
						new QPropertyUndoCommand(piti.data(), "font", QVariant(piti->font()), QVariant(font), parent_undo);
					}
				}
			}
			else if ((m_font_is_selected && !font_equal) ||
					 (m_font_is_selected && (font_equal && (m_selected_font != font_))))
			{
				for (QPointer<IndependentTextItem> piti : m_text_list)
				{
					if (piti)
					{
						if (!parent_undo) {
							parent_undo = new QUndoCommand(tr("Change the font of several text fields"));
						}
						new QPropertyUndoCommand(piti.data(), "font", piti->font(), m_selected_font, parent_undo);
					}
				}
			}
			else if ((width_equal && (ui->m_width_sb->value() != width_)) ||
					 (!width_equal && (ui->m_width_sb->value() != ui->m_width_sb->minimum())))
			{
				for (QPointer<IndependentTextItem> piti : m_text_list)
				{
					if (piti)
					{
						if (!parent_undo) {
							parent_undo = new QUndoCommand(tr("Change the width of several text fields"));
						}
						new QPropertyUndoCommand(piti.data(), "textWidth", piti->textWidth(), qreal(ui->m_width_sb->value()), parent_undo);
					}
				}
			}
			return parent_undo;
		}
	}
		//In mode not live edit, only one text can be edited
	else if (m_text_list.isEmpty())
	{
		QUndoCommand *undo = new QUndoCommand(tr("Modify the properties of a text"));
		if(ui->m_x_sb->value() != m_text->pos().x()) {
			new QPropertyUndoCommand(m_text.data(), "x", QVariant(m_text->pos().x()), QVariant(ui->m_x_sb->value()), undo);
		}
		if(ui->m_y_sb->value() != m_text->pos().y()) {
			new QPropertyUndoCommand(m_text.data(), "y", QVariant(m_text->pos().y()), QVariant(ui->m_y_sb->value()), undo);
		}
		if(ui->m_angle_sb->value() != m_text->rotation()) {
			new QPropertyUndoCommand(m_text.data(), "rotation", QVariant(m_text->rotation()), QVariant(ui->m_angle_sb->value()), undo);
		}
		if (ui->m_line_edit->text() != m_text->toPlainText()) {
			new ChangeDiagramTextCommand(m_text.data(), m_text->toHtml(), ui->m_line_edit->text(), undo);
		}
		if (!TextSizeBox::isUnchanged(ui->m_size_sb, sizePxPerMm(), m_text->font().pointSizeF(), m_text->font()))
		{
			QFont font = m_text->font();
			font.setPointSizeF(TextSizeBox::points(ui->m_size_sb, sizePxPerMm(), font));
			new QPropertyUndoCommand(m_text.data(), "font", m_text->font(), font, undo);
		}
		if (m_font_is_selected && m_selected_font != m_text->font()) {
			new QPropertyUndoCommand(m_text.data(), "font", m_text->font(), m_selected_font, undo);
		}
		if (ui->m_width_sb->value() != spinBoxWidth(m_text->textWidth())) {
			new QPropertyUndoCommand(m_text.data(), "textWidth", m_text->textWidth(), qreal(ui->m_width_sb->value()), undo);
		}
		
		if (undo->childCount()) {
			return undo;
		} else {
			return nullptr;
		}
	}
	else {
		return nullptr;
	}
}

/**
	@brief IndiTextPropertiesWidget::setUpEditConnection
	Disconnect the previous connection, and reconnect the connection between the editors widgets and void IndiTextPropertiesWidget::apply function
*/
void IndiTextPropertiesWidget::setUpEditConnection()
{
	for (QMetaObject::Connection c : m_edit_connection) {
		disconnect(c);
	}
	m_edit_connection.clear();
	
	if (m_text_list.isEmpty())
	{
		m_edit_connection << connect(ui->m_x_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
		m_edit_connection << connect(ui->m_y_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
		m_edit_connection << connect(ui->m_line_edit, &QLineEdit::textEdited, this, &IndiTextPropertiesWidget::apply);
	}
	m_edit_connection << connect(ui->m_angle_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
	m_edit_connection << connect(ui->m_width_sb, QOverload<int>::of(&QSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
	m_edit_connection << connect(ui->m_size_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this]()
	{
		const qreal mm = sizePxPerMm();
			//Keep the size of the selected font when the box still shows it,
			//so a size in mm does not drift by rounding
		if (!TextSizeBox::isUnchanged(ui->m_size_sb, mm, m_selected_font.pointSizeF(), m_selected_font)) {
			this->m_selected_font.setPointSizeF(TextSizeBox::points(ui->m_size_sb, mm, m_selected_font));
		}
		this->apply();
	});
}

/**
	@brief IndiTextPropertiesWidget::updateUi
*/
void IndiTextPropertiesWidget::updateUi()
{
	if (!m_text && m_text_list.isEmpty()) {
		return;
	}
	m_size_px_per_mm = projectSizePxPerMm();

		//Disconnect every connections of editor widgets
		//to avoid an unwanted edition (QSpinBox emit valueChanged no matter if changer by user or by program)
	for (QMetaObject::Connection c : m_edit_connection) {
		disconnect(c);
	}
	m_edit_connection.clear();
	
	{
		const QFont size_font = sizeFont();
		TextSizeBox::configure(ui->m_size_sb, sizePxPerMm(), 4, 50, size_font);
	}

	ui->m_x_sb->setEnabled(m_text_list.isEmpty() ? true : false);
	ui->m_y_sb->setEnabled(m_text_list.isEmpty() ? true : false);
	ui->m_line_edit->setEnabled(m_text_list.isEmpty() ? true : false);
	ui->m_advanced_editor_pb->setEnabled(m_text_list.isEmpty() ? true : false);
	
	if (m_text_list.isEmpty())
	{
		ui->m_x_sb->setValue(m_text->pos().x());
		ui->m_y_sb->setValue(m_text->pos().y());
		ui->m_line_edit->setText(m_text->toPlainText());
		ui->m_angle_sb->setValue(m_text->rotation());
		TextSizeBox::setPoints(ui->m_size_sb, sizePxPerMm(), m_text->font().pointSizeF(), m_text->font());
		ui->m_width_sb->setValue(spinBoxWidth(m_text->textWidth()));
		
		ui->m_line_edit->setDisabled(m_text->isHtml() ? true : false);
		ui->m_size_sb->setDisabled(m_text->isHtml() ? true : false);
		ui->m_label->setVisible(m_text->isHtml() ? true : false);
		ui->m_break_html_pb->setVisible(m_text->isHtml() ? true : false);
		ui->m_font_pb->setDisabled(m_text->isHtml() ? true : false);
		ui->m_font_pb->setText(m_text->isHtml() ? tr("Font") : m_text->font().family());
	}
	else
	{
		bool size_equal = true;
		bool angle_equal = true;
		bool font_equal = true;
		bool width_equal = true;
		qreal rotation_ = m_text_list.first()->rotation();
		const qreal mm_ = sizePxPerMm();
		qreal size_ = shownSize(m_text_list.first(), mm_);
		QFont font_ = m_text_list.first()->font();
		int width_ = spinBoxWidth(m_text_list.first()->textWidth());

		for (QPointer<IndependentTextItem> piti : m_text_list)
		{
			if (piti->rotation() != rotation_) {
				angle_equal = false;
			}
			if (shownSize(piti, mm_) != size_) {
				size_equal = false;
			}
			if (piti->font() != font_) {
				font_equal = false;
			}
			if (spinBoxWidth(piti->textWidth()) != width_) {
				width_equal = false;
			}
		}
		ui->m_angle_sb->setValue(angle_equal ? rotation_ : 0);
			//Different widths show the minimum, as the other boxes do
		ui->m_width_sb->setValue(width_equal ? width_ : ui->m_width_sb->minimum());
		
		bool valid_ = true;
		for (QPointer<IndependentTextItem> piti : m_text_list) {
			if (piti->isHtml()) {
				valid_ = false;
			}
		}
		ui->m_font_pb->setEnabled(valid_);
		ui->m_font_pb->setText(font_equal ? font_.family() : tr("Font"));
		ui->m_size_sb->setEnabled(valid_);
		ui->m_size_sb->setValue(size_equal ? size_ : 0);
		ui->m_label->setVisible(false);
		ui->m_break_html_pb->setVisible(true);
	}

	
		//Set the connection now
	setUpEditConnection();
}

/**
	@brief IndiTextPropertiesWidget::on_m_advanced_editor_pb_clicked
*/
void IndiTextPropertiesWidget::on_m_advanced_editor_pb_clicked()
{
	if (m_text) {
		m_text->edit();
	}
}

void IndiTextPropertiesWidget::on_m_break_html_pb_clicked()
{
	if (m_text) {
		m_text->setPlainText(m_text->toPlainText());
	}
	for (QPointer<IndependentTextItem> piti : m_text_list) {
		piti->setPlainText(piti->toPlainText());
	}
	
	updateUi();
}

void IndiTextPropertiesWidget::on_m_font_pb_clicked()
{
	if (!m_text && m_text_list.isEmpty()) {
		return;
	}
	bool ok;
	QFont font = m_text ? m_text->font() : m_text_list.first()->font();
	m_selected_font = QFontDialog::getFont(&ok, font, this, QString(),
										   QFontDialog::DontUseNativeDialog);
	if (ok) {
		m_font_is_selected = true;
		ui->m_font_pb->setText(font.family());
		TextSizeBox::configure(ui->m_size_sb, sizePxPerMm(), 4, 50, m_selected_font);
		TextSizeBox::setPoints(ui->m_size_sb, sizePxPerMm(), m_selected_font.pointSizeF(), m_selected_font);
		LastUsedStyle::setTextFont(m_selected_font);
		apply();
	} else {
		ui->m_font_pb->setText(tr("Font"));
		m_font_is_selected = false;
	}
}
