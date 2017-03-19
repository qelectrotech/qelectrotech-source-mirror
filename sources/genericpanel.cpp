/*
	Copyright 2006-2017 The QElectroTech Team
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
GenericPanel::~GenericPanel() {
}

/**
	@return the type of the current item
*/
int GenericPanel::currentItemType() {
	QTreeWidgetItem *current_qtwi = currentItem();
	if (!current_qtwi) return(0);
	return(current_qtwi -> type());
}

QETProject *GenericPanel::projectForItem(QTreeWidgetItem *item) const {
	if (item && item -> type() == QET::Project) {
		return(valueForItem<QETProject *>(item));
	}
	return(0);
	
}

/**
	
*/
Diagram *GenericPanel::diagramForItem(QTreeWidgetItem *item) const {
	if (item && item -> type() == QET::Diagram) {
		return(valueForItem<Diagram *>(item));
	}
	return(0);
}

/**
	
*/
TitleBlockTemplateLocation GenericPanel::templateLocationForItem(QTreeWidgetItem *item) const {
	if (item && item -> type() & QET::TitleBlockTemplatesCollectionItem) {
		return(valueForItem<TitleBlockTemplateLocation>(item));
	}
	return(TitleBlockTemplateLocation());
}

/**
	
*/
QETProject *GenericPanel::selectedProject() const {
	return(projectForItem(currentItem()));
}

/**
	
*/
Diagram *GenericPanel::selectedDiagram() const {
	return(diagramForItem(currentItem()));
}

/**
	
*/
TitleBlockTemplateLocation GenericPanel::selectedTemplateLocation() const {
	return(templateLocationForItem(currentItem()));
}

/**
	
*/
QTreeWidgetItem *GenericPanel::addProject(QETProject *project, QTreeWidgetItem *parent_item, PanelOptions options) {
	if (!project) return(0);
	bool creation_required;
	
	QTreeWidgetItem *project_qtwi = getItemForProject(project, &creation_required);
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
	if (!project) return(0);
	return(projects_.value(project, 0));
}

/**
	@param project A standard project.
	@param created if provided with a pointer to a boolean, this method will
	update it to reflect whether the returned item has been freshly created or
	not.
	@return the tree item representing the provided project. If it does not
	appear within this panel, it is created.
*/
QTreeWidgetItem *GenericPanel::getItemForProject(QETProject *project, bool *created) {
	if (!project) return(0);
	
	QTreeWidgetItem *project_qtwi = projects_.value(project, 0);
	if (project_qtwi) {
		if (created) *created = false;
		return(project_qtwi);
	}
	
	project_qtwi = makeItem(QET::Project);
	if (created) *created = true;
	return(project_qtwi);
}

