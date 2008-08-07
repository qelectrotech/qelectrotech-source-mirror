/*
	Copyright 2006-2008 Xavier Guerrin
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
#include <QtDebug>
#include <limits>
#include "conductor.h"
#include "conductorsegment.h"
#include "conductorsegmentprofile.h"
#include "diagramtextitem.h"
#include "element.h"
#include "diagram.h"
#include "diagramcommands.h"
#define PR(x) qDebug() << #x " = " << x;

bool Conductor::pen_and_brush_initialized = false;
QPen Conductor::conductor_pen = QPen();
QBrush Conductor::conductor_brush = QBrush();
QBrush Conductor::square_brush = QBrush(Qt::darkGreen);
/**
	Constructeur
	@param p1     Premiere Borne a laquelle le conducteur est lie
	@param p2     Seconde Borne a laquelle le conducteur est lie
	@param parent Element parent du conducteur (0 par defaut)
	@param scene  QGraphicsScene a laquelle appartient le conducteur
*/
Conductor::Conductor(Terminal *p1, Terminal* p2, Element *parent, QGraphicsScene *scene) :
	QObject(),
	QGraphicsPathItem(parent, scene),
	terminal1(p1),
	terminal2(p2),
	destroyed(false),
	segments(NULL),
	moving_point(false),
	moving_segment(false),
	previous_z_value(zValue()),
	modified_path(false),
	has_to_save_profile(false)
{
	// ajout du conducteur a la liste de conducteurs de chacune des deux bornes
	bool ajout_p1 = terminal1 -> addConductor(this);
	bool ajout_p2 = terminal2 -> addConductor(this);
	
	// en cas d'echec de l'ajout (conducteur deja existant notamment)
	if (!ajout_p1 || !ajout_p2) return;
	
	// attributs de dessin par defaut (communs a tous les conducteurs)
	if (!pen_and_brush_initialized) {
		conductor_pen.setJoinStyle(Qt::MiterJoin);
		conductor_pen.setCapStyle(Qt::SquareCap);
		conductor_pen.setColor(Qt::black);
		conductor_pen.setStyle(Qt::SolidLine);
		conductor_pen.setWidthF(1.0);
		conductor_brush.setColor(Qt::white);
		conductor_brush.setStyle(Qt::NoBrush);
		pen_and_brush_initialized = true;
	}
	
	// par defaut, les 4 profils sont des profils nuls = il faut utiliser priv_calculeConductor
	conductor_profiles.insert(Qt::TopLeftCorner,     ConductorProfile());
	conductor_profiles.insert(Qt::TopRightCorner,    ConductorProfile());
	conductor_profiles.insert(Qt::BottomLeftCorner,  ConductorProfile());
	conductor_profiles.insert(Qt::BottomRightCorner, ConductorProfile());

	// calcul du rendu du conducteur
	priv_calculeConductor(terminal1 -> amarrageConductor(), terminal1 -> orientation(), terminal2 -> amarrageConductor(), terminal2 -> orientation());
	setFlags(QGraphicsItem::ItemIsSelectable);
	setAcceptsHoverEvents(true);
	
	// ajout du champ de texte editable
	text_item = new DiagramTextItem();
	text_item -> setFlag(QGraphicsItem::ItemIsMovable, false);
	text_item -> setTextInteractionFlags(Qt::TextEditorInteraction);
	text_item -> setPlainText(properties_.text);
	text_item -> previous_text = properties_.text;
	calculateTextItemPosition();
	text_item -> setParentItem(this);
	connect(
		text_item,
		SIGNAL(diagramTextChanged(DiagramTextItem *, const QString &, const QString &)),
		this,
		SLOT(displayedTextChanged())
	);
}

/**
	Destructeur
	Detruit le conducteur ainsi que ses segments. Il ne detruit pas les bornes
	mais s'en detache
*/
Conductor::~Conductor() {
	// se detache des bornes
	if (!isDestroyed()) destroy();
	
	// supprime les segments
	deleteSegments();
}

/**
	Met a jour la representation graphique du conducteur.
	@param rect Rectangle a mettre a jour
*/
void Conductor::update(const QRectF &rect) {
	// appelle la bonne fonction pour calculer l'aspect du conducteur
	if (nbSegments() && !conductor_profiles[currentPathType()].isNull()) {
		priv_modifieConductor(
			terminal1 -> amarrageConductor(), terminal1 -> orientation(),
			terminal2 -> amarrageConductor(), terminal2 -> orientation()
		);
	} else {
		priv_calculeConductor(
			terminal1 -> amarrageConductor(), terminal1 -> orientation(),
			terminal2 -> amarrageConductor(), terminal2 -> orientation()
		);
	}
	
	calculateTextItemPosition();
	QGraphicsPathItem::update(rect);
}

/**
	Met a jour la representation graphique du conducteur en considerant que la
	borne b a pour position pos. Cette fonction est appelee lorsqu'une seule
	des bornes du conducteur a change de position.
	@param rect Rectangle a mettre a jour
	@param b Borne
	@param newpos position de la borne b
*/
void Conductor::updateWithNewPos(const QRectF &rect, const Terminal *b, const QPointF &newpos) {
	QPointF p1, p2;
	if (b == terminal1) {
		p1 = newpos;
		p2 = terminal2 -> amarrageConductor();
	} else if (b == terminal2) {
		p1 = terminal1 -> amarrageConductor();
		p2 = newpos;
	} else {
		p1 = terminal1 -> amarrageConductor();
		p2 = terminal2 -> amarrageConductor();
	}
	if (nbSegments() && !conductor_profiles[currentPathType()].isNull())
		priv_modifieConductor(p1, terminal1 -> orientation(), p2, terminal2 -> orientation());
	else
		priv_calculeConductor(p1, terminal1 -> orientation(), p2, terminal2 -> orientation());
	calculateTextItemPosition();
	QGraphicsPathItem::update(rect);
}

