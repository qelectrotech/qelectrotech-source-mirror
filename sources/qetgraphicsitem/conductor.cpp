/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "conductor.h"
#include "conductorsegment.h"
#include "conductorsegmentprofile.h"
#include "conductortextitem.h"
#include "element.h"
#include "diagram.h"
#include "diagramcommands.h"
#include "qetdiagrameditor.h"
#include "terminal.h"
#include "conductorautonumerotation.h"
#include "conductorpropertiesdialog.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "numerotationcontextcommands.h"

#define PR(x) qDebug() << #x " = " << x;

bool Conductor::pen_and_brush_initialized = false;
QPen Conductor::conductor_pen = QPen();
QBrush Conductor::conductor_brush = QBrush();

/**
 * @brief Conductor::Conductor
 * Default constructor.
 * @param p1 : first terminal of this conductor.
 * @param p2 : second terminal of this conductor.
 */
Conductor::Conductor(Terminal *p1, Terminal* p2) :
	QObject(),
	QGraphicsPathItem(0),
	terminal1(p1),
	terminal2(p2),
	setSeq(true),
	freeze_label(false),
	bMouseOver(false),
	m_handler(10),
	text_item(0),
	segments(NULL),
	moving_segment(false),
	modified_path(false),
	has_to_save_profile(false),
	must_highlight_(Conductor::None)
{
		//Set the default conductor properties.
	if (p1->diagram())
		properties_ = p1->diagram()->defaultConductorProperties;
	else if (p2->diagram())
		properties_ = p2->diagram()->defaultConductorProperties;

		//set Zvalue at 11 to be upper than the DiagramImageItem and element
	setZValue(11);
	previous_z_value = zValue();

		//Add this conductor to the list of conductor of each of the two terminal
	bool ajout_p1 = terminal1 -> addConductor(this);
	bool ajout_p2 = terminal2 -> addConductor(this);
		//m_valid become false if the conductor can't be added to terminal (conductor already exist)
	m_valid = (!ajout_p1 || !ajout_p2) ? false : true;
	
		//Default attribut for paint a conductor
	if (!pen_and_brush_initialized)
	{
		conductor_pen.setJoinStyle(Qt::MiterJoin);
		conductor_pen.setCapStyle(Qt::SquareCap);
		conductor_pen.setColor(Qt::black);
		conductor_pen.setStyle(Qt::SolidLine);
		conductor_pen.setWidthF(1.0);
		conductor_brush.setColor(Qt::white);
		conductor_brush.setStyle(Qt::NoBrush);
		pen_and_brush_initialized = true;
	}
	
		//By default, the 4 profils are nuls -> we must to use priv_calculeConductor
	conductor_profiles.insert(Qt::TopLeftCorner,     ConductorProfile());
	conductor_profiles.insert(Qt::TopRightCorner,    ConductorProfile());
	conductor_profiles.insert(Qt::BottomLeftCorner,  ConductorProfile());
	conductor_profiles.insert(Qt::BottomRightCorner, ConductorProfile());

		//Generate the path of this conductor.
	generateConductorPath(terminal1 -> dockConductor(), terminal1 -> orientation(), terminal2 -> dockConductor(), terminal2 -> orientation());
	setFlags(QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
	
		// Add the text field
	text_item = new ConductorTextItem(properties_.text, this);
	connect(text_item, &ConductorTextItem::diagramTextChanged, this, &Conductor::displayedTextChanged);
}

/**
 * @brief Conductor::~Conductor
 * Destructor. The conductor is removed from is terminal
 */
Conductor::~Conductor()
{
	terminal1->removeConductor(this);
	terminal2->removeConductor(this);
	deleteSegments();
}

/**
 * @brief Conductor::isValid
 * @return true if conductor is valid else false;
 * A non valid conductor, is a conductor without two terminal
 */
bool Conductor::isValid() const {
	return m_valid;
}

/**
	Met a jour la representation graphique du conducteur en recalculant son
	trace. Cette fonction est typiquement appelee lorsqu'une seule des bornes du
	conducteur a change de position.
	@param rect Rectangle a mettre a jour
	@see QGraphicsPathItem::update()
*/
void Conductor::updatePath(const QRectF &rect) {
	QPointF p1, p2;
	p1 = terminal1 -> dockConductor();
	p2 = terminal2 -> dockConductor();
	if (segmentsCount() && !conductor_profiles[currentPathType()].isNull())
		updateConductorPath(p1, terminal1 -> orientation(), p2, terminal2 -> orientation());
	else
		generateConductorPath(p1, terminal1 -> orientation(), p2, terminal2 -> orientation());
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
void Conductor::updateConductorPath(const QPointF &p1, Qet::Orientation o1, const QPointF &p2, Qet::Orientation o2) {
	Q_UNUSED(o1);
	Q_UNUSED(o2);
	
	ConductorProfile &conductor_profile = conductor_profiles[currentPathType()];
	
	Q_ASSERT_X(conductor_profile.segmentsCount(QET::Both) > 1, "Conductor::priv_modifieConductor", "pas de points a modifier");
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
void Conductor::generateConductorPath(const QPointF &p1, Qet::Orientation o1, const QPointF &p2, Qet::Orientation o2) {
	QPointF sp1, sp2, depart, newp1, newp2, arrivee, depart0, arrivee0;
	Qet::Orientation ori_depart, ori_arrivee;
	
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
		if ((ori_depart == Qet::North && (ori_arrivee == Qet::South || ori_arrivee == Qet::West)) || (ori_depart == Qet::East && ori_arrivee == Qet::West)) {
			// cas "3"
			int ligne_inter_x = qRound(depart.x() + arrivee.x()) / 2;
			while (ligne_inter_x % Diagram::xGrid) -- ligne_inter_x;
			points << QPointF(ligne_inter_x, depart.y());
			points << QPointF(ligne_inter_x, arrivee.y());
		} else if ((ori_depart == Qet::South && (ori_arrivee == Qet::North || ori_arrivee == Qet::East)) || (ori_depart == Qet::West && ori_arrivee == Qet::East)) {
			// cas "4"
			int ligne_inter_y = qRound(depart.y() + arrivee.y()) / 2;
			while (ligne_inter_y % Diagram::yGrid) -- ligne_inter_y;
			points << QPointF(depart.x(), ligne_inter_y);
			points << QPointF(arrivee.x(), ligne_inter_y);
		} else if ((ori_depart == Qet::North || ori_depart == Qet::East) && (ori_arrivee == Qet::North || ori_arrivee == Qet::East)) {
			points << QPointF(arrivee.x(), depart.y()); // cas "2"
		} else {
			points << QPointF(depart.x(), arrivee.y()); // cas "1"
		}
	} else {
		// trajet montant
		if ((ori_depart == Qet::West && (ori_arrivee == Qet::East || ori_arrivee == Qet::South)) || (ori_depart == Qet::North && ori_arrivee == Qet::South)) {
			// cas "3"
			int ligne_inter_y = qRound(depart.y() + arrivee.y()) / 2;
			while (ligne_inter_y % Diagram::yGrid) -- ligne_inter_y;
			points << QPointF(depart.x(), ligne_inter_y);
			points << QPointF(arrivee.x(), ligne_inter_y);
		} else if ((ori_depart == Qet::East && (ori_arrivee == Qet::West || ori_arrivee == Qet::North)) || (ori_depart == Qet::South && ori_arrivee == Qet::North)) {
			// cas "4"
			int ligne_inter_x = qRound(depart.x() + arrivee.x()) / 2;
			while (ligne_inter_x % Diagram::xGrid) -- ligne_inter_x;
			points << QPointF(ligne_inter_x, depart.y());
			points << QPointF(ligne_inter_x, arrivee.y());
		} else if ((ori_depart == Qet::West || ori_depart == Qet::North) && (ori_arrivee == Qet::West || ori_arrivee == Qet::North)) {
			points << QPointF(depart.x(), arrivee.y()); // cas "2"
		} else {
			points << QPointF(arrivee.x(), depart.y()); // cas "1"
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
QPointF Conductor::extendTerminal(const QPointF &terminal, Qet::Orientation terminal_orientation, qreal ext_size) {
	QPointF extended_terminal;
	switch(terminal_orientation) {
		case Qet::North:
			extended_terminal = QPointF(terminal.x(), terminal.y() - ext_size);
			break;
		case Qet::East:
			extended_terminal = QPointF(terminal.x() + ext_size, terminal.y());
			break;
		case Qet::South:
			extended_terminal = QPointF(terminal.x(), terminal.y() + ext_size);
			break;
		case Qet::West:
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
void Conductor::paint(QPainter *qp, const QStyleOptionGraphicsItem *options, QWidget *qw)
{
	Q_UNUSED(qw);
	qp -> save();
	qp -> setRenderHint(QPainter::Antialiasing, false);
	
	// determine la couleur du conducteur
	QColor final_conductor_color(properties_.color);
	if (must_highlight_ == Normal) {
		final_conductor_color = QColor::fromRgb(69, 137, 255, 255);
	} else if (must_highlight_ == Alert) {
		final_conductor_color =QColor::fromRgb(255, 69, 0, 255);
	} else if (isSelected()) {
		final_conductor_color = Qt::red;
	} else {
		if (Diagram *parent_diagram = diagram()) {
			if (!parent_diagram -> drawColoredConductors()) {
				final_conductor_color = Qt::black;
			}
		}
	}
	
		//Draw the conductor bigger when is hovered
	conductor_pen.setWidthF(bMouseOver? (properties_.cond_size) +4 : (properties_.cond_size));

	// affectation du QPen et de la QBrush modifies au QPainter
	qp -> setBrush(conductor_brush);
	QPen final_conductor_pen = conductor_pen;
	
	// modification du QPen generique pour lui affecter la couleur et le style adequats
	final_conductor_pen.setColor(final_conductor_color);
	final_conductor_pen.setStyle(properties_.style);
	final_conductor_pen.setJoinStyle(Qt::SvgMiterJoin); // meilleur rendu des pointilles
	
	// utilisation d'un trait "cosmetique" en-dessous d'un certain zoom
	if (options && options -> levelOfDetail < 1.0) {
		final_conductor_pen.setCosmetic(true);
	}
	
	qp -> setPen(final_conductor_pen);
	
	// dessin du conducteur
	qp -> drawPath(path());
	if (properties_.type == ConductorProperties::Single) {
		qp -> setBrush(final_conductor_color);
		properties_.singleLineProperties.draw(
			qp,
			middleSegment() -> isHorizontal() ? QET::Horizontal : QET::Vertical,
			QRectF(middleSegment() -> middle() - QPointF(12.0, 12.0), QSizeF(24.0, 24.0))
		);
		if (isSelected()) qp -> setBrush(Qt::NoBrush);
	}
	
		//Draw the squares used to modify the path of conductor when he is selected
	if (isSelected())
		m_handler.drawHandler(qp, handlerPoints());
	
	// dessine les eventuelles jonctions
	QList<QPointF> junctions_list = junctions();
	if (!junctions_list.isEmpty()) {
		final_conductor_pen.setStyle(Qt::SolidLine);
		QBrush junction_brush(final_conductor_color, Qt::SolidPattern);
		qp -> setPen(final_conductor_pen);
		qp -> setBrush(junction_brush);
		qp -> setRenderHint(QPainter::Antialiasing, true);
		foreach(QPointF point, junctions_list) {
			qp -> drawEllipse(QRectF(point.x() - 1.5, point.y() - 1.5, 3.0, 3.0));
		}
	}
	qp -> restore();
}

/// @return le Diagram auquel ce conducteur appartient, ou 0 si ce conducteur est independant
Diagram *Conductor::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}

/**4
	@return le champ de texte associe a ce conducteur
*/
ConductorTextItem *Conductor::textItem() const {
	return(text_item);
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
 * @brief Conductor::mouseDoubleClickEvent
 * Manage the mouse double click
 * @param event
 */
void Conductor::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	event->accept();
	editProperty();
}

/**
 * @brief Conductor::mousePressEvent
 * Manage the mouse press event
 * @param event
 */
void Conductor::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
		//Left clic
	if (event->buttons() & Qt::LeftButton)
	{
			//If user click on a handler (square used to modify the path of conductor),
			//we get the segment corresponding to the handler
		int index = m_handler.pointIsHoverHandler(event->pos(), handlerPoints());
		if (index > -1)
		{
			moving_segment = true;
			moved_segment = segmentsList().at(index+1);
			before_mov_text_pos_ = text_item -> pos();
		}
	}

	QGraphicsPathItem::mousePressEvent(event);

	if (event -> modifiers() & Qt::ControlModifier)
		setSelected(!isSelected());
}

/**
 * @brief Conductor::mouseMoveEvent
 * Manage the mouse move event
 * @param event
 */
void Conductor::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
		//Left clic
	if ((event->buttons() & Qt::LeftButton) && moving_segment)
	{
			//Snap the mouse pos to grid
		QPointF pos_ = Diagram::snapToGrid(event->pos());

			//Position of the last point
		QPointF p = moved_segment -> middle();

			//Calcul the movement
		moved_segment -> moveX(pos_.x() - p.x());
		moved_segment -> moveY(pos_.y() - p.y());
			
			//Apply the movement
		modified_path = true;
		has_to_save_profile = true;
		segmentsToPath();
		calculateTextItemPosition();
	}

	QGraphicsPathItem::mouseMoveEvent(event);
}

/**
 * @brief Conductor::mouseReleaseEvent
 * Manage the mouse release event
 * @param event
 */
void Conductor::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	moving_segment = false;
	if (has_to_save_profile)
	{
		saveProfile();
		has_to_save_profile = false;
	}

	if (!(event -> modifiers() & Qt::ControlModifier))
		QGraphicsPathItem::mouseReleaseEvent(event);
}

/**
 * @brief Conductor::hoverEnterEvent
 * Manage the hover enter event
 * @param event
 */
void Conductor::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
	Q_UNUSED(event);
	bMouseOver = true;
	update();
}

/**
 * @brief Conductor::hoverLeaveEvent
 * Manage the mouse leave event
 * @param event
 */
void Conductor::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
	Q_UNUSED(event);
	update();
	bMouseOver = false;
}

/**
 * @brief Conductor::hoverMoveEvent conductor
 * @param e QGraphicsSceneHoverEvent describing the event
 */
void Conductor::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	if (isSelected())
	{
			//If user hover an handler (square used to modify the path of conductor),
			//we get the segment corresponding to the handler
		int index = m_handler.pointIsHoverHandler(event->pos(), handlerPoints());
		if (index > -1)
		{
			ConductorSegment *segment_ = segmentsList().at(index+1);
			if (m_handler.pointIsInHandler(event->pos(), segment_->secondPoint()))
				setCursor(Qt::ForbiddenCursor);
			else if (m_handler.pointIsInHandler(event->pos(), segment_->middle()))
				setCursor(segmentsList().at(index+1)->isVertical() ? Qt::SplitHCursor : Qt::SplitVCursor);
		}
		else
			setCursor(Qt::ArrowCursor);
	}

	QGraphicsPathItem::hoverMoveEvent(event);
}

