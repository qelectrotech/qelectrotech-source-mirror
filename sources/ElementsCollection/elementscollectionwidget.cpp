/*
        Copyright 2006-2015 The QElectroTech Team
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
#include "elementscollectionwidget.h"
#include "elementscollectionmodel.h"
#include "elementcollectionitem.h"
#include "qeticons.h"
#include "fileelementcollectionitem.h"
#include "elementslocation.h"
#include "qetapp.h"
#include "qetmessagebox.h"
#include "elementscategoryeditor.h"
#include "newelementwizard.h"
#include "elementscategory.h"
#include "xmlprojectelementcollectionitem.h"
#include "qetproject.h"

#include <QVBoxLayout>
#include <QTreeView>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>

/**
 * @brief ElementsCollectionWidget::ElementsCollectionWidget
 * Default constructor.
 * @param parent : parent widget of this widget.
 */
ElementsCollectionWidget::ElementsCollectionWidget(QWidget *parent):
	QWidget(parent),
	m_model(nullptr)
{
	setUpWidget();
	setUpAction();
	setUpConnection();

	reload();
}

/**
 * @brief ElementsCollectionWidget::expandFirstItems
 * Expand each first item in the tree view
 */
void ElementsCollectionWidget::expandFirstItems()
{
	for (int i=0; i < m_model->rowCount() ; i++)
		showAndExpandItem(m_model->index(i, 0), false);
}

/**
 * @brief ElementsCollectionWidget::model
 * @return The ElementsCollectionModel used by the tree view
 */
ElementsCollectionModel *ElementsCollectionWidget::model() const {
	return m_model;
}

/**
 * @brief ElementsCollectionWidget::addProject
 * Add @project to be displayed
 * @param project
 */
void ElementsCollectionWidget::addProject(QETProject *project) {
	m_model->addProject(project);
}

void ElementsCollectionWidget::removeProject(QETProject *project) {
	m_model->removeProject(project);
}

void ElementsCollectionWidget::setUpAction()
{
	m_open_dir =       new QAction(QET::Icons::DocumentOpen,  tr("Ouvrir le dossier correspondant"), this);
	m_edit_element =   new QAction(QET::Icons::ElementEdit,   tr("Éditer l'élément"),                this);
	m_delete_element = new QAction(QET::Icons::ElementDelete, tr("Supprimer l'élément"),             this);
	m_delete_dir =     new QAction(QET::Icons::FolderDelete,  tr("Supprimer le dossier"),            this);
	m_reload =         new QAction(QET::Icons::ViewRefresh,   tr("Recharger les collections"),       this);
	m_edit_dir =       new QAction(QET::Icons::FolderEdit,    tr("Éditer le dossier"),               this);
	m_new_directory =  new QAction(QET::Icons::FolderNew,     tr("Nouveau dossier"),                 this);
	m_new_element =    new QAction(QET::Icons::ElementNew,    tr("Nouvel élément"),                  this);
	m_show_this_dir =  new QAction(QET::Icons::ZoomDraw,      tr("Afficher uniquement ce dossier"),  this);
	m_show_all_dir =   new QAction(QET::Icons::ZoomOriginal,  tr("Afficher tous les dossiers"),      this);
}

/**
 * @brief ElementsCollectionWidget::setUpWidget
 * Setup this widget
 */
void ElementsCollectionWidget::setUpWidget()
{
		//Setup the main layout
	m_main_vlayout = new QVBoxLayout(this);
	this->setLayout(m_main_vlayout);

	m_search_field = new QLineEdit(this);
	m_search_field->setPlaceholderText(tr("Rechercher"));
	m_search_field->setClearButtonEnabled(true);
	m_main_vlayout->addWidget(m_search_field);

		//Setup the tree view
	m_tree_view = new QTreeView(this);
	m_tree_view->setHeaderHidden(true);
	m_tree_view->setIconSize(QSize(50, 50));
	m_tree_view->setDragDropMode(QAbstractItemView::DragDrop);
	m_tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
	m_tree_view->setAutoExpandDelay(500);
	m_tree_view->setAnimated(true);
	m_tree_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_main_vlayout->addWidget(m_tree_view);

		//Setup the progress bar
	m_progress_bar = new QProgressBar(this);
	m_progress_bar->setFormat(tr("Chargement") + " %p%");
	m_main_vlayout->addWidget(m_progress_bar);
	m_progress_bar->hide();

	m_context_menu = new QMenu(this);
}

/**
 * @brief ElementsCollectionWidget::setUpConnection
 * Setup the connection used in this widget
 */
