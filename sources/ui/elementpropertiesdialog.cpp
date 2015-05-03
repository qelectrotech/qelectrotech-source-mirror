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
#include "elementpropertiesdialog.h"
#include "elementpropertieswidget.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

/**
 * @brief ElementPropertiesDialog::ElementPropertiesDialog
 * default constructor
 * @param elmt
 * @param parent
 */
ElementPropertiesDialog::ElementPropertiesDialog(Element *elmt, QWidget *parent) :
	QDialog(parent),
	element_ (elmt)
{
	m_editor = new ElementPropertiesWidget(elmt, this);

	connect(m_editor, SIGNAL(editElementRequired(ElementsLocation)), this , SLOT(editElement(ElementsLocation)));
	connect(m_editor, SIGNAL(findElementRequired(ElementsLocation)), this, SLOT(findInPanel(ElementsLocation)));

	dbb = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Reset, Qt::Horizontal, this);
	connect(dbb, SIGNAL(clicked(QAbstractButton*)), this, SLOT(standardButtonClicked(QAbstractButton*)));

	QVBoxLayout *main_layout = new QVBoxLayout(this);
	main_layout -> addWidget(m_editor);
	main_layout -> addWidget(dbb);
	setLayout(main_layout);
}

/**
 * @brief ElementPropertiesDialog::standardButtonClicked
 * apply action when click in the dialog standard button box
 * @param button
 * the cliked button
 */
void ElementPropertiesDialog::standardButtonClicked(QAbstractButton *button) {
	int  answer = dbb -> buttonRole(button);

	switch (answer) {
		case QDialogButtonBox::ResetRole:
			m_editor->reset();
			break;
		case QDialogButtonBox::ApplyRole:
			m_editor->apply();
			accept();
			break;
		default:
			reject();
			break;
	}
}

/**
 * @brief ElementPropertiesDialog::findInPanel
 * Slot
 */
void ElementPropertiesDialog::findInPanel(const ElementsLocation &location)
{
	emit findElementRequired(location);
	reject();
}

/**
 * @brief ElementPropertiesDialog::editElement
 * Slot
 */
void ElementPropertiesDialog::editElement(const ElementsLocation &location)
{
	emit findElementRequired(location);
	emit editElementRequired(location);
	reject();
}
