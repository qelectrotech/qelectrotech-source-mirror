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
#ifndef ELEMENTS_PANEL_WIDGET_H
#define ELEMENTS_PANEL_WIDGET_H
#include <QtWidgets>
#include "elementspanel.h"
/**
	This class embeds an elements panel under a toolbar providing various actions
	to manage elements.
	@see ElementsPanel
*/
class ElementsPanelWidget : public QWidget {
	Q_OBJECT
	
	// constructors, destructor
	public:
	ElementsPanelWidget(QWidget * = 0);
	virtual ~ElementsPanelWidget();
	
	private:
	ElementsPanelWidget(const ElementsPanelWidget &);
	
	// attributes
	private:
	ElementsPanel *elements_panel;
	QToolBar *toolbar;
	QAction *open_directory, *copy_path;
	QAction *reload;
	QAction *new_category, *edit_category, *delete_category;
	QAction *delete_collection;
	QAction *new_element, *edit_element, *delete_element, *open_element;
	QAction *prj_activate, *prj_close, *prj_edit_prop, *prj_prop_diagram, *prj_add_diagram, *prj_del_diagram, *prj_move_diagram_up, *prj_move_diagram_top, *prj_move_diagram_down, *prj_move_diagram_upx10, *prj_move_diagram_downx10;
	QAction *tbt_add, *tbt_edit, *tbt_remove;
	QAction *copy_elements_, *move_elements_, *cancel_elements_;
	QMenu *context_menu;
	QLineEdit *filter_textfield;
	ElementsCollectionItem *dnd_item_src_, *dnd_item_dst_;
	QProgressBar *progress_bar_;
	
	// methods
	public:
	inline ElementsPanel &elementsPanel() const;
	
	signals:
	void requestForDiagram(Diagram *);
	void requestForProject(QETProject *);
	void requestForNewDiagram(QETProject *);
	void requestForProjectClosing(QETProject *);
	void requestForProjectPropertiesEdition(QETProject *);
	void requestForDiagramPropertiesEdition(Diagram *);
	void requestForDiagramDeletion(Diagram *);
	void requestForDiagramMoveUp(Diagram *);
	void requestForDiagramMoveDown(Diagram *);
	void requestForDiagramMoveUpTop(Diagram *);
	void requestForDiagramMoveUpx10(Diagram *);
	void requestForDiagramMoveDownx10(Diagram *);
	
	public slots:
	void openDirectoryForSelectedItem();
	void copyPathForSelectedItem();
	void reloadAndFilter();
	void activateProject();
	void closeProject();
	void editProjectProperties();
	void editDiagramProperties();
	void newDiagram();
	void deleteDiagram();
	void moveDiagramUp();
	void moveDiagramDown();
	void moveDiagramUpTop();
	void moveDiagramUpx10();
	void moveDiagramDownx10();
	void addTitleBlockTemplate();
	void editTitleBlockTemplate();
	void removeTitleBlockTemplate();
	void newCategory();
	void newElement();
	void openElementFromFile();
	void editCategory();
	void editElement();
	void deleteCategory();
	void deleteElement();
	void updateButtons();
	void setElementsActionEnabled(bool);
	int  launchCategoriesManager();
	void handleContextMenu(const QPoint &);
	void handleCollectionRequest(const ElementsLocation &);
	void handleMoveElementsRequest(ElementsCollectionItem *, ElementsCollectionItem *, const QPoint & = QPoint());
	void moveElements();
	void moveElements(ElementsCollectionItem *, ElementsCollectionItem *);
	void copyElements();
	void copyElements(ElementsCollectionItem *, ElementsCollectionItem *);
	void collectionsRead();
	void collectionsReadFinished();
	void updateProgressBar(int, int);
	void loadingFinished();
	void filterEdited(const QString &);
	
	private:
	void launchElementEditor(const ElementsLocation &);
	void launchCategoryEditor(const ElementsLocation &);
	ElementsCategory *writableSelectedCategory();
	QString previous_filter_;
};

/**
	@return The elements panel embedded within this widget.
*/
inline ElementsPanel &ElementsPanelWidget::elementsPanel() const {
	return(*elements_panel);
}

#endif
