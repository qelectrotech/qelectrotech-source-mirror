/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "genericpanel.h"
#include <QTreeWidgetItem>
#include "qetproject.h"
#include "diagram.h"
#include "titleblock/templatescollection.h"
#include "titleblock/templatelocation.h"
#include "qeticons.h"
#include "qetapp.h"

/**
	Constructor
	@param parent Parent QWidget
*/
GenericPanel::GenericPanel(QWidget *parent) :
	QTreeWidget(parent),
	first_activation_(true)
{
	header() -> hide();
	setIconSize(QSize(50, 50));
}

/**
	Destructor
*/
GenericPanel::~GenericPanel()
{
}

/**
	@return the type of the current item
*/
int GenericPanel::currentItemType()
{
	QTreeWidgetItem *current_qtwi = currentItem();
	if (!current_qtwi) return(0);
	return(current_qtwi -> type());
}

/**
	@brief GenericPanel::projectForItem
	@param item
	@return nullptr
*/
QETProject *GenericPanel::projectForItem(QTreeWidgetItem *item) const
{
	if (item && item -> type() == QET::Project) {
		return(valueForItem<QETProject *>(item));
	}
	return(nullptr);
	
}

/**
	@brief GenericPanel::diagramForItem
	@param item
	@return nullptr
*/
Diagram *GenericPanel::diagramForItem(QTreeWidgetItem *item) const
{
	if (item && item -> type() == QET::Diagram) {
		return(valueForItem<Diagram *>(item));
	}
	return(nullptr);
}

/**
	@brief GenericPanel::templateLocationForItem
	@param item
	@return TitleBlockTemplateLocation()
*/
TitleBlockTemplateLocation GenericPanel::templateLocationForItem(
		QTreeWidgetItem *item) const
{
	if (item && item -> type() & QET::TitleBlockTemplatesCollectionItem) {
		return(valueForItem<TitleBlockTemplateLocation>(item));
	}
	return(TitleBlockTemplateLocation());
}

/**
	@brief GenericPanel::selectedProject
	@return projectForItem(currentItem())
*/
QETProject *GenericPanel::selectedProject() const
{
	return(projectForItem(currentItem()));
}

/**
	@brief GenericPanel::selectedDiagram
	@return diagramForItem(currentItem())
*/
Diagram *GenericPanel::selectedDiagram() const
{
	return(diagramForItem(currentItem()));
}

/**
	@brief GenericPanel::selectedTemplateLocation
	@return templateLocationForItem(currentItem())
*/
TitleBlockTemplateLocation GenericPanel::selectedTemplateLocation() const
{
	return(templateLocationForItem(currentItem()));
}

/**
	@brief GenericPanel::addProject
	@param project
	@param parent_item
	@param options
	@return project_qtwi
*/
QTreeWidgetItem *GenericPanel::addProject(QETProject *project,
					  QTreeWidgetItem *parent_item,
					  PanelOptions options) {
	if (!project) return(nullptr);
	bool creation_required;
	
	QTreeWidgetItem *project_qtwi = getItemForProject(project,
							  &creation_required);
	updateProjectItem(project_qtwi, project, options, creation_required);
	reparent(project_qtwi, parent_item);
	fillProjectItem(project_qtwi, project, options, creation_required);
	
	return(project_qtwi);
}

/**
	@param project A standard project.
	@return the tree item representing the provided project or 0 if this
	project does not appear within this panel.
*/
QTreeWidgetItem *GenericPanel::itemForProject(QETProject *project) {
	if (!project) return(nullptr);
	return(projects_.value(project, nullptr));
}

/**
	@param project A standard project.
	@param created if provided with a pointer to a boolean, this method will
	update it to reflect whether the returned item has been freshly created or
	not.
	@return the tree item representing the provided project. If it does not
	appear within this panel, it is created.
*/
QTreeWidgetItem *GenericPanel::getItemForProject(QETProject *project,
						 bool *created) {
	if (!project) return(nullptr);
	
	QTreeWidgetItem *project_qtwi = projects_.value(project, nullptr);
	if (project_qtwi) {
		if (created) *created = false;
		return(project_qtwi);
	}
	
	project_qtwi = makeItem(QET::Project);
	if (created) *created = true;
	return(project_qtwi);
}

