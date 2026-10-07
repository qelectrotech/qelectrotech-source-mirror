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
#include "diagrameventfillet.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../qetgraphicsitem/qetshapeitem.h"
#include "../qgimanager.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QInputDialog>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QUndoCommand>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

const char *radiusSettingKey = "diagrameditor/fillet_radius";

/**
	Adds the fillet's arc to the diagram exactly where it was computed.
	AddGraphicsObjectCommand is not used: its redo() calls setPos(),
	and QetShapeItem::setPos() snaps the outline's corner to the grid,
	which would pull the arc off the two lines it has to touch.
*/
class AddFilletArcCommand : public QUndoCommand
{
	public:
		AddFilletArcCommand(QetShapeItem *arc, Diagram *diagram, QUndoCommand *parent) :
			QUndoCommand(parent), m_arc(arc), m_diagram(diagram)
		{ m_diagram->qgiManager().manage(m_arc); }
		~AddFilletArcCommand() override
		{ m_diagram->qgiManager().release(m_arc); }
		void redo() override { m_diagram->addItem(m_arc); }
		void undo() override { m_diagram->removeItem(m_arc); }

	private:
		QetShapeItem *m_arc;
		Diagram *m_diagram;
};

qreal length(const QPointF &v) { return std::hypot(v.x(), v.y()); }
qreal dot(const QPointF &a, const QPointF &b) { return a.x() * b.x() + a.y() * b.y(); }

qreal distanceToSegment(const QPointF &p, const QLineF &segment)
{
	const QPointF d = segment.p2() - segment.p1();
	const qreal len2 = dot(d, d);
	if (len2 <= 0)
		return length(p - segment.p1());
	const qreal t = qBound(qreal(0), dot(p - segment.p1(), d) / len2, qreal(1));
	return length(p - (segment.p1() + t * d));
}

QLineF sceneLine(const QetShapeItem *line)
{
	return QLineF(line->mapToScene(line->line().p1()), line->mapToScene(line->line().p2()));
}

	//Angle of a point on a circle, in the convention QPainterPath::arcTo()
	//and QetShapeItem use: degrees, anticlockwise from three o'clock.
qreal arcAngle(const QPointF &centre, const QPointF &p)
{
	return qRadiansToDegrees(std::atan2(-(p.y() - centre.y()), p.x() - centre.x()));
}

	//Same test as QetShapeItem::setStartAngle()/setEndAngle() use to snap
	//an arc back to a full ellipse.
bool nearlyFullTurn(qreal span)
{
	qreal wrapped = std::fmod(span, 360.0);
	if (wrapped < 0) wrapped += 360.0;
	return wrapped < 5.0 || wrapped > 355.0;
}

} // namespace

DiagramEventFillet::DiagramEventFillet(Diagram *diagram) :
	DiagramEventInterface(diagram)
{
	m_running = true;
		//Deferred for the same reason as DiagramEventAddShape's hint:
		//the previous tool's destructor clears the status bar after
		//this constructor returns.
	QTimer::singleShot(0, this, [this]() { showDefaultHint(); });
}

DiagramEventFillet::~DiagramEventFillet()
{
	clearFirstPick();
	if (m_diagram && !m_diagram->views().isEmpty())
	{
		if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst()))
			editor->statusBar()->clearMessage();
	}
}

void DiagramEventFillet::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	event->setAccepted(true);
	if (Q_UNLIKELY(m_diagram->isReadOnly()))
		return;

	if (event->button() == Qt::RightButton)
	{
		if (m_first_line) {
			clearFirstPick();
			showDefaultHint();
		} else {
			m_running = false;
			emit finish();
		}
		return;
	}
	if (event->button() != Qt::LeftButton)
		return;

	const QPointF pos = event->scenePos();
	const QList<QetShapeItem *> lines = linesNear(pos);
	if (lines.isEmpty()) {
		showHint(tr("No line here: click a line drawn on the sheet"));
		return;
	}

	if (!m_first_line)
	{
			//One click where two lines meet picks both of them
		if (lines.size() >= 2) {
			filletLines({lines.at(0), pos}, {lines.at(1), pos});
			return;
		}
		m_first = {lines.first(), pos};
		m_first_line = lines.first();
		m_diagram->clearSelection();
		m_first_line->setSelected(true);
		showHint(tr("Click the second line; right click: cancel"));
		return;
	}

	const auto other = std::find_if(lines.cbegin(), lines.cend(),
		[this](QetShapeItem *line) { return line != m_first_line; });
	if (other == lines.cend()) {
		showHint(tr("Click another line; right click: cancel"));
		return;
	}
	const Pick first = m_first;
	clearFirstPick();
	filletLines(first, {*other, pos});
}

