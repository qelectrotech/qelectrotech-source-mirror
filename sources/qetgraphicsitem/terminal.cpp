/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "../qetgraphicsitem/terminal.h"

#include "../conductorautonumerotation.h"
#include "../diagram.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../properties/terminaldata.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "conductortextitem.h"

#include <utility>

QColor Terminal::neutralColor      = QColor(Qt::blue);
QColor Terminal::allowedColor      = QColor(Qt::darkGreen);
QColor Terminal::warningColor      = QColor("#ff8000");
QColor Terminal::forbiddenColor    = QColor(Qt::red);
const qreal Terminal::terminalSize = 4.0;
const qreal Terminal::Z = 1000;

/**
	@brief Terminal::init
	Methode privee pour initialiser la borne.
	Private method to initialize the terminal.
	@param number of terminal
	@param name of terminal
	@param hiddenName
*/
void Terminal::init()
{
		//Calculate the docking point of the element
		//m_pos of d is the docking point of conductor
	dock_elmt_ = d->m_pos;
	switch(d->m_orientation) {
		case Qet::North: dock_elmt_ += QPointF(0, Terminal::terminalSize);  break;
		case Qet::East : dock_elmt_ += QPointF(-Terminal::terminalSize, 0); break;
		case Qet::West : dock_elmt_ += QPointF(Terminal::terminalSize, 0);  break;
		case Qet::South: dock_elmt_ += QPointF(0, -Terminal::terminalSize); break;
	}

		//Calculate the bounding rect
	qreal dcx = d->m_pos.x();
	qreal dcy = d->m_pos.y();
	qreal dex = dock_elmt_.x();
	qreal dey = dock_elmt_.y();
	QPointF origin = (dcx <= dex && dcy <= dey ? d->m_pos : dock_elmt_);
	origin += QPointF(-3.0, -3.0);
	qreal w = qAbs(dcx - dex) + 7;
	qreal h = qAbs(dcy - dey) + 7;
	m_br = QRectF(origin, QSizeF(w, h));

	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton);
	setToolTip(QObject::tr("Borne", "tooltip"));
	setZValue(Z);
}

Terminal::Terminal(TerminalData* data, Element* e) :
	QGraphicsObject(e),
	d(data),
	parent_element_(e)
{
	d->setParent(this);
	init();
}

/**
 * @brief Terminal::~Terminal
 * Destruction of the terminal, and also docked conductor
 */
Terminal::~Terminal() {
	qDeleteAll(m_conductors_list);
}

/**
	Permet de connaitre l'orientation de la borne. Si le parent de la borne
	est bien un Element, cette fonction renvoie l'orientation par rapport a
	la scene de la borne, en tenant compte du fait que l'element ait pu etre
	pivote. Sinon elle renvoie son sens normal.
	Used to find out the orientation of the terminal. If the terminal's parent
	is in fact an Element, this function returns the orientation of the
	terminal with respect to the scene, taking into account the angle of
	rotation. scene, taking into account the fact that the element may have
	been rotated. Otherwise it returns its normal direction.
	@return L'orientation actuelle de la Terminal.
*/
Qet::Orientation Terminal::orientation() const
{
	if (Element *elt = qgraphicsitem_cast<Element *>(parentItem())) {
		// orientations actuelle et par defaut de l'element
		// current and default element orientations
		int ori_cur = elt -> orientation();
	if (ori_cur == 0) return(d->m_orientation);
		else {
			// calcul l'angle de rotation implique par l'orientation de l'element parent
			// angle de rotation de la borne sur la scene, divise par 90
			// calculates the angle of rotation implied by the orientation of the parent
			// element angle of rotation of the terminal on the scene, divided by 90
			int angle = ori_cur + d->m_orientation;
			while (angle >= 4) angle -= 4;
			return((Qet::Orientation)angle);
		}
	} else return(d->m_orientation);
}

/**
	@brief Terminal::addConductor
	Add a conductor to this terminal
	@param conductor : the conductor to add.
	@return true if the conductor was successfully added
*/
bool Terminal::addConductor(Conductor *conductor)
{
	if (!conductor) return(false);

	Q_ASSERT_X(((conductor -> terminal1 == this) ^ (conductor -> terminal2 == this)),
		   "Terminal::addConductor",
		   "The conductor must be linked exactly once to this terminal");

	//Get the other terminal where the conductor must be linked
	Terminal *other_terminal = (conductor -> terminal1 == this)
			? conductor->terminal2 : conductor->terminal1;

	//Check if this terminal isn't already linked with other_terminal
	foreach (Conductor* cond, m_conductors_list)
		if (cond -> terminal1 == other_terminal || cond -> terminal2 == other_terminal)
			return false; //They already a conductor linked to this and other_terminal

	m_conductors_list.append(conductor);
	emit conductorWasAdded(conductor);
	return(true);
}