/**
	@brief GenericPanel::updateProjectItem
	@param project_qtwi
	@param project
	@param options (unused)
	@param freshly_created
	@return updateItem(project_qtwi, options, freshly_created)
*/
QTreeWidgetItem *GenericPanel::updateProjectItem(QTreeWidgetItem *project_qtwi,
						 QETProject *project,
						 PanelOptions options,
						 bool freshly_created) {
	Q_UNUSED(options)
	if (!project_qtwi || !project) return(nullptr);
	
	if (freshly_created) {
		project_qtwi -> setData(0, GenericPanel::Item,
					QVariant::fromValue(project));
		projects_.insert(project, project_qtwi);
		
		connect(
			project,&QETProject::projectInformationsChanged,
			this,&GenericPanel::projectInformationsChanged);
		connect(
			project,&QETProject::readOnlyChanged,
			this,&GenericPanel::projectInformationsChanged);
	}
	
	// text
	project_qtwi -> setText(0, project -> pathNameTitle());
	// tooltip
	QString final_tooltip = QDir::toNativeSeparators(project -> filePath());
	if (final_tooltip.isEmpty()) {
		final_tooltip = tr(
			"Pas de fichier",
			"tooltip for a file-less project in the element panel"
		);
	}
	project_qtwi -> setToolTip(0, final_tooltip);
	QString project_whatsthis = tr("Ceci est un projet QElectroTech, "
		"c'est-à-dire un fichier d'extension .qet regroupant "
		"plusieurs folios. Il embarque également les éléments et "
		"modèles de cartouches utilisés dans ces folios.",
		"\"What's this\" tip");
	project_qtwi -> setWhatsThis(0, project_whatsthis);
	return(updateItem(project_qtwi, options, freshly_created));
}

/**
	@brief GenericPanel::fillProjectItem
	@param project_qtwi
	@param project
	@param options
	@param freshly_created
	@return fillItem(project_qtwi, options, freshly_created)
*/
QTreeWidgetItem *GenericPanel::fillProjectItem(QTreeWidgetItem *project_qtwi,
					       QETProject *project,
					       PanelOptions options,
					       bool freshly_created) {
	if (!project_qtwi || !project) return(nullptr);
	
	
	if (options & AddChildDiagrams) {
		if (freshly_created) {
			connect(
				project,&QETProject::diagramAdded,
				this,&GenericPanel::diagramAdded);
			connect(
				project,&QETProject::diagramRemoved,
				this,&GenericPanel::diagramRemoved);
			connect(
				project,&QETProject::projectDiagramsOrderChanged,
				this,&GenericPanel::projectDiagramsOrderChanged);
		} else {
			// remove diagrams unknown to the project (presumably removed)
			removeObsoleteItems(project -> diagrams(),
					    project_qtwi, QET::Diagram, false);
		}
		int index = 0;
		foreach (Diagram *diagram, project -> diagrams()) {
			QTreeWidgetItem *diagram_qtwi = addDiagram(diagram,
								   nullptr,
								   options);
			project_qtwi -> insertChild(index, diagram_qtwi);
			++ index;
		}
	}
	
	if (options & AddChildTemplatesCollection) {
		if (freshly_created) {
			connect(
				project,&QETProject::diagramUsedTemplate,
				this,&GenericPanel::diagramUsedTemplate);
		}
		addTemplatesCollection(
			project -> embeddedTitleBlockTemplatesCollection(),
			project_qtwi,
			options);
	}

	return(fillItem(project_qtwi, options, freshly_created));
}

