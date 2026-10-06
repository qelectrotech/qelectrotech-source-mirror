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
#include "toolbarsettings.h"

#include <QApplication>
#include <QMainWindow>
#include <QSettings>
#include <QToolBar>

namespace {
	const QString ICON_SIZE    = QStringLiteral("toolbars/icon_size");
	const QString BUTTON_STYLE = QStringLiteral("toolbars/button_style");
	const QString LOCKED       = QStringLiteral("toolbars/locked");
		//Set on a window by applyTo()
	const char *const APPLIED  = "qetToolbarSettings";
}

int ToolbarSettings::iconSize()
{
	return QSettings().value(ICON_SIZE, 0).toInt();
}

Qt::ToolButtonStyle ToolbarSettings::buttonStyle()
{
	const int style = QSettings().value(BUTTON_STYLE, int(Qt::ToolButtonIconOnly)).toInt();
	switch (style) {
		case Qt::ToolButtonTextBesideIcon:
		case Qt::ToolButtonTextUnderIcon:
			return Qt::ToolButtonStyle(style);
		default:
			return Qt::ToolButtonIconOnly;
	}
}

bool ToolbarSettings::locked()
{
	return QSettings().value(LOCKED, false).toBool();
}

/**
	@brief ToolbarSettings::save
	Store the three values; a default value removes its key.
*/
void ToolbarSettings::save(int icon_size, Qt::ToolButtonStyle style, bool locked)
{
	QSettings settings;
	if (icon_size > 0) settings.setValue(ICON_SIZE, icon_size);
	else settings.remove(ICON_SIZE);
	if (style != Qt::ToolButtonIconOnly) settings.setValue(BUTTON_STYLE, int(style));
	else settings.remove(BUTTON_STYLE);
	if (locked) settings.setValue(LOCKED, true);
	else settings.remove(LOCKED);
}

/**
	@brief ToolbarSettings::applyTo
	Apply the settings to \a window and its toolbars. The window passes
	its icon size and button style on to every toolbar that has none of
	its own; an invalid size gives the style's default back. The window
	is marked, so applyToAll() updates it again later.
*/
void ToolbarSettings::applyTo(QMainWindow *window)
{
	if (!window) {
		return;
	}
	window->setProperty(APPLIED, true);
	const int size = iconSize();
	window->setIconSize(size > 0 ? QSize(size, size) : QSize());
	window->setToolButtonStyle(buttonStyle());
	const bool movable = !locked();
	for (QToolBar *toolbar : window->findChildren<QToolBar *>()) {
			//Not a toolbar inside a panel: only the window's own
		if (window->toolBarArea(toolbar) != Qt::NoToolBarArea) {
			toolbar->setMovable(movable);
		}
	}
}

/**
	@brief ToolbarSettings::applyToAll
	Apply the settings again to every open window that took them with
	applyTo(), after they change. Other windows are left alone: they
	would lose the settings the next time they open.
*/
void ToolbarSettings::applyToAll()
{
	for (QWidget *widget : QApplication::topLevelWidgets()) {
		auto *window = qobject_cast<QMainWindow *>(widget);
		if (window && window->property(APPLIED).toBool()) {
			applyTo(window);
		}
	}
}
