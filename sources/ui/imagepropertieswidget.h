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
		void on_m_width_sb_valueChanged(double value);
		void on_m_height_sb_valueChanged(double value);
		void on_m_angle_sb_valueChanged(double value);
		void on_m_skew_x_sb_valueChanged(double value);
		void on_m_skew_y_sb_valueChanged(double value);
		void on_m_lock_ratio_tb_toggled(bool checked);
		void on_m_restore_ratio_pb_clicked();
		void on_m_lock_pos_cb_clicked();

	private:
		Ui::ImagePropertiesWidget *ui;
		DiagramImageItem *m_image;
		bool m_movable;
		// All tracked independently -- the image may already be
		// non-uniformly scaled, rotated, and/or skewed via the resize/
		// rotate/skew handles before this dialog is even opened, and
		// undo/reset have to restore each to whatever it actually was.
		qreal m_scaleX;
		qreal m_scaleY;
		qreal m_rotation;
		qreal m_skewX;
		qreal m_skewY;
		// Guards the width/height spinboxes' mutual updates when
		// "Conserver les proportions" is checked, so setting one
		// programmatically in response to the other doesn't re-trigger
		// its own valueChanged and recurse.
		bool m_updating_ratio = false;
};

#endif // IMAGEPROPERTIESWIDGET_H
