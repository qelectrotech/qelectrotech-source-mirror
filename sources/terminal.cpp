/*
	Copyright 2006-2009 Xavier Guerrin
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
#include "element.h"
#include "conductor.h"
#include "diagramcommands.h"
#include "qetapp.h"

QColor Terminal::couleur_neutre   = QColor(Qt::blue);
QColor Terminal::couleur_autorise = QColor(Qt::darkGreen);
QColor Terminal::couleur_prudence = QColor("#ff8000");
QColor Terminal::couleur_interdit = QColor(Qt::red);
const qreal Terminal::terminalSize = 4.0;

/**
	Fonction privee pour initialiser la borne.
	@param pf  position du point d'amarrage pour un conducteur
	@param o   orientation de la borne : Qt::Horizontal ou Qt::Vertical
*/
void Terminal::initialise(QPointF pf, QET::Orientation o) {
	// definition du pount d'amarrage pour un conducteur
	amarrage_conductor  = pf;
	
	// definition de l'orientation de la terminal (par defaut : sud)
	if (o < QET::North || o > QET::West) sens = QET::South;
	else sens = o;
	
	// calcul de la position du point d'amarrage a l'element
	amarrage_elmt = amarrage_conductor;
	switch(sens) {
		case QET::North: amarrage_elmt += QPointF(0, Terminal::terminalSize);  break;
		case QET::East : amarrage_elmt += QPointF(-Terminal::terminalSize, 0); break;
		case QET::West : amarrage_elmt += QPointF(Terminal::terminalSize, 0);  break;
		case QET::South:
		default        : amarrage_elmt += QPointF(0, -Terminal::terminalSize);
	}
	
	// par defaut : pas de conducteur
	
	// QRectF null
	br = new QRectF();
	terminal_precedente = NULL;
	// divers
	setAcceptsHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton);
	hovered = false;
	setToolTip(QObject::tr("Borne", "tooltip"));
}

/**
	Constructeur par defaut
*/
Terminal::Terminal() :
	QGraphicsItem(0, 0),
	couleur_hovered(Terminal::couleur_neutre)
{
	initialise(QPointF(0.0, 0.0), QET::South);
}

/**
	initialise une borne
	@param pf  position du point d'amarrage pour un conducteur
	@param o   orientation de la borne : Qt::Horizontal ou Qt::Vertical
	@param e   Element auquel cette borne appartient
	@param s   Scene sur laquelle figure cette borne
*/
Terminal::Terminal(QPointF pf, QET::Orientation o, Element *e, Diagram *s) :
	QGraphicsItem(e, s),
	couleur_hovered(Terminal::couleur_neutre)
{
	initialise(pf, o);
}

/**
	initialise une borne
	@param pf_x Abscisse du point d'amarrage pour un conducteur
	@param pf_y Ordonnee du point d'amarrage pour un conducteur
	@param o    orientation de la borne : Qt::Horizontal ou Qt::Vertical
	@param e    Element auquel cette borne appartient
	@param s    Scene sur laquelle figure cette borne
*/
Terminal::Terminal(qreal pf_x, qreal pf_y, QET::Orientation o, Element *e, Diagram *s) :
	QGraphicsItem(e, s),
	couleur_hovered(Terminal::couleur_neutre)
{
	initialise(QPointF(pf_x, pf_y), o);
}

/**
	Destructeur
	La destruction de la borne entraine la destruction des conducteurs
	associes.
*/
Terminal::~Terminal() {
	//qDebug() << "Terminal::~Terminal" << (void *)this;
	foreach(Conductor *c, liste_conductors) delete c;
	delete br;
}

/**
	Permet de connaitre l'orientation de la borne. Si le parent de la borne
	est bien un Element, cette fonction renvoie l'orientation par rapport a
	la scene de la borne, en tenant compte du fait que l'element ait pu etre
	pivote. Sinon elle renvoie son sens normal.
	@return L'orientation actuelle de la Terminal.
*/
QET::Orientation Terminal::orientation() const {
	if (Element *elt = qgraphicsitem_cast<Element *>(parentItem())) {
		// orientations actuelle et par defaut de l'element
		QET::Orientation ori_cur = elt -> orientation().current();
		QET::Orientation ori_def = elt -> orientation().defaultOrientation();
		if (ori_cur == ori_def) return(sens);
		else {
			// calcul l'angle de rotation implique par l'orientation de l'element parent
			// angle de rotation de la borne sur la scene, divise par 90
			int angle = ori_cur - ori_def + sens;
			while (angle >= 4) angle -= 4;
			return((QET::Orientation)angle);
		}
	} else return(sens);
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
	foreach (Conductor* conductor, liste_conductors) {
		if (conductor -> terminal1 == autre_terminal || conductor -> terminal2 == autre_terminal) deja_liees = true;
	}
	
	// si les deux bornes sont deja reliees, on refuse d'ajouter le conducteur
	if (deja_liees) return(false);
	
	// sinon on ajoute le conducteur
	liste_conductors.append(f);
	return(true);
}