/**
	@brief DiagramEventFillet::linesNear
	@return the Line shapes passing within a few screen pixels of
	scenePos, nearest first. Only the items under the cursor are looked
	at; the diagram is not walked.
*/
QList<QetShapeItem *> DiagramEventFillet::linesNear(const QPointF &scenePos) const
{
	qreal scale = 1;
	if (!m_diagram->views().isEmpty())
		scale = m_diagram->views().constFirst()->transform().m11();
	const qreal tolerance = 6.0 / qMax(scale, qreal(0.01));

	QList<QPair<qreal, QetShapeItem *>> found;
	const QRectF area(scenePos - QPointF(tolerance, tolerance), QSizeF(2 * tolerance, 2 * tolerance));
	for (QGraphicsItem *item : m_diagram->items(area))
	{
		if (item->type() != QetShapeItem::Type)
			continue;
		auto *shape = static_cast<QetShapeItem *>(item);
		if (shape->shapeType() != QetShapeItem::Line)
			continue;
		const qreal distance = distanceToSegment(scenePos, sceneLine(shape));
		if (distance <= tolerance)
			found.append({distance, shape});
	}
	std::sort(found.begin(), found.end(),
		[](const auto &a, const auto &b) { return a.first < b.first; });

	QList<QetShapeItem *> lines;
	for (const auto &f : found)
		lines.append(f.second);
	return lines;
}