/**
	@brief GenericPanel::addDiagram
	@param diagram
	@param parent_item
	@param options (unused)
	@return diagram_qtwi
*/
QTreeWidgetItem *GenericPanel::addDiagram(Diagram *diagram,
					  QTreeWidgetItem *parent_item,
					  PanelOptions options) {
	Q_UNUSED(options)
	if (!diagram) return(nullptr);
	
	bool creation_required;
	
	QTreeWidgetItem *diagram_qtwi = getItemForDiagram(diagram,
							  &creation_required);
	updateDiagramItem(diagram_qtwi, diagram, options, creation_required);
	reparent(diagram_qtwi, parent_item);
	fillDiagramItem(diagram_qtwi, options, creation_required);
	
	return(diagram_qtwi);
}

/**
	@brief GenericPanel::getItemForDiagram
	@param diagram
	@param created
	@return diagram_qtwi
*/
QTreeWidgetItem *GenericPanel::getItemForDiagram(Diagram *diagram,
						 bool *created) {
	if (!diagram) return(nullptr);
	
	QTreeWidgetItem *diagram_qtwi = diagrams_.value(diagram, nullptr);
	if (diagram_qtwi) {
		if (created) *created = false;
		return(diagram_qtwi);
	}
	
	diagram_qtwi = makeItem(QET::Diagram);
	if (created) *created = true;
	return(diagram_qtwi);
}

/**
	@brief GenericPanel::updateDiagramItem
	@param diagram_qtwi
	@param diagram
	@param options
	@param freshly_created
	@return updateItem(diagram_qtwi, options, freshly_created)
*/
QTreeWidgetItem *GenericPanel::updateDiagramItem(QTreeWidgetItem *diagram_qtwi,
						 Diagram *diagram,
						 PanelOptions options,
						 bool freshly_created) {
	if (!diagram || !diagram_qtwi) return(nullptr);
	QSettings settings;
	
	QString displayed_title = diagram -> title();
	if (displayed_title.isEmpty())
	{
		displayed_title = tr("Folio sans titre",
				     "Fallback label when a diagram has no title");
	}

	if (settings.value("genericpanel/folio", true).toBool())
	{
		QString displayed_label =
				diagram ->border_and_titleblock.finalfolio();
		int diagram_folio_idx = diagram -> folioIndex();
		if (diagram_folio_idx != -1)
		{
			displayed_label = QString(
				tr(
				   "%1 - %2",
				   "label displayed for a diagram in the panel ;"
				   " %1 is the folio index, %2 is the diagram title"
				   )
				).arg(displayed_label).arg(displayed_title);
			diagram_qtwi -> setText(0, displayed_label);
		}

	}
	else
	{
		QString displayed_label;
		int diagram_folio_idx = diagram -> folioIndex();
		if (diagram_folio_idx != -1)
		{
			displayed_label = QString(
				tr(
				   "%1 - %2",
				   "label displayed for a diagram in the panel ;"
				   " %1 is the folio index, %2 is the diagram title"
				   )
				).arg(diagram_folio_idx + 1).arg(displayed_title);
		}

		diagram_qtwi -> setText(0, displayed_label);

	}
	if (freshly_created)
	{
		diagram_qtwi -> setData(0,
					GenericPanel::Item,
					QVariant::fromValue(diagram));
		diagrams_.insert(diagram, diagram_qtwi);
		
		connect(diagram, &Diagram::diagramTitleChanged,
			this, &GenericPanel::diagramTitleChanged);
	}
	
	return(updateItem(diagram_qtwi, options, freshly_created));
}



/**
	@brief GenericPanel::fillDiagramItem
	@param diagram_qtwi
	@param options
	@param freshly_created
	@return fillItem(diagram_qtwi, options, freshly_created)
*/
QTreeWidgetItem *GenericPanel::fillDiagramItem(
		QTreeWidgetItem *diagram_qtwi,
		PanelOptions options,
		bool freshly_created)
{
	return(fillItem(diagram_qtwi, options, freshly_created));
}