/**
	
*/
QTreeWidgetItem *GenericPanel::updateProjectItem(QTreeWidgetItem *project_qtwi, QETProject *project, PanelOptions options, bool freshly_created) {
	Q_UNUSED(options)
	if (!project_qtwi || !project) return(0);
	
	if (freshly_created) {
		project_qtwi -> setData(0, GenericPanel::Item, qVariantFromValue(project));
		projects_.insert(project, project_qtwi);
		
		connect(
			project, SIGNAL(projectInformationsChanged(QETProject *)),
			this,    SLOT  (projectInformationsChanged(QETProject *))
		);
		connect(
			project, SIGNAL(readOnlyChanged(QETProject *, bool)),
			this,    SLOT  (projectInformationsChanged(QETProject *))
		);
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
    QString project_whatsthis = tr("Ceci est un projet QElectroTech, c'est-à-dire un fichier d'extension .qet regroupant plusieurs folios. Il embarque également les éléments et modèles de cartouches utilisés dans ces folios.", "\"What's this\" tip");
	project_qtwi -> setWhatsThis(0, project_whatsthis);
	return(updateItem(project_qtwi, options, freshly_created));
}

/**
	
*/
QTreeWidgetItem *GenericPanel::fillProjectItem(QTreeWidgetItem *project_qtwi, QETProject *project, PanelOptions options, bool freshly_created) {
	if (!project_qtwi || !project) return(0);
	
	
	if (options & AddChildDiagrams) {
		if (freshly_created) {
			connect(
				project, SIGNAL(diagramAdded(QETProject *, Diagram *)),
				this,    SLOT  (diagramAdded(QETProject *, Diagram *))
			);
			connect(
				project, SIGNAL(diagramRemoved(QETProject *, Diagram *)),
				this,    SLOT  (diagramRemoved(QETProject *, Diagram *))
			);
			connect(
				project, SIGNAL(projectDiagramsOrderChanged(QETProject *, int, int)),
				this,    SLOT  (projectDiagramsOrderChanged(QETProject *, int, int))
			);
		} else {
			// remove diagrams unknown to the project (presumably removed)
			removeObsoleteItems(project -> diagrams(), project_qtwi, QET::Diagram, false);
		}
		int index = 0;
		foreach (Diagram *diagram, project -> diagrams()) {
			QTreeWidgetItem *diagram_qtwi = addDiagram(diagram, 0, options);
			project_qtwi -> insertChild(index, diagram_qtwi);
			++ index;
		}
	}
	
	if (options & AddChildTemplatesCollection) {
		if (freshly_created) {
			connect(
				project, SIGNAL(diagramUsedTemplate(TitleBlockTemplatesCollection *, const QString &)),
				this,    SLOT  (diagramUsedTemplate(TitleBlockTemplatesCollection *, const QString &))
			);
		}
		addTemplatesCollection(
			project -> embeddedTitleBlockTemplatesCollection(),
			project_qtwi,
			options
		);
	}

	return(fillItem(project_qtwi, options, freshly_created));
}

/**
	
*/
QTreeWidgetItem *GenericPanel::addDiagram(Diagram *diagram, QTreeWidgetItem *parent_item, PanelOptions options) {
	Q_UNUSED(options)
	if (!diagram) return(0);
	
	bool creation_required;
	
	QTreeWidgetItem *diagram_qtwi = getItemForDiagram(diagram, &creation_required);
	updateDiagramItem(diagram_qtwi, diagram, options, creation_required);
	reparent(diagram_qtwi, parent_item);
	fillDiagramItem(diagram_qtwi, diagram, options, creation_required);
	
	return(diagram_qtwi);
}

/**
	
*/
QTreeWidgetItem *GenericPanel::getItemForDiagram(Diagram *diagram, bool *created) {
	if (!diagram) return(0);
	
	QTreeWidgetItem *diagram_qtwi = diagrams_.value(diagram, 0);
	if (diagram_qtwi) {
		if (created) *created = false;
		return(diagram_qtwi);
	}
	
	diagram_qtwi = makeItem(QET::Diagram);
	if (created) *created = true;
	return(diagram_qtwi);
}

/**
	
*/
QTreeWidgetItem *GenericPanel::updateDiagramItem(QTreeWidgetItem *diagram_qtwi, Diagram *diagram, PanelOptions options, bool freshly_created) {
	Q_UNUSED(options)
	if (!diagram || !diagram_qtwi) return(0);
	QSettings settings;
	
	QString displayed_title = diagram -> title();
	if (displayed_title.isEmpty())
	{
		displayed_title = tr("Folio sans titre", "Fallback label when a diagram has no title");
	}

	if (settings.value("genericpanel/folio", true).toBool())
	{
		QString displayed_label = diagram ->border_and_titleblock.finalfolio();
		int diagram_folio_idx = diagram -> folioIndex();
		if (diagram_folio_idx != -1)
		{
			displayed_label = QString(
						tr(
							"%1 - %2",
							"label displayed for a diagram in the panel ; %1 is the folio index, %2 is the diagram title"
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
							"label displayed for a diagram in the panel ; %1 is the folio index, %2 is the diagram title"
							)
						).arg(diagram_folio_idx + 1).arg(displayed_title);
		}

		diagram_qtwi -> setText(0, displayed_label);

	}
	if (freshly_created)
	{
		diagram_qtwi -> setData(0, GenericPanel::Item, qVariantFromValue(diagram));
		diagrams_.insert(diagram, diagram_qtwi);
		
		connect(diagram, &Diagram::diagramTitleChanged, this, &GenericPanel::diagramTitleChanged);
	}
	
	return(updateItem(diagram_qtwi, options, freshly_created));
}



/**
	
*/
QTreeWidgetItem *GenericPanel::fillDiagramItem(QTreeWidgetItem *diagram_qtwi, Diagram *diagram, PanelOptions options, bool freshly_created) {
	Q_UNUSED(diagram)
	Q_UNUSED(options)
	Q_UNUSED(freshly_created)
	return(fillItem(diagram_qtwi, options, freshly_created));
}

/**
	
*/
QTreeWidgetItem *GenericPanel::addTemplatesCollection(TitleBlockTemplatesCollection *tbt_collection, QTreeWidgetItem *parent_item, PanelOptions options) {
	if (!tbt_collection) return(0);
	bool creation_required;
	
	QTreeWidgetItem *tbt_collection_qtwi = getItemForTemplatesCollection(tbt_collection, &creation_required);
	updateTemplatesCollectionItem(tbt_collection_qtwi, tbt_collection, options, creation_required);
	reparent(tbt_collection_qtwi, parent_item);
	fillTemplatesCollectionItem(tbt_collection_qtwi, tbt_collection, options, creation_required);
	
	return(tbt_collection_qtwi);
}

/**
	
*/
QTreeWidgetItem *GenericPanel::itemForTemplatesCollection(TitleBlockTemplatesCollection *tbt_collection) {
	if (!tbt_collection) return(0);
	return(tb_templates_.value(tbt_collection -> location(), 0));
}

/**
	
*/
QTreeWidgetItem *GenericPanel::getItemForTemplatesCollection(TitleBlockTemplatesCollection *tbt_collection, bool *created) {
	if (!tbt_collection) return(0);
	QTreeWidgetItem *tbt_collection_item = tb_templates_.value(tbt_collection -> location(), 0);
	if (tbt_collection_item) {
		if (created) *created = false;
		return(tbt_collection_item);
	}
	
	tbt_collection_item = makeItem(QET::TitleBlockTemplatesCollection);
	if (created) *created = true;
	return(tbt_collection_item);
}

/**
	
*/
QTreeWidgetItem *GenericPanel::updateTemplatesCollectionItem(QTreeWidgetItem *tbt_collection_qtwi, TitleBlockTemplatesCollection *tbt_collection, PanelOptions options, bool freshly_created) {
	Q_UNUSED(options)
	QString label = tbt_collection -> title();
	if (label.isEmpty()) label = tr("Modèles de cartouche");
	
	tbt_collection_qtwi -> setText(0, label);
	tbt_collection_qtwi -> setToolTip(0, tbt_collection -> location().toString());
	
	if (freshly_created) {
		tbt_collection_qtwi -> setData(0, GenericPanel::Item, qVariantFromValue(tbt_collection -> location()));
		tb_templates_.insert(tbt_collection -> location(), tbt_collection_qtwi);
	}
	
	return(updateItem(tbt_collection_qtwi, options, freshly_created));
}

/**
	
*/
QTreeWidgetItem *GenericPanel::fillTemplatesCollectionItem(QTreeWidgetItem *tbt_collection_qtwi, TitleBlockTemplatesCollection *tbt_collection, PanelOptions options, bool freshly_created) {
	if (!tbt_collection_qtwi || !tbt_collection) return(tbt_collection_qtwi);
	
	if (options & AddChildTemplates) {
		if (freshly_created) {
			connect(
				tbt_collection, SIGNAL(changed(TitleBlockTemplatesCollection*,QString)),
				this, SLOT(templatesCollectionChanged(TitleBlockTemplatesCollection*, const QString &))
			);
			if (QETProject *project = tbt_collection -> parentProject()) {
				connect(
					project, SIGNAL(diagramUsedTemplate(TitleBlockTemplatesCollection *, const QString &)),
					this,    SLOT  (templatesCollectionChanged(TitleBlockTemplatesCollection *, const QString &))
				);
			}
		} else {
			// remove templates unknown to the collection (presumably removed)
			removeObsoleteItems(tbt_collection -> templatesLocations(), tbt_collection_qtwi, QET::TitleBlockTemplate, false);
		}
		
		int index = 0;
		foreach (QString template_name, tbt_collection -> templates()) {
			QTreeWidgetItem *template_item = addTemplate(tbt_collection -> location(template_name), 0, options);
			tbt_collection_qtwi -> insertChild(index ++, template_item);
		}
	}
	
	return(fillItem(tbt_collection_qtwi, options, freshly_created));
}

/**
	
*/
QTreeWidgetItem *GenericPanel::addTemplate(const TitleBlockTemplateLocation &tb_template, QTreeWidgetItem *parent_item, PanelOptions options) {
	if (!tb_template.isValid()) return(0);
	bool creation_required;
	
	QTreeWidgetItem *tb_template_qtwi = getItemForTemplate(tb_template, &creation_required);
	updateTemplateItem(tb_template_qtwi, tb_template, options, creation_required);
	reparent(tb_template_qtwi, parent_item);
	fillTemplateItem(tb_template_qtwi, tb_template, options, creation_required);
	
	return(tb_template_qtwi);
}

/**
	
*/
QTreeWidgetItem *GenericPanel::getItemForTemplate(const TitleBlockTemplateLocation &tb_template, bool *created) {
	if (!tb_template.isValid()) return(0);
	
	QTreeWidgetItem *tb_template_qtwi = tb_templates_.value(tb_template, 0);
	if (tb_template_qtwi) {
		if (created) *created = false;
		return(tb_template_qtwi);
	}
	
	tb_template_qtwi = makeItem(QET::TitleBlockTemplate);
	if (created) *created = true;
	return(tb_template_qtwi);
}

/**
	
*/
QTreeWidgetItem *GenericPanel::updateTemplateItem(QTreeWidgetItem *tb_template_qtwi, const TitleBlockTemplateLocation &tb_template, PanelOptions options, bool freshly_created) {
	Q_UNUSED(options)
	tb_template_qtwi -> setText(0, tr("Modèle \"%1\"", "used to display a title block template").arg(tb_template.name()));
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
		tb_template_qtwi -> setData(0, GenericPanel::Item, qVariantFromValue(tb_template));
		tb_templates_.insert(tb_template, tb_template_qtwi);
	}
	return(updateItem(tb_template_qtwi, options, freshly_created));
}

