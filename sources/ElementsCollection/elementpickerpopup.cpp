/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "elementpickerpopup.h"

#include "elementscollectionwidget.h"

#include <QAction>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QScreen>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include "../qetapp.h"
#include "../shortcutmanager.h"
#include "elementslocation.h"

	//The palette is read from disk each time the picker opens. It is meant as
	//a shortlist, and a large custom collection would otherwise make opening
	//slow and the grid unusable.
static const int max_palette_entries = 60;

/**
	@brief ElementPickerPopup::ElementPickerPopup
	@param source : the collection widget whose model the search runs against
	@param parent
*/
ElementPickerPopup::ElementPickerPopup(ElementsCollectionWidget *source,
				       QWidget *parent) :
	QFrame(parent, Qt::Popup),
	m_source(source)
{
	setFrameShape(QFrame::StyledPanel);
	setFrameShadow(QFrame::Raised);
	setMinimumWidth(340);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(6, 6, 6, 6);
	layout->setSpacing(4);

		//Command row, shown when the picker is opened as the shortcut bar
	m_commands = new QWidget(this);
	m_commands_layout = new QHBoxLayout(m_commands);
	m_commands_layout->setContentsMargins(0, 0, 0, 0);
	m_commands_layout->setSpacing(2);
	m_commands->hide();
		//SolidWorks: right-click the bar to customise it
	m_commands->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_commands, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
		QMenu menu;
		menu.addAction(tr("Personnaliser la barre…"), this, &ElementPickerPopup::startCustomising);
		menu.exec(m_commands->mapToGlobal(pos));
	});

		//Customising, shown instead of everything else. Two lists that
		//commands are dragged between: the bar, left to right, and the
		//commands that are not on it.
	m_editor = new QWidget(this);
	auto *editor_layout = new QVBoxLayout(m_editor);
	editor_layout->setContentsMargins(0, 0, 0, 0);
	auto *help = new QLabel(
		tr("Glissez les commandes dans la barre, hors de la barre, ou "
		   "d'une place à l'autre. Un double-clic fait passer une commande "
		   "d'une liste à l'autre."), m_editor);
	help->setWordWrap(true);
	m_edit_row = new QListWidget(m_editor);
	m_edit_row->setFlow(QListView::LeftToRight);
	m_edit_row->setWrapping(false);
	m_edit_row->setIconSize(QSize(24, 24));
	m_edit_row->setFixedHeight(44);
	m_edit_available = new QListWidget(m_editor);
	m_edit_available->setIconSize(QSize(20, 20));
	m_edit_available->setMinimumHeight(220);
	for (QListWidget *list : {m_edit_row, m_edit_available}) {
		list->setDragDropMode(QAbstractItemView::DragDrop);
		list->setDefaultDropAction(Qt::MoveAction);
		list->setSelectionMode(QAbstractItemView::SingleSelection);
	}
		//A drop copies the item as it was in the other list, so a command
		//dragged onto the bar would keep its text and one dragged off would
		//have none. Set the text for the list it has landed in.
		//Deferred: a drop inserts the row first and copies the item's data
		//into it afterwards.
	auto relabel = [this](QListWidget *list, bool icon_only) {
		return [this, list, icon_only]() {
			QTimer::singleShot(0, this, [list, icon_only]() {
				for (int i = 0 ; i < list->count() ; ++i) {
					QListWidgetItem *item = list->item(i);
					item->setText(icon_only && !item->icon().isNull()
						      ? QString() : item->toolTip());
				}
			});
		};
	};
	connect(m_edit_row->model(), &QAbstractItemModel::rowsInserted,
		this, relabel(m_edit_row, true));
	connect(m_edit_available->model(), &QAbstractItemModel::rowsInserted,
		this, relabel(m_edit_available, false));

	connect(m_edit_row, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
		const QString id = item->data(Qt::UserRole).toString();
		delete item;
		m_edit_available->addItem(commandItem(id, false));
	});
	connect(m_edit_available, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
		const QString id = item->data(Qt::UserRole).toString();
		delete item;
		m_edit_row->addItem(commandItem(id, true));
	});
	auto *defaults = new QPushButton(tr("Valeurs par défaut"), m_editor);
	auto *cancel = new QPushButton(tr("Annuler"), m_editor);
	auto *done = new QPushButton(tr("Terminé"), m_editor);
	done->setDefault(true);
	connect(defaults, &QPushButton::clicked, this, [this]() {
		fillCustomising(ShortcutBarSettings::defaultIds(m_context));
	});
	connect(cancel, &QPushButton::clicked, this, [this]() { finishCustomising(false); });
	connect(done, &QPushButton::clicked, this, [this]() { finishCustomising(true); });
	auto *editor_buttons = new QHBoxLayout();
	editor_buttons->addWidget(defaults);
	editor_buttons->addStretch();
	editor_buttons->addWidget(cancel);
	editor_buttons->addWidget(done);
	editor_layout->addWidget(help);
	editor_layout->addWidget(new QLabel(tr("Dans la barre :"), m_editor));
	editor_layout->addWidget(m_edit_row);
	editor_layout->addWidget(new QLabel(tr("Autres commandes :"), m_editor));
	editor_layout->addWidget(m_edit_available);
	editor_layout->addLayout(editor_buttons);
	m_editor->hide();

	m_search = new QLineEdit(this);
	m_search->setPlaceholderText(tr("Rechercher un élément…"));
	m_search->setClearButtonEnabled(true);

	m_model = new QStandardItemModel(this);
	m_view = new QListView(this);
	m_view->setModel(m_model);
	m_view->setIconSize(QSize(40, 40));
	m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_view->setMinimumHeight(260);

	m_hint = new QLabel(tr("Entrée pour insérer · Échap pour fermer"), this);
	m_hint->setEnabled(false);

	layout->addWidget(m_commands);
	layout->addWidget(m_search);
	layout->addWidget(m_view);
	layout->addWidget(m_hint);
	layout->addWidget(m_editor);

		//Search as you type, after a short idle: shorter than the dock's
		//500 ms, since the popup is opened and closed in a few seconds, but
		//the collection is large enough that every keystroke would show.
	auto *timer = new QTimer(this);
	timer->setSingleShot(true);
	timer->setInterval(300);
	connect(m_search, &QLineEdit::textChanged, this,
		[timer]() { timer->start(); });
	connect(timer, &QTimer::timeout, this, &ElementPickerPopup::runSearch);

	connect(m_view, &QListView::doubleClicked, this,
		[this](const QModelIndex &) { chooseCurrent(); });
}