/**
	Genere le QPainterPath a partir de la liste des points
*/
void Conductor::segmentsToPath() {
	// chemin qui sera dessine
	QPainterPath path;
	
	// s'il n'y a pa des segments, on arrete la
	if (segments == NULL) setPath(path);
	
	// demarre le chemin
	path.moveTo(segments -> firstPoint());
	
	// parcourt les segments pour dessiner le chemin
	ConductorSegment *segment = segments;
	while(segment -> hasNextSegment()) {
		path.lineTo(segment -> secondPoint());
		segment = segment -> nextSegment();
	}
	
	// termine le chemin
	path.lineTo(segment -> secondPoint());
	
	// affecte le chemin au conducteur
	setPath(path);
}

/**
	Gere les updates 
	@param p1 Coordonnees du point d'amarrage de la borne 1
	@param o1 Orientation de la borne 1
	@param p2 Coordonnees du point d'amarrage de la borne 2
	@param o2 Orientation de la borne 2
*/
void Conductor::priv_modifieConductor(const QPointF &p1, QET::Orientation, const QPointF &p2, QET::Orientation) {
	
	ConductorProfile &conductor_profile = conductor_profiles[currentPathType()];
	
	Q_ASSERT_X(conductor_profile.nbSegments(QET::Both) > 1, "Conductor::priv_modifieConductor", "pas de points a modifier");
	Q_ASSERT_X(!conductor_profile.isNull(),                 "Conductor::priv_modifieConductor", "pas de profil utilisable");
	
	// recupere les coordonnees fournies des bornes
	QPointF new_p1 = mapFromScene(p1);
	QPointF new_p2 = mapFromScene(p2);
	QRectF new_rect = QRectF(new_p1, new_p2);
	
	// recupere la largeur et la hauteur du profil
	qreal profile_width  = conductor_profile.width();
	qreal profile_height = conductor_profile.height();
	
	// calcule les differences verticales et horizontales a appliquer
	qreal h_diff = (qAbs(new_rect.width())  - qAbs(profile_width) ) * getSign(profile_width);
	qreal v_diff = (qAbs(new_rect.height()) - qAbs(profile_height)) * getSign(profile_height);
	
	// applique les differences aux segments
	QHash<ConductorSegmentProfile *, qreal> segments_lengths;
	segments_lengths.unite(shareOffsetBetweenSegments(h_diff, conductor_profile.horizontalSegments()));
	segments_lengths.unite(shareOffsetBetweenSegments(v_diff, conductor_profile.verticalSegments()));
	
	// en deduit egalement les coefficients d'inversion (-1 pour une inversion, +1 pour conserver le meme sens)
	int horiz_coeff = getCoeff(new_rect.width(),  profile_width);
	int verti_coeff = getCoeff(new_rect.height(), profile_height);
	
	// genere les nouveaux points
	QList<QPointF> points;
	points << new_p1;
	int limit = conductor_profile.segments.count() - 1;
	for (int i = 0 ; i < limit ; ++ i) {
		// dernier point
		QPointF previous_point = points.last();
		
		// profil de segment de conducteur en cours
		ConductorSegmentProfile *csp = conductor_profile.segments.at(i);
		
		// coefficient et offset a utiliser pour ce point
		qreal coeff = csp -> isHorizontal ? horiz_coeff : verti_coeff;
		qreal offset_applied = segments_lengths[csp];
		
		// applique l'offset et le coeff au point
		if (csp -> isHorizontal) {
			points << QPointF (
				previous_point.x() + (coeff * offset_applied),
				previous_point.y()
			);
		} else {
			points << QPointF (
				previous_point.x(),
				previous_point.y() + (coeff * offset_applied)
			);
		}
	}
	points << new_p2;
	pointsToSegments(points);
	segmentsToPath();
}

/**
	@param offset Longueur a repartir entre les segments
	@param segments_list Segments sur lesquels il faut repartir la longueur
	@param precision seuil en-deca duquel on considere qu'il ne reste rien a repartir
*/
QHash<ConductorSegmentProfile *, qreal> Conductor::shareOffsetBetweenSegments(
	const qreal &offset,
	const QList<ConductorSegmentProfile *> &segments_list,
	const qreal &precision
) const {
	// construit le QHash qui sera retourne
	QHash<ConductorSegmentProfile *, qreal> segments_hash;
	foreach(ConductorSegmentProfile *csp, segments_list) {
		segments_hash.insert(csp, csp -> length);
	}
	
	// memorise le signe de la longueur de chaque segement
	QHash<ConductorSegmentProfile *, int> segments_signs;
	foreach(ConductorSegmentProfile *csp, segments_hash.keys()) {
		segments_signs.insert(csp, getSign(csp -> length));
	}
	
	//qDebug() << "repartition d'un offset de" << offset << "px sur" << segments_list.count() << "segments";
	
	// repartit l'offset sur les segments
	qreal remaining_offset = offset;
	while (remaining_offset > precision || remaining_offset < -precision) {
		// recupere le nombre de segments differents ayant une longueur non nulle
		uint segments_count = 0;
		foreach(ConductorSegmentProfile *csp, segments_hash.keys()) if (segments_hash[csp]) ++ segments_count;
		//qDebug() << "  remaining_offset =" << remaining_offset;
		qreal local_offset = remaining_offset / segments_count;
		//qDebug() << "  repartition d'un offset local de" << local_offset << "px sur" << segments_count << "segments";
		remaining_offset = 0.0;
		foreach(ConductorSegmentProfile *csp, segments_hash.keys()) {
			// ignore les segments de longueur nulle
			if (!segments_hash[csp]) continue;
			// applique l'offset au segment
			//qreal segment_old_length = segments_hash[csp];
			segments_hash[csp] += local_offset;
			
			// (la longueur du segment change de signe) <=> (le segment n'a pu absorbe tout l'offset)
			if (segments_signs[csp] != getSign(segments_hash[csp])) {
				
				// on remet le trop-plein dans la reserve d'offset
				remaining_offset += qAbs(segments_hash[csp]) * getSign(local_offset);
				//qDebug() << "    trop-plein de" << qAbs(segments_hash[csp]) * getSign(local_offset) << "remaining_offset =" << remaining_offset;
				segments_hash[csp] = 0.0;
			} else {
				//qDebug() << "    offset local de" << local_offset << "accepte";
			}
		}
	}
	
	return(segments_hash);
}

