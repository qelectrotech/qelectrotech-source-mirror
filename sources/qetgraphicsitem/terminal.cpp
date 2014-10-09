/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "terminal.h"
#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/conductor.h"
#include "diagramcommands.h"
#include "qetapp.h"

QColor Terminal::neutralColor      = QColor(Qt::blue);
QColor Terminal::allowedColor      = QColor(Qt::darkGreen);
QColor Terminal::warningColor      = QColor("#ff8000");
QColor Terminal::forbiddenColor    = QColor(Qt::red);
const qreal Terminal::terminalSize = 4.0;

/**
	Methode privee pour initialiser la borne.
	@param pf  position du point d'amarrage pour un conducteur
	@param o   orientation de la borne : Qt::Horizontal ou Qt::Vertical
	@param number of terminal
	@param name of terminal
*/
void Terminal::init(QPointF pf, Qet::Orientation o, QString number, QString name, bool hiddenName) {
	// definition du pount d'amarrage pour un conducteur
	dock_conductor_  = pf;
	
	// definition de l'orientation de la borne (par defaut : sud)
	if (o < Qet::North || o > Qet::West) ori_ = Qet::South;
	else ori_ = o;
	
	// calcul de la position du point d'amarrage a l'element
	dock_elmt_ = dock_conductor_;
	switch(ori_) {
		case Qet::North: dock_elmt_ += QPointF(0, Terminal::terminalSize);  break;
		case Qet::East : dock_elmt_ += QPointF(-Terminal::terminalSize, 0); break;
		case Qet::West : dock_elmt_ += QPointF(Terminal::terminalSize, 0);  break;
		case Qet::South:
		default        : dock_elmt_ += QPointF(0, -Terminal::terminalSize);
	}
	// Number of terminal
	number_terminal_ = number;
	// Name of terminal
	name_terminal_ = name;
	name_terminal_hidden = hiddenName;
	// par defaut : pas de conducteur
	
	// QRectF null
	br_ = new QRectF();
	previous_terminal_ = 0;
	// divers
	setAcceptsHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton);
	hovered_ = false;
	setToolTip(QObject::tr("Borne", "tooltip"));
}

/**
	initialise une borne
	@param pf  position du point d'amarrage pour un conducteur
	@param o   orientation de la borne : Qt::Horizontal ou Qt::Vertical
	@param e   Element auquel cette borne appartient
	@param s   Scene sur laquelle figure cette borne
*/
Terminal::Terminal(QPointF pf, Qet::Orientation o, Element *e, Diagram *s) :
	QGraphicsItem(e, s),
	parent_element_(e),
	hovered_color_(Terminal::neutralColor)
{
	init(pf, o, "_", "_", false);
}

/**
	initialise une borne
	@param pf_x Abscisse du point d'amarrage pour un conducteur
	@param pf_y Ordonnee du point d'amarrage pour un conducteur
	@param o    orientation de la borne : Qt::Horizontal ou Qt::Vertical
	@param e    Element auquel cette borne appartient
	@param s    Scene sur laquelle figure cette borne
*/
Terminal::Terminal(qreal pf_x, qreal pf_y, Qet::Orientation o, Element *e, Diagram *s) :
	QGraphicsItem(e, s),
	parent_element_(e),
	hovered_color_(Terminal::neutralColor)
{
	init(QPointF(pf_x, pf_y), o, "_", "_", false);
}

/**
	initialise une borne
	@param pf  position du point d'amarrage pour un conducteur
	@param o   orientation de la borne : Qt::Horizontal ou Qt::Vertical
	@param num number of terminal (ex 3 - 4 for NO)
	@param name of terminal
	@param hiddenName hide or show the name
	@param e   Element auquel cette borne appartient
	@param s   Scene sur laquelle figure cette borne
*/
Terminal::Terminal(QPointF pf, Qet::Orientation o, QString num, QString name, bool hiddenName, Element *e, Diagram *s) :
	QGraphicsItem(e, s),
	parent_element_(e),
	hovered_color_(Terminal::neutralColor)
{
	init(pf, o, num, name, hiddenName);
}

