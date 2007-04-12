#ifndef ELEMENT_TEXT_ITEM_H
#define ELEMENT_TEXT_ITEM_H
#include <QGraphicsTextItem>
#include <QtXml>
/**
	Cette classe represente un element de texte editable.
	Il est possible pour ce champ de texte de rester dans le sens de la lecture
	malgre les rotations de son element parent.
*/
class ElementTextItem : public QGraphicsTextItem {
	// constructeurs, destructeur
	public:
	ElementTextItem(QGraphicsItem * = 0, QGraphicsScene * = 0);
	ElementTextItem(const QString &, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~ElementTextItem();
	
	// attributs
	public:
	enum { Type = UserType + 1003 };
	
	private:
	bool follow_parent_rotations;
	
	// methodes
	public:
	virtual int type() const { return Type; }
	bool followParentRotations() const;
	void setFollowParentRotations(bool);
	void fromXml(const QDomElement &);
	QDomElement toXml(QDomDocument &) const;
	void setPos(const QPointF &);
	void setPos(qreal, qreal);
	QPointF pos() const;
};

/**
	Permet de savoir si le champ de texte suit les rotations de son parent.
	@return true si le champ de texte suit les rotations de son parent, false
	sinon
*/
inline bool ElementTextItem::followParentRotations() const {
	return(follow_parent_rotations);
}

/**
	Permet de specifier si le champ de texte suit les rotations de son parent.
	@param frp true si le champ de texte doit suivre  les rotations de son
	parent, false pour qu'ils ne les suivent pas
*/
inline void ElementTextItem::setFollowParentRotations(bool frp) {
	follow_parent_rotations = frp;
}

#endif
