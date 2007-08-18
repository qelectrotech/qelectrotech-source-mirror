#ifndef PART_TEXTFIELD
#define PART_TEXTFIELD
#include <QtGui>
#include "customelementpart.h"
class TextFieldEditor;
class PartTextField : public QGraphicsTextItem, public CustomElementPart {
	// constructeurs, destructeur
	public:
	PartTextField(QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartTextField();
	
	private:
	PartTextField(const PartTextField &);
	
	// attributs
	TextFieldEditor *infos;
	bool follow_parent_rotations;
	
	// methodes
	public:
	void fromXml(const QDomElement &);
	const QDomElement toXml(QDomDocument &) const;
	QWidget *elementInformations();
	QPointF pos() const;
	void setPos(const QPointF &);
	void setPos(qreal, qreal);
	bool followParentRotations();
	void setFollowParentRotations(bool);
	
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
