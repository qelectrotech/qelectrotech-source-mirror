#ifndef PART_ELLIPSE_H
#define PART_ELLIPSE_H
#include <QtGui>
#include "customelementgraphicpart.h"
class EllipseEditor;
class PartEllipse : public QGraphicsEllipseItem, public CustomElementGraphicPart {
	// constructeurs, destructeur
	public:
	PartEllipse(QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartEllipse() {
		qDebug() << "~PartEllipse()";
	}
	
	private:
	PartEllipse(const PartEllipse &);
	
	// attributs
	private:
	EllipseEditor *informations;
	
	// methodes
	public:
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	virtual const QDomElement toXml(QDomDocument &) const;
	virtual void fromXml(const QDomElement &);
	virtual QPointF sceneTopLeft() const;
	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
};
#endif
