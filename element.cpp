#include "element.h"
#include "schema.h"
#include <QtDebug>

/*** Methodes publiques ***/

/**
	Constructeur pour un element sans scene ni parent
*/
Element::Element(QGraphicsItem *parent, Schema *scene) : QGraphicsItem(parent, scene) {
	sens = true;
	peut_relier_ses_propres_bornes = false;
}

/**
	Methode principale de dessin de l'element
	@param painter Le QPainter utilise pour dessiner l'elment
	@param options Les options de style a prendre en compte
	@param widget  Le widget sur lequel on dessine
*/
void Element::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *) {
	// Dessin de l'element lui-meme
	paint(painter, options);
	
	// Dessin du cadre de selection si necessaire
	if (isSelected()) drawSelection(painter, options);
}

/**
	@return Le rectangle delimitant le contour de l'element
*/
QRectF Element::boundingRect() const {
	return(QRectF(QPointF(-hotspot_coord.x(), -hotspot_coord.y()), dimensions));
}

/**
	Definit la taille de l'element sur le schema. Les tailles doivent etre
	des multiples de 10 ; si ce n'est pas le cas, les dimensions indiquees
	seront arrrondies aux dizaines superieures.
	@param wid Largeur de l'element
	@param hei Hauteur de l'element
	@return La taille finale de l'element
*/
QSize Element::setSize(int wid, int hei) {
	prepareGeometryChange();
	// chaque dimension indiquee est arrondie a la dizaine superieure
	while (wid % 10) ++ wid;
	while (hei % 10) ++ hei;
	// les dimensions finales sont conservees et retournees
	return(dimensions = QSize(wid, hei));
}

/**
	Definit le hotspot de l'element par rapport au coin superieur gauche de son rectangle delimitant.
	Necessite que la taille ait deja ete definie
	@param hsx Abscisse du hotspot
	@param hsy Ordonnee du hotspot
*/
QPoint Element::setHotspot(QPoint hs) {
	// la taille doit avoir ete definie
	prepareGeometryChange();
	if (dimensions.isNull()) hotspot_coord = QPoint(0, 0);
	else {
		// les coordonnees indiquees ne doivent pas depasser les dimensions de l'element
		int hsx = hs.x() > dimensions.width() ? dimensions.width() : hs.x();
		int hsy = hs.y() > dimensions.height() ? dimensions.height() : hs.y();
		hotspot_coord = QPoint(hsx, hsy);
	}
	return(hotspot_coord);
}

/**
	@return Le hotspot courant de l'element
*/
QPoint Element::hotspot() const {
	return(hotspot_coord);
}

/**
	Selectionne l'element
*/
void Element::select() {
	setSelected(true);
}

/**
	Deselectionne l'element
*/
void Element::deselect() {
	setSelected(false);
}

/**
	@return La pixmap de l'element
*/
QPixmap Element::pixmap() {
	if (apercu.isNull()) updatePixmap(); // on genere la pixmap si ce n'est deja fait
	return(apercu);
}

/**
	@todo distinguer les bornes avec un cast dynamique
*/
QVariant Element::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemSelectedChange) {
		foreach(QGraphicsItem *qgi, children()) {
			if (Borne *p = qgraphicsitem_cast<Borne *>(qgi)) p -> updateConducteur();
		}
	}
	return(QGraphicsItem::itemChange(change, value));
}

/**
	@return L'orientation en cours de l'element : true pour une orientation verticale, false pour une orientation horizontale
*/
bool Element::orientation() const {
	return(sens);
}

/**
	Inverse l'orientation de l'element
	@return La nouvelle orientation : true pour une orientation verticale, false pour une orientation horizontale
*/
bool Element::invertOrientation() {
	// inversion du sens
	sens = !sens;
	// on cache temporairement l'element pour eviter un bug graphique
	hide();
	// rotation en consequence et rafraichissement de l'element graphique
	rotate(sens ? 90.0 : -90.0);
	// on raffiche l'element, on le reselectionne et on le rafraichit
	show();
	select();
	update();
	return(sens);
}

/*** Methodes protegees ***/

/**
	Dessine un petit repere (axes x et y) relatif a l'element
	@param painter Le QPainter a utiliser pour dessiner les axes
	@param options Les options de style a prendre en compte
*/
void Element::drawAxes(QPainter *painter, const QStyleOptionGraphicsItem *) {
	painter -> setPen(Qt::blue);
	painter -> drawLine(0, 0, 10, 0);
	painter -> drawLine(7,-3, 10, 0);
	painter -> drawLine(7, 3, 10, 0);
	painter -> setPen(Qt::red);
	painter -> drawLine(0,  0, 0, 10);
	painter -> drawLine(0, 10,-3,  7);
	painter -> drawLine(0, 10, 3,  7);
}

