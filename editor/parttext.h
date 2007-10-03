#ifndef PART_TEXT
#define PART_TEXT
#include <QtGui>
#include "customelementpart.h"
class TextEditor;
class PartText : public QGraphicsTextItem, public CustomElementPart {
	// constructeurs, destructeur
	public:
	PartText(QETElementEditor *, QGraphicsItem * = 0, ElementScene * = 0);
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
	virtual void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	
	protected:
	virtual void focusOutEvent(QFocusEvent *);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
	virtual void keyReleaseEvent(QKeyEvent *);
	virtual QVariant itemChange(GraphicsItemChange, const QVariant &);
	QRectF boundingRect() const;
	
	private:
	QPointF margin() const;
};
#endif
