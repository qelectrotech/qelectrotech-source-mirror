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
#include "propertieseditordockwidget.h"
#include "ui_propertieseditordockwidget.h"
#include "propertieseditorwidget.h"

#include <QAbstractScrollArea>
#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QEvent>
#include <QMainWindow>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>

/**
	@brief PropertiesEditorDockWidget::PropertiesEditorDockWidget
	Constructor
	@param parent : parent widget
*/
PropertiesEditorDockWidget::PropertiesEditorDockWidget(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::PropertiesEditorDockWidget)
{
	ui->setupUi(this);
		// The editors scroll when the dock is lower than they are, rather
		// than spilling over the dock below; see fitScrollArea()
	ui->m_scroll_contents->installEventFilter(this);
		// The background of the dock, not of a text field, in every theme
	ui->m_scroll_area->setBackgroundRole(QPalette::Window);
	ui->m_scroll_area->viewport()->setBackgroundRole(QPalette::Window);
	ui->m_scroll_contents->setAutoFillBackground(false);
	fitScrollArea();
		// Tabbed or not can change without the selection changing
	connect(this, &QDockWidget::dockLocationChanged,
		this, &PropertiesEditorDockWidget::updateSizeConstraint);
	connect(this, &QDockWidget::topLevelChanged,
		this, &PropertiesEditorDockWidget::updateSizeConstraint);
	connect(this, &QDockWidget::visibilityChanged,
		this, &PropertiesEditorDockWidget::updateSizeConstraint);
}

/**
	@brief PropertiesEditorDockWidget::~PropertiesEditorDockWidget
	Destructor
*/
PropertiesEditorDockWidget::~PropertiesEditorDockWidget()
{
	clear();
	delete ui;
}

/**
	@brief PropertiesEditorDockWidget::clear
	Remove all editor present in this dock and delete it.
	They also disabled the button box at the bottom of this dock
*/
void PropertiesEditorDockWidget::clear()
{
	if (!m_editor_list.isEmpty())
		rememberHeight();

	foreach (PropertiesEditorWidget *editor, m_editor_list)
	{
		m_editor_list.removeOne(editor);
		ui->m_main_vlayout->removeWidget(editor);
		delete editor;
	}

	m_editor_list.clear();
	updateSizeConstraint();
}

/**
	@brief PropertiesEditorDockWidget::apply
	Call the apply method for each editor present in this dock
*/
void PropertiesEditorDockWidget::apply()
{
	foreach(PropertiesEditorWidget *editor, m_editor_list)
		editor->apply();
}

/**
	@brief PropertiesEditorDockWidget::reset
	Call the reset method for each editor present in this widget
*/
void PropertiesEditorDockWidget::reset()
{
	foreach(PropertiesEditorWidget *editor, m_editor_list)
		editor->reset();
}

/**
	@brief PropertiesEditorDockWidget::addEditor
	Add an editor in this dock at index in the main vertical layout
	(note the button box are displayed at bottom of this layout by default)
	When an editor is added, we enable the button box
	@param editor : editor to add;
	@param index : index of editor in the layout
	@return true if was added (or already add)
	or false if can't be add (editor = nullptr)
*/
bool PropertiesEditorDockWidget::addEditor(PropertiesEditorWidget *editor,
					   int index)
{
	if (!editor) return false;
	if (m_editor_list.contains(editor)) return true;

	const bool was_empty = m_editor_list.isEmpty();
	ui -> m_main_vlayout -> insertWidget(index, editor);
	m_editor_list << editor;
	if (was_empty)
		restoreHeight();
	updateSizeConstraint();
	return true;
}

/**
	@brief PropertiesEditorDockWidget::editors
	@return all editor used in this dock
*/
QList<PropertiesEditorWidget *> PropertiesEditorDockWidget::editors() const
{
	return m_editor_list;
}

/**
	@brief PropertiesEditorDockWidget::removeEditor
	Remove editor from this dock.
	The editor wasn't delete a the end of this method
	If the editor was the last on this widget, we disabled the button box
	@param editor : editor to remove
	@return true on success, else false
*/
bool PropertiesEditorDockWidget::removeEditor(PropertiesEditorWidget *editor)
{
	if (m_editor_list.size() == 1 && m_editor_list.first() == editor)
		rememberHeight();

	bool result = m_editor_list.removeOne(editor);
	if (result)
		ui -> m_main_vlayout -> removeWidget(editor);
	updateSizeConstraint();

	return result;
}

/**
	@brief PropertiesEditorDockWidget::rememberHeight
	The dock shrinks to its title bar while it has no editor, and the
	other docks of its area take the room. Keep the heights the docks
	of the area have now, the heights the user gave them, to give them
	back when an editor comes back.
*/
void PropertiesEditorDockWidget::rememberHeight()
{
		// Still the shrunk height: the last editor came and went
		// before the heights were given back
	if (m_restore_pending || isFloating() || !isVisible())
		return;
		// A tabbed dock does not shrink: nothing to give back
	if (isTabbed()) {
		m_area_docks.clear();
		m_area_heights.clear();
		return;
	}
	auto main_window = qobject_cast<QMainWindow *>(parentWidget());
	if (!main_window)
		return;

	m_area_docks.clear();
	m_area_heights.clear();
	const Qt::DockWidgetArea area = main_window->dockWidgetArea(this);
	for (QDockWidget *dock : main_window->findChildren<QDockWidget *>())
	{
		if (dock->parentWidget() == main_window && !dock->isFloating()
			&& dock->isVisible()
			&& main_window->dockWidgetArea(dock) == area)
		{
			m_area_docks << dock;
			m_area_heights << dock->height();
		}
	}
}

