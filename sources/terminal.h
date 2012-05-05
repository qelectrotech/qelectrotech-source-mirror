/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef TERMINAL_H
#define TERMINAL_H
#include <QtGui>
#include <QtXml>
#include "qet.h"
class Conductor;
class Diagram;
class Element;
/**
	Cette classe represente une borne d'un element, c'est-a-dire un
	branchement possible pour un conducteur.
*/
class Terminal : public QGraphicsItem {
	
	// constructeurs, destructeur
	public:
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
	int conductorsCount() const;
	Diagram *diagram() const;
	Element *parentElement() const;
	
	// methodes de lecture
	QList<Conductor *> conductors() const;
	QET::Orientation orientation() const;
	QPointF dockConductor() const;
	void updateConductor();
	bool isLinkedTo(Terminal *);
	bool canBeLinkedTo(Terminal *);
	
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
	static QColor neutralColor;
	/// couleur indiquant une action autorisee
	static QColor allowedColor;
	/// couleur indiquant une action autorisee mais pas forcement recommandee
	static QColor warningColor;
	/// couleur indiquant une action interdite
	static QColor forbiddenColor;
	
	// attributs prives
	private:
	/// Pointeur vers l'element parent
	Element *parent_element_;
	/// coordonnees du point d'amarrage du conducteur
	QPointF dock_conductor_;
	/// coordonnees du point d'amarrage de l'element
	QPointF dock_elmt_;
	/// orientation de la borne
	QET::Orientation ori_;
	/// liste des conductors lies a cette borne
	QList<Conductor *> conductors_;
	/// pointeur vers un rectangle correspondant au bounding rect
	/// permet de ne calculer le bounding rect qu'une seule fois
	/// le pointeur c'est parce que le compilo exige une methode const
	QRectF *br_;
	/// Derniere borne mise en contact avec celle-ci
	Terminal *previous_terminal_;
	/// Booleen indiquant si le pointeur est au-dessus de la borne ou non
	bool hovered_;
	/// couleur de l'effet hover de la borne
	QColor hovered_color_;
	
	// methodes privees
	private:
	// methode initialisant les differents membres de la borne
	void init(QPointF, QET::Orientation);
};

/**
	@return Le nombre de conducteurs associes a la borne
*/
inline int Terminal::conductorsCount() const {
	return(conductors_.size());
}

/**
	@return La position du point d'amarrage de la borne
*/
inline QPointF Terminal::dockConductor() const {
	return(mapToScene(dock_conductor_));
}

#endif
