#include <QtDebug>
#include "conducteur.h"
#include "element.h"

bool Conducteur::pen_and_brush_initialized = false;
QPen Conducteur::conducer_pen = QPen();
QBrush Conducteur::conducer_brush = QBrush();

/**
	Constructeur
	@param p1     Premiere Borne auquel le conducteur est lie
	@param p2     Seconde Borne auquel le conducteur est lie
	@param parent Element parent du conducteur (0 par defaut)
	@param scene  QGraphicsScene auquelle appartient le conducteur
*/
Conducteur::Conducteur(Terminal *p1, Terminal* p2, Element *parent, QGraphicsScene *scene) : QGraphicsPathItem(parent, scene) {
	// bornes que le conducteur relie
	terminal1 = p1;
	terminal2 = p2;
	// ajout du conducteur a la liste de conducteurs de chacune des deux bornes
	bool ajout_p1 = terminal1 -> addConducteur(this);
	bool ajout_p2 = terminal2 -> addConducteur(this);
	// en cas d'echec de l'ajout (conducteur deja existant notamment)
	if (!ajout_p1 || !ajout_p2) return;
	destroyed = false;
	modified_path = false;
	// attributs de dessin par defaut (communs a tous les conducteurs)
	if (!pen_and_brush_initialized) {
		conducer_pen.setJoinStyle(Qt::MiterJoin);
		conducer_pen.setCapStyle(Qt::SquareCap);
		conducer_pen.setColor(Qt::black);
		conducer_pen.setStyle(Qt::SolidLine);
		conducer_pen.setWidthF(1.0);
		conducer_brush.setColor(Qt::white);
		conducer_brush.setStyle(Qt::NoBrush);
		pen_and_brush_initialized = true;
	}
	// calcul du rendu du conducteur
	priv_calculeConducteur(terminal1 -> amarrageConducteur(), terminal1 -> orientation(), terminal2 -> amarrageConducteur(), terminal2 -> orientation());
	setFlags(QGraphicsItem::ItemIsSelectable);
}

/**
	Met a jour la representation graphique du conducteur.
	@param rect Rectangle a mettre a jour
*/
void Conducteur::update(const QRectF &rect) {
	// utilise soit la fonction priv_modifieConducteur soit la fonction priv_calculeConducteur
	void (Conducteur::* fonction_update) (const QPointF &, Terminal::Orientation, const QPointF &, Terminal::Orientation);
	fonction_update = (points.count() && modified_path) ? &Conducteur::priv_modifieConducteur : &Conducteur::priv_calculeConducteur;
	
	// appelle la bonne fonction pour calculer l'aspect du conducteur
	(this ->* fonction_update)(
		terminal1 -> amarrageConducteur(), terminal1 -> orientation(),
		terminal2 -> amarrageConducteur(), terminal2 -> orientation()
	);
	QGraphicsPathItem::update(rect);
}

/**
	Met a jour la representation graphique du conducteur en considerant que la borne b
	a pour position pos
	@param rect Rectangle a mettre a jour
	@param b Borne
	@param pos position de la borne b
*/
void Conducteur::updateWithNewPos(const QRectF &rect, const Terminal *b, const QPointF &newpos) {
	QPointF p1, p2;
	if (b == terminal1) {
		p1 = newpos;
		p2 = terminal2 -> amarrageConducteur();
	} else if (b == terminal2) {
		p1 = terminal1 -> amarrageConducteur();
		p2 = newpos;
	} else {
		p1 = terminal1 -> amarrageConducteur();
		p2 = terminal2 -> amarrageConducteur();
	}
	if (points.count() && modified_path)
		priv_modifieConducteur(p1, terminal1 -> orientation(), p2, terminal2 -> orientation());
	else
		priv_calculeConducteur(p1, terminal1 -> orientation(), p2, terminal2 -> orientation());
	QGraphicsPathItem::update(rect);
}

