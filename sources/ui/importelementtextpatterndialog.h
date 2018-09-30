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
#ifndef IMPORTELEMENTTEXTPATTERNDIALOG_H
#define IMPORTELEMENTTEXTPATTERNDIALOG_H

#include <QDialog>

namespace Ui {
	class ImportElementTextPatternDialog;
}

/**
 * @brief The ImportElementTextPatternDialog class
 * A dialog use for ask user to select a element text pattern.
 * This dialog is highly inspired from QInputDialog::getItem.
 * In fact this the same + a check box.
 */
class ImportElementTextPatternDialog : public QDialog
{
	Q_OBJECT
	
	public:
		explicit ImportElementTextPatternDialog (QWidget *parent = nullptr);
		~ImportElementTextPatternDialog();
	
		static QString getItem (QWidget *parent, const QString &title, const QString &label,
								const QStringList &items, bool *ok = nullptr, bool *erase = nullptr);
		
		void setLabelText (const QString &label);
		void setComboBoxItems (const QStringList &items);
		QString textValue() const;
	
	private:
		Ui::ImportElementTextPatternDialog *ui;
};

#endif // IMPORTELEMENTTEXTPATTERNDIALOG_H
