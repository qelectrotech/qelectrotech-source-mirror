#ifndef ELEMENT_H
#define ELEMENT_H
#include <QtGui>
#include "terminal.h"
/**
	Cette classe abstraite represente un element electrique.
*/
class Diagram;
class Element : public QGraphicsItem {
	
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
	bool    ori_n;
	bool    ori_s;
	bool    ori_e;
	bool    ori_w;
	Terminal::Orientation ori_d;
	Terminal::Orientation ori;
	
	private:
	QSize   dimensions;
	QPoint  hotspot_coord;
	QPixmap apercu;
	QMenu   menu;
	
	// methodes
	public:
	virtual int type() const { return Type; }
	
	// methodes virtuelles pures a definir dans les classes enfants
	virtual int nbTerminals() const = 0;
	virtual int nbTerminalsMin() const = 0;
	virtual int nbTerminalsMax() const = 0;
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *) = 0;
	virtual QString typeId() const = 0;
	virtual QString nom() const = 0;
	
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	QVariant itemChange(GraphicsItemChange, const QVariant &);
	QRectF boundingRect() const;
	QSize setSize(int, int);
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
	bool connexionsInternesAcceptees();
	void setConnexionsInternesAcceptees(bool cia);
	
	// methodes relatives aux fichiers XML
	static bool valideXml(QDomElement &);
	virtual bool fromXml(QDomElement &, QHash<int, Terminal *>&);
	virtual QDomElement toXml(QDomDocument &, QHash<Terminal *, int>&) const;
	
	// methodes d'acces aux possibilites d'orientation
	Terminal::Orientation orientation() const;
	Terminal::Orientation defaultOrientation() const;
	bool acceptOrientation(Terminal::Orientation o) const;
	Terminal::Orientation nextAcceptableOrientation() const;
	Terminal::Orientation previousAcceptableOrientation() const;
	bool setOrientation(Terminal::Orientation o);
	
	protected:
	void drawAxes(QPainter *, const QStyleOptionGraphicsItem *);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	
	private:
	bool peut_relier_ses_propres_terminals;
	void drawSelection(QPainter *, const QStyleOptionGraphicsItem *);
	void updatePixmap();
	Terminal::Orientation nextOrientation(Terminal::Orientation o) const;
	Terminal::Orientation previousOrientation(Terminal::Orientation o) const;
	static QList<QDomElement> findInDomElement(QDomElement, QString, QString);
};

/**
	Permet de savoir si l'element accepte les connexions internes,
	c'est-a-dire que ses bornes peuvent etre reliees entre elles
	@return true si l'element accepte les connexions internes, false sinon
*/
inline bool Element::connexionsInternesAcceptees() {
	return(peut_relier_ses_propres_terminals);
}

/**
	Permet de specifier si l'element accepte les connexions internes,
	c'est-a-dire que ses bornes peuvent etre reliees entre elles
	@param cia true pour que l'element accepte les connexions internes, false pour
	qu'il les interdise
*/
inline void Element::setConnexionsInternesAcceptees(bool cia) {
	peut_relier_ses_propres_terminals = cia;
}

/**
	Permet de connaitre l'orientation actuelle de l'element
	@return L'orientation actuelle de l'element
*/
inline Terminal::Orientation Element::orientation() const {
	return(ori);
}

/**
	Permet de savoir si l'element peut etre positionne dans une orientation 
	donnee.
	@param o L'orientation en question
	@return true si l'orientation est utilisable, false sinon
*/
inline bool Element::acceptOrientation(Terminal::Orientation o) const {
	switch(o) {
		case Terminal::Nord:  return(ori_n);
		case Terminal::Est:   return(ori_e);
		case Terminal::Sud:   return(ori_s);
		case Terminal::Ouest: return(ori_w);
		default: return(false);
	}
}

/**
	Permet de connaitre l'orientation par defaut de l'element
	@return l'orientation par defaut de l'element
*/
inline Terminal::Orientation Element::defaultOrientation() const {
	return(ori_d);
}

/**
	Permet de connaitre la prochaine orientation autorisee pour cet element
	@return la prochaine orientation autorisee pour cet element
*/
inline Terminal::Orientation Element::nextAcceptableOrientation() const {
	Terminal::Orientation retour = nextOrientation(ori);
	for (int i = 0 ; i < 4 ; ++ i) {
		if (acceptOrientation(retour)) return(retour);
		retour = nextOrientation(retour);
	}
	// on ne devrait pas arriver la : renvoi d'une valeur par defaut = nord
	return(Terminal::Nord);
}

/**
	Permet de connaitre la precedente orientation autorisee pour cet element
	@return la precedente orientation autorisee pour cet element
*/
inline Terminal::Orientation Element::previousAcceptableOrientation() const {
	Terminal::Orientation retour = previousOrientation(ori);
	for (int i = 0 ; i < 4 ; ++ i) {
		if (acceptOrientation(retour)) return(retour);
		retour = previousOrientation(retour);
	}
	// on ne devrait pas arriver la : renvoi d'une valeur par defaut = nord
	return(Terminal::Nord);
}

/**
	Permet de connaitre l'orientation suivante apres celle donnee en parametre.
	Les orientations sont generalement presentees dans l'ordre suivant : Nord,
	Est, Sud, Ouest.
	@param o une orientation
	@return l'orientation suivante
*/
inline Terminal::Orientation Element::nextOrientation(Terminal::Orientation o) const {
	if (o < 0 || o > 2) return(Terminal::Nord);
	return((Terminal::Orientation)(o + 1));
}

/**
	Permet de connaitre l'orientation precedant celle donnee en parametre.
	Les orientations sont generalement presentees dans l'ordre suivant : Nord,
	Est, Sud, Ouest.
	@param o une orientation
	@return l'orientation precedente
*/
inline Terminal::Orientation Element::previousOrientation(Terminal::Orientation o) const {
	if (o < 0 || o > 3) return(Terminal::Nord);
	if (o == Terminal::Nord) return(Terminal::Ouest);
	return((Terminal::Orientation)(o - 1));
}

#endif