/**
	
*/
QTreeWidgetItem *GenericPanel::fillTemplateItem(QTreeWidgetItem *tb_template_qtwi, const TitleBlockTemplateLocation &tb_template, PanelOptions options, bool freshly_created) {
	Q_UNUSED(tb_template)
	Q_UNUSED(options)
	Q_UNUSED(freshly_created)
	return(fillItem(tb_template_qtwi, options, freshly_created));
}

/**
	This generic method is called at the end of each update*Item method. Its
	only purpose is being reimplemented in a subclass. The default
	implementation does nothing.
*/
QTreeWidgetItem *GenericPanel::updateItem(QTreeWidgetItem *qtwi, PanelOptions options, bool freshly_created) {
	Q_UNUSED(qtwi);
	Q_UNUSED(options);
	Q_UNUSED(freshly_created);
	QApplication::processEvents();
	return(qtwi);
}

/**
	This generic method is called at the end of each fill*Item method. Its
	only purpose is being reimplemented in a subclass. The default
	implementation does nothing.
*/
QTreeWidgetItem *GenericPanel::fillItem(QTreeWidgetItem *qtwi, PanelOptions options, bool freshly_created) {
	Q_UNUSED(qtwi);
	Q_UNUSED(options);
	Q_UNUSED(freshly_created);
	return(qtwi);
}