/**
	@brief ElementPickerPopup::popUpAt
	Show the picker at @a global_pos, kept on screen, with the search field
	focused and any previous query cleared.
	@param global_pos
*/
void ElementPickerPopup::popUpAt(const QPoint &global_pos)
{
	m_bar_mode = false;
	setCommands({});
	show(global_pos);
}

/**
	@brief ElementPickerPopup::popUpShortcutBar
	Show the picker at @a global_pos with the shortcut bar's commands for
	@a context above it.
	@param global_pos
	@param context
*/
void ElementPickerPopup::popUpShortcutBar(const QPoint &global_pos,
					  ShortcutBarSettings::Context context)
{
	m_bar_mode = true;
	m_context = context;
	QList<QAction *> commands;
	for (const QString &id : ShortcutBarSettings::ids(context)) {
		if (QAction *action = commandAction(id)) {
			commands << action;
		}
	}
	setCommands(commands);
	show(global_pos);
}

/**
	@brief ElementPickerPopup::commandAction
	@return the action registered under @a id by the window this popup
	belongs to, or nullptr
*/
QAction *ElementPickerPopup::commandAction(const QString &id) const
{
	return ShortcutManager::instance().action(id, parentWidget());
}

/**
	@brief ElementPickerPopup::show
	Reset the search and show at @a global_pos, kept on screen.
*/
void ElementPickerPopup::show(const QPoint &global_pos)
{
	m_search->clear();
	m_model->clear();
	showPalette();

	adjustSize();
	QPoint pos = global_pos;

		//Keep it fully on the screen the cursor is on: opening at the cursor
		//near a right or bottom edge would otherwise push it off.
	if (QScreen *screen = QGuiApplication::screenAt(global_pos)) {
		const QRect avail = screen->availableGeometry();
		pos.setX(qBound(avail.left(),
				pos.x(), avail.right() - width()));
		pos.setY(qBound(avail.top(),
				pos.y(), avail.bottom() - height()));
	}

	move(pos);
	QFrame::show();
	m_search->setFocus();
}

