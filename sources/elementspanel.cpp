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
#include "elementscategory.h"
#include "elementscollectioncache.h"
#include "factory/elementfactory.h"
#include "fileelementscollection.h"
#include "fileelementdefinition.h"
#include "qeticons.h"
#include "templatescollection.h"
#include "treecoloranimation.h"
#include "element.h"

/*
	Lorsque le flag ENABLE_PANEL_DND_CHECKS est defini, le panel d'elements
	effectue des verifications lors des drag'n drop d'elements et categories.
	Par exemple, il verifie qu'une categorie cible est accessible en ecriture
	avant d'y autoriser le drop d'un element.
	Supprimer ce flag permet de tester le comportement des fonctions de gestion
	des items (copy, move, etc.).
*/
#define ENABLE_PANEL_DND_CHECKS

/*
	Largeur maximale, en pixels, de la pixmap accrochee au pointeur de la
	souris
*/
#define QET_MAX_DND_PIXMAP_WIDTH 500

/*
	Hauteur maximale, en pixels, de la pixmap accrochee au pointeur de la
	souris
*/
#define QET_MAX_DND_PIXMAP_HEIGHT 375

/**
	Constructeur
	@param parent Le QWidget parent du panel d'appareils
*/
ElementsPanel::ElementsPanel(QWidget *parent) :
	GenericPanel(parent),
	common_collection_item_(0),
	custom_collection_item_(0),
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
	
	setElementsCache(QETApp::collectionCache());	
}

/**
	Destructeur
*/
ElementsPanel::~ElementsPanel() {
}

/**
	@param qtwi Un QTreeWidgetItem
	@return true si qtwi represente un element, false sinon
*/
bool ElementsPanel::itemIsWritable(QTreeWidgetItem *qtwi) const {
	if (ElementsCollectionItem *qtwi_item = collectionItemForItem(qtwi)) {
		return(qtwi_item -> isWritable());
	}
	return(false);
}


/**
	@return true si l'item selectionne est accessible en ecriture, false sinon
*/
bool ElementsPanel::selectedItemIsWritable() const {
	if (ElementsCollectionItem *selected_item = selectedItem()) {
		return(selected_item -> isWritable());
	}
	return(false);
}