/**
	
*/
void GenericPanel::projectInformationsChanged(QETProject *project) {
	addProject(project, 0, 0);
	emit(panelContentChanged());
}

/**
	
*/
void GenericPanel::diagramAdded(QETProject *project, Diagram *diagram) {
	Q_UNUSED(diagram)
	addProject(project, 0, GenericPanel::AddChildDiagrams);
	emit(panelContentChanged());
}

/**
	
*/
void GenericPanel::diagramRemoved(QETProject *project, Diagram *diagram) {
	Q_UNUSED(diagram)
	addProject(project, 0, GenericPanel::AddChildDiagrams);
	emit(panelContentChanged());
}

/**
 * @brief GenericPanel::projectDiagramsOrderChanged
 * @param project project where diagram moved
 * @param from Index of diagram before move
 * @param to Index of diagram after move
 */
void GenericPanel::projectDiagramsOrderChanged(QETProject *project, int from, int to)
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
	Inform this panel the diagram \a diagram has changed its title to \a title.
*/
void GenericPanel::diagramTitleChanged(Diagram *diagram, const QString &title) {
	Q_UNUSED(title)
	GenericPanel::addDiagram(diagram);
	emit(panelContentChanged());
}

/**
	@param collection Title block templates collection that changed and should be updated
	@param template_name Name of the changed template (unused)
*/
void GenericPanel::templatesCollectionChanged(TitleBlockTemplatesCollection*collection, const QString &template_name) {
	Q_UNUSED(template_name)
	addTemplatesCollection(collection);
	emit(panelContentChanged());
}