/**
	Genere le QPainterPath a partir de la liste des points
*/
void Conducteur::pointsToPath() {
	QPainterPath path;
	bool moveto_done = false;
	foreach(QPointF point, points) {
		if (!moveto_done) {
			path.moveTo(point);
			moveto_done = true;
		} else path.lineTo(point);
	}
	setPath(path);
}

/**
	Gere les updates 
	@param p1 Coordonnees du point d'amarrage de la borne 1
	@param o1 Orientation de la borne 1
	@param p2 Coordonnees du point d'amarrage de la borne 2
	@param o2 Orientation de la borne 2
*/
void Conducteur::priv_modifieConducteur(const QPointF &p1, Terminal::Orientation, const QPointF &p2, Terminal::Orientation) {
	Q_ASSERT_X(points.count() > 1, "priv_modifieConducteur", "pas de points a modifier");
	
	// recupere les dernieres coordonnees connues des bornes
	QPointF old_p1 = mapFromScene(terminal1 -> amarrageConducteur());
	QPointF old_p2 = mapFromScene(terminal2 -> amarrageConducteur());
	
	// recupere les coordonnees fournies des bornes
	QPointF new_p1 = mapFromScene(p1);
	QPointF new_p2 = mapFromScene(p2);
	
	// les distances horizontales et verticales entre les anciennes bornes
	// sont stockees dans orig_dist_2_terms_x et orig_dist_2_terms_y
	
	// calcule les distances horizontales et verticales entre les nouvelles bornes
	qreal new_dist_2_terminals_x = new_p2.x() - new_p1.x();
	qreal new_dist_2_terminals_y = new_p2.y() - new_p1.y();
	
	// en deduit les coefficients de "redimensionnement"
	qreal coeff_x = new_dist_2_terminals_x / orig_dist_2_terms_x;
	qreal coeff_y = new_dist_2_terminals_y / orig_dist_2_terms_y;
	
	// genere les nouveaux points
	int limite = moves_x.size() - 1;
	int coeff = type_trajet_x ? 1 : -1;
	points.clear();
	points << (type_trajet_x ? new_p1 : new_p2);
	for (int i = 0 ; i < limite ; ++ i) {
		QPointF previous_point = points.last();
		points << QPointF (
			previous_point.x() + (moves_x.at(i) * coeff_x * coeff),
			previous_point.y() + (moves_y.at(i) * coeff_y * coeff)
		);
	}
	points << (type_trajet_x ? new_p2 : new_p1);
	
	pointsToPath();
}

