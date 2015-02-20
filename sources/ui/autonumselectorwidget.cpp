/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "autonumselectorwidget.h"
#include "ui_autonumselectorwidget.h"

/**
 * @brief AutonumSelectorWidget::AutonumSelectorWidget
 * Constructor with texts to fill the combo box
 * @param items, items for fill the combo box
 * @param parent, parent widget
 */
AutonumSelectorWidget::AutonumSelectorWidget(const QStringList &items, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::AutonumSelectorWidget)
{
	ui->setupUi(this);
	ui -> m_available_autonum_cb -> addItems(items);
}

/**
 * @brief AutonumSelectorWidget::~AutonumSelectorWidget
 * Destructor
 */
AutonumSelectorWidget::~AutonumSelectorWidget() {
	delete ui;
}

/**
 * @brief AutonumSelectorWidget::setCurrentItem
 * Set the combo box current index by text.
 * If text doesn't exist, set current index -1
 * @param item, item of index
 */
void AutonumSelectorWidget::setCurrentItem(const QString &item) {
	ui -> m_available_autonum_cb -> setCurrentIndex( ui -> m_available_autonum_cb -> findText(item));
}

/**
 * @brief AutonumSelectorWidget::setItems
 * Populate this widget with the content of @items
 * Previous items is clear.
 * @param items
 */
void AutonumSelectorWidget::setItems(const QStringList &items) {
	ui -> m_available_autonum_cb -> clear();
	ui -> m_available_autonum_cb -> addItems(items);
}

/**
 * @brief AutonumSelectorWidget::text
 * @return the current displayed text
 */
QString AutonumSelectorWidget::text() const {
	return ui -> m_available_autonum_cb -> currentText();
}

/**
 * @brief AutonumSelectorWidget::on_m_edit_autonum_pb_clicked
 * Just emit the signal openAutonumEditor.
 * The owner of AutonumSelectorWidget need to connect the signal.
 */
void AutonumSelectorWidget::on_m_edit_autonum_pb_clicked() {
	emit openAutonumEditor();
}
