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
#include "diagrameventaddbridge.h"

#include <bridge/addbridgecommand.h>
#include <bridge/bridge.h>
#include <bridge/bridgecreatedialog.h>
#include <bridge/bridgepart.h>
#include <diagram.h>
#include <qetapp.h>
#include <qetdiagrameditor.h>
#include <qetproject.h>

#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QLineF>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
#include <QUndoStack>

namespace {

	///A click closer than this to the start point is not an end point
	const qreal min_end_length = 2.0;

} // namespace

/**
	@brief DiagramEventAddBridge::DiagramEventAddBridge
	@param diagram the folio the bridge is drawn on
*/
DiagramEventAddBridge::DiagramEventAddBridge(Diagram *diagram) :
	DiagramEventInterface(diagram)
{
	m_running = true;
		//Done here as well as in init(): Diagram::setEventInterface()
		//only calls init() when it replaces another tool, so a fresh
		//tool would otherwise never be set up at all.
	init();
		//The message itself waits for the next turn of the event loop:
		//setEventInterface() destroys the tool which was running before
		//this constructor returns, and its destructor clears the status
		//bar, which would wipe out anything shown before it is done.
	QTimer::singleShot(0, this, [this]() { updateHint(); });
}

/**
	@brief DiagramEventAddBridge::~DiagramEventAddBridge
*/
DiagramEventAddBridge::~DiagramEventAddBridge()
{
	delete m_preview;
	delete m_counter;
	delete m_help_horiz;
	delete m_help_verti;

	if (m_diagram)
	{
		for (QGraphicsView *view : m_diagram->views()) {
			view->setContextMenuPolicy(Qt::DefaultContextMenu);
		}
	}
	clearHint();
}

/**
	@brief DiagramEventAddBridge::init
	While the tool runs, the right button belongs to it alone: it cancels
	the line being drawn or ends the tool.
*/
void DiagramEventAddBridge::init()
{
	m_running = true;
	if (m_diagram)
	{
		for (QGraphicsView *view : m_diagram->views()) {
			view->setContextMenuPolicy(Qt::NoContextMenu);
		}
	}
	updateHint();
}

/**
	@brief DiagramEventAddBridge::reset
	Put the tool back to where it was before the first click: no line
	kept, no counter, nothing remembered.
*/
void DiagramEventAddBridge::reset()
{
	if (m_diagram) {
		if (m_preview) m_diagram->removeItem(m_preview);
		if (m_counter) m_diagram->removeItem(m_counter);
	}
	delete m_preview;
	m_preview = nullptr;
	delete m_counter;
	m_counter = nullptr;
	m_has_anchor = false;
	m_anchor = QPointF();
	m_crossings.clear();
	updateHint();
}

/**
	@brief DiagramEventAddBridge::clearHint
*/
void DiagramEventAddBridge::clearHint() const
{
	if (!m_diagram || m_diagram->views().isEmpty()) return;
	if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst())) {
		editor->statusBar()->clearMessage();
	}
}

/**
	@brief DiagramEventAddBridge::updateHint
	Say what the next click does, and -- once the line has been pulled
	out -- how many terminals it runs across, so the user sees what the
	bridge is going to hold before the window opens.
*/
void DiagramEventAddBridge::updateHint() const
{
	if (!m_diagram || m_diagram->views().isEmpty()) return;
	auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst());
	if (!editor) return;

	if (!m_has_anchor)
	{
		editor->statusBar()->showMessage(tr(
			"Left click: place the bridge start; "
			"drag or second click: finish; "
			"right click: cancel; Esc: quit"));
		return;
	}

	int klemmen = 0;
	for (const BridgeCrossing &crossing : std::as_const(m_crossings)) {
		if (!crossing.reference) {
			++klemmen;
		}
	}
	editor->statusBar()->showMessage(tr(
		"Left click or release: finish the line "
		"(%n Klemme(n) recognized)",
		"how many terminals the line being drawn already crosses",
		klemmen));
}

/**
	@brief DiagramEventAddBridge::mousePressEvent
	@param event
*/
void DiagramEventAddBridge::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (Q_UNLIKELY(!m_diagram || m_diagram->isReadOnly())) return;

	if (event->button() == Qt::RightButton)
	{
			//Accepted first: emitting finish() makes the diagram destroy
			//this very tool, so nothing may touch its members afterwards.
		event->setAccepted(true);

			//First right click takes the line being drawn back, second one
			//leaves the tool.
		if (m_has_anchor) {
			reset();
		} else {
			m_running = false;
			emit finish();
		}
		return;
	}
	if (event->button() != Qt::LeftButton) return;

	const QPointF pos = Diagram::snapToGrid(event->scenePos());

	if (!m_has_anchor)
	{
		m_has_anchor = true;
		m_anchor = pos;
		updatePreview(pos);
		event->setAccepted(true);
		return;
	}

	const QPointF end = Diagram::snapToGrid(event->scenePos());
	if (QLineF(m_anchor, end).length() >= min_end_length) {
		finishLine(end);
	}
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddBridge::mouseMoveEvent
	@param event
