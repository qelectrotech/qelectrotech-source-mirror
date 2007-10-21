#ifndef PART_CIRCLE_H
#define PART_CIRCLE_H
#include <QtGui>
#include "customelementgraphicpart.h"
class CircleEditor;
/**
	Cette classe represente un cercle pouvant etre utilise pour composer le
	dessin d'un element dans l'editeur d'element.
*/
class PartCircle : public QGraphicsEllipseItem, public CustomElementGraphicPart {
	// constructeurs, destructeur
	public:
	PartCircle(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartCircle() {}
	
	private:
	PartCircle(const PartCircle &);
	
	// attributs
	private:
	CircleEditor *informations;
	
	// methodes
	public:
	enum { Type = UserType + 1102 };
	virtual int type() const { return Type; }
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	virtual const QDomElement toXml(QDomDocument &) const;
	virtual void fromXml(const QDomElement &);
	virtual QPointF sceneTopLeft() const;
	virtual QRectF boundingRect() const;
	QPointF sceneCenter() const;
	virtual void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
};
#endif
