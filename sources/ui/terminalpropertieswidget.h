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
#ifndef TERMINALPROPERTIESWIDGET_H
#define TERMINALPROPERTIESWIDGET_H

#include "abstractelementpropertieseditorwidget.h"

class QUndoCommand;

namespace Ui {
	class TerminalPropertiesWidget;
}

/**
	@brief The TerminalPropertiesWidget class
	What a terminal is: its type, its function, the form it is built as
	and whether it carries an indicator -- the same four choices the
	terminal strip manager shows, here at the terminal of the folio.
*/
class TerminalPropertiesWidget : public AbstractElementPropertiesEditorWidget
{
	Q_OBJECT

	public:
		explicit TerminalPropertiesWidget(Element *elmt,
						  QWidget *parent = nullptr);
		~TerminalPropertiesWidget() override;

		void setElement(Element *element) override;
		void apply() override;
		void reset() override;
		QUndoCommand *associatedUndo() const override;
		QString title() const override {
			return tr("Terminal");}
		bool setLiveEdit(bool live_edit) override;

	public slots:
		void updateUi() override;

	protected:
		void enableLiveEdit() override;
		void disableLiveEdit() override;

	private:
		Ui::TerminalPropertiesWidget *ui;
		bool m_live_edit = false;
};

#endif // TERMINALPROPERTIESWIDGET_H
