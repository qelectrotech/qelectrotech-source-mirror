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
class ElementsPanel : public QTreeWidget {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	ElementsPanel(QWidget * = 0);
	virtual ~ElementsPanel();
	
	private:
	ElementsPanel(const ElementsPanel &);
	
	// methodes
	public:
	// methodes pour determiner ce que represente un item donne
	bool itemIsACollection(QTreeWidgetItem *) const;
	bool itemIsACategory(QTreeWidgetItem *) const;
	bool itemIsAnElement(QTreeWidgetItem *) const;
	bool itemIsAProject(QTreeWidgetItem *) const;
	bool itemIsADiagram(QTreeWidgetItem *) const;
	bool itemHasLocation(QTreeWidgetItem *) const;
	bool itemIsWritable(QTreeWidgetItem *) const;
	bool itemIsATitleBlockTemplatesCollection(QTreeWidgetItem *) const;
	bool itemIsATitleBlockTemplate(QTreeWidgetItem *) const;
	
	// methodes pour obtenir ce que represente un item donne
	ElementsCollectionItem *collectionItemForItem(QTreeWidgetItem *) const;
	QETProject *projectForItem(QTreeWidgetItem *) const;
	Diagram *diagramForItem(QTreeWidgetItem *) const;
	ElementsLocation locationForItem(QTreeWidgetItem *) const;
	ElementsCategory *categoryForItem(QTreeWidgetItem *);
	ElementsCategory *categoryForPos(const QPoint &);
	TitleBlockTemplateLocation locationForTitleBlockTemplate(QTreeWidgetItem *);
	QString nameOfTitleBlockTemplate(QTreeWidgetItem *);
	
	// methodes pour determiner ce que represente l'item selectionne
	bool selectedItemIsACollection() const;
	bool selectedItemIsACategory() const;
	bool selectedItemIsAnElement() const;
	bool selectedItemIsAProject() const;
	bool selectedItemIsADiagram() const;
	bool selectedItemHasLocation() const;
	bool selectedItemIsWritable() const;
	bool selectedItemIsATitleBlockTemplatesDirectory() const;
	bool selectedItemIsATitleBlockTemplate() const;
	
	// methodes pour obtenir ce que represente l'item selectionne
	ElementsCollectionItem *selectedItem() const;
	QETProject *selectedProject() const;
	Diagram *selectedDiagram() const;
	ElementsLocation selectedLocation() const;
	
	void reloadCollections();
	int elementsCollectionItemsCount();
	
	signals:
	void requestForProject(QETProject *);
	void requestForDiagram(Diagram *);
	void requestForCollectionItem(ElementsCollectionItem *);
	void requestForMoveElements(ElementsCollectionItem *, ElementsCollectionItem *, QPoint);
	void requestForTitleBlockTemplate(const TitleBlockTemplateLocation &);
	void readingAboutToBegin();
	void readingFinished();
	void loadingProgressed(int, int);
	
	public slots:
	void slot_doubleClick(QTreeWidgetItem *, int);
	void reload(bool = false);
	void filter(const QString &);
	void projectWasOpened(QETProject *);
	void projectWasClosed(QETProject *);
	void projectInformationsChanged(QETProject *);
	void titleBlockTemplatesCollectionChanged(TitleBlockTemplatesCollection*, const QString & = QString());
	void diagramWasAdded(QETProject *, Diagram *);
	void diagramWasRemoved(QETProject *, Diagram *);
	void diagramTitleChanged(QETProject *, Diagram *);
	void diagramOrderChanged(QETProject *, int, int);
	bool scrollToElement(const ElementsLocation &);
	
	protected:
	void dragEnterEvent(QDragEnterEvent *);
	void dragMoveEvent(QDragMoveEvent *);
	void dropEvent(QDropEvent *);
	void startDrag(Qt::DropActions);
	void startElementDrag(const ElementsLocation &);
	void startTitleBlockTemplateDrag(const TitleBlockTemplateLocation &);
	bool event(QEvent *);
	
	private:
	QTreeWidgetItem *addProject   (QTreeWidgetItem *, QETProject         *);
	QTreeWidgetItem *addDiagram   (QTreeWidgetItem *, Diagram            *);
	QTreeWidgetItem *addCollection(QTreeWidgetItem *, ElementsCollection *, const QString & = QString(), const QIcon & = QIcon());
	QTreeWidgetItem *addCategory  (QTreeWidgetItem *, ElementsCategory   *, const QString & = QString(), const QIcon & = QIcon());
	QTreeWidgetItem *addElement   (QTreeWidgetItem *, ElementDefinition  *, const QString & = QString());
	QTreeWidgetItem *addTitleBlockTemplatesCollection(QTreeWidgetItem *, TitleBlockTemplatesCollection *, const QString & = QString(), const QIcon & = QIcon());
	void saveExpandedCategories();
	QTreeWidgetItem *findLocation(const ElementsLocation &) const;
	QTreeWidgetItem *findLocation(const QString &) const;
	void deleteItem(QTreeWidgetItem *);
	void updateProjectItemInformations(QETProject *);
	void updateDiagramLabel(QTreeWidgetItem *, int);
	QString diagramTitleToDisplay(Diagram *) const;
	QString titleBlockTemplateNameToDisplay(const QString &) const;
	void ensureHierarchyIsVisible(QList<QTreeWidgetItem *>);
	void markItemAsUnused(QTreeWidgetItem *);
	
	// attributs
	private:
	QStringList expanded_directories;
	QString last_selected_item;
	QHash<QTreeWidgetItem *, ElementsLocation> locations_;
	QHash<QTreeWidgetItem *, TitleBlockTemplateLocation> title_blocks_;
	QSet<QETProject *> projects_to_display_;
	QHash<QTreeWidgetItem *, QETProject *> projects_;
	QHash<QTreeWidgetItem *, Diagram *> diagrams_;
	QHash<QTreeWidgetItem *, TitleBlockTemplatesCollection *> title_blocks_collections_;
	QTreeWidgetItem *common_collection_item_;
	QTreeWidgetItem *common_tbt_collection_item_;
	QTreeWidgetItem *custom_collection_item_;
	QTreeWidgetItem *custom_tbt_collection_item_;
	int loading_progress_;
	bool first_activation_;
	bool first_reload_;
	ElementsCollectionCache *cache_;
};
#endif
