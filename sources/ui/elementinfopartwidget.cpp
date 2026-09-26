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
#include "elementinfopartwidget.h"

#include "../SearchAndReplace/searchandreplaceworker.h"
#include "../qetinformation.h"
#include "ui_elementinfopartwidget.h"
#include <QCompleter>
#include <QRegularExpressionValidator>
#include <QStringListModel>

#include <utility>

typedef SearchAndReplaceWorker sarw;

/**
	@brief ElementInfoPartWidget::ElementInfoPartWidget
	Constructor
	@param key the string key what represent this info part
	@param translated_key the string key translated
	@param parent parent widget
*/
ElementInfoPartWidget::ElementInfoPartWidget(
		QString key,
		const QString& translated_key,
		QWidget *parent):
	QWidget(parent),
	ui(new Ui::ElementInfoPartWidget),
	key_(std::move(key))
{
	ui->setupUi(this);
	ui->label_->setText(translated_key);
	ui->m_erase_text->setVisible(false);

	if (key_ == QETInformation::ELMT_WIDTH || key_ == QETInformation::ELMT_HEIGHT || key_ == QETInformation::ELMT_DEPTH)
	{
		auto *validator = new QETInformation::NumericInfoValidator(ui->line_edit);
		ui->line_edit->setValidator(validator);
		ui->line_edit->setPlaceholderText(tr("ex. 80.5"));
		ui->line_edit->setToolTip(tr("Nombre décimal avec un point comme séparateur (ex. 80.5)"));
	}

	connect(ui->line_edit, &QLineEdit::textEdited,
		this, &ElementInfoPartWidget::textEdited);
	connect(ui->line_edit, &QLineEdit::textChanged,
		this, &ElementInfoPartWidget::textChanged);
}

/**
	@brief ElementInfoPartWidget::~ElementInfoPartWidget
	destructor
*/
ElementInfoPartWidget::~ElementInfoPartWidget()
{
	delete ui;
}

/**
	@brief ElementInfoPartWidget::text
	@return the text in the line edit
*/
QString ElementInfoPartWidget::text() const
{
	return (ui->line_edit->text());
}

/**
	@brief ElementInfoPartWidget::hasAcceptableInput
	@return whether the line edit's current text satisfies its validator
*/
bool ElementInfoPartWidget::hasAcceptableInput() const
{
	return ui->line_edit->hasAcceptableInput();
}

/**
	@brief ElementInfoPartWidget::setText
	Set text to line edit
	@param txt
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
	@brief ElementInfoPartWidget::setPlaceHolderText
	@param text
*/
void ElementInfoPartWidget::setPlaceHolderText(const QString &text)
{
	ui->line_edit->setPlaceholderText(text);
}

/**
	@brief ElementInfoPartWidget::setSuggestions
	Offer suggestions as a drop-down list while typing in the line edit,
	matching anywhere in the text and ignoring case.
	An empty list removes the drop-down.
	@param suggestions
*/
void ElementInfoPartWidget::setSuggestions(const QStringList &suggestions)
{
	if (suggestions.isEmpty()) {
		ui->line_edit->setCompleter(nullptr);
		return;
	}

	if (!m_completer) {
		m_suggestions_model = new QStringListModel(this);
		m_completer = new QCompleter(m_suggestions_model, this);
		m_completer->setCaseSensitivity(Qt::CaseInsensitive);
		m_completer->setFilterMode(Qt::MatchContains);
	}
	m_suggestions_model->setStringList(suggestions);
	ui->line_edit->setCompleter(m_completer);
}

/**
	@brief ElementInfoPartWidget::setFocusTolineEdit
	Set the focus to the line edit
*/
void ElementInfoPartWidget::setFocusTolineEdit()
{
	ui->line_edit->setFocus();
}

/**
	@brief ElementInfoPartWidget::setEnabled
	enable the line edit
	@param e
*/
void ElementInfoPartWidget::setEnabled(bool e)
{
	ui->line_edit->setEnabled(e);
}

/**
	@brief ElementInfoPartWidget::setDisabled
	disable the line edit
	@param d
*/
void ElementInfoPartWidget::setDisabled(bool d)
{
	ui->line_edit->setDisabled(d);
}

/**
	@brief ElementInfoPartWidget::setEraseTextVisible
	@param visible
*/
void ElementInfoPartWidget::setEraseTextVisible(bool visible)
{
	ui->m_erase_text->setVisible(visible);
	m_show_erase = visible;
}

/**
	@brief ElementInfoPartWidget::setEraseTextChecked
	@param check
*/
void ElementInfoPartWidget::setEraseTextChecked(bool check)
{
	ui->m_erase_text->setChecked(check);
}

/**
	@brief ElementInfoPartWidget::EraseTextCheckState
	@return 
*/
Qt::CheckState ElementInfoPartWidget::EraseTextCheckState() const
{
	return ui->m_erase_text->checkState();
}

void ElementInfoPartWidget::on_m_erase_text_clicked()
{
	ui->line_edit->setText(ui->m_erase_text->isChecked()
			       ? SearchAndReplaceWorker::eraseText()
			       : QString());
	ui->line_edit->setDisabled(ui->m_erase_text->isChecked());
}