/**
	@return la collection, la categorie ou l'element selectionne(e)
*/
ElementsCollectionItem *ElementsPanel::selectedItem() const {


	ElementsLocation selected_location(selectedElementLocation());
	if (!selected_location.isNull()) {
		return(QETApp::collectionItem(selected_location));
	}
	return(0);
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
	Gere le mouvement lors d'un drag'n drop
*/
void ElementsPanel::dragMoveEvent(QDragMoveEvent *e)
{
	// scrolle lorsque le curseur est pres des bords
	int limit = 40;
	QScrollBar *scroll_bar = verticalScrollBar();
	if (e -> pos().y() < limit) {
		scroll_bar -> setValue(scroll_bar -> value() - 1);
	} else if (e -> pos().y() > height() - limit) {
		scroll_bar -> setValue(scroll_bar -> value() + 1);
	}
	
	QTreeWidget::dragMoveEvent(e);
	
	// recupere la categorie cible pour le deplacement / la copie
	ElementsCategory *target_category = categoryForPos(e -> pos());
	if (!target_category) {
		e -> ignore();
		return;
	}
	
		// recupere la source (categorie ou element) pour le deplacement / la copie
	ElementsLocation dropped_location = ElementsLocation(e -> mimeData() -> text());
	ElementsCollectionItem *source_item = QETApp::collectionItem(dropped_location, false);
	if (!source_item) {
		e -> ignore();
		return;
	}
	
#ifdef ENABLE_PANEL_DND_CHECKS
	// ne prend pas en consideration le drop d'un item sur lui-meme ou une categorie imbriquee
	if (
		source_item -> location() == target_category -> location() ||\
		target_category -> isChildOf(source_item)
	) {
		e -> ignore();
		return;
	}
	
	// s'assure que la categorie cible est accessible en ecriture
	if (!target_category -> isWritable()) {
		e -> ignore();
		return;
	}
#endif


	e -> accept();
	/// @todo mettre en valeur le lieu de depot
}

/**
	Gere le depot lors d'un drag'n drop
	@param e QDropEvent decrivant le depot
*/
void ElementsPanel::dropEvent(QDropEvent *e)
{
		// recupere la categorie cible pour le deplacement / la copie
	ElementsCategory *target_category = categoryForPos(e -> pos());
	if (!target_category) {
		e -> ignore();
		return;
	}
	
		// recupere la source (categorie ou element) pour le deplacement / la copie
	ElementsLocation dropped_location = ElementsLocation(e -> mimeData() -> text());
	ElementsCollectionItem *source_item = QETApp::collectionItem(dropped_location, false);
	if (!source_item) {
		e -> ignore();
		return;
	}
	
#ifdef ENABLE_PANEL_DND_CHECKS
	// ne prend pas en consideration le drop d'un item sur lui-meme ou une categorie imbriquee
	if (
		source_item -> location() == target_category -> location() ||\
		target_category -> isChildOf(source_item)
	) {
		e -> ignore();
		return;
	}
	
	// s'assure que la categorie cible est accessible en ecriture
	if (!target_category -> isWritable()) {
		e -> ignore();
		return;
	}
#endif
	
	e -> accept();
	emit(requestForMoveElements(source_item, target_category, e -> pos()));
}

/**
	Gere le debut des drag'n drop
	@param supportedActions Les actions supportees
*/
void ElementsPanel::startDrag(Qt::DropActions supportedActions) {
	Q_UNUSED(supportedActions);
	// recupere l'emplacement selectionne
	ElementsLocation element_location = selectedElementLocation();
	if (!element_location.isNull()) {
		startElementDrag(element_location);
		return;
	}
	
	TitleBlockTemplateLocation tbt_location = selectedTemplateLocation();
	if (tbt_location.isValid()) {
		startTitleBlockTemplateDrag(tbt_location);
		return;
	}
}

/**
	Handle the dragging of an element.
	@param location Location of the dragged element
*/
void ElementsPanel::startElementDrag(const ElementsLocation &location) {
	// recupere la selection
	ElementsCollectionItem *selected_item = QETApp::collectionItem(location);
	if (!selected_item) return;
	
	// objet QDrag pour realiser le drag'n drop
	QDrag *drag = new QDrag(this);
	
	// donnees qui seront transmises par le drag'n drop
	QString location_string(location.toString());
	QMimeData *mimeData = new QMimeData();
	mimeData -> setText(location_string);
	
	if (selected_item -> isCategory() || selected_item -> isCollection()) {
		mimeData -> setData("application/x-qet-category-uri", location_string.toLatin1());
		drag -> setPixmap(QET::Icons::Folder.pixmap(22, 22));
	} else if (selected_item -> isElement()) {
		mimeData -> setData("application/x-qet-element-uri", location_string.toLatin1());
		
		// element temporaire pour fournir un apercu
		int elmt_creation_state;
		Element *temp_elmt = ElementFactory::Instance() -> createElement(location, 0, &elmt_creation_state);
		if (elmt_creation_state) {
			delete temp_elmt;
			return;
		}
		
		// accrochage d'une pixmap representant l'appareil au pointeur
		QPixmap elmt_pixmap(temp_elmt -> pixmap());
		QPoint elmt_hotspot(temp_elmt -> hotspot());
		
		// ajuste la pixmap si celle-ci est trop grande
		QPoint elmt_pixmap_size(elmt_pixmap.width(), elmt_pixmap.height());
		if (elmt_pixmap.width() > QET_MAX_DND_PIXMAP_WIDTH || elmt_pixmap.height() > QET_MAX_DND_PIXMAP_HEIGHT) {
			elmt_pixmap = elmt_pixmap.scaled(QET_MAX_DND_PIXMAP_WIDTH, QET_MAX_DND_PIXMAP_HEIGHT, Qt::KeepAspectRatio);
			elmt_hotspot = QPoint(
				elmt_hotspot.x() * elmt_pixmap.width() / elmt_pixmap_size.x(),
				elmt_hotspot.y() * elmt_pixmap.height() / elmt_pixmap_size.y()
			);
		}
		
		drag -> setPixmap(elmt_pixmap);
		drag -> setHotSpot(elmt_hotspot);
		
		// suppression de l'appareil temporaire
		delete temp_elmt;
	}
	
	// realisation du drag'n drop
	drag -> setMimeData(mimeData);
	drag -> start(Qt::MoveAction | Qt::CopyAction);
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
	Inform this panel the project \a project has integrated the element at \a location
*/
QList<ElementsLocation> ElementsPanel::elementIntegrated(QETProject *project, const ElementsLocation &location) {
	// the base implementation simply refreshes the adequate category and returns the list of added locations
	QList<ElementsLocation> added_locations = GenericPanel::elementIntegrated(project, location);
	if (!added_locations.count() || !mustHighlightIntegratedElements()) {
		return(added_locations);
	}
	
	// the additional job of this method consists in displaying the integrated elements...
	if (QTreeWidgetItem *integrated_element_qtwi = itemForElementsLocation(location)) {
		ensureHierarchyIsVisible(QList<QTreeWidgetItem *>() << integrated_element_qtwi);
		scrollToItem(integrated_element_qtwi, QAbstractItemView::PositionAtCenter);
	}
	
	// and make them "flash" (not too obviously though) so the user notices they have been integrated.
	QList<QTreeWidgetItem *> items;
	foreach (ElementsLocation loc, added_locations) {
		if (QTreeWidgetItem *added_item = itemForElementsLocation(loc)) {
			items << added_item;
		}
	}
	highlightItems(items, this, SLOT(scrollToSelectedItem()));
	
	return(added_locations);
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
	
	if (ElementsCollection *elmt_collection = project -> embeddedCollection()) {
		if (QTreeWidgetItem *elmt_collection_qtwi = itemForElementsCollection(elmt_collection)) {

			if (first_add) elmt_collection_qtwi -> setExpanded(true);
		}
	}
	
	qtwi_project -> setStatusTip(0, tr("Double-cliquez pour réduire ou développer ce projet", "Status tip"));
	
	return(qtwi_project);
}

/**
	Methode privee permettant d'ajouter une collection d'elements au panel d'elements
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere la collection d'elements
	@param collection Collection a inserer dans le panel d'elements - si
	collection vaut 0, cette methode retourne 0.
	@param coll_name Nom a utiliser pour la collection
	@param icon Icone a utiliser pour l'affichage de la collection
	@return Le QTreeWidgetItem insere le plus haut
*/
QTreeWidgetItem *ElementsPanel::addCollection(ElementsCollection *collection) {
	PanelOptions options = GenericPanel::AddAllChild;
	options |= GenericPanel::DisplayElementsPreview;
	return(addElementsCollection(collection, invisibleRootItem(), options));
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

QTreeWidgetItem *ElementsPanel::updateElementsCategoryItem(QTreeWidgetItem *category_qtwi, ElementsCategory *category, PanelOptions options, bool freshly_created) {
	QTreeWidgetItem *item = GenericPanel::updateElementsCategoryItem(category_qtwi, category, options, freshly_created);
	item -> setStatusTip(
		0,
		tr(
			"Double-cliquez pour réduire ou développer cette catégorie d'éléments",
			"Status tip displayed by elements category"
		)
	);
	emit(loadingProgressed(++ loading_progress_, -1));
	return(item);
}

QTreeWidgetItem *ElementsPanel::updateElementsCollectionItem(QTreeWidgetItem *collection_qtwi, ElementsCollection *collection, PanelOptions options, bool freshly_created) {
	QTreeWidgetItem *c_qtwi = GenericPanel::updateElementsCollectionItem(collection_qtwi, collection, options, freshly_created);
	if (collection && collection -> project()) {
		c_qtwi -> setText(0, tr("Collection embarquée"));
		c_qtwi -> setStatusTip(0, tr("Double-cliquez pour réduire ou développer cette collection d'éléments embarquée", "Status tip"));
	}
	return(c_qtwi);
}

QTreeWidgetItem *ElementsPanel::updateElementItem(QTreeWidgetItem *element_qtwi, ElementDefinition *element, PanelOptions options, bool freshly_created) {
	QTreeWidgetItem *item = GenericPanel::updateElementItem(element_qtwi, element, options, freshly_created);
	
	QString status_tip = tr(
        "Glissez-déposez cet élément « %1 » sur un folio pour l'y insérer, double-cliquez dessus pour l'éditer",
		"Status tip displayed in the status bar when selecting an element"
	);
	item -> setStatusTip(0, status_tip.arg(item -> text(0)));
	
	item -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
	
	emit(loadingProgressed(++ loading_progress_, -1));
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
	Reloads the following collections:
	  * common collection
	  * custom collection
	  * collection of every project displayed in this panel
*/
void ElementsPanel::reloadCollections() {
	QETApp::commonElementsCollection() -> reload();
	QETApp::customElementsCollection() -> reload();
	
	// reloads collection of every project displayed in this panel
	foreach(QETProject *project, projects_to_display_) {
		if (ElementsCollection *project_collection = project -> embeddedCollection()) {
			project_collection -> reload();
		}
	}
}

/**
	@return the count of categories and elements within the following collections:
	  * common collection
	  * custom collection
	  * collection of every project displayed in this panel
*/
int ElementsPanel::elementsCollectionItemsCount() {
	int items_count = 0;
	items_count += QETApp::commonElementsCollection() -> count();
	items_count += QETApp::customElementsCollection() -> count();
	foreach(QETProject *project, projects_to_display_.values()) {
		if (ElementsCollection *project_collection = project -> embeddedCollection()) {
			items_count += project_collection -> count();
		}
	}
	return(items_count);
}

/**
	@return true if freshly integrated elements should be highlighted, false otherwise.
*/
bool ElementsPanel::mustHighlightIntegratedElements() const
{
	QSettings settings;
	return(settings.value("diagrameditor/highlight-integrated-elements", true).toBool());
}

/**
 * @brief ElementsPanel::reload
 * Reload the elements tree
 * @param reload_collections true for read all collections since their sources (files, projects ...)
 */
void ElementsPanel::reload(bool reload_collections) {
	if (reload_collections) {
		emit(readingAboutToBegin());
		reloadCollections();
		emit(readingFinished());
	}
	
	QIcon system_icon(":/ico/16x16/qet.png");
	QIcon user_icon(":/ico/16x16/go-home.png");
	
	// estimates the number of categories and elements to load
	int items_count = elementsCollectionItemsCount();
	emit(loadingProgressed(loading_progress_ = 0, items_count));
	
	// load the common title block templates collection
	TitleBlockTemplatesCollection *common_tbt_collection = QETApp::commonTitleBlockTemplatesCollection();
	common_tbt_collection_item_ = addTemplatesCollection(common_tbt_collection, invisibleRootItem());
	common_tbt_collection_item_ -> setIcon(0, system_icon);
	common_tbt_collection_item_ -> setStatusTip(0, tr("Double-cliquez pour réduire ou développer la collection de cartouches QElectroTech", "Status tip"));
	common_tbt_collection_item_ -> setWhatsThis(0, tr("Ceci est la collection de cartouches fournie avec QElectroTech. Installée en tant que composant système, vous ne pouvez normalement pas la personnaliser.", "\"What's this\" tip"));
	if (first_reload_) common_tbt_collection_item_ -> setExpanded(true);
	
	// load the common elements collection
	if (QETApp::commonElementsCollection() -> rootCategory()) {
		// first check local
		QETApp::commonElementsCollection()->cache()->setLocale(QETApp::langFromSetting());
		common_collection_item_ = addCollection(QETApp::commonElementsCollection());
		common_collection_item_ -> setStatusTip(0, tr("Double-cliquez pour réduire ou développer la collection d'éléments QElectroTech", "Status tip"));
		common_collection_item_ -> setWhatsThis(0, tr("Ceci est la collection d'éléments fournie avec QElectroTech. Installée en tant que composant système, vous ne pouvez normalement pas la personnaliser.", "\"What's this\" tip"));
		if (first_reload_) common_collection_item_ -> setExpanded(true);
	}
	
	// load the custom title block templates collection
	TitleBlockTemplatesCollection *custom_tbt_collection = QETApp::customTitleBlockTemplatesCollection();
	custom_tbt_collection_item_ = addTemplatesCollection(custom_tbt_collection, invisibleRootItem());
	custom_tbt_collection_item_ -> setIcon(0, user_icon);
	custom_tbt_collection_item_ -> setStatusTip(0, tr("Double-cliquez pour réduire ou développer votre collection personnelle de cartouches", "Status tip"));
	custom_tbt_collection_item_ -> setWhatsThis(0, tr("Ceci est votre collection personnelle de cartouches -- utilisez-la pour créer, stocker et éditer vos propres cartouches.", "\"What's this\" tip"));
	if (first_reload_) custom_tbt_collection_item_ -> setExpanded(true);
	
	// load the custom elements collection
	if (QETApp::customElementsCollection() -> rootCategory()) {
		// first check local
		QETApp::customElementsCollection()->cache()->setLocale(QETApp::langFromSetting());
		custom_collection_item_ = addCollection(QETApp::customElementsCollection());
		custom_collection_item_ -> setStatusTip(0, tr("Double-cliquez pour réduire ou développer votre collection personnelle d'éléments", "Status tip"));
		custom_collection_item_ -> setWhatsThis(0, tr("Ceci est votre collection personnelle d'éléments -- utilisez-la pour créer, stocker et éditer vos propres éléments.", "\"What's this\" tip"));
		if (first_reload_) custom_collection_item_ -> setExpanded(true);
	}
	
	// add projects
	foreach(QETProject *project, projects_to_display_.values()) {
		addProject(project);
	}
	
	// the first time, expand the first level of collections
	if (first_reload_) first_reload_ = false;
	
	emit(loadingFinished());
}

/**
	Gere le double-clic sur un element.
	Si un double-clic sur un projet est effectue, le signal requestForProject
	est emis.
	Si un double-clic sur un schema est effectue, le signal requestForDiagram
	est emis.
	Si un double-clic sur une collection, une categorie ou un element est
	effectue, le signal requestForCollectionItem est emis.
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
	} else if (qtwi_type & QET::ElementsCollectionItem) {
		ElementsLocation element = valueForItem<ElementsLocation>(qtwi);
		emit(requestForCollectionItem(element));
	} else if (qtwi_type == QET::TitleBlockTemplate) {
		TitleBlockTemplateLocation tbt = valueForItem<TitleBlockTemplateLocation>(qtwi);
		emit(requestForTitleBlockTemplate(tbt));
	}
}

/**
	@param qtwi Un QTreeWidgetItem
	@return L'ElementsCollectionItem represente par qtwi, ou 0 si qtwi ne
	represente pas un ElementsCollectionItem
*/
ElementsCollectionItem *ElementsPanel::collectionItemForItem(QTreeWidgetItem *qtwi) const {
	if (qtwi && qtwi -> type() & QET::ElementsCollectionItem) {
		ElementsLocation item_location = elementLocationForItem(qtwi);
		return(QETApp::collectionItem(item_location));
	}
	return(0);
}

/**
	Cette methode permet d'acceder a la categorie correspondant a un item donne.
	Si cet item represente une collection, c'est sa categorie racine qui est renvoyee.
	Si cet item represente une categorie, c'est cette categorie qui est renvoyee.
	Si cet item represente un element, c'est sa categorie parente qui est renvoyee.
	@param qtwi un QTreeWidgetItem
	@return la categorie correspondant au QTreeWidgetItem qtwi, ou 0 s'il n'y a
	aucune categorie correspondante.
*/
ElementsCategory *ElementsPanel::categoryForItem(QTreeWidgetItem *qtwi) {
	if (!qtwi) return(0);
	
	// Recupere le CollectionItem associe a cet item
	ElementsCollectionItem *collection_item = collectionItemForItem(qtwi);
	if (!collection_item) return(0);
	
	// recupere la categorie cible pour le deplacement
	return(collection_item -> toCategory());
}

/**
	@param pos Position dans l'arborescence
	@return La categorie situee sous la position pos, ou 0 s'il n'y a aucune
	categorie correspondante.
	@see categoryForItem
*/
ElementsCategory *ElementsPanel::categoryForPos(const QPoint &pos) {
	// Accede a l'item sous la position
	QTreeWidgetItem *pos_qtwi = itemAt(pos);
	if (!pos_qtwi) {
		return(0);
	}
	
	return(categoryForItem(pos_qtwi));
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
	
	ElementsCollectionItem *collection_item = collectionItemForItem(item);
	if (collection_item) {
		if (collection_item -> hasFilePath()) {
			return(collection_item -> filePath());
		}
	}
	else {
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
	Affiche un element etant donne son emplacement
	@param location Emplacement de l'element a afficher
*/
bool ElementsPanel::scrollToElement(const ElementsLocation &location) {
	// recherche l'element dans le panel
	QTreeWidgetItem *item = itemForElementsLocation(location);
	if (!item) return(false);
	
	// s'assure que l'item ne soit pas filtre
	item -> setHidden(false);
	setCurrentItem(item);
	ensureHierarchyIsVisible(QList<QTreeWidgetItem *>() << item);
	scrollToItem(item);
	return(true);
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

/**
	Scroll to the currently selected item.
*/
void ElementsPanel::scrollToSelectedItem() {
	QList<QTreeWidgetItem *> selected_items = selectedItems();
	if (selected_items.count()) {
		scrollToItem(selected_items.first(), QAbstractItemView::PositionAtCenter);
	}
}

/**
	Scroll to and highlight \a items. Once the animation is finished, the slot
	\a method is called on the object \a receiver.
*/
void ElementsPanel::highlightItems(const QList<QTreeWidgetItem *> &items, const QObject *receiver, const char *method) {
	TreeColorAnimation *animation1 = new TreeColorAnimation(items);
	animation1 -> setStartValue(QColor(Qt::white));
	animation1 -> setEndValue(QColor(Qt::yellow));
	animation1 -> setDuration(400);
	animation1 -> setEasingCurve(QEasingCurve::InQuad);
	
	TreeColorAnimation *animation2 = new TreeColorAnimation(items);
	animation2 -> setStartValue(QColor(Qt::yellow));
	animation2 -> setEndValue(QColor(Qt::white));
	animation2 -> setDuration(500);
	animation2 -> setEasingCurve(QEasingCurve::OutInQuint);
	
	QSequentialAnimationGroup *animation = new QSequentialAnimationGroup(this);
	animation -> addAnimation(animation1);
	animation -> addAnimation(new QPauseAnimation(700));
	animation -> addAnimation(animation2);
	if (receiver) {
		connect(animation, SIGNAL(finished()), receiver, method);
	}
	animation -> start(QAbstractAnimation::DeleteWhenStopped);
}