/**
	Gere les changements relatifs au conducteur
	Reimplemente ici pour :
	  * positionner le conducteur en avant-plan lorsqu'il est selectionne
	@param change Type de changement
	@param value  Valeur relative au changement
*/
QVariant Conductor::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (change == QGraphicsItem::ItemSelectedChange) {
		if (value.toBool()) {
			// le conducteur vient de se faire selectionner
			previous_z_value = zValue();
			setZValue(qAbs(previous_z_value) + 10000);
		} else {
			// le conducteur vient de se faire deselectionner
			setZValue(previous_z_value);
		}
	} else if (change == QGraphicsItem::ItemSceneHasChanged) {
		// permet de positionner correctement le texte du conducteur lors de son ajout a un schema
		calculateTextItemPosition();
	} else if (change == QGraphicsItem::ItemVisibleHasChanged) {
		// permet de positionner correctement le texte du conducteur lors de son ajout a un schema
		calculateTextItemPosition();
	}
	return(QGraphicsPathItem::itemChange(change, value));
}

/**
 * @brief Conductor::boundingRect
 * @return
 */
QRectF Conductor::boundingRect() const
{
	QRectF br = shape().boundingRect();
	return br.adjusted(-10, -10, 10, 10);
}

/**
 * @brief Conductor::shape
 * @return the shape of conductor.
 * The shape thickness is bigger when conductor is hovered
 */