/**
	Calcule un trajet "par defaut" pour le conducteur
	@param p1 Coordonnees du point d'amarrage de la borne 1
	@param o1 Orientation de la borne 1
	@param p2 Coordonnees du point d'amarrage de la borne 2
	@param o2 Orientation de la borne 2
*/
void Conducteur::priv_calculeConducteur(const QPointF &p1, Terminal::Orientation o1, const QPointF &p2, Terminal::Orientation o2) {
	QPointF sp1, sp2, depart, newp1, newp2, arrivee, depart0, arrivee0;
	Terminal::Orientation ori_depart, ori_arrivee;
	points.clear();
	type_trajet_x = p1.x() < p2.x();
	// mappe les points par rapport a la scene
	sp1 = mapFromScene(p1);
	sp2 = mapFromScene(p2);
	
	// prolonge les bornes
	newp1 = extendTerminal(sp1, o1);
	newp2 = extendTerminal(sp2, o2);
	
	// distingue le depart de l'arrivee : le trajet se fait toujours de gauche a droite (apres prolongation)
	if (newp1.x() <= newp2.x()) {
		depart      = newp1;
		arrivee     = newp2;
		depart0     = sp1;
		arrivee0    = sp2;
		ori_depart  = o1;
		ori_arrivee = o2;
	} else {
		depart      = newp2;
		arrivee     = newp1;
		depart0     = sp2;
		arrivee0    = sp1;
		ori_depart  = o2;
		ori_arrivee = o1;
	}
	
	// debut du trajet
	points << depart0;
	
	// prolongement de la borne de depart 
	points << depart;
	
	// commence le vrai trajet
	if (depart.y() < arrivee.y()) {
		// trajet descendant
		if ((ori_depart == Terminal::Nord && (ori_arrivee == Terminal::Sud || ori_arrivee == Terminal::Ouest)) || (ori_depart == Terminal::Est && ori_arrivee == Terminal::Ouest)) {
			// cas « 3 »
			qreal ligne_inter_x = (depart.x() + arrivee.x()) / 2.0;
			points << QPointF(ligne_inter_x, depart.y());
			points << QPointF(ligne_inter_x, arrivee.y());
		} else if ((ori_depart == Terminal::Sud && (ori_arrivee == Terminal::Nord || ori_arrivee == Terminal::Est)) || (ori_depart == Terminal::Ouest && ori_arrivee == Terminal::Est)) {
			// cas « 4 »
			qreal ligne_inter_y = (depart.y() + arrivee.y()) / 2.0;
			points << QPointF(depart.x(), ligne_inter_y);
			points << QPointF(arrivee.x(), ligne_inter_y);
		} else if ((ori_depart == Terminal::Nord || ori_depart == Terminal::Est) && (ori_arrivee == Terminal::Nord || ori_arrivee == Terminal::Est)) {
			points << QPointF(arrivee.x(), depart.y()); // cas « 2 »
		} else {
			points << QPointF(depart.x(), arrivee.y()); // cas « 1 »
		}
	} else {
		// trajet montant
		if ((ori_depart == Terminal::Ouest && (ori_arrivee == Terminal::Est || ori_arrivee == Terminal::Sud)) || (ori_depart == Terminal::Nord && ori_arrivee == Terminal::Sud)) {
			// cas « 3 »
			qreal ligne_inter_y = (depart.y() + arrivee.y()) / 2.0;
			points << QPointF(depart.x(), ligne_inter_y);
			points << QPointF(arrivee.x(), ligne_inter_y);
		} else if ((ori_depart == Terminal::Est && (ori_arrivee == Terminal::Ouest || ori_arrivee == Terminal::Nord)) || (ori_depart == Terminal::Sud && ori_arrivee == Terminal::Nord)) {
			// cas « 4 »
			qreal ligne_inter_x = (depart.x() + arrivee.x()) / 2.0;
			points << QPointF(ligne_inter_x, depart.y());
			points << QPointF(ligne_inter_x, arrivee.y());
		} else if ((ori_depart == Terminal::Ouest || ori_depart == Terminal::Nord) && (ori_arrivee == Terminal::Ouest || ori_arrivee == Terminal::Nord)) {
			points << QPointF(depart.x(), arrivee.y()); // cas « 2 »
		} else {
			points << QPointF(arrivee.x(), depart.y()); // cas « 1 »
		}
	}
	
	// fin du vrai trajet
	points << arrivee;
	
	// prolongement de la borne d'arrivee
	points << arrivee0;
	
	pointsToPath();
}

/**
	Prolonge une borne.
	@param terminal Le point correspondant a la borne
	@param terminal_orientation L'orientation de la borne
	@param ext_size la taille de la prolongation
	@return le point correspondant a la borne apres prolongation
*/
QPointF Conducteur::extendTerminal(const QPointF &terminal, Terminal::Orientation terminal_orientation, qreal ext_size) {
	QPointF extended_terminal;
	switch(terminal_orientation) {
		case Terminal::Nord:
			extended_terminal = QPointF(terminal.x(), terminal.y() - ext_size);
			break;
		case Terminal::Est:
			extended_terminal = QPointF(terminal.x() + ext_size, terminal.y());
			break;
		case Terminal::Sud:
			extended_terminal = QPointF(terminal.x(), terminal.y() + ext_size);
			break;
		case Terminal::Ouest:
			extended_terminal = QPointF(terminal.x() - ext_size, terminal.y());
			break;
		default: extended_terminal = terminal;
	}
	return(extended_terminal);
}

