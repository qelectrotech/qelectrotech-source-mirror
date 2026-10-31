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
#include "dockfold.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QEvent>
#include <QMainWindow>
#include <QPointer>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QStyle>
#include <QTimer>
#include <QToolButton>

namespace {
	const char *const watched_by = "qetDockFoldWatched";
		/// Every bar, so a change of the setting reaches them all
	QList<QPointer<DockFold>> all_folds;

	/**
		A triangle pointing left or right, in @p color
	*/
	QIcon triangle(bool left, const QColor &color)
	{
		QPixmap pixmap(6, 10);
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(Qt::NoPen);
		painter.setBrush(color);
		const QPolygonF points = left
				? QPolygonF({QPointF(6, 0), QPointF(6, 10), QPointF(0, 5)})
				: QPolygonF({QPointF(0, 0), QPointF(0, 10), QPointF(6, 5)});
		painter.drawPolygon(points);
		return QIcon(pixmap);
	}
}

DockFold::DockFold(QMainWindow *window, Qt::DockWidgetArea area) :
	QObject(window),
	m_window(window),
	m_area(area),
	m_button(new QToolButton(window)),
	m_action(new QAction(this)),
	m_refresh_timer(new QTimer(this))
{
	all_folds << this;
	m_refresh_timer->setSingleShot(true);
	m_refresh_timer->setInterval(0);
	connect(m_refresh_timer, &QTimer::timeout, this, &DockFold::refresh);
	connect(m_action, &QAction::triggered, this, &DockFold::toggle);
	window->addAction(m_action);
	m_button->setObjectName(area == Qt::RightDockWidgetArea
				? QStringLiteral("qet_dock_fold_right")
				: QStringLiteral("qet_dock_fold_left"));
		//A bar with a triangle, as beside the sidebar of LibreOffice, in
		//the colours of the theme
	m_button->setStyleSheet(QStringLiteral(
		"QToolButton { background: palette(mid); border: none; border-radius: 1px; }"
		"QToolButton:hover { background: palette(dark); }"
		"QToolButton:pressed { background: palette(shadow); }"));
	m_button->setIconSize(QSize(6, 10));
	m_button->setFocusPolicy(Qt::NoFocus);
	m_button->setCursor(Qt::PointingHandCursor);
	m_button->setFixedSize(10, 56);
	m_button->hide();
	connect(m_button, &QToolButton::clicked, this, &DockFold::toggle);

		//Folded at the last close: the docks stay folded, still hidden
		//by the window's saved state
	QSettings settings;
	m_folded = settings.value(settingKey() + QStringLiteral("/docks")).toStringList();
	m_in_front = settings.value(settingKey() + QStringLiteral("/in_front")).toStringList();
	m_width = settings.value(settingKey() + QStringLiteral("/width"), 0).toInt();
	for (const QVariant &height : settings.value(settingKey() + QStringLiteral("/heights")).toList())
		m_heights << height.toInt();

	window->installEventFilter(this);
	if (window->centralWidget())
		window->centralWidget()->installEventFilter(this);
	scheduleRefresh();
}

bool DockFold::enabled()
{
	return QSettings().value(QStringLiteral("diagrameditor/dock_fold_buttons"), false).toBool();
}

void DockFold::setEnabled(bool on)
{
	QSettings().setValue(QStringLiteral("diagrameditor/dock_fold_buttons"), on);
	for (const QPointer<DockFold> &fold : std::as_const(all_folds))
		if (fold)
			fold->refresh();
}

/**
	Refresh once the events of this round are through: a resize brings
	several of them, one refresh is enough
*/
void DockFold::scheduleRefresh()
{
	if (!m_busy)
		m_refresh_timer->start();
}

void DockFold::toggle()
{
	if (isFolded())
		unfold();
	else
		fold();
}

bool DockFold::eventFilter(QObject *watched, QEvent *event)
{
	switch (event->type()) {
		case QEvent::Resize:
		case QEvent::Move:
		case QEvent::Show:
		case QEvent::Hide:
		case QEvent::LayoutRequest:
		case QEvent::ChildAdded:
			scheduleRefresh();
			break;
		default:
			break;
	}
	return QObject::eventFilter(watched, event);
}

/**
	@return the docks of this side that are open, in front of their tab
	group or behind it, not floating
*/
QList<QDockWidget *> DockFold::openDocks() const
{
	QList<QDockWidget *> docks;
	for (QDockWidget *dock : m_window->findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly))
		if (m_window->dockWidgetArea(dock) == m_area && !dock->isFloating()
		    && dock->toggleViewAction()->isChecked())
			docks << dock;
	return docks;
}

QDockWidget *DockFold::dockNamed(const QString &name) const
{
	for (QDockWidget *dock : m_window->findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly))
		if (dock->objectName() == name)
			return dock;
	return nullptr;
}

