#ifndef QETSHAPEITEM_H
#define QETSHAPEITEM_H

#include "qetgraphicsitem.h"

class QetShapeItem : public QetGraphicsItem
{
	public:

	enum ShapeType {
		Line,
		Rectangle,
		Ellipse
	};

	QetShapeItem(QPointF, QPointF, ShapeType, bool lineAngle = false, QGraphicsItem *parent = 0);
	virtual ~QetShapeItem();

	private:
	ShapeType    _shapeType;
	Qt::PenStyle _shapeStyle;
	QRectF       _boundingRect;
	bool		 _lineAngle;  // false if line from topleft corner to bottomright corner
							  // and true if line from topright corner to bottomleft corner

	void setStyle(Qt::PenStyle);
	virtual void editProperty() {}

	protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QRectF boundingRect() const;

};

#endif // QETSHAPEITEM_H
