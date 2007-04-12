#ifndef ELEMENTFIXE_H
#define ELEMENTFIXE_H
#include "element.h"
/**
	Cette classe represente un element avec un nombre fixe de bornes.
*/
class FixedElement : public Element {
	
	// constructeurs, destructeur
	public:
	FixedElement(QGraphicsItem * = 0, Diagram * = 0);
	virtual ~FixedElement();
	
	// methodes
	public:
	int nbTerminalsMin() const;
	int nbTerminalsMax() const;
	virtual int nbTerminals() const = 0;
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *) = 0;
	virtual QString typeId() const = 0;
	virtual QString  nom() const = 0;
};
#endif