/**
	Calcule un trajet "par defaut" pour le conducteur
	@param p1 Coordonnees du point d'amarrage de la borne 1
	@param o1 Orientation de la borne 1
	@param p2 Coordonnees du point d'amarrage de la borne 2
	@param o2 Orientation de la borne 2
*/
void Conductor::priv_calculeConductor(const QPointF &p1, QET::Orientation o1, const QPointF &p2, QET::Orientation o2) {
	QPointF sp1, sp2, depart, newp1, newp2, arrivee, depart0, arrivee0;
	QET::Orientation ori_depart, ori_arrivee;
	
	// s'assure qu'il n'y a ni points
	QList<QPointF> points;
	
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
		if ((ori_depart == QET::North && (ori_arrivee == QET::South || ori_arrivee == QET::West)) || (ori_depart == QET::East && ori_arrivee == QET::West)) {
			// cas « 3 »
			int ligne_inter_x = qRound(depart.x() + arrivee.x()) / 2;
			while (ligne_inter_x % Diagram::xGrid) -- ligne_inter_x;
			points << QPointF(ligne_inter_x, depart.y());
			points << QPointF(ligne_inter_x, arrivee.y());
		} else if ((ori_depart == QET::South && (ori_arrivee == QET::North || ori_arrivee == QET::East)) || (ori_depart == QET::West && ori_arrivee == QET::East)) {
			// cas « 4 »
			int ligne_inter_y = qRound(depart.y() + arrivee.y()) / 2;
			while (ligne_inter_y % Diagram::yGrid) -- ligne_inter_y;
			points << QPointF(depart.x(), ligne_inter_y);
			points << QPointF(arrivee.x(), ligne_inter_y);
		} else if ((ori_depart == QET::North || ori_depart == QET::East) && (ori_arrivee == QET::North || ori_arrivee == QET::East)) {
			points << QPointF(arrivee.x(), depart.y()); // cas « 2 »
		} else {
			points << QPointF(depart.x(), arrivee.y()); // cas « 1 »
		}
	} else {
		// trajet montant
		if ((ori_depart == QET::West && (ori_arrivee == QET::East || ori_arrivee == QET::South)) || (ori_depart == QET::North && ori_arrivee == QET::South)) {
			// cas « 3 »
			int ligne_inter_y = qRound(depart.y() + arrivee.y()) / 2;
			while (ligne_inter_y % Diagram::yGrid) -- ligne_inter_y;
			points << QPointF(depart.x(), ligne_inter_y);
			points << QPointF(arrivee.x(), ligne_inter_y);
		} else if ((ori_depart == QET::East && (ori_arrivee == QET::West || ori_arrivee == QET::North)) || (ori_depart == QET::South && ori_arrivee == QET::North)) {
			// cas « 4 »
			int ligne_inter_x = qRound(depart.x() + arrivee.x()) / 2;
			while (ligne_inter_x % Diagram::xGrid) -- ligne_inter_x;
			points << QPointF(ligne_inter_x, depart.y());
			points << QPointF(ligne_inter_x, arrivee.y());
		} else if ((ori_depart == QET::West || ori_depart == QET::North) && (ori_arrivee == QET::West || ori_arrivee == QET::North)) {
			points << QPointF(depart.x(), arrivee.y()); // cas « 2 »
		} else {
			points << QPointF(arrivee.x(), depart.y()); // cas « 1 »
		}
	}
	
	// fin du vrai trajet
	points << arrivee;
	
	// prolongement de la borne d'arrivee
	points << arrivee0;
	
	// inverse eventuellement l'ordre des points afin que le trajet soit exprime de la borne 1 vers la borne 2
	if (newp1.x() > newp2.x()) {
		QList<QPointF> points2;
		for (int i = points.size() - 1 ; i >= 0 ; -- i) points2 << points.at(i);
		points = points2;
	}
	
	pointsToSegments(points);
	segmentsToPath();
}

/**
	Prolonge une borne.
	@param terminal Le point correspondant a la borne
	@param terminal_orientation L'orientation de la borne
	@param ext_size la taille de la prolongation
	@return le point correspondant a la borne apres prolongation
*/
QPointF Conductor::extendTerminal(const QPointF &terminal, QET::Orientation terminal_orientation, qreal ext_size) {
	QPointF extended_terminal;
	switch(terminal_orientation) {
		case QET::North:
			extended_terminal = QPointF(terminal.x(), terminal.y() - ext_size);
			break;
		case QET::East:
			extended_terminal = QPointF(terminal.x() + ext_size, terminal.y());
			break;
		case QET::South:
			extended_terminal = QPointF(terminal.x(), terminal.y() + ext_size);
			break;
		case QET::West:
			extended_terminal = QPointF(terminal.x() - ext_size, terminal.y());
			break;
		default: extended_terminal = terminal;
	}
	return(extended_terminal);
}

