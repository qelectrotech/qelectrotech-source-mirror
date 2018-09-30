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
#include "importelementtextpatterndialog.h"
#include "ui_importelementtextpatterndialog.h"

ImportElementTextPatternDialog::ImportElementTextPatternDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ImportElementTextPatternDialog) {
	ui->setupUi(this);
}

ImportElementTextPatternDialog::~ImportElementTextPatternDialog()
{
	delete ui;
}

/**
 * @brief ImportElementTextPatternDialog::getItem
 * For all arguments see QInputDialog::getItem, except for erase, they store the state of the check box.
 * @param parent
 * @param title
 * @param label
 * @param items
 * @param ok
 * @param erase
 * @return 
 */
QString ImportElementTextPatternDialog::getItem(QWidget *parent, const QString &title, const QString &label, const QStringList &items, bool *ok, bool *erase)
{
	QString text(items.value(0));
   
   QScopedPointer<ImportElementTextPatternDialog> dialog(new ImportElementTextPatternDialog(parent));
   dialog->setWindowTitle(title);
   dialog->setLabelText(label);
   dialog->setComboBoxItems(items);
   dialog->setInputMethodHints(Qt::ImhNone);
   const int ret = dialog->exec();
   if (ok)
	   *ok = !!ret;
   if(erase)
	   *erase = dialog->ui->m_erase_existing_text->isChecked();
   if (ret) {
	   return dialog->textValue();
   } else {
	   return text;
   }
}

void ImportElementTextPatternDialog::setLabelText(const QString &label) {
	ui->m_label->setText(label);
}

void ImportElementTextPatternDialog::setComboBoxItems(const QStringList &items) {
	ui->m_combo_box->addItems(items);
}

QString ImportElementTextPatternDialog::textValue() const {
	return ui->m_combo_box->currentText();
}