/**
	Dessine le conducteur sans antialiasing.
	@param qp Le QPainter a utiliser pour dessiner le conducteur
	@param qsogi Les options de style pour le conducteur
	@param qw Le QWidget sur lequel on dessine 
*/
void Conducteur::paint(QPainter *qp, const QStyleOptionGraphicsItem */*qsogi*/, QWidget */*qw*/) {
	qp -> save();
	qp -> setRenderHint(QPainter::Antialiasing, false);
	
	// affectation du QPen et de la QBrush modifies au QPainter
	qp -> setBrush(conducer_brush);
	//qp -> setBrush(Qt::green);
	qp -> setPen(conducer_pen);
	if (isSelected()) {
		QPen tmp = qp -> pen();
		tmp.setColor(Qt::red);
		qp -> setPen(tmp);
	}
	
	// dessin du conducteur
	qp -> drawPath(path());
	
	// dessin des points d'accroche du conducteur si celui-ci est selectionne
	if (isSelected()) {
		qp -> setRenderHint(QPainter::Antialiasing, true);
		for (int i = 1 ; i < (points.size() -1) ; ++ i) {
			QPointF point = points.at(i);
			qp -> drawEllipse(QRectF(point.x() - 3.0, point.y() - 3.0, 6.0, 6.0));
		}
	}
	qp -> restore();
}

/**
	Indique si deux orientations de Borne sont sur le meme axe (Vertical / Horizontal).
	@param a La premiere orientation de Borne
	@param b La seconde orientation de Borne
	@return Un booleen a true si les deux orientations de bornes sont sur le meme axe
*/
bool Conducteur::surLeMemeAxe(Terminal::Orientation a, Terminal::Orientation b) {
	if ((a == Terminal::Nord || a == Terminal::Sud) && (b == Terminal::Nord || b == Terminal::Sud)) return(true);
	else if ((a == Terminal::Est || a == Terminal::Ouest) && (b == Terminal::Est || b == Terminal::Ouest)) return(true);
	else return(false);
}

/**
	Indique si une orientation de borne est horizontale (Est / Ouest).
	@param a L'orientation de borne
	@return True si l'orientation de borne est horizontale, false sinon
*/
bool Conducteur::estHorizontale(Terminal::Orientation a) {
	return(a == Terminal::Est || a == Terminal::Ouest);
}

/**
	Indique si une orientation de borne est verticale (Nord / Sud).
	@param a L'orientation de borne
	@return True si l'orientation de borne est verticale, false sinon
*/
bool Conducteur::estVerticale(Terminal::Orientation a) {
	return(a == Terminal::Nord || a == Terminal::Sud);
}

/**
	Methode de preparation a la destruction du conducteur ; le conducteur se detache de ses deux bornes
*/
void Conducteur::destroy() {
	destroyed = true;
	terminal1 -> removeConducteur(this);
	terminal2 -> removeConducteur(this);
}

/**
	Methode de validation d'element XML
	@param e Un element XML sense represente un Conducteur
	@return true si l'element XML represente bien un Conducteur ; false sinon
*/
bool Conducteur::valideXml(QDomElement &e){
	// verifie le nom du tag
	if (e.tagName() != "conducteur") return(false);
	
	// verifie la presence des attributs minimaux
	if (!e.hasAttribute("borne1")) return(false);
	if (!e.hasAttribute("borne2")) return(false);
	
	bool conv_ok;
	// parse l'abscisse
	e.attribute("borne1").toInt(&conv_ok);
	if (!conv_ok) return(false);
	
	// parse l'ordonnee
	e.attribute("borne2").toInt(&conv_ok);
	if (!conv_ok) return(false);
	return(true);
}

/**
	Gere les clics sur le conducteur.
	@param e L'evenement decrivant le clic.
*/
void Conducteur::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	// clic gauche
	if (e -> buttons() & Qt::LeftButton) {
		press_point = mapFromScene(e -> pos());
		moving_point = false;
		for (int i = 1 ; i < points.count() ; ++ i) {
			QPointF point = points.at(i);
			if (
				press_point.x() >= point.x() - 5.0 &&\
				press_point.x() <  point.x() + 5.0 &&\
				press_point.y() >= point.y() - 5.0 &&\
				press_point.y() <  point.y() + 5.0
			) {
				moving_point = true;
				moved_point = i;
				break;
			}
		}
	}
	QGraphicsPathItem::mousePressEvent(e);
}

