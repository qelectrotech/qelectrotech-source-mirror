#ifndef QETSHAPEITEM_H
#define QETSHAPEITEM_H

#include "qetgraphicsitem.h"

class QetShapeItem : public QetGraphicsItem
{
	public:

	enum ShapeType {
		Line = 0,
		Rectangle,
		Ellipse
	};

	QetShapeItem(QPointF, QPointF = QPointF(0,0), ShapeType = Line, bool lineAngle = false, QGraphicsItem *parent = 0);
	virtual ~QetShapeItem();

	// attributes
	public:
	enum { Type = UserType + 1008 };

	// methods
	public:
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		QetShapeItem
		@return the QGraphicsItem type
	*/
	virtual int type() const { return Type; }

	void setStyle(Qt::PenStyle);
	Qt::PenStyle getStyle() const		{ return _shapeStyle;	   }
	ShapeType getType()		const		{ return _shapeType;	   }
	void setBoundingRect(QRectF rec)	{ _boundingRect = rec;	   }
	void setLineAngle(bool lineAngle)	{ _lineAngle = lineAngle;  }
	void setFullyBuilt(bool isBuilt);
	QLineF *getLine();
	QRectF *getRectangle();
	QRectF *getEllipse();
	virtual bool fromXml(const QDomElement &);
	virtual QDomElement toXml(QDomDocument &document) const;
	void setWritingXml(bool writing)	{ _writingXml = writing;   }
	virtual void editProperty();

	private:
	ShapeType    _shapeType;
	Qt::PenStyle _shapeStyle;
	QRectF       _boundingRect;
	bool		 _lineAngle;  // false if line from topleft corner to bottomright corner
							  // and true if line from topright corner to bottomleft corner
	bool		 _isFullyBuilt;
	QPointF		 _lineP1;
	QPointF		 _lineP2;
	bool		_writingXml;

	protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QRectF boundingRect() const;
	QPainterPath shape() const;

};

#endif // QETSHAPEITEM_H
