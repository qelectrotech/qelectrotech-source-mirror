#ifndef PART_LINE_H
#define PART_LINE_H
#include <QtGui>
#include "customelementgraphicpart.h"
class LineEditor;
class PartLine : public QGraphicsLineItem, public CustomElementGraphicPart {
	// constructeurs, destructeur
	public:
	PartLine(QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartLine() {
		qDebug() << "~PartLine()";
	}
	
	private:
	PartLine(const PartLine &);
	
	// attributs
	private:
	LineEditor *informations;
	
	// methodes
	public:
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	virtual const QDomElement toXml(QDomDocument &) const;
	virtual void fromXml(const QDomElement &);
	virtual QPointF sceneP1() const;
	virtual QPointF sceneP2() const;
	virtual QPainterPath shape() const;
	virtual QRectF boundingRect() const;
	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
	
	private:
	QList<QPointF> fourShapePoints() const;
};
#endif