/**
	Dessine le conducteur sans antialiasing.
	@param qp Le QPainter a utiliser pour dessiner le conducteur
	@param options Les options de style pour le conducteur
	@param qw Le QWidget sur lequel on dessine 
*/
void Conductor::paint(QPainter *qp, const QStyleOptionGraphicsItem *options, QWidget */*qw*/) {
	qp -> save();
	qp -> setRenderHint(QPainter::Antialiasing, false);
	
	// affectation du QPen et de la QBrush modifies au QPainter
	qp -> setBrush(conductor_brush);
	qp -> setPen(conductor_pen);
	if (isSelected()) {
		QPen tmp = qp -> pen();
		tmp.setColor(Qt::red);
		qp -> setPen(tmp);
	}
	
	// dessin du conducteur
	qp -> drawPath(path());
	if (properties_.type == ConductorProperties::Single) {
		if (isSelected()) qp -> setBrush(Qt::red);
		properties_.singleLineProperties.draw(
			qp,
			middleSegment() -> isHorizontal() ? QET::Horizontal : QET::Vertical,
			QRectF(middleSegment() -> middle() - QPointF(10.0, 7.5), QSizeF(20.0, 15.0))
		);
		if (isSelected()) qp -> setBrush(Qt::NoBrush);
	}
	
	// decalage ideal pour le rendu centre d'un carre / cercle de 5.0 px de cote / diametre
	qreal pretty_offset = (options -> levelOfDetail == 1 ? 2.0 : 2.5);
	
	// dessin des points d'accroche du conducteur si celui-ci est selectionne
	if (isSelected()) {
		QList<QPointF> points = segmentsToPoints();
		QPointF previous_point;
		for (int i = 1 ; i < (points.size() -1) ; ++ i) {
			QPointF point = points.at(i);
			
			// dessine le carre de saisie du segment
			if (i > 1) {
				qp -> fillRect(
					QRectF(
						((previous_point.x() + point.x()) / 2.0 ) - pretty_offset,
						((previous_point.y() + point.y()) / 2.0 ) - pretty_offset,
						5.0,
						5.0
					),
					square_brush
				);
			}
			previous_point = point;
		}
	}
	
	// dessine les eventuelles jonctions
	QList<QPointF> junctions_list = junctions();
	if (!junctions_list.isEmpty()) {
		QBrush junction_brush(Qt::SolidPattern);
		junction_brush.setColor(isSelected() ? Qt::red : Qt::black);
		qp -> setBrush(junction_brush);
		qp -> setRenderHint(QPainter::Antialiasing, true);
		foreach(QPointF point, junctions_list) {
			qp -> drawEllipse(QRectF(point.x() - pretty_offset, point.y() - pretty_offset, 5.0, 5.0));
		}
	}
	qp -> restore();
}

/**
	Methode de preparation a la destruction du conducteur ; le conducteur se detache de ses deux bornes
*/
void Conductor::destroy() {
	destroyed = true;
	terminal1 -> removeConductor(this);
	terminal2 -> removeConductor(this);
}

/// @return le Diagram auquel ce conducteur appartient, ou 0 si ce conducteur est independant
Diagram *Conductor::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}

/**
	Methode de validation d'element XML
	@param e Un element XML sense represente un Conducteur
	@return true si l'element XML represente bien un Conducteur ; false sinon
*/
bool Conductor::valideXml(QDomElement &e){
	// verifie le nom du tag
	if (e.tagName() != "conductor") return(false);
	
	// verifie la presence des attributs minimaux
	if (!e.hasAttribute("terminal1")) return(false);
	if (!e.hasAttribute("terminal2")) return(false);
	
	bool conv_ok;
	// parse l'abscisse
	e.attribute("terminal1").toInt(&conv_ok);
	if (!conv_ok) return(false);
	
	// parse l'ordonnee
	e.attribute("terminal2").toInt(&conv_ok);
	if (!conv_ok) return(false);
	return(true);
}

/**
	Gere les clics sur le conducteur.
	@param e L'evenement decrivant le clic.
*/
void Conductor::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	// clic gauche
	if (e -> buttons() & Qt::LeftButton) {
		// recupere les coordonnees du clic
		press_point = e -> pos();
		
		/*
			parcourt les segments pour determiner si le clic a eu lieu
			- sur l'extremite d'un segment
			- sur le milieu d'un segment
			- ailleurs
		*/
		ConductorSegment *segment = segments;
		while (segment -> hasNextSegment()) {
			if (hasClickedOn(press_point, segment -> secondPoint())) {
				moving_point = true;
				moving_segment = false;
				previous_z_value = zValue();
				setZValue(5000.0);
				moved_segment = segment;
				break;
			} else if (hasClickedOn(press_point, segment -> middle())) {
				moving_point = false;
				moving_segment = true;
				previous_z_value = zValue();
				setZValue(5000.0);
				moved_segment = segment;
				break;
			}
			segment = segment -> nextSegment();
		}
	}
	QGraphicsPathItem::mousePressEvent(e);
}