QPainterPath Conductor::shape() const
{
	QPainterPathStroker pps;
	pps.setWidth(bMouseOver? 5 : 1);
	pps.setJoinStyle(conductor_pen.joinStyle());

	QPainterPath shape_(pps.createStroke(path()));

	if (isSelected())
		foreach (QRectF rect, m_handler.handlerRect(handlerPoints()))
			shape_.addRect(rect);

	return shape_;
}

/**
 * @brief Conductor::nearShape
 * @return : An area in which it is considered a point is near this conductor.
 */
QPainterPath Conductor::nearShape() const
{
	QPainterPathStroker pps;
	pps.setWidth(120);
	pps.setJoinStyle(conductor_pen.joinStyle());
	return pps.createStroke(path());
}

/**
	@param type Type de Segments
	@return Le nombre de segments composant le conducteur.
*/
uint Conductor::segmentsCount(QET::ConductorSegmentType type) const {
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
 * @brief Conductor::fromXml
 * Load the conductor and her information from xml element
 * @param e
 * @return true is loading success else return false
 */
bool Conductor::fromXml(QDomElement &e) {
	setPos(e.attribute("x", 0).toDouble(),
		   e.attribute("y", 0).toDouble());

	bool return_ = pathFromXml(e);

	text_item -> fromXml(e);
	ConductorProperties pr;
	pr.fromXml(e);

	//Load Sequential Values
	loadSequential(&e,"sequ_",&seq_unit);
	loadSequential(&e,"sequf_",&seq_unitfolio);
	loadSequential(&e,"seqt_",&seq_ten);
	loadSequential(&e,"seqtf_",&seq_tenfolio);
	loadSequential(&e,"seqh_",&seq_hundred);
	loadSequential(&e,"seqhf_",&seq_hundredfolio);

	m_frozen_label = e.attribute("frozenlabel");

	setProperties(pr);

	return return_;
}

/**
	Load Sequentials to display on conductor label
	@param QDomElement to set Attributes
	@param Qstring seq to be retrieved
	@param QStringList list to be inserted values
*/
void Conductor::loadSequential(QDomElement* e, QString seq, QStringList* list) {
	//Load Sequential Values
	int i = 0;
	while (!e->attribute(seq + QString::number(i+1)).isEmpty()) {
		list->append(e->attribute(seq + QString::number(i+1)));
		i++;
	}
	setSeq = false;
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

	e.setAttribute("x", pos().x());
	e.setAttribute("y", pos().y());
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
			current_segment.setAttribute("length", QString("%1").arg(segment -> length()));
			e.appendChild(current_segment);
		}
	}

	// Save Conductor sequential values to Xml
	// Save Unit Sequential Values
	for (int i = 0; i < seq_unit.size(); i++) {
			e.setAttribute("sequ_" + QString::number(i+1),seq_unit.at(i));
	}

	// Save UnitFolio Sequential Values
	for (int i = 0; i < seq_unitfolio.size(); i++) {
			e.setAttribute("sequf_" + QString::number(i+1),seq_unitfolio.at(i));
	}

	// Save Ten Sequential Values
	for (int i = 0; i < seq_ten.size(); i++) {
			e.setAttribute("seqt_" + QString::number(i+1),seq_ten.at(i));
	}

	// Save TenFolio Sequential Values
	for (int i = 0; i < seq_tenfolio.size(); i++) {
			e.setAttribute("seqtf_" + QString::number(i+1),seq_tenfolio.at(i));
	}

	// Save Hundred Sequential Values
	for (int i = 0; i < seq_hundred.size(); i++) {
			e.setAttribute("seqh_" + QString::number(i+1),seq_hundred.at(i));
	}

	// Save Hundred Sequential Values
	for (int i = 0; i < seq_hundredfolio.size(); i++) {
			e.setAttribute("seqhf_" + QString::number(i+1),seq_hundredfolio.at(i));
	}

	if (m_frozen_label != "") e.setAttribute("frozenlabel", m_frozen_label);
	
	// Export the properties and text
	properties_. toXml(e);
	text_item -> toXml(e);

	return(e);
}

