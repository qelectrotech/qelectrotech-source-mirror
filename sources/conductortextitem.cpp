#include "conductortextitem.h"
#include "conductor.h"

/**
	Constructeur
	@param parent_conductor  Conducteur auquel ce texte est rattache
	@param parent_diagram    Schema auquel ce texte et son conducteur parent sont rattaches
*/
ConductorTextItem::ConductorTextItem(Conductor *parent_conductor, Diagram *parent_diagram) :
	DiagramTextItem(parent_conductor, parent_diagram),
	parent_conductor_(parent_conductor)
{
	// par defaut, les DiagramTextItem sont Selectable et Movable
	// on desactive Movable pour les textes des conducteurs
	setFlag(QGraphicsItem::ItemIsMovable, false);
}

/**
	Constructeur
	@param text Le texte affiche par le champ de texte
	@param parent_conductor  Conducteur auquel ce texte est rattache
	@param parent_diagram    Schema auquel ce texte et son conducteur parent sont rattaches
*/
ConductorTextItem::ConductorTextItem(const QString &text, Conductor *parent_conductor, Diagram *parent_diagram) :
	DiagramTextItem(text, parent_conductor, parent_diagram),
	parent_conductor_(parent_conductor)
{
	// par defaut, les DiagramTextItem sont Selectable et Movable
	// on desactive Movable pour les textes des conducteurs
	setFlag(QGraphicsItem::ItemIsMovable, false);
}

/**
	Destructeur
*/
ConductorTextItem::~ConductorTextItem() {
}

/**
	@return le conducteur parent de ce champ de texte, ou 0 si celui-ci n'en a
	pas
*/
Conductor *ConductorTextItem::parentConductor() const {
	return(parent_conductor_);
}

/**
	Permet de lire le texte a mettre dans le champ a partir d'un element XML.
	Cette methode se base sur la position du champ pour assigner ou non la
	valeur a ce champ.
	@param e L'element XML representant le champ de texte
*/
void ConductorTextItem::fromXml(const QDomElement &e) {
	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setPlainText(e.attribute("text"));
	setRotationAngle(e.attribute("rotation").toDouble());
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant ce champ de texte
*/
QDomElement ConductorTextItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("input");
	result.setAttribute("x", QString("%1").arg(pos().x()));
	result.setAttribute("y", QString("%1").arg(pos().y()));
	result.setAttribute("text", toPlainText());
	if (rotationAngle()) {
		result.setAttribute("rotation", QString("%1").arg(rotationAngle()));
	}
	return(result);
}
