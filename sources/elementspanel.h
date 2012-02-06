/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef PANELAPPAREILS_H
#define PANELAPPAREILS_H
#include <QtGui>
#include "genericpanel.h"
#include "elementslocation.h"
#include "templatelocation.h"
class QETProject;
class Diagram;
class ElementsCollection;
class ElementsCollectionItem;
class ElementsCategory;
class ElementDefinition;
class ElementsCollectionCache;
class TitleBlockTemplatesFilesCollection;

/**
	Cette classe represente le panel d'appareils (en tant qu'element
	graphique) dans lequel l'utilisateur choisit les composants de
	son choix et les depose sur le schema par drag'n drop.
*/
class ElementsPanel : public GenericPanel {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	ElementsPanel(QWidget * = 0);
	virtual ~ElementsPanel();
	
	private:
	ElementsPanel(const ElementsPanel &);
	
	// methods
	public:
	// methodes pour determiner ce que represente un item donne
	bool itemIsWritable(QTreeWidgetItem *) const;
	bool selectedItemIsWritable() const;
	
	// methodes pour obtenir ce que represente un item donne
	ElementsCollectionItem *collectionItemForItem(QTreeWidgetItem *) const;
	ElementsCollectionItem *selectedItem() const;
	ElementsCategory *categoryForItem(QTreeWidgetItem *);
	ElementsCategory *categoryForPos(const QPoint &);
	
	void reloadCollections();
	int elementsCollectionItemsCount();
	
	signals:
	void requestForProject(QETProject *);
	void requestForDiagram(Diagram *);
	void requestForCollectionItem(const ElementsLocation &);
	void requestForMoveElements(ElementsCollectionItem *, ElementsCollectionItem *, QPoint);
	void requestForTitleBlockTemplate(const TitleBlockTemplateLocation &);
	void readingAboutToBegin();
	void readingFinished();
	void loadingProgressed(int, int);
	
	public slots:
	void slot_doubleClick(QTreeWidgetItem *, int);
	void reload(bool = false);
	void filter(const QString &, QET::Filtering = QET::RegularFilter);
	void projectWasOpened(QETProject *);
	void projectWasClosed(QETProject *);
	bool scrollToElement(const ElementsLocation &);
	
	protected:
	void dragEnterEvent(QDragEnterEvent *);
	void dragMoveEvent(QDragMoveEvent *);
	void dropEvent(QDropEvent *);
	void startDrag(Qt::DropActions);
	void startElementDrag(const ElementsLocation &);
	void startTitleBlockTemplateDrag(const TitleBlockTemplateLocation &);
	
	protected slots:
	void firstActivation();
	
	private:
	QTreeWidgetItem *addProject   (QETProject *);
	QTreeWidgetItem *addCollection(ElementsCollection *, const QString & = QString(), const QIcon & = QIcon());
	QTreeWidgetItem *updateTemplateItem        (QTreeWidgetItem *, const TitleBlockTemplateLocation &,  PanelOptions, bool = false);
	QTreeWidgetItem *updateElementsCategoryItem(QTreeWidgetItem *, ElementsCategory *,  PanelOptions, bool = false);
	QTreeWidgetItem *updateElementItem         (QTreeWidgetItem *, ElementDefinition *, PanelOptions, bool = false);
	
	void ensureHierarchyIsVisible(QList<QTreeWidgetItem *>);
	
	// attributes
	private:
	QSet<QETProject *> projects_to_display_;       ///< list of projects that have been added to this panel
	QTreeWidgetItem *common_collection_item_;      ///< pointer to the item representing the common elements collection
	QTreeWidgetItem *common_tbt_collection_item_;  ///< pointer to the item representing the common templates collection
	QTreeWidgetItem *custom_collection_item_;      ///< pointer to the item representing the user elements collection
	QTreeWidgetItem *custom_tbt_collection_item_;  ///< pointer to the item representing the user templates collection
	int loading_progress_;                         ///< used to track the loading progress of elements collections
	bool first_reload_;                            ///< used to distinguish the first time this panel is reloaded
};
#endif
