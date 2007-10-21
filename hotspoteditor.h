#ifndef HOTSPOT_EDITOR_H
#define HOTSPOT_EDITOR_H
#include <QtGui>
#include "diagram.h"
/**
	Ce widget permet a l'utilisateur d'editer les dimensions ainsi que le point
	de saisie d'un element.
*/
class HotspotEditor : public QWidget {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	HotspotEditor(QWidget * = 0);
	virtual ~HotspotEditor() {};
	
	private:
	HotspotEditor(const HotspotEditor &);
	
	// attributs
	private:
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
	
	// methodes
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
	
	public slots:
	void updateScene();
	void updateHotspotLimits();
};
#endif