/**
	Destructeur
	La destruction de la borne entraine la destruction des conducteurs
	associes.
*/
Terminal::~Terminal() {
	foreach(Conductor *c, conductors_) delete c;
	delete br_;
}

/**
	Permet de connaitre l'orientation de la borne. Si le parent de la borne
	est bien un Element, cette fonction renvoie l'orientation par rapport a
	la scene de la borne, en tenant compte du fait que l'element ait pu etre
	pivote. Sinon elle renvoie son sens normal.
	@return L'orientation actuelle de la Terminal.
*/
Qet::Orientation Terminal::orientation() const {
	if (Element *elt = qgraphicsitem_cast<Element *>(parentItem())) {
		// orientations actuelle et par defaut de l'element
		int ori_cur = elt -> orientation();
		if (ori_cur == 0) return(ori_);
		else {
			// calcul l'angle de rotation implique par l'orientation de l'element parent
			// angle de rotation de la borne sur la scene, divise par 90
			int angle = ori_cur + ori_;
			while (angle >= 4) angle -= 4;
			return((Qet::Orientation)angle);
		}
	} else return(ori_);
}


/**
 * @brief Terminal::setNumber
 * @param number
 */
void Terminal::setNumber(QString number) {
	number_terminal_ = number;
}

/**
 * @brief Terminal::setName
 * @param name
 */
void Terminal::setName(QString name, bool hiddenName) {
	name_terminal_ = name;
	name_terminal_hidden = hiddenName;
}

/**
	Attribue un conductor a la borne
	@param f Le conducteur a rattacher a cette borne
*/
bool Terminal::addConductor(Conductor *f) {
	// pointeur 0 refuse
	if (!f) return(false);
	
	// une seule des deux bornes du conducteur doit etre this
	Q_ASSERT_X(((f -> terminal1 == this) ^ (f -> terminal2 == this)), "Terminal::addConductor", "Le conductor devrait etre relie exactement une fois a la terminal en cours");
	
	// determine l'autre borne a laquelle cette borne va etre relie grace au conducteur
	Terminal *autre_terminal = (f -> terminal1 == this) ? f -> terminal2 : f -> terminal1;
	
	// verifie que la borne n'est pas deja reliee avec l'autre borne
	bool deja_liees = false;
	foreach (Conductor* conductor, conductors_) {
		if (conductor -> terminal1 == autre_terminal || conductor -> terminal2 == autre_terminal) deja_liees = true;
	}
	
	// si les deux bornes sont deja reliees, on refuse d'ajouter le conducteur
	if (deja_liees) return(false);
	
	// sinon on ajoute le conducteur
	conductors_.append(f);
	return(true);
}

/**
	Enleve un conducteur donne a la borne
	@param f Conducteur a enlever
*/
void Terminal::removeConductor(Conductor *f) {
	int index = conductors_.indexOf(f);
	if (index == -1) return;
	conductors_.removeAt(index);
}

/**
	Fonction de dessin des bornes
	@param p Le QPainter a utiliser
	@param options Les options de dessin
	@param widget Le widget sur lequel on dessine
*/
void Terminal::paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) {
	// en dessous d'un certain zoom, les bornes ne sont plus dessinees
	if (options && options -> levelOfDetail < 0.5) return;
	
	p -> save();
	
#ifndef Q_WS_WIN
	// corrige un bug de rendu ne se produisant que lors du rendu sur QGraphicsScene sous X11 au zoom par defaut
	static bool must_correct_rendering_bug = QETApp::settings().value("correct-rendering", false).toBool();
	if (must_correct_rendering_bug) {
		Diagram *dia = diagram();
		if (dia && options -> levelOfDetail == 1.0 && widget) {
			// calcule la rotation qu'a subi l'element
			qreal applied_rotation = 0.0;
			if (Element *elt = qgraphicsitem_cast<Element *>(parentItem())) {
				// orientations actuelle et par defaut de l'element
				int ori_cur = elt -> orientation();
				applied_rotation = QET::correctAngle(90.0 * ori_cur);
			}
			if (applied_rotation == 90.0) p -> translate(1.0, -1.0);
			else if (applied_rotation == 180.0) p -> translate(-1.0, -1.0);
			else if (applied_rotation == 270.0) p -> translate(-1.0, 1.0);
		}
	}
#endif
	
	//annulation des renderhints
	p -> setRenderHint(QPainter::Antialiasing,          false);
	p -> setRenderHint(QPainter::TextAntialiasing,      false);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	// on travaille avec les coordonnees de l'element parent
	QPointF c = mapFromParent(dock_conductor_);
	QPointF e = mapFromParent(dock_elmt_);
	
	QPen t;
	t.setWidthF(1.0);
	
	if (options && options -> levelOfDetail < 1.0) {
		t.setCosmetic(true);
	}
	
	// dessin de la borne en rouge
	t.setColor(Qt::red);
	p -> setPen(t);
	p -> drawLine(c, e);
	
	// dessin du point d'amarrage au conducteur en bleu
	t.setColor(hovered_color_);
	p -> setPen(t);
	p -> setBrush(hovered_color_);
	if (hovered_) {
		p -> setRenderHint(QPainter::Antialiasing, true);
		p -> drawEllipse(QRectF(c.x() - 2.5, c.y() - 2.5, 5.0, 5.0));
	} else p -> drawPoint(c);
	
	p -> restore();
}

