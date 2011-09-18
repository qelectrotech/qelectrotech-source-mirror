/*
	Copyright 2006-2011 Xavier Guerrin
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
#include "elementspanelwidget.h"
#include "newelementwizard.h"
#include "elementscategorieswidget.h"
#include "elementscollectionitem.h"
#include "qetelementeditor.h"
#include "elementdeleter.h"
#include "elementscategoryeditor.h"
#include "elementscategorydeleter.h"
#include "qetapp.h"
#include "interactivemoveelementshandler.h"
#include "qetproject.h"
#include "diagram.h"
#include "qeticons.h"

/*
	Lorsque le flag ENABLE_PANEL_WIDGET_DND_CHECKS est defini, le panel
	effectue des verifications lors des drag'n drop d'elements et categories.
	Par exemple, il verifie qu'une categorie cible est accessible en ecriture
	avant d'y autoriser le drop d'un element.
	Supprimer ce flag permet de tester le comportement des fonctions de gestion
	des items (copy, move, etc.).
*/
#define ENABLE_PANEL_WIDGET_DND_CHECKS

/**
	Constructeur
	@param parent Le QWidget parent de ce widget
*/
ElementsPanelWidget::ElementsPanelWidget(QWidget *parent) : QWidget(parent) {
	// initialize the progress bar (hidden by default)
	progress_bar_ = new QProgressBar(this);
	progress_bar_ -> setVisible(false);
	progress_bar_ -> setTextVisible(true);
	// initalise le panel d'elements
	elements_panel = new ElementsPanel(this);
	
	// initialise les actions
	reload                = new QAction(QET::Icons::ViewRefresh,               tr("Recharger les collections"),           this);
	new_category          = new QAction(QET::Icons::FolderNew,                 tr("Nouvelle cat\351gorie"),               this);
	edit_category         = new QAction(QET::Icons::FolderEdit,                tr("\311diter la cat\351gorie"),           this);
	delete_category       = new QAction(QET::Icons::FolderDelete,              tr("Supprimer la cat\351gorie"),           this);
	delete_collection     = new QAction(QET::Icons::FolderDelete,              tr("Vider la collection"),                 this);
	new_element           = new QAction(QET::Icons::ElementNew,                tr("Nouvel \351l\351ment"),                this);
	import_element        = new QAction(QET::Icons::DocumentImport,            tr("Importer un \351l\351ment"),           this);
	edit_element          = new QAction(QET::Icons::ElementEdit,               tr("\311diter l'\351l\351ment"),           this);
	delete_element        = new QAction(QET::Icons::ElementDelete,             tr("Supprimer l'\351l\351ment"),           this);
	prj_close             = new QAction(QET::Icons::DocumentClose,             tr("Fermer ce projet"),                    this);
	prj_edit_prop         = new QAction(QET::Icons::DialogInformation,         tr("Propri\351t\351s du projet"),          this);
	prj_prop_diagram      = new QAction(QET::Icons::DialogInformation,         tr("Propri\351t\351s du sch\351ma"),       this);
	prj_add_diagram       = new QAction(QET::Icons::DiagramAdd,                tr("Ajouter un sch\351ma"),                this);
	prj_del_diagram       = new QAction(QET::Icons::DiagramDelete,             tr("Supprimer ce sch\351ma"),              this);
	prj_move_diagram_up   = new QAction(QET::Icons::GoUp,                      tr("Remonter ce sch\351ma"),               this);
	prj_move_diagram_down = new QAction(QET::Icons::GoDown,                    tr("Abaisser ce sch\351ma"),               this);
	tbt_add               = new QAction(QET::Icons::TitleBlock,                tr("Importer un nouveau mod\350le"),       this);
	tbt_edit              = new QAction(QET::Icons::TitleBlock,                tr("\311diter ce mod\350le"),              this);
	tbt_remove            = new QAction(QET::Icons::TitleBlock,                tr("Supprimer ce mod\350le"),              this);
	move_elements_        = new QAction(QET::Icons::MoveFile,                  tr("D\351placer dans cette cat\351gorie"), this);
	copy_elements_        = new QAction(QET::Icons::CopyFile,                  tr("Copier dans cette cat\351gorie"),      this);
	cancel_elements_      = new QAction(QET::Icons::Cancel,                    tr("Annuler"),                             this);
	erase_textfield       = new QAction(QET::Icons::EditClearLocationBar,      tr("Effacer le filtre"),                   this);
	
	// initialise le champ de texte pour filtrer avec une disposition horizontale
	QLabel *filter_label = new QLabel(tr("Filtrer : "), this);
	filter_textfield = new QLineEdit(this);
	filter_toolbar = new QToolBar("filter");
	filter_toolbar -> addAction(erase_textfield);
	filter_toolbar -> addWidget(filter_label);
	filter_toolbar -> addWidget(filter_textfield);
	
	// ajoute une petite marge a la droite du champ pour filtrer lorsque le style CleanLooks est utilise
	if (qobject_cast<QCleanlooksStyle *>(QApplication::style())) {
		int l, t, r, b;
		filter_toolbar -> getContentsMargins(&l, &t, &r, &b);
		filter_toolbar -> setContentsMargins (l, t, r + 4, b);
	}
	
	context_menu = new QMenu(this);
	
	connect(reload,                SIGNAL(triggered()), this,           SLOT(reloadAndFilter()));
	connect(new_category,          SIGNAL(triggered()), this,           SLOT(newCategory()));
	connect(edit_category,         SIGNAL(triggered()), this,           SLOT(editCategory()));
	connect(delete_category,       SIGNAL(triggered()), this,           SLOT(deleteCategory()));
	connect(delete_collection,     SIGNAL(triggered()), this,           SLOT(deleteCategory()));
	connect(new_element,           SIGNAL(triggered()), this,           SLOT(newElement()));
	connect(import_element,        SIGNAL(triggered()), this,           SLOT(importElement()));
	connect(edit_element,          SIGNAL(triggered()), this,           SLOT(editElement()));
	connect(delete_element,        SIGNAL(triggered()), this,           SLOT(deleteElement()));
	connect(prj_close,             SIGNAL(triggered()), this,           SLOT(closeProject()));
	connect(prj_edit_prop,         SIGNAL(triggered()), this,           SLOT(editProjectProperties()));
	connect(prj_prop_diagram,      SIGNAL(triggered()), this,           SLOT(editDiagramProperties()));
	connect(prj_add_diagram,       SIGNAL(triggered()), this,           SLOT(newDiagram()));
	connect(prj_del_diagram,       SIGNAL(triggered()), this,           SLOT(deleteDiagram()));
	connect(prj_move_diagram_up,   SIGNAL(triggered()), this,           SLOT(moveDiagramUp()));
	connect(prj_move_diagram_down, SIGNAL(triggered()), this,           SLOT(moveDiagramDown()));
	connect(tbt_add,               SIGNAL(triggered()), this,           SLOT(addTitleBlockTemplate()));
	connect(tbt_edit,              SIGNAL(triggered()), this,           SLOT(editTitleBlockTemplate()));
	connect(tbt_remove,            SIGNAL(triggered()), this,           SLOT(removeTitleBlockTemplate()));
	connect(move_elements_,        SIGNAL(triggered()), this,           SLOT(moveElements()));
	connect(copy_elements_,        SIGNAL(triggered()), this,           SLOT(copyElements()));
	
	connect(erase_textfield,       SIGNAL(triggered()),                 this,             SLOT(clearFilterTextField()));
	connect(filter_textfield,      SIGNAL(textEdited(const QString &)), elements_panel,   SLOT(filter(const QString &)));
	
	connect(elements_panel,        SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(updateButtons()));
	connect(elements_panel,        SIGNAL(customContextMenuRequested(const QPoint &)),               this, SLOT(handleContextMenu(const QPoint &)));
	connect(elements_panel,        SIGNAL(requestForCollectionItem(ElementsCollectionItem *)),       this, SLOT(handleCollectionRequest(ElementsCollectionItem *)));
	connect(
		elements_panel,
		SIGNAL(requestForMoveElements(ElementsCollectionItem *, ElementsCollectionItem *, QPoint)),
		this,
		SLOT(handleMoveElementsRequest(ElementsCollectionItem *, ElementsCollectionItem *, const QPoint &)),
		Qt::QueuedConnection
	);
	connect(elements_panel, SIGNAL(loadingProgressed(int, int)),  this, SLOT(updateProgressBar(int, int)));
	connect(elements_panel, SIGNAL(readingAboutToBegin()),        this, SLOT(collectionsRead()));
	connect(elements_panel, SIGNAL(readingFinished()),            this, SLOT(collectionsReadFinished()));
	
	// initialise la barre d'outils
	toolbar = new QToolBar(this);
	toolbar -> setMovable(false);
	toolbar -> addAction(reload);
	toolbar -> addSeparator();
	toolbar -> addAction(new_category);
	toolbar -> addAction(edit_category);
	toolbar -> addAction(delete_category);
	toolbar -> addSeparator();
	toolbar -> addAction(new_element);
	toolbar -> addAction(import_element);
	toolbar -> addAction(edit_element);
	toolbar -> addAction(delete_element);
	
	// disposition verticale
	QVBoxLayout *vlayout = new QVBoxLayout(this);
	vlayout -> setMargin(0);
	vlayout -> setSpacing(0);
	vlayout -> addWidget(toolbar);
	vlayout -> addWidget(filter_toolbar);
	vlayout -> addWidget(elements_panel);
	vlayout -> addWidget(progress_bar_);
	vlayout -> setStretchFactor(elements_panel, 75000);
	setLayout(vlayout);
}

