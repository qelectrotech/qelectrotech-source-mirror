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
#ifndef DIAGRAMTOOLBARSETTINGS_H
#define DIAGRAMTOOLBARSETTINGS_H

#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

class QAction;
class QToolBar;

/**
	@brief What each of the diagram editor's toolbars holds.

	A toolbar is a list of ids, in order: ShortcutManager command ids, the
	separator id, and the ids of the few toolbar buttons that are widgets
	rather than commands (the colour buttons, the text grid, the handle
	size). Stored in QSettings, one key per toolbar, by the toolbar's
	object name; a toolbar the user never changed uses the defaults below,
	which are the toolbars as they were before they could be changed.

	The user can also add toolbars of their own. They start empty.

	An id no command carries any more is skipped, so a list saved by
	another version still opens.
*/
class DiagramToolbarSettings
{
	public:
		struct Toolbar {
			QString name;   ///< object name, used for storage and by QMainWindow::saveState()
			QString title;  ///< shown to the user
			bool custom = false;
		};

		static QList<Toolbar> toolbars();
		static QStringList builtInNames();
		static QString builtInTitle(const QString &name);
		static QList<Toolbar> customToolbars();
		static void setCustomToolbars(const QList<Toolbar> &toolbars);
		static QString newCustomName(const QList<Toolbar> &existing);

		static QStringList ids(const QString &name);
		static QStringList defaultIds(const QString &name);
		static void setIds(const QString &name, const QStringList &ids);
		static void removeIds(const QString &name);

		static QString separatorId();
		static QStringList widgetIds();
		static bool isWidget(const QString &id);
		static QString widgetTitle(const QString &id);
		static QStringList availableCommandIds();

		static void fill(QToolBar *toolbar, const QStringList &ids,
				 const std::function<QAction *(const QString &)> &resolve);

		static void markWindow(QObject *window);
		static void applyToAll();
};

#endif // DIAGRAMTOOLBARSETTINGS_H