/**
	@brief GenericPanel::addTemplatesCollection
	@param tbt_collection
	@param parent_item
	@param options
	@return tbt_collection_qtwi
*/
QTreeWidgetItem *GenericPanel::addTemplatesCollection(
		TitleBlockTemplatesCollection *tbt_collection,
		QTreeWidgetItem *parent_item,
		PanelOptions options) {
	if (!tbt_collection) return(nullptr);
	bool creation_required;
	
	QTreeWidgetItem *tbt_collection_qtwi =
			getItemForTemplatesCollection(tbt_collection,
						      &creation_required);
	updateTemplatesCollectionItem(tbt_collection_qtwi,
				      tbt_collection,
				      options,
				      creation_required);
	reparent(tbt_collection_qtwi, parent_item);
	fillTemplatesCollectionItem(tbt_collection_qtwi,
				    tbt_collection,
				    options,
				    creation_required);
	
	return(tbt_collection_qtwi);
}

/**
	@brief GenericPanel::itemForTemplatesCollection
	@param tbt_collection
	@return tb_templates_.value(tbt_collection -> location(), nullptr)
	@return nullptr
*/
QTreeWidgetItem *GenericPanel::itemForTemplatesCollection(
		TitleBlockTemplatesCollection *tbt_collection) {
	if (!tbt_collection) return(nullptr);
	return(tb_templates_.value(tbt_collection -> location(), nullptr));
}

/**
	@brief GenericPanel::getItemForTemplatesCollection
	@param tbt_collection
	@param created
	@return tbt_collection_item
*/
QTreeWidgetItem *GenericPanel::getItemForTemplatesCollection(
		TitleBlockTemplatesCollection *tbt_collection,
		bool *created) {
	if (!tbt_collection) return(nullptr);
	QTreeWidgetItem *tbt_collection_item =
			tb_templates_.value(tbt_collection -> location(),
					    nullptr);
	if (tbt_collection_item) {
		if (created) *created = false;
		return(tbt_collection_item);
	}
	
	tbt_collection_item = makeItem(QET::TitleBlockTemplatesCollection);
	if (created) *created = true;
	return(tbt_collection_item);
}

/**
	@brief GenericPanel::updateTemplatesCollectionItem
	@param tbt_collection_qtwi
	@param tbt_collection
	@param options
	@param freshly_created
	@return updateItem(tbt_collection_qtwi, options, freshly_created)
*/
QTreeWidgetItem *GenericPanel::updateTemplatesCollectionItem(
		QTreeWidgetItem *tbt_collection_qtwi,
		TitleBlockTemplatesCollection *tbt_collection,
		PanelOptions options,
		bool freshly_created) {
	QString label = tbt_collection -> title();
	if (label.isEmpty()) label = tr("Modèles de cartouche");
	
	tbt_collection_qtwi -> setText(0, label);
	tbt_collection_qtwi -> setToolTip(0,
					  tbt_collection->location().toString());
	
	if (freshly_created) {
		tbt_collection_qtwi -> setData(0,
					       GenericPanel::Item,
					       QVariant::fromValue(
						   tbt_collection->location()));
		tb_templates_.insert(tbt_collection -> location(),
				     tbt_collection_qtwi);
	}
	
	return(updateItem(tbt_collection_qtwi, options, freshly_created));
}

/**
	@brief GenericPanel::fillTemplatesCollectionItem
	@param tbt_collection_qtwi
	@param tbt_collection
	@param options
	@param freshly_created
	@return fillItem(tbt_collection_qtwi, options, freshly_created)
*/
QTreeWidgetItem *GenericPanel::fillTemplatesCollectionItem(
		QTreeWidgetItem *tbt_collection_qtwi,
		TitleBlockTemplatesCollection *tbt_collection,
		PanelOptions options,
		bool freshly_created) {
	if (!tbt_collection_qtwi || !tbt_collection)
		return(tbt_collection_qtwi);
	
	if (options & AddChildTemplates) {
		if (freshly_created) {
			connect(
				tbt_collection,
				&TitleBlockTemplatesCollection::changed,
				this,
				&GenericPanel::templatesCollectionChanged);
			if (QETProject *project = tbt_collection -> parentProject()) {
				connect(
					project, &QETProject::diagramUsedTemplate,
					this,
					&GenericPanel::templatesCollectionChanged);
			}
		} else {
		// remove templates unknown to the collection (presumably removed)
			removeObsoleteItems(
					tbt_collection -> templatesLocations(),
					tbt_collection_qtwi,
					QET::TitleBlockTemplate,
					false);
		}
		
		int index = 0;
		foreach (QString template_name, tbt_collection -> templates()) {
			QTreeWidgetItem *template_item =
					addTemplate(
						tbt_collection -> location(
							template_name),
						nullptr,
						options);
			tbt_collection_qtwi -> insertChild(index ++,
							   template_item);
		}
	}
	
	return(fillItem(tbt_collection_qtwi, options, freshly_created));
}