/**
	Destructeur
*/
ElementsPanelWidget::~ElementsPanelWidget() {
}

/**
	Vide le champ de texte permettant a l'utilisateur de filtrer, donne le
	focus a ce champ et annule le filtrage.
*/
void ElementsPanelWidget::clearFilterTextField() {
	filter_textfield -> clear();
	filter_textfield -> setFocus();
	elements_panel -> filter(QString());
}

/**
	Recharge le panel d'elements
*/
void ElementsPanelWidget::reloadAndFilter() {
	// recharge tous les elements
	elements_panel -> reload(true);
	
	// reapplique le filtre
	elements_panel -> filter(filter_textfield -> text());
}

/**
	Emet le signal requestForProjectClosing avec le projet selectionne
*/
void ElementsPanelWidget::closeProject() {
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForProjectClosing(selected_project));
	}
}

/**
	Emet le signal requestForProjectPropertiesEdition avec le projet selectionne
*/
void ElementsPanelWidget::editProjectProperties() {
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForProjectPropertiesEdition(selected_project));
	}
}

/**
	Emet le signal requestForDiagramPropertiesEdition avec le schema selectionne
*/
void ElementsPanelWidget::editDiagramProperties() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramPropertiesEdition(selected_diagram));
	}
}

/**
	Emet le signal requestForNewDiagram avec le projet selectionne
*/
void ElementsPanelWidget::newDiagram() {
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForNewDiagram(selected_project));
	}
}

