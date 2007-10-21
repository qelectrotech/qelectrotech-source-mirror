#ifndef PART_LINE_H
#define PART_LINE_H
#include <QtGui>
#include "customelementgraphicpart.h"
class LineEditor;
/**
	Cette classe represente une ligne pouvant etre utilisee pour composer le
	dessin d'un element dans l'editeur d'element.
*/
class PartLine : public QGraphicsLineItem, public CustomElementGraphicPart {
	// constructeurs, destructeur
	public:
	PartLine(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartLine() {}
	
	private:
	PartLine(const PartLine &);
	
	// attributs
	private:
	LineEditor *informations;
	
	// methodes
	public:
	enum { Type = UserType + 1104 };
	virtual int type() const { return Type; }
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	virtual const QDomElement toXml(QDomDocument &) const;
	virtual void fromXml(const QDomElement &);
	virtual QPointF sceneP1() const;
	virtual QPointF sceneP2() const;
	virtual QPainterPath shape() const;
	virtual QRectF boundingRect() const;
	virtual void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
	
	private:
	QList<QPointF> fourShapePoints() const;
};
#endif
