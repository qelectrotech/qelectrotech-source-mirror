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
#include "diagramtoolbarsettings.h"

#include "shortcutmanager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QSettings>
#include <QToolBar>

namespace {
	const QString CONTENTS = QStringLiteral("diagrameditor/toolbars/");
	const QString CUSTOM_NAMES  = QStringLiteral("diagrameditor/custom_toolbars/names");
	const QString CUSTOM_TITLES = QStringLiteral("diagrameditor/custom_toolbars/titles");
		//Set on a window by markWindow()
	const char *const MARKED = "qetDiagramToolbars";
}

/**
	@return every toolbar, built-in first, then the user's own in the order
	they were added
*/
QList<DiagramToolbarSettings::Toolbar> DiagramToolbarSettings::toolbars()
{
	QList<Toolbar> list;
	for (const QString &name : builtInNames()) {
		list << Toolbar{name, builtInTitle(name), false};
	}
	return list << customToolbars();
}

/**
	@return the object names of the toolbars the diagram editor always has.
	The scripts toolbar is not one: it is filled from the scripts folder.
*/
QStringList DiagramToolbarSettings::builtInNames()
{
	return {QStringLiteral("toolbar"),
		QStringLiteral("display"),
		QStringLiteral("diagram"),
		QStringLiteral("adding"),
		QStringLiteral("diagram_depth_toolbar")};
}

QString DiagramToolbarSettings::builtInTitle(const QString &name)
{
	if (name == QLatin1String("toolbar"))
		return QCoreApplication::translate("QETDiagramEditor", "Tools");
	if (name == QLatin1String("display"))
		return QCoreApplication::translate("QETDiagramEditor", "Display");
	if (name == QLatin1String("diagram"))
		return QCoreApplication::translate("QETDiagramEditor", "Diagram");
	if (name == QLatin1String("adding"))
		return QCoreApplication::translate("QETDiagramEditor", "Add");
	if (name == QLatin1String("diagram_depth_toolbar"))
		return QCoreApplication::translate("QETDiagramEditor", "Depth", "toolbar title");
	return QString();
}

/**
	@return the toolbars the user added, in the order they were added
*/
QList<DiagramToolbarSettings::Toolbar> DiagramToolbarSettings::customToolbars()
{
	QSettings settings;
	const QStringList names = settings.value(CUSTOM_NAMES).toStringList();
	const QStringList titles = settings.value(CUSTOM_TITLES).toStringList();
	QList<Toolbar> list;
	for (int i = 0 ; i < names.size() ; ++i) {
		list << Toolbar{names.at(i), titles.value(i, names.at(i)), true};
	}
	return list;
}

/**
	@brief DiagramToolbarSettings::setCustomToolbars
	Store the user's own toolbars. The contents of one that is no longer
	in @a toolbars are removed with it.
*/
void DiagramToolbarSettings::setCustomToolbars(const QList<Toolbar> &toolbars)
{
	QStringList names, titles;
	for (const Toolbar &toolbar : toolbars) {
		names << toolbar.name;
		titles << toolbar.title;
	}
	for (const Toolbar &old : customToolbars()) {
		if (!names.contains(old.name)) {
			removeIds(old.name);
		}
	}
	QSettings settings;
	if (names.isEmpty()) {
		settings.remove(CUSTOM_NAMES);
		settings.remove(CUSTOM_TITLES);
	} else {
		settings.setValue(CUSTOM_NAMES, names);
		settings.setValue(CUSTOM_TITLES, titles);
	}
}

/**
	@return an object name for a new toolbar that none of @a existing has
	and none had before (stored positions stay with the toolbar they
	belonged to)
*/
QString DiagramToolbarSettings::newCustomName(const QList<Toolbar> &existing)
{
	QStringList taken;
	for (const Toolbar &toolbar : existing) taken << toolbar.name;
	for (const Toolbar &toolbar : customToolbars()) taken << toolbar.name;
	for (int i = 1 ; ; ++i) {
		const QString name = QStringLiteral("custom_%1").arg(i);
		if (!taken.contains(name)) return name;
	}
}