void ElementsCollectionWidget::setUpConnection()
{
	connect(m_tree_view,      &QTreeView::customContextMenuRequested, this, &ElementsCollectionWidget::customContextMenu);
	connect(m_search_field,   &QLineEdit::textEdited, this, &ElementsCollectionWidget::search);
	connect(m_open_dir,       &QAction::triggered, this, &ElementsCollectionWidget::openDir);
	connect(m_edit_element,   &QAction::triggered, this, &ElementsCollectionWidget::editElement);
	connect(m_delete_element, &QAction::triggered, this, &ElementsCollectionWidget::deleteElement);
	connect(m_delete_dir,     &QAction::triggered, this, &ElementsCollectionWidget::deleteDirectory);
	connect(m_reload,         &QAction::triggered, this, &ElementsCollectionWidget::reload);
	connect(m_edit_dir,       &QAction::triggered, this, &ElementsCollectionWidget::editDirectory);
	connect(m_new_directory,  &QAction::triggered, this, &ElementsCollectionWidget::newDirectory);
	connect(m_new_element,    &QAction::triggered, this, &ElementsCollectionWidget::newElement);
	connect(m_show_this_dir,  &QAction::triggered, this, &ElementsCollectionWidget::showThisDir);
	connect(m_show_all_dir,   &QAction::triggered, this, &ElementsCollectionWidget::resetShowThisDir);

	connect(m_tree_view, &QTreeView::doubleClicked, [this](const QModelIndex &index) {
		this->m_index_at_context_menu = index ;
		this->editElement();});
}

/**
 * @brief ElementsCollectionWidget::customContextMenu
 * Display the context menu of this widget at @point
 * @param point
 */
void ElementsCollectionWidget::customContextMenu(const QPoint &point)
{
	m_index_at_context_menu = m_tree_view->indexAt(point);
	if (!m_index_at_context_menu.isValid()) return;

	m_context_menu->clear();

	ElementCollectionItem *eci = elementCollectionItemForIndex(m_index_at_context_menu);
	bool add_open_dir = false;

	if (eci->isElement())
		m_context_menu->addAction(m_edit_element);

	if (eci->type() == FileElementCollectionItem::Type)
	{
		add_open_dir = true;
		FileElementCollectionItem *feci = static_cast<FileElementCollectionItem*>(eci);
		if (!feci->isCommonCollection())
		{
			if (feci->isDir())
			{
				m_context_menu->addAction(m_new_element);
				m_context_menu->addAction(m_new_directory);
				if (!feci->isCollectionRoot())
				{
					m_context_menu->addAction(m_edit_dir);
					m_context_menu->addAction(m_delete_dir);
				}
			}
			else
				m_context_menu->addAction(m_delete_element);
		}
	}
	if (eci->type() == XmlProjectElementCollectionItem::Type)
	{
		XmlProjectElementCollectionItem *xpeci = static_cast<XmlProjectElementCollectionItem *>(eci);
		if (xpeci->isCollectionRoot())
			add_open_dir = true;
	}

	m_context_menu->addSeparator();
	if (eci->isDir())
	{
		m_context_menu->addAction(m_show_this_dir);
			//there is a current filtered dir, add entry to reset it
		if (m_showed_index.isValid())
			m_context_menu->addAction(m_show_all_dir);
	}
	if (add_open_dir)
		m_context_menu->addAction(m_open_dir);
	m_context_menu->addAction(m_reload);

	m_context_menu->popup(mapToGlobal(m_tree_view->mapToParent(point)));
}

/**
 * @brief ElementsCollectionWidget::openDir
 * Open the directory represented by the current selected item
 */
void ElementsCollectionWidget::openDir()
{
	ElementCollectionItem *eci = elementCollectionItemForIndex(m_index_at_context_menu);
	if (!eci) return;

	if (eci->type() == FileElementCollectionItem::Type)
		QDesktopServices::openUrl(static_cast<FileElementCollectionItem*>(eci)->dirPath());
	else if (eci->type() == XmlProjectElementCollectionItem::Type)
		QDesktopServices::openUrl(static_cast<XmlProjectElementCollectionItem*>(eci)->project()->currentDir());
}

/**
 * @brief ElementsCollectionWidget::editElement
 * Edit the element represented by the current selected item
 */
void ElementsCollectionWidget::editElement()
{
	ElementCollectionItem *eci = elementCollectionItemForIndex(m_index_at_context_menu);

	if (!eci ||
		!eci->isElement() ||
		(eci->type() != FileElementCollectionItem::Type)) return;

	ElementsLocation location(static_cast<FileElementCollectionItem*>(eci)->collectionPath());
	QETApp::instance()->openElementLocations(QList<ElementsLocation>() << location);
}

/**
 * @brief ElementsCollectionWidget::deleteElement
 * Delete the element represented by the current selected item.
 */
