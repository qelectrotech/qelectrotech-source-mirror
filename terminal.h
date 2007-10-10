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
	// permet de caster un QGraphicsItem en Borne avec qgraphicsitem_cast
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
	static const qreal terminalSize;
	
	// differentes couleurs statiques utilisables pour l'effet "hover"
	static QColor couleur_neutre;
	static QColor couleur_autorise;
	static QColor couleur_prudence;
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