/**
	@brief ElementPickerPopup::setCommands
	Show @a commands as a row of buttons above the search field, or hide the
	row when there are none. A disabled command keeps its place, greyed out,
	so the row looks the same every time for a given selection.

	Clicking a button closes the picker first, then triggers the action: a
	command such as "add a line" starts a mode on the folio, which needs the
	focus the popup holds.
	@param commands
*/
void ElementPickerPopup::setCommands(const QList<QAction *> &commands)
{
	while (QLayoutItem *item = m_commands_layout->takeAt(0)) {
		delete item->widget();
		delete item;
	}

	for (QAction *action : commands)
	{
		auto *button = new QToolButton(m_commands);
		button->setAutoRaise(true);
		button->setIconSize(QSize(24, 24));
		const QString text = action->text().remove(QLatin1Char('&'));
		if (action->icon().isNull()) {
			button->setText(text);
			button->setToolButtonStyle(Qt::ToolButtonTextOnly);
		} else {
			button->setIcon(action->icon());
		}
		const QKeySequence key = action->shortcut();
		button->setToolTip(key.isEmpty()
				   ? text
				   : QStringLiteral("%1 (%2)").arg(
					     text, key.toString(QKeySequence::NativeText)));
		button->setEnabled(action->isEnabled());
		button->setFocusPolicy(Qt::NoFocus);
		connect(button, &QToolButton::clicked, this, [this, action]() {
			hide();
			action->trigger();
		});
		m_commands_layout->addWidget(button);
	}
	m_commands_layout->addStretch();
	if (m_bar_mode)
	{
		auto *customise = new QToolButton(m_commands);
		customise->setAutoRaise(true);
		customise->setText(QStringLiteral("…"));
		customise->setToolTip(tr("Personnaliser la barre…"));
		customise->setFocusPolicy(Qt::NoFocus);
		connect(customise, &QToolButton::clicked, this, &ElementPickerPopup::startCustomising);
		m_commands_layout->addWidget(customise);
	}
		//An empty bar still shows, so it can be customised back
	m_commands->setVisible(m_bar_mode || !commands.isEmpty());
}

/**
	@brief ElementPickerPopup::commandItem
	@return a list item for command @a id: icon only, for the bar row, or
	icon and text, for the list of other commands. The id is in UserRole.
*/
QListWidgetItem *ElementPickerPopup::commandItem(const QString &id, bool icon_only) const
{
	QAction *action = commandAction(id);
	const QString text = action ? action->text().remove(QLatin1Char('&')) : id;
	auto *item = new QListWidgetItem();
	item->setData(Qt::UserRole, id);
	item->setToolTip(text);
	if (action && !action->icon().isNull()) {
		item->setIcon(action->icon());
	}
	if (!icon_only || item->icon().isNull()) {
		item->setText(text);
	}
	return item;
}

/**
	@brief ElementPickerPopup::startCustomising
	Turn the bar into a small window holding the two lists. A Qt::Popup
	closes as soon as the mouse is pressed outside it and keeps the mouse
	grab during a drag, so the window is re-shown as a tool window for the
	time of the edit.
*/
void ElementPickerPopup::startCustomising()
{
	const QPoint where = pos();
	m_customising = true;
	hide();
	setWindowFlags(Qt::Tool);
	setWindowTitle(tr("Personnaliser la barre de raccourcis : %1")
		       .arg(ShortcutBarSettings::title(m_context)));

	fillCustomising(ShortcutBarSettings::ids(m_context));
	m_commands->hide();
	setPickerVisible(false);
	m_editor->show();

	adjustSize();
	move(where);
	QFrame::show();
	activateWindow();
}

