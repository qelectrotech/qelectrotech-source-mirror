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
#ifndef MACROLABELSDIALOG_H
#define MACROLABELSDIALOG_H

#include <diagramevent/macrolabels.h>

#include <QDialog>

class QTableWidget;

/**
	@brief The MacroLabelsDialog class
	Asks for the device labels and question-mark texts of a macro being
	placed. Accept All takes the proposed labels; Cancel places nothing.
*/
class MacroLabelsDialog : public QDialog
{
	Q_OBJECT
	public:
		explicit MacroLabelsDialog(const QList<MacroLabels::Field> &fields, QWidget *parent = nullptr);
		QList<MacroLabels::Field> fields() const;

	public slots:
		void acceptAll();

	private:
		QList<MacroLabels::Field> m_fields;
		QTableWidget *m_table;
};

#endif // MACROLABELSDIALOG_H
