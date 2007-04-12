#include "fixedelement.h"
/**
	Constructeur
*/
FixedElement::FixedElement(QGraphicsItem *parent, Diagram *scene) : Element(parent, scene) {
}

/**
	Destructeur
*/
FixedElement::~FixedElement() {
}

/**
	@return Le nombre minimal de bornes que l'element peut avoir
*/
int FixedElement::nbTerminalsMin() const {
	return(nbTerminals());
}

/**
	@return Le nombre maximal de bornes que l'element peut avoir
*/
int FixedElement::nbTerminalsMax() const {
	return(nbTerminals());
}