/**
	@return the ids toolbar @a name holds: the user's list if they saved
	one, the defaults otherwise. A saved empty list stays empty.
*/
QStringList DiagramToolbarSettings::ids(const QString &name)
{
	QSettings settings;
	const QString key = CONTENTS + name;
	if (!settings.contains(key)) {
		return defaultIds(name);
	}
	return settings.value(key).toStringList();
}

/**
	@return what toolbar @a name holds for a new user: the toolbars as
	they were built before they could be changed. A user's own toolbar
	starts empty.
*/
QStringList DiagramToolbarSettings::defaultIds(const QString &name)
{
	const QString sep = separatorId();
	if (name == QLatin1String("toolbar")) {
		return {QStringLiteral("diagrameditor.new_file"),
			QStringLiteral("diagrameditor.open_file"),
			QStringLiteral("diagrameditor.save_file"),
			QStringLiteral("diagrameditor.save_file_as"),
			QStringLiteral("diagrameditor.close_file"),
			QStringLiteral("diagrameditor.print"),
			QStringLiteral("diagrameditor.export_to_pdf"),
			sep,
			QStringLiteral("diagrameditor.undo"),
			QStringLiteral("diagrameditor.redo"),
			sep,
			QStringLiteral("diagrameditor.cut"),
			QStringLiteral("diagrameditor.copy"),
			QStringLiteral("diagrameditor.paste"),
			QStringLiteral("diagrameditor.duplicate"),
			sep,
			QStringLiteral("diagrameditor.delete_selection"),
			QStringLiteral("diagrameditor.rotate_selection")};
	}
	if (name == QLatin1String("display")) {
		return {QStringLiteral("diagrameditor.mode_selection"),
			QStringLiteral("diagrameditor.mode_visualise"),
			sep,
			QStringLiteral("widget:handler_size"),
			sep,
			QStringLiteral("diagrameditor.draw_grid"),
			QStringLiteral("widget:text_grid"),
			QStringLiteral("diagrameditor.draw_guides"),
			QStringLiteral("widget:background_color"),
			sep,
			QStringLiteral("diagrameditor.zoom_content"),
			QStringLiteral("diagrameditor.zoom_fit"),
			QStringLiteral("diagrameditor.zoom_reset")};
	}
	if (name == QLatin1String("diagram")) {
		return {QStringLiteral("diagrameditor.edit_diagram_properties"),
			QStringLiteral("diagrameditor.conductor_reset"),
			QStringLiteral("diagrameditor.auto_conductor"),
			QStringLiteral("diagrameditor.auto_break_conductor"),
			QStringLiteral("widget:conductor_color")};
	}
	if (name == QLatin1String("adding")) {
			//add_pdf is skipped when built without QtPdf, as before
		return {QStringLiteral("diagrameditor.add_text"),
			QStringLiteral("diagrameditor.add_image"),
			QStringLiteral("diagrameditor.add_pdf"),
			QStringLiteral("diagrameditor.add_line"),
			QStringLiteral("diagrameditor.add_rectangle"),
			QStringLiteral("diagrameditor.add_ellipse"),
			QStringLiteral("diagrameditor.add_arc"),
			QStringLiteral("diagrameditor.add_polyline"),
			QStringLiteral("diagrameditor.add_path"),
			QStringLiteral("diagrameditor.add_fillet"),
			QStringLiteral("diagrameditor.add_terminal_strip"),
			QStringLiteral("diagrameditor.add_generic_device")};
	}
	if (name == QLatin1String("diagram_depth_toolbar")) {
		return {QStringLiteral("depth.forward"),
			QStringLiteral("depth.raise"),
			QStringLiteral("depth.lower"),
			QStringLiteral("depth.backward")};
	}
	return {};
}

