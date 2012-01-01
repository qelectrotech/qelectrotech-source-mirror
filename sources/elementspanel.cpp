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
#include "elementspanel.h"
#include "qetapp.h"
#include "qetproject.h"
#include "diagram.h"
#include "elementscategory.h"
#include "elementscollectioncache.h"
#include "customelement.h"
#include "fileelementscollection.h"
#include "fileelementdefinition.h"
#include "qeticons.h"

/**
	This class implements a thread reloading the following elements
	collections:
	  * the common collection
	  * the custom collection
	  * the embedded collection of each project listed in the projects_
	attribute.
*/
class ReloadCollectionThread : public QThread {
	public:
	void run();
	/// list of projects whose embedded collection should be reloaded.
	QList<QETProject *> projects_;
};

/**
	Reloads collections.
*/
void ReloadCollectionThread::run() {
	QETApp::commonElementsCollection() -> reload();
	QETApp::customElementsCollection() -> reload();
	
	// reloads collection of every project displayed in this panel
	foreach(QETProject *project, projects_) {
		if (ElementsCollection *project_collection = project -> embeddedCollection()) {
			project_collection -> reload();
		}
	}
	exit();
}

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
	QTreeWidget(parent),
	common_collection_item_(0),
	custom_collection_item_(0),
	first_activation_(true),
	first_reload_(true)
{
	// selection unique
	setSelectionMode(QAbstractItemView::SingleSelection);
	setColumnCount(1);
	setExpandsOnDoubleClick(true);
	header() -> hide();
	
	// drag'n drop autorise
	setDragEnabled(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setAutoExpandDelay(1000);
	
	// taille des elements
	setIconSize(QSize(50, 50));
	
	// force du noir sur une alternance de blanc (comme le schema) et de gris
	// clair, avec du blanc sur bleu pas trop fonce pour la selection
	QPalette qp = palette();
	qp.setColor(QPalette::Text,            Qt::black);
	qp.setColor(QPalette::Base,            Qt::white);
	qp.setColor(QPalette::AlternateBase,   QColor("#e8e8e8"));
	qp.setColor(QPalette::Highlight,       QColor("#678db2"));
	qp.setColor(QPalette::HighlightedText, Qt::white);
	setPalette(qp);
	
	// double-cliquer sur un element permet de l'editer
	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(slot_doubleClick(QTreeWidgetItem *, int)));
	
	// emet un signal au lieu de gerer son menu contextuel
	setContextMenuPolicy(Qt::CustomContextMenu);
	
	QString cache_path = QETApp::configDir() + "/elements_cache.sqlite";
	cache_ = new ElementsCollectionCache(cache_path, this);
	cache_ -> setLocale(QLocale::system().name().left(2)); // @todo we need a unique function to get the good language
}

/**
	Destructeur
*/
ElementsPanel::~ElementsPanel() {
}

/**
	@param qtwi Un QTreeWidgetItem
	@return true si qtwi represente une collection, false sinon
*/
bool ElementsPanel::itemIsACollection(QTreeWidgetItem *qtwi) const {
	if (ElementsCollectionItem *qtwi_item = collectionItemForItem(qtwi)) {
		return(qtwi_item -> isCollection());
	}
	return(false);
}

/**
	@param qtwi Un QTreeWidgetItem
	@return true si qtwi represente une categorie, false sinon
*/
bool ElementsPanel::itemIsACategory(QTreeWidgetItem *qtwi) const {
	if (ElementsCollectionItem *qtwi_item = collectionItemForItem(qtwi)) {
		return(qtwi_item -> isCategory());
	}
	return(false);
}

/**
	@param qtwi Un QTreeWidgetItem
	@return true si qtwi represente un element, false sinon
*/
bool ElementsPanel::itemIsAnElement(QTreeWidgetItem *qtwi) const {
	if (ElementsCollectionItem *qtwi_item = collectionItemForItem(qtwi)) {
		return(qtwi_item -> isElement());
	}
	return(false);
}

/**
	@param qtwi Un QTreeWidgetItem
	@return true si qtwi represente un projet, false sinon
*/
bool ElementsPanel::itemIsAProject(QTreeWidgetItem *qtwi) const {
	return(projects_.contains(qtwi));
}

/**
	@param qtwi Un QTreeWidgetItem
	@return true si ce que represente qtwi est accessible en ecriture
*/
bool ElementsPanel::itemIsADiagram(QTreeWidgetItem *qtwi) const {
	return(diagrams_.contains(qtwi));
}