/**
	@return Le rectangle (en precision flottante) delimitant la borne et ses alentours.
*/
QRectF Terminal::boundingRect() const {
	if (br_ -> isNull()) {
		qreal dcx = dock_conductor_.x();
		qreal dcy = dock_conductor_.y();
		qreal dex = dock_elmt_.x();
		qreal dey = dock_elmt_.y();
		QPointF origin = (dcx <= dex && dcy <= dey ? dock_conductor_ : dock_elmt_);
		origin += QPointF(-3.0, -3.0);
		qreal w = qAbs((int)(dcx - dex)) + 7;
		qreal h = qAbs((int)(dcy - dey)) + 7;
		*br_ = QRectF(origin, QSizeF(w, h));
	}
	return(*br_);
}

/**
	Gere l'entree de la souris sur la zone de la Borne.
*/
void Terminal::hoverEnterEvent(QGraphicsSceneHoverEvent *) {
	hovered_ = true;
	update();
}

/**
	Gere les mouvements de la souris sur la zone de la Borne.
*/
void Terminal::hoverMoveEvent(QGraphicsSceneHoverEvent *) {
}

/**
	Gere le fait que la souris sorte de la zone de la Borne.
*/
void Terminal::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
	hovered_ = false;
	update();
}

/**
	Gere le fait qu'on enfonce un bouton de la souris sur la Borne.
	@param e L'evenement souris correspondant
*/
void Terminal::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	if (Diagram *d = diagram()) {
		d -> setConductorStart(mapToScene(QPointF(dock_conductor_)));
		d -> setConductorStop(e -> scenePos());
		d -> setConductor(true);
		//setCursor(Qt::CrossCursor);
	}
}

/**
	Gere le fait qu'on bouge la souris sur la Borne.
	@param e L'evenement souris correspondant
*/
void Terminal::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	// pendant la pose d'un conducteur, on adopte un autre curseur 
	//setCursor(Qt::CrossCursor);
	
	// d'un mouvement a l'autre, il faut retirer l'effet hover de la borne precedente
	if (previous_terminal_) {
		if (previous_terminal_ == this) hovered_ = true;
		else previous_terminal_ -> hovered_ = false;
		previous_terminal_ -> hovered_color_ = previous_terminal_ -> neutralColor;
		previous_terminal_ -> update();
	}
	
	
	Diagram *d = diagram();
	if (!d) return;
	// si la scene est un Diagram, on actualise le poseur de conducteur
	d -> setConductorStop(e -> scenePos());
	
	// on recupere la liste des qgi sous le pointeur
	QList<QGraphicsItem *> qgis = d -> items(e -> scenePos());
	
	/* le qgi le plus haut
	   = le poseur de conductor
	   = le premier element de la liste
	   = la liste ne peut etre vide
	   = on prend le deuxieme element de la liste
	*/
	Q_ASSERT_X(!(qgis.isEmpty()), "Terminal::mouseMoveEvent", "La liste d'items ne devrait pas etre vide");
	
	// s'il n'y rien d'autre que le poseur de conducteur dans la liste, on arrete la
	if (qgis.size() <= 1) return;
	
	// sinon on prend le deuxieme element de la liste et on verifie s'il s'agit d'une borne
	QGraphicsItem *qgi = qgis.at(1);
	// si le qgi est une borne...
	Terminal *other_terminal = qgraphicsitem_cast<Terminal *>(qgi);
	if (!other_terminal) return;
	previous_terminal_ = other_terminal;
	
	// s'il s'agit d'une borne, on lui applique l'effet hover approprie
	if (!canBeLinkedTo(other_terminal)) {
		other_terminal -> hovered_color_ = forbiddenColor;
	} else if (other_terminal -> conductorsCount()) {
		other_terminal -> hovered_color_ = warningColor;
	} else {
		other_terminal -> hovered_color_ = allowedColor;
	}
	
	other_terminal -> hovered_ = true;
	other_terminal -> update();
}

