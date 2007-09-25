#ifndef BORNE_H
#define BORNE_H
#define TAILLE_BORNE 4
#include <QtGui>
#include <QtXml>
#include "qet.h"
class Conducer;
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
	bool addConducer(Conducer *);
	void removeConducer(Conducer *);
	int nbConducers() const;
	Diagram *diagram() const;
	
	// methodes de lecture
	QList<Conducer *> conducers() const;
	QET::Orientation orientation() const;
	QPointF amarrageConducer() const;
	void updateConducer(QPointF = QPointF());
	
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
	
	// differentes couleurs statiques utilisables pour l'effet "hover"
	static QColor couleur_neutre;
	static QColor couleur_autorise;
	static QColor couleur_prudence;
	static QColor couleur_interdit;
	
	private:
	// coordonnees des points d'amarrage
	QPointF amarrage_conducer;
	QPointF amarrage_elmt;
	// orientation de la borne
	QET::Orientation sens;
	// liste des conducers lies a cette borne
	QList<Conducer *> liste_conducers;
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
inline int Terminal::nbConducers() const {
	return(liste_conducers.size());
}

/**
	@return La position du point d'amarrage de la borne
*/
inline QPointF Terminal::amarrageConducer() const {
	return(mapToScene(amarrage_conducer));
}

#endif
