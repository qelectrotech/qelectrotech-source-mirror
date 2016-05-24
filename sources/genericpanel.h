/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef GENERIC_PANEL_H
#define GENERIC_PANEL_H
#include "qet.h"
#include <QTreeWidget>
#include "elementslocation.h"
class QTreeWidgetItem;
class QETProject;
class Diagram;
class ElementsCollection;
class ElementsCategory;
class ElementDefinition;
class TitleBlockTemplatesCollection;
class TitleBlockTemplateLocation;
class ElementsCollectionCache;

/**
	The generic panel is a QTreeWidget subclass providing extra methods
	allowing developers to easily add objects (projects, diagrams, title block
	templates, elements, ...) to it; it also ensures the displayed information
	remains up to date.
*/
class GenericPanel : public QTreeWidget {
	Q_OBJECT
	
	public:
	enum PanelOption {
		AddChildDiagrams              =   1,
		AddChildTemplatesCollection   =   2,
		AddChildTemplates             =   4,
		AddAllChildTemplates          =   6,
		AddChildElementsCollections   =   8,
		AddChildElementsCategories    =  16,
		AddChildElementsContainers    =  24,
		AddChildElements              =  32,
		AddAllChildElements           =  56,
		AddAllChild                   =  63,
		DisplayElementsPreview        =  64,
		All                           = 127
	};
	Q_DECLARE_FLAGS(PanelOptions, PanelOption)
	
	enum MetaData {
		Item = Qt::UserRole + 1,
		AliasItem,
		Parent,
		PanelFlags
	};
	
	// Constructors, destructor
	public:
	GenericPanel(QWidget * = 0);
	virtual ~GenericPanel();
	
	// cache-related methods
	public:
	virtual ElementsCollectionCache *elementsCache();
	virtual bool setElementsCache(ElementsCollectionCache *, ElementsCollectionCache ** = 0);
	
	protected:
	virtual ElementsCollectionCache *getElementsCache();
	
	public:
	// convenience methods to obtain what an item represents
	virtual int currentItemType();
	virtual QETProject *projectForItem(QTreeWidgetItem *) const;
	virtual Diagram *diagramForItem(QTreeWidgetItem *) const;
	virtual TitleBlockTemplateLocation templateLocationForItem(QTreeWidgetItem *) const;
	virtual ElementsLocation elementLocationForItem(QTreeWidgetItem *) const;
	
	// convenience methods to obtain what the selected item represents
	virtual QETProject *selectedProject() const;
	virtual Diagram *selectedDiagram() const;
	virtual TitleBlockTemplateLocation selectedTemplateLocation() const;
	virtual ElementsLocation selectedElementLocation() const;
	
	// project-related methods
	public:
	virtual QTreeWidgetItem *addProject(QETProject *, QTreeWidgetItem * = 0, PanelOptions = AddAllChild);
	virtual QTreeWidgetItem *itemForProject(QETProject *);
	protected:
	virtual QTreeWidgetItem *getItemForProject(QETProject *, bool * = 0);
	virtual QTreeWidgetItem *updateProjectItem(QTreeWidgetItem *, QETProject *, PanelOptions = AddAllChild, bool = false);
	virtual QTreeWidgetItem *fillProjectItem  (QTreeWidgetItem *, QETProject *, PanelOptions = AddAllChild, bool = false);
	
	// diagram-related methods
	public:
	virtual QTreeWidgetItem *addDiagram(Diagram *, QTreeWidgetItem * = 0, PanelOptions = AddAllChild);
	virtual QTreeWidgetItem *itemForDiagram(Diagram *);
	protected:
	virtual QTreeWidgetItem *getItemForDiagram(Diagram *, bool * = 0);
	virtual QTreeWidgetItem *updateDiagramItem(QTreeWidgetItem *, Diagram *, PanelOptions = AddAllChild, bool = false);
	virtual QTreeWidgetItem *fillDiagramItem  (QTreeWidgetItem *, Diagram *, PanelOptions = AddAllChild, bool = false);
	
	// title block templates collections methods
	public:
	virtual QTreeWidgetItem *addTemplatesCollection(TitleBlockTemplatesCollection *, QTreeWidgetItem * = 0, PanelOptions = AddAllChild);
	virtual QTreeWidgetItem *itemForTemplatesCollection(TitleBlockTemplatesCollection *);
	protected:
	virtual QTreeWidgetItem *getItemForTemplatesCollection(TitleBlockTemplatesCollection *, bool * = 0);
	virtual QTreeWidgetItem *updateTemplatesCollectionItem(QTreeWidgetItem *, TitleBlockTemplatesCollection *, PanelOptions = AddAllChild, bool = false);
	virtual QTreeWidgetItem *fillTemplatesCollectionItem  (QTreeWidgetItem *, TitleBlockTemplatesCollection *, PanelOptions = AddAllChild, bool = false);
	
	// title block templates methods
	public:
	virtual QTreeWidgetItem *addTemplate(const TitleBlockTemplateLocation &, QTreeWidgetItem * = 0, PanelOptions = AddAllChild);
	virtual QTreeWidgetItem *itemForTemplate(const TitleBlockTemplateLocation &);
	protected:
	virtual QTreeWidgetItem *getItemForTemplate(const TitleBlockTemplateLocation &, bool * = 0);
	virtual QTreeWidgetItem *updateTemplateItem(QTreeWidgetItem *, const TitleBlockTemplateLocation &, PanelOptions = AddAllChild, bool = false);
	virtual QTreeWidgetItem *fillTemplateItem  (QTreeWidgetItem *, const TitleBlockTemplateLocation &, PanelOptions = AddAllChild, bool = false);
	