/**
	Gere le fait qu'on relache la souris sur la Borne.
	@param e L'evenement souris correspondant
*/
void Terminal::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	//setCursor(Qt::ArrowCursor);
	previous_terminal_ = 0;
	hovered_color_  = neutralColor;
	// verifie que la scene est bien un Diagram
	if (Diagram *d = diagram()) {
		// on arrete de dessiner l'apercu du conducteur
		d -> setConductor(false);
		// on recupere l'element sous le pointeur lors du MouseReleaseEvent
		QGraphicsItem *qgi = d -> itemAt(e -> scenePos());
		// s'il n'y a rien, on arrete la
		if (!qgi) return;
		// idem si l'element obtenu n'est pas une borne
		Terminal *other_terminal = qgraphicsitem_cast<Terminal *>(qgi);
		if (!other_terminal) return;
		// on remet la couleur de hover a sa valeur par defaut
		other_terminal -> hovered_color_ = neutralColor;
		other_terminal -> hovered_ = false;
		// on s'arrete la s'il n'est pas possible de relier les bornes
		if (!canBeLinkedTo(other_terminal)) return;
		// autrement, on pose un conducteur
		Conductor *new_conductor = new Conductor(this, other_terminal);
		new_conductor -> setProperties(d -> defaultConductorProperties);
		d -> undoStack().push(new AddConductorCommand(d, new_conductor));
		new_conductor -> autoText();
	}
}

/**
	Met a jour l'eventuel conducteur relie a la borne.
	@param newpos Position de l'element parent a prendre en compte
*/
void Terminal::updateConductor() {
	if (!scene() || !parentItem()) return;
	foreach (Conductor *conductor, conductors_) {
		if (conductor -> isDestroyed()) continue;
		conductor -> updatePath();
	}
}

/**
	@param other_terminal Autre borne
	@return true si cette borne est reliee a other_terminal, false sion
*/
bool Terminal::isLinkedTo(Terminal *other_terminal) {
	if (other_terminal == this) return(false);
	
	bool already_linked = false;
	foreach (Conductor *c, conductors_) {
		if (c -> terminal1 == other_terminal || c -> terminal2 == other_terminal) {
			already_linked = true;
			break;
		}
	}
	return(already_linked);
}

/**
	@param other_terminal Autre borne
	@return true si cette borne peut etre reliee a other_terminal, false sion
*/
bool Terminal::canBeLinkedTo(Terminal *other_terminal) {
	if (other_terminal == this) return(false);
	
	// l'autre borne appartient-elle au meme element ?
	bool same_element = other_terminal -> parentElement() == parentElement();
	// les connexions internes sont-elles autorisees ?
	bool internal_connections_allowed = parentElement() -> internalConnections();
	// les deux bornes sont-elles deja liees ?
	bool already_linked = isLinkedTo(other_terminal);
	// la liaison des deux bornes est-elle interdite ?
	bool link_forbidden = (same_element && !internal_connections_allowed) || already_linked;
	
	return(!link_forbidden);
}

/**
	@return La liste des conducteurs lies a cette borne
*/
QList<Conductor *> Terminal::conductors() const {
	return(conductors_);
}

