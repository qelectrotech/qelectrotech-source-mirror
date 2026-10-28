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
#include "bridgepart.h"

#include "../diagram.h"
#include "../qetproject.h"
#include "bridgecreatedialog.h"
#include "bridgemanager.h"
#include "bridgeundocommands.h"

#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QUndoStack>

namespace {
		///How close to one of its ends a click has to land to take hold
		///of it: generous enough for the hand, tight enough that the
		///middle of the line stays free for selecting
	constexpr qreal grip_radius = 6.0;

		///Every bridge line drawn anywhere in the application
	QList<BridgePart *> bridge_parts;
}

/**
	@return every bridge line drawn anywhere in the application
*/
QList<BridgePart *> BridgePart::allParts() {
	return bridge_parts;
}

/**
	@brief BridgePart::BridgePart
	@param data the section of the bridge this line stands for
	@param parent
*/
BridgePart::BridgePart(const BridgePartData &data, QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_part_uuid(data.uuid),
	m_p1(data.p1),
	m_p2(data.p2)
{
	setMovable(false); //the shape is the user's own, changing it comes later
	setZValue(10); //over the conductors, like any line drawn above a wire
	setFlag(QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
	bridge_parts << this;
}

/**
	@brief BridgePart::~BridgePart
*/
BridgePart::~BridgePart()
{
	bridge_parts.removeAll(this);
}

/**
	@param bridge the bridge this line is drawn for
*/
void BridgePart::setBridge(Bridge *bridge)
{
	m_bridge = bridge;
}

/**
	@brief BridgePart::setRefused
	@param refused
*/
void BridgePart::setRefused(bool refused)
{
	if (m_refused == refused) {
		return;
	}
	m_refused = refused;
	update();
}

/**
	@return true while this line is drawn in the colour of a problem
*/
bool BridgePart::isRefused() const {
	return m_refused;
}

/**
	@return the bridge this line is drawn for, or nullptr
*/
Bridge *BridgePart::bridge() const {
	return m_bridge;
}

/**
	@return which section of its bridge this line is
*/
QUuid BridgePart::partUuid() const {
	return m_part_uuid;
}

/**
	@param p1
	@param p2
*/
void BridgePart::setLine(const QPointF &p1, const QPointF &p2)
{
	prepareGeometryChange();
	m_p1 = p1;
	m_p2 = p2;
	update();
}

/**
	@return the line this section is drawn as
*/
QLineF BridgePart::line() const {
	return QLineF(m_p1, m_p2);
}

/**
	@return the area the line is drawn in, with a margin for the pen
	and the end grips
*/
QRectF BridgePart::boundingRect() const
{
	const QRectF rect(m_p1, m_p2);
	const qreal margin = 4 + lineWidth();
	return rect.normalized().adjusted(-margin, -margin, margin, margin);
}

/**
	@return the area which answers the mouse: the line itself, thick
	enough to be clicked without aiming
*/
QPainterPath BridgePart::shape() const
{
	QPainterPath path(m_p1);
	path.lineTo(m_p2);
	QPainterPathStroker stroker;
	stroker.setWidth(qMax(8.0, lineWidth() + 6.0));
	return stroker.createStroke(path);
}

/**
	@brief BridgePart::paint
	@param painter
	@param option
	@param widget
*/
void BridgePart::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

		//As thick as the bridge says, in the same way a conductor takes
		//its size: the line is drawn the way the real bar is thick. The
		//colour of a problem while the bridge refuses a terminal: what
		//stops the hand is seen where the hand is.
	QPen pen(m_refused ? QColor(200, 0, 0) : drawColor(), lineWidth(), Qt::SolidLine, Qt::RoundCap);
	painter->setPen(pen);
	painter->drawLine(m_p1, m_p2);

		//The two ends of the line when it is chosen, so the hand has
		//something to hold on to (stretching the bridge comes later)
	if (isSelected()) {
		QPen grip_pen(QColor(28, 134, 238), 1);
		grip_pen.setCosmetic(true);
		painter->setPen(grip_pen);
		painter->setBrush(Qt::white);
		painter->drawEllipse(m_p1, 3, 3);
		painter->drawEllipse(m_p2, 3, 3);
	}
}

/**
	@return the name of this kind of item, for the undo texts
*/
QString BridgePart::name() const {
	return tr("bridge");
}

/**
	@brief BridgePart::mousePressEvent
	Taking hold of one of the ends starts stretching the line: the end
	follows the mouse and, with it, which terminals the bridge lands on.
	@param event
*/
void BridgePart::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	const QPointF pos = mapToScene(event->pos());
	if (QLineF(pos, m_p1).length() <= grip_radius) {
		m_stretch_end = 1;
	} else if (QLineF(pos, m_p2).length() <= grip_radius) {
		m_stretch_end = 2;
	}

	if (m_stretch_end) {
		m_stretched_from = line();
		setSelected(true);
		event->accept();
		return;
	}
	QetGraphicsItem::mousePressEvent(event);
}

