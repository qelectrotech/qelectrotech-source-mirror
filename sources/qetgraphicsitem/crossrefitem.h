#ifndef CROSSREFITEM_H
#define CROSSREFITEM_H

#include "qetgraphicsitem/qetgraphicsitem.h"
class element;


class CrossRefItem : public QetGraphicsItem
{
	Q_OBJECT

	//Methods
	public:
	explicit CrossRefItem(Element *elmt, QetGraphicsItem *parent = 0);
	~CrossRefItem();

	QRectF boundingRect() const;

	signals:

	public slots:
	void updateLabel();
	void autoPos();

	protected:
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *e);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *e);

	private:
	void fillCrossRef(QPainter *painter);

	//Attributes
	private:
	Element *element_; //element to display the cross reference
	QPicture drawing_;
	bool b;

};

#endif // CROSSREFITEM_H
