#ifndef POLYGON_EDITOR_H
#define POLYGON_EDITOR_H
#include "elementitemeditor.h"
class PartPolygon;
class PolygonEditor : public ElementItemEditor {
	
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	PolygonEditor(QETElementEditor *, PartPolygon *, QWidget * = 0);
	virtual ~PolygonEditor() {}
	
	private:
	PolygonEditor(const PolygonEditor &);
	
	// attributs
	private:
	PartPolygon *part;
	QTreeWidget points_list;
	QCheckBox close_polygon;
	
	// methodes
	private:
	QVector<QPointF> getPointsFromTree();
	
	public slots:
	void updatePolygon();
	void updatePolygonPoints();
	void updatePolygonClosedState();
	void updateForm();
	void validColumn(QTreeWidgetItem *qtwi, int column);
	
	private:
	void activeConnections(bool);
};
#endif