/**
	@brief ElementPickerPopup::fillCustomising
	Put @a ids on the bar row, and every other available command in the
	list below it.
*/
void ElementPickerPopup::fillCustomising(const QStringList &ids)
{
	m_edit_row->clear();
	m_edit_available->clear();
	for (const QString &id : ids) {
		m_edit_row->addItem(commandItem(id, true));
	}
	for (const QString &id : ShortcutBarSettings::availableIds()) {
		if (!ids.contains(id)) {
			m_edit_available->addItem(commandItem(id, false));
		}
	}
	m_edit_available->sortItems();
}

/**
	@brief ElementPickerPopup::finishCustomising
	Leave the edit, saving the bar row when @a save, and show the bar again
	where it was so the result can be seen and used straight away.
*/
void ElementPickerPopup::finishCustomising(bool save)
{
	if (save)
	{
		QStringList ids;
		for (int i = 0 ; i < m_edit_row->count() ; ++i) {
			ids << m_edit_row->item(i)->data(Qt::UserRole).toString();
		}
		ShortcutBarSettings::setIds(m_context, ids);
	}

	const QPoint where = pos();
	m_customising = false;
	hide();
	m_editor->hide();
	setPickerVisible(true);
	setWindowFlags(Qt::Popup);
	setWindowTitle(QString());
	popUpShortcutBar(where, m_context);
}

/**
	@brief ElementPickerPopup::closeEvent
	Closing the customising window from its title bar cancels the edit,
	like the Annuler button, rather than leaving the bar half-edited.
*/
void ElementPickerPopup::closeEvent(QCloseEvent *event)
{
	if (m_customising) {
		event->ignore();
		finishCustomising(false);
		return;
	}
	QFrame::closeEvent(event);
}

/**
	@brief ElementPickerPopup::setPickerVisible
	Show or hide the search field, the results and the hint line.
*/
void ElementPickerPopup::setPickerVisible(bool visible)
{
	m_search->setVisible(visible);
	m_view->setVisible(visible);
	m_hint->setVisible(visible);
}

/**
	@brief ElementPickerPopup::runSearch
*/
void ElementPickerPopup::runSearch()
{
	m_model->clear();
	if (!m_source) {
		return;
	}

	if (m_search->text().isEmpty()) {
		showPalette();
		return;
	}

	m_palette_mode = false;
	m_view->setViewMode(QListView::ListMode);
	m_view->setGridSize(QSize());
	m_view->setIconSize(QSize(40, 40));

	const QVector<ElementSearchHit> hits = m_source->rankedSearch(m_search->text());
	for (const ElementSearchHit &hit : hits)
	{
		auto *item = new QStandardItem(hit.name);
		item->setIcon(hit.icon);
		item->setEditable(false);
		item->setToolTip(hit.folder);
		item->setData(hit.path, Qt::UserRole + 2);
		m_model->appendRow(item);
	}

		//Preselect the best hit so Enter works straight from the search field
		//without arrowing down first -- that is the whole point of the popup.
	if (m_model->rowCount()) {
		m_view->setCurrentIndex(m_model->index(0, 0));
	}
	m_hint->setText(hits.isEmpty()
			? tr("Aucun résultat")
			: tr("Entrée pour insérer · Échap pour fermer"));
}

/**
	@brief ElementPickerPopup::chooseCurrent
	Emit the highlighted element and close.
*/
void ElementPickerPopup::chooseCurrent()
{
	const QModelIndex index = m_view->currentIndex();
	if (!index.isValid()) {
		return;
	}
	const QString path = index.data(Qt::UserRole + 2).toString();
	if (path.isEmpty()) {
		return;
	}

	ElementsLocation location(path);
	if (!location.exist()) {
		return;
	}

		//Close before emitting: placement mode wants the focus, and a popup
		//still up would keep the grab.
	hide();
	emit elementChosen(location);
}

