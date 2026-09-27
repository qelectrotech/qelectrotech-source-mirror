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
#include "diagramcontexttoolbar.h"

#include <QAction>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QSettings>
#include <QToolButton>

namespace {
	/// Fully opaque within this distance of the toolbar, in pixels...
const int opaque_distance = 30;
	/// ...fading to hidden at this one
const int hide_distance = 200;
}

/**
	@brief DiagramContextToolbar::DiagramContextToolbar
	@param viewport : the view's viewport, which it is drawn on and watches
*/
DiagramContextToolbar::DiagramContextToolbar(QWidget *viewport) :
	QFrame(viewport)
{
	setFrameShape(QFrame::StyledPanel);
	setAutoFillBackground(true);
	setFocusPolicy(Qt::NoFocus);

	m_layout = new QHBoxLayout(this);
	m_layout->setContentsMargins(2, 2, 2, 2);
	m_layout->setSpacing(1);

	m_opacity = new QGraphicsOpacityEffect(this);
	setGraphicsEffect(m_opacity);
	hide();

	viewport->installEventFilter(this);
		//Key presses go to the view, not its viewport
	if (viewport->parentWidget()) {
		viewport->parentWidget()->installEventFilter(this);
	}
}

/**
	@return whether the toolbar should be shown at all (a preference)
*/
bool DiagramContextToolbar::isEnabled()
{
	return QSettings().value(QStringLiteral("diagrameditor/context_toolbar"),
				 true).toBool();
}

/**
	@brief DiagramContextToolbar::showAt
	Show @a actions just above and to the right of @a viewport_pos, kept
	inside the viewport. Clicking a button triggers the action and leaves
	the toolbar up, so a command such as rotate can be clicked again.
*/
void DiagramContextToolbar::showAt(const QPoint &viewport_pos,
				   const QList<QAction *> &actions)
{
	while (QLayoutItem *item = m_layout->takeAt(0)) {
		delete item->widget();
		delete item;
	}
	for (QAction *action : actions.mid(0, max_actions))
	{
		auto *button = new QToolButton(this);
		button->setAutoRaise(true);
		button->setFocusPolicy(Qt::NoFocus);
		button->setIconSize(QSize(20, 20));
		const QString text = action->text().remove(QLatin1Char('&'));
		if (action->icon().isNull()) {
			button->setText(text);
		} else {
			button->setIcon(action->icon());
		}
		button->setToolTip(text);
		button->setEnabled(action->isEnabled());
		connect(button, &QToolButton::clicked, action, &QAction::trigger);
		m_layout->addWidget(button);
	}
	if (actions.isEmpty()) {
		hide();
		return;
	}

	adjustSize();
	const QRect area = parentWidget()->rect();
	QPoint pos = viewport_pos + QPoint(16, -height() - 16);
	pos.setX(qBound(area.left(), pos.x(), area.right() - width()));
	pos.setY(qBound(area.top(), pos.y(), area.bottom() - height()));
	move(pos);
	m_opacity->setOpacity(1.0);
	show();
	raise();
}

/**
	@brief DiagramContextToolbar::followMouse
	Fade with the mouse's distance from the toolbar, and hide past
	hide_distance.
*/
void DiagramContextToolbar::followMouse(const QPoint &viewport_pos)
{
	const QRect r = geometry();
	const int dx = qMax(qMax(r.left() - viewport_pos.x(), 0), viewport_pos.x() - r.right());
	const int dy = qMax(qMax(r.top() - viewport_pos.y(), 0), viewport_pos.y() - r.bottom());
	const int distance = qMax(dx, dy);

	if (distance >= hide_distance) {
		hide();
		return;
	}
	const qreal fade = qreal(distance - opaque_distance)
			   / (hide_distance - opaque_distance);
	m_opacity->setOpacity(qBound(0.15, 1.0 - fade, 1.0));
}

/**
	@brief DiagramContextToolbar::eventFilter
	Watch the viewport and the view while shown: follow the mouse, and hide
	on a press elsewhere, a wheel turn or a key press.
*/
bool DiagramContextToolbar::eventFilter(QObject *watched, QEvent *event)
{
	Q_UNUSED(watched)
	if (!isVisible()) {
		return false;
	}

	switch (event->type())
	{
		case QEvent::MouseMove:
			followMouse(static_cast<QMouseEvent *>(event)->position().toPoint());
			break;
		case QEvent::MouseButtonPress:
		case QEvent::Wheel:
		case QEvent::KeyPress:
		case QEvent::Leave:
			hide();
			break;
		default:
			break;
	}
	return false;
}