/**
	@brief Terminal::removeConductor
	Remove a conductor from this terminal
	@param conductor : conductor to remove
*/
void Terminal::removeConductor(Conductor *conductor)
{
	int index = m_conductors_list.indexOf(conductor);
	if (index == -1) return;
	m_conductors_list.removeAt(index);
	emit conductorWasRemoved(conductor);
}

/**
	@brief Terminal::paint
	Fonction de dessin des bornes
	@param painter Le QPainter a utiliser
	@param options Les options de dessin
*/
void Terminal::paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *options,
		QWidget *)
{
	// en dessous d'un certain zoom, les bornes ne sont plus dessinees
	// below a certain zoom level, the terminals are no longer drawn
	if (options && options->levelOfDetailFromTransform(painter->worldTransform()) < 0.5)
		return;
	painter -> save();

	// annulation des renderhints
	// cancel renderhints
	painter -> setRenderHint(QPainter::Antialiasing,          false);
	painter -> setRenderHint(QPainter::TextAntialiasing,      false);
	painter -> setRenderHint(QPainter::SmoothPixmapTransform, false);

	// on travaille avec les coordonnees de l'element parent
	// work with the coordinates of the parent element
	QPointF c = mapFromParent(d->m_pos);
	QPointF e = mapFromParent(dock_elmt_);

	QPen t;
	t.setWidthF(1.0);

	if (options && options->levelOfDetailFromTransform(painter->worldTransform()) < 1.0)
	{
		t.setCosmetic(true);
	}

	// dessin de la borne en rouge
	// draw the terminal in red
	t.setColor(Qt::red);
	painter -> setPen(t);
	painter -> drawLine(c, e);

	// dessin du point d'amarrage au conducteur en bleu
	// draw the docking point to the conductor in blue
	t.setColor(m_hovered_color);
	painter -> setPen(t);
	painter -> setBrush(m_hovered_color);
	if (m_hovered) {
		painter -> setRenderHint(QPainter::Antialiasing, true);
		painter -> drawEllipse(QRectF(c.x() - 2.5, c.y() - 2.5, 5.0, 5.0));
	} else painter -> drawPoint(c);

	//Draw help line if needed,
	if (diagram() && m_draw_help_line)
	{
		//Draw the help line with same orientation of terminal
		//Only if there isn't docked conductor
		if (conductors().isEmpty())
		{
			if (!m_help_line)
				m_help_line = new QGraphicsLineItem(this);
			QPen pen;
			pen.setColor(Qt::darkBlue);

			QLineF line(HelpLine());

			if (diagram() -> project() -> autoConductor())
			{
				Terminal *t = alignedWithTerminal();
				if (t)
				{
					line.setP2(t -> dockConductor());
					pen.setColor(Qt::darkGreen);
				}
			}

			//Map the line (in scene coordinate) to m_help_line coordinate
			line.setP1(m_help_line -> mapFromScene(line.p1()));
			line.setP2(m_help_line -> mapFromScene(line.p2()));
			m_help_line -> setPen(pen);
			m_help_line -> setLine(line);
		}

		//Draw the help line perpendicular to the terminal
		if (!m_help_line_a)
		{
			m_help_line_a = new QGraphicsLineItem(this);
			QPen pen;
			pen.setColor(Diagram::background_color == Qt::darkGray ? Qt::lightGray : Qt::darkGray);
			m_help_line_a -> setPen(pen);
		}

		QRectF rect = diagram() -> border_and_titleblock.insideBorderRect();
		QLineF line;

		if (Qet::isHorizontal(orientation()))
		{
			line.setP1(QPointF(dockConductor().x(), rect.topLeft().y()));
			line.setP2(QPointF(dockConductor().x(), rect.bottomLeft().y()));
		}
		else
		{
			line.setP1(QPointF(rect.topLeft().x(), dockConductor().y()));
			line.setP2(QPointF(rect.topRight().x(), dockConductor().y()));
		}

			//Map the line (in scene coordinate) to m_help_line_a coordinate
		line.setP1(m_help_line_a -> mapFromScene(line.p1()));
		line.setP2(m_help_line_a -> mapFromScene(line.p2()));
		m_help_line_a -> setLine(line);
	}

	painter -> restore();
}