/*** Methodes privees ***/

/**
	Dessine le cadre de selection de l'element de maniere systematiquement non antialiasee.
	@param qp Le QPainter a utiliser pour dessiner les bornes.
	@param options Les options de style a prendre en compte
 */
void Element::drawSelection(QPainter *painter, const QStyleOptionGraphicsItem *) {
	painter -> save();
	// Annulation des renderhints
	painter -> setRenderHint(QPainter::Antialiasing,          false);
	painter -> setRenderHint(QPainter::TextAntialiasing,      false);
	painter -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	// Dessin du cadre de selection en gris
	QPen t;
	t.setColor(Qt::gray);
	t.setStyle(Qt::DashDotLine);
	painter -> setPen(t);
	// Le dessin se fait a partir du rectangle delimitant
	painter -> drawRoundRect(boundingRect(), 10, 10);
	painter -> restore();
}

/**
	Fonction initialisant et dessinant la pixmap de l'element.
*/
void Element::updatePixmap() {
	// Pixmap transparente faisant la taille de base de l'element
	apercu = QPixmap(dimensions);
	apercu.fill(QColor(255, 255, 255, 0));
	// QPainter sur la pixmap, avec antialiasing
	QPainter p(&apercu);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);
	// Translation de l'origine du repere de la pixmap
	p.translate(hotspot_coord);
	// L'element se dessine sur la pixmap
	paint(&p, 0);
}

/**
	Change la position de l'element en veillant a ce que l'element
	reste sur la grille du Schema auquel il appartient.
	@param p Nouvelles coordonnees de l'element
*/
void Element::setPos(const QPointF &p) {
	if (p == pos()) return;
	// pas la peine de positionner sur la grille si l'element n'est pas sur un Schema
	if (scene()) {
		// arrondit l'abscisse a 10 px pres
		int p_x = qRound(p.x() / 10.0) * 10;
		// arrondit l'ordonnee a 10 px pres
		int p_y = qRound(p.y() / 10.0) * 10;
		QGraphicsItem::setPos(p_x, p_y);
	} else QGraphicsItem::setPos(p);
	// actualise les bornes / conducteurs
	foreach(QGraphicsItem *qgi, children()) {
		if (Borne *p = qgraphicsitem_cast<Borne *>(qgi)) p -> updateConducteur();
	}
}

/**
	Change la position de l'element en veillant a ce que l'element
	reste sur la grille du Schema auquel il appartient.
	@param x Nouvelle abscisse de l'element
	@param y Nouvelle ordonnee de l'element
*/
void Element::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

/**
	Gere les mouvements de souris lies a l'element, notamment
*/
void Element::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	/*&& (flags() & ItemIsMovable)*/ // on le sait qu'il est movable
	if (e -> buttons() & Qt::LeftButton) {
		QPointF oldPos = pos();
		setPos(mapToParent(e->pos()) - matrix().map(e->buttonDownPos(Qt::LeftButton)));
		QPointF diff = pos() - oldPos;
		
		// Recupere la liste des elements selectionnes
		QList<QGraphicsItem *> selectedItems;
		if (scene()) {
			selectedItems = scene() -> selectedItems();
		} else if (QGraphicsItem *parent = parentItem()) {
			while (parent && parent->isSelected()) selectedItems << parent;
		}
		
		// Deplace tous les elements selectionnes
		foreach (QGraphicsItem *item, selectedItems) {
			if (!item->parentItem() || !item->parentItem()->isSelected())
				if (item != this) item->setPos(item->pos() + diff);
		}
	} else e -> ignore();
}

/**
	Permet de savoir si un element XML (QDomElement) represente bien un element
	@param e Le QDomElement a valide
	@return true si l'element XML est un Element, false sinon
*/
bool Element::valideXml(QDomElement &e) {
	// verifie le nom du tag
	if (e.tagName() != "element") return(false);
	
	// verifie la presence des attributs minimaux
	if (!e.hasAttribute("type")) return(false);
	if (!e.hasAttribute("x"))    return(false);
	if (!e.hasAttribute("y"))    return(false);
	
	bool conv_ok;
	// parse l'abscisse
	e.attribute("x").toDouble(&conv_ok);
	if (!conv_ok) return(false);
	
	// parse l'ordonnee
	e.attribute("y").toDouble(&conv_ok);
	if (!conv_ok) return(false);
	return(true);
}
