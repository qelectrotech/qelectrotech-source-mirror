#ifndef PART_TEXTFIELD_H
#define PART_TEXTFIELD_H
#include <QtGui>
#include "customelementpart.h"
class TextFieldEditor;
class QETElementEditor;
/**
	Cette classe represente un champ de texte editable pouvant etre utilise
	pour composer le dessin d'un element dans l'editeur d'element.
	L'utilisateur peut specifier un valeur par defaut. Le champ sera editable
	lorsque l'element sera pose sur un schema.
*/
class PartTextField : public QGraphicsTextItem, public CustomElementPart {
	// constructeurs, destructeur
	public:
	PartTextField(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartTextField();
	
	private:
	PartTextField(const PartTextField &);
	
	// attributs
	TextFieldEditor *infos;
	bool follow_parent_rotations;
	
	// methodes
	public:
	enum { Type = UserType + 1108 };
	virtual int type() const { return Type; }
	void fromXml(const QDomElement &);
	const QDomElement toXml(QDomDocument &) const;
	QWidget *elementInformations();
	QPointF pos() const;
	void setPos(const QPointF &);
	void setPos(qreal, qreal);
	bool followParentRotations();
	void setFollowParentRotations(bool);
	virtual void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	
	protected:
	virtual void focusOutEvent(QFocusEvent *);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
	virtual QVariant itemChange(GraphicsItemChange, const QVariant &);
	QRectF boundingRect() const;
	
	private:
	QPointF margin() const;
};
#endif