*/
void DiagramEventAddBridge::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	showGuides(event->scenePos());
	if (!m_has_anchor) return;

	updatePreview(event->scenePos());
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddBridge::mouseReleaseEvent
	Letting go of a drag which was pulled out far enough lays the line
	down straight away; a click without a drag only puts the start point
	down and waits for the next click.
	@param event
*/
void DiagramEventAddBridge::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() != Qt::LeftButton || !m_has_anchor) return;

	const QPointF end = Diagram::snapToGrid(event->scenePos());
	if (QLineF(m_anchor, end).length() >= min_end_length) {
		finishLine(end);
	}
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddBridge::keyPressEvent
	@param event
*/
void DiagramEventAddBridge::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape)
	{
		if (m_has_anchor)
		{
			reset();
			event->setAccepted(true);
			return;
		}
	}
	DiagramEventInterface::keyPressEvent(event);
}

/**
	@brief DiagramEventAddBridge::updatePreview
	Show the line as it will be laid down, and how many terminals it
	runs across at this very moment. The line is the user's own: it goes
	wherever he pulls it, straight or not.
	@param scene_pos where the mouse stands
*/
void DiagramEventAddBridge::updatePreview(const QPointF &scene_pos)
{
	if (!m_has_anchor || !m_diagram) return;

	const QPointF end = Diagram::snapToGrid(scene_pos);

	if (!m_preview)
	{
		m_preview = new QGraphicsLineItem;
		QPen pen(QColor(0, 120, 215));
		pen.setWidthF(2.0);
		pen.setStyle(Qt::DashLine);
		m_preview->setPen(pen);
		m_preview->setZValue(1000.0);
		m_diagram->addItem(m_preview);
	}
	m_preview->setLine(QLineF(m_anchor, end));

		//The count follows the mouse, so what the bridge is going to
		//hold is seen where the line is drawn, not only in the status bar
	if (!m_counter)
	{
		m_counter = new QGraphicsSimpleTextItem;
		m_counter->setBrush(QColor(0, 120, 215));
		m_counter->setZValue(1001.0);
		m_diagram->addItem(m_counter);
	}
	m_counter->setPos(end + QPointF(8, -24));

	m_crossings = BridgeManager::crossedTerminals(m_diagram, QLineF(m_anchor, end));
		//What is counted is what the bridge lands on: a Klemme. A sheet
		//reference is the way to the next sheet and never an Anschluss
		//of the bridge -- it is not counted as one, here or anywhere.
	int klemmen = 0;
	int references = 0;
	for (const BridgeCrossing &crossing : std::as_const(m_crossings)) {
		if (crossing.reference) {
			++references;
		} else {
			++klemmen;
		}
	}
	QString counted = tr("%n Klemme(n)", "counted while the line is pulled", klemmen);
	if (references) {
		counted += tr(", %n sheet reference(s)", "the way to the next sheet", references);
	}
	m_counter->setText(counted);
	updateHint();
}

/**
	@brief DiagramEventAddBridge::showGuides
	The thin cross QElectroTech draws while any shape is drawn.
	@param scene_pos where the mouse stands
*/
void DiagramEventAddBridge::showGuides(const QPointF &scene_pos)
{
	if (!m_diagram) return;

	if (!m_help_horiz || !m_help_verti)
	{
		QPen pen;
		pen.setWidthF(0.4);
		pen.setCosmetic(true);
		pen.setColor(Diagram::background_color == Qt::darkGray ? Qt::lightGray : Qt::darkGray);

		const QRectF rect = m_diagram->border_and_titleblock.insideBorderRect();

		if (!m_help_horiz)
		{
			m_help_horiz = new QGraphicsLineItem(rect.topLeft().x(), 0, rect.topRight().x(), 0);
			m_help_horiz->setPen(pen);
			m_help_horiz->setZValue(999.0);
			m_diagram->addItem(m_help_horiz);
		}
		if (!m_help_verti)
		{
			m_help_verti = new QGraphicsLineItem(0, rect.topLeft().y(), 0, rect.bottomLeft().y());
			m_help_verti->setPen(pen);
			m_help_verti->setZValue(999.0);
			m_diagram->addItem(m_help_verti);
		}
	}

	const QPointF point = Diagram::snapToGrid(scene_pos);
	m_help_horiz->setY(point.y());
	m_help_verti->setX(point.x());
}

