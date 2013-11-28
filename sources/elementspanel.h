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
	This class provides a tree widget listing known filesystem-based elements
	collections and opened project files along with their content (diagrams,
	embedded collections, ...). It enables users to perform various actions on
	the displayed content
*/
class ElementsPanel : public GenericPanel {
	Q_OBJECT
	
	// constructors, destructor
	public:
	ElementsPanel(QWidget * = 0);
	virtual ~ElementsPanel();
	
	private:
	ElementsPanel(const ElementsPanel &);
	
	// methods
	public:
	// methods used to determine what is represented by a particular visual item
	bool itemIsWritable(QTreeWidgetItem *) const;
	bool selectedItemIsWritable() const;
	
	// methods used to get what is represented by a particular visual item
	ElementsCollectionItem *collectionItemForItem(QTreeWidgetItem *) const;
	ElementsCollectionItem *selectedItem() const;
	ElementsCategory *categoryForItem(QTreeWidgetItem *);
	ElementsCategory *categoryForPos(const QPoint &);
	QString dirPathForItem(QTreeWidgetItem *);
	QString filePathForItem(QTreeWidgetItem *);
	
	void reloadCollections();
	int elementsCollectionItemsCount();
	bool mustHighlightIntegratedElements() const;
	
	signals:
	void requestForProject(QETProject *);
	void requestForDiagram(Diagram *);
	void requestForCollectionItem(const ElementsLocation &);
	void requestForMoveElements(ElementsCollectionItem *, ElementsCollectionItem *, QPoint);
	void requestForTitleBlockTemplate(const TitleBlockTemplateLocation &);
	void readingAboutToBegin();
	void readingFinished();
	void loadingProgressed(int, int);
	void loadingFinished();
	
	public slots:
	void slot_doubleClick(QTreeWidgetItem *, int);
	void reload(bool = false);
	void filter(const QString &, QET::Filtering = QET::RegularFilter);
	void projectWasOpened(QETProject *);
	void projectWasClosed(QETProject *);
	bool scrollToElement(const ElementsLocation &);
	void applyCurrentFilter(const QList<QTreeWidgetItem *> &);
	void ensureHierarchyIsVisible(const QList<QTreeWidgetItem *> &);
	void scrollToSelectedItem();
	void highlightItems(const QList<QTreeWidgetItem *> &, const QObject * = 0, const char * = 0);
	
	protected:
	void dragEnterEvent(QDragEnterEvent *);
	void dragMoveEvent(QDragMoveEvent *);
	void dropEvent(QDropEvent *);
	void startDrag(Qt::DropActions);
	void startElementDrag(const ElementsLocation &);
	void startTitleBlockTemplateDrag(const TitleBlockTemplateLocation &);
	bool matchesCurrentFilter(const QTreeWidgetItem *) const;
	
	protected slots:
	void firstActivation();
	void panelContentChange();
	virtual QList<ElementsLocation> elementIntegrated(QETProject *, const ElementsLocation &);
	
	private:
	virtual QTreeWidgetItem *addProject   (QETProject *);
	virtual QTreeWidgetItem *addCollection(ElementsCollection *);
	virtual QTreeWidgetItem *updateTemplatesCollectionItem(QTreeWidgetItem *, TitleBlockTemplatesCollection *, PanelOptions = AddAllChild, bool = false);
	virtual QTreeWidgetItem *updateTemplateItem        (QTreeWidgetItem *, const TitleBlockTemplateLocation &,  PanelOptions, bool = false);
	virtual QTreeWidgetItem *updateElementsCategoryItem(QTreeWidgetItem *, ElementsCategory *,  PanelOptions, bool = false);
	virtual QTreeWidgetItem *updateElementsCollectionItem(QTreeWidgetItem *, ElementsCollection *,  PanelOptions, bool = false);
	virtual QTreeWidgetItem *updateElementItem         (QTreeWidgetItem *, ElementDefinition *, PanelOptions, bool = false);
	
	// attributes
	private:
	QSet<QETProject *> projects_to_display_;       ///< list of projects that have been added to this panel
	QTreeWidgetItem *common_collection_item_;      ///< pointer to the item representing the common elements collection
	QTreeWidgetItem *common_tbt_collection_item_;  ///< pointer to the item representing the common templates collection
	QTreeWidgetItem *custom_collection_item_;      ///< pointer to the item representing the user elements collection
	QTreeWidgetItem *custom_tbt_collection_item_;  ///< pointer to the item representing the user templates collection
	int loading_progress_;                         ///< used to track the loading progress of elements collections
	bool first_reload_;                            ///< used to distinguish the first time this panel is reloaded
	QString filter_;                               ///< Currently applied filter
	QTreeWidgetItem *it_prev_, *it_;
};
#endif