/**
	Methode d'export en XML
	@param doc Le Document XML a utiliser pour creer l'element XML
	@return un QDomElement representant cette borne
*/
QDomElement Terminal::toXml(QDomDocument &doc) const {
	QDomElement qdo = doc.createElement("terminal");
	qdo.setAttribute("x", QString("%1").arg(dock_elmt_.x()));
	qdo.setAttribute("y",  QString("%1").arg(dock_elmt_.y()));
	qdo.setAttribute("orientation", ori_);
	qdo.setAttribute("number", number_terminal_);
	qdo.setAttribute("name", name_terminal_);
	qdo.setAttribute("nameHidden", name_terminal_hidden);
	return(qdo);
}

/**
	Permet de savoir si un element XML represente une borne
	@param terminal Le QDomElement a analyser
	@return true si le QDomElement passe en parametre est une borne, false sinon
*/
bool Terminal::valideXml(QDomElement &terminal) {
	// verifie le nom du tag
	if (terminal.tagName() != "terminal") return(false);
	
	// verifie la presence des attributs minimaux
	if (!terminal.hasAttribute("x")) return(false);
	if (!terminal.hasAttribute("y")) return(false);
	if (!terminal.hasAttribute("orientation")) return(false);
	
	bool conv_ok;
	// parse l'abscisse
	terminal.attribute("x").toDouble(&conv_ok);
	if (!conv_ok) return(false);
	
	// parse l'ordonnee
	terminal.attribute("y").toDouble(&conv_ok);
	if (!conv_ok) return(false);
	
	// parse l'id
	terminal.attribute("id").toInt(&conv_ok);
	if (!conv_ok) return(false);
	
	// parse l'orientation
	int terminal_or = terminal.attribute("orientation").toInt(&conv_ok);
	if (!conv_ok) return(false);
	if (terminal_or != Qet::North && terminal_or != Qet::South && terminal_or != Qet::East && terminal_or != Qet::West) return(false);
	
	// a ce stade, la borne est syntaxiquement correcte
	return(true);
}

/**
	Permet de savoir si un element XML represente cette borne. Attention, l'element XML n'est pas verifie
	@param terminal Le QDomElement a analyser
	@return true si la borne "se reconnait" (memes coordonnes, meme orientation), false sinon
*/
bool Terminal::fromXml(QDomElement &terminal) {
	number_terminal_ = terminal.attribute("number");
	name_terminal_ = terminal.attribute("name");
	name_terminal_hidden = terminal.attribute("nameHidden").toInt();
	return (
		qFuzzyCompare(terminal.attribute("x").toDouble(), dock_elmt_.x()) &&
		qFuzzyCompare(terminal.attribute("y").toDouble(), dock_elmt_.y()) &&
		(terminal.attribute("orientation").toInt() == ori_)
	);
}

/**
	@return le Diagram auquel cette borne appartient, ou 0 si cette borne est independant
*/
Diagram *Terminal::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}

/**
	@return L'element auquel cette borne est rattachee
*/
Element *Terminal::parentElement() const {
	return(parent_element_);
}

/**
 * @brief Conductor::relatedPotentialTerminal
 * Return terminal at the same potential from the same
 * parent element of @t.
 * For folio report, return the terminal of linked other report.
 * For Terminal element, return the other terminal of terminal element.
 * @param t terminal to start search
 * @param all_diagram :if true return all related terminal,
 * false return only terminal in the same diagram of @t
 * @return
 */
Terminal * relatedPotentialTerminal (const Terminal *terminal, const bool all_diagram) {
	// If terminal parent element is a folio report.
	if (all_diagram && terminal -> parentElement() -> linkType() & Element::AllReport) {
		QList <Element *> elmt_list = terminal -> parentElement() -> linkedElements();
		if (!elmt_list.isEmpty()) {
			return (elmt_list.first() -> terminals().first());
		}
	}
	// If terminal parent element is a Terminal element.
	else if (terminal -> parentElement() -> linkType() & Element::Terminale) {
		QList <Terminal *> terminals = terminal -> parentElement() -> terminals();
		terminals.removeAll(const_cast<Terminal *> (terminal));
		if (!terminals.isEmpty())
			return terminals.first();
	}

	return nullptr;
}

