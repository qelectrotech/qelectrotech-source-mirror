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
#include "addtabledialog.h"
#include "ui_addtabledialog.h"
#include "elementquerywidget.h"
#include "marginseditdialog.h"

#include <QFontDialog>

/**
 * @brief AddTableDialog::AddNomenclatureDialog
 * @param parent
 */
AddTableDialog::AddTableDialog(QWidget *parent) :
    QDialog(parent),
	ui(new Ui::AddTableDialog)
{
    ui->setupUi(this);
	ui->m_header_font_pb->setText(m_header_font.family());
	ui->m_table_font_pb->setText(m_table_font.family());
	ui->m_tab->addTab(m_query_widget, tr("Contenu"));
	ui->m_config_gb->setDisabled(true);
}

/**
 * @brief AddTableDialog::~AddNomenclatureDialog
 */
AddTableDialog::~AddTableDialog() {
	delete ui;
}

/**
 * @brief AddTableDialog::setQueryWidget
 * Not implemented yet
 * @param widget
 */
void AddTableDialog::setQueryWidget(QWidget *widget) {
	Q_UNUSED(widget)
}

/**
 * @brief AddTableDialog::queryStr
 * @return
 */
QString AddTableDialog::queryStr() {
	return m_query_widget->queryStr();
}

/**
 * @brief AddTableDialog::adjustTableToFolio
 * @return
 */
bool AddTableDialog::adjustTableToFolio() const {
	return ui->m_adjust_table_size_cb->isChecked();
}

/**
 * @brief AddTableDialog::addNewTableToNewDiagram
 * @return
 */
bool AddTableDialog::addNewTableToNewDiagram() const {
	return ui->m_add_table_and_folio->isChecked();
}

/**
 * @brief AddTableDialog::tableName
 * @return
 */
QString AddTableDialog::tableName() const {
	return ui->m_table_name_le->text();
}

/**
 * @brief AddTableDialog::headerMargins
 * @return
 */
QMargins AddTableDialog::headerMargins() const {
	return  m_header_margins;
}

/**
 * @brief AddTableDialog::headerAlignment
 * @return
 */
Qt::Alignment AddTableDialog::headerAlignment() const
{
	switch (ui->m_header_alignment_cb->currentIndex()) {
		case 0 :
			return  Qt::AlignLeft;
		case 1:
			return Qt::AlignCenter;
		default:
			return Qt::AlignRight;
	}
}

/**
 * @brief AddTableDialog::headerFont
 * @return
 */
QFont AddTableDialog::headerFont() const {
	return m_header_font;
}

/**
 * @brief AddTableDialog::tableMargins
 * @return
 */
QMargins AddTableDialog::tableMargins() const {
	return m_table_margins;
}

/**
 * @brief AddTableDialog::tableAlignment
 * @return
 */
Qt::Alignment AddTableDialog::tableAlignment() const
{
	switch (ui->m_table_alignment_cb->currentIndex()) {
		case 0 :
			return  Qt::AlignLeft;
		case 1:
			return Qt::AlignCenter;
		default:
			return Qt::AlignRight;
	}
}

/**
 * @brief AddTableDialog::tableFont
 * @return
 */
QFont AddTableDialog::tableFont() const {
	return m_table_font;
}

void AddTableDialog::on_m_header_font_pb_clicked()
{
	bool b;
	auto font = QFontDialog::getFont(&b, m_header_font, this, tr("Sélectionné la police des en tête du tableau"));
	if (b) {
		m_header_font = font;
		ui->m_header_font_pb->setText(font.family());
	}

}

void AddTableDialog::on_m_table_font_pb_clicked()
{
	bool b;
	auto font = QFontDialog::getFont(&b, m_table_font, this, tr("Sélectionné la police des cellules du tableau"));
	if (b) {
		m_table_font = font;
		ui->m_table_font_pb->setText(font.family());
	}
}

void AddTableDialog::on_m_edit_header_margins_pb_clicked()
{
	bool accept;
	auto margins_ = MarginsEditDialog::getMargins(m_header_margins, &accept, this);
	if (accept)
		m_header_margins = margins_;
}

void AddTableDialog::on_m_table_margins_pb_clicked()
{
	bool accept;
	auto margins_ = MarginsEditDialog::getMargins(m_table_margins, &accept, this);
	if (accept)
		m_table_margins = margins_;
}
