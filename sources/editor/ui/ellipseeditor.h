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
#ifndef ELLIPSEEDITOR_H
#define ELLIPSEEDITOR_H

#include "elementitemeditor.h"

namespace Ui {
	class EllipseEditor;
}

class PartEllipse;
class StyleEditor;

/**
 * @brief The EllipseEditor class
 * This class provide a widget used to edit the properties of a @PartEllipse
 */
class EllipseEditor : public ElementItemEditor
{
    Q_OBJECT

	public:
		explicit EllipseEditor (QETElementEditor *editor, PartEllipse *part = nullptr, QWidget *parent = nullptr);
		~EllipseEditor() override;

		bool setPart(CustomElementPart *part) override;
		bool setParts(QList<CustomElementPart *> parts) override;
		CustomElementPart *currentPart() const override;
		QList<CustomElementPart*> currentParts() const override;
		void updateForm() override;

	private slots:
		void on_m_x_sb_editingFinished();
		void on_m_y_sb_editingFinished();
		void on_m_horizontal_diameter_sb_editingFinished();
		void on_m_vertical_diameter_sb_editingFinished();

	private:
		void updateRect();
		void setUpChangeConnections();
		void disconnectChangeConnections();

	private:
		Ui::EllipseEditor *ui;
		PartEllipse *m_part = nullptr;
		StyleEditor *m_style = nullptr;
		QList <QMetaObject::Connection> m_change_connections;

};

#endif // ELLIPSEEDITOR_H
