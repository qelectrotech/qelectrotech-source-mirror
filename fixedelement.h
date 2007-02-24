#ifndef ELEMENTFIXE_H
	#define ELEMENTFIXE_H
	#include "element.h"
	class FixedElement : public Element {
		public:
		FixedElement(QGraphicsItem * = 0, Diagram * = 0);
		int nbTerminalsMin() const;
		int nbTerminalsMax() const;
		virtual int nbTerminals() const = 0;
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *) = 0;
		virtual QString typeId() const = 0;
		virtual QString  nom() const = 0;
	};
#endif
