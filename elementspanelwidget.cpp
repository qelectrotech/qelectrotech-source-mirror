#include "elementspanelwidget.h"
#include "newelementwizard.h"
#include "elementscategorieswidget.h"
/**
	Constructeur
	@param parent Le QWidget parent de ce widget
*/
ElementsPanelWidget::ElementsPanelWidget(QWidget *parent) : QWidget(parent) {
	// initalise le panel d'elements
	elements_panel = new ElementsPanel(this);
	
	// initialise les actions
	reload          = new QAction(QIcon(":/ico/reload.png"),          tr("Recharger les collections"), this);
	new_category    = new QAction(QIcon(":/ico/category_new.png"),    tr("Nouvelle cat\351gorie"),     this);
	edit_category   = new QAction(QIcon(":/ico/category_edit.png"),   tr("\311diter la cat\351gorie"), this);
	delete_category = new QAction(QIcon(":/ico/category_delete.png"), tr("Supprimer la cat\351gorie"), this);
	new_element     = new QAction(QIcon(":/ico/new.png"),             tr("Nouvel \351l\351ment"),      this);
	edit_element    = new QAction(QIcon(":/ico/edit.png"),            tr("\311diter l'\351l\351ment"), this);
	delete_element  = new QAction(QIcon(":/ico/delete.png"),          tr("Supprimer l'\351l\351ment"), this);
	
	connect(reload,          SIGNAL(triggered()), elements_panel, SLOT(reload()));
	connect(new_category,    SIGNAL(triggered()), this,           SLOT(newCategory()));
	connect(edit_category,   SIGNAL(triggered()), elements_panel, SLOT(editCategory()));
	connect(delete_category, SIGNAL(triggered()), elements_panel, SLOT(deleteCategory()));
	connect(new_element,     SIGNAL(triggered()), this,           SLOT(newElement()));
	connect(edit_element,    SIGNAL(triggered()), elements_panel, SLOT(editElement()));
	connect(delete_element,  SIGNAL(triggered()), elements_panel, SLOT(deleteElement()));
	
	connect(elements_panel, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(updateButtons()));
	
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
	toolbar -> addAction(edit_element);
	toolbar -> addAction(delete_element);
	
	// disposition verticale
	QVBoxLayout *vlayout = new QVBoxLayout(this);
	vlayout -> setMargin(0);
	vlayout -> setSpacing(0);
	vlayout -> addWidget(toolbar);
	vlayout -> addWidget(elements_panel);
	vlayout -> setStretchFactor(elements_panel, 75000);
	setLayout(vlayout);
}

/**
	Destructeur
*/
ElementsPanelWidget::~ElementsPanelWidget() {
}

/**
	Appelle l'assistant de creation de nouvel element
*/
void ElementsPanelWidget::newElement() {
	NewElementWizard new_element_wizard;
	new_element_wizard.exec();
}

/**
	Lance le gestionnaire de categories
*/
void ElementsPanelWidget::newCategory() {
	QDialog new_category_dialog;
	new_category_dialog.setFixedSize(480, 280);
	new_category_dialog.setWindowTitle(tr("Gestionnaire de cat\351gories"));
	
	QVBoxLayout *layout = new QVBoxLayout(&new_category_dialog);
	QLabel *explication = new QLabel(tr("Vous pouvez utiliser ce gestionnaire pour ajouter, supprimer ou modifier les cat\351gories."));
	explication -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication -> setWordWrap(true);
	layout -> addWidget(explication);
	
	layout -> addWidget(new ElementsCategoriesWidget());
	
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
	connect(buttons, SIGNAL(rejected()), &new_category_dialog, SLOT(accept()));
	layout -> addWidget(buttons);
	
	new_category_dialog.exec();
	elements_panel -> reload();
}

/**
	Met a jour les boutons afin d'assurer la coherence de l'interface
*/
void ElementsPanelWidget::updateButtons() {
	bool category_selected = elements_panel -> selectedItemIsACategory();
	bool element_selected = elements_panel -> selectedItemIsAnElement();
	edit_category   -> setEnabled(category_selected);
	delete_category -> setEnabled(category_selected);
	edit_element    -> setEnabled(element_selected);
	delete_element  -> setEnabled(element_selected);
}