/**
	@brief DiagramEventAddBridge::finishLine
	Put the line down and hand it over to the window which turns it into
	a bridge -- a new one, or one the project already holds.
	@param end where the line stops
*/
void DiagramEventAddBridge::finishLine(const QPointF &end)
{
	if (!m_diagram || !m_diagram->project()) {
		reset();
		return;
	}

	const QLineF line(m_anchor, Diagram::snapToGrid(end));
	if (line.length() < min_end_length) {
		reset();
		return;
	}

	m_crossings = BridgeManager::crossedTerminals(m_diagram, line);

		//A line over no terminal is no bridge: what is not touched is
		//not taken, and there is nothing here to take.
	if (m_crossings.isEmpty()) {
		if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst())) {
			editor->statusBar()->showMessage(tr("No terminal crossed: the bridge lands on nothing."));
		}
		reset();
		return;
	}

	QList<BridgeTerminal> terminals;
	for (const BridgeCrossing &crossing : std::as_const(m_crossings)) {
		BridgeTerminal terminal;
		terminal.element = crossing.element->uuid();
		terminal.terminal = crossing.terminal;
		terminal.reference = crossing.reference;
		terminal.terminal = crossing.terminal;
		terminals << terminal;
	}

	BridgeCreateDialog dialog(m_diagram, m_crossings,
							  m_diagram->views().isEmpty() ? nullptr : m_diagram->views().constFirst());
	if (dialog.exec() != QDialog::Accepted) {
		reset();
		return;
	}

	Bridge *bridge = dialog.bridge();
	bool own_bridge = !bridge;

		//What is one bridge carried on over the sheets is one bridge:
		//a line coming to a sheet reference joins the bridge that
		//reference stands for, as if it had been drawn onto it from the
		//start -- one unit, one voltage, one limit.
	Bridge *joined = BridgeManager::joinTarget(m_diagram->project(), terminals, bridge);
	if (own_bridge && joined) {
		bridge = joined;
		own_bridge = false;
	}
	const BridgeInformation information_before{
				bridge ? bridge->tensionText() : QString()};

		//What the bridge carries: the function and the voltage of the
		//terminals it lands on -- and of the bridge it lands on, when
		//there already is one. When they do not agree the user is asked
		//which the bridge takes, the way sheet references are linked --
		//and cancelling aborts: nothing is drawn.
	QList<BridgeInformation> candidates =
			BridgeManager::informationSets(m_diagram->project(), terminals);
	if (!own_bridge) {
		candidates.prepend(information_before);
	}
	bool take_information = false;
	const BridgeInformation information = BridgeCreateDialog::askForInformation(
				candidates, &take_information,
				m_diagram->views().isEmpty() ? nullptr : m_diagram->views().constFirst());
	if (!take_information) {
		reset();
		return;
	}

	if (own_bridge)
	{
		bridge = m_diagram->project()->newBridge();
		bridge->setColor(dialog.newColor());
		bridge->setMaxConnections(dialog.newMaxConnections());
		bridge->setLineWidth(dialog.newLineWidth());
		bridge->setName(dialog.bridgeName());
		bridge->setTensionText(information.tension);

			//Which parallel track the bridge runs in: the first free one
			//at the terminals it crosses. Four are all a terminal takes.
		const int column = BridgeManager::freeColumn(m_diagram->project(), terminals);
		if (!column) {
			m_diagram->project()->removeBridge(bridge);
			delete bridge;
			QMessageBox::warning(nullptr, tr("No free track"),
								 tr("Four bridges already run side by side at one of these "
									"terminals: no track is left for another one."));
			reset();
			return;
		}
		bridge->setColumn(column);
	}

		//The one limit which is never crossed, whatever was drawn: the
		//Klemmen the line lands on and did not land on before. A sheet
		//reference is the way to the next sheet and fills no place of
		//the bridge, so it is not counted here either.
	int added = 0;
	for (const BridgeTerminal &terminal : std::as_const(terminals)) {
		if (!terminal.reference && !bridge->hasTerminal(terminal)) {
			++added;
		}
	}
	if (bridge->wouldExceed(added)) {
		if (own_bridge) {
			m_diagram->project()->removeBridge(bridge);
			delete bridge;
		}
		reset();
		return;
	}

		//One bar of a bridge lies across one Klemmleiste: what the line
		//would take of two of them cannot be built
	const QStringList strips =
			BridgeManager::clashingStrips(m_diagram->project(), terminals);
	if (!strips.isEmpty()) {
		if (own_bridge) {
			m_diagram->project()->removeBridge(bridge);
			delete bridge;
		}
		QMessageBox::warning(nullptr, tr("That bridge cannot be built"),
							 tr("A bridge lies across one terminal strip: the "
								"line would take terminals of %1.")
							 .arg(strips.join(QStringLiteral(", "))));
		reset();
		return;
	}

	BridgePartData part;
	part.uuid = QUuid::createUuid();
	part.diagram = m_diagram->uuid();
	part.p1 = line.p1();
	part.p2 = line.p2();

	bridge->addPart(part);
	for (const BridgeTerminal &terminal : std::as_const(terminals)) {
		bridge->addTerminal(terminal);
	}

	auto *item = new BridgePart(part);
	item->setBridge(bridge);
	m_diagram->addItem(item);

	auto *command = new AddBridgeCommand(bridge, item, m_diagram, own_bridge,
										 own_bridge ? QList<BridgeTerminal>() : terminals,
										 information_before, information);
		//What is one bridge carried on over the sheets is one pair of
		//linked references: they find each other when the halves are
		//joined, and the cross-reference comes with the link
	BridgeManager::pairReferences(m_diagram->project(), bridge, command);
		//Bridge and wires say the same thing: what the bridge carries
		//goes onto the wires it sits on, in the very same undo step
	BridgeManager::writeInformation(m_diagram->project(), bridge->terminals(),
									information, command);
	m_diagram->project()->undoStack()->push(command);

	reset();
}