/**
 * @brief Conductor::pathFromXml
 * Generate the path from xml file
 * @param e
 * @return true if generate path success else return false
 */
bool Conductor::pathFromXml(const QDomElement &e) {
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

	//If there isn't segment we generate automatic path and return true
	if (!segments_x.size()) {
		generateConductorPath(terminal1 -> dockConductor(), terminal1 -> orientation(), terminal2 -> dockConductor(), terminal2 -> orientation());
		return(true);
	}

	// les longueurs recueillies doivent etre coherentes avec les positions des bornes
	qreal width = 0.0, height = 0.0;
	foreach (qreal t, segments_x) width  += t;
	foreach (qreal t, segments_y) height += t;
	QPointF t1 = terminal1 -> dockConductor();
	QPointF t2 = terminal2 -> dockConductor();
	qreal expected_width  = t2.x() - t1.x();
	qreal expected_height = t2.y() - t1.y();

	// on considere que le trajet est incoherent a partir d'une unite de difference avec l'espacement entre les bornes
	if (
		qAbs(expected_width  - width)  > 1.0 ||
		qAbs(expected_height - height) > 1.0
	) {
		qDebug() << "Conductor::fromXml : les segments du conducteur ne semblent pas coherents - utilisation d'un trajet automatique";
		return(false);
	}

	/* on recree les segments a partir des donnes XML */
	// cree la liste de points
	QList<QPointF> points_list;
	points_list << mapFromScene(t1);
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
 * @brief Conductor::handlerPoints
 * @return The points used to draw the handler square, used to modify
 * the path of the conductor.
 * The points stored in the QVector are the middle point of each segments that compose the conductor,
 * at exception of the first and last segment because there just here to extend the terminal.
 */
QVector<QPointF> Conductor::handlerPoints() const
{
	QList <ConductorSegment *> sl = segmentsList();
	if (sl.size() >= 3)
	{
		sl.removeFirst();
		sl.removeLast();
	}

	QVector <QPointF> middle_points;

	foreach(ConductorSegment *segment, sl)
		middle_points.append(segment->middle());

	return middle_points;
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
 * @brief Conductor::length
 * @return the length of this conductor
 */
qreal Conductor::length() const{
	return path().length();
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
 * @brief Conductor::posForText
 * Calculate and return the better pos for text.
 * @param flag : flag is used to know if text pos is near of
 * a vertical or horizontal conductor segment.
 */
QPointF Conductor::posForText(Qt::Orientations &flag) {

	ConductorSegment *segment      = segments;
	bool all_segment_is_vertical   = true;
	bool all_segment_is_horizontal = true;

	//Go to first segement
	while (!segment->isFirstSegment()) {
		segment = segment->previousSegment();
	}


	QPointF p1 = segment -> firstPoint(); //<First point of conductor
	ConductorSegment *biggest_segment = segment; //<biggest segment: contain the longest segment of conductor.

	if (segment -> firstPoint().x() != segment -> secondPoint().x()) all_segment_is_vertical   = false;
	if (segment -> firstPoint().y() != segment -> secondPoint().y()) all_segment_is_horizontal = false;

	while (segment -> hasNextSegment()) {
		segment = segment -> nextSegment();

		if (segment -> firstPoint().x() != segment -> secondPoint().x()) all_segment_is_vertical   = false;
		if (segment -> firstPoint().y() != segment -> secondPoint().y()) all_segment_is_horizontal = false;

		//We must to compare length segment, but they can be negative
		//so we multiply by -1 to make it positive.
		int saved = biggest_segment -> length();
		if (saved < 0) saved *= -1;
		int curent = segment->length();
		if (curent < 0) curent *= -1;

		if (curent > saved) biggest_segment = segment;
	}

	QPointF p2 = segment -> secondPoint();//<Last point of conductor

	//If the conductor is horizontal or vertical
	//Return the point at the middle of conductor
	if (all_segment_is_vertical) { //<Vertical
		flag = Qt::Vertical;
		if (p1.y() > p2.y()) {
			p1.setY(p1.y() - (length()/2));
		} else {
			p1.setY(p1.y() + (length()/2));
		}
	} else if (all_segment_is_horizontal) { //<Horizontal
		flag = Qt::Horizontal;
		if (p1.x() > p2.x()) {
			p1.setX(p1.x() - (length()/2));
		} else {
			p1.setX(p1.x() + (length()/2));
		}
	} else { //Return the point at the middle of biggest segment.
		p1 = biggest_segment->middle();
		flag = (biggest_segment->isHorizontal())? Qt::Horizontal : Qt::Vertical;
	}
	return p1;
}

/**
 * @brief Conductor::calculateTextItemPosition
 * Move the text at middle of conductor (if is vertical or horizontal)
 * otherwise, move conductor at the middle of the longest segment of conductor.
 * If text was moved by user, this function do nothing, except check if text is near conductor.
 */
void Conductor::calculateTextItemPosition() {
	if (!text_item || !diagram() || properties_.type != ConductorProperties::Multi) return;

	if (diagram() -> defaultConductorProperties.m_one_text_per_folio == true &&
		relatedPotentialConductors(false).size() > 0) {

		Conductor *longuest_conductor = longuestConductorInPotential(this);

		//The longuest conductor isn't this conductor
		//we call calculateTextItemPosition of the longuest conductor
		if(longuest_conductor != this) {
			longuest_conductor -> calculateTextItemPosition();
			return;
		}

		//At this point this conductor is the longuest conductor we hide all text of conductor_list
		foreach (Conductor *c, relatedPotentialConductors(false)) {
					c -> textItem() -> setVisible(false);
			}
		//Make sure text item is visible
		text_item -> setVisible(true);
	}

	//position
	if (text_item -> wasMovedByUser()) {
		//Text field was moved by user :
		//we check if text field is yet  near the conductor
		QPointF text_item_pos = text_item -> pos();
		QPainterPath near_shape = nearShape();
		if (!near_shape.contains(text_item_pos)) {
			text_item -> setPos(movePointIntoPolygon(text_item_pos, near_shape));
		}
	} else {
		//Position and rotation of text is calculated.
		Qt::Orientations rotation;
		QPointF text_pos = posForText(rotation);

		if (!text_item -> wasRotateByUser()) {
			rotation == Qt::Vertical ? text_item -> setRotationAngle(properties_.verti_rotate_text):
									   text_item -> setRotationAngle(properties_.horiz_rotate_text);
		}

		//Adjust the position of text if his rotation
		//is 0° or 270°, to be exactly centered to the conductor
		if (text_item -> rotation() == 0)
			text_pos.rx() -= text_item -> boundingRect().width()/2;
		else if (text_item -> rotation() == 270)
			text_pos.ry() += text_item -> boundingRect().width()/2;

		//Finaly set the position of text
		text_item -> setPos(text_pos);
	}
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
		ChangeConductorCommand *undo_object = new ChangeConductorCommand(
			this,
			old_profile,
			conductor_profiles[current_path_type],
			current_path_type
		);
		undo_object -> setConductorTextItemMove(before_mov_text_pos_, text_item -> pos());
		dia -> undoStack().push(undo_object);
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
			generateConductorPath(terminal1 -> dockConductor(), terminal1 -> orientation(), terminal2 -> dockConductor(), terminal2 -> orientation());
			modified_path = false;
		} else {
			updateConductorPath(terminal1 -> dockConductor(), terminal1 -> orientation(), terminal2 -> dockConductor(), terminal2 -> orientation());
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
	QString label = text_item->toPlainText();
	return(label);
}

/**
 * @brief Conductor::assignVariables
 * Apply variables to conductor label
 * @param label to be processed
 * @return label with variables assigned
 */
QString Conductor::assignVariables(QString label) {
	//The check below was introduced to avoid crash caused by the addition of terminal elements
	//Needs further debbugging.
	if (diagram() == NULL) return label;

	//Titleblock Variables
		for (int i = 0; i < diagram()->border_and_titleblock.additionalFields().count(); i++)
	{
		QString folio_variable = diagram()->border_and_titleblock.additionalFields().keys().at(i);
		QVariant folio_value = diagram()->border_and_titleblock.additionalFields().operator [](folio_variable);

		if (label.contains(folio_variable)) {
			label.replace("%{" + folio_variable + "}", folio_value.toString());
			label.replace("%"  + folio_variable      , folio_value.toString());
		}
	}

	//Project Variables
	for (int i = 0; i < diagram()->project()->projectProperties().count(); i++)
	{
		QString folio_variable = diagram()->project()->projectProperties().keys().at(i);
		QVariant folio_value = diagram()->project()->projectProperties().operator [](folio_variable);

		if (label.contains(folio_variable)) {
			label.replace("%{" + folio_variable + "}", folio_value.toString());
			label.replace("%"  + folio_variable      , folio_value.toString());
		}
	}

	//Default Variables
	label.replace("%f", QString::number(diagram()->folioIndex()+1));
	label.replace("%F", diagram() -> border_and_titleblock.folio());
	label.replace("%id", QString::number(diagram()->folioIndex()+1));
	label.replace("%total", QString::number(diagram()->border_and_titleblock.folioTotal()));
	label.replace("%M",  diagram() -> border_and_titleblock.machine());
	label.replace("%LM",  diagram() -> border_and_titleblock.locmach());
	label = assignSeq(label, this);
	return label;
}

/**
 * @brief Conductor::setSequential
 * Set sequential values to conductor
 */
void Conductor::setSequential() {
	if (diagram()==NULL) return;
	QString conductor_currentAutoNum = diagram()->project()->conductorCurrentAutoNum();
	QString formula = diagram()->project()->conductorAutoNumCurrentFormula();
	QString label = this->text();
	NumerotationContext nc = diagram()->project()->conductorAutoNum(conductor_currentAutoNum);
	NumerotationContextCommands ncc (nc);
	if (!nc.isEmpty()) {
		if (label.contains("%sequ_"))
			setSequentialToList(&seq_unit,&nc,"unit");
		if (label.contains("%sequf_")) {
			setSequentialToList(&seq_unitfolio,&nc,"unitfolio");
			setFolioSequentialToHash(&seq_unitfolio,&diagram()->m_cnd_unitfolio_max,conductor_currentAutoNum);
		}
		if (label.contains("%seqt_"))
			setSequentialToList(&seq_ten,&nc,"ten");
		if (label.contains("%seqtf_")) {
			setSequentialToList(&seq_tenfolio,&nc,"tenfolio");
			setFolioSequentialToHash(&seq_tenfolio,&diagram()->m_cnd_tenfolio_max,conductor_currentAutoNum);
		}
		if (label.contains("%seqh_"))
			setSequentialToList(&seq_hundred,&nc,"hundred");
		if (label.contains("%seqhf_")) {
			setSequentialToList(&seq_hundredfolio,&nc,"hundredfolio");
			setFolioSequentialToHash(&seq_hundredfolio,&diagram()->m_cnd_hundredfolio_max,conductor_currentAutoNum);
		}
	this->diagram()->project()->addConductorAutoNum(conductor_currentAutoNum,ncc.next());
	}
}

/**
 * @brief Conductor::setSequentialToList
 * This class appends all sequential to selected list
 * @param list to have values inserted
 * @param nc to retrieve values from
 * @param sequential type
 */
void Conductor::setSequentialToList(QStringList* list, NumerotationContext* nc, QString type) {
	for (int i = 0; i < nc->size(); i++) {
		if (nc->itemAt(i).at(0) == type) {
			QString number;
			if (type == "ten" || type == "tenfolio")
				number = QString("%1").arg(nc->itemAt(i).at(1).toInt(), 2, 10, QChar('0'));
			else if (type == "hundred" || type == "hundredfolio")
				number = QString("%1").arg(nc->itemAt(i).at(1).toInt(), 3, 10, QChar('0'));
			else number = QString::number(nc->itemAt(i).at(1).toInt());
				list->append(number);
		}
	}
}

/**
 * @brief Conductor::setFolioSequentialToHash
 * This class inserts all conductors from list to hash
 * @param list to retrieve values from
 * @param hash to have values inserted
 * @param current element autonum to insert on hash
 */
void Conductor::setFolioSequentialToHash(QStringList* list, QHash<QString, QStringList> *hash, QString conductor_currentAutoNum) {
	if (hash->isEmpty() || (!(hash->contains(conductor_currentAutoNum)))) {
		QStringList max;
		for (int i = 0; i < list->size(); i++) {
			max.append(list->at(i));
		}
		hash->insert(conductor_currentAutoNum,max);
	}
	else if (hash->contains(conductor_currentAutoNum)) {
		//Load the String List and update it
		QStringList max = hash->value(conductor_currentAutoNum);
		for (int i = 0; i < list->size(); i++) {
			if ((list->at(i).toInt()) > max.at(i).toInt()) {
				max.replace(i,list->at(i));
				hash->remove(conductor_currentAutoNum);
				hash->insert(conductor_currentAutoNum,max);
			}
		}
	}
}

/**
 * @brief Conductor::assignSeq
 * Replace sequential values to conductor label
 * @param label to be replaced
 * @return replaced label
 */
QString Conductor::assignSeq(QString label, Conductor* cnd) {
	for (int i = 1; i <= qMax(qMax(qMax(cnd->seq_unitfolio.size(), cnd->seq_tenfolio.size()),qMax(cnd->seq_hundredfolio.size(),cnd->seq_unit.size())),qMax(cnd->seq_hundred.size(),cnd->seq_ten.size())); i++) {
		if (label.contains("%sequ_" + QString::number(i)) && !cnd->seq_unit.isEmpty()) {
			label.replace("%sequ_" + QString::number(i),cnd->seq_unit.at(i-1));
		}
		if (label.contains("%seqt_" + QString::number(i)) && !cnd->seq_ten.isEmpty()) {
			label.replace("%seqt_" + QString::number(i),cnd->seq_ten.at(i-1));
		}
		if (label.contains("%seqh_" + QString::number(i)) && !cnd->seq_hundred.isEmpty()) {
			label.replace("%seqh_" + QString::number(i),cnd->seq_hundred.at(i-1));
		}
		if (label.contains("%sequf_" + QString::number(i)) && !cnd->seq_unitfolio.isEmpty()) {
			label.replace("%sequf_" + QString::number(i),cnd->seq_unitfolio.at(i-1));
		}
		if (label.contains("%seqtf_" + QString::number(i)) && !cnd->seq_tenfolio.isEmpty()) {
			label.replace("%seqtf_" + QString::number(i),cnd->seq_tenfolio.at(i-1));
		}
		if (label.contains("%seqhf_" + QString::number(i)) && !cnd->seq_hundredfolio.isEmpty()) {
			label.replace("%seqhf_" + QString::number(i),cnd->seq_hundredfolio.at(i-1));
		}
	}
	return label;
}

/**
 * @brief Conductor::setOthersSequential
 * Copy sequentials from conductor in argument to this conductor
 * @param conductor to copy sequentials from
 */
void Conductor::setOthersSequential(Conductor *other) {
	QString conductor_currentAutoNum = other->diagram()->project()->conductorCurrentAutoNum();
	NumerotationContext nc = other->diagram()->project()->conductorAutoNum(conductor_currentAutoNum);
	seq_unit = other->seq_unit;
	seq_unitfolio = other->seq_unitfolio;
	seq_ten = other->seq_ten;
	seq_tenfolio = other->seq_tenfolio;
	seq_hundred = other->seq_hundred;
	seq_hundredfolio = other->seq_hundredfolio;
}

/**
 * @brief Conductor::setText
 * The text of this conductor
 * @param t
 */
void Conductor::setText(const QString &t) {
	text_item->setPlainText(t);
	if (setSeq) {
		setSequential();
		setSeq = false;
	}
	QString label = assignVariables(t);
	text_item -> setPlainText(label);
}

/**
 * @brief Conductor::setProperties
 * Set new properties for this conductor
 * Also change the common properties for every conductors at the same potential.
 * (text, function and tension/protocol) other value of properties isn't changed.
 * @param properties : properties
 */
void Conductor::setProperties(const ConductorProperties &properties)
{
	if (properties_ == properties) return;

	properties_ = properties;

	foreach(Conductor *other_conductor, relatedPotentialConductors())
	{
		ConductorProperties other_properties = other_conductor->properties();
		other_properties.text = properties_.text;
		other_properties.color = properties_.color;
		other_properties.cond_size = properties_.cond_size;
		other_properties.m_function = properties_.m_function;
		other_properties.m_tension_protocol = properties_.m_tension_protocol;
		other_conductor->setProperties(other_properties);
	}
	setText(properties_.text);
	text_item -> setFontSize(properties_.text_size);

	 if (terminal1 != NULL && terminal1->diagram() != NULL) {
		if (terminal1->diagram()->item_paste)
			m_frozen_label = "";
		else
			m_frozen_label = properties_.text;
	}
	if (freeze_label)
		freezeLabel();
	if (properties_.type != ConductorProperties::Multi)
		text_item -> setVisible(false);
	else
		text_item -> setVisible(properties_.m_show_text);
	calculateTextItemPosition();
	update();

	emit propertiesChange();
}

/**
 * @brief Conductor::properties
 * @return the properties of this Conductor
 */
ConductorProperties Conductor::properties() const {
	return(properties_);
}

/**
	@return true si le conducteur est mis en evidence
*/
Conductor::Highlight Conductor::highlight() const {
	return(must_highlight_);
}

/**
	@param hl true pour mettre le conducteur en evidence, false sinon
*/
void Conductor::setHighlighted(Conductor::Highlight hl) {
	must_highlight_ = hl;
	update();
}

/**
 * @brief Conductor::displayedTextChanged
 * Update the properties (text) of this conductor and other conductors
 * at the same potential of this conductor.
 */
void Conductor::displayedTextChanged()
{
	if ((text_item->toPlainText() == assignVariables(properties_.text)) || !diagram()) return;

	QVariant old_value, new_value;
	old_value.setValue(properties_);
	ConductorProperties new_properties(properties_);
	new_properties.text = text_item -> toPlainText();
	new_value.setValue(new_properties);

	QPropertyUndoCommand *undo = new QPropertyUndoCommand(this, "properties", old_value, new_value);
	undo->setText(tr("Modifier les propriétés d'un conducteur", "undo caption"));

	if (!relatedPotentialConductors().isEmpty())
	{
		undo->setText(tr("Modifier les propriétés de plusieurs conducteurs", "undo caption"));

		foreach (Conductor *potential_conductor, relatedPotentialConductors())
		{
			old_value.setValue(potential_conductor->properties());
			ConductorProperties new_properties = potential_conductor->properties();
			new_properties.text = text_item->toPlainText();
			new_value.setValue(new_properties);
			new QPropertyUndoCommand (potential_conductor, "properties", old_value, new_value, undo);
		}
	}

	diagram()->undoStack().push(undo);
}


/**
 * @brief Conductor::relatedPotentialConductors
 * Return all conductors at the same potential of this conductor, this conductor isn't
 * part of the returned QSet.
 * @param all_diagram : if true search in all diagram of the project,
 * false search only in the parent diagram of this conductor
 * @param t_list, a list of terminal already cheched for the serach of potential.
 * @return  a QSet of conductor at the same potential.
 */
QSet<Conductor *> Conductor::relatedPotentialConductors(const bool all_diagram, QList <Terminal *> *t_list) {
	bool declar_t_list = false;
	if (t_list == 0) {
		declar_t_list = true;
		t_list = new QList <Terminal *>;
	}

	QSet <Conductor *> other_conductors;
	QList <Terminal *> this_terminal;
	this_terminal << terminal1 << terminal2;

	// Return all conductor of terminal 1 and 2
	foreach (Terminal *terminal, this_terminal) {
		if (!t_list -> contains(terminal)) {
			t_list -> append(terminal);
			QList <Conductor *> other_conductors_list_t = terminal -> conductors();

			//get terminal share the same potential of @terminal, of parent element
			Terminal *t1_bis = relatedPotentialTerminal(terminal, all_diagram);
			if (t1_bis && !t_list->contains(t1_bis)) {
				t_list -> append(t1_bis);
				other_conductors_list_t += t1_bis->conductors();
			}

			other_conductors_list_t.removeAll(this);
			// Research the conductors connected to conductors already found
			foreach (Conductor *c, other_conductors_list_t) {
				other_conductors += c -> relatedPotentialConductors(all_diagram, t_list);
			}
			other_conductors += other_conductors_list_t.toSet();
		}
	}

	other_conductors.remove(this);

	if (declar_t_list) delete t_list;
	return(other_conductors);
}

/**
 * @brief Conductor::diagramEditor
 * @return The parent diagram editor or nullptr;
 */
QETDiagramEditor* Conductor::diagramEditor() const {
	if (!diagram())                     return nullptr;
	if (diagram() -> views().isEmpty()) return nullptr;

	QWidget *w = const_cast<QGraphicsView *>(diagram() -> views().at(0));
	while (w -> parentWidget() && !w -> isWindow()) {
		w = w -> parentWidget();
	}
	return(qobject_cast<QETDiagramEditor *>(w));
}

/**
 * @brief Conductor::editProperty
 */
void Conductor::editProperty() {
	ConductorPropertiesDialog::PropertiesDialog(this, diagramEditor());
}

/**
	@param a point
	@param b point
	@param c point
	@return true si le point a est contenu dans le rectangle delimite par les points b et c
*/
bool isContained(const QPointF &a, const QPointF &b, const QPointF &c) {
	return(
		isBetween(a.x(), b.x(), c.x()) &&
		isBetween(a.y(), b.y(), c.y())
	);
}

/**
	@return la liste des positions des jonctions avec d'autres conducteurs
*/
QList<QPointF> Conductor::junctions() const {
	QList<QPointF> junctions_list;
	
	// pour qu'il y ait des jonctions, il doit y avoir d'autres conducteurs et des bifurcations
	QList<Conductor *> other_conductors = relatedConductors(this);
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
				// un point commun a ete trouve sur ce segment
				if (isContained(conductor_point, segment -> firstPoint(), segment -> secondPoint())) {
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
	return(movementType(terminal1 -> dockConductor(), terminal2 -> dockConductor()));
}

/// @return les profils de ce conducteur
ConductorProfilesGroup Conductor::profiles() const {
	return(conductor_profiles);
}

/**
 * @brief Conductor::setProfiles
 * @param cpg : the new profils of conductor
 */
void Conductor::setProfiles(const ConductorProfilesGroup &cpg) {
	conductor_profiles = cpg;
	if (conductor_profiles[currentPathType()].isNull()) {
		generateConductorPath(terminal1 -> dockConductor(), terminal1 -> orientation(), terminal2 -> dockConductor(), terminal2 -> orientation());
		modified_path = false;
	} else {
		updateConductorPath(terminal1 -> dockConductor(), terminal1 -> orientation(), terminal2 -> dockConductor(), terminal2 -> orientation());
		modified_path = true;
	}
	if (properties().type == ConductorProperties::Multi) {
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

/**
	@param point Un point situe a l'exterieur du polygone
	@param polygon Le polygone dans lequel on veut rapatrier le point
	@return la position du point, une fois ramene dans le polygone, ou plus
	exactement sur le bord du polygone
*/
QPointF Conductor::movePointIntoPolygon(const QPointF &point, const QPainterPath &polygon) {
	// decompose le polygone en lignes et points
	QList<QPolygonF> polygons = polygon.simplified().toSubpathPolygons();
	QList<QLineF> lines;
	QList<QPointF> points;
	foreach(QPolygonF polygon, polygons) {
		if (polygon.count() <= 1) continue;
		
		// on recense les lignes et les points
		for (int i = 1 ; i < polygon.count() ; ++ i) {
			lines << QLineF(polygon.at(i - 1), polygon.at(i));
			points << polygon.at(i -1);
		}
	}
	
	// on fait des projetes orthogonaux du point sur les differents segments du
	// polygone, en les triant par longueur croissante
	QMap<qreal, QPointF> intersections;
	foreach (QLineF line, lines) {
		QPointF intersection_point;
		if (QET::orthogonalProjection(point, line, &intersection_point)) {
			intersections.insert(QLineF(intersection_point, point).length(), intersection_point);
		}
	}
	if (intersections.count()) {
		// on determine la plus courte longueur pour un projete orthogonal
		QPointF the_point = intersections[intersections.keys().first()];
		return(the_point);
	} else {
			// determine le coin du polygone le plus proche du point exterieur
			qreal minimum_length = -1;
			int point_index = -1;
			for (int i = 0 ; i < points.count() ; ++ i) {
				qreal length = qAbs(QLineF(points.at(i), point).length());
				if (minimum_length < 0 || length < minimum_length) {
					minimum_length = length;
					point_index    = i;
				}
			}
			// on connait desormais le coin le plus proche du texte
		
		// aucun projete orthogonal n'a donne quoi que ce soit, on met le texte sur un des coins du polygone
		return(points.at(point_index));
	}
}

/**
 * @brief longuestConductorInPotential
 * @param conductor : a conductor in the potential to search
 * @param all_diagram : true -> search in the whole project, false -> search only in the diagram of conductor
 * @return the longuest conductor in the same potential of conductor
 */
Conductor * longuestConductorInPotential(Conductor *conductor, bool all_diagram) {
	Conductor *longuest_conductor = conductor;
	//Search the longuest conductor
	foreach (Conductor *c, conductor -> relatedPotentialConductors(all_diagram))
		if (c -> length() > longuest_conductor -> length())
			longuest_conductor = c;

	return longuest_conductor;
}

/**
 * @brief relatedConductors
 * @param conductor
 * @return return all conductors who share the same terminals of @conductor given as parametre,
 * except @conductor himself.
 */
QList <Conductor *> relatedConductors(const Conductor *conductor) {
	QList<Conductor *> other_conductors_list = conductor -> terminal1 -> conductors();
	other_conductors_list << conductor -> terminal2->conductors();
	other_conductors_list.removeAll(const_cast<Conductor *> (conductor));
	return(other_conductors_list);
}

/**
 * @brief Conductor::freezeLabel
 * Freeze this conductor label
 */
void Conductor::freezeLabel() {
	QString freezelabel = this->text_item->toPlainText();
	m_frozen_label = properties_.text;
	this->setText(freezelabel);
	this->properties_.text = freezelabel;
}

/**
 * @brief Conductor::unfreezeLabel
 * Unfreeze this conductor label
 */
void Conductor::unfreezeLabel() {
	this->setText(m_frozen_label);
	if (m_frozen_label == "") return;
	properties_.text = m_frozen_label;
}
