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
	m_item_at_context_menu(nullptr)
{
	setUpWidget();
	setUpAction();
	setUpConnection();
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
	m_main_vlayout->addWidget(m_tree_view);

		//Setup the element collection model
	m_model = new ElementsCollectionModel(m_tree_view);
	m_model->addCommonCollection();
	m_model->addCustomCollection();
	m_tree_view->setModel(m_model);

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

	connect(m_tree_view, &QTreeView::doubleClicked, [this](const QModelIndex &index) {
		this->m_item_at_context_menu = static_cast<ElementCollectionItem*>(index.internalPointer());
		this->editElement();});
}

/**
 * @brief ElementsCollectionWidget::customContextMenu
 * Display the context menu of this widget at @point
 * @param point
 */
void ElementsCollectionWidget::customContextMenu(const QPoint &point)
{
	QModelIndex index = m_tree_view->indexAt(point);
	if (!index.isValid()) return;

	m_context_menu->clear();
	ElementCollectionItem *eci = static_cast<ElementCollectionItem*>(index.internalPointer());
	m_item_at_context_menu = eci;

	if (eci->isElement())
		m_context_menu->addAction(m_edit_element);

	if (eci->type() == FileElementCollectionItem::Type)
	{
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

	m_context_menu->addSeparator();
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
	ElementCollectionItem *eci = m_item_at_context_menu;
	m_item_at_context_menu = nullptr;

	if (!eci || (eci->type() != FileElementCollectionItem::Type)) return;

	QDesktopServices::openUrl(static_cast<FileElementCollectionItem*>(eci)->dirPath());
}

/**
 * @brief ElementsCollectionWidget::editElement
 * Edit the element represented by the current selected item
 */
void ElementsCollectionWidget::editElement()
{
	ElementCollectionItem *eci = m_item_at_context_menu;
	m_item_at_context_menu = nullptr;

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
	ElementCollectionItem *eci = m_item_at_context_menu;
	m_item_at_context_menu = nullptr;

	if (!eci ||
		!eci->isElement() ||
		(eci->type() != FileElementCollectionItem::Type)) return;


	FileElementCollectionItem *feci = static_cast<FileElementCollectionItem*>(eci);
		//We can't remove an element in the common collection
	if (feci->isCommonCollection()) return;

	if (QET::QetMessageBox::question(this,
									 tr("Supprimer l'élément ?", "message box title"),
									 tr("Êtes-vous sûr  de vouloir supprimer cet élément ?\n", "message box content"),
									 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
	{
		QFile file(feci->fileSystemPath());
		if (!file.remove())
		{
			QET::QetMessageBox::warning(this,
										tr("Suppression de l'élément", "message box title"),
										tr("La suppression de l'élément a échoué.", "message box content"));
		}
		else
			reload();
	}
}

/**
 * @brief ElementsCollectionWidget::deleteDirectory
 * Delete directory represented by the current selected item
 */
void ElementsCollectionWidget::deleteDirectory()
{
	ElementCollectionItem *eci = m_item_at_context_menu;
	m_item_at_context_menu = nullptr;

	if (!eci ||
		!eci->isDir() ||
		(eci->type() != FileElementCollectionItem::Type)) return;

	FileElementCollectionItem *feci = static_cast<FileElementCollectionItem*>(eci);
		//We can't remove directory int the common collection or remove the elements directory
	if (feci->isCommonCollection() || feci->isCollectionRoot()) return;

	if (QET::QetMessageBox::question(this,
									 tr("Supprimer le dossier?", "message box title"),
									 tr("Êtes-vous sûr  de vouloir supprimer le dossier ?\n"
										"Tout les éléments et les dossier contenus dans ce dossier seront supprimés.",
										"message box content"),
									 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
	{
		QDir dir(feci->fileSystemPath());
		if (!dir.removeRecursively())
		{
			QET::QetMessageBox::warning(this,
										tr("Suppression du dossier", "message box title"),
										tr("La suppression du dossier a échoué.", "message box content"));
		}
		else
			reload();
	}
}

/**
 * @brief ElementsCollectionWidget::editDirectory
 * Edit the directory represented by the current selected item
 */
void ElementsCollectionWidget::editDirectory()
{
	ElementCollectionItem *eci = m_item_at_context_menu;
	m_item_at_context_menu = nullptr;

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
	ElementCollectionItem *eci = m_item_at_context_menu;
	m_item_at_context_menu = nullptr;

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
	ElementCollectionItem *eci = m_item_at_context_menu;
	m_item_at_context_menu = nullptr;

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
 * @brief ElementsCollectionWidget::reload, the displayed collections.
 */
void ElementsCollectionWidget::reload()
{
	ElementsCollectionModel *new_model = new ElementsCollectionModel(m_tree_view);
	new_model->addCommonCollection();
	new_model->addCustomCollection();
	m_tree_view->setModel(new_model);
	delete m_model;
	m_model = new_model;
	expandFirstItems();
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
		m_tree_view->reset();
		expandFirstItems();

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
	QModelIndexList match_index = m_model->match(m_model->index(0,0), Qt::DisplayRole, QVariant(text), -1, Qt::MatchContains | Qt::MatchRecursive);
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
 * If recursive is true, ensure parents of @index is show and expanded
 * @param index- index to show
 * @param recursive- Apply to parent
 */
void ElementsCollectionWidget::showAndExpandItem(const QModelIndex &index, bool recursive)
{
	if (recursive && index.isValid())
		showAndExpandItem(index.parent(), recursive);

	m_tree_view->setRowHidden(index.row(), index.parent(), false);
	m_tree_view->expand(index);
}
