/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef HOTSPOT_EDITOR_H
#define HOTSPOT_EDITOR_H
#include <QtGui>
#include "diagram.h"
/**
	This widget enable users to edit the dimensions and the hotspot of an
	electrical element.
*/
class HotspotEditor : public QWidget {
	Q_OBJECT
	
	// constructors, destructor
	public:
	HotspotEditor(QWidget * = 0);
	virtual ~HotspotEditor() {};
	
	private:
	HotspotEditor(const HotspotEditor &);
	
	// attributes
	private:
	QLabel *informations_label_;
	QSpinBox *sb_width;
	QSpinBox *sb_height;
	QSpinBox *sb_hotspot_x;
	QSpinBox *sb_hotspot_y;
	QCheckBox *hotspot_sync;
	Diagram *diagram_scene;
	QGraphicsView *diagram_view;
	QRectF parts_rect;
	QPoint old_hotspot;
	bool parts_rect_enabled;
	QVBoxLayout *vlayout;
	
	// methods
	public:
	void setElementWidth(uint);
	void setElementHeight(uint);
	uint elementWidth10px() const;
	uint elementWidth() const;
	uint elementHeight10px() const;
	uint elementHeight() const;
	void setElementSize(const QSize &);
	QSize elementSize10px() const;
	QSize elementSize() const;
	void setHotspot(const QPoint &);
	QPoint hotspot() const;
	void setOldHotspot(const QPoint &);
	QPoint oldHotspot() const;
	void setPartsRect(const QRectF &);
	QRectF partsRect() const;
	void setPartsRectEnabled(bool);
	bool partsRectEnabled();
	bool mustTranslateParts() const;
	QPoint offsetParts() const;
	bool isReadOnly() const;
	void setReadOnly(bool);
	
	public slots:
	void updateScene();
	void updateHotspotLimits();
};
#endif