/**
	Gere les deplacements de souris sur le conducteur.
	@param e L'evenement decrivant le deplacement de souris.
	@todo
	-calculer le trajet du conducteur differemment selon l'etat du flag "trajet modifie"
	-garder une liste des points constituants le trajet
	-lorsque le fil est selectionne, dessiner ces points (cercles)
	-lors d'un mousemoveevent: detecter la position du clic : si cela tombe dans la zone d'un point :
		-deplacer ce point en consequence
		-mettre le flag "trajet modifie" a true
	-gerer les contraintes
*/
void Conducteur::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	// clic gauche
	if (e -> buttons() & Qt::LeftButton) {
		if (moving_point) {
			/* recuperation de quelques joyeusetes tres souvent consultees */
			// indice du dernier point ( = point non modifiable)
			int ind_max_point = points.count() - 1;
			
			// position precedente du point
			QPointF p = points.at(moved_point);
			qreal p_x = p.x();
			qreal p_y = p.y();
			
			// position pointee par la souris
			qreal mouse_x = e -> pos().x();
			qreal mouse_y = e -> pos().y();
			
			// position du point apres le deplacement
			qreal new_pos_x;
			qreal new_pos_y;
			
			if (moved_point == 1 || moved_point == ind_max_point - 1) {
				/* premier et dernier points modifiables du conducteur */
				// repere le point qui va imposer la contrainte de base
				int ind_depend = moved_point == 1 ? 0 : ind_max_point;
				qreal depend_x = points.at(ind_depend).x();
				qreal depend_y = points.at(ind_depend).y();
				
				// repere le point voisin suivant
				int ind_voisin = moved_point == 1 ? 2 : moved_point - 1;
				qreal voisin_x = points.at(ind_voisin).x();
				qreal voisin_y = points.at(ind_voisin).y();
			 
				if (p_x == depend_x && p_y != depend_y) {
					// deplacements limites a l'axe vertical
					new_pos_x = p_x;
					// si on peut aller plus loin que le point voisin suivant, on le fait... en deplacant le point voisin
					if (p_x > voisin_x - 1 && p_x < voisin_x + 1) new_pos_y = conducer_bound(mouse_y, depend_y, voisin_y);
					else {
						new_pos_y = conducer_bound(mouse_y, depend_y, depend_y < voisin_y);
						points.replace(ind_voisin, QPointF(voisin_x, new_pos_y));
					}
				} else {
					// deplacements limites a l'axe horizontal
					// si on peut aller plus loin que le point voisin suivant, on le fait... en deplacant le point voisin
					if (p_y > voisin_y - 1 && p_y < voisin_y + 1) new_pos_x = conducer_bound(mouse_x, depend_x, voisin_x);
					else {
						new_pos_x = conducer_bound(mouse_x, depend_x, depend_x < voisin_x);
						points.replace(ind_voisin, QPointF(new_pos_x, voisin_y));
					}
					new_pos_y = p_y;
				}
			} else {
				/* autres points */
				new_pos_x = mouse_x;
				new_pos_y = mouse_y;
				
				/* deplace les deux points voisins (sans cela, le deplacement du point n'est pas possible) */
				// point precedent
				int ind_point_precedent = moved_point - 1;
				qreal pp_x = points.at(ind_point_precedent).x();
				qreal pp_y = points.at(ind_point_precedent).y();
				if (ind_point_precedent != 1) {
					if (pp_x > p_x - 1 && pp_x < p_x + 1) {
						points.replace(ind_point_precedent, QPointF(new_pos_x, pp_y));
					} else {
						points.replace(ind_point_precedent, QPointF(pp_x, new_pos_y));
					}
				} else {
					if (pp_x > p_x - 1 && pp_x < p_x + 1) {
						new_pos_x = p_x;
					} else {
						new_pos_y = p_y;
					}
				}
				
				// point suivant
				int ind_point_suivant = moved_point + 1;
				qreal ps_x = points.at(ind_point_suivant).x();
				qreal ps_y = points.at(ind_point_suivant).y();
				if (ind_point_suivant != ind_max_point - 1) {
					if (ps_x > p_x - 1 && ps_x < p_x + 1) {
						points.replace(ind_point_suivant, QPointF(new_pos_x, ps_y));
					} else {
						points.replace(ind_point_suivant, QPointF(ps_x, new_pos_y));
					}
				} else {
					if (ps_x > p_x - 1 && ps_x < p_x + 1) {
						new_pos_x = p_x;
					} else {
						new_pos_y = p_y;
					}
				}
			}
			
			// application du deplacement
			modified_path = true;
			points.replace(moved_point, QPointF(new_pos_x, new_pos_y));
			updatePoints();
			pointsToPath();
		}
	}
	QGraphicsPathItem::mouseMoveEvent(e);
}

