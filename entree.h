#ifndef ENTREE_H
	#define ENTREE_H
	#include "elementfixe.h"
	/**
		Cette classe herite de la classe Element Fixe pour definir une
		entree.
	*/
	class Entree : public ElementFixe{
		public:
		Entree(QGraphicsItem * = 0, Schema * = 0);
		virtual int nbBornes() const;
		void paint(QPainter *, const QStyleOptionGraphicsItem *);
		QString typeId();
		QString nom() { return("Entr\351e"); }
	};
#endif