/**
	Gere les deplacements de souris sur le conducteur.
	@param e L'evenement decrivant le deplacement de souris.
*/
void Conductor::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	// clic gauche
	if (e -> buttons() & Qt::LeftButton) {
		// position pointee par la souris
		qreal mouse_x = e -> pos().x();
		qreal mouse_y = e -> pos().y();
		
		bool snap_conductors_to_grid = e -> modifiers() ^ Qt::ShiftModifier;
		if (snap_conductors_to_grid) {
			mouse_x = qRound(mouse_x / (Diagram::xGrid * 1.0)) * Diagram::xGrid;
			mouse_y = qRound(mouse_y / (Diagram::yGrid * 1.0)) * Diagram::yGrid;
		}
		if (moving_point) {
			// la modification par points revient bientot
			/*
			// position precedente du point
			QPointF p = moved_segment -> secondPoint();
			qreal p_x = p.x();
			qreal p_y = p.y();
			
			// calcul du deplacement
			moved_segment -> moveX(mouse_x - p_x());
			moved_segment -> moveY(mouse_y - p_y());
			
			// application du deplacement
			modified_path = true;
			updatePoints();
			segmentsToPath();
			*/
		} else if (moving_segment) {
			// position precedente du point
			QPointF p = moved_segment -> middle();
			
			// calcul du deplacement
			moved_segment -> moveX(mouse_x - p.x());
			moved_segment -> moveY(mouse_y - p.y());
			
			// application du deplacement
			modified_path = true;
			has_to_save_profile = true;
			segmentsToPath();
			calculateTextItemPosition();
		}
	}
	QGraphicsPathItem::mouseMoveEvent(e);
}

/**
	Gere les relachements de boutons de souris sur le conducteur 
	@param e L'evenement decrivant le lacher de bouton.
*/
void Conductor::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	// clic gauche
	moving_point = false;
	moving_segment = false;
	if (has_to_save_profile) {
		saveProfile();
		has_to_save_profile = false;
	}
	setZValue(previous_z_value);
	QGraphicsPathItem::mouseReleaseEvent(e);
	calculateTextItemPosition();
}

/**
	Gere les mouvements de souris au dessus du conducteur
	@param e Le QGraphicsSceneHoverEvent decrivant l'evenement
*/
void Conductor::hoverMoveEvent(QGraphicsSceneHoverEvent *e) {
	/*
	if (isSelected()) {
		QPointF hover_point = mapFromScene(e -> pos());
		ConductorSegment *segment = segments;
		bool cursor_set = false;
		while (segment -> hasNextSegment()) {
			if (hasClickedOn(hover_point, segment -> secondPoint())) {
				setCursor(Qt::CrossCursor);
				cursor_set = true;
			} else if (hasClickedOn(hover_point, segment -> middle())) {
				setCursor(segment -> isVertical() ? Qt::SplitHCursor : Qt::SplitVCursor);
				cursor_set = true;
			}
			segment = segment -> nextSegment();
		}
		if (!cursor_set) setCursor(Qt::ArrowCursor);
	}
	*/
	QGraphicsPathItem::hoverMoveEvent(e);
}

/**
	@return Le rectangle delimitant l'espace de dessin du conducteur
*/
QRectF Conductor::boundingRect() const {
	QRectF retour = QGraphicsPathItem::boundingRect();
	retour.adjust(-5.0, -5.0, 5.0, 5.0);
	return(retour);
}

