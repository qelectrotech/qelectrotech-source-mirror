/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef SHAPEGRAPHICSITEMPROPERTIESWIDGET_H
#define SHAPEGRAPHICSITEMPROPERTIESWIDGET_H

#include "PropertiesEditor/propertieseditorwidget.h"

namespace Ui {
	class ShapeGraphicsItemPropertiesWidget;
}

class QetShapeItem;

/**
 * @brief The ShapeGraphicsItemPropertiesWidget class
 * Provide a widget to edit the properties of a QetShapeItem
 */
class ShapeGraphicsItemPropertiesWidget : public PropertiesEditorWidget
{
		Q_OBJECT

	public:
		explicit ShapeGraphicsItemPropertiesWidget(QetShapeItem *item, QWidget *parent = 0);
		~ShapeGraphicsItemPropertiesWidget();

		void setItem(QetShapeItem *shape);

	public slots:
		virtual void apply();
		virtual void reset();
	public:
		virtual QUndoCommand* associatedUndo() const;
		virtual QString title() const { return tr("Éditer les propriétés d'une primitive "); }
		virtual void updateUi();
		virtual bool setLiveEdit(bool live_edit);

	private:
		void setPenColorButton(const QColor &color);
		void setBrushColorButton(const QColor &color);

	private slots:
		void on_m_lock_pos_cb_clicked();
		void on_m_color_pb_clicked();
		void on_m_brush_color_pb_clicked();

	private:
		Ui::ShapeGraphicsItemPropertiesWidget *ui;
		QetShapeItem *m_shape;
};

#endif // SHAPEGRAPHICSITEMPROPERTIESWIDGET_H
