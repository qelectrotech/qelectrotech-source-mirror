#ifndef ELEMENTFIXE_H
	#define ELEMENTFIXE_H
	#include "element.h"
	class ElementFixe : public Element {
		public:
		ElementFixe(QGraphicsItem * = 0, Schema * = 0);
		int nbBornesMin() const;
		int nbBornesMax() const;
		virtual bool fromXml(QDomElement &, QHash<int, Borne *>&);
		virtual int nbBornes() const = 0;
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *) = 0;
		virtual QString typeId() = 0;
		virtual QString  nom() = 0;
	};
#endif
