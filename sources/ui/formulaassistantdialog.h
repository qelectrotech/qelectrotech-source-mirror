/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef FORMULAASSISTANTDIALOG_H
#define FORMULAASSISTANTDIALOG_H

#include <QDialog>
#include <QRegularExpression>

namespace Ui {
	class FormulaAssistantDialog;
}

class FormulaAssistantDialog : public QDialog
{
		Q_OBJECT
		
	public:
		FormulaAssistantDialog(QWidget *parent = 0);
		~FormulaAssistantDialog();
		
		void setForbiddenVariables(QStringList list);
		void setText(QString text);
		void setFormula(QString text);
		QString formula() const;
		
	private slots:
		void on_m_line_edit_textChanged(const QString &arg1);
		
	private:
		Ui::FormulaAssistantDialog *ui;
		QRegularExpression m_rx;
		QString m_formula;
};

#endif // FORMULAASSISTANTDIALOG_H