/**
	Emet le signal requestForDiagramDeletion avec le schema selectionne
*/
void ElementsPanelWidget::deleteDiagram() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramDeletion(selected_diagram));
	}
}

/**
	Emet le signal requestForDiagramMoveUp avec le schema selectionne
*/
void ElementsPanelWidget::moveDiagramUp() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramMoveUp(selected_diagram));
	}
}

/**
	Emet le signal requestForDiagramMoveDown avec le schema selectionne
*/
void ElementsPanelWidget::moveDiagramDown() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramMoveDown(selected_diagram));
	}
}

/**
	Opens a template editor to create a new title block template.
*/
void ElementsPanelWidget::addTitleBlockTemplate() {
	QTreeWidgetItem *current_item = elements_panel -> currentItem();
	if (!current_item) return;
	
	QETProject *parent_project = 0;
	if (elements_panel -> itemIsATitleBlockTemplate(current_item)) {
		parent_project = elements_panel -> projectForTitleBlockTemplate(current_item);
	} else if (elements_panel -> itemIsATitleBlockTemplatesDirectory(current_item)) {
		parent_project = elements_panel -> projectForTitleBlockTemplatesDirectory(current_item);
	}
	
	if (parent_project) {
		QETApp::instance() -> openTitleBlockTemplate(parent_project);
	}
}

