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
#ifndef ELEMENT_H
#define ELEMENT_H
#include <QtGui>
#include "terminal.h"
#include "orientationset.h"
class Diagram;
class ElementTextItem;
/**
	Cette classe abstraite represente un element electrique.
*/
class Element : public QObject, public QGraphicsItem {
	
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)
	
	// constructeurs, destructeur
	public:
	Element(QGraphicsItem * = 0, Diagram * = 0);
	virtual ~Element();
	
	private:
	Element(const Element &);
	
	// attributs
	public:
	enum { Type = UserType + 1000 };
	
	protected:
	/**
		orientations de l'element :
			* autorisations
			* orientation en cours
			* orientation par defaut
		@see OrientationSet
	*/
	OrientationSet ori;
	
	private:
	QSize   dimensions;
	QPoint  hotspot_coord;
	QPixmap preview;
	
	// methodes
	public:
	/**
		permet de caster un QGraphicsItem en Element avec qgraphicsitem_cast
		@return le type de QGraphicsItem
	*/
	virtual int type() const { return Type; }
	
	// methodes virtuelles pures a definir dans les classes enfants
	/// @return la liste des bornes de cet element
	virtual QList<Terminal *> terminals() const = 0;
	/// @return la liste des conducteurs relies a cet element
	virtual QList<Conductor *> conductors() const = 0;
	/// @return la liste des champs de textes de cet element
	virtual QList<ElementTextItem *> texts() const = 0;
	/// @return le nombre de bornes actuel de cet element
	virtual int terminalsCount() const = 0;
	/// @return le nombre de bornes minimum de cet element
	virtual int minTerminalsCount() const = 0;
	/// @return le nombre de bornes maximum de cet element
	virtual int maxTerminalsCount() const = 0;
	/**
		Dessine l'element
	*/
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *) = 0;
	/// @return L'ID du type de l'element
	virtual QString typeId() const = 0;
	/// @return Le nom de l'element
	virtual QString name() const = 0;
	Diagram *diagram() const;
	
	virtual bool isHighlighted() const;
	virtual void setHighlighted(bool);
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	QRectF boundingRect() const;
	QSize setSize(int, int);
	QSize size() const;
	QPixmap  pixmap();
	
	// methodes relatives au point de saisie
	QPoint setHotspot(QPoint);
	QPoint hotspot() const;
	
	// methodes relatives a la selection
	void select();
	void deselect();
	
	// methodes relatives a la position
	void setPos(const QPointF &);
	void setPos(qreal, qreal);
	
	// methodes relatives aux connexions internes
	bool internalConnections();
	void setInternalConnections(bool);
	
	// methodes relatives aux fichiers XML
	static bool valideXml(QDomElement &);
	virtual bool fromXml(QDomElement &, QHash<int, Terminal *> &, bool = false);
	virtual QDomElement toXml(QDomDocument &, QHash<Terminal *, int> &) const;
	
	// methodes d'acces aux possibilites d'orientation
	bool setOrientation(QET::Orientation o);
	const OrientationSet &orientation() const;
	
	protected:
	void drawAxes(QPainter *, const QStyleOptionGraphicsItem *);
	void mousePressEvent(QGraphicsSceneMouseEvent *);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	
	private:
	bool internal_connections;
	bool must_highlight_;
	bool first_move_;
	void drawSelection(QPainter *, const QStyleOptionGraphicsItem *);
	void drawHighlight(QPainter *, const QStyleOptionGraphicsItem *);
	void updatePixmap();
};

/**
	Permet de savoir si l'element accepte les connexions internes,
	c'est-a-dire que ses bornes peuvent etre reliees entre elles
	@return true si l'element accepte les connexions internes, false sinon
*/
inline bool Element::internalConnections() {
	return(internal_connections);
}

/**
	Permet de specifier si l'element accepte les connexions internes,
	c'est-a-dire que ses bornes peuvent etre reliees entre elles
	@param ic true pour que l'element accepte les connexions internes, false pour
	qu'il les interdise
*/
inline void Element::setInternalConnections(bool ic) {
	internal_connections = ic;
}

/**
	Permet de connaitre l'orientation actuelle de l'element
	@return L'orientation actuelle de l'element
*/
inline const OrientationSet & Element::orientation() const {
	return(ori);
}

#endif
