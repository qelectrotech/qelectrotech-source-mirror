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
#ifndef RECTANGLEEDITOR_H
#define RECTANGLEEDITOR_H

#include "elementitemeditor.h"
#include <QWidget>

class StyleEditor;
class PartRectangle;

namespace Ui {
	class RectangleEditor;
}

/**
 * @brief The RectangleEditor class
 * This class provides a widget to edit rectangles within the element editor.
 */
class RectangleEditor : public ElementItemEditor
{
	Q_OBJECT
	
	public:
		explicit RectangleEditor(QETElementEditor *editor, PartRectangle *rect = nullptr, QWidget *parent = nullptr);
		~RectangleEditor();
	
		bool setPart(CustomElementPart *part) override;
		CustomElementPart *currentPart() const override;
		QPointF editedTopLeft () const;
	
	public slots:
		void updateForm() override;
	private:
		void editingFinished();
		void activeConnections(bool active);
	
	private:
		bool m_locked = false;
		StyleEditor *m_style;
		PartRectangle *m_part;
		Ui::RectangleEditor *ui;
};

#endif // RECTANGLEEDITOR_H