/**
	@brief PropertiesEditorDockWidget::restoreHeight
	Give the docks of the area back the heights they had before this
	dock shrank. Without this the area is split again from the docks'
	size hints, and giving this dock alone its height back takes the
	room only from its neighbours.
*/
void PropertiesEditorDockWidget::restoreHeight()
{
	if (m_area_docks.isEmpty() || m_restore_pending)
		return;

	m_restore_pending = true;
		// Once the layout has given the dock its maximum height back
	QTimer::singleShot(0, this, [this]() {
		m_restore_pending = false;
		auto main_window = qobject_cast<QMainWindow *>(parentWidget());
		if (!main_window || isFloating() || !isVisible()
			|| m_editor_list.isEmpty() || isTabbed())
			return;

		QList<QDockWidget *> docks;
		QList<int> heights;
		const Qt::DockWidgetArea area = main_window->dockWidgetArea(this);
		for (int i = 0 ; i < m_area_docks.size() ; ++i)
		{
			QDockWidget *dock = m_area_docks.at(i);
				// Closed, moved or deleted since
			if (dock && !dock->isFloating() && dock->isVisible()
				&& main_window->dockWidgetArea(dock) == area)
			{
				docks << dock;
				heights << m_area_heights.at(i);
			}
		}
		if (docks.contains(this))
			main_window->resizeDocks(docks, heights, Qt::Vertical);
	});
}

/**
	@brief PropertiesEditorDockWidget::updateSizeConstraint
	The empty dock shrinks to its title bar, but not while it shares a tab
	stack with other docks: the stack takes the smallest maximum height of
	its tabs, so the empty dock would hold every other tab at its title bar
	and the stack could not be resized.
*/
void PropertiesEditorDockWidget::updateSizeConstraint()
{
	const bool tabbed = isTabbed();
	fitScrollArea();
	const QLayout::SizeConstraint constraint = tabbed
			? QLayout::SetDefaultConstraint
			: QLayout::SetMaximumSize;
	if (ui->verticalLayout->sizeConstraint() == constraint)
		return;

	ui->verticalLayout->setSizeConstraint(constraint);
		// SetMaximumSize left its maximum on the contents
	if (tabbed)
		ui->dockWidgetContents->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

/**
	@brief PropertiesEditorDockWidget::eventFilter
	Fit the scroll area again whenever the editors' layout changes: an
	editor came or went, or one shows or hides a part of itself.
*/
bool PropertiesEditorDockWidget::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == ui->m_scroll_contents && event->type() == QEvent::LayoutRequest)
		fitScrollArea();
		// The wheel over a field that is not being edited scrolls the
		// dock, rather than changing the field's value under the mouse
	if (event->type() == QEvent::Wheel && watched != ui->m_scroll_contents) {
		auto *field = qobject_cast<QWidget *>(watched);
		if (field && !field->hasFocus()) {
			QCoreApplication::sendEvent(ui->m_scroll_area->verticalScrollBar(), event);
			return true;
		}
	}
	return QDockWidget::eventFilter(watched, event);
}

/**
	@brief PropertiesEditorDockWidget::fitScrollArea
	The scroll area is no higher than the editors in it, so the dock still
	shrinks to them, and to its title bar when it has none; it may be lower,
	and then scrolls. Its width follows the editors', the scroll bar
	included, as the editors do not scroll sideways.
*/
void PropertiesEditorDockWidget::fitScrollArea()
{
	const QSize hint = ui->m_scroll_contents->sizeHint();
		// An editor that grows with the dock (the wire editor, #500) or
		// has a list of its own that scrolls would be held to its size
		// hint by a maximum: none then
	bool grows = false;
	for (QWidget *editor : std::as_const(m_editor_list))
		if ((editor->sizePolicy().verticalPolicy() & QSizePolicy::ExpandFlag)
		    || editor->findChild<QAbstractScrollArea *>())
			grows = true;
		// Tabbed, the stack takes the smallest maximum height of its tabs
		// (see updateSizeConstraint()): no maximum here then either
	ui->m_scroll_area->setMaximumHeight(isTabbed() || grows
			? QWIDGETSIZE_MAX
			: m_editor_list.isEmpty() ? 0 : hint.height());
	for (QWidget *field : ui->m_scroll_contents->findChildren<QWidget *>())
		if ((qobject_cast<QAbstractSpinBox *>(field) || qobject_cast<QComboBox *>(field)
		     || qobject_cast<QAbstractSlider *>(field))
		    && !field->property("qetWheelWatched").toBool()) {
			field->setProperty("qetWheelWatched", true);
			field->setFocusPolicy(Qt::StrongFocus);
			field->installEventFilter(this);
		}
	ui->m_scroll_area->setMinimumWidth(
				ui->m_scroll_contents->minimumSizeHint().width()
				+ ui->m_scroll_area->verticalScrollBar()->sizeHint().width());
}

/**
	@brief PropertiesEditorDockWidget::isTabbed
	@return true if this dock shares a tab stack with other docks
*/
bool PropertiesEditorDockWidget::isTabbed() const
{
	auto main_window = qobject_cast<QMainWindow *>(parentWidget());
	return main_window && !isFloating()
			&& !main_window->tabifiedDockWidgets(
				const_cast<PropertiesEditorDockWidget *>(this)).isEmpty();
}