/**
	@brief BridgePart::mouseMoveEvent
	@param event
*/
void BridgePart::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_stretch_end) {
		QetGraphicsItem::mouseMoveEvent(event);
		return;
	}

	const QPointF end = Diagram::snapToGrid(event->scenePos());
	if (m_stretch_end == 1) {
		setLine(end, m_p2);
	} else {
		setLine(m_p1, end);
	}
	event->accept();
}

/**
	@brief BridgePart::mouseReleaseEvent
	Letting go of the end leaves the line where it is and gives the
	bridge the terminals it crosses now -- and takes back the ones it no
	longer runs over. A bridge which cannot hold what the line would
	take keeps its old length: that one cannot be built, so it is not
	drawn.
	@param event
*/
void BridgePart::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_stretch_end) {
		QetGraphicsItem::mouseReleaseEvent(event);
		return;
	}
	m_stretch_end = 0;

	Bridge *bridge = m_bridge;
	Diagram *folio = diagram();
	QETProject *project = folio ? folio->project() : nullptr;
	if (!bridge || !project) {
		return;
	}

	const QLineF before_line = m_stretched_from;
	const QLineF after_line = line();
	const QList<BridgeTerminal> before = bridge->terminals();
	const QList<BridgeTerminal> after = BridgeManager::terminalsOfBridge(project, bridge);

	if (before_line == after_line && before == after) {
		return;
	}

		//What is one bridge carried on over the sheets is one bridge:
		//an end coming to a sheet reference joins the bridge that
		//reference stands for -- one unit, one voltage, one limit -- the
		//way a line drawn onto an existing bridge joins it
	Bridge *joined = BridgeManager::joinTarget(project, after, bridge);
	Bridge *unit = joined ? joined : bridge;

		//A sheet reference is the way to the next sheet and fills no
		//place of the bridge: what is counted are the Klemmen the line
		//lands on and did not land on before
	int added = 0;
	for (const BridgeTerminal &terminal : after) {
		if (!terminal.reference && !unit->hasTerminal(terminal)) {
			++added;
		}
	}
	if (added && unit->wouldExceed(added)) {
		setLine(before_line.p1(), before_line.p2());
		QMessageBox::warning(nullptr, tr("That bridge cannot be built"),
							 tr("The line would land on %1 terminal(s) more than the "
								"bridge may hold: it keeps the length it had.")
							 .arg(added));
		event->accept();
		return;
	}

		//What the bridge carries as information follows the terminals:
		//when they do not agree the user is asked which the bridge takes
		//-- its own among the candidates -- and cancelling leaves the
		//line the length it had. The unit joined carries its own voltage
		//to the new half of it.
	const QString unit_tension = joined
			? (joined->tensionText().isEmpty()
			   ? bridge->tensionText() : joined->tensionText())
			: bridge->tensionText();
	const BridgeInformation information_before{joined ? unit_tension
													  : bridge->tensionText()};
	QList<BridgeInformation> candidates = BridgeManager::informationSets(project, after);
	candidates.prepend(information_before);
	bool take_information = false;
	const BridgeInformation information_after =
			BridgeCreateDialog::askForInformation(candidates, &take_information);
	if (!take_information) {
		setLine(before_line.p1(), before_line.p2());
		event->accept();
		return;
	}

	if (joined)
	{
			//One undo step for the two of them: first the bridges become
			//one, then the line is the length it was pulled to on the
			//unit which now holds it
		auto *outer = new QUndoCommand();
		outer->setText(tr("Joining a bridge over a sheet reference", "undo command text"));
		new MergeBridgesCommand(bridge, joined, outer);

		QList<BridgeTerminal> merged_before = joined->terminals();
		for (const BridgeTerminal &terminal : before) {
			if (!merged_before.contains(terminal)) {
				merged_before << terminal;
			}
		}
		QList<BridgeTerminal> merged_after = joined->terminals();
		for (const BridgeTerminal &terminal : after) {
			if (!merged_after.contains(terminal)) {
				merged_after << terminal;
			}
		}
		const BridgeInformation carried{joined->tensionText().isEmpty()
					? information_after.tension : joined->tensionText()};
		new ChangeBridgeGeometryCommand(joined, this,
										before_line, after_line,
										merged_before, merged_after,
										carried, carried, outer);
		BridgeManager::pairReferences(project, joined, outer);
		BridgeManager::writeInformation(project, merged_after, carried, outer);
		folio->undoStack().push(outer);
		event->accept();
		return;
	}

	auto *command = new ChangeBridgeGeometryCommand(bridge, this,
													before_line, after_line,
													before, after,
													information_before, information_after);
		//A reference which has come to this bridge finds the one on the
		//other sheet: they link with the change
	BridgeManager::pairReferences(project, bridge, command);
		//Bridge and wires say the same thing: what the bridge carries
		//goes onto the wires it sits on, in the very same undo step
	BridgeManager::writeInformation(project, after, information_after, command);
	folio->undoStack().push(command);
	event->accept();
}

