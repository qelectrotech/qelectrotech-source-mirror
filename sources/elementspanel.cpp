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
#include "elementspanel.h"
#include "qetapp.h"
#include "qetproject.h"
#include "diagram.h"
#include "qeticons.h"
#include "templatescollection.h"

/*
	Lorsque le flag ENABLE_PANEL_DND_CHECKS est defini, le panel d'elements
	effectue des verifications lors des drag'n drop d'elements et categories.
	Par exemple, il verifie qu'une categorie cible est accessible en ecriture
	avant d'y autoriser le drop d'un element.
	Supprimer ce flag permet de tester le comportement des fonctions de gestion
	des items (copy, move, etc.).
*/
#define ENABLE_PANEL_DND_CHECKS

/**
	Constructeur
	@param parent Le QWidget parent du panel d'appareils
*/
ElementsPanel::ElementsPanel(QWidget *parent) :
	GenericPanel(parent),
	first_reload_(true)
{
	// selection unique
	setSelectionMode(QAbstractItemView::SingleSelection);
	setColumnCount(1);
	setExpandsOnDoubleClick(true);
	setMouseTracking(true);
	
	// drag'n drop autorise
	setDragEnabled(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setAutoExpandDelay(1000);
	
	// force du noir sur une alternance de blanc (comme le schema) et de gris
	// clair, avec du blanc sur bleu pas trop fonce pour la selection
	QPalette qp = palette();
	qp.setColor(QPalette::Text,            Qt::black);
	qp.setColor(QPalette::Base,            Qt::white);
	qp.setColor(QPalette::AlternateBase,   QColor("#e8e8e8"));
	qp.setColor(QPalette::Highlight,       QColor("#678db2"));
	qp.setColor(QPalette::HighlightedText, Qt::black);
	setPalette(qp);
	
		// we handle double click on items ourselves
	connect(this, &ElementsPanel::itemDoubleClicked, this, &ElementsPanel::slot_doubleClick);
	connect(this, &GenericPanel::firstActivated, [this]() {QTimer::singleShot(250, this, SLOT(reload()));});
	connect(this, &ElementsPanel::panelContentChanged, this, &ElementsPanel::panelContentChange);
	
		//Emit a signal instead au manage is own context menu
	setContextMenuPolicy(Qt::CustomContextMenu);
}

/**
	Destructeur
*/
ElementsPanel::~ElementsPanel() {
}

/**
	Gere l'entree d'un drag'n drop. L'evenement est accepte si les donnees
	fournies contiennent un type MIME representant une categorie ou un element
	QET.
	@param e QDragEnterEvent decrivant l'entree du drag'n drop
*/
void ElementsPanel::dragEnterEvent(QDragEnterEvent *e) {
	if (e -> mimeData() -> hasFormat("application/x-qet-category-uri")) {
		e -> acceptProposedAction();
	} else if (e -> mimeData() -> hasFormat("application/x-qet-element-uri")) {
		e -> acceptProposedAction();
	}
}

/**
	Gere le debut des drag'n drop
	@param supportedActions Les actions supportees
*/
void ElementsPanel::startDrag(Qt::DropActions supportedActions) {
	Q_UNUSED(supportedActions);
	
	TitleBlockTemplateLocation tbt_location = selectedTemplateLocation();
	if (tbt_location.isValid()) {
		startTitleBlockTemplateDrag(tbt_location);
		return;
	}
}

/**
	Handle the dragging of a title block template
	@param location Location of the dragged template.
*/
void ElementsPanel::startTitleBlockTemplateDrag(const TitleBlockTemplateLocation &location) {
	QString location_string = location.toString();
	
	QMimeData *mime_data = new QMimeData();
	mime_data -> setText(location_string);
	mime_data -> setData("application/x-qet-titleblock-uri", location_string.toLatin1());
	
	QDrag *drag = new QDrag(this);
	drag -> setMimeData(mime_data);
	drag -> setPixmap(QET::Icons::TitleBlock.pixmap(22, 16));
	drag -> start(Qt::CopyAction);
}

/**
	Ensure the filter is applied again after the panel content has changed.
*/
void ElementsPanel::panelContentChange() {
	if (!filter_.isEmpty()) {
		filter(filter_);
	}
}

/**
	Methode permettant d'ajouter un projet au panel d'elements.
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere le projet
	@param project Projet a inserer dans le panel d'elements
	@return Le QTreeWidgetItem insere le plus haut
*/
QTreeWidgetItem *ElementsPanel::addProject(QETProject *project) {
	bool first_add = (first_reload_ || !projects_to_display_.contains(project));
	
	// create the QTreeWidgetItem representing the project
	QTreeWidgetItem *qtwi_project = GenericPanel::addProject(project, 0, GenericPanel::All);
	// the project will be inserted right before the common tb templates collection
	invisibleRootItem() -> insertChild(
		indexOfTopLevelItem(common_tbt_collection_item_),
		qtwi_project
	);
	if (first_add) qtwi_project -> setExpanded(true);
	
	if (TitleBlockTemplatesCollection *tbt_collection = project -> embeddedTitleBlockTemplatesCollection()) {
		if (QTreeWidgetItem *tbt_collection_qtwi = itemForTemplatesCollection(tbt_collection)) {
			if (first_add) tbt_collection_qtwi -> setExpanded(true);
		}
	}
	
	qtwi_project -> setStatusTip(0, tr("Double-cliquez pour réduire ou développer ce projet", "Status tip"));
	
	return(qtwi_project);
}

QTreeWidgetItem *ElementsPanel::updateTemplatesCollectionItem(QTreeWidgetItem *tbt_collection_qtwi, TitleBlockTemplatesCollection *tbt_collection, PanelOptions options, bool freshly_created) {
	QTreeWidgetItem *tbtc_qtwi = GenericPanel::updateTemplatesCollectionItem(tbt_collection_qtwi, tbt_collection, options, freshly_created);
	if (tbt_collection && tbt_collection -> parentProject()) {
		tbtc_qtwi -> setText(0, tr("Cartouches embarqués"));
		tbtc_qtwi -> setStatusTip(0, tr("Double-cliquez pour réduire ou développer cette collection de cartouches embarquée", "Status tip"));
	}
	return(tbtc_qtwi);
}

QTreeWidgetItem *ElementsPanel::updateTemplateItem(QTreeWidgetItem *tb_template_qtwi, const TitleBlockTemplateLocation &tb_template, PanelOptions options, bool freshly_created) {
	QTreeWidgetItem *item = GenericPanel::updateTemplateItem(tb_template_qtwi, tb_template, options, freshly_created);
	item -> setStatusTip(
		0,
		tr(
            "Glissez-déposez ce modèle de cartouche sur un folio pour l'y appliquer.",
			"Status tip displayed when selecting a title block template"
		)
	);
	return(item);
}

/**
	@return true if \a item matches the current filter, false otherwise
*/
bool ElementsPanel::matchesCurrentFilter(const QTreeWidgetItem *item) const {
	if (!item) return(false);
	
	// no filter => we consider the item matches
	if (filter_.isEmpty()) return(true);
	
	bool item_matches = item -> text(0).contains(filter_, Qt::CaseInsensitive);
	
	return(item_matches);
}

/**
	@return true if \a item matches the  filter, false otherwise
*/
bool ElementsPanel::matchesFilter(const QTreeWidgetItem *item, QString filter) const {
	if (!item) return(false);
	
	// no filter => we consider the item matches
	if (filter.isEmpty()) return(true);
	
	bool item_matches = item -> text(0).contains(filter, Qt::CaseInsensitive);
	
	return(item_matches);
}

/**
 * @brief ElementsPanel::reload
 * Reload the elements tree
 * @param reload_collections true for read all collections since their sources (files, projects ...)
 */
void ElementsPanel::reload(bool reload_collections) {

	Q_UNUSED(reload_collections);
	
	QIcon system_icon(":/ico/16x16/qet.png");
	QIcon user_icon(":/ico/16x16/go-home.png");
	
	// load the common title block templates collection
	TitleBlockTemplatesCollection *common_tbt_collection = QETApp::commonTitleBlockTemplatesCollection();
	common_tbt_collection_item_ = addTemplatesCollection(common_tbt_collection, invisibleRootItem());
	common_tbt_collection_item_ -> setIcon(0, system_icon);
	common_tbt_collection_item_ -> setStatusTip(0, tr("Double-cliquez pour réduire ou développer la collection de cartouches QElectroTech", "Status tip"));
	common_tbt_collection_item_ -> setWhatsThis(0, tr("Ceci est la collection de cartouches fournie avec QElectroTech. Installée en tant que composant système, vous ne pouvez normalement pas la personnaliser.", "\"What's this\" tip"));
	if (first_reload_) common_tbt_collection_item_ -> setExpanded(true);
	
	// load the custom title block templates collection
	TitleBlockTemplatesCollection *custom_tbt_collection = QETApp::customTitleBlockTemplatesCollection();
	custom_tbt_collection_item_ = addTemplatesCollection(custom_tbt_collection, invisibleRootItem());
	custom_tbt_collection_item_ -> setIcon(0, user_icon);
	custom_tbt_collection_item_ -> setStatusTip(0, tr("Double-cliquez pour réduire ou développer votre collection personnelle de cartouches", "Status tip"));
	custom_tbt_collection_item_ -> setWhatsThis(0, tr("Ceci est votre collection personnelle de cartouches -- utilisez-la pour créer, stocker et éditer vos propres cartouches.", "\"What's this\" tip"));
	if (first_reload_) custom_tbt_collection_item_ -> setExpanded(true);
	
	// add projects
	foreach(QETProject *project, projects_to_display_.values()) {
		addProject(project);
	}
	
	// the first time, expand the first level of collections
	if (first_reload_) first_reload_ = false;
}

/**
	Gere le double-clic sur un element.
	Si un double-clic sur un projet est effectue, le signal requestForProject
	est emis.
	Si un double-clic sur un schema est effectue, le signal requestForDiagram
	est emis.
	@param qtwi
*/
void ElementsPanel::slot_doubleClick(QTreeWidgetItem *qtwi, int) {
	int qtwi_type = qtwi -> type();
	if (qtwi_type == QET::Project) {
		QETProject *project = valueForItem<QETProject *>(qtwi);
		emit(requestForProject(project));
	} else if (qtwi_type == QET::Diagram) {
		Diagram *diagram = valueForItem<Diagram *>(qtwi);
		emit(requestForDiagram(diagram));
	} else if (qtwi_type == QET::TitleBlockTemplate) {
		TitleBlockTemplateLocation tbt = valueForItem<TitleBlockTemplateLocation>(qtwi);
		emit(requestForTitleBlockTemplate(tbt));
	}
}

/**
	@param qtwi a QTreeWidgetItem
	@return the directory path of the object represented by \a qtwi
*/
QString ElementsPanel::dirPathForItem(QTreeWidgetItem *item) {
	QString file_path = filePathForItem(item);
	if (!file_path.isEmpty()) {
		QFileInfo path_info(file_path);
		if (path_info.isDir()) {
			return(file_path);
		}
		else {
			return(path_info.canonicalPath());
		}
	}
	return(QString());
}

/**
	@param qtwi a QTreeWidgetItem
	@return the filepath of the object represented by \a qtwi
*/
QString ElementsPanel::filePathForItem(QTreeWidgetItem *item) {
	if (!item) return(QString());
	
	TitleBlockTemplateLocation tbt_location = templateLocationForItem(item);
	TitleBlockTemplatesCollection *tbt_collection = tbt_location.parentCollection();
	if (tbt_collection && tbt_collection -> hasFilePath()) {
		return(tbt_collection -> filePath());
	}
	else {
		QETProject *project = projectForItem(item);
		if (project) {
			return(project -> filePath());
		}
	}
	return(QString());
}

/**
	Hide items that do not match the provided string, ensure others are visible
	along with their parent hierarchy. When ending the filtering, restore the tree
	as it was before the filtering (except the current item) and scroll to the
	currently selected item.
	@param m String to be matched
	@param filtering whether to begin/apply/end the filtering
	@see QET::Filtering
*/
void ElementsPanel::filter(const QString &m, QET::Filtering filtering) {
	QList<QTreeWidgetItem *> items = findItems("*", Qt::MatchRecursive | Qt::MatchWildcard);
	const int expanded_role = 42; // magic number? So you consider Douglas Adams wrote about magic?
	
	if (filtering == QET::BeginFilter) {
		foreach (QTreeWidgetItem *item, items) {
			item -> setData(0, expanded_role, item -> isExpanded());
		}
	}
	
	if (filtering != QET::EndFilter) {
		filter_ = m;
		applyCurrentFilter(items);
	} else { // filtering == QET::EndFilter
		filter_ = QString();
		QTreeWidgetItem *current_item = currentItem();
		
		// restore the tree as it was before the filtering
		foreach (QTreeWidgetItem *qtwi, items) {
			qtwi -> setHidden(false);
			qtwi -> setExpanded(qtwi -> data(0, expanded_role).toBool());
		}
		
		// avoid hiding the currently selected item
		if (current_item) {
			ensureHierarchyIsVisible(QList<QTreeWidgetItem *>() << current_item);
			scrollToItem(current_item);
		}
	}
}

/**
	Rajoute un projet au panel d'elements
	@param project Projet ouvert a rajouter au panel
*/
void ElementsPanel::projectWasOpened(QETProject *project) {
	addProject(project);
	projects_to_display_ << project;
	emit(panelContentChanged());
}

/**
	Enleve un projet du panel d'elements
	@param project Projet a enlever du panel
*/
void ElementsPanel::projectWasClosed(QETProject *project) {
	if (QTreeWidgetItem *item_to_remove = itemForProject(project)) {
		GenericPanel::deleteItem(item_to_remove);
		projects_to_display_.remove(project);
	}
	emit(panelContentChanged());
}

/**
	Build filter list for multiple filter
*/
void ElementsPanel::buildFilterList() {
	if (filter_.isEmpty()) return;
	filter_list_ = filter_.split( '+' );
	/*
	qDebug() << "*******************";
	foreach( QString filter , filter_list_ )	{
		filter = filter.trimmed();
		qDebug() << filter;
	}
	*/
}

/**
	Apply the current filter to a given item.
*/
void ElementsPanel::applyCurrentFilter(const QList<QTreeWidgetItem *> &items) {
	if (filter_.isEmpty()) return;
	buildFilterList();
	QList<QTreeWidgetItem *> matching_items;

	foreach (QTreeWidgetItem *item, items) {
		bool item_matches = true;

		foreach( QString filter , filter_list_ )	{
			filter = filter.trimmed();
			if ( !filter.isEmpty() )	{
				item_matches &= matchesFilter(item, filter);
			}
		}
		if (item_matches) matching_items << item;
		item -> setHidden(!item_matches);
	}
	ensureHierarchyIsVisible(matching_items);
}

/**
	@param items une liste de QTreeWidgetItem pour lesquels il faut s'assurer
	que eux et leurs parents sont visibles
*/
void ElementsPanel::ensureHierarchyIsVisible(const QList<QTreeWidgetItem *> &items) {
	// remonte l'arborescence pour lister les categories contenant les elements filtres
	QSet<QTreeWidgetItem *> parent_items;
	foreach(QTreeWidgetItem *item, items) {
		for (QTreeWidgetItem *parent_qtwi = item -> parent() ; parent_qtwi ; parent_qtwi = parent_qtwi -> parent()) {
			parent_items << parent_qtwi;
		}
	}
	
	// etend les parents
	foreach(QTreeWidgetItem *parent_qtwi, parent_items) {
		if (!parent_qtwi -> isExpanded()) parent_qtwi -> setExpanded(true);
	}
	
	// affiche les parents
	foreach(QTreeWidgetItem *parent_qtwi, parent_items) {
		if (parent_qtwi -> isHidden()) parent_qtwi -> setHidden(false);
	}
}
