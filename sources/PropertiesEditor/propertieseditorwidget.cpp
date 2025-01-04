/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "propertieseditorwidget.h"
#include <QUndoCommand>

/**
	@brief PropertiesEditorWidget::PropertiesEditorWidget
	Constructor
	@param parent : parent widget
*/
PropertiesEditorWidget::PropertiesEditorWidget(QWidget *parent) :
	QWidget(parent),
	m_live_edit(false)
{}

/**
	@brief PropertiesEditorWidget::associatedUndo
	By default, return a nullptr
	@return nullptr
*/
QUndoCommand *PropertiesEditorWidget::associatedUndo() const{
	return nullptr;
}

/**
	@brief PropertiesEditorWidget::title
	@return the title of this editor
*/
QString PropertiesEditorWidget::title() const
{
	return QString();
}

/**
	@brief PropertiesEditorWidget::setLiveEdit
	Set the editor in live edit mode.
	When an editor is in live edit mode,
	every change is applied immediately (no need to call apply).
	If live edit can be enabled, return true, or else false.
	By default this method does nothing and returns false
	(live edit is disabled).
	Inherited class of PropertiesEditorWidget must reimplement
	this method to manage the live edit mode.
	@param live_edit true to enable live edit
	@return true if live edit is enabled, else false.
*/
bool PropertiesEditorWidget::setLiveEdit(bool live_edit) {
	Q_UNUSED(live_edit);
	return false;
}

/**
	@brief PropertiesEditorWidget::isLiveEdit
	@return true if this editor is in live edit mode
	else return false.
*/
bool PropertiesEditorWidget::isLiveEdit() const
{
	return m_live_edit;
}