/**
	@brief DiagramToolbarSettings::setIds
	Save @a ids for toolbar @a name. Saving the defaults removes the key, so
	a later change of defaults still reaches this user.
*/
void DiagramToolbarSettings::setIds(const QString &name, const QStringList &ids)
{
	if (ids == defaultIds(name)) {
		removeIds(name);
	} else {
		QSettings().setValue(CONTENTS + name, ids);
	}
}

void DiagramToolbarSettings::removeIds(const QString &name)
{
	QSettings().remove(CONTENTS + name);
}

QString DiagramToolbarSettings::separatorId()
{
	return QStringLiteral("separator");
}

/**
	@return the toolbar buttons that are widgets, not commands. Each can be
	on one toolbar at a time.
*/
QStringList DiagramToolbarSettings::widgetIds()
{
	return {QStringLiteral("widget:handler_size"),
		QStringLiteral("widget:text_grid"),
		QStringLiteral("widget:background_color"),
		QStringLiteral("widget:conductor_color")};
}

bool DiagramToolbarSettings::isWidget(const QString &id)
{
	return id.startsWith(QLatin1String("widget:"));
}

QString DiagramToolbarSettings::widgetTitle(const QString &id)
{
	if (id == QLatin1String("widget:handler_size"))
		return QCoreApplication::translate("DiagramToolbarSettings", "Handle size");
	if (id == QLatin1String("widget:text_grid"))
		return QCoreApplication::translate("DiagramToolbarSettings", "Text grid");
	if (id == QLatin1String("widget:background_color"))
		return QCoreApplication::translate("DiagramToolbarSettings", "Sheet background color");
	if (id == QLatin1String("widget:conductor_color"))
		return QCoreApplication::translate("DiagramToolbarSettings", "Conductor color");
	return id;
}

/**
	@return every command that can go on a toolbar: the diagram editor's,
	its stored scripts', and the depth commands it shares with the element
	editor
*/
QStringList DiagramToolbarSettings::availableCommandIds()
{
	QStringList ids;
	for (const ShortcutManager::ShortcutInfo &info :
	     ShortcutManager::instance().allShortcuts())
	{
		if (info.id.startsWith(QLatin1String("diagrameditor."))
		    || info.id.startsWith(QLatin1String("depth."))) {
			ids << info.id;
		}
	}
	return ids;
}

/**
	@brief DiagramToolbarSettings::fill
	Empty @a toolbar and put the commands of @a ids on it, in order.
	@a resolve gives the window's action for an id, a widget id included;
	an id it has no action for is skipped. A separator never starts or
	ends the toolbar and never follows another: those are left out, as
	Qt would otherwise draw them.
*/
void DiagramToolbarSettings::fill(QToolBar *toolbar, const QStringList &ids,
				  const std::function<QAction *(const QString &)> &resolve)
{
		//Separators belong to the toolbar that made them; commands do not
	const QList<QAction *> old_actions = toolbar->actions();
	toolbar->clear();
	for (QAction *action : old_actions) {
		if (action->isSeparator() && action->parent() == toolbar) delete action;
	}
	bool pending_separator = false;
	for (const QString &id : ids)
	{
		if (id == separatorId()) {
			pending_separator = !toolbar->actions().isEmpty();
			continue;
		}
		QAction *action = resolve(id);
		if (!action) {
			continue;
		}
		if (pending_separator) {
			toolbar->addSeparator();
			pending_separator = false;
		}
		toolbar->addAction(action);
	}
}

/**
	@brief DiagramToolbarSettings::markWindow
	Mark @a window as one whose toolbars applyToAll() rebuilds. It must
	have a rebuildToolBars() slot or invokable.
*/
void DiagramToolbarSettings::markWindow(QObject *window)
{
	window->setProperty(MARKED, true);
}

/**
	@brief DiagramToolbarSettings::applyToAll
	Rebuild the toolbars of every open window marked by markWindow(),
	after the settings change.
*/
void DiagramToolbarSettings::applyToAll()
{
	for (QWidget *widget : QApplication::topLevelWidgets()) {
		if (widget->property(MARKED).toBool()) {
			QMetaObject::invokeMethod(widget, "rebuildToolBars");
		}
	}
}
