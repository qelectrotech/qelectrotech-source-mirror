/*
	Copyright 2006-2018 The QElectroTech Team
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
#include "elementinfopartwidget.h"

#include <utility>
#include "ui_elementinfopartwidget.h"
#include "searchandreplaceworker.h"

typedef SearchAndReplaceWorker sarw;

/**
 * @brief ElementInfoPartWidget::ElementInfoPartWidget
 * Constructor
 * @param key the string key what represent this info part
 * @param translated_key the string key translated
 * @param parent parent widget
 */
ElementInfoPartWidget::ElementInfoPartWidget(QString key, const QString& translated_key, QWidget *parent):
	QWidget(parent),
	ui(new Ui::ElementInfoPartWidget),
	key_(std::move(key))
{
	ui->setupUi(this);
	ui->label_->setText(translated_key);
	ui->m_erase_text->setVisible(false);

	connect(ui->line_edit, &QLineEdit::textEdited,  this, &ElementInfoPartWidget::textEdited);
	connect(ui->line_edit, &QLineEdit::textChanged, this, &ElementInfoPartWidget::textChanged);
}

/**
 * @brief ElementInfoPartWidget::~ElementInfoPartWidget
 * destructor
 */
ElementInfoPartWidget::~ElementInfoPartWidget()
{
	delete ui;
}

/**
 * @brief ElementInfoPartWidget::setText
 * Set text to line edit
 * @param txt
 */
void ElementInfoPartWidget::setText(const QString &txt)
{
	if (m_show_erase) {
		sarw::setupLineEdit(ui->line_edit, ui->m_erase_text, txt);
	} else {
		ui->line_edit->setText(txt);
	}
}

/**
 * @brief ElementInfoPartWidget::text
 * @return the text in the line edit
 */
QString ElementInfoPartWidget::text() const {
	return (ui->line_edit->text());
}

/**
 * @brief ElementInfoPartWidget::setPlaceHolderText
 * @param text
 */
void ElementInfoPartWidget::setPlaceHolderText(const QString &text) {
	ui->line_edit->setPlaceholderText(text);
}

/**
 * @brief ElementInfoPartWidget::setFocusTolineEdit
 * Set the focus to the line edit
 */
void ElementInfoPartWidget::setFocusTolineEdit() {
	ui->line_edit->setFocus();
}

/**
 * @brief ElementInfoPartWidget::setEnabled
 * enable the line edit
 * @param e
 */
void ElementInfoPartWidget::setEnabled(bool e) {
	ui->line_edit->setEnabled(e);
}

/**
 * @brief ElementInfoPartWidget::setDisabled
 * disable the line edit
 * @param d
 */
void ElementInfoPartWidget::setDisabled(bool d) {
	ui->line_edit->setDisabled(d);
}

/**
 * @brief ElementInfoPartWidget::setEraseTextVisible
 * @param visible
 */
void ElementInfoPartWidget::setEraseTextVisible(bool visible) {
	ui->m_erase_text->setVisible(visible);
	m_show_erase = visible;
}

/**
 * @brief ElementInfoPartWidget::setEraseTextChecked
 * @param check
 */
void ElementInfoPartWidget::setEraseTextChecked(bool check) {
	ui->m_erase_text->setChecked(check);
}

/**
 * @brief ElementInfoPartWidget::EraseTextCheckState
 * @return 
 */
Qt::CheckState ElementInfoPartWidget::EraseTextCheckState() const {
	return ui->m_erase_text->checkState();
}

void ElementInfoPartWidget::on_m_erase_text_clicked()
{
    ui->line_edit->setText(ui->m_erase_text->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->line_edit->setDisabled(ui->m_erase_text->isChecked());
}
