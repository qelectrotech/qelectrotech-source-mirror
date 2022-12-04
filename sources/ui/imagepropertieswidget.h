/*
	Copyright 2006-2021 The QElectroTech Team
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

#include "../PropertiesEditor/propertieseditorwidget.h"

class DiagramImageItem;

namespace Ui {
	class ImagePropertiesWidget;
}

/**
	@brief The ImagePropertiesWidget class
	This class provides a widget to edit the properties of a DiagramImageItem
*/
class ImagePropertiesWidget : public PropertiesEditorWidget
{
		Q_OBJECT

	public:
		explicit ImagePropertiesWidget(DiagramImageItem *image = nullptr, QWidget *parent = nullptr);
		~ImagePropertiesWidget() override;
		void setImageItem (DiagramImageItem *image);

		void apply() override;
		void reset() override;
		bool setLiveEdit(bool live_edit) override;
		QUndoCommand* associatedUndo() const override;

	private:
		void updateUi() override;

	private slots:
		void on_m_scale_slider_valueChanged(int value);
		void on_m_lock_pos_cb_clicked();

	private:
		Ui::ImagePropertiesWidget *ui;
		DiagramImageItem *m_image;
		bool m_movable;
		qreal m_scale;
};

#endif // IMAGEPROPERTIESWIDGET_H
