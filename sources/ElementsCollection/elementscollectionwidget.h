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
#ifndef ELEMENTSCOLLECTIONWIDGET_H
#define ELEMENTSCOLLECTIONWIDGET_H

#include "elementslocation.h"

#include <QWidget>
#include <QModelIndex>
#include <QTimer>
#include <QElapsedTimer>
#include <QScopedPointer>
#include <QTabWidget>
#include <QTreeView>

class ElementsCollectionModel;
class QVBoxLayout;
class QMenu;
class QLineEdit;
class ElementCollectionItem;
class QProgressBar;
class QETProject;
class ElementsTreeView;

/**
	@brief The ElementsCollectionWidget class
	This widget embedd a tree view that display the element collection (common, custom, embedded)
	and all action needed to use this widget.
	This is the element collection widget used in the diagram editor.
*/
class QListView;
class QStandardItemModel;

/**
	@brief One ranked hit from a collection search.
	Carries everything a result row needs, so a consumer does not have to hold
	a model index -- which matters for the picker popup, whose list is not
	backed by the collection model.
*/
struct ElementSearchHit
{
	QString path;    ///< collection path, enough to build an ElementsLocation
	QString name;    ///< display name
	QString folder;  ///< where it lives, for disambiguation
	QIcon icon;
};

class ElementsCollectionWidget : public QWidget
{
	Q_OBJECT

	public:
		ElementsCollectionWidget(QWidget *parent = nullptr);

		void expandFirstItems();

		void addProject (QETProject *project);
		void removeProject (QETProject *project);
		void highlightUnusedElement();
		void setCurrentLocation(const ElementsLocation &location);
		QVector<ElementSearchHit> rankedSearch(const QString &text);

	protected:
		void leaveEvent(QEvent *event) override;

	private:
		void setUpAction();
		void setUpWidget();
		void setUpConnection();
		void customContextMenu(const QPoint &point);
		void openDir();
		void editElement();
		void deleteElement();
		void deleteDirectory();
		void editDirectory();
		void newDirectory();
		void newElement();
		bool confirmEdzImportTerms();
		void importEdz();
		void showThisDir();
		void resetShowThisDir();
		void dirProperties();
		void search();
		void hideCollection(bool hide = true);
		void hideItem(bool hide, const QModelIndex &index = QModelIndex(), bool recursive = true);
		void showAndExpandItem (const QModelIndex &index, bool parent = true, bool child = false);
		ElementCollectionItem *elementCollectionItemForIndex (const QModelIndex &index);

	public slots:
		void reload();
		void loadingFinished();
		void insertCurrentElement();

	signals:
		/**
			Emitted when the user asks for an element to be placed on the
			current folio. Whoever hosts this widget decides which view
			receives it -- the dock is inside a diagram editor, but the
			picker popup is not, so the widget must not reach for an
			ancestor editor itself.
		*/
		void insertElementRequested(const ElementsLocation &location);

	private:
		void locationWasSaved(const ElementsLocation& location);
		void activateIndex(const QModelIndex &index);
		void showFlatResults(const QModelIndexList &matches, const QString &needle);
		void clearFlatResults();
		static int rankMatch(const QString &needle, const QString &name,
				     const QString &haystack);


	private:
		ElementsCollectionModel *m_model = nullptr;
		ElementsCollectionModel *m_new_model = nullptr;
		ElementsCollectionModel *m_macros_model = nullptr;
		QLineEdit *m_search_field;
		QTimer m_search_timer;
		ElementsTreeView *m_tree_view;
		ElementsTreeView *m_macros_tree_view = nullptr;
		QTabWidget *m_tab_widget = nullptr;
			/// Flat ranked results, shown in place of the tree while searching
		QListView *m_search_results = nullptr;
		QStandardItemModel *m_search_model = nullptr;
		QVBoxLayout *m_main_vlayout;
		QMenu *m_context_menu;
		QModelIndex m_index_at_context_menu;
		QModelIndex m_showed_index;
		QProgressBar *m_progress_bar;

		QAction *m_open_dir,
				*m_edit_element,
				*m_delete_element,
				*m_delete_dir,
				*m_reload,
				*m_edit_dir,
				*m_new_directory,
				*m_new_element,
				*m_import_edz,
				*m_show_this_dir,
				*m_show_all_dir,
				*m_dir_propertie;

		bool m_first_show = true;
		QList<QETProject *> m_waiting_project;
		QScopedPointer<QElapsedTimer> m_loading_timer;
};

#endif // ELEMENTSCOLLECTIONWIDGET_H
