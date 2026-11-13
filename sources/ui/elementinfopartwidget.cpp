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

#include <SearchAndReplace/searchandreplaceworker.h>
#include <qeticons.h>
#include <qetinformation.h>
#include "ui_elementinfopartwidget.h"
#include <QAbstractItemView>
#include <QApplication>
#include <QCompleter>
#include <QMouseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QRegularExpressionValidator>
#include <QStringListModel>
#include <QToolButton>

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
	ui->m_material_btn->setVisible(false);

		//The line edit must swallow the whole width of the row: without
		//this the material button column would steal room from it.
	ui->gridLayout_2->setColumnStretch(0, 1);

	connect(ui->m_material_btn, &QToolButton::clicked,
			this, &ElementInfoPartWidget::materialButtonClicked);

	ui->m_multiline_btn->setIcon(QET::Icons::EditText);
	connect(ui->m_multiline_btn, &QToolButton::clicked,
			this, &ElementInfoPartWidget::editOnSeveralLines);

	if (key_ == QETInformation::ELMT_WIDTH || key_ == QETInformation::ELMT_HEIGHT || key_ == QETInformation::ELMT_DEPTH)
	{
		ui->m_multiline_btn->setVisible(false);
		auto *validator = new QETInformation::NumericInfoValidator(ui->line_edit);
		ui->line_edit->setValidator(validator);
		ui->line_edit->setPlaceholderText(tr("e.g. 80.5"));
		ui->line_edit->setToolTip(tr("Decimal number with a dot as separator (e.g. 80.5)"));
	}

	connect(ui->line_edit, &QLineEdit::textEdited,
		this, &ElementInfoPartWidget::textEdited);
	connect(ui->line_edit, &QLineEdit::textChanged,
		this, &ElementInfoPartWidget::textChanged);
		//The line edit shows a line break as a space: the tooltip shows
		//the text as it is drawn.
	connect(ui->line_edit, &QLineEdit::textChanged, this, [this](const QString &txt) {
		ui->line_edit->setToolTip(txt.contains(QLatin1Char('\n')) ? txt : QString());
	});
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
	@brief ElementInfoPartWidget::editOnSeveralLines
	Edit the text in a box that takes line breaks (bugtracker #348):
	the line edit cannot show nor type them.
*/
void ElementInfoPartWidget::editOnSeveralLines()
{
	QDialog dialog(this);
	dialog.setWindowTitle(ui->label_->text());

	auto *edit = new QPlainTextEdit(&dialog);
	edit->setPlainText(ui->line_edit->text());
	auto *buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	auto *layout = new QVBoxLayout(&dialog);
	layout->addWidget(edit);
	layout->addWidget(buttons);
	edit->setFocus();

	if (dialog.exec() != QDialog::Accepted)
		return;

	const QString txt = edit->toPlainText();
	if (txt == ui->line_edit->text())
		return;
	ui->line_edit->setText(txt);
	emit textEdited(txt);
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
			//After the completer's own filter, so this one is asked first
		m_completer->popup()->installEventFilter(this);
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
	@brief ElementInfoPartWidget::setMaterialButtonVisible
	Show or hide the "..." button opening the material file for this row.
	@param visible
*/
void ElementInfoPartWidget::setMaterialButtonVisible(bool visible)
{
	ui->m_material_btn->setVisible(visible);
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

/**
	@brief ElementInfoPartWidget::eventFilter
	A click outside the open list of suggestions closes the list, and Qt
	drops that click: the Apply or OK button clicked to finish the edit did
	nothing, and the edit could then be lost. The click is passed on to the
	widget under it.
*/
bool ElementInfoPartWidget::eventFilter(QObject *watched, QEvent *event)
{
	if (m_completer && watched == m_completer->popup()
			&& event->type() == QEvent::MouseButtonPress)
	{
		QWidget *popup = m_completer->popup();
		auto *mouse = static_cast<QMouseEvent *>(event);
		const QPoint global = mouse->globalPosition().toPoint();
		if (!popup->rect().contains(popup->mapFromGlobal(global)))
		{
			popup->hide();
				//A whole click: the release that follows goes to the
				//closed list, not to the widget, so it is sent too
			if (QWidget *target = QApplication::widgetAt(global)) {
				const QPointF local = target->mapFromGlobal(global);
				QCoreApplication::postEvent(target, new QMouseEvent(
						QEvent::MouseButtonPress, local, global, mouse->button(),
						mouse->buttons(), mouse->modifiers()));
				QCoreApplication::postEvent(target, new QMouseEvent(
						QEvent::MouseButtonRelease, local, global, mouse->button(),
						Qt::NoButton, mouse->modifiers()));
			}
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}