/**
	Opens an editor to edit the currently selected title block template, if any.
*/
void ElementsPanelWidget::editTitleBlockTemplate() {
	QTreeWidgetItem *current_item = elements_panel -> currentItem();
	if (current_item && elements_panel -> itemIsATitleBlockTemplate(current_item)) {
		QETProject *parent_project = elements_panel -> projectForTitleBlockTemplate(current_item);
		QString template_name = elements_panel -> nameOfTitleBlockTemplate(current_item);
		QETApp::instance() -> openTitleBlockTemplate(parent_project, template_name);
	}
}

/**
	Delete the currently selected title block template, if any.
*/
void ElementsPanelWidget::removeTitleBlockTemplate() {
	QTreeWidgetItem *current_item = elements_panel -> currentItem();
	if (current_item && elements_panel -> itemIsATitleBlockTemplate(current_item)) {
		QETProject *parent_project = elements_panel -> projectForTitleBlockTemplate(current_item);
		QString template_name = elements_panel -> nameOfTitleBlockTemplate(current_item);
		parent_project -> removeTemplateByName(template_name);
	}
}

/**
	Appelle l'assistant de creation de nouvel element
*/
void ElementsPanelWidget::newElement() {
	ElementsCategory *selected_category = writableSelectedCategory();
	
	NewElementWizard new_element_wizard(this);
	if (selected_category) {
		new_element_wizard.preselectCategory(selected_category);
	}
	new_element_wizard.exec();
}

/**
	Import d'element en passant par l'editeur
*/
void ElementsPanelWidget::importElement() {
	QString fileName = QETElementEditor::getOpenElementFileName(this);
	
	// Ouverture de l'element dans l'editeur pour pouvoir ensuite l'enregistrer dans la categorie voulue
	if (!fileName.isEmpty()) {
		QETApp::instance() -> openElementFiles(QStringList() << fileName);
	}
}

/**
	Si une categorie accessible en ecriture est selectionnee, cette methode
	affiche directement un formulaire de creation de categorie en utilisant la
	selection comme categorie parente.
	Sinon, elle affiche un gestionnaire de categories, permettant ainsi a
	l'utilisateur de choisir une categorie parente.
*/
void ElementsPanelWidget::newCategory() {
	ElementsCategory *selected_category = writableSelectedCategory();
	
	if (selected_category) {
		ElementsCategoryEditor new_category_dialog(selected_category -> location(), false, this);
		if (new_category_dialog.exec() == QDialog::Accepted) {
			elements_panel -> reload();
		}
	} else {
		launchCategoriesManager();
		elements_panel -> reload();
	}
}

/**
	Met a jour les boutons afin d'assurer la coherence de l'interface
*/
void ElementsPanelWidget::updateButtons() {
	bool collection_selected = elements_panel -> selectedItemIsACollection();
	bool category_selected = collection_selected || elements_panel -> selectedItemIsACategory();
	bool element_selected = elements_panel -> selectedItemIsAnElement();
	
	if (collection_selected || category_selected || element_selected) {
		bool element_writable = elements_panel -> selectedItemIsWritable();
		delete_collection -> setEnabled(collection_selected && element_writable);
		new_category      -> setEnabled(category_selected && element_writable);
		edit_category     -> setEnabled(category_selected && !collection_selected);
		delete_category   -> setEnabled(category_selected && element_writable);
		new_element       -> setEnabled(category_selected && element_writable);
		edit_element      -> setEnabled(element_selected);
		delete_element    -> setEnabled(element_selected && element_writable);
	} else if (elements_panel -> selectedItemIsAProject()) {
		bool is_writable = !(elements_panel -> selectedProject() -> isReadOnly());
		prj_add_diagram -> setEnabled(is_writable);
	} else if (elements_panel -> selectedItemIsADiagram()) {
		Diagram    *selected_diagram         = elements_panel -> selectedDiagram();
		QETProject *selected_diagram_project = selected_diagram -> project();
		
		bool is_writable           = !(selected_diagram_project -> isReadOnly());
		int project_diagrams_count = selected_diagram_project -> diagrams().count();
		int diagram_position       = selected_diagram_project -> diagrams().indexOf(selected_diagram);
		
		prj_del_diagram       -> setEnabled(is_writable);
		prj_move_diagram_up   -> setEnabled(is_writable && diagram_position > 0);
		prj_move_diagram_down -> setEnabled(is_writable && diagram_position < project_diagrams_count - 1);
	} else if (elements_panel -> selectedItemIsATitleBlockTemplatesDirectory()) {
		bool is_writable = !(elements_panel -> projectForTitleBlockTemplatesDirectory(elements_panel -> currentItem()) -> isReadOnly());
		tbt_add -> setEnabled(is_writable);
	} else if (elements_panel -> selectedItemIsATitleBlockTemplate()) {
		bool is_writable = !(elements_panel -> projectForTitleBlockTemplate(elements_panel -> currentItem()) -> isReadOnly());
		tbt_add -> setEnabled(is_writable);
		tbt_edit -> setEnabled(is_writable);
		tbt_remove -> setEnabled(is_writable);
	}
}

