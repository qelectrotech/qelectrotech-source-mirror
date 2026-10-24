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
#include "terminalstriplabels.h"

#include "../diagram.h"
#include "../qetproject.h"
#include "../qetgraphicsitem/terminalelement.h"
#include "terminalstrip.h"
#include "../qetgraphicsitem/terminal.h"

#include <QHash>
#include <QPointer>
#include <QThread>
#include <QTimer>

namespace {

	/// One per folio: gathers the changes of a moment into one refresh
class Updater : public QObject
{
	public:
		explicit Updater(Diagram *diagram) : QObject(diagram), m_diagram(diagram)
		{
			setObjectName(QStringLiteral("TerminalStripLabelsUpdater"));
			m_timer.setSingleShot(true);
			m_timer.setInterval(0);
			connect(&m_timer, &QTimer::timeout, this, [this]() {
				TerminalStripLabels::refresh(m_diagram);
			});
		}
		void schedule() { m_timer.start(); }
			/// A strip renamed or reordered changes its labels too
		void follow(TerminalStrip *strip)
		{
			if (m_followed.contains(strip))
				return;
			m_followed << strip;
			connect(strip, &TerminalStrip::dataChanged, this, &Updater::schedule);
			connect(strip, &TerminalStrip::orderChanged, this, &Updater::schedule);
			connect(strip, &TerminalStrip::bridgeChanged, this, &Updater::schedule);
			connect(strip, &QObject::destroyed, this, [this, strip]() {
				m_followed.removeAll(strip);
				schedule();
			});
		}

	private:
		Diagram *m_diagram;
		QTimer m_timer;
		QList<TerminalStrip *> m_followed;
};

Updater *updaterOf(Diagram *diagram)
{
	QObject *found = diagram->findChild<QObject *>(QStringLiteral("TerminalStripLabelsUpdater"),
												   Qt::FindDirectChildrenOnly);
	return found ? static_cast<Updater *>(found) : new Updater(diagram);
}

	/// @return true when @a a comes before @a b in reading order
bool before(const TerminalElement *a, const TerminalElement *b)
{
	const QPointF pa = a->scenePos(), pb = b->scenePos();
	if (!qFuzzyCompare(pa.x() + 1, pb.x() + 1))
		return pa.x() < pb.x();
	return pa.y() < pb.y();
}

	/// The bridge from @a a to the next terminal of its row, in @a a's
	/// coordinates, or a null line when it has none
QLineF bridgeLine(TerminalElement *a, TerminalStrip *strip, const QList<TerminalElement *> &row)
{
	const auto bridge = strip->isBridged(a->realTerminal());
	if (!bridge)
		return QLineF();

		//The next terminal along the row: on a's own axis, after it
	TerminalElement *next = nullptr;
	qreal next_x = 0;
	for (TerminalElement *b : row)
	{
		if (b == a || qAbs(b->sceneTransform().map(QLineF(0, 0, 1, 0)).angleTo(
						   a->sceneTransform().map(QLineF(0, 0, 1, 0)))) > 0.5)
			continue;
		const QPointF p = a->mapFromScene(b->scenePos());
		if (qAbs(p.y()) > 0.5 || p.x() <= 0.5)
			continue;
		if (!next || p.x() < next_x) {
			next = b;
			next_x = p.x();
		}
	}
	if (!next || strip->isBridged(next->realTerminal()) != bridge)
		return QLineF();

		//From the last connection point of a to the first of the next
	qreal from = 0, to = next_x;
	bool first = true;
	for (Terminal *t : a->terminals())
		from = first ? (first = false, a->mapFromScene(t->dockConductor()).x())
					 : qMax(from, a->mapFromScene(t->dockConductor()).x());
	first = true;
	for (Terminal *t : next->terminals())
		to = first ? (first = false, a->mapFromScene(t->dockConductor()).x())
				   : qMin(to, a->mapFromScene(t->dockConductor()).x());
	return to > from ? QLineF(from, 0, to, 0) : QLineF();
}

} // namespace

QString TerminalStripLabels::text(const TerminalStrip *strip)
{
	if (!strip)
		return QString();
	const QString name = strip->name();
	if (name.isEmpty() || name.startsWith(QLatin1Char('-')))
		return name;
	return QLatin1Char('-') + name;
}

void TerminalStripLabels::schedule(Diagram *diagram)
{
	if (!diagram)
		return;
		//Without a running event loop (an export from the command line,
		//a script run headless) a timer never fires: work them out now
	if (QThread::currentThread()->loopLevel() == 0)
		refresh(diagram);
	else
		updaterOf(diagram)->schedule();
}

void TerminalStripLabels::refresh(Diagram *diagram)
{
	if (!diagram)
		return;
	const bool shown = diagram->project() && diagram->project()->terminalStripLabelsShown();
	const bool bridges = diagram->project() && diagram->project()->terminalStripBridgesShown();
	QHash<TerminalStrip *, QList<TerminalElement *>> rows;

	QList<TerminalElement *> terminals;
	QHash<TerminalStrip *, TerminalElement *> first;
	for (Element *e : diagram->elements())
	{
		auto *te = qobject_cast<TerminalElement *>(e);
		if (!te)
			continue;
		terminals << te;
		TerminalStrip *strip = te->parentTerminalStrip();
		if (!strip || !(shown || bridges))
			continue;
		updaterOf(diagram)->follow(strip);
		rows[strip] << te;
		if (!shown)
			continue;
		TerminalElement *&current = first[strip];
		if (!current || before(te, current))
			current = te;
	}

	for (TerminalElement *te : std::as_const(terminals))
	{
		TerminalStrip *strip = te->parentTerminalStrip();
		te->setStripLabel(strip && first.value(strip) == te ? text(strip) : QString());
		te->setBridgeLine(bridges && strip ? bridgeLine(te, strip, rows.value(strip)) : QLineF());
	}
}