/**
	@brief GenericPanel::addTemplate
	@param tb_template
	@param parent_item
	@param options
	@return tb_template_qtwi
*/
QTreeWidgetItem *GenericPanel::addTemplate(
		const TitleBlockTemplateLocation &tb_template,
		QTreeWidgetItem *parent_item,
		PanelOptions options) {
	if (!tb_template.isValid()) return(nullptr);
	bool creation_required;
	
	QTreeWidgetItem *tb_template_qtwi = getItemForTemplate(tb_template,
							       &creation_required);
	updateTemplateItem(tb_template_qtwi,
			   tb_template,
			   options,
			   creation_required);
	reparent(tb_template_qtwi, parent_item);
	fillTemplateItem(tb_template_qtwi,
			 tb_template,
			 options,
			 creation_required);
	
	return(tb_template_qtwi);
}

/**
	@brief GenericPanel::getItemForTemplate
	@param tb_template
	@param created
	@return tb_template_qtwi
*/
QTreeWidgetItem *GenericPanel::getItemForTemplate(
		const TitleBlockTemplateLocation &tb_template,
		bool *created) {
	if (!tb_template.isValid()) return(nullptr);
	
	QTreeWidgetItem *tb_template_qtwi = tb_templates_.value(tb_template,
								nullptr);
	if (tb_template_qtwi) {
		if (created) *created = false;
		return(tb_template_qtwi);
	}
	
	tb_template_qtwi = makeItem(QET::TitleBlockTemplate);
	if (created) *created = true;
	return(tb_template_qtwi);
}

/**
	@brief GenericPanel::updateTemplateItem
	@param tb_template_qtwi
	@param tb_template
	@param options
	@param freshly_created
	@return updateItem(tb_template_qtwi, options, freshly_created)
*/
QTreeWidgetItem *GenericPanel::updateTemplateItem(
		QTreeWidgetItem *tb_template_qtwi,
		const TitleBlockTemplateLocation &tb_template,
		PanelOptions options,
		bool freshly_created) {
	tb_template_qtwi -> setText(0,
		tr("Modèle \"%1\"",
		   "used to display a title block template").arg(
					    tb_template.name()));
	QString tbt_whatsthis = tr(
		"Ceci est un modèle de cartouche, qui peut être appliqué à un folio.",
		"\"What's this\" tip"
	);
	tb_template_qtwi -> setWhatsThis(0, tbt_whatsthis);
	// note the following lines are technically marking the template as used
	tb_template_qtwi -> setToolTip(0, tb_template.toString());
	tb_template_qtwi -> setBackground(0, QBrush());
	
	// special action for templates that belong to a project
	if (QETProject *tbt_project = tb_template.parentProject()) {
		// display unused templates using a red background
		if (!tbt_project -> usesTitleBlockTemplate(tb_template)) {
			markItemAsUnused(tb_template_qtwi);
		}
	}
	
	if (freshly_created) {
		tb_template_qtwi -> setData(0,
					    GenericPanel::Item,
					    QVariant::fromValue(tb_template));
		tb_templates_.insert(tb_template, tb_template_qtwi);
	}
	return(updateItem(tb_template_qtwi, options, freshly_created));
}

