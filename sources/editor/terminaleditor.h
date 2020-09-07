/*
	Copyright 2006-2020 The QElectroTech Team
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
#ifndef TERMINAL_EDITOR_H
#define TERMINAL_EDITOR_H

#include "elementitemeditor.h"

#include <QLineEdit>

class PartTerminal;
class QDoubleSpinBox;
class QComboBox;

/**
	@brief The TerminalEditor class
	This class provides a widget to edit terminals within the element editor.
	The class is capable to change the values of multiple parts of the same time.
	The displayed values are from the first selected element
*/
class TerminalEditor : public ElementItemEditor {
	Q_OBJECT

	// Constructors, destructor
	public:
		TerminalEditor(
				QETElementEditor *,
				QList<PartTerminal *>& terms,
				QWidget * = nullptr);
		TerminalEditor(QETElementEditor *, QWidget * = nullptr);
		~TerminalEditor() override;
	private:
		TerminalEditor(const TerminalEditor &);
		void init();

	// attributes
	private:
		QList<PartTerminal *> m_terminals;
		PartTerminal *m_part{nullptr};
		QDoubleSpinBox *qle_x, *qle_y;
		QComboBox *orientation;
		QLineEdit *name;
		bool m_locked{false};

	// methods
	public:
		bool setPart(CustomElementPart *) override;
		bool setParts(QList<CustomElementPart *> parts) override;
		CustomElementPart *currentPart() const override;
		QList<CustomElementPart*> currentParts() const override;

	public slots:
		void updateTerminalO();
		void updateXPos();
		void updateYPos();
		void updateName();
		void updateForm() override;

	private:
		void activeConnections(bool);
};
#endif