void DockFold::fold()
{
	const QList<QDockWidget *> docks = openDocks();
	if (docks.isEmpty())
		return;
	m_busy = true;
	m_folded.clear();
	m_in_front.clear();
	m_heights.clear();
	m_width = 0;
	for (QDockWidget *dock : docks) {
		m_folded << dock->objectName();
		if (dock->isVisible()) {
			m_in_front << dock->objectName();
			m_heights << dock->height();
			m_width = qMax(m_width, dock->width());
		}
	}
	for (QDockWidget *dock : docks)
		dock->hide();
	m_busy = false;
	remember();
	refresh();
}

void DockFold::unfold()
{
	m_busy = true;
	QList<QDockWidget *> shown;
	for (const QString &name : std::as_const(m_folded))
		if (QDockWidget *dock = dockNamed(name))
			dock->show();
	QList<int> heights;
	for (int i = 0; i < m_in_front.size(); ++i)
		if (QDockWidget *dock = dockNamed(m_in_front.at(i))) {
			dock->raise();
			shown << dock;
			heights << m_heights.value(i, dock->height());
		}
	const int width = m_width;
	m_folded.clear();
	m_in_front.clear();
	m_heights.clear();
	m_width = 0;
	m_busy = false;
	remember();
		//Once the docks have their place again
	QPointer<QMainWindow> window = m_window;
	QTimer::singleShot(0, this, [this, window, shown, heights, width] {
		if (!window)
			return;
		QList<QDockWidget *> docks;
		QList<int> sizes;
		for (int i = 0; i < shown.size(); ++i)
			if (shown.at(i)) {
				docks << shown.at(i);
				sizes << heights.at(i);
			}
		if (!docks.isEmpty()) {
			if (width > 0)
				window->resizeDocks({docks.first()}, {width}, Qt::Horizontal);
			window->resizeDocks(docks, sizes, Qt::Vertical);
		}
		refresh();
	});
}

void DockFold::refresh()
{
	if (m_busy)
		return;
	const QList<QDockWidget *> open = openDocks();
	for (QDockWidget *dock : m_window->findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly))
		if (!dock->property(watched_by).toBool()) {
			dock->setProperty(watched_by, true);
			connect(dock, &QDockWidget::visibilityChanged, this, &DockFold::scheduleRefresh);
			connect(dock, &QDockWidget::dockLocationChanged, this, &DockFold::scheduleRefresh);
		}
		//A dock of this side opened another way: the side is no longer
		//folded
	if (isFolded() && !open.isEmpty()) {
		m_folded.clear();
		m_in_front.clear();
		m_heights.clear();
		m_width = 0;
		remember();
	}

	const bool right = m_area == Qt::RightDockWidgetArea;
		//The arrow points where the docks go
	m_button->setIcon(triangle(isFolded() == right,
				   m_button->palette().color(QPalette::ButtonText)));
	m_button->setToolTip(isFolded() ? tr("Show the side panels again")
					: tr("Fold the side panels away"));
	m_action->setText(right
		? (isFolded() ? tr("Show the right side panels again") : tr("Fold the right side panels away"))
		: (isFolded() ? tr("Show the left side panels again") : tr("Fold the left side panels away")));
	m_action->setEnabled(isFolded() || !open.isEmpty());
	reserveStrip(enabled());
	if (!enabled() || (open.isEmpty() && !isFolded())) {
		m_button->hide();
		return;
	}
	place();
	m_button->show();
	m_button->raise();
}

/**
	Keep a strip of the bar's width free at this side of the central
	widget, or give it back
*/
void DockFold::reserveStrip(bool on)
{
	QWidget *central = m_window->centralWidget();
	if (!central)
		return;
	QMargins margins = central->contentsMargins();
	const int strip = on ? m_button->width() : 0;
	if (m_area == Qt::RightDockWidgetArea) {
		if (margins.right() == strip)
			return;
		margins.setRight(strip);
	} else {
		if (margins.left() == strip)
			return;
		margins.setLeft(strip);
	}
	central->setContentsMargins(margins);
}

/**
	The bar stands in its strip at this side of the central widget, in
	the middle of its height
*/
void DockFold::place()
{
	QWidget *central = m_window->centralWidget();
	if (!central)
		return;
	const QRect area = central->geometry();
	const int x = m_area == Qt::RightDockWidgetArea
			? area.right() + 1 - m_button->width()
			: area.left();
	m_button->move(x, area.center().y() - m_button->height() / 2);
}

QString DockFold::settingKey() const
{
	return m_area == Qt::RightDockWidgetArea ? QStringLiteral("diagrameditor/folded_right")
						 : QStringLiteral("diagrameditor/folded_left");
}

void DockFold::remember()
{
	QSettings settings;
	if (!isFolded()) {
		settings.remove(settingKey());
		return;
	}
	settings.setValue(settingKey() + QStringLiteral("/docks"), m_folded);
	settings.setValue(settingKey() + QStringLiteral("/in_front"), m_in_front);
	settings.setValue(settingKey() + QStringLiteral("/width"), m_width);
	QVariantList heights;
	for (int height : std::as_const(m_heights))
		heights << height;
	settings.setValue(settingKey() + QStringLiteral("/heights"), heights);
}
