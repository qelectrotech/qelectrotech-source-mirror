// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef FORMULABUILDERDIALOG_H
#define FORMULABUILDERDIALOG_H

#include "../formulabuilder.h"

#include <QDialog>

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;

namespace FormulaBuilderDialogPrivate { class NodeEditor; }

/**
	@brief The FormulaBuilderDialog class
	Builds the formula of a table column without writing SQL: the user
	picks a field, a value or an operation, and an operation's inputs can be
	operations in their turn. The SQL it makes is shown, read only.
*/
class FormulaBuilderDialog : public QDialog
{
	Q_OBJECT

	public:
		FormulaBuilderDialog(const QList<FormulaBuilder::Field> &fields, QWidget *parent = nullptr);

		QString title() const;
		QString expression() const {return m_expression;}

	private:
		void update();

		QList<FormulaBuilder::Field> m_fields;
		QLineEdit *m_title;
		FormulaBuilderDialogPrivate::NodeEditor *m_editor;
		QLabel *m_error;
		QPlainTextEdit *m_sql;
		QDialogButtonBox *m_buttons;
		QString m_expression;
};

#endif // FORMULABUILDERDIALOG_H
