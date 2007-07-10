#ifndef EDITOR_SCNE_H
#define EDITOR_SCNE_H
#include <QtGui>
#include <QtXml>
#include "nameslistwidget.h"
#include "orientationsetwidget.h"
class PartLine;
class PartEllipse;
class PartCircle;
class PartPolygon;
class PartArc;
class EditorScene : public QGraphicsScene {
	Q_OBJECT
	
	// enum
	enum Behavior { Normal, Line, Circle, Ellipse, Polygon, Text, Terminal, Arc, TextField };
	
	// constructeurs, destructeur
	public:
	EditorScene(QObject * = 0);
	virtual ~EditorScene();
	
	private:
	EditorScene(const EditorScene &);
	
	// attributs
	private:
	/// longueur de l'element en dizaines de pixels
	uint _width;
	/// hauteur de l'element en dizaines de pixels
	uint _height;
	/// position du point de saisie
	QPoint _hotspot;
	/// Liste des noms de l'element
	NamesList _names;
	/// Liste des orientations de l'element
	OrientationSet ori;
	
	/// Variables relatives a la gestion du dessin des parties sur la scene
	Behavior behavior;
	PartLine *current_line;
	PartEllipse *current_ellipse;
	PartCircle *current_circle;
	PartPolygon *current_polygon;
	PartArc *current_arc;
	
	// methodes
	public:
	void setWidth(const uint& theValue);
	uint width() const;
	void setHeight(const uint& theValue);
	uint height() const;
	void setHotspot(const QPoint &);
	QPoint hotspot() const;
	void setNames(const NamesList);
	NamesList names() const;
	OrientationSet orientations();
	void setOrientations(const OrientationSet &);
	virtual const QDomDocument toXml() const;
	virtual void fromXml(const QDomDocument &);
	
	protected:
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	virtual void drawBackground(QPainter *, const QRectF &);
	virtual void drawForeground(QPainter *, const QRectF &);
	
	public slots:
	void slot_move();
	void slot_addLine();
	void slot_addCircle();
	void slot_addEllipse();
	void slot_addPolygon();
	void slot_addText();
	void slot_addArc();
	void slot_addTerminal();
	void slot_addTextField();
	void slot_checkSelectionChanged();
	void slot_selectAll();
	void slot_deselectAll();
	void slot_invertSelection();
	void slot_delete();
	void slot_editSize();
	void slot_editHotSpot();
	void slot_editNames();
	void slot_editOrientations();
	
	signals:
	void selectionChanged();
	void needNormalMode();
};

inline void EditorScene::setWidth(const uint &wid) {
	_width = wid;
	while (_width % 10) ++ _width;
	_width /= 10;
}

inline uint EditorScene::width() const {
	return(_width * 10);
}

inline void EditorScene::setHeight(const uint &hei) {
	_height = hei;
	while (_height % 10) ++ _height;
	_height /= 10;
}

inline uint EditorScene::height() const {
	return(_height * 10);
}

inline void EditorScene::setHotspot(const QPoint &hs) {
	_hotspot = hs;
}

inline QPoint EditorScene::hotspot() const {
	return(_hotspot);
}

inline void EditorScene::setNames(const NamesList nameslist) {
	_names = nameslist;
}

inline NamesList EditorScene::names() const {
	return(_names);
}

inline OrientationSet EditorScene::orientations() {
	return(ori);
}

inline void EditorScene::setOrientations(const OrientationSet &orientation_set) {
	ori = orientation_set;
}

#endif
