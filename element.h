#ifndef ELEMENT_H
#define ELEMENT_H
#include <QtGui>
#include "terminal.h"
#include "orientationset.h"
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
	OrientationSet ori;
	
	private:
	QSize   dimensions;
	QPoint  hotspot_coord;
	QPixmap apercu;
	QMenu   menu;
	
	// methodes
	public:
	virtual int type() const { return Type; }
	
	// methodes virtuelles pures a definir dans les classes enfants
	virtual QList<Terminal *> terminals() const = 0;
	virtual QList<Conducer *> conducers() const = 0;
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
	bool setOrientation(QET::Orientation o);
	const OrientationSet &orientation() const;
	
	protected:
	void drawAxes(QPainter *, const QStyleOptionGraphicsItem *);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	
	private:
	bool peut_relier_ses_propres_terminals;
	void drawSelection(QPainter *, const QStyleOptionGraphicsItem *);
	void updatePixmap();
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
inline const OrientationSet & Element::orientation() const {
	return(ori);
}

#endif