/**
	@brief Terminal::drawHelpLine
	@param draw : true, display the help line
	false, hide it.
*/
void Terminal::drawHelpLine(bool draw)
{
	if (m_draw_help_line == draw) return;

	m_draw_help_line = draw;

	if (!draw)
	{
		if (m_help_line)
		{
			delete m_help_line;
			m_help_line = nullptr;
		}
		if (m_help_line_a)
		{
			delete m_help_line_a;
			m_help_line_a = nullptr;
		}
	}
}

/**
	@brief Terminal::HelpLine
	@return a line with coordinate P1 the dock point of conductor
	and P2 the border of diagram, according to the orientation of terminal
	The line is in scene coordinate;
*/
QLineF Terminal::HelpLine() const
{
	QPointF scene_dock = dockConductor();
	QRectF  rect       = diagram() -> border_and_titleblock.insideBorderRect();

	QLineF line(scene_dock , QPointF());

		//Set the second point of line to the edge of diagram,
		//according with the orientation of this terminal
	switch (orientation())
	{
		case Qet::North:
			line.setP2(QPointF(scene_dock.x(), rect.top()));
			break;
		case Qet::East:
			line.setP2(QPointF(rect.right() , scene_dock.y()));
			break;
		case Qet::South:
			line.setP2(QPointF(scene_dock.x(), rect.bottom()));
			break;
		case Qet::West:
			line.setP2(QPointF(rect.left(), scene_dock.y()));
			break;
	}

	return line;
}

/**
	@brief Terminal::boundingRect
	@return Le rectangle (en precision flottante) delimitant la borne et ses alentours.
*/
QRectF Terminal::boundingRect() const {
	return m_br;
}

/**
	@brief Terminal::alignedWithTerminal
	If this terminal is aligned with an other terminal
	and is orientation is opposed return the other terminal
	else return nullptr
	@return
*/
Terminal* Terminal::alignedWithTerminal() const
{
	QLineF line(HelpLine());

	QPainterPath path;
	path.moveTo(line.p1());
	path.lineTo(line.p2());

	//Get all QGraphicsItem in the alignement of this terminal
	QList<QGraphicsItem *> qgi_list = diagram() -> items(path);

	//Remove all terminals of the parent element
	foreach (Terminal *t, parent_element_ -> terminals())
		qgi_list.removeAll(t);

	if (qgi_list.isEmpty()) return nullptr;

	//Get terminals only if orientation is opposed with this terminal
	QList <Terminal *>  available_terminals;
	foreach (QGraphicsItem *qgi, qgi_list)
	{
		if (Terminal *tt = qgraphicsitem_cast <Terminal *> (qgi))
		{
			//Call QET::lineContainsPoint to be sure the line intersect
			//the dock point and not an other part of terminal
			if (Qet::isOpposed(orientation(), tt -> orientation()) &&
				QET::lineContainsPoint(line, tt -> dockConductor()))
			{
				available_terminals << tt;
			}
		}
	}

	if (available_terminals.isEmpty())   return nullptr;
	if (available_terminals.size() == 1) return (available_terminals.first());

	//Available_terminals have several terminals, we get the nearest terminal
	line.setP2(available_terminals.first() -> dockConductor());
	qreal     current_lenght   = line.length();
	Terminal *nearest_terminal = available_terminals.takeFirst();

	//Search the nearest terminal to this one
	foreach (Terminal *terminal, available_terminals)
	{
		line.setP2(terminal -> dockConductor());
		if (line.length() < current_lenght)
		{
			current_lenght   = line.length();
			nearest_terminal = terminal;
		}
	}

	return nearest_terminal;
}

/**
	@brief Terminal::hoverEnterEvent
	Gere l'entree de la souris sur la zone de la Borne.
*/
void Terminal::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
	m_hovered = true;
	update();
}

/**
	@brief Terminal::hoverMoveEvent
	Gere les mouvements de la souris sur la zone de la Borne.
*/
void Terminal::hoverMoveEvent(QGraphicsSceneHoverEvent *) {}

/**
	@brief Terminal::hoverLeaveEvent
	Gere le fait que la souris sorte de la zone de la Borne.
*/
void Terminal::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
	m_hovered = false;
	update();
}

/**
	@brief Terminal::mousePressEvent
	Gere le fait qu'on enfonce un bouton de la souris sur la Borne.
	@param e L'evenement souris correspondant
*/
void Terminal::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
	if (Diagram *diag = diagram()) {
		diag -> setConductorStart(mapToScene(QPointF(d->m_pos)));
		diag -> setConductorStop(e -> scenePos());
		diag -> setConductor(true);
		//setCursor(Qt::CrossCursor);
	}
}