/**
	Lance le gestionnaire de categories. Il s'agit d'un petit dialogue listant
	les categories accessibles en ecriture et permettant de les editer, de les
	supprimer et d'en creer de nouvelles.
*/
int ElementsPanelWidget::launchCategoriesManager() {
	QDialog new_category_dialog(this);
	new_category_dialog.setMinimumSize(480, 280);
	new_category_dialog.setWindowTitle(tr("Gestionnaire de cat\351gories", "window title"));
	
	QVBoxLayout *layout = new QVBoxLayout(&new_category_dialog);
	QLabel *explication = new QLabel(tr("Vous pouvez utiliser ce gestionnaire pour ajouter, supprimer ou modifier les cat\351gories."));
	explication -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication -> setWordWrap(true);
	layout -> addWidget(explication);
	
	layout -> addWidget(new ElementsCategoriesWidget());
	
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
	connect(buttons, SIGNAL(rejected()), &new_category_dialog, SLOT(accept()));
	layout -> addWidget(buttons);
	
	return(new_category_dialog.exec());
}

/**
	Gere le menu contextuel du panel d'elements
	@param pos Position ou le menu contextuel a ete demande
*/
void ElementsPanelWidget::handleContextMenu(const QPoint &pos) {
	// recupere l'item concerne par l'evenement ainsi que son chemin
	QTreeWidgetItem *item = elements_panel -> itemAt(pos);
	if (!item) return;
	
	updateButtons();
	context_menu -> clear();
	
	if (elements_panel -> itemHasLocation(item)) {
		// recupere l'emplacement associe a l'item
		ElementsCollectionItem *selected_item = elements_panel -> collectionItemForItem(item);
		
		if (selected_item) {
			if (selected_item -> isCategory()) {
				context_menu -> addAction(new_category);
				context_menu -> addAction(edit_category);
				context_menu -> addAction(delete_category);
				context_menu -> addAction(new_element);
			} else if (selected_item -> isElement()) {
				context_menu -> addAction(edit_element);
				context_menu -> addAction(delete_element);
			} else if (selected_item -> isCollection()) {
				// categorie racine / collection
				context_menu -> addAction(new_category);
				context_menu -> addAction(delete_collection);
				context_menu -> addAction(new_element);
			}
		}
	} else {
		if (elements_panel -> itemIsAProject(item)) {
			context_menu -> addAction(prj_edit_prop);
			context_menu -> addAction(prj_add_diagram);
			context_menu -> addAction(prj_close);
		} else if (elements_panel -> itemIsADiagram(item)) {
			context_menu -> addAction(prj_prop_diagram);
			context_menu -> addAction(prj_del_diagram);
			context_menu -> addAction(prj_move_diagram_up);
			context_menu -> addAction(prj_move_diagram_down);
		} else if (elements_panel -> itemIsATitleBlockTemplatesDirectory(item)) {
			context_menu -> addAction(tbt_add);
		} else if (elements_panel -> itemIsATitleBlockTemplate(item)) {
			context_menu -> addAction(tbt_add);
			context_menu -> addAction(tbt_edit);
			context_menu -> addAction(tbt_remove);
		}
	}
	
	// affiche le menu
	if (!context_menu -> isEmpty()) {
		context_menu -> popup(mapToGlobal(elements_panel -> mapTo(this, pos + QPoint(2, 2))));
	}
}