/**
	@param event
*/
void BridgePart::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event)
	editProperty();
}

/**
	Opens the dialog editing the properties of the whole bridge: its
	colour and how many terminals it may hold. Both are the bridge's own
	-- every line of it is drawn in that colour -- and what is changed
	here is one undo step.
*/
void BridgePart::editProperty()
{
	Bridge *bridge = m_bridge;
	Diagram *folio = diagram();
	if (!bridge || !folio) {
		return;
	}

	BridgeCreateDialog dialog(bridge,
							  folio->views().isEmpty() ? nullptr : folio->views().constFirst());
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}

	const QColor color = dialog.newColor();
	const int max_connections = dialog.newMaxConnections();
	const qreal line_width = dialog.newLineWidth();
	const QString name = dialog.bridgeName();
	if (color == bridge->color()
		&& max_connections == bridge->maxConnections()
		&& qFuzzyCompare(line_width, bridge->lineWidth())
		&& name == bridge->name()) {
		return;
	}
	folio->undoStack().push(
		new ChangeBridgePropertiesCommand(bridge, color, max_connections, line_width, name));
}

/**
	@return the colour this line is drawn in: its bridge's colour,
	black when it has no bridge (yet)
*/
QColor BridgePart::drawColor() const {
	return m_bridge ? m_bridge->color() : QColor(Qt::black);
}

/**
	@return how thick this line is drawn: as thick as its bridge says,
	as thick as a conductor is by default when it has no bridge (yet)
*/
qreal BridgePart::lineWidth() const {
	return m_bridge ? m_bridge->lineWidth() : Bridge::default_line_width;
}
