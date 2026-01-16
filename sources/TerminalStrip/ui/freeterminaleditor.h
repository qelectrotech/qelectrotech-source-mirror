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
#ifndef FREETERMINALEDITOR_H
#define FREETERMINALEDITOR_H

#include <QWidget>
#include "../../qetproject.h"

class RealTerminal;
class FreeTerminalModel;
class QTableView;

namespace Ui {
	class FreeTerminalEditor;
}

class FreeTerminalEditor : public QWidget
{
		Q_OBJECT

	public:
		explicit FreeTerminalEditor(QETProject *project, QWidget *parent = nullptr);
		~FreeTerminalEditor();

		void reload();
		void apply();

        void setProject(QETProject *project);

	private slots:
		void on_m_type_cb_activated(int index);
		void on_m_function_cb_activated(int index);
		void on_m_led_cb_activated(int index);
		void on_m_move_pb_clicked();

	private:
		void selectionChanged();
		void setDisabledMove(bool b=true);

	private:
		Ui::FreeTerminalEditor *ui;
        QPointer <QETProject> m_project;
        FreeTerminalModel *m_model {nullptr};
};
#endif // FREETERMINALEDITOR_H