/**
	Gere les demandes d'edition de categories ou d'elements
	@param item Item de la collection a editer
*/
void ElementsPanelWidget::handleCollectionRequest(ElementsCollectionItem *item) {
	if (!item) return;
	if (item -> isElement()) {
		// il s'agit d'un element
		launchElementEditor(item -> location());
	}
	// we could edit it categories, but instead people prefer the double-clic to
	// expand/collapse them
}

/**
	Gere le drop d'un collectionItem sur un autre.
	Elle memorise dans les attributs de cette classe l'item source et l'item
	destination du drag'n drop.
	Un menu est ensuite affiche pour demander a l'utilisateur ce qu'il
	souhaite faire (deplacer, copier ou annuler).
	@param src Item source
	@param dst Item cible
	@param pos Position ou le menu contextuel a ete demande
*/
void ElementsPanelWidget::handleMoveElementsRequest(ElementsCollectionItem *src, ElementsCollectionItem *dst, const QPoint &pos) {
	if (!src || !dst || !dst -> isCategory()) return;
	
	// memorise les items source et cible du drag'n drop
	dnd_item_src_ = src;
	dnd_item_dst_ = dst;
	
#ifdef ENABLE_PANEL_WIDGET_DND_CHECKS
	// active ou desactive les actions selon la source et la cible
	copy_elements_ -> setEnabled(src -> isReadable() && dst -> isWritable());
	move_elements_ -> setEnabled(!src -> isRootCategory() && src -> isWritable() && dst -> isWritable());
#endif
	
	// affiche un menu contextuel pour que l'utilisateur indique s'il souhaite
	// effectuer un deplacement ou une copie
	context_menu -> clear();
	context_menu -> addAction(copy_elements_);
	context_menu -> addAction(move_elements_);
	context_menu -> addSeparator();
	context_menu -> addAction(cancel_elements_);
	
	context_menu -> popup(mapToGlobal(elements_panel -> mapTo(this, pos + QPoint(2, 2))));
}

/**
	Cette classe memorise l'item source et l'item destination du dernier drag'n
	drop. Cette methode effectue le deplacement de l'item source memorise dans
	l'item destination memorise.
	@see handleMoveElementsRequest
*/
void ElementsPanelWidget::moveElements() {
	moveElements(dnd_item_src_, dnd_item_dst_);
}

/**
	Deplace l'item src dans l'item dst
*/
void ElementsPanelWidget::moveElements(ElementsCollectionItem *src, ElementsCollectionItem *dst) {
	InteractiveMoveElementsHandler *interactive_handler = new InteractiveMoveElementsHandler();
	src -> move(dst -> toCategory(), interactive_handler);
	delete interactive_handler;
}

/**
	Cette classe memorise l'item source et l'item destination du dernier drag'n
	drop. Cette methode effectue la copie de l'item source memorise dans l'item
	destination memorise.
	@see handleMoveElementsRequest
*/
void ElementsPanelWidget::copyElements() {
	copyElements(dnd_item_src_, dnd_item_dst_);
}

/**
	Reflects the fact that collections are being read (i.e from filesystem) in
	the progress bar.
*/
void ElementsPanelWidget::collectionsRead() {
	progress_bar_ -> setMinimum(0);
	progress_bar_ -> setMaximum(1);
	progress_bar_ -> setValue(0);
	progress_bar_ -> setFormat(tr("Lecture...", "Reading of elements/categories files"));
	progress_bar_ -> setVisible(true);
}

/**
	Reflects the fact that collections have been read (i.e from filesystem) in
	the progress bar.
*/
void ElementsPanelWidget::collectionsReadFinished() {
	// we do not hide the progress bar because it will be used by updateProgressBar
}

