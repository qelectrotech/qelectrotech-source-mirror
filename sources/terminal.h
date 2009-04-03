/*
	Copyright 2006-2009 Xavier Guerrin
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BORNE_H
#define BORNE_H
#define TAILLE_BORNE 4
#include <QtGui>
#include <QtXml>
#include "qet.h"
class Conductor;
class Diagram;
class Element;
/**
	Classe modelisant la « borne » d'un appareil, c'est-a-dire un
	branchement possible pour un Conducteur.
*/
class Terminal : public QGraphicsItem {
	
	// constructeurs, destructeur
	public:
	Terminal();
	Terminal(QPointF,      QET::Orientation, Element * = 0, Diagram * = 0);
	Terminal(qreal, qreal, QET::Orientation, Element * = 0, Diagram * = 0);
	virtual ~Terminal();
	
	private:
	Terminal(const Terminal &);
	
	// methodes
	public:
	/**
		permet de caster un QGraphicsItem en Terminal avec qgraphicsitem_cast
		@return le type de QGraphicsItem
	*/
	virtual int type() const { return Type; }
	
	// implementation des methodes virtuelles pures de QGraphicsItem
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	QRectF boundingRect() const;
	
	// methodes de manipulation des conducteurs lies a cette borne
	bool addConductor(Conductor *);
	void removeConductor(Conductor *);
	int nbConductors() const;
	Diagram *diagram() const;
	
	// methodes de lecture
	QList<Conductor *> conductors() const;
	QET::Orientation orientation() const;
	QPointF amarrageConductor() const;
	void updateConductor(QPointF = QPointF());
	
	// methodes relatives a l'import/export au format XML
	static bool valideXml(QDomElement  &);
	bool fromXml (QDomElement &);
	QDomElement toXml (QDomDocument &) const;
	
	protected:
	// methodes de gestion des evenements
	void hoverEnterEvent  (QGraphicsSceneHoverEvent *);
	void hoverMoveEvent   (QGraphicsSceneHoverEvent *);
	void hoverLeaveEvent  (QGraphicsSceneHoverEvent *);
	void mousePressEvent  (QGraphicsSceneMouseEvent *);
	void mouseMoveEvent   (QGraphicsSceneMouseEvent *);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	
	// attributs
	public:
	enum { Type = UserType + 1002 };
	/// Longueur d'une borne
	static const qreal terminalSize;
	
	// differentes couleurs statiques utilisables pour l'effet "hover"
	/// couleur par defaut
	static QColor couleur_neutre;
	/// couleur indiquant une action autorisee
	static QColor couleur_autorise;
	/// couleur indiquant une action autorisee mais pas forcement recommandee
	static QColor couleur_prudence;
	/// couleur indiquant une action interdite
	static QColor couleur_interdit;
	
	private:
	// coordonnees des points d'amarrage
	QPointF amarrage_conductor;
	QPointF amarrage_elmt;
	// orientation de la borne
	QET::Orientation sens;
	// liste des conductors lies a cette borne
	QList<Conductor *> liste_conductors;
	// pointeur vers un rectangle correspondant au bounding rect ; permet de ne calculer le bounding rect qu'une seule fois ; le pointeur c'est parce que le compilo exige une methode const
	QRectF *br;
	Terminal *terminal_precedente;
	bool hovered;
	// methode initialisant les differents membres de la borne
	void initialise(QPointF, QET::Orientation);
	// couleur de l'effet hover de la patte
	QColor couleur_hovered;
};

/**
	@return Le nombre de conducteurs associes a la borne
*/
inline int Terminal::nbConductors() const {
	return(liste_conductors.size());
}

/**
	@return La position du point d'amarrage de la borne
*/
inline QPointF Terminal::amarrageConductor() const {
	return(mapToScene(amarrage_conductor));
}

#endif
