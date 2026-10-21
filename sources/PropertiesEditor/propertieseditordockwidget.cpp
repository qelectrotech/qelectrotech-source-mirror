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

#include <QMainWindow>
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
			|| m_editor_list.isEmpty())
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