/**
	Enleve un conducteur donne a la borne
	@param f Conducteur a enlever
*/
void Terminal::removeConductor(Conductor *f) {
	//qDebug() << "Terminal::removeConductor" << (void *)this;
	int index = liste_conductors.indexOf(f);
	if (index == -1) return;
	liste_conductors.removeAt(index);
}

/**
	Fonction de dessin des bornes
	@param p Le QPainter a utiliser
	@param options Les options de dessin
	@param widget Le widget sur lequel on dessine
*/
void Terminal::paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) {
	p -> save();
	
#ifdef Q_WS_X11
	// corrige un bug de rendu ne se produisant que lors du rendu sur QGraphicsScene sous X11 au zoom par defaut
	static bool must_correct_rendering_bug = QETApp::settings().value("correct-rendering", false).toBool();
	if (must_correct_rendering_bug) {
		Diagram *dia = diagram();
		if (dia && options -> levelOfDetail == 1.0 && widget) {
			// calcule la rotation qu'a subi l'element
			qreal applied_rotation = 0.0;
			if (Element *elt = qgraphicsitem_cast<Element *>(parentItem())) {
				// orientations actuelle et par defaut de l'element
				QET::Orientation ori_cur = elt -> orientation().current();
				QET::Orientation ori_def = elt -> orientation().defaultOrientation();
				applied_rotation = 90.0 * (ori_cur - ori_def);
				while (applied_rotation < 360.0) applied_rotation += 360.0;
				while (applied_rotation > 360.0) applied_rotation -= 360.0;
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
	QPointF f = mapFromParent(amarrage_conductor);
	QPointF e = mapFromParent(amarrage_elmt);
	
	QPen t;
	t.setWidthF(1.0);
	
	// dessin de la borne en rouge
	t.setColor(Qt::red);
	p -> setPen(t);
	p -> drawLine(f, e);
	
	// dessin du point d'amarrage au conducteur en bleu
	t.setColor(couleur_hovered);
	p -> setPen(t);
	p -> setBrush(couleur_hovered);
	if (hovered) {
		p -> setRenderHint(QPainter::Antialiasing, true);
		p -> drawEllipse(QRectF(f.x() - 2.5, f.y() - 2.5, 5.0, 5.0));
	} else p -> drawPoint(f);
	
	p -> restore();
}

/**
	@return Le rectangle (en precision flottante) delimitant la borne et ses alentours.
*/
QRectF Terminal::boundingRect() const {
	if (br -> isNull()) {
		qreal afx = amarrage_conductor.x();
		qreal afy = amarrage_conductor.y();
		qreal aex = amarrage_elmt.x();
		qreal aey = amarrage_elmt.y();
		QPointF origine;
		origine = (afx <= aex && afy <= aey ? amarrage_conductor : amarrage_elmt);
		origine += QPointF(-3.0, -3.0);
		qreal w = qAbs((int)(afx - aex)) + 7;
		qreal h = qAbs((int)(afy - aey)) + 7;
		*br = QRectF(origine, QSizeF(w, h));
	}
	return(*br);
}

/**
	Gere l'entree de la souris sur la zone de la Borne.
*/
void Terminal::hoverEnterEvent(QGraphicsSceneHoverEvent *) {
	hovered = true;
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
	hovered = false;
	update();
}

/**
	Gere le fait qu'on enfonce un bouton de la souris sur la Borne.
	@param e L'evenement souris correspondant
*/
void Terminal::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	if (Diagram *s = diagram()) {
		s -> setConductorStart(mapToScene(QPointF(amarrage_conductor)));
		s -> setConductorStop(e -> scenePos());
		s -> setConductor(true);
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
	if (terminal_precedente != NULL) {
		if (terminal_precedente == this) hovered = true;
		else terminal_precedente -> hovered = false;
		terminal_precedente -> couleur_hovered = terminal_precedente -> couleur_neutre;
		terminal_precedente -> update();
	}
	
	
	Diagram *s = diagram();
	if (!s) return;
	// si la scene est un Diagram, on actualise le poseur de conducteur
	s -> setConductorStop(e -> scenePos());
	
	// on recupere la liste des qgi sous le pointeur
	QList<QGraphicsItem *> qgis = s -> items(e -> scenePos());
	
	/* le qgi le plus haut
	   = le poseur de conductor
	   = le premier element de la liste
	   = la liste ne peut etre vide
	   = on prend le deuxieme element de la liste
	*/
	Q_ASSERT_X(!(qgis.isEmpty()), "Terminal::mouseMoveEvent", "La liste d'items ne devrait pas etre vide");
	
	// s'il y a autre chose que le poseur de conducteur dans la liste
	if (qgis.size() > 1) {
		// on prend le deuxieme element de la liste
		QGraphicsItem *qgi = qgis.at(1);
		// si le qgi est une borne...
		if (Terminal *p = qgraphicsitem_cast<Terminal *>(qgi)) {
			// ...on lui applique l'effet hover approprie
			if (p == this) {
				// effet si l'on hover sur la borne de depart
				couleur_hovered = couleur_interdit;
			} else if (p -> parentItem() == parentItem()) {
				// effet si l'on hover sur une borne du meme appareil
				if (((Element *)parentItem()) -> internalConnections())
					p -> couleur_hovered = p -> couleur_autorise;
				else p -> couleur_hovered = p -> couleur_interdit;
			} else if (p -> nbConductors()) {
				// si la borne a deja un conducteur
				// verifie que cette borne n'est pas deja reliee a l'autre borne
				bool deja_reliee = false;
				foreach (Conductor *f, liste_conductors) {
					if (f -> terminal1 == p || f -> terminal2 == p) {
						deja_reliee = true;
						break;
					}
				}
				// interdit si les bornes sont deja reliees, prudence sinon
				p -> couleur_hovered = deja_reliee ? p -> couleur_interdit : p -> couleur_prudence;
			} else {
				// effet si on peut poser le conducteur
				p -> couleur_hovered = p -> couleur_autorise;
			}
			terminal_precedente = p;
			p -> hovered = true;
			p -> update();
		}
	}
}

/**
	Gere le fait qu'on relache la souris sur la Borne.
	@param e L'evenement souris correspondant
*/
void Terminal::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	//setCursor(Qt::ArrowCursor);
	terminal_precedente = NULL;
	couleur_hovered  = couleur_neutre;
	// verifie que la scene est bien un Diagram
	if (Diagram *s = diagram()) {
		// on arrete de dessiner l'apercu du conducteur
		s -> setConductor(false);
		// on recupere l'element sous le pointeur lors du MouseReleaseEvent
		QGraphicsItem *qgi = s -> itemAt(e -> scenePos());
		// s'il n'y a rien, on arrete la
		if (!qgi) return;
		// idem si l'element obtenu n'est pas une borne
		Terminal *p = qgraphicsitem_cast<Terminal *>(qgi);
		if (!p) return;
		// on remet la couleur de hover a sa valeur par defaut
		p -> couleur_hovered = p -> couleur_neutre;
		// idem s'il s'agit de la borne actuelle
		if (p == this) return;
		// idem s'il s'agit d'une borne de l'element actuel et que l'element n'a pas le droit de relier ses propres bornes
		bool cia = ((Element *)parentItem()) -> internalConnections();
		if (!cia) foreach(QGraphicsItem *item, parentItem() -> children()) if (item == p) return;
		// derniere verification : verifier que cette borne n'est pas deja reliee a l'autre borne
		foreach (Conductor *f, liste_conductors) if (f -> terminal1 == p || f -> terminal2 == p) return;
		// autrement, on pose un conducteur
		Conductor * new_conductor = new Conductor(this, p);
		new_conductor -> setProperties(s -> defaultConductorProperties);
		s -> undoStack().push(new AddConductorCommand(s, new_conductor));
	}
}

/**
	Met a jour l'eventuel conducteur relie a la Terminal.
	@param newpos Position de l'element parent a prendre en compte
*/
void Terminal::updateConductor(QPointF newpos) {
	if (!scene() || !parentItem()) return;
	foreach (Conductor *conductor, liste_conductors) {
		if (conductor -> isDestroyed()) continue;
		if (newpos == QPointF()) conductor -> update(QRectF());
		else {
			// determine la translation subie par l'element parent
			QPointF translation = newpos - parentItem() -> pos();
			// rafraichit le conducteur en tenant compte de la translation
			conductor -> updateWithNewPos(QRectF(), this, amarrageConductor() + translation);
		}
	}
}

/**
	@return La liste des conducteurs lies a cette borne
*/
QList<Conductor *> Terminal::conductors() const {
	return(liste_conductors);
}

/**
	Methode d'export en XML
	@param doc Le Document XML a utiliser pour creer l'element XML
	@return un QDomElement representant cette borne
*/
QDomElement Terminal::toXml(QDomDocument &doc) const {
	QDomElement qdo = doc.createElement("terminal");
	qdo.setAttribute("x", QString("%1").arg(amarrage_elmt.x()));
	qdo.setAttribute("y",  QString("%1").arg(amarrage_elmt.y()));
	qdo.setAttribute("orientation", sens);
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
	if (terminal_or != QET::North && terminal_or != QET::South && terminal_or != QET::East && terminal_or != QET::West) return(false);
	
	// a ce stade, la borne est syntaxiquement correcte
	return(true);
}

/**
	Permet de savoir si un element XML represente cette borne. Attention, l'element XML n'est pas verifie
	@param terminal Le QDomElement a analyser
	@return true si la borne "se reconnait" (memes coordonnes, meme orientation), false sinon
*/
bool Terminal::fromXml(QDomElement &terminal) {
	return (
		terminal.attribute("x").toDouble() == amarrage_elmt.x() &&\
		terminal.attribute("y").toDouble() == amarrage_elmt.y() &&\
		terminal.attribute("orientation").toInt() == sens
	);
}

/// @return le Diagram auquel cette borne appartient, ou 0 si cette borne est independant
Diagram *Terminal::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}