/**
	Gere les relachements de boutons de souris sur le conducteur 
	@param e L'evenement decrivant le lacher de bouton.
*/
void Conducteur::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	// clic gauche
	if (e -> buttons() & Qt::LeftButton) {
		moving_point = false;
		QGraphicsPathItem::mouseReleaseEvent(e);
	}
}

/**
	@return Le rectangle delimitant l'espace de dessin du conducteur
*/
QRectF Conducteur::boundingRect() const {
	QRectF retour = QGraphicsPathItem::boundingRect();
	retour.adjust(-5.0, -5.0, 5.0, 5.0);
	return(retour);
}

/**
	@return La forme / zone "cliquable" du conducteur
*/
QPainterPath Conducteur::shape() const {
	QPainterPath area;
	QPointF previous_point;
	QPointF *point1, *point2;
	foreach(QPointF point, points) {
		if (!previous_point.isNull()) {
			if (point.x() == previous_point.x()) {
				if (point.y() <= previous_point.y()) {
					point1 = &point;
					point2 = &previous_point;
				} else {
					point1 = &previous_point;
					point2 = &point;
				}
			} else {
				if (point.x() <= previous_point.x()) {
					point1 = &point;
					point2 = &previous_point;
				} else {
					point1 = &previous_point;
					point2 = &point;
				}
			}
			qreal p1_x = point1 -> x();
			qreal p1_y = point1 -> y();
			qreal p2_x = point2 -> x();
			qreal p2_y = point2 -> y();
			area.setFillRule(Qt::OddEvenFill);
			area.addRect(p1_x - 5.0, p1_y - 5.0, 10.0 + p2_x - p1_x, 10.0 + p2_y - p1_y);
		}
		previous_point = point;
		area.setFillRule(Qt::WindingFill);
		area.addRect(point.x() - 5.0, point.y() - 5.0, 10.0, 10.0);
	}
	return(area);
}

/**
	Met à jour deux listes de reels.
*/
void Conducteur::updatePoints() {
	int s = points.size();
	moves_x.clear();
	moves_y.clear();
	for (int i = 1 ; i < s ; ++ i) {
		moves_x << points.at(i).x() - points.at(i - 1).x();
		moves_y << points.at(i).y() - points.at(i - 1).y();
	}
	QPointF b1 = points.at(0);
	QPointF b2 = points.at(s - 1);
	orig_dist_2_terms_x = b2.x() - b1.x();
	orig_dist_2_terms_y = b2.y() - b1.y();
}

qreal Conducteur::conducer_bound(qreal tobound, qreal bound1, qreal bound2) {
	qreal space = 5.0;
	if (bound1 < bound2) {
		return(qBound(bound1 + space, tobound, bound2 - space));
	} else {
		return(qBound(bound2 + space, tobound, bound1 - space));
	}
}

qreal Conducteur::conducer_bound(qreal tobound, qreal bound, bool positive) {
	qreal space = 5.0;
	return(positive ? qMax(tobound, bound + space) : qMin(tobound, bound - space));
}
