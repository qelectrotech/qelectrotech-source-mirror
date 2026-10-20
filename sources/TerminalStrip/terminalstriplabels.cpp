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

#include <QHash>
#include <QPointer>
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
	if (diagram)
		updaterOf(diagram)->schedule();
}

void TerminalStripLabels::refresh(Diagram *diagram)
{
	if (!diagram)
		return;
	const bool shown = diagram->project() && diagram->project()->terminalStripLabelsShown();

	QList<TerminalElement *> terminals;
	QHash<TerminalStrip *, TerminalElement *> first;
	for (Element *e : diagram->elements())
	{
		auto *te = qobject_cast<TerminalElement *>(e);
		if (!te)
			continue;
		terminals << te;
		TerminalStrip *strip = te->parentTerminalStrip();
		if (!shown || !strip)
			continue;
		updaterOf(diagram)->follow(strip);
		TerminalElement *&current = first[strip];
		if (!current || before(te, current))
			current = te;
	}

	for (TerminalElement *te : std::as_const(terminals))
	{
		TerminalStrip *strip = te->parentTerminalStrip();
		te->setStripLabel(strip && first.value(strip) == te ? text(strip) : QString());
	}
}
