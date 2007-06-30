#ifndef POLYGON_EDITOR_H
#define POLYGON_EDITOR_H
#include <QtGui>
class PartPolygon;
class PolygonEditor : public QWidget {
	
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	PolygonEditor(PartPolygon *, QWidget * = 0);
	~PolygonEditor() {
		qDebug() << "~PolygonEditor()";
	}
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
};
#endif
