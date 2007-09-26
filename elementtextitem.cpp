#include "elementtextitem.h"
#include "diagram.h"
#include "diagramcommands.h"

/**
	Constructeur
	@param parent Le QGraphicsItem parent du champ de texte
	@param scene La scene a laquelle appartient le champ de texte
*/
ElementTextItem::ElementTextItem(QGraphicsItem *parent, QGraphicsScene *scene) :
	DiagramTextItem(parent, scene),
	follow_parent_rotations(false)
{
}

/**
	Constructeur
	@param parent Le QGraphicsItem parent du champ de texte
	@param scene La scene a laquelle appartient le champ de texte
	@param text Le texte affiche par le champ de texte
*/
ElementTextItem::ElementTextItem(const QString &text, QGraphicsItem *parent, QGraphicsScene *scene) :
	DiagramTextItem(text, parent, scene),
	follow_parent_rotations(false)
{
}

/// Destructeur
ElementTextItem::~ElementTextItem() {
}

/**
	Modifie la position du champ de texte
	@param pos La nouvelle position du champ de texte
*/
void ElementTextItem::setPos(const QPointF &pos) {
	QPointF actual_pos = pos;
	actual_pos -= QPointF(0.0, boundingRect().height() / 2.0);
	DiagramTextItem::setPos(actual_pos);
}

/**
	Modifie la position du champ de texte
	@param x La nouvelle abscisse du champ de texte
	@param y La nouvelle ordonnee du champ de texte
*/
void ElementTextItem::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

/**
	@return La position (bidouillee) du champ de texte
*/
QPointF ElementTextItem::pos() const {
	QPointF actual_pos = DiagramTextItem::pos();
	actual_pos += QPointF(0.0, boundingRect().height() / 2.0);
	return(actual_pos);
}

/**
	Permet de lire le texte a mettre dans le champ a partir d'un element XML.
	Cette methode se base sur la position du champ pour assigner ou non la
	valeur a ce champ.
	@param e L'element XML representant le champ de texte
*/
void ElementTextItem::fromXml(const QDomElement &e) {
	QPointF _pos = pos();
	if (e.attribute("x").toDouble() == _pos.x() && e.attribute("y").toDouble() == _pos.y()) {
		setPlainText(e.attribute("text"));
		previous_text = e.attribute("text");
	}
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant ce champ de texte
*/
QDomElement ElementTextItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("input");
	result.setAttribute("x", pos().x());
	result.setAttribute("y", pos().y());
	result.setAttribute("text", toPlainText());
	return(result);
}