/**
	Updates the progress bar
	@param current value that should be displayed
	@param maximum maximum expected value; -1 means "use the previously known one"
*/
void ElementsPanelWidget::updateProgressBar(int current, int maximum) {
	int provided_maximum = maximum == -1 ? progress_bar_ -> maximum() : maximum;
	if (provided_maximum != progress_bar_ -> maximum()) {
		progress_bar_ -> setMaximum(maximum);
	}
	if (!current) {
		progress_bar_ -> setFormat(tr("Chargement : %p%", "Visual rendering of elements/categories files - %p is the progress percentage"));
		progress_bar_ -> setVisible(true);
	} else if (current == provided_maximum) {
		QTimer::singleShot(500, progress_bar_, SLOT(hide()));
	}
	progress_bar_ -> setValue(current);
}

/**
	Copie l'item src dans l'item dst
*/
void ElementsPanelWidget::copyElements(ElementsCollectionItem *src, ElementsCollectionItem *dst) {
	InteractiveMoveElementsHandler *interactive_handler = new InteractiveMoveElementsHandler();
	src -> copy(dst -> toCategory(), interactive_handler, true);
	delete interactive_handler;
}

/**
	Edite la categorie selectionnee
*/
void ElementsPanelWidget::editCategory() {
	if (ElementsCollectionItem *selected_item = elements_panel -> selectedItem()) {
		if (selected_item -> isCategory()) {
			launchCategoryEditor(selected_item -> location());
		}
	}
}

/**
	Edite l'element selectionne
*/
void ElementsPanelWidget::editElement() {
	if (ElementsCollectionItem *selected_item = elements_panel -> selectedItem()) {
		if (selected_item -> isElement()) {
			launchElementEditor(selected_item -> location());
		}
	}
}

/**
	Supprime la categorie selectionnee
*/
void ElementsPanelWidget::deleteCategory() {
	if (ElementsCollectionItem *selected_item = elements_panel -> selectedItem()) {
		if (selected_item -> isCategory() || selected_item -> isCollection()) {
			ElementsCategoryDeleter cat_deleter(selected_item -> location(), this);
			if (cat_deleter.exec()) elements_panel -> reload(true);
		}
	}
}

/**
	Supprime l'element selectionne
*/
void ElementsPanelWidget::deleteElement() {
	if (ElementsCollectionItem *selected_item = elements_panel -> selectedItem()) {
		if (selected_item -> isElement()) {
			ElementDeleter elmt_deleter(selected_item -> location(), this);
			if (elmt_deleter.exec()) elements_panel -> reload(true);
		}
	}
}

/**
	Lance l'editeur d'element pour l'element filename
	@param location Emplacement de l'element a editer
*/
void ElementsPanelWidget::launchElementEditor(const ElementsLocation &location) {
	QETApp::instance() -> openElementLocations(QList<ElementsLocation>() << location);
}

/**
	Lance l'editeur de categorie pour la categorie path
	@param location Emplacement de la categorie a editer
*/
void ElementsPanelWidget::launchCategoryEditor(const ElementsLocation &location) {
	ElementsCategoryEditor ece(location, true);
	if (ece.exec() == QDialog::Accepted) {
		elements_panel -> reload();
	}
}

/**
	@return la categorie selectionnee s'il y en a une et que celle-ci est
	accessible en ecriture ; sinon retourne 0
	@see ElementsPanel::categoryForItem(QTreeWidgetItem *)
*/
ElementsCategory *ElementsPanelWidget::writableSelectedCategory() {
	// recupere l'element selectionne
	QTreeWidgetItem *selected_qtwi = elements_panel -> currentItem();
	if (!selected_qtwi) return(0);
	
	// l'element selectionne doit pouvoir correspondre a une categorie
	ElementsCategory *selected_category = elements_panel -> categoryForItem(selected_qtwi);
	if (!selected_category) return(0);
	
	// la categorie doit etre accessible en ecriture
	if (!selected_category -> isWritable()) return(0);
	
	return(selected_category);
}
