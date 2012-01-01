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
#include "element.h"
#include "qetapp.h"
#include "diagram.h"
#include "conductor.h"
#include "elementtextitem.h"
#include "diagramcommands.h"
#include <QtDebug>

/**
	Constructeur pour un element sans scene ni parent
*/
Element::Element(QGraphicsItem *parent, Diagram *scene) :
	QObject(),
	QGraphicsItem(parent, scene),
	internal_connections(false),
	must_highlight_(false),
	first_move_(true)
{
	setZValue(10);
}

/**
	Destructeur
*/
Element::~Element() {
}

/**
	@return true si l'element est mis en evidence
*/
bool Element::isHighlighted() const {
	return(must_highlight_);
}

/**
	@param hl true pour mettre l'element en evidence, false sinon
*/
void Element::setHighlighted(bool hl) {
	must_highlight_ = hl;
	update();
}

/**
	Methode principale de dessin de l'element
	@param painter Le QPainter utilise pour dessiner l'elment
	@param options Les options de style a prendre en compte
	@param widget  Le widget sur lequel on dessine
*/
void Element::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget) {
	
#ifndef Q_WS_WIN
	// corrige un bug de rendu ne se produisant que lors du rendu sur QGraphicsScene sous X11 au zoom par defaut
	static bool must_correct_rendering_bug = QETApp::settings().value("correct-rendering", false).toBool();
	if (must_correct_rendering_bug) {
		Diagram *dia = diagram();
		if (dia && options -> levelOfDetail == 1.0 && widget) {
			// calcule la rotation qu'a subi l'element
			qreal applied_rotation = 90.0 * (ori.current() - ori.defaultOrientation());
			while (applied_rotation < 360.0) applied_rotation += 360.0;
			while (applied_rotation > 360.0) applied_rotation -= 360.0;
			if (applied_rotation == 90.0) painter -> translate(1.0, -1.0);
			else if (applied_rotation == 180.0) painter -> translate(-1.0, -1.0);
			else if (applied_rotation == 270.0) painter -> translate(-1.0, 1.0);
		}
	}
#endif
	if (must_highlight_) drawHighlight(painter, options);
	
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
	@return la taille de l'element sur le schema
*/
QSize Element::size() const {
	return(dimensions);
}

/**
	Definit le hotspot de l'element par rapport au coin superieur gauche de son rectangle delimitant.
	Necessite que la taille ait deja ete definie
	@param hs Coordonnees du hotspot
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
	if (preview.isNull()) updatePixmap(); // on genere la pixmap si ce n'est deja fait
	return(preview);
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
	foreach(QGraphicsItem *qgi, childItems()) {
		if (Terminal *p = qgraphicsitem_cast<Terminal *>(qgi)) {
			p -> updateConductor();
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
void Element::drawAxes(QPainter *painter, const QStyleOptionGraphicsItem *options) {
	Q_UNUSED(options);
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
	@param painter Le QPainter a utiliser pour dessiner les bornes.
	@param options Les options de style a prendre en compte
 */
void Element::drawSelection(QPainter *painter, const QStyleOptionGraphicsItem *options) {
	Q_UNUSED(options);
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
	Dessine le cadre de selection de l'element de maniere systematiquement non antialiasee.
	@param painter Le QPainter a utiliser pour dessiner les bornes.
	@param options Les options de style a prendre en compte
 */
void Element::drawHighlight(QPainter *painter, const QStyleOptionGraphicsItem *options) {
	Q_UNUSED(options);
	painter -> save();
	
	qreal gradient_radius = qMin(boundingRect().width(), boundingRect().height()) / 2.0;
	QRadialGradient gradient(
		boundingRect().center(),
		gradient_radius,
		boundingRect().center()
	);
	gradient.setColorAt(0.0, QColor::fromRgb(69, 137, 255, 255));
	gradient.setColorAt(1.0, QColor::fromRgb(69, 137, 255, 0));
	QBrush brush(gradient);
	
	painter -> setPen(Qt::NoPen);
	painter -> setBrush(brush);
	// Le dessin se fait a partir du rectangle delimitant
	painter -> drawRoundRect(boundingRect().adjusted(1, 1, -1, -1), 10, 10);
	painter -> restore();
}

/**
	Fonction initialisant et dessinant la pixmap de l'element.
*/
void Element::updatePixmap() {
	// Pixmap transparente faisant la taille de base de l'element
	preview = QPixmap(dimensions);
	preview.fill(QColor(255, 255, 255, 0));
	// QPainter sur la pixmap, avec antialiasing
	QPainter p(&preview);
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
		int p_x = qRound(p.x() / (Diagram::xGrid * 1.0)) * Diagram::xGrid;
		// arrondit l'ordonnee a 10 px pres
		int p_y = qRound(p.y() / (Diagram::yGrid * 1.0)) * Diagram::yGrid;
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
	Gere le clic sur l'element
	@param e Objet decrivant l'evenement souris
*/
void Element::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	first_move_ = true;
	if (e -> modifiers() & Qt::ControlModifier) {
		setSelected(!isSelected());
	}
	QGraphicsItem::mousePressEvent(e);
}

/**
	Gere les mouvements de souris lies a l'element
	@param e Objet decrivant l'evenement souris
*/
void Element::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (isSelected() && e -> buttons() & Qt::LeftButton) {
		// l'element est en train d'etre deplace
		Diagram *diagram_ptr = diagram();
		if (diagram_ptr) {
			if (first_move_) {
				// il s'agit du premier mouvement du deplacement, on le signale
				// au schema parent
				diagram_ptr -> beginMoveElements(this);
			}
		}
		
		// on applique le mouvement impose par la souris
		QPointF old_pos = pos();
		setPos(mapToParent(e -> pos()) - matrix().map(e -> buttonDownPos(Qt::LeftButton)));
		
		// on calcule le mouvement reellement applique par setPos()
		QPointF effective_movement = pos() - old_pos;
		
		if (diagram_ptr) {
			// on signale le mouvement ainsi applique au schema parent, qui
			// l'appliquera aux autres items selectionnes selon son bon vouloir
			diagram_ptr -> continueMoveElements(effective_movement);
		}
	} else e -> ignore();
	
	if (first_move_) {
		first_move_ = false;
	}
}

/**
	Gere le relachement de souris
	Cette methode a ete reimplementee pour tenir a jour la liste des elements
	et conducteurs a deplacer au niveau du schema.
*/
void Element::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	if (Diagram *diagram_ptr = diagram()) {
		diagram_ptr -> endMoveElements();
	}
	
	if (!(e -> modifiers() & Qt::ControlModifier)) {
		QGraphicsItem::mouseReleaseEvent(e);
	}
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
bool Element::fromXml(QDomElement &e, QHash<int, Terminal *> &table_id_adr, bool handle_inputs_rotation) {
	/*
		les bornes vont maintenant etre recensees pour associer leurs id a leur adresse reelle
		ce recensement servira lors de la mise en place des fils
	*/
	QList<QDomElement> liste_terminals;
	foreach(QDomElement qde, QET::findInDomElement(e, "terminals", "terminal")) {
		if (Terminal::valideXml(qde)) liste_terminals << qde;
	}
	
	QHash<int, Terminal *> priv_id_adr;
	int terminals_non_trouvees = 0;
	foreach(QGraphicsItem *qgi, childItems()) {
		if (Terminal *p = qgraphicsitem_cast<Terminal *>(qgi)) {
			bool terminal_trouvee = false;
			foreach(QDomElement qde, liste_terminals) {
				if (p -> fromXml(qde)) {
					priv_id_adr.insert(qde.attribute("id").toInt(), p);
					terminal_trouvee = true;
					// We used to break here, because we did not expect
					// several terminals to share the same position.
					// Of course, it finally happened.
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
	QList<QDomElement> inputs = QET::findInDomElement(e, "inputs", "input");
	foreach(QGraphicsItem *qgi, childItems()) {
		if (ElementTextItem *eti = qgraphicsitem_cast<ElementTextItem *>(qgi)) {
			foreach(QDomElement input, inputs) eti -> fromXml(input);
		}
	}
	
	// position, selection
	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	
	// orientation
	bool conv_ok;
	int read_ori = e.attribute("orientation").toInt(&conv_ok);
	if (!conv_ok || read_ori < 0 || read_ori > 3) read_ori = ori.defaultOrientation();
	if (handle_inputs_rotation) {
		RotateElementsCommand::rotateElement(this, (QET::Orientation)read_ori);
	} else {
		setOrientation((QET::Orientation)read_ori);
	}
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
	element.setAttribute("type", typeId());
	
	// position, selection et orientation
	element.setAttribute("x", QString("%1").arg(pos().x()));
	element.setAttribute("y", QString("%1").arg(pos().y()));
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
	QDomElement xml_terminals = document.createElement("terminals");
	// pour chaque enfant de l'element
	foreach(Terminal *t, terminals()) {
		// alors on enregistre la borne
		QDomElement terminal = t -> toXml(document);
		terminal.setAttribute("id", id_terminal);
		table_adr_id.insert(t, id_terminal ++);
		xml_terminals.appendChild(terminal);
	}
	element.appendChild(xml_terminals);
	
	// enregistrement des champ de texte de l'appareil
	QDomElement inputs = document.createElement("inputs");
	foreach(ElementTextItem *eti, texts()) {
		inputs.appendChild(eti -> toXml(document));
	}
	element.appendChild(inputs);
	
	return(element);
}

/// @return le Diagram auquel cet element appartient, ou 0 si cet element est independant
Diagram *Element::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}
