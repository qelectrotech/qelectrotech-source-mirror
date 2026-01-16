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
#ifndef TERMINALEDITOR_H
#define TERMINALEDITOR_H

#include <QWidget>
#include "../elementitemeditor.h"

namespace Ui {
	class TerminalEditor;
}

class PartTerminal;

/**
 * @brief The TerminalEditor class
 * Provide a widget used to edit the properties of a PartTerminal
 */
class TerminalEditor : public ElementItemEditor
{
	Q_OBJECT

	public:
		TerminalEditor(QETElementEditor *editor, QWidget *parent = nullptr);
		~TerminalEditor() override;

		void updateForm() override;
		bool setPart(CustomElementPart *new_part) override;
		CustomElementPart *currentPart() const override;
		QList<CustomElementPart *> currentParts() const override {return QList<CustomElementPart *>();}

	private:
		void init();
		void posEdited();
		void orientationEdited();
		void nameEdited();
		void typeEdited();
		void activeConnections(bool active);
		void activeChangeConnections(bool active);

	private:
		Ui::TerminalEditor *ui;
		QVector<QMetaObject::Connection> m_editor_connections,
										 m_change_connections;
		PartTerminal *m_part = nullptr;
		bool m_locked = false;
};

#endif // TERMINALEDITOR_H
