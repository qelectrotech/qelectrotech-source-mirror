/*
	Copyright 2006-2017 The QElectroTech Team
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

/**
 * @brief PropertiesEditorDockWidget::PropertiesEditorDockWidget
 * Constructor
 * @param parent : parent widget
 */
PropertiesEditorDockWidget::PropertiesEditorDockWidget(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::PropertiesEditorDockWidget)
{
	ui->setupUi(this);
}

/**
 * @brief PropertiesEditorDockWidget::~PropertiesEditorDockWidget
 * Destructor
 */
PropertiesEditorDockWidget::~PropertiesEditorDockWidget()
{
	clear();
	delete ui;
}

/**
 * @brief PropertiesEditorDockWidget::clear
 * Remove all editor present in this dock and delete it.
 * They also disabled the button box at the bottom of this dock
 */
void PropertiesEditorDockWidget::clear()
{
	for (PropertiesEditorWidget *editor: m_editor_list)
	{
		m_editor_list.removeOne(editor);
		ui->m_main_vlayout->removeWidget(editor);
		editor->deleteLater();
	}

	m_editor_list.clear();
}

/**
 * @brief PropertiesEditorDockWidget::apply
 * Call the apply method for each editor present in this dock
 */
void PropertiesEditorDockWidget::apply()
{
	for (PropertiesEditorWidget *editor: m_editor_list)
		editor->apply();
}

/**
 * @brief PropertiesEditorDockWidget::reset
 * Call the reset method for each editor present in this widget
 */
void PropertiesEditorDockWidget::reset()
{
	for (PropertiesEditorWidget *editor: m_editor_list)
		editor->reset();
}

/**
 * @brief PropertiesEditorDockWidget::addEditor
 * Add an @editor in this dock at @index in the main vertical layout (note the button box
 * are displayed at bottom of this layout by default)
 * When an editor is added, we enable the button box
 * @param editor : editor to add;
 * @param index : index of editor in the layout
 * @return true if was added (or already add) or false if can't be add (editor = nullptr)
 */
bool PropertiesEditorDockWidget::addEditor(PropertiesEditorWidget *editor, int index)
{
	if (!editor) return false;
	if (m_editor_list.contains(editor)) return true;

	ui -> m_main_vlayout -> insertWidget(index, editor);
	m_editor_list << editor;
	return true;
}

/**
 * @brief PropertiesEditorDockWidget::editors
 * @return all editor used in this dock
 */
QList<PropertiesEditorWidget *> PropertiesEditorDockWidget::editors() const {
	return m_editor_list;
}

/**
 * @brief PropertiesEditorDockWidget::removeEditor
 * Remove @editor from this dock. The editor wasn't delete a the end of this method
 * If the editor was the last on this widget, we disabled the button box
 * @param editor : editor to remove
 * @return true on success, else false
 */
bool PropertiesEditorDockWidget::removeEditor(PropertiesEditorWidget *editor)
{
	bool result = m_editor_list.removeOne(editor);
	if (result)
		ui -> m_main_vlayout -> removeWidget(editor);

	return result;
}
