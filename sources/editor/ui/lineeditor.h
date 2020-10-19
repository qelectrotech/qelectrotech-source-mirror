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
#ifndef LINEEDITOR_H
#define LINEEDITOR_H

#include "elementitemeditor.h"

namespace Ui {
    class LineEditor;
}

class PartLine;
class StyleEditor;

/**
 * @brief The LineEditor class
 * Provide a widget editor used to edit the properties of a PartLine
 */
class LineEditor : public ElementItemEditor
{
     Q_OBJECT

    public:
	explicit LineEditor(QETElementEditor *editor, PartLine *part = nullptr, QWidget *parent = nullptr);
	~LineEditor() override;

		bool setPart(CustomElementPart *part)            override;
		bool setParts(QList <CustomElementPart *> parts) override;
		CustomElementPart *currentPart()           const override;
		QList<CustomElementPart *> currentParts()  const override;
		void updateForm()                                override;

	private:
		void setUpChangeConnections();
		void disconnectChangeConnections();
		void activeConnection(bool active);

		void lineEdited();
		void firstEndEdited();
		void firstEndLenghtEdited();
		void secondEndEdited();
		void secondEndLenghtEdited();

	private:
		PartLine *m_part = nullptr;
	Ui::LineEditor *ui;
		StyleEditor *m_style = nullptr;
		QList <QMetaObject::Connection> m_change_connections;
		bool m_locked = false;
};

#endif // LINEEDITOR_H