/**
	@brief DiagramEventFillet::filletLines
	Rounds the corner between the two picked lines. For each line, the
	part kept is the one on the side of the corner where it was clicked;
	a click on the corner itself keeps the longer part. The other end of
	each line moves to the point where the arc touches it -- shortening
	the line, or lengthening it when the two lines did not reach each
	other. One undo step undoes the whole fillet.
*/
void DiagramEventFillet::filletLines(const Pick &a, const Pick &b)
{
	const QLineF s1 = sceneLine(a.line);
	const QLineF s2 = sceneLine(b.line);

	QPointF corner;
	if (s1.intersects(s2, &corner) == QLineF::NoIntersection) {
		showHint(tr("These two lines are parallel: no fillet possible"));
		return;
	}

	qreal scale = 1;
	if (!m_diagram->views().isEmpty())
		scale = m_diagram->views().constFirst()->transform().m11();
	const qreal cornerTolerance = 6.0 / qMax(scale, qreal(0.01));

	auto keptEnd = [&](const QLineF &segment, const QPointF &pick) -> int {
		const QPointF ends[2] = {segment.p1(), segment.p2()};
		const QPointF towardPick = pick - corner;
		const bool onCorner = length(towardPick) < cornerTolerance;
		int best = -1;
		for (int pass = 0; pass < 2 && best < 0; ++pass) {
			qreal bestLength = -1;
			for (int i = 0; i < 2; ++i) {
				const QPointF v = ends[i] - corner;
				if (pass == 0 && !onCorner && dot(v, towardPick) <= 0)
					continue;
				if (length(v) > bestLength) { best = i; bestLength = length(v); }
			}
		}
		return best;
	};
	const int keep1 = keptEnd(s1, a.pos);
	const int keep2 = keptEnd(s2, b.pos);
	const QPointF e1 = keep1 == 0 ? s1.p1() : s1.p2();
	const QPointF e2 = keep2 == 0 ? s2.p1() : s2.p2();
	const qreal len1 = length(e1 - corner);
	const qreal len2 = length(e2 - corner);
	if (len1 < 1e-6 || len2 < 1e-6) {
		showHint(tr("One of the lines stops at the corner: nothing to round on that side"));
		return;
	}

	const QPointF u1 = (e1 - corner) / len1;
	const QPointF u2 = (e2 - corner) / len2;
	const qreal theta = std::acos(qBound(qreal(-1), dot(u1, u2), qreal(1)));
	if (qRadiansToDegrees(theta) > 174.0) {
		showHint(tr("These two lines are almost in line: no fillet possible"));
		return;
	}

	QSettings settings;
	bool ok = false;
	QWidget *parent = m_diagram->views().isEmpty() ? nullptr : m_diagram->views().constFirst();
	const qreal radius = QInputDialog::getDouble(parent, tr("Fillet"), tr("Radius:"),
		settings.value(radiusSettingKey, 20.0).toDouble(), 0.1, 100000.0, 1, &ok);
	if (!ok || !m_diagram)
		return;
	settings.setValue(radiusSettingKey, radius);

	const qreal tangentDistance = radius / std::tan(theta / 2);
	if (tangentDistance > len1 + 1e-6 || tangentDistance > len2 + 1e-6) {
		showHint(tr("Radius too large for these lines (%1 at most)")
				 .arg(std::min(len1, len2) * std::tan(theta / 2), 0, 'f', 1));
		return;
	}

	const QPointF t1 = corner + u1 * tangentDistance;
	const QPointF t2 = corner + u2 * tangentDistance;
	const QPointF bisector = (u1 + u2) / length(u1 + u2);
	const QPointF centre = corner + bisector * (radius / std::sin(theta / 2));

		//Each line keeps its picked end and moves its other end to the
		//tangent point; p1/p2 keep their roles, mapped back into the
		//line's own coordinates (it may be rotated or skewed).
	auto newLine = [](QetShapeItem *line, int keep, const QPointF &kept, const QPointF &tangent) {
		const QPointF p1 = keep == 0 ? kept : tangent;
		const QPointF p2 = keep == 0 ? tangent : kept;
		return QLineF(line->mapFromScene(p1), line->mapFromScene(p2));
	};

	auto *arc = new QetShapeItem(centre - QPointF(radius, radius),
								 centre + QPointF(radius, radius), QetShapeItem::Ellipse);
	arc->setPen(a.line->pen());
	qreal start = std::fmod(arcAngle(centre, t1), 360.0);
	if (start < 0) start += 360.0;
	const qreal span = std::fmod(arcAngle(centre, t2) - start + 540.0, 360.0) - 180.0;
		//Set the two angles in an order whose intermediate state is not
		//close to a full turn, which the setters would snap to an ellipse.
	if (!nearlyFullTurn(360.0 - start)) {
		arc->setStartAngle(start);
		arc->setEndAngle(start + span);
	} else {
		arc->setEndAngle(start + span);
		arc->setStartAngle(start);
	}

	auto *undo = new QUndoCommand(tr("Add a fillet"));
	new QPropertyUndoCommand(a.line, "line", a.line->line(), newLine(a.line, keep1, e1, t1), undo);
	new QPropertyUndoCommand(b.line, "line", b.line->line(), newLine(b.line, keep2, e2, t2), undo);
	new AddFilletArcCommand(arc, m_diagram, undo);
	m_diagram->undoStack().push(undo);
	showDefaultHint();
}

void DiagramEventFillet::clearFirstPick()
{
	if (m_first_line)
		m_first_line->setSelected(false);
	m_first_line.clear();
	m_first = Pick();
}

void DiagramEventFillet::showHint(const QString &text) const
{
	if (!m_diagram || m_diagram->views().isEmpty())
		return;
	if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst()))
		editor->statusBar()->showMessage(text);
}

void DiagramEventFillet::showDefaultHint() const
{
	showHint(tr("Fillet: click two lines, or once where they meet; Esc or right click: "
				"finish"));
}
