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
#ifndef PANELAPPAREILS_H
#define PANELAPPAREILS_H

#include "genericpanel.h"
#include "templatelocation.h"

class QETProject;
class Diagram;
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
	
	// methods used to get what is represented by a particular visual item
	QString dirPathForItem(QTreeWidgetItem *);
	QString filePathForItem(QTreeWidgetItem *);
	
	signals:
	void requestForProject(QETProject *);
	void requestForDiagram(Diagram *);
	void requestForTitleBlockTemplate(const TitleBlockTemplateLocation &);
	
	public slots:
	void slot_doubleClick(QTreeWidgetItem *, int);
	void reload(bool = false);
	void filter(const QString &, QET::Filtering = QET::RegularFilter);
	void projectWasOpened(QETProject *);
	void projectWasClosed(QETProject *);
	void buildFilterList();
	void applyCurrentFilter(const QList<QTreeWidgetItem *> &);
	void ensureHierarchyIsVisible(const QList<QTreeWidgetItem *> &);
	
	protected:
	void dragEnterEvent(QDragEnterEvent *);
	void startDrag(Qt::DropActions);
	void startTitleBlockTemplateDrag(const TitleBlockTemplateLocation &);
	bool matchesCurrentFilter(const QTreeWidgetItem *) const;
	bool matchesFilter(const QTreeWidgetItem *, QString) const;
	
	protected slots:
	void panelContentChange();
	
	private:
	virtual QTreeWidgetItem *addProject   (QETProject *);
	virtual QTreeWidgetItem *updateTemplatesCollectionItem(QTreeWidgetItem *, TitleBlockTemplatesCollection *, PanelOptions = AddAllChild, bool = false);
	virtual QTreeWidgetItem *updateTemplateItem        (QTreeWidgetItem *, const TitleBlockTemplateLocation &,  PanelOptions, bool = false);
	
	// attributes
	private:
	QSet<QETProject *> projects_to_display_;       ///< list of projects that have been added to this panel
	QTreeWidgetItem *common_tbt_collection_item_;  ///< pointer to the item representing the common templates collection
	QTreeWidgetItem *custom_tbt_collection_item_;  ///< pointer to the item representing the user templates collection
	int loading_progress_;                         ///< used to track the loading progress of elements collections
	bool first_reload_;                            ///< used to distinguish the first time this panel is reloaded
	QString filter_;                               ///< Currently applied filter
	QStringList filter_list_;                       ///< Currently applied list of filter
};
#endif