/**
	@brief GenericPanel::fillTemplateItem
	@param tb_template_qtwi
	@param tb_template (unused)
	@param options
	@param freshly_created
	@return fillItem(tb_template_qtwi, options, freshly_created)
*/
QTreeWidgetItem *GenericPanel::fillTemplateItem(
		QTreeWidgetItem *tb_template_qtwi,
		const TitleBlockTemplateLocation &tb_template,
		PanelOptions options,
		bool freshly_created) {
	Q_UNUSED(tb_template)
	return(fillItem(tb_template_qtwi, options, freshly_created));
}

/**
	@brief GenericPanel::updateItem
	This generic method is called at the end of each update*Item method.
	Its only purpose is being reimplemented in a subclass.
	The default implementation does nothing.
	@param qtwi
	@param options (unused)
	@param freshly_created (unused)
	@return qtwi
*/
QTreeWidgetItem *GenericPanel::updateItem(QTreeWidgetItem *qtwi,
					  PanelOptions options,
					  bool freshly_created) {
	Q_UNUSED(options);
	Q_UNUSED(freshly_created);
	QApplication::processEvents();
	return(qtwi);
}

/**
	@brief GenericPanel::fillItem
	This generic method is called at the end of each fill*Item method.
	Its only purpose is being reimplemented in a subclass.
	The default implementation does nothing.
	@param qtwi
	@param options (unused)
	@param freshly_created (unused)
	@return qtwi
*/
QTreeWidgetItem *GenericPanel::fillItem(QTreeWidgetItem *qtwi,
					PanelOptions options,
					bool freshly_created) {
	Q_UNUSED(options);
	Q_UNUSED(freshly_created);
	return(qtwi);
}

/**
	@brief GenericPanel::projectInformationsChanged
	@param project
*/
void GenericPanel::projectInformationsChanged(QETProject *project) {
	addProject(project, nullptr, AddAllChild);
	emit(panelContentChanged());
}

/**
	@brief GenericPanel::diagramAdded
	@param project
	@param diagram (unused)
*/
void GenericPanel::diagramAdded(QETProject *project, Diagram *diagram) {
	Q_UNUSED(diagram)
	addProject(project, nullptr, GenericPanel::AddChildDiagrams);
	emit(panelContentChanged());
}

/**
	@brief GenericPanel::diagramRemoved
	@param project
	@param diagram (unused)
*/
void GenericPanel::diagramRemoved(QETProject *project, Diagram *diagram) {
	Q_UNUSED(diagram)
	addProject(project, nullptr, GenericPanel::AddChildDiagrams);
	emit(panelContentChanged());
}

/**
	@brief GenericPanel::projectDiagramsOrderChanged
	@param project project where diagram moved
	@param from Index of diagram before move
	@param to Index of diagram after move
*/
void GenericPanel::projectDiagramsOrderChanged(QETProject *project,
					       int from,
					       int to)
{
	// get the item representing the provided project
	QTreeWidgetItem *qtwi_project = itemForProject(project);
	if (!qtwi_project) return;
	
	// get the item representing the moved diagram
	QTreeWidgetItem *moved_qtwi_diagram = qtwi_project -> child(from);
	if (!moved_qtwi_diagram) return;
	
	// remove the QTWI then insert it back at the adequate location
	bool was_selected = moved_qtwi_diagram -> isSelected();
	qtwi_project -> removeChild (moved_qtwi_diagram);
	qtwi_project -> insertChild (to, moved_qtwi_diagram);
	
	// update the QTWI labels because they may display the folio index
	for (int i = qMin(from, to); i < qMax(from, to) + 1; i++)
	{
		QTreeWidgetItem *qtwi_diagram = qtwi_project -> child(i);
		if (!qtwi_diagram) continue;

		Diagram *diagram = valueForItem<Diagram *>(qtwi_diagram);
		if (diagram)
			updateDiagramItem(qtwi_diagram, diagram);
	}
	
	if (was_selected)
		setCurrentItem(moved_qtwi_diagram);

	emit(panelContentChanged());
}

/**
	@brief GenericPanel::diagramTitleChanged
	Inform this panel the diagram \a diagram has changed its title to \a title.
	@param diagram
	@param title (unused)
*/
void GenericPanel::diagramTitleChanged(Diagram *diagram, const QString &title) {
	Q_UNUSED(title)
	GenericPanel::addDiagram(diagram);
	emit(panelContentChanged());
}