/**
	@brief Terminal::mouseMoveEvent
	Gere le fait qu'on bouge la souris sur la Borne.
	@param e L'evenement souris correspondant
*/
void Terminal::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
	// pendant la pose d'un conducteur, on adopte un autre curseur
	//setCursor(Qt::CrossCursor);

	// d'un mouvement a l'autre, il faut retirer l'effet hover de la borne precedente
	if (m_previous_terminal) {
		if (m_previous_terminal == this) m_hovered = true;
		else m_previous_terminal -> m_hovered = false;
		m_previous_terminal -> m_hovered_color = m_previous_terminal -> neutralColor;
		m_previous_terminal -> update();
	}

	Diagram *diag = diagram();
	if (!diag) return;
	// si la scene est un Diagram, on actualise le poseur de conducteur
	diag -> setConductorStop(e -> scenePos());

	// on recupere la liste des qgi sous le pointeur
	QList<QGraphicsItem *> qgis = diag -> items(e -> scenePos());

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
	m_previous_terminal = other_terminal;

	// s'il s'agit d'une borne, on lui applique l'effet hover approprie
	if (!canBeLinkedTo(other_terminal)) {
		other_terminal -> m_hovered_color = forbiddenColor;
	} else if (other_terminal -> conductorsCount()) {
		other_terminal -> m_hovered_color = warningColor;
	} else {
		other_terminal -> m_hovered_color = allowedColor;
	}

	other_terminal -> m_hovered = true;
	other_terminal -> update();
}


/**
	@brief Terminal::mouseReleaseEvent
	@param e
*/
void Terminal::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
	m_previous_terminal = nullptr;
	m_hovered_color     = neutralColor;

	if (!diagram()) return;

	//Stop conductor preview
	diagram() -> setConductor(false);

	//Get item under cursor
	QGraphicsItem *qgi = diagram() -> itemAt(e -> scenePos(), QTransform());
	if (!qgi) return;

	//Element must be a terminal
	Terminal *other_terminal = qgraphicsitem_cast<Terminal *>(qgi);
	if (!other_terminal) return;

	other_terminal -> m_hovered_color = neutralColor;
	other_terminal -> m_hovered       = false;

	//We stop her if we can't link this terminal with other terminal
	if (!canBeLinkedTo(other_terminal)) return;

	//Create conductor
	Conductor *new_conductor = new Conductor(this, other_terminal);

	//Get all conductors at the same potential of new conductors
	QSet <Conductor *> conductors_list = new_conductor->relatedPotentialConductors();

	//Compare the properties of every conductors stored in conductors_list,
	//if every conductors properties is equal, we use this properties for the new conductor.
	ConductorProperties others_properties;
	bool use_properties = false;
	if (!conductors_list.isEmpty())
	{
		use_properties = true;
		others_properties = (*conductors_list.begin())->properties();
		foreach (Conductor *conductor, conductors_list) {
			if (conductor->properties() != others_properties)
				use_properties = false;
		}
	}


	QUndoCommand *undo = new QUndoCommand();
	QUndoCommand *aic = new AddGraphicsObjectCommand(new_conductor, diagram(), QPointF(), undo);
	undo->setText(aic->text());

	if (use_properties)
	{
		Conductor *other = conductors_list.values().first();
		new_conductor->rSequenceNum() = other->sequenceNum();
		new_conductor->setProperties(others_properties);
	}
	else
	{
		//Autonum it
		ConductorAutoNumerotation can (new_conductor, diagram(), undo);
		can.numerate();
	}
	//Add undo command to the parent diagram
	diagram() -> undoStack().push(undo);
	if (use_properties)
	{
		Conductor *other = conductors_list.values().first();
		new_conductor->setProperties(other->properties());
	}
}

/**
	@brief Terminal::updateConductor
	Update the path of conductor docked to this terminal
*/
void Terminal::updateConductor()
{
	foreach (Conductor *conductor, m_conductors_list)
		conductor->updatePath();
}

/**
	@brief Terminal::isLinkedTo
	@param other_terminal Autre borne
	@return true si cette borne est reliee a other_terminal, false sion
*/
bool Terminal::isLinkedTo(Terminal *other_terminal) {
	if (other_terminal == this) return(false);

	bool already_linked = false;
	foreach (Conductor *c, m_conductors_list) {
		if (c -> terminal1 == other_terminal || c -> terminal2 == other_terminal) {
			already_linked = true;
			break;
		}
	}
	return(already_linked);
}

