/*
	Copyright 2006-2014 The QElectroTech Team
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
 * default constructor
 * @param parent, parent widget
 */
AutonumSelectorWidget::AutonumSelectorWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::AutonumSelectorWidget)
{
	ui->setupUi(this);
	ui->m_edit_autonum_pb->setDisabled(true);
}

/**
 * @brief AutonumSelectorWidget::AutonumSelectorWidget
 * Constructor with texts to fill the combo box
 * @param text, texts for fill the combo box
 * @param parent, parent widget
 */
AutonumSelectorWidget::AutonumSelectorWidget(const QList <QString> &text, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::AutonumSelectorWidget)
{
	ui->setupUi(this);
	ui -> m_available_autonum_cb -> addItems(text);
	ui->m_edit_autonum_pb->setDisabled(true);
}

/**
 * @brief AutonumSelectorWidget::~AutonumSelectorWidget
 * Destructor
 */
AutonumSelectorWidget::~AutonumSelectorWidget()
{
	delete ui;
}

/**
 * @brief AutonumSelectorWidget::setCurrentItem
 * Set the combo box current index by text.
 * If text doesn't exist, set current index -1
 * @param text, text of index
 */
void AutonumSelectorWidget::setCurrentItem(const QString &text) {
	ui -> m_available_autonum_cb -> setCurrentIndex( ui -> m_available_autonum_cb -> findText(text));
}

/**
 * @brief AutonumSelectorWidget::text
 * @return the current displayed text
 */
QString AutonumSelectorWidget::text() const {
	return ui -> m_available_autonum_cb -> currentText();
}