/**
	@brief GenericPanel::templatesCollectionChanged
	@param collection :
	Title block templates collection that changed and should be updated
	@param template_name : Name of the changed template (unused)
*/
void GenericPanel::templatesCollectionChanged(
		TitleBlockTemplatesCollection*collection,
		const QString &template_name) {
	Q_UNUSED(template_name)
	addTemplatesCollection(collection);
	emit(panelContentChanged());
}

/**
	@brief GenericPanel::diagramUsedTemplate
	@param collection
	@param name : (unused)
*/
void GenericPanel::diagramUsedTemplate(
		TitleBlockTemplatesCollection *collection,
		const QString &name) {
	Q_UNUSED(name)
	addTemplatesCollection(collection);
	emit(panelContentChanged());
}

/**
	@brief GenericPanel::defaultText
	@param type
	@return the default text for \a type
*/
QString GenericPanel::defaultText(QET::ItemType type) {
	switch(type) {
		case QET::ElementsCollectionItem:
		case QET::Element: return("element");
		case QET::ElementsContainer:
		case QET::ElementsCategory: return("elements category");
		case QET::ElementsCollection: return("elements collection");
		case QET::TitleBlockTemplatesCollectionItem:
		case QET::TitleBlockTemplate: return("title block template");
		case QET::TitleBlockTemplatesCollection:
			return("title block templates collection");
		case QET::Diagram: return("diagram");
		case QET::Project: return("project");
		default: return(QString());
	}
	return(QString());
}

/**
	@brief GenericPanel::defaultIcon
	@param type Item type we want the default icon for
	@return the default icon for \a type
*/
QIcon GenericPanel::defaultIcon(QET::ItemType type) {
	if (type & QET::ElementsContainer) {
		return(QET::Icons::Folder);
	} else if (type & QET::TitleBlockTemplatesCollectionItem) {
		return(QIcon(QET::Icons::TitleBlock.pixmap(QSize(16, 16))));
	} else if (type == QET::Diagram) {
		return(QET::Icons::Diagram);
	} else if (type == QET::Project) {
		return(QIcon(QET::Icons::ProjectFileGP.pixmap(QSize(16, 16))));
	}
	return(QIcon());
}

/**
	@brief GenericPanel::makeItem
	Create a QTreeWidgetItem
	@param type Item type (e.g QET::Diagram, QET::Project, ...)
	@param parent Parent for the created item
	@param label Label for the created item
	@param icon Icon for the created item
	@return the create QTreeWidgetItem
*/
QTreeWidgetItem *GenericPanel::makeItem(QET::ItemType type,
					QTreeWidgetItem *parent,
					const QString &label,
					const QIcon &icon) {
	QTreeWidgetItem *qtwi = new QTreeWidgetItem(parent, type);
	qtwi -> setText(0, label.isEmpty() ? defaultText(type) : label);
	qtwi -> setIcon(0, icon.isNull() ? defaultIcon(type) : icon);
	return(qtwi);
}

/**
	@brief GenericPanel::deleteItem
	Delete and item and its children.
	@param item item to delete
	@param deleted_on_cascade true if the item is not being directly deleted
	but is undergoing the deletion of its parent. (unused)
*/
void GenericPanel::deleteItem(QTreeWidgetItem *item, bool deleted_on_cascade) {
	Q_UNUSED(deleted_on_cascade)
	// recursively delete child items first
	for (int i = item -> childCount() - 1 ; i >= 0 ; -- i) {
		deleteItem(item -> child(i), true);
	}
	
	// delete the item itself
	unregisterItem(item);
	delete item;
}