/**
	@brief Terminal::canBeLinkedTo
	Checking if the terminal can be linked to \p other_terminal or not
	Reasons for not linable:
	 - \p other_terminal is this terminal
	 - this terminal is already connected to \p other_terminal
	@param other_terminal
	@return true if this terminal can be linked to other_terminal,
	otherwise false
*/
bool Terminal::canBeLinkedTo(Terminal *other_terminal)
{
	if (other_terminal == this || isLinkedTo(other_terminal))
		return false;

	return true;
}

/**
	@brief Terminal::conductors
	@return La liste des conducteurs lies a cette borne
*/
QList<Conductor *> Terminal::conductors() const
{
	return(m_conductors_list);
}

/**
	@brief Terminal::toXml
	Methode d'export en XML
	@param doc Le Document XML a utiliser pour creer l'element XML
	@return un QDomElement representant cette borne
*/
QDomElement Terminal::toXml(QDomDocument &doc) const
{
	QDomElement qdo = doc.createElement("terminal");

	// for backward compatibility
	qdo.setAttribute("x", QString("%1").arg(dock_elmt_.x()));
	qdo.setAttribute("y", QString("%1").arg(dock_elmt_.y()));
	// end for backward compatibility

	qdo.setAttribute("orientation", d->m_orientation);
	return(qdo);
}

/**
	@brief Terminal::valideXml
	Permet de savoir si un element XML represente une borne
	@param terminal Le QDomElement a analyser
	@return true si le QDomElement passe en parametre est une borne, false sinon
*/
bool Terminal::valideXml(QDomElement &terminal)
{
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
	if (terminal_or != Qet::North
			&& terminal_or != Qet::South
			&& terminal_or != Qet::East
			&& terminal_or != Qet::West) return(false);

	// a ce stade, la borne est syntaxiquement correcte
	return(true);
}

/**
	@brief Terminal::fromXml
	Enables you to find out whether an XML element represents this terminal.
	Warning, the XML element is not checked
	Permet de savoir si un element XML represente cette borne. Attention,
	l'element XML n'est pas verifie
	@param terminal Le QDomElement a analyser / QDomElement to check
	@return true si la borne "se reconnait"
	(memes coordonnes, meme orientation), false sinon
	true, if the terminal ‘recognises’ itself (same coordinates,
	same orientation), false otherwise
*/
bool Terminal::fromXml(QDomElement &terminal)
{
	return (
		qFuzzyCompare(terminal.attribute("x").toDouble(), dock_elmt_.x()) &&
		qFuzzyCompare(terminal.attribute("y").toDouble(), dock_elmt_.y()) &&
		(terminal.attribute("orientation").toInt() == d->m_orientation)
	);
}

/**
	@brief Terminal::dockConductor
	@return the position, relative to the scene, of the docking point for
	conductors.
*/
QPointF Terminal::dockConductor() const
{
	return(mapToScene(d->m_pos));
}

/**
	@brief Terminal::diagram
	@return le Diagram auquel cette borne appartient,
	ou 0 si cette borne est independant
*/
Diagram *Terminal::diagram() const
{
	return(qobject_cast<Diagram *>(scene()));
}

/**
	@brief Terminal::parentElement
	@return L'element auquel cette borne est rattachee
*/
Element *Terminal::parentElement() const
{
	return(parent_element_);
}

QUuid Terminal::uuid() const
{
	return d->m_uuid;
}

QString Terminal::name() const
{
	return d->m_name;
}

/**
	@brief Conductor::relatedPotentialTerminal
	Return terminal at the same potential from the same
	parent element of terminal.
	For folio report, return the terminal of linked other report.
	For Terminal element, return the other terminal of terminal element.
	@param terminal : to start search
	@param all_diagram :if true return all related terminal,
	false return only terminal in the same diagram of t
	@return the list of terminal at the same potential
*/
QList<Terminal *> relatedPotentialTerminal (
		const Terminal *terminal, const bool all_diagram)
{
	// If terminal parent element is a folio report.
	if (all_diagram && terminal -> parentElement() -> linkType() & Element::AllReport)
	{
		QList <Element *> elmt_list = terminal -> parentElement() -> linkedElements();
		if (!elmt_list.isEmpty())
		{
			return (elmt_list.first()->terminals());
		}
	}
	// If terminal parent element is a Terminal element.
	else if (terminal -> parentElement() -> linkType() & Element::Terminale)
	{
		QList <Terminal *> terminals = terminal->parentElement()->terminals();
		terminals.removeAll(const_cast<Terminal *>(terminal));
		return terminals;
	}

	return QList<Terminal *>();
}