/**
	@return La forme / zone "cliquable" du conducteur
*/
QPainterPath Conductor::shape() const {
	QList<QPointF> points = segmentsToPoints();
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
	Renvoie une valeur donnee apres l'avoir bornee entre deux autres valeurs,
	en y ajoutant une marge interne.
	@param tobound valeur a borner
	@param bound1 borne 1
	@param bound2 borne 2
	@return La valeur bornee
*/
qreal Conductor::conductor_bound(qreal tobound, qreal bound1, qreal bound2, qreal space) {
	qDebug() << "will bound" << tobound << "between" << bound1 << "and" << bound2 ;
	if (bound1 < bound2) {
		return(qBound(bound1 + space, tobound, bound2 - space));
	} else {
		return(qBound(bound2 + space, tobound, bound1 - space));
	}
}

/**
	Renvoie une valeur donnee apres l'avoir bornee avant ou apres une valeur.
	@param tobound valeur a borner
	@param bound borne
	@param positive true pour borner la valeur avant la borne, false sinon
	@return La valeur bornee
*/
qreal Conductor::conductor_bound(qreal tobound, qreal bound, bool positive) {
	qreal space = 5.0;
	return(positive ? qMax(tobound, bound + space) : qMin(tobound, bound - space));
}

/**
	@param type Type de Segments
	@return Le nombre de segments composant le conducteur.
*/
uint Conductor::nbSegments(QET::ConductorSegmentType type) const {
	QList<ConductorSegment *> segments_list = segmentsList();
	if (type == QET::Both) return(segments_list.count());
	uint nb_seg = 0;
	foreach(ConductorSegment *conductor_segment, segments_list) {
		if (conductor_segment -> type() == type) ++ nb_seg;
	}
	return(nb_seg);
}

/**
	Genere une liste de points a partir des segments de ce conducteur
	@return La liste de points representant ce conducteur
*/
QList<QPointF> Conductor::segmentsToPoints() const {
	// liste qui sera retournee
	QList<QPointF> points_list;
	
	// on retourne la liste tout de suite s'il n'y a pas de segments
	if (segments == NULL) return(points_list);
	
	// recupere le premier point
	points_list << segments -> firstPoint();
	
	// parcourt les segments pour recuperer les autres points
	ConductorSegment *segment = segments;
	while(segment -> hasNextSegment()) {
		points_list << segment -> secondPoint();
		segment = segment -> nextSegment();
	}
	
	// recupere le dernier point
	points_list << segment -> secondPoint();
	
	//retourne la liste
	return(points_list);
}

/**
	Regenere les segments de ce conducteur a partir de la liste de points passee en parametre
	@param points_list Liste de points a utiliser pour generer les segments
*/
void Conductor::pointsToSegments(QList<QPointF> points_list) {
	// supprime les segments actuels
	deleteSegments();
	
	// cree les segments a partir de la liste de points
	ConductorSegment *last_segment = NULL;
	for (int i = 0 ; i < points_list.size() - 1 ; ++ i) {
		last_segment = new ConductorSegment(points_list.at(i), points_list.at(i + 1), last_segment);
		if (!i) segments = last_segment;
	}
}

/**
	Permet de savoir si un point est tres proche d'un autre. Cela sert surtout
	pour determiner si un clic a ete effectue pres d'un point donne.
	@param press_point Point effectivement clique
	@param point point cliquable
	@return true si l'on peut considerer que le point a ete clique, false sinon
*/
bool Conductor::hasClickedOn(QPointF press_point, QPointF point) const {
	return (
		press_point.x() >= point.x() - 5.0 &&\
		press_point.x() <  point.x() + 5.0 &&\
		press_point.y() >= point.y() - 5.0 &&\
		press_point.y() <  point.y() + 5.0
	);
}

/**
	Charge les caracteristiques du conducteur depuis un element XML.
	@param e Un element XML
	@return true si le chargement a reussi, false sinon
*/
bool Conductor::fromXml(QDomElement &e) {
	// recupere la "configuration" du conducteur
	properties_.fromXml(e);
	readProperties();
	
	// parcourt les elements XML "segment" et en extrait deux listes de longueurs
	// les segments non valides sont ignores
	QList<qreal> segments_x, segments_y;
	for (QDomNode node = e.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		// on s'interesse aux elements XML "segment"
		QDomElement current_segment = node.toElement();
		if (current_segment.isNull() || current_segment.tagName() != "segment") continue;
		
		// le segment doit avoir une longueur
		if (!current_segment.hasAttribute("length")) continue;
		
		// cette longueur doit etre un reel
		bool ok;
		qreal segment_length = current_segment.attribute("length").toDouble(&ok);
		if (!ok) continue;
		
		if (current_segment.attribute("orientation") == "horizontal") {
			segments_x << segment_length;
			segments_y << 0.0;
		} else {
			segments_x << 0.0;
			segments_y << segment_length;
		}
	}
	
	// s'il n'y a pas de segments, on renvoie true
	if (!segments_x.size()) return(true);
	// les longueurs recueillies doivent etre coherentes avec les positions des bornes
	qreal width = 0.0, height = 0.0;
	foreach (qreal t, segments_x) width  += t;
	foreach (qreal t, segments_y) height += t;
	QPointF t1 = terminal1 -> amarrageConductor();
	QPointF t2 = terminal2 -> amarrageConductor();
	qreal expected_width  = t2.x() - t1.x();
	qreal expected_height = t2.y() - t1.y();
	qreal precision = std::numeric_limits<qreal>::epsilon();
	if (
		expected_width > width + precision ||\
		expected_width < width - precision ||\
		expected_height > height + precision ||\
		expected_height < height - precision
	) return(false);
	
	/* on recree les segments a partir des donnes XML */
	// cree la liste de points
	QList<QPointF> points_list;
	points_list << t1;
	for (int i = 0 ; i < segments_x.size() ; ++ i) {
		points_list << QPointF(
			points_list.last().x() + segments_x.at(i),
			points_list.last().y() + segments_y.at(i)
		);
	}
	
	pointsToSegments(points_list);
	
	// initialise divers parametres lies a la modification des conducteurs
	modified_path = true;
	saveProfile(false);
	
	segmentsToPath();
	return(true);
}

/**
	Exporte les caracteristiques du conducteur sous forme d'une element XML.
	@param d Le document XML a utiliser pour creer l'element XML
	@param table_adr_id Hash stockant les correspondances entre les ids des
	bornes dans le document XML et leur adresse en memoire
	@return Un element XML representant le conducteur
*/
QDomElement Conductor::toXml(QDomDocument &d, QHash<Terminal *, int> &table_adr_id) const {
	QDomElement e = d.createElement("conductor");
	e.setAttribute("terminal1", table_adr_id.value(terminal1));
	e.setAttribute("terminal2", table_adr_id.value(terminal2));
	
	// on n'exporte les segments du conducteur que si ceux-ci ont
	// ete modifies par l'utilisateur
	if (modified_path) {
		// parcours et export des segments
		QDomElement current_segment;
		foreach(ConductorSegment *segment, segmentsList()) {
			current_segment = d.createElement("segment");
			current_segment.setAttribute("orientation", segment -> isHorizontal() ? "horizontal" : "vertical");
			current_segment.setAttribute("length", segment -> length());
			e.appendChild(current_segment);
		}
	}
	
	// exporte la "configuration" du conducteur
	properties_.toXml(d, e);
	return(e);
}

/// @return les segments de ce conducteur
const QList<ConductorSegment *> Conductor::segmentsList() const {
	if (segments == NULL) return(QList<ConductorSegment *>());
	
	QList<ConductorSegment *> segments_vector;
	ConductorSegment *segment = segments;
	
	while (segment -> hasNextSegment()) {
		segments_vector << segment;
		segment = segment -> nextSegment();
	}
	segments_vector << segment;
	return(segments_vector);
}

/**
	@return La longueur totale du conducteur
*/
qreal Conductor::length() {
	qreal length = 0.0;
	
	ConductorSegment *s = segments;
	while (s -> hasNextSegment()) {
		length += qAbs(s -> length());
		s = s -> nextSegment();
	}
	
	return(length);
}

/**
	@return Le segment qui contient le point au milieu du conducteur
*/
ConductorSegment *Conductor::middleSegment() {
	if (segments == NULL) return(NULL);
	
	qreal half_length = length() / 2.0;
	
	ConductorSegment *s = segments;
	qreal l = 0;
	
	while (s -> hasNextSegment()) {
		l += qAbs(s -> length());
		if (l >= half_length) break;
		s = s -> nextSegment();
	}
	// s est le segment qui contient le point au milieu du conducteur
	return(s);
}

/**
	Positionne le texte du conducteur au milieu du segment qui contient le
	point au milieu du conducteur
	@see middleSegment()
*/
void Conductor::calculateTextItemPosition() {
	if (properties_.type != ConductorProperties::Multi) return;
	text_item -> setPos(middleSegment() -> middle());
}

/**
	Sauvegarde le profil courant du conducteur pour l'utiliser ulterieurement
	dans priv_modifieConductor.
*/
void Conductor::saveProfile(bool undo) {
	Qt::Corner current_path_type = currentPathType();
	ConductorProfile old_profile(conductor_profiles[current_path_type]);
	conductor_profiles[current_path_type].fromConductor(this);
	Diagram *dia = diagram();
	if (undo && dia) {
		dia -> undoStack().push(new ChangeConductorCommand(this, old_profile, conductor_profiles[current_path_type], current_path_type));
	}
}

/**
	@param value1 Premiere valeur
	@param value2 Deuxieme valeur
	@return 1 si les deux valeurs sont de meme signe, -1 sinon
*/
int Conductor::getCoeff(const qreal &value1, const qreal &value2) {
	return(getSign(value1) * getSign(value2));
}

/**
	@param value valeur
	@return 1 si valeur est negatif, 1 s'il est positif ou nul
*/
int Conductor::getSign(const qreal &value) {
	return(value < 0 ? -1 : 1);
}

/**
	Applique un nouveau profil a ce conducteur
	@param cp Profil a appliquer a ce conducteur
	@param path_type Type de trajet pour lequel ce profil convient
*/
void Conductor::setProfile(const ConductorProfile &cp, Qt::Corner path_type) {
	conductor_profiles[path_type] = cp;
	// si le type de trajet correspond a l'actuel
	if (currentPathType() == path_type) {
		if (conductor_profiles[path_type].isNull()) {
			priv_calculeConductor(terminal1 -> amarrageConductor(), terminal1 -> orientation(), terminal2 -> amarrageConductor(), terminal2 -> orientation());
			modified_path = false;
		} else {
			priv_modifieConductor(terminal1 -> amarrageConductor(), terminal1 -> orientation(), terminal2 -> amarrageConductor(), terminal2 -> orientation());
			modified_path = true;
		}
		if (type() == ConductorProperties::Multi) {
			calculateTextItemPosition();
		}
	}
}

/// @return le profil de ce conducteur
ConductorProfile Conductor::profile(Qt::Corner path_type) const {
	return(conductor_profiles[path_type]);
}

/// @return le texte du conducteur
QString Conductor::text() const {
	return(text_item -> toPlainText());
}

/**
	@param t Nouveau texte du conducteur
*/
void Conductor::setText(const QString &t) {
	text_item -> setPlainText(t);
	text_item -> previous_text = t;
}

/// @param p les proprietes de ce conducteur
void Conductor::setProperties(const ConductorProperties &p) {
	properties_ = p;
	readProperties();
}

/// @return les proprietes de ce conducteur
ConductorProperties Conductor::properties() const {
	return(properties_);
}

/**
	Relit les proprietes et les applique
*/
void Conductor::readProperties() {
	setText(properties_.text);
	text_item -> setVisible(properties_.type == ConductorProperties::Multi);
}

/**
	Met a jour les proprietes du conducteur apres modification du champ de texte affiche
*/
void Conductor::displayedTextChanged() {
	// verifie que le texte a reellement change
	if (text_item -> toPlainText() == properties_.text) return;
	
	// initialise l'objet UndoCommand correspondant
	if (Diagram *my_diagram = diagram()) {
		ConductorProperties new_properties(properties_);
		new_properties.text = text_item -> toPlainText();
		
		ChangeConductorPropertiesCommand *ccpc = new ChangeConductorPropertiesCommand(this);
		ccpc -> setOldSettings(properties_);
		ccpc -> setNewSettings(new_properties);
		my_diagram -> undoStack().push(ccpc);
	}
}

/**
	@return les conducteurs avec lesquels ce conducteur partage des bornes
	communes
*/
QSet<Conductor *> Conductor::relatedConductors() const {
	QList<Conductor *> other_conductors_list = terminal1 -> conductors();
	other_conductors_list += terminal2 -> conductors();
	QSet<Conductor *> other_conductors = other_conductors_list.toSet();
	other_conductors.remove(const_cast<Conductor *>(this));
	return(other_conductors);
}

/**
	@return la liste des positions des jonctions avec d'autres conducteurs
*/
QList<QPointF> Conductor::junctions() const {
	QList<QPointF> junctions_list;
	
	// pour qu'il y ait des jonctions, il doit y avoir d'autres conducteurs et des bifurcations
	QSet<Conductor *> other_conductors = relatedConductors();
	QList<ConductorBend> bends_list = bends();
	if (other_conductors.isEmpty() || bends_list.isEmpty()) {
		return(junctions_list);
	}
	
	QList<QPointF> points = segmentsToPoints();
	for (int i = 1 ; i < (points.size() -1) ; ++ i) {
		QPointF point = points.at(i);
		
		// determine si le point est une bifurcation ou non
		bool is_bend = false;
		Qt::Corner current_bend_type = Qt::TopLeftCorner;
		foreach(ConductorBend cb, bends_list) {
			if (cb.first == point) {
				is_bend = true;
				current_bend_type = cb.second;
				break;
			}
		}
		// si le point n'est pas une bifurcation, il ne peut etre une jonction (enfin pas au niveau de ce conducteur)
		if (!is_bend) continue;
		
		bool is_junction = false;
		QPointF scene_point = mapToScene(point);
		foreach(Conductor *c, other_conductors) {
			// exprime le point dans les coordonnees de l'autre conducteur
			QPointF conductor_point = c -> mapFromScene(scene_point);
			// recupere les segments de l'autre conducteur
			QList<ConductorSegment *> c_segments = c -> segmentsList();
			if (c_segments.isEmpty()) continue;
			// parcoure les segments a la recherche d'un point commun
			for (int j = 0 ; j < c_segments.count() ; ++ j) {
				ConductorSegment *segment = c_segments[j];
				QRectF rect(segment -> firstPoint(), segment -> secondPoint());
				// un point commun a ete trouve sur ce segment
				if (rect.contains(conductor_point)) {
					is_junction = true;
					// ce point commun ne doit pas etre une bifurcation identique a celle-ci
					QList<ConductorBend> other_conductor_bends = c -> bends();
					foreach(ConductorBend cb, other_conductor_bends) {
						if (cb.first == conductor_point && cb.second == current_bend_type) {
							is_junction = false;
						}
					}
				}
				if (is_junction) junctions_list << point;
			}
		}
	}
	return(junctions_list);
}

/**
	@return la liste des bifurcations de ce conducteur ; ConductorBend est un
	typedef pour une QPair\<QPointF, Qt::Corner\>. Le point indique la position
	(en coordonnees locales) de la bifurcation tandis que le Corner indique le
	type de bifurcation.
*/
QList<ConductorBend> Conductor::bends() const {
	QList<ConductorBend> points;
	if (!segments) return(points);
	
	// recupere la liste des segments de taille non nulle
	QList<ConductorSegment *> visible_segments;
	ConductorSegment *segment = segments;
	while (segment -> hasNextSegment()) {
		if (!segment -> isPoint()) visible_segments << segment;
		segment = segment -> nextSegment();
	}
	if (!segment  -> isPoint()) visible_segments << segment;
	
	ConductorSegment *next_segment;
	for (int i = 0 ; i < visible_segments.count() -1 ; ++ i) {
		segment = visible_segments[i];
		next_segment = visible_segments[i + 1];
		if (!segment -> isPoint() && !next_segment -> isPoint()) {
			// si les deux segments ne sont pas dans le meme sens, on a une bifurcation
			if (next_segment -> type() != segment -> type()) {
				Qt::Corner bend_type;
				qreal sl = segment -> length();
				qreal nsl = next_segment -> length();
				
				if (segment -> isHorizontal()) {
					if (sl < 0 && nsl < 0) {
						bend_type = Qt::BottomLeftCorner;
					} else if (sl < 0 && nsl > 0) {
						bend_type = Qt::TopLeftCorner;
					} else if (sl > 0 && nsl < 0) {
						bend_type = Qt::BottomRightCorner;
					} else {
						bend_type = Qt::TopRightCorner;
					}
				} else {
					if (sl < 0 && nsl < 0) {
						bend_type = Qt::TopRightCorner;
					} else if (sl < 0 && nsl > 0) {
						bend_type = Qt::TopLeftCorner;
					} else if (sl > 0 && nsl < 0) {
						bend_type = Qt::BottomRightCorner;
					} else {
						bend_type = Qt::BottomLeftCorner;
					}
				}
				points << qMakePair(segment -> secondPoint(), bend_type);
			}
		}
	}
	return(points);
}

/**
	@param p Point, en coordonnees locales
	@return true si le point p appartient au trajet du conducteur
*/
bool Conductor::containsPoint(const QPointF &p) const {
	if (!segments) return(false);
	ConductorSegment *segment = segments;
	while (segment -> hasNextSegment()) {
		QRectF rect(segment -> firstPoint(), segment -> secondPoint());
		if (rect.contains(p)) return(true);
		segment = segment -> nextSegment();
	}
	return(false);
}

/**
	@param start Point de depart
	@param end Point d'arrivee
	@return le coin vers lequel se dirige le trajet de start vers end
*/
Qt::Corner Conductor::movementType(const QPointF &start, const QPointF &end) {
	Qt::Corner result = Qt::BottomRightCorner;
	if (start.x() <= end.x()) {
		result = start.y() <= end.y() ? Qt::BottomRightCorner : Qt::TopRightCorner;
	} else {
		result = start.y() <= end.y() ? Qt::BottomLeftCorner : Qt::TopLeftCorner;
	}
	return(result);
}

/// @return le type de trajet actuel de ce conducteur
Qt::Corner Conductor::currentPathType() const {
	return(movementType(terminal1 -> amarrageConductor(), terminal2 -> amarrageConductor()));
}

/// @return les profils de ce conducteur
ConductorProfilesGroup Conductor::profiles() const {
	return(conductor_profiles);
}

/**
	@param cpg Les nouveaux profils de ce conducteur
*/
void Conductor::setProfiles(const ConductorProfilesGroup &cpg) {
	conductor_profiles = cpg;
	if (conductor_profiles[currentPathType()].isNull()) {
		priv_calculeConductor(terminal1 -> amarrageConductor(), terminal1 -> orientation(), terminal2 -> amarrageConductor(), terminal2 -> orientation());
		modified_path = false;
	} else {
		priv_modifieConductor(terminal1 -> amarrageConductor(), terminal1 -> orientation(), terminal2 -> amarrageConductor(), terminal2 -> orientation());
		modified_path = true;
	}
	if (type() == ConductorProperties::Multi) {
		calculateTextItemPosition();
	}
}

/// Supprime les segments
void Conductor::deleteSegments() {
	if (segments != NULL) {
		while (segments -> hasNextSegment()) delete segments -> nextSegment();
		delete segments;
		segments = NULL;
	}
}