/**
	@brief ElementPickerPopup::keyPressEvent
	Drive the list from the search field, so the hands never leave the
	keyboard: Up/Down move the selection, Enter places, Esc closes.
	@param event
*/
void ElementPickerPopup::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
		case Qt::Key_Escape:
			if (m_customising) {
				finishCustomising(false);
			} else {
				hide();
			}
			return;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			if (m_customising) {
				finishCustomising(true);
			} else {
				chooseCurrent();
			}
			return;
		case Qt::Key_Down:
		case Qt::Key_Up:
		case Qt::Key_PageDown:
		case Qt::Key_PageUp:
			if (!m_customising && m_model->rowCount()) {
					//Forwarded rather than focus-switched, so typing carries
					//on going to the search field.
				QCoreApplication::sendEvent(m_view, event);
				return;
			}
			break;
		default:
			break;
	}
	QFrame::keyPressEvent(event);
}

/**
	@brief ElementPickerPopup::showPalette
	With an empty search field, show the quick palette as an icon grid.

	The palette is a folder, not a config file. QET already has a user-owned,
	drag-populated, icon-rendering collection -- so a palette is just a
	directory whose subfolders are categories and whose contents are entries.
	That gives set-up (drag into the folder), ordering (the 01_/02_ filename
	convention the shipped collection already uses), team sharing
	(companyElementsDir) and version control for free, with no schema, parser,
	merge rules or "reset to defaults" to build or maintain.

	Only the path lives in QSettings, defaulting to the user's custom
	collection.
*/
void ElementPickerPopup::showPalette()
{
	m_palette_mode = true;
	m_model->clear();

	m_view->setViewMode(QListView::IconMode);
	m_view->setIconSize(QSize(48, 48));
	m_view->setGridSize(QSize(92, 84));
	m_view->setResizeMode(QListView::Adjust);
	m_view->setWordWrap(true);
	m_view->setMovement(QListView::Static);

	QSettings settings;
	const QString path = settings.value(
		QStringLiteral("elementscollection/palette-path"),
		QETApp::customElementsDir()).toString();

	const int count = loadPaletteDir(path, QString(), 0);

	if (!count) {
		m_hint->setText(
			tr("Palette vide — glissez des éléments dans votre collection "
			   "personnelle, ou tapez pour rechercher"));
	} else {
		m_hint->setText(tr("Entrée pour insérer · Échap pour fermer"));
		m_view->setCurrentIndex(m_model->index(0, 0));
	}
}

/**
	@brief ElementPickerPopup::loadPaletteDir
	Add every .elmt under @a dir_path to the grid, recursing into subfolders.
	@param dir_path
	@param prefix : folder path so far, shown as the entry's tooltip
	@param depth : recursion guard -- a palette is a shortlist, and the whole
	shipped collection would not be usable as a grid anyway
	@return how many entries were added
*/
int ElementPickerPopup::loadPaletteDir(const QString &dir_path,
				       const QString &prefix, int depth)
{
	if (depth > 3 || m_model->rowCount() >= max_palette_entries) {
		return 0;
	}
	QDir dir(dir_path);
	if (!dir.exists()) {
		return 0;
	}

	int added = 0;
		//Sorted by name, which is what makes the 01_/02_ filename convention
		//work as the ordering mechanism.
	const QFileInfoList entries = dir.entryInfoList(
		QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

	for (const QFileInfo &info : entries)
	{
		if (m_model->rowCount() >= max_palette_entries) {
			break;
		}
		if (info.isDir()) {
			added += loadPaletteDir(info.absoluteFilePath(),
						prefix.isEmpty()
							? info.fileName()
							: prefix + QStringLiteral(" / ")
								+ info.fileName(),
						depth + 1);
			continue;
		}
		if (info.suffix().compare(QStringLiteral("elmt"),
					  Qt::CaseInsensitive) != 0) {
			continue;
		}

		ElementsLocation location(info.absoluteFilePath());
		if (!location.exist()) {
			continue;
		}

		auto *item = new QStandardItem(location.name().isEmpty()
						       ? info.completeBaseName()
						       : location.name());
		item->setIcon(location.icon());
		item->setEditable(false);
		item->setToolTip(prefix.isEmpty() ? info.completeBaseName() : prefix);
		item->setData(info.absoluteFilePath(), Qt::UserRole + 2);
		item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
		m_model->appendRow(item);
		++added;
	}
	return added;
}
