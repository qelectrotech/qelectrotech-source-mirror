#include "element.h"
#include "qetapp.h"
#include "diagram.h"
#include "conducer.h"
#include "elementtextitem.h"
#include "diagramcommands.h"
#include <QtDebug>

/**
	Constructeur pour un element sans scene ni parent
*/
Element::Element(QGraphicsItem *parent, Diagram *scene) : QGraphicsItem(parent, scene) {
	peut_relier_ses_propres_terminals = false;
	setZValue(10);
}

/**
	Destructeur
*/
Element::~Element() {
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
		int hsx = qMin(hs.x(), dimensions.width());
		int hsy = qMin(hs.y(), dimensions.height());
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
	Permet de specifier l'orientation de l'element
	@param o la nouvelle orientation de l'objet
	@return true si l'orientation a pu etre appliquee, false sinon
*/
bool Element::setOrientation(QET::Orientation o) {
	// verifie que l'orientation demandee est acceptee
	if (!ori.accept(o)) return(false);
	prepareGeometryChange();
	// rotation en consequence et rafraichissement de l'element graphique
	qreal rotation_value = 90.0 * (o - ori.current());
	rotate(rotation_value);
	ori.setCurrent(o);
	update();
	foreach(QGraphicsItem *qgi, children()) {
		if (Terminal *p = qgraphicsitem_cast<Terminal *>(qgi)) p -> updateConducer();
		else if (ElementTextItem *eti = qgraphicsitem_cast<ElementTextItem *>(qgi)) {
			// applique une rotation contraire si besoin
			if (!eti -> followParentRotations())  {
				QMatrix new_matrix = eti -> matrix();
				qreal dx = eti -> boundingRect().width()  / 2.0;
				qreal dy = eti -> boundingRect().height() / 2.0;
				new_matrix.translate(dx, dy);
				new_matrix.rotate(-rotation_value);
				new_matrix.translate(-dx, -dy);
				eti -> setMatrix(new_matrix);
			}
		}
	}
	return(true);
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
	painter -> drawRoundRect(boundingRect().adjusted(1, 1, -1, -1), 10, 10);
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
	reste sur la grille du Diagram auquel il appartient.
	@param p Nouvelles coordonnees de l'element
*/
void Element::setPos(const QPointF &p) {
	if (p == pos()) return;
	// pas la peine de positionner sur la grille si l'element n'est pas sur un Diagram
	if (scene()) {
		// arrondit l'abscisse a 10 px pres
		int p_x = qRound(p.x() / 10.0) * 10;
		// arrondit l'ordonnee a 10 px pres
		int p_y = qRound(p.y() / 10.0) * 10;
		QGraphicsItem::setPos(p_x, p_y);
	} else QGraphicsItem::setPos(p);
}

/**
	Change la position de l'element en veillant a ce que l'element
	reste sur la grille du Diagram auquel il appartient.
	@param x Nouvelle abscisse de l'element
	@param y Nouvelle ordonnee de l'element
*/
void Element::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

/**
	Gere les mouvements de souris lies a l'element
*/
void Element::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (e -> buttons() & Qt::LeftButton) {
		QPointF oldPos = pos();
		setPos(mapToParent(e -> pos()) - matrix().map(e -> buttonDownPos(Qt::LeftButton)));
		moveOtherElements(pos() - oldPos);
	} else e -> ignore();
}

void Element::moveOtherElements(const QPointF &diff) {
	// inutile de deplacer les autres elements s'il n'y a pas eu de mouvement concret
	if (diff.isNull()) return;
	
	// recupere le schema parent
	Diagram *diagram_ptr = diagram();
	if (!diagram_ptr) return;
	
	diagram_ptr -> current_movement += diff;
	
	// deplace les elements selectionnes
	foreach(Element *element, diagram_ptr -> elementsToMove()) {
		if (element == this) continue;
		element -> setPos(element -> pos() + diff);
	}
	
	// deplace certains conducteurs
	foreach(Conducer *conducer, diagram_ptr -> conducersToMove()) {
		conducer -> setPos(conducer -> pos() + diff);
	}
	
	// recalcule les autres conducteurs
	const QHash<Conducer *, Terminal *> &conducers_modify = diagram_ptr -> conducersToUpdate();
	foreach(Conducer *conducer, conducers_modify.keys()) {
		conducer -> updateWithNewPos(QRectF(), conducers_modify[conducer], conducers_modify[conducer] -> amarrageConducer());
	}
}

void Element::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	Diagram *diagram_ptr = diagram();
	if (diagram_ptr) {
		if (!diagram_ptr -> current_movement.isNull()) {
			diagram_ptr -> undoStack().push(
				new MoveElementsCommand(
					diagram_ptr,
					diagram_ptr -> elementsToMove(),
					diagram_ptr -> conducersToMove(),
					diagram_ptr -> conducersToUpdate(),
					diagram_ptr -> current_movement
				)
			);
			diagram_ptr -> current_movement = QPointF();
		}
		diagram_ptr -> invalidateMovedElements();
	}
	QGraphicsItem::mouseReleaseEvent(e);
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

/**
	Methode d'import XML. Cette methode est appelee lors de l'import de contenu
	XML (coller, import, ouverture de fichier...) afin que l'element puisse
	gerer lui-meme l'importation de ses bornes. Ici, comme cette classe est
	caracterisee par un nombre fixe de bornes, l'implementation exige de
	retrouver exactement ses bornes dans le fichier XML.
	@param e L'element XML a analyser.
	@param table_id_adr Reference vers la table de correspondance entre les IDs
	du fichier XML et les adresses en memoire. Si l'import reussit, il faut y
	ajouter les bons couples (id, adresse).
	@return true si l'import a reussi, false sinon
	
*/
bool Element::fromXml(QDomElement &e, QHash<int, Terminal *> &table_id_adr) {
	/*
		les bornes vont maintenant etre recensees pour associer leurs id à leur adresse reelle
		ce recensement servira lors de la mise en place des fils
	*/
	QList<QDomElement> liste_terminals;
	foreach(QDomElement qde, findInDomElement(e, "terminals", "terminal")) {
		if (Terminal::valideXml(qde)) liste_terminals << qde;
	}
	
	QHash<int, Terminal *> priv_id_adr;
	int terminals_non_trouvees = 0;
	foreach(QGraphicsItem *qgi, children()) {
		if (Terminal *p = qgraphicsitem_cast<Terminal *>(qgi)) {
			bool terminal_trouvee = false;
			foreach(QDomElement qde, liste_terminals) {
				if (p -> fromXml(qde)) {
					priv_id_adr.insert(qde.attribute("id").toInt(), p);
					terminal_trouvee = true;
					break;
				}
			}
			if (!terminal_trouvee) ++ terminals_non_trouvees;
		}
	}
	
	if (terminals_non_trouvees > 0) {
		return(false);
	} else {
		// verifie que les associations id / adr n'entrent pas en conflit avec table_id_adr
		foreach(int id_trouve, priv_id_adr.keys()) {
			if (table_id_adr.contains(id_trouve)) {
				// cet element possede un id qui est deja reference (= conflit)
				return(false);
			}
		}
		// copie des associations id / adr
		foreach(int id_trouve, priv_id_adr.keys()) {
			table_id_adr.insert(id_trouve, priv_id_adr.value(id_trouve));
		}
	}
	
	// importe les valeurs des champs de texte
	QList<QDomElement> inputs = findInDomElement(e, "inputs", "input");
	foreach(QGraphicsItem *qgi, children()) {
		if (ElementTextItem *eti = qgraphicsitem_cast<ElementTextItem *>(qgi)) {
			foreach(QDomElement input, inputs) eti -> fromXml(input);
		}
	}
	
	// position, selection et orientation
	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	bool conv_ok;
	int read_ori = e.attribute("orientation").toInt(&conv_ok);
	if (!conv_ok || read_ori < 0 || read_ori > 3) read_ori = ori.defaultOrientation();
	setOrientation((QET::Orientation)read_ori);
	setSelected(e.attribute("selected") == "selected");
	
	return(true);
}

/**
	Permet d'exporter l'element en XML
	@param document Document XML a utiliser
	@param table_adr_id Table de correspondance entre les adresses des bornes
	et leur id dans la representation XML ; cette table completee par cette
	methode
	@return L'element XML representant cet element electrique
*/
QDomElement Element::toXml(QDomDocument &document, QHash<Terminal *, int> &table_adr_id) const {
	QDomElement element = document.createElement("element");
	
	// type
	QString chemin_elmt = typeId();
	QString type_elmt = QETApp::symbolicPath(chemin_elmt);
	element.setAttribute("type", type_elmt);
	
	// position, selection et orientation
	element.setAttribute("x", pos().x());
	element.setAttribute("y", pos().y());
	if (isSelected()) element.setAttribute("selected", "selected");
	element.setAttribute("orientation", QString("%1").arg(ori.current()));
	
	/* recupere le premier id a utiliser pour les bornes de cet element */
	int id_terminal = 0;
	if (!table_adr_id.isEmpty()) {
		// trouve le plus grand id
		int max_id_t = -1;
		foreach (int id_t, table_adr_id.values()) {
			if (id_t > max_id_t) max_id_t = id_t;
		}
		id_terminal = max_id_t + 1;
	}
	
	// enregistrement des bornes de l'appareil
	QDomElement terminals = document.createElement("terminals");
	// pour chaque enfant de l'element
	foreach(QGraphicsItem *child, children()) {
		// si cet enfant est une borne
		if (Terminal *t = qgraphicsitem_cast<Terminal *>(child)) {
			// alors on enregistre la borne
			QDomElement terminal = t -> toXml(document);
			terminal.setAttribute("id", id_terminal);
			table_adr_id.insert(t, id_terminal ++);
			terminals.appendChild(terminal);
		}
	}
	element.appendChild(terminals);
	
	// enregistrement des champ de texte de l'appareil
	QDomElement inputs = document.createElement("inputs");
	// pour chaque enfant de l'element
	foreach(QGraphicsItem *child, children()) {
		// si cet enfant est un champ de texte
		if (ElementTextItem *eti = qgraphicsitem_cast<ElementTextItem *>(child)) {
			// alors on enregistre le champ de texte
			inputs.appendChild(eti -> toXml(document));
		}
	}
	element.appendChild(inputs);
	
	return(element);
}

/**
	Methode statique sans rapport direct avec la manipulation des elements.
	Etant donne un element XML e, elle renvoie la liste de tous les elements
	children imbriques dans les elements parent, eux-memes enfants de l'elememt e
	@param e Element XML a explorer
	@param parent tag XML intermediaire
	@param children tag XML a rechercher
	@return La liste des elements XML children
*/
QList<QDomElement> Element::findInDomElement(QDomElement e, QString parent, QString children) {
	// recense les champs de texte
	QList<QDomElement> return_list;
	// parcours des enfants de l'element
	for (QDomNode enfant = e.firstChild() ; !enfant.isNull() ; enfant = enfant.nextSibling()) {
		// on s'interesse a l'element XML "parent"
		QDomElement parents = enfant.toElement();
		if (parents.isNull() || parents.tagName() != parent) continue;
		// parcours des enfants de l'element XML "parent"
		for (QDomNode node_children = parents.firstChild() ; !node_children.isNull() ; node_children = node_children.nextSibling()) {
			// on s'interesse a l'element XML "children"
			QDomElement n_children = node_children.toElement();
			if (!n_children.isNull() && n_children.tagName() == children) return_list.append(n_children);
		}
	}
	return(return_list);
}

/// @return le Diagram auquel cet element appartient, ou 0 si cet element est independant
Diagram *Element::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}