/**
	@param qtwi Un QTreeWidgetItem
	@return true si le qtwi est associe a une ElementsLocation
*/
bool ElementsPanel::itemHasLocation(QTreeWidgetItem *qtwi) const {
	return(locations_.contains(qtwi));
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
	@param qtwi A QTreeWidgetItem 
	@return true if the given QTreeWidgetItem represents a block templates directory
*/
bool ElementsPanel::itemIsATitleBlockTemplatesDirectory(QTreeWidgetItem *qtwi) const {
	return(title_blocks_directories_.contains(qtwi));
}

/**
	@param qtwi A QTreeWidgetItem 
	@return true if the given QTreeWidgetItem represents a block template
*/
bool ElementsPanel::itemIsATitleBlockTemplate(QTreeWidgetItem *qtwi) const {
	// does this QTreeWidgetItem have a parent?
	if (!qtwi -> parent()) return(false);
	return(itemIsATitleBlockTemplatesDirectory(qtwi -> parent()));
}

/**
	@param qtwi Un QTreeWidgetItem
	@return L'ElementsCollectionItem represente par qtwi, ou 0 si qtwi ne
	represente pas un ElementsCollectionItem
*/
ElementsCollectionItem *ElementsPanel::collectionItemForItem(QTreeWidgetItem *qtwi) const {
	if (locations_.contains(qtwi)) {
		return(QETApp::collectionItem(locations_[qtwi]));
	}
	return(0);
}

/**
	@param qtwi Un QTreeWidgetItem
	@return Le projet represente par qtwi, ou 0 si qtwi ne represente pas un
	projet
*/
QETProject *ElementsPanel::projectForItem(QTreeWidgetItem *qtwi) const {
	if (projects_.contains(qtwi)) {
		return(projects_[qtwi]);
	}
	return(0);
}

/**
	@param qtwi Un QTreeWidgetItem
	@return Le schema represente par qtwi, ou 0 si qtwi ne represente pas un
	schema
*/
Diagram *ElementsPanel::diagramForItem(QTreeWidgetItem *qtwi) const {
	if (diagrams_.contains(qtwi)) {
		return(diagrams_[qtwi]);
	}
	return(0);
}

/**
	@param qtwi QTreeWidgetItem dont on veut connaitre l'emplacement
	@return L'emplacement associe a qtwi, ou un emplacement nul s'il n'y a pas
	d'emplacement associe a qtwi
*/
ElementsLocation ElementsPanel::locationForItem(QTreeWidgetItem *qtwi) const {
	if (locations_.contains(qtwi)) {
		return(locations_[qtwi]);
	}
	return(ElementsLocation());
}

/**
	@return true si une collection est selectionnee, false sinon
*/
bool ElementsPanel::selectedItemIsACollection() const {
	if (ElementsCollectionItem *selected_item = selectedItem()) {
		return(selected_item -> isCollection());
	}
	return(false);
}

/**
	@return true si une categorie est selectionnee, false sinon
*/
bool ElementsPanel::selectedItemIsACategory() const {
	if (ElementsCollectionItem *selected_item = selectedItem()) {
		return(selected_item -> isCategory());
	}
	return(false);
}

/**
	@return true si un element est selectionne, false sinon
*/
bool ElementsPanel::selectedItemIsAnElement() const {
	if (ElementsCollectionItem *selected_item = selectedItem()) {
		return(selected_item -> isElement());
	}
	return(false);
}

/**
	@return true si un projet est selectionne, false sinon
*/
bool ElementsPanel::selectedItemIsAProject() const {
	return(projects_.contains(currentItem()));
}

/**
	@return true si un schema est selectionne, false sinon
*/
bool ElementsPanel::selectedItemIsADiagram() const {
	QTreeWidgetItem *current = currentItem();
	return(diagrams_.contains(current));
}

/**
	@return true si l'element selectionne est associe a une ElementsLocation
*/
bool ElementsPanel::selectedItemHasLocation() const {
	return(locations_.contains(currentItem()));
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
	@return true if the currently selected item represents a title block
	templates directory
*/
bool ElementsPanel::selectedItemIsATitleBlockTemplatesDirectory() const {
	return(itemIsATitleBlockTemplatesDirectory(currentItem()));
}

/**
	@return true if the currently selected item represents a title block
	template
*/
bool ElementsPanel::selectedItemIsATitleBlockTemplate() const {
	if (!currentItem()) return(false);
	return(itemIsATitleBlockTemplate(currentItem()));
}

/**
	@return la collection, la categorie ou l'element selectionne(e)
*/
ElementsCollectionItem *ElementsPanel::selectedItem() const {
	ElementsLocation selected_location(selectedLocation());
	if (!selected_location.isNull()) {
		return(QETApp::collectionItem(selected_location));
	}
	return(0);
}

/**
	@return Le projet selectionne, ou 0 s'il n'y en a pas
*/
QETProject *ElementsPanel::selectedProject() const {
	return(projectForItem(currentItem()));
}

/**
	@return Le schema selectionne, ou 0 s'il n'y en a pas
*/
Diagram *ElementsPanel::selectedDiagram() const {
	return(diagramForItem(currentItem()));
}

/**
	@return L'emplacement selectionne, ou un emplacement nul s'il n'y en a pas
*/
ElementsLocation ElementsPanel::selectedLocation() const {
	return(locationForItem(currentItem()));
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
void ElementsPanel::dragMoveEvent(QDragMoveEvent *e) {
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
	ElementsLocation dropped_location = ElementsLocation::locationFromString(e -> mimeData() -> text());
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
void ElementsPanel::dropEvent(QDropEvent *e) {
	// recupere la categorie cible pour le deplacement / la copie
	ElementsCategory *target_category = categoryForPos(e -> pos());
	if (!target_category) {
		e -> ignore();
		return;
	}
	
	// recupere la source (categorie ou element) pour le deplacement / la copie
	ElementsLocation dropped_location = ElementsLocation::locationFromString(e -> mimeData() -> text());
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
	ElementsLocation location = selectedLocation();
	if (location.isNull()) return;
	
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
		mimeData -> setData("application/x-qet-category-uri", location_string.toAscii());
		drag -> setPixmap(QET::Icons::Folder.pixmap(22, 22));
	} else if (selected_item -> isElement()) {
		mimeData -> setData("application/x-qet-element-uri", location_string.toAscii());
		
		// element temporaire pour fournir un apercu
		int elmt_creation_state;
		Element *temp_elmt = new CustomElement(location, 0, 0, &elmt_creation_state);
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
	@param event Object describing the received event 
*/
bool ElementsPanel::event(QEvent *event) {
	if (first_activation_ && event -> type() == QEvent::WindowActivate) {
		QTimer::singleShot(250, this, SLOT(reload()));
		first_activation_ = false;
	}
	return(QTreeWidget::event(event));
}

/**
	Methode permettant d'ajouter un projet au panel d'elements.
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere le projet
	@param project Projet a inserer dans le panel d'elements
	@return Le QTreeWidgetItem insere le plus haut
*/
QTreeWidgetItem *ElementsPanel::addProject(QTreeWidgetItem *qtwi_parent, QETProject *project) {
	// le projet sera insere juste avant la collection commune
	QTreeWidgetItem *last_project = 0;
	if (int common_collection_item_idx = indexOfTopLevelItem(common_collection_item_)) {
		last_project = topLevelItem(common_collection_item_idx - 1);
	}
	
	// creation du QTreeWidgetItem representant le projet
	QTreeWidgetItem *qtwi_project = new QTreeWidgetItem(qtwi_parent, last_project);
	qtwi_project -> setExpanded(true);
	projects_.insert(qtwi_project, project);
	updateProjectItemInformations(project);
	connect(
		project, SIGNAL(projectInformationsChanged(QETProject *)),
		this,    SLOT  (projectInformationsChanged(QETProject *))
	);
	
	// ajoute les schemas du projet
	foreach (Diagram *diagram, project -> diagrams()) {
		addDiagram(qtwi_project, diagram);
	}
	
	// add the title blocks templates embedded within the project
	updateProjectTemplates(project);
	connect(
		project, SIGNAL(projectTemplatesChanged(QETProject *)),
		this,    SLOT  (projectTemplatesChanged(QETProject *))
	);
	
	// ajoute la collection du projet
	addCollection(qtwi_project, project -> embeddedCollection(), tr("Collection projet"));
	
	return(qtwi_project);
}

/**
	Methode permettant d'ajouter un schema au panel d'elements.
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere le schema
	@param diagram Schema a inserer dans le panel d'elements
	@return Le QTreeWidgetItem insere le plus haut
*/
QTreeWidgetItem *ElementsPanel::addDiagram(QTreeWidgetItem *qtwi_parent, Diagram *diagram) {
	// determine le nom du schema
	QString final_name = diagramTitleToDisplay(diagram);
	
	// repere le dernier element correspondant a un schema, s'il existe
	QTreeWidgetItem *previous_diagram_item = 0;
	if (QETProject *project = diagram -> project()) {
		int added_diagram_index = project -> diagrams().indexOf(diagram);
		if (added_diagram_index > 0) {
			Diagram *previous_diagram = project -> diagrams().at(added_diagram_index - 1);
			previous_diagram_item = diagrams_.key(previous_diagram, 0);
		}
	}
	
	// creation du QTreeWidgetItem representant le schema
	QTreeWidgetItem *qtwi_diagram;
	if (previous_diagram_item) {
		qtwi_diagram = new QTreeWidgetItem(qtwi_parent, previous_diagram_item);
	} else {
		qtwi_diagram = new QTreeWidgetItem();
		qtwi_parent -> insertChild(0, qtwi_diagram);
	}
	qtwi_diagram -> setText(0, final_name);
	qtwi_diagram -> setIcon(0, QET::Icons::Diagram);
	diagrams_.insert(qtwi_diagram, diagram);
	
	return(qtwi_diagram);
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
QTreeWidgetItem *ElementsPanel::addCollection(QTreeWidgetItem *qtwi_parent, ElementsCollection *collection, const QString &coll_name, const QIcon &icon) {
	if (!collection) return(0);
	
	cache_ -> beginCollection(collection);
	QTreeWidgetItem *qtwi_coll = addCategory(qtwi_parent, collection -> rootCategory(), coll_name, icon);
	cache_ -> endCollection(collection);
	return(qtwi_coll);
}

/**
	Methode privee permettant d'ajouter une categorie au panel d'elements
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere la categorie
	@param category Categorie d'elements a inserer - si category vaut 0, cette
	methode retourne 0.
	@param cat_name Parametre facultatif permettant de forcer le nom affiche
	S'il n'est pas precise, la methode utilise le nom declare par la categorie.
	@param icon Icone a utiliser pour l'affichage de la categorie
	Si elle n'est pas precisee, une icone par defaut est utilisee
	@return Le QTreeWidgetItem insere le plus haut
*/
QTreeWidgetItem *ElementsPanel::addCategory(QTreeWidgetItem *qtwi_parent, ElementsCategory *category, const QString &cat_name, const QIcon &icon) {
	if (!category) return(0);
	
	// recupere le nom de la categorie
	QString final_name(cat_name.isEmpty() ? category -> name() : cat_name);
	QIcon final_icon(icon.isNull() ? QET::Icons::Folder : icon);
	
	// creation du QTreeWidgetItem representant le dossier
	QTreeWidgetItem *qtwi_category = new QTreeWidgetItem(qtwi_parent, QStringList(final_name));
	qtwi_category -> setToolTip(0, category -> location().toString());
	qtwi_category -> setIcon(0, final_icon);
	QLinearGradient t(0, 0, 200, 0);
	t.setColorAt(0, QColor("#e8e8e8"));
	t.setColorAt(1, QColor("#ffffff"));
	qtwi_category -> setBackground(0, QBrush(t));
	locations_.insert(qtwi_category, category -> location());
	emit(loadingProgressed(++ loading_progress_, -1));
	
	// reduit le dossier si besoin
	qtwi_category -> setExpanded(expanded_directories.contains(category -> location().toString()));
	
	// ajout des sous-categories
	foreach(ElementsCategory *sub_cat, category -> categories()) addCategory(qtwi_category, sub_cat);
	
	// ajout des elements
	foreach(ElementDefinition *elmt, category -> elements()) {
		addElement(qtwi_category, elmt);
		emit(loadingProgressed(++ loading_progress_, -1));
	}
	
	return(qtwi_category);
}

/**
	Methode privee permettant d'ajouter un element au panel d'elements
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere l'element
	@param element Element a inserer
	@param elmt_name Parametre facultatif permettant de forcer le nom affiche
	S'il n'est pas precise, la methode utilise le nom declare par l'element.
	Une icone sera generee a partir de l'element.
	@return Le QTreeWidgetItem insere
*/
QTreeWidgetItem *ElementsPanel::addElement(QTreeWidgetItem *qtwi_parent, ElementDefinition *element, const QString &elmt_name) {
	if (!element) return(0);
	
	if (!cache_ -> fetchElement(element)) {
		return(0);
	}
	QString custom_element_name   = cache_ -> name();
	QPixmap custom_element_pixmap = cache_ -> pixmap();
	
	QString whats_this = tr("Ceci est un \351l\351ment que vous pouvez ins\351rer dans votre sch\351ma par cliquer-d\351placer");
	QString tool_tip = tr("Cliquer-d\351posez cet \351l\351ment sur le sch\351ma pour ins\351rer un \351l\351ment ");
	QString final_name(elmt_name.isEmpty() ? custom_element_name : elmt_name);
	QTreeWidgetItem *qtwi = new QTreeWidgetItem(qtwi_parent, QStringList(final_name));
	qtwi -> setStatusTip(0, tool_tip + "\253 " + custom_element_name + " \273");
	qtwi -> setToolTip(0, element -> location().toString());
	qtwi -> setWhatsThis(0, whats_this);
	qtwi -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
	qtwi -> setIcon(0, QIcon(custom_element_pixmap));
	
	// actions speciales pour les elements appartenant a un projet
	if (QETProject *element_project = element -> location().project()) {
		// affiche en rouge les elements inutilises dans un projet
		if (!element_project -> usesElement(element -> location())) {
			QLinearGradient t(0, 0, 200, 0);
			t.setColorAt(0, QColor("#ffc0c0"));
			t.setColorAt(1, QColor("#ffffff"));
			qtwi -> setBackground(0, QBrush(t));
			qtwi -> setToolTip(0, QString(tr("%1 [non utilis\351 dans le projet]")).arg(qtwi -> toolTip(0)));
		}
	}
	locations_.insert(qtwi, element -> location());
	
	return(qtwi);
}

/**
	Reloads the following collections:
	  * common collection
	  * custom collection
	  * collection of every project displayed in this panel
*/
void ElementsPanel::reloadCollections() {
	ReloadCollectionThread thread;
	thread.projects_ = projects_to_display_.values();
	thread.start();
	while(!thread.wait(100)) {
		QApplication::processEvents();
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
	Recharge l'arbre des elements
	@param reload_collections true pour relire les collections depuis leurs sources (fichiers, projets...)
*/
void ElementsPanel::reload(bool reload_collections) {
	// sauvegarde la liste des repertoires reduits
	saveExpandedCategories();
	
	if (reload_collections) {
		emit(readingAboutToBegin());
		reloadCollections();
		emit(readingFinished());
	}
	
	// vide l'arbre et le hash
	clear();
	locations_.clear();
	projects_.clear();
	diagrams_.clear();
	title_blocks_directories_.clear();
	common_collection_item_ = 0;
	custom_collection_item_ = 0;
	
	// estimates the number of categories and elements to load
	int items_count = elementsCollectionItemsCount();
	emit(loadingProgressed(loading_progress_ = 0, items_count));
	
	// chargement des elements de la collection QET
	if (QETApp::commonElementsCollection()->rootCategory()) {
		common_collection_item_ = addCollection(invisibleRootItem(), QETApp::commonElementsCollection(), tr("Collection QET"),         QIcon(":/ico/16x16/qet.png"));
		if (first_reload_) common_collection_item_ -> setExpanded(true);
	}
	
	// chargement des elements de la collection utilisateur
	if (QETApp::customElementsCollection()->rootCategory()) {
		custom_collection_item_ = addCollection(invisibleRootItem(), QETApp::customElementsCollection(), tr("Collection utilisateur"), QIcon(":/ico/16x16/go-home.png"));
		if (first_reload_) custom_collection_item_ -> setExpanded(true);
	}
	
	// the first time, expand the first level of collections
	if (first_reload_) first_reload_ = false;
	
	// chargement des projets
	foreach(QETProject *project, projects_to_display_.values()) {
		addProject(invisibleRootItem(), project);
	}
	
	// reselectionne le dernier element selectionne
	if (!last_selected_item.isNull()) {
		QTreeWidgetItem *qtwi = findLocation(last_selected_item);
		if (qtwi) setCurrentItem(qtwi);
	}
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
	if (QETProject *project = projectForItem(qtwi)) {
		emit(requestForProject(project));
	} else if (Diagram *diagram = diagramForItem(qtwi)) {
		emit(requestForDiagram(diagram));
	} else if (ElementsCollectionItem *item = collectionItemForItem(qtwi)) {
		emit(requestForCollectionItem(item));
	} else if (itemIsATitleBlockTemplate(qtwi)) {
		if (QETProject *project = projectForTitleBlockTemplate(qtwi)) {
			emit(requestForTitleBlockTemplate(project, nameOfTitleBlockTemplate(qtwi)));
		}
	}
}

/**
	Enregistre la liste des categories repliees ainsi que le dernier element
	selectionne
*/
void ElementsPanel::saveExpandedCategories() {
	expanded_directories.clear();
	QList<QTreeWidgetItem *> items = findItems("*", Qt::MatchRecursive|Qt::MatchWildcard);
	foreach(QTreeWidgetItem *item, items) {
		QString file = locations_[item].toString();
		if (!file.endsWith(".elmt") && item -> isExpanded()) {
			expanded_directories << file;
		}
	}
	
	// sauvegarde egalement le dernier element selectionne
	QTreeWidgetItem *current_item = currentItem();
	if (current_item) last_selected_item = locations_[current_item].toString();
}

/**
	@param location emplacement a retrouver dans l'arborescence
	@return le QTreeWidgetItem correspondant a l'emplacaement location ou 0 si celui-ci n'est pas trouve
*/
QTreeWidgetItem *ElementsPanel::findLocation(const ElementsLocation &location) const {
	if (location.isNull()) return(0);
	return(locations_.key(location, 0));
}

/**
	@param location emplacement a retrouver dans l'arborescence
	@return le QTreeWidgetItem correspondant a l'emplacaement location ou 0 si celui-ci n'est pas trouve
*/
QTreeWidgetItem *ElementsPanel::findLocation(const QString &location) const {
	return(findLocation(ElementsLocation(location)));
}

/**
	Enleve et supprime un item du panel en nettoyant les structures le referencant.
	Note : Ce nettoyage est recursif
	@param removed_item Item a enlever et supprimer
*/
void ElementsPanel::deleteItem(QTreeWidgetItem *removed_item) {
	if (!removed_item) return;
	
	if (locations_.contains(removed_item)) {
		locations_.remove(removed_item);
	} else if (diagrams_.contains(removed_item)) {
		diagrams_.remove(removed_item);
	} else if (projects_.contains(removed_item)) {
		projects_.remove(removed_item);
	} else if (title_blocks_directories_.contains(removed_item)) {
		title_blocks_directories_.remove(removed_item);
	}
	
	// supprime les eventuels enfants de l'item
	foreach(QTreeWidgetItem *child_item, removed_item -> takeChildren()) {
		deleteItem(child_item);
	}
	
	delete removed_item;
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
	@param qtwi A QTreeWidgetItem, supposed to represent a templates directory
	@return the project that embeds the given templates directory, if
	applicable, 0 otherwise
*/
QETProject *ElementsPanel::projectForTitleBlockTemplatesDirectory(QTreeWidgetItem *qtwi) {
	if (title_blocks_directories_.contains(qtwi)) {
		return(title_blocks_directories_[qtwi]);
	}
	return(0);
}

/**
	@param qtwi A QTreeWidgetItem, supposed to represent a title block template
	@return the project that embeds the given template, if applicable, 0
	otherwise
*/
QETProject *ElementsPanel::projectForTitleBlockTemplate(QTreeWidgetItem *qtwi) {
	if (qtwi->parent()) {
		return(projectForTitleBlockTemplatesDirectory(qtwi->parent()));
	}
	return(0);
}

/**
	@param qtwi A QTreeWidgetItem, supposed to represent a title block template
	@return the name of the given template, if applicable, 0 otherwise
*/
QString ElementsPanel::nameOfTitleBlockTemplate(QTreeWidgetItem *qtwi) {
	if (itemIsATitleBlockTemplate(qtwi)) {
		return(qtwi -> data(0, 42).toString());
	}
	return(QString());
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
	N'affiche que les elements contenant une chaine donnee
	@param m Chaine a filtrer
*/
void ElementsPanel::filter(const QString &m) {
	QList<QTreeWidgetItem *> items = findItems("*", Qt::MatchRecursive | Qt::MatchWildcard);
	if (m.isEmpty()) {
		// la chaine est vide : affiche tout
		foreach(QTreeWidgetItem *item, items) item -> setHidden(false);
	} else {
		// repere les items correspondant au filtre
		QList<QTreeWidgetItem *> matching_items;
		foreach(QTreeWidgetItem *item, items) {
			bool item_matches = item -> text(0).contains(m, Qt::CaseInsensitive);
			if (item_matches) matching_items << item;
			item -> setHidden(!item_matches);
		}
		
		ensureHierarchyIsVisible(matching_items);
	}
}

/**
	Rajoute un projet au panel d'elements
	@param project Projet ouvert a rajouter au panel
*/
void ElementsPanel::projectWasOpened(QETProject *project) {
	projects_to_display_ << project;
	addProject(invisibleRootItem(), project);
}

/**
	Enleve un projet du panel d'elements
	@param project Projet a enlever du panel
*/
void ElementsPanel::projectWasClosed(QETProject *project) {
	if (QTreeWidgetItem *item_to_remove = projects_.key(project, 0)) {
		deleteItem(item_to_remove);
		projects_to_display_.remove(project);
	}
}

/**
	Gere le fait que les proprietes d'un projet change (exemple : fichier,
	titre, ...).
	@param project Projet modifie
*/
void ElementsPanel::projectInformationsChanged(QETProject *project) {
	updateProjectItemInformations(project);
}

/**
	Handles the fact that the title block templates of a project changed.
	@param project the modified project
*/
void ElementsPanel::projectTemplatesChanged(QETProject *project) {
	updateProjectTemplates(project);
}

/**
	Gere l'ajout d'un schema dans un projet
	@param project Projet auquel a ete ajouter le schema
	@param diagram Schema ajoute
*/
void ElementsPanel::diagramWasAdded(QETProject *project, Diagram *diagram) {
	// repere le QTWI du projet
	if (QTreeWidgetItem *qtwi_project = projects_.key(project)) {
		addDiagram(qtwi_project, diagram);
	}
}

/**
	Gere la suppression d'un schema dans un projet
	@param project Projet duquel a ete supprime le schema
	@param diagram Schema supprime
*/
void ElementsPanel::diagramWasRemoved(QETProject *project, Diagram *diagram) {
	// on verifie que le projet apparait dans le panel
	if (projects_.key(project, 0)) {
		// on verifie que le schema apparait dans le panel
		if (QTreeWidgetItem *item_to_remove = diagrams_.key(diagram, 0)) {
			QTreeWidgetItem *parent_qtwi = item_to_remove -> parent();
			
			// we keep the index of the deleted item
			int deleted_index = -1;
			if (parent_qtwi) {
				deleted_index = parent_qtwi -> indexOfChild(item_to_remove);
			}
			
			// deletion itself
			deleteItem(item_to_remove);
			
			// we also need to update the labels of following diagrams
			// because they may display the folio index
			if (deleted_index != -1) {
				// -2 avoids the title blocks directory and the common collection
				for (int i = deleted_index ; i < parent_qtwi -> childCount() - 2; ++i) {
					updateDiagramLabel(parent_qtwi, i);
				}
			}
		}
	}
}

/**
	@param project Projet auquel appartient le schema concerne
	@param diagram schema dont le titre a change
*/
void ElementsPanel::diagramTitleChanged(QETProject *project, Diagram *diagram) {
	// on verifie que le projet apparait dans le panel
	if (projects_.key(project, 0)) {
		// on verifie que le schema apparait dans le panel
		if (QTreeWidgetItem *qtwi_diagram = diagrams_.key(diagram)) {
			qtwi_diagram -> setText(0, diagramTitleToDisplay(diagram));
		}
	}
}

/**
	@param project Projet auquel appartiennent les schemas concernes
	@param from Index de l'onglet avant le deplacement
	@param to   Index de l'onglet apres le deplacement
*/
void ElementsPanel::diagramOrderChanged(QETProject *project, int from, int to) {
	// repere le QTWI correspondant au projet
	QTreeWidgetItem *qtwi_project = projects_.key(project);
	if (!qtwi_project) return;
	
	// repere le QTWI representant le schema deplace
	QTreeWidgetItem *moved_qtwi_diagram = qtwi_project -> child(from);
	if (!moved_qtwi_diagram) return;
	
	// enleve le QTWI et le reinsere au bon endroit
	bool was_selected = moved_qtwi_diagram -> isSelected();
	qtwi_project -> removeChild(moved_qtwi_diagram);
	qtwi_project -> insertChild(to, moved_qtwi_diagram);
	
	// update the QTWI labels because they may display the folio index
	updateDiagramLabel(qtwi_project, from);
	updateDiagramLabel(qtwi_project, to);
	
	if (was_selected) {
		setCurrentItem(moved_qtwi_diagram);
	}
}

/**
	Affiche un element etant donne son emplacement
	@param location Emplacement de l'element a afficher
*/
bool ElementsPanel::scrollToElement(const ElementsLocation &location) {
	// recherche l'element dans le panel
	QTreeWidgetItem *item = findLocation(location);
	if (!item) return(false);
	
	// s'assure que l'item ne soit pas filtre
	item -> setHidden(false);
	setCurrentItem(item);
	ensureHierarchyIsVisible(QList<QTreeWidgetItem *>() << item);
	scrollToItem(item);
	return(true);
}

/**
	Met a jour le nom, l'info-bulle et l'icone de l'item representant un projet.
	@param project le projet dont il faut mettre a jour l'affichage
*/
void ElementsPanel::updateProjectItemInformations(QETProject *project) {
	// determine le QTWI correspondant au projet
	QTreeWidgetItem *qtwi_project = projects_.key(project);
	if (!qtwi_project) return;
	
	// determine le nom et l'icone du projet
	QString final_name(project -> pathNameTitle());
	QString final_tooltip = project -> filePath();
	if (final_tooltip.isEmpty()) {
		final_tooltip = tr(
			"Pas de fichier",
			"tooltip for a file-less project in the element panel"
		);
	}
	
	qtwi_project -> setText(0, final_name);
	qtwi_project -> setToolTip(0, final_tooltip);
	qtwi_project -> setIcon(0, QET::Icons::Project);
}

/**
	(Re)generates the templates list of a given project.
	@param project the project we want to update the templates
*/
void ElementsPanel::updateProjectTemplates(QETProject *project) {
	// determine the QTWI for the templates directory of the given project
	QTreeWidgetItem *qtwi_project = projects_.key(project);
	if (!qtwi_project) return;
	
	// determine the templates directory for the given project, if any
	QTreeWidgetItem *titleblock_templates_qtwi = title_blocks_directories_.key(project);
	if (!titleblock_templates_qtwi) {
		// the poor thing does not exist... let's create it.
		titleblock_templates_qtwi = new QTreeWidgetItem(qtwi_project, QStringList() << tr("Mod\350les de cartouche"));
		titleblock_templates_qtwi -> setIcon(0, QET::Icons::Folder);
		titleblock_templates_qtwi -> setExpanded(true);
		title_blocks_directories_.insert(titleblock_templates_qtwi, project);
	} else {
		// oh, what a shiny templates directory... let's clear it.
		foreach(QTreeWidgetItem *titleblock_template_qtwi, titleblock_templates_qtwi -> takeChildren()) {
			deleteItem(titleblock_template_qtwi);
		}
	}
	
	// we can now populate the templates directory
	foreach (QString titleblock_name, project -> embeddedTitleBlockTemplates()) {
		QString final_name = QString(tr("Mod\350le \"%1\"")).arg(titleblock_name);
		QTreeWidgetItem *titleblock_template_qtwi = new QTreeWidgetItem(titleblock_templates_qtwi, QStringList() << final_name);
		titleblock_template_qtwi -> setIcon(0, QET::Icons::TitleBlock);
		titleblock_template_qtwi -> setData(0, 42, titleblock_name); // we store the original title block template name here, since the displayed one could be modified
	}
}

/**
	Updates the label of a diagram displayed by the elements panel
	@param qtwi_project QTreeWidgetItem representing the diagram's parent project
	@param diagram_index Index of the diagram within the parent project
*/
void ElementsPanel::updateDiagramLabel(QTreeWidgetItem *qtwi_project, int diagram_index) {
	if (!qtwi_project) return;
	
	QTreeWidgetItem *qtwi_diagram = qtwi_project -> child(diagram_index);
	if (!qtwi_diagram) return;
	
	Diagram *diagram = diagrams_[qtwi_diagram];
	if (diagram) {
		qtwi_diagram -> setText(0, diagramTitleToDisplay(diagram));
	}
}

/**
	@param diagram Schema dont on souhaite affiche le titre
	@return Un titre affichable, tenant compte du fait que le titre du schema
	peut etre vide.
*/
QString ElementsPanel::diagramTitleToDisplay(Diagram *diagram) const {
	QString displayed_title = diagram -> title();
	if (displayed_title.isEmpty()) {
		displayed_title = tr("Sch\351ma sans titre", "Fallback label when a diagram has no title");
	}
	
	QString displayed_label;
	int diagram_folio_idx = diagram -> folioIndex();
	if (diagram_folio_idx != -1) {
		displayed_label = QString(
			tr(
				"%1 - %2",
				"label displayed for a diagram in the panel ; %1 is the folio index, %2 is the diagram title"
			)
		).arg(diagram_folio_idx + 1).arg(displayed_title);
	} else {
		displayed_label = displayed_title;
	}
	return(displayed_label);
}

/**
	@param items une liste de QTreeWidgetItem pour lesquels il faut s'assurer
	que eux et leurs parents sont visibles
*/
void ElementsPanel::ensureHierarchyIsVisible(QList<QTreeWidgetItem *> items) {
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

