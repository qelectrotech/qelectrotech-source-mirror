/*
	Copyright 2006-2019 The QElectroTech Team
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
		explicit ShapeGraphicsItemPropertiesWidget(QetShapeItem *item, QWidget *parent = nullptr);
		ShapeGraphicsItemPropertiesWidget(QList<QetShapeItem *> items_list, QWidget *parent =nullptr);
		~ShapeGraphicsItemPropertiesWidget() override;

		void setItem(QetShapeItem *shape);
		void setItems(QList<QetShapeItem *> shapes_list);

	public slots:
		void apply() override;
		void reset() override;
	public:
		QUndoCommand* associatedUndo() const override;
		QString title() const override { return tr("Éditer les propriétés d'une primitive "); }
		void updateUi() override;
		bool setLiveEdit(bool live_edit) override;

	private:
		void setUpEditConnection();

	private slots:
		void on_m_lock_pos_cb_clicked();
		void on_m_color_kpb_changed(const QColor &newColor);
		void on_m_brush_color_kpb_changed(const QColor &newColor);

		private:
		Ui::ShapeGraphicsItemPropertiesWidget *ui;
		QetShapeItem *m_shape;
		QList <QPointer<QetShapeItem>> m_shapes_list;
		QList <QMetaObject::Connection> m_connect_list,
										m_edit_connection;
};

#endif // SHAPEGRAPHICSITEMPROPERTIESWIDGET_H
