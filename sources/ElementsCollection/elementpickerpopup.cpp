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

#include <QGuiApplication>
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
#include "elementslocation.h"

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

	layout->addWidget(m_search);
	layout->addWidget(m_view);
	layout->addWidget(m_hint);

		//Search as you type, on the same 500ms idle the dock uses -- the
		//collection is large enough that filtering on every keystroke is
		//noticeable.
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
	show();
	m_search->setFocus();
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
			hide();
			return;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			chooseCurrent();
			return;
		case Qt::Key_Down:
		case Qt::Key_Up:
		case Qt::Key_PageDown:
		case Qt::Key_PageUp:
			if (m_model->rowCount()) {
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
	if (depth > 3) {
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
