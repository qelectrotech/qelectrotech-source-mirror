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
#ifndef ELEMENTTERMINALSWIDGET_H
#define ELEMENTTERMINALSWIDGET_H

#include "abstractelementpropertieseditorwidget.h"
#include "properties/terminallabel.h"

#include <QList>
#include <QPointer>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class Terminal;

/**
	@brief The ElementTerminalsWidget class
	The "Terminals" tab of a placed symbol's properties: one row per
	terminal of the symbol, whether its name is shown, what the name says
	and how it is drawn.
	Everything a row changes belongs to this copy of the symbol: the
	definition keeps its own values, and so do the other copies, so three
	copies of one symbol can read differently
	(@see Element::terminalLabels()).
*/
class ElementTerminalsWidget : public AbstractElementPropertiesEditorWidget
{
	Q_OBJECT

	public:
		explicit ElementTerminalsWidget(Element *element, QWidget *parent = nullptr);

		void setElement(Element *element) override;
		void apply() override;
		void reset() override {updateUi();}
		QUndoCommand *associatedUndo() const override;
		QString title() const override;
		bool setLiveEdit(bool live_edit) override;
		void updateUi() override;

	private:
		/// One row of the table: the terminal it edits, the values the
		/// row holds so far, and the widgets it is made of
		struct Row
		{
			QPointer<Terminal> terminal;
			TerminalLabel label;
			QCheckBox *show = nullptr;
			QLineEdit *name = nullptr;
			QPushButton *font = nullptr;
			QPushButton *reset = nullptr;
		};

		void buildRow(int row, Terminal *terminal);
		void showRowValues(int row);
		void nameChanged(int row);
		void showChanged(int row);
		void fontClicked(int row);
		void resetClicked(int row);

	private:
		QTableWidget *m_table = nullptr;
		QLabel *m_empty_lb = nullptr;
		QList<Row> m_rows;
};

#endif // ELEMENTTERMINALSWIDGET_H