	// elements collections methods
	public:
	virtual QTreeWidgetItem *itemForElementsLocation(const ElementsLocation &);
	virtual QTreeWidgetItem *addElementsCollection(ElementsCollection *, QTreeWidgetItem *, PanelOptions = AddAllChild);
	protected:
	virtual QTreeWidgetItem *getItemForElementsCollection(ElementsCollection *, bool * = 0);
	virtual QTreeWidgetItem *updateElementsCollectionItem(QTreeWidgetItem *, ElementsCollection *, PanelOptions = AddAllChild, bool = false);
	virtual QTreeWidgetItem *fillElementsCollectionItem  (QTreeWidgetItem *, ElementsCollection *, PanelOptions = AddAllChild, bool = false);
	
	// elements categories methods
	public:
	virtual QTreeWidgetItem *addElementsCategory(ElementsCategory *, QTreeWidgetItem * = 0, PanelOptions = AddAllChild);
	virtual QTreeWidgetItem *itemForElementsCategory(ElementsCategory *);
	protected:
	virtual QTreeWidgetItem *getItemForElementsCategory(ElementsCategory *, bool * = 0);
	virtual QTreeWidgetItem *updateElementsCategoryItem(QTreeWidgetItem *, ElementsCategory *, PanelOptions = AddAllChild, bool = false);
	virtual QTreeWidgetItem *fillElementsCategoryItem  (QTreeWidgetItem *, ElementsCategory *, PanelOptions = AddAllChild, bool = false);
	
	// elements methods
	public:
	virtual QTreeWidgetItem *addElement(ElementDefinition *, QTreeWidgetItem * = 0, PanelOptions = AddAllChild);
	virtual QTreeWidgetItem *itemForElement(ElementDefinition *);
	protected:
	virtual QTreeWidgetItem *getItemForElement(ElementDefinition *, bool * = 0);
	virtual QTreeWidgetItem *updateElementItem(QTreeWidgetItem *, ElementDefinition *, PanelOptions = AddAllChild, bool = false);
	virtual QTreeWidgetItem *fillElementItem  (QTreeWidgetItem *, ElementDefinition *, PanelOptions = AddAllChild, bool = false);
	
	// generic methods
	protected:
	virtual QTreeWidgetItem *updateItem(QTreeWidgetItem *, PanelOptions = AddAllChild, bool = false);
	virtual QTreeWidgetItem *fillItem  (QTreeWidgetItem *, PanelOptions = AddAllChild, bool = false);
	
	// slots used to receive change notifications from added objects
	protected slots:
	virtual void projectInformationsChanged(QETProject *);
	virtual void diagramAdded(QETProject *, Diagram *);
	virtual void diagramRemoved(QETProject *, Diagram *);
	virtual void projectDiagramsOrderChanged(QETProject *, int, int);
	virtual void diagramTitleChanged(Diagram *, const QString &);
	virtual void templatesCollectionChanged(TitleBlockTemplatesCollection*, const QString &);
	virtual void diagramUsedTemplate(TitleBlockTemplatesCollection *, const QString &);
	virtual void elementsCollectionChanged(ElementsCollection *);
	
	// various other methods
	protected:
	virtual QString defaultText(QET::ItemType);
	virtual QIcon   defaultIcon(QET::ItemType);
	virtual QTreeWidgetItem *makeItem(QET::ItemType, QTreeWidgetItem * = 0, const QString & = QString(), const QIcon & = QIcon());
	virtual void deleteItem(QTreeWidgetItem *, bool = false);
	virtual void markItemAsContainer(QTreeWidgetItem *);
	virtual void markItemAsUnused(QTreeWidgetItem *);
	virtual void reparent(QTreeWidgetItem *, QTreeWidgetItem *);
	QList<QTreeWidgetItem *> childItems(QTreeWidgetItem *, QET::ItemType, bool = false) const;
	template<typename T> void removeObsoleteItems(const QList<T> &, QTreeWidgetItem *, QET::ItemType, bool);
	template<typename T> T valueForItem(QTreeWidgetItem *) const;
	void unregisterItem(QTreeWidgetItem *);
	void clearPanel();
	
	bool event(QEvent *);
	
	signals:
	bool firstActivated();
	void panelContentChanged();
	
	private slots:
	void emitFirstActivated();
	
	protected:
	ElementsCollectionCache *cache_; ///< Cache used to render elements
	
	private:
	bool first_activation_; ///< boolean used to track the first time this widget is activated/shown
	QHash<QETProject *, QTreeWidgetItem *>               projects_;     ///< Allow quick retrieval of the item representing a given project
	QHash<Diagram *, QTreeWidgetItem *>                  diagrams_;     ///< Allow quick retrieval of the item representing a given diagram
	QHash<TitleBlockTemplateLocation, QTreeWidgetItem *> tb_templates_; ///< Allow quick retrieval of the item representing a title block template
	QHash<ElementsLocation, QTreeWidgetItem *>           elements_;     ///< Allow quick retrieval of the item representing an element
};
#endif