void ElementsCollectionWidget::deleteElement()
{
	ElementCollectionItem *eci = elementCollectionItemForIndex(m_index_at_context_menu);

	if (!eci) return;
	if (!(eci->isElement() && eci->canRemoveContent())) return;

	if (QET::QetMessageBox::question(this,
									 tr("Supprimer l'élément ?", "message box title"),
									 tr("Êtes-vous sûr  de vouloir supprimer cet élément ?\n", "message box content"),
									 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
	{
		if (!eci->removeContent())
		{
			QET::QetMessageBox::warning(this,
										tr("Suppression de l'élément", "message box title"),
										tr("La suppression de l'élément a échoué.", "message box content"));
		}
		else
			m_model->removeRows(m_index_at_context_menu.row(), 1, m_index_at_context_menu.parent());
	}
}

/**
 * @brief ElementsCollectionWidget::deleteDirectory
 * Delete directory represented by the current selected item
 */
void ElementsCollectionWidget::deleteDirectory()
{
	ElementCollectionItem *eci = elementCollectionItemForIndex(m_index_at_context_menu);

	if (!eci) return;
	if (!(eci->isDir() && eci->canRemoveContent())) return;

	if (QET::QetMessageBox::question(this,
									 tr("Supprimer le dossier?", "message box title"),
									 tr("Êtes-vous sûr  de vouloir supprimer le dossier ?\n"
										"Tout les éléments et les dossier contenus dans ce dossier seront supprimés.",
										"message box content"),
									 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
	{
		if (!eci->removeContent())
		{
			QET::QetMessageBox::warning(this,
										tr("Suppression du dossier", "message box title"),
										tr("La suppression du dossier a échoué.", "message box content"));
		}
		else
			m_model->removeRows(m_index_at_context_menu.row(), 1, m_index_at_context_menu.parent());
	}
}

/**
 * @brief ElementsCollectionWidget::editDirectory
 * Edit the directory represented by the current selected item
 */
void ElementsCollectionWidget::editDirectory()
{
	ElementCollectionItem *eci = elementCollectionItemForIndex(m_index_at_context_menu);

	if (eci->type() != FileElementCollectionItem::Type) return;

	FileElementCollectionItem *feci = static_cast<FileElementCollectionItem*>(eci);
	if(feci->isCommonCollection()) return;

	ElementsLocation location(feci->collectionPath());
	ElementsCategoryEditor ece(location, true, this);
	if (ece.exec() == QDialog::Accepted)
		reload();
}

/**
 * @brief ElementsCollectionWidget::newDirectory
 * Create a new directory
 */
void ElementsCollectionWidget::newDirectory()
{
	ElementCollectionItem *eci = elementCollectionItemForIndex(m_index_at_context_menu);

	if (eci->type() != FileElementCollectionItem::Type) return;

	FileElementCollectionItem *feci = static_cast<FileElementCollectionItem*>(eci);
	if(feci->isCommonCollection()) return;

	ElementsLocation location(feci->collectionPath());
	ElementsCategoryEditor new_dir_editor(location, false, this);
	if (new_dir_editor.exec() == QDialog::Accepted)
		reload();;
}

/**
 * @brief ElementsCollectionWidget::newElement
 * Create a new element.
 */
void ElementsCollectionWidget::newElement()
{
	ElementCollectionItem *eci = elementCollectionItemForIndex(m_index_at_context_menu);

	if (eci->type() != FileElementCollectionItem::Type) return;

	FileElementCollectionItem *feci = static_cast<FileElementCollectionItem*>(eci);
	if(feci->isCommonCollection()) return;

	ElementsCollectionItem *category = QETApp::collectionItem(ElementsLocation(feci->collectionPath()), false);
	ElementsCategory *selected_category = category -> toCategory();
	if (!selected_category) return;

	NewElementWizard elmt_wizard(this);
	elmt_wizard.preselectCategory(selected_category);
	elmt_wizard.exec();
}

/**
 * @brief ElementsCollectionWidget::showThisDir
 * Hide all directories except the pointed dir;
 */
void ElementsCollectionWidget::showThisDir()
{
		//Disable the yellow background of the previous index
	if (m_showed_index.isValid())
	{
		ElementCollectionItem *eci = elementCollectionItemForIndex(m_showed_index);
		if (eci)
			eci->setBackgroundColor(Qt::yellow, false);
	}

	m_showed_index = m_index_at_context_menu;
	if (m_showed_index.isValid())
	{
		hideCollection(true);
		showAndExpandItem(m_showed_index, true, true);
		ElementCollectionItem *eci = elementCollectionItemForIndex(m_showed_index);
		if (eci)
			eci->setBackgroundColor(Qt::yellow, true);
		search(m_search_field->text());
	}
	else
		resetShowThisDir();
}

/**
 * @brief ElementsCollectionWidget::resetShowThisDir
 * reset show this dir, all collection are show.
 * If search field isn't empty, apply the search after show all collection
 */
void ElementsCollectionWidget::resetShowThisDir()
{
	if (m_showed_index.isValid())
	{
		ElementCollectionItem *eci = elementCollectionItemForIndex(m_showed_index);
		if (eci)
			eci->setBackgroundColor(Qt::yellow, false);
	}

	m_showed_index = QModelIndex();
	search(m_search_field->text());
}

/**
 * @brief ElementsCollectionWidget::reload, the displayed collections.
 */
void ElementsCollectionWidget::reload()
{
	m_progress_bar->show();
	ElementsCollectionModel *new_model = new ElementsCollectionModel(m_tree_view);
	new_model->addCommonCollection();
	new_model->addCustomCollection();

	if (m_model)
		foreach (QETProject *project, m_model->project())
			new_model->addProject(project);

	QList <ElementCollectionItem *> list = new_model->items();
	m_progress_bar->setMaximum(list.size());
	m_progress_bar->setValue(0);
	foreach (ElementCollectionItem *item, new_model->items())
	{
		item->name();
		m_progress_bar->setValue(m_progress_bar->value() + 1);
	}

	m_tree_view->setModel(new_model);
	if (m_model) delete m_model;
	m_model = new_model;
	expandFirstItems();
	m_progress_bar->hide();
}

/**
 * @brief ElementsCollectionWidget::search
 * Search every item (directory or element) that match the string @text
 * and display it, other item who does not match @text is hidden
 * @param text
 */
void ElementsCollectionWidget::search(const QString &text)
{
		//Reset the tree
	if (text.isEmpty())
	{
		QModelIndex current_index = m_tree_view->currentIndex();

		if (m_showed_index.isValid())
		{
			hideCollection(true);
			showAndExpandItem(m_showed_index, true, true);
		}
		else
		{
			m_tree_view->reset();
			expandFirstItems();
		}

			//Expand the tree and scroll to the last selected index
		if (current_index.isValid())
		{
			showAndExpandItem(current_index);
			m_tree_view->setCurrentIndex(current_index);
			m_tree_view->scrollTo(current_index);
		}
		return;
	}

	hideCollection(true);
	QModelIndexList match_index = m_model->match(m_showed_index.isValid() ? m_model->index(0,0,m_showed_index) : m_model->index(0,0),
												 Qt::DisplayRole, QVariant(text), -1, Qt::MatchContains | Qt::MatchRecursive);
	foreach(QModelIndex index, match_index)
		showAndExpandItem(index);
}

/**
 * @brief ElementsCollectionWidget::hideCollection
 * Hide all collection displayed in this tree
 * @param hide- true = hide , false = visible
 */
void ElementsCollectionWidget::hideCollection(bool hide)
{
	for (int i=0 ; i <m_model->rowCount() ; i++)
		hideItem(hide, m_model->index(i, 0), true);
}

/**
 * @brief ElementsCollectionWidget::hideItem
 * Hide the item @index. If @recursive is true, hide all subchilds of @index
 * @param hide- true = hide , false = visible
 * @param index- index to hide
 * @param recursive- true = apply to child , false = only for @index
 */
void ElementsCollectionWidget::hideItem(bool hide, const QModelIndex &index, bool recursive)
{
	m_tree_view->setRowHidden(index.row(), index.parent(), hide);

	if (recursive)
		for (int i=0 ; i<m_model->rowCount(index) ; i++)
			hideItem(hide, m_model->index(i, 0, index), recursive);
}

/**
 * @brief ElementsCollectionWidget::showAndExpandItem
 * Show the item @index and expand it.
 * If parent is true, ensure parents of @index is show and expanded
 * If child is true, ensure all childs of @index is show and expended
 * @param index- index to show
 * @param parent- Apply to parent
 * @param child- Apply to all childs
 */
void ElementsCollectionWidget::showAndExpandItem(const QModelIndex &index, bool parent, bool child)
{
	if (parent && index.isValid())
		showAndExpandItem(index.parent(), parent);

	hideItem(false, index, child);
	m_tree_view->expand(index);
}

/**
 * @brief ElementsCollectionWidget::elementCollectionItemForIndex
 * @param index
 * @return The internal pointer of index casted to ElementCollectionItem;
 */
ElementCollectionItem *ElementsCollectionWidget::elementCollectionItemForIndex(const QModelIndex &index) {
	return static_cast<ElementCollectionItem*>(index.internalPointer());
}
