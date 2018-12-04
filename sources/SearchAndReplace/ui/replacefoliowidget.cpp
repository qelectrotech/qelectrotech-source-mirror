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
#include "replacefoliowidget.h"
#include "ui_replacefoliowidget.h"
#include "diagramcontextwidget.h"
#include "searchandreplaceworker.h"

#include <QDialogButtonBox>

ReplaceFolioWidget::ReplaceFolioWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ReplaceFolioWidget)
{
	ui->setupUi(this);
	m_diagram_context_widget = new DiagramContextWidget(this);
	ui->m_tab2_vlayout->addWidget(m_diagram_context_widget);
}

ReplaceFolioWidget::~ReplaceFolioWidget()
{
	delete ui;
}

/**
 * @brief ReplaceFolioWidget::titleBlockProperties
 * @return the title block properties edited by this widget
 */
TitleBlockProperties ReplaceFolioWidget::titleBlockProperties() const
{
	TitleBlockProperties prop;
	prop.title    = ui->m_title_le ->text();
	prop.author   = ui->m_author_le->text();
	prop.filename = ui->m_file_le  ->text();
    prop.plant  = ui->m_plant     ->text();
	prop.locmach  = ui->m_loc      ->text();
	prop.indexrev = ui->m_indice   ->text();
	prop.folio    = ui->m_folio_le ->text();

	if (ui->m_unchanged_date->isChecked()) {
		prop.date = QDate();
		prop.useDate = TitleBlockProperties::UseDateValue;
	}
	if (ui->m_no_date_rb->isChecked()) {
		prop.date = SearchAndReplaceWorker::eraseDate();
		prop.useDate = TitleBlockProperties::UseDateValue;
	}
	else if (ui->m_fixed_date_rb->isChecked()) {
		prop.date = ui->m_date_edit->date();
		prop.useDate = TitleBlockProperties::UseDateValue;
	}

	prop.context = m_diagram_context_widget->context();
	return prop;
}

/**
 * @brief ReplaceFolioWidget::setTitleBlockProperties
 * Set the title block properties edited by this widget
 * @param properties
 */
void ReplaceFolioWidget::setTitleBlockProperties(const TitleBlockProperties &properties)
{
	ui->m_title_le ->setText (properties.title);
	ui->m_author_le->setText (properties.author);
	ui->m_file_le  ->setText (properties.filename);
    ui->m_plant     ->setText (properties.plant);
	ui->m_loc      ->setText (properties.locmach);
	ui->m_indice   ->setText (properties.indexrev);
	ui->m_folio_le ->setText (properties.folio);

		//About date	
	ui->m_date_now_pb->setDisabled(true);
	ui->m_date_edit  ->setDisabled(true);
	ui->m_date_edit  ->setDate(QDate::currentDate());


	if (properties.useDate == TitleBlockProperties::CurrentDate) {
		ui -> m_fixed_date_rb ->setChecked(true);
	}
	else
	{
		if (properties.date.isNull()) {
			ui->m_unchanged_date->setChecked(true);
		}
		else if (properties.date == SearchAndReplaceWorker::eraseDate()) {
			ui->m_no_date_rb->setChecked(true);
		}
		else
		{
			ui->m_fixed_date_rb->setChecked(true);
			ui->m_date_edit->setDate(properties.date);
		}
	}
		//About date
	
	m_diagram_context_widget->setContext(properties.context);
}

ReplaceFolioDialog::ReplaceFolioDialog(QWidget *parent) : 
	QDialog(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_widget = new ReplaceFolioWidget(this));
	layout->addWidget(m_button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset, this));
	
	connect(m_button_box, &QDialogButtonBox::clicked, [this](QAbstractButton *button_)
	{
		this->done(m_button_box->buttonRole(button_));
	});
}

ReplaceFolioDialog::~ReplaceFolioDialog()
{}

/**
 * @brief ReplaceFolioDialog::titleBlockProperties
 * @return The title block properties edited by this dialog
 */
TitleBlockProperties ReplaceFolioDialog::titleBlockProperties() const {
	return m_widget->titleBlockProperties();
}

/**
 * @brief ReplaceFolioDialog::setTitleBlockProperties
 * @param properties : set the title block properties edited by this dialog
 */
void ReplaceFolioDialog::setTitleBlockProperties(const TitleBlockProperties &properties) {
	m_widget->setTitleBlockProperties(properties);
}
void ReplaceFolioWidget::on_m_title_cb_clicked() {
	ui->m_title_le->setText(ui->m_title_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->m_title_le->setDisabled(ui->m_title_cb->isChecked());
}

void ReplaceFolioWidget::on_m_author_cb_clicked() {
    ui->m_author_le->setText(ui->m_author_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->m_author_le->setDisabled(ui->m_author_cb->isChecked());
}

void ReplaceFolioWidget::on_m_file_cb_clicked() {
    ui->m_file_le->setText(ui->m_file_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->m_file_le->setDisabled(ui->m_file_cb->isChecked());
}

void ReplaceFolioWidget::on_m_folio_cb_clicked() {
    ui->m_folio_le->setText(ui->m_folio_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->m_folio_le->setDisabled(ui->m_folio_cb->isChecked());
}

void ReplaceFolioWidget::on_m_plant_cb_clicked() {
    ui->m_plant->setText(ui->m_plant_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
    ui->m_plant->setDisabled(ui->m_plant_cb->isChecked());
}

void ReplaceFolioWidget::on_m_loc_cb_clicked() {
    ui->m_loc->setText(ui->m_loc_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->m_loc->setDisabled(ui->m_loc_cb->isChecked());
}

void ReplaceFolioWidget::on_m_indice_cb_clicked() {
    ui->m_indice->setText(ui->m_indice_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->m_indice->setDisabled(ui->m_indice_cb->isChecked());
}