/**
	
*/
void GenericPanel::diagramUsedTemplate(TitleBlockTemplatesCollection *collection, const QString &name) {
	Q_UNUSED(collection)
	Q_UNUSED(name)
	addTemplatesCollection(collection);
	emit(panelContentChanged());
}

/**
	
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
		case QET::TitleBlockTemplatesCollection: return("title block templates collection");
		case QET::Diagram: return("diagram");
		case QET::Project: return("project");
		default: return(QString());
	}
	return(QString());
}

/**
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
	Create a QTreeWidgetItem
	@param parent Parent for the created item
	@param type Item type (e.g QET::Diagram, QET::Project, ...)
	@param label Label for the created item
	@param icon Icon for the created item
	@return the create QTreeWidgetItem
*/
QTreeWidgetItem *GenericPanel::makeItem(QET::ItemType type, QTreeWidgetItem *parent, const QString &label, const QIcon &icon) {
	QTreeWidgetItem *qtwi = new QTreeWidgetItem(parent, type);
	qtwi -> setText(0, label.isEmpty() ? defaultText(type) : label);
	qtwi -> setIcon(0, icon.isNull() ? defaultIcon(type) : icon);
	return(qtwi);
}

/**
	Delete and item and its children.
	@param item item to delete
	@param deleted_on_cascade true if the item is not being directly deleted
	but is undergoing the deletion of its parent.
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
	Mark the provided QTreeWidgetItem as unused in its parent project.
	@param qtwi A QTreeWidgetItem
*/
void GenericPanel::markItemAsUnused(QTreeWidgetItem *qtwi) {
	QLinearGradient t(0, 0, 200, 0);
	t.setColorAt(0, QColor("#ffc0c0"));
	t.setColorAt(1, QColor("#ffffff"));
	qtwi -> setBackground(0, QBrush(t));
	qtwi -> setToolTip(0, QString(tr("%1 [non utilisé dans le projet]")).arg(qtwi -> toolTip(0)));
}

/**
	
*/
void GenericPanel::reparent(QTreeWidgetItem *item, QTreeWidgetItem *parent) {
	if (parent && item -> parent() != parent) {
		parent -> addChild(item);
	}
}

/**
	@return the child items of \a item of type \a type
	@param item Parent item that will be searched.
	@param type Type of items to look for.
	@param recursive Whether to search recursively.
*/
QList<QTreeWidgetItem *> GenericPanel::childItems(QTreeWidgetItem *item, QET::ItemType type, bool recursive) const {
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
void GenericPanel::removeObsoleteItems(const QList<T> &expected_items, QTreeWidgetItem *item, QET::ItemType type, bool recursive) {
	// remove items not found in expected_items
	foreach (QTreeWidgetItem *child_item, childItems(item, type, recursive)) {
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
T GenericPanel::valueForItem(QTreeWidgetItem *item) const {
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
	Handle various events; reimplemented here to emit the signal
	firstActivated().
*/
bool GenericPanel::event(QEvent *event) {
	if (first_activation_) {
		if (event -> type() == QEvent::WindowActivate || event -> type() == QEvent::Show) {
			QTimer::singleShot(250, this, SLOT(emitFirstActivated()));
			first_activation_ = false;
		}
	}
	return(QTreeWidget::event(event));
}

/**
	Emit the signal firstActivated().
*/
void GenericPanel::emitFirstActivated() {
	emit(firstActivated());
}