/**
	@brief GenericPanel::markItemAsUnused
	Mark the provided QTreeWidgetItem as unused in its parent project.
	@param qtwi A QTreeWidgetItem
*/
void GenericPanel::markItemAsUnused(QTreeWidgetItem *qtwi) {
	QLinearGradient t(0, 0, 200, 0);
	t.setColorAt(0, QColor("#ffc0c0"));
	t.setColorAt(1, QColor("#ffffff"));
	qtwi -> setBackground(0, QBrush(t));
	qtwi -> setToolTip(0,
			   QString(tr("%1 [non utilisé dans le projet]")).arg(
				   qtwi -> toolTip(0)));
}

/**
	@brief GenericPanel::reparent
	@param item
	@param parent
*/
void GenericPanel::reparent(QTreeWidgetItem *item, QTreeWidgetItem *parent) {
	if (parent && item -> parent() != parent) {
		parent -> addChild(item);
	}
}

/**
	@brief GenericPanel::childItems
	@param item Parent item that will be searched.
	@param type Type of items to look for.
	@param recursive Whether to search recursively.
	@return the child items of \a item of type \a type
*/
QList<QTreeWidgetItem *> GenericPanel::childItems(
		QTreeWidgetItem *item,
		QET::ItemType type,
		bool recursive) const
{
	QList<QTreeWidgetItem *> items;
	if (!item) return(items);
	for (int i = 0 ; i < item -> childCount() ; ++ i) {
		QTreeWidgetItem *current_item = item -> child(i);
		if (!current_item) continue;
		if (current_item -> type() == type) {
			items << current_item;
		}
		if (recursive) {
			items << childItems(current_item, type, true);
		}
	}
	return(items);
}

/**
	This variant of childItems() removes any child considered obsolete, i.e.
	not found in \a expected_items.
	@param expected_items A list of non-obsolete values
	@param item Parent item that will be searched.
	@param type Type of items to look for.
	@param recursive Whether to search recursively.
	@see GenericPanel::childItems()
*/
template<typename T>
void GenericPanel::removeObsoleteItems(
		const QList<T> &expected_items,
		QTreeWidgetItem *item,
		QET::ItemType type,
		bool recursive) {
	// remove items not found in expected_items
	foreach (QTreeWidgetItem *child_item,
		 childItems(item, type, recursive)) {
		T child_value = valueForItem<T>(child_item);
		if (!expected_items.contains(child_value)) {
			deleteItem(child_item);
		}
	}
}

/**
	@return the value stored in \a item
*/
template<typename T>
T GenericPanel::valueForItem(QTreeWidgetItem *item) const
{
	return item -> data(0, GenericPanel::Item).value<T>();
}
template TitleBlockTemplateLocation
GenericPanel::valueForItem(QTreeWidgetItem *item) const;
template ElementsLocation GenericPanel::valueForItem(QTreeWidgetItem*)
const;
template QETProject* GenericPanel::valueForItem(QTreeWidgetItem*)
const;
template Diagram* GenericPanel::valueForItem(QTreeWidgetItem*)
const;

/**
	@brief GenericPanel::unregisterItem
	@param item
*/
void GenericPanel::unregisterItem(QTreeWidgetItem *item) {
	if (!item) return;
	
	int type = item ->type();
	if (type & QET::TitleBlockTemplatesCollectionItem) {
		tb_templates_.remove(valueForItem<TitleBlockTemplateLocation>(item));
	} else if (type == QET::Diagram) {
		diagrams_.remove(valueForItem<Diagram *>(item));
	} else if (type == QET::Project) {
		projects_.remove(valueForItem<QETProject *>(item));
	}
}

/**
	@brief GenericPanel::event
	Handle various events; reimplemented here to emit the signal
	firstActivated().
	@param event
	@return
*/
bool GenericPanel::event(QEvent *event) {
	if (first_activation_) {
		if (event -> type() == QEvent::WindowActivate
				|| event -> type() == QEvent::Show) {
			QTimer::singleShot(250, this, SLOT(emitFirstActivated()));
			first_activation_ = false;
		}
	}
	return(QTreeWidget::event(event));
}

/**
	@brief GenericPanel::emitFirstActivated
	Emit the signal firstActivated().
*/
void GenericPanel::emitFirstActivated()
{
	emit(firstActivated());
}
