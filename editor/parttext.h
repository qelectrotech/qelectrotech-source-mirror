#ifndef PART_TEXT
#define PART_TEXT
#include <QtGui>
#include "customelementgraphicpart.h"
class TextEditor;
class PartText : public QGraphicsTextItem, public CustomElementPart {
	// constructeurs, destructeur
	public:
	PartText(QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartText();
	
	private:
	PartText(const PartText &);
	
	// attributs
	TextEditor *infos;
	
	// methodes
	public:
	void fromXml(const QDomElement &);
	const QDomElement toXml(QDomDocument &) const;
	QWidget *elementInformations();
	QPointF pos() const;
	void setPos(const QPointF &);
	void setPos(qreal, qreal);
	
	protected:
	virtual void focusOutEvent(QFocusEvent *);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
	virtual QVariant itemChange(GraphicsItemChange, const QVariant &);
	QRectF boundingRect() const;
	
	public:
	bool can_check_changes;
	
	private:
	QPointF margin() const;
};
#endif
