/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef IMAGEPROPERTIESWIDGET_H
#define IMAGEPROPERTIESWIDGET_H

#include <QWidget>
#include "PropertiesEditor/propertieseditorwidget.h"

class DiagramImageItem;

namespace Ui {
	class ImagePropertiesWidget;
}

/**
 * @brief The ImagePropertiesWidget class
 * This class provide a widget to edit the propertie of a DiagramImageItem
 */
class ImagePropertiesWidget : public PropertiesEditorWidget
{
		Q_OBJECT

	public:
		explicit ImagePropertiesWidget(DiagramImageItem *image = nullptr, QWidget *parent = 0);
		~ImagePropertiesWidget();
		void setImageItem (DiagramImageItem *image);

		void apply();
		void reset();
		QUndoCommand* associatedUndo();

	private:
		void updateUi();

	private slots:
		void on_m_scale_slider_valueChanged(int value);
		void imageWasDeleted();

		void on_m_lock_pos_cb_clicked();

	private:
		Ui::ImagePropertiesWidget *ui;
		DiagramImageItem *m_image;
		bool m_movable;
		qreal m_scale;
};

#endif // IMAGEPROPERTIESWIDGET_H
