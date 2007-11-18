#include "configdialog.h"
#include "qetapp.h"

/**
	Constructeur
	@param parent QWidget parent
*/
ConfigDialog::ConfigDialog(QWidget *parent) : QDialog(parent) {
	
	setWindowTitle(tr("Configurer QElectroTech"));
	setMinimumSize(900, 400);
	
	// liste des pages
	pages_list = new QListWidget();
	pages_list -> setViewMode(QListView::IconMode);
	pages_list -> setIconSize(QSize(48, 48));
	pages_list -> setMovement(QListView::Static);
	pages_list -> setMinimumWidth(135);
	pages_list -> setMaximumWidth(135);
	pages_list -> setSpacing(4);
	
	// pages
	pages_widget = new QStackedWidget();
	addPage(new NewDiagramPage());
	buildPagesList();
	
	// boutons
	buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	
	// layouts
	QHBoxLayout *hlayout1 = new QHBoxLayout();
	hlayout1 -> addWidget(pages_list);
	hlayout1 -> addWidget(pages_widget);
	
	QVBoxLayout *vlayout1 = new QVBoxLayout();
	vlayout1 -> addLayout(hlayout1);
	vlayout1 -> addWidget(buttons);
	setLayout(vlayout1);
	
	// connexion signaux / slots
	connect(buttons, SIGNAL(accepted()), this, SLOT(applyConf()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
	connect(pages_list, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
}

/// Destructeur
ConfigDialog::~ConfigDialog() {
}

/**
	Gere les changements de page dans le dialogue de configuration
*/
void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous) {
	if (!current) current = previous;
	pages_widget -> setCurrentIndex(pages_list -> row(current));
}

/**
	Construit la liste des pages sur la gauche
*/
void ConfigDialog::buildPagesList() {
	pages_list -> clear();
	foreach(ConfigPage *page, pages) {
		QListWidgetItem *new_button = new QListWidgetItem(pages_list);
		new_button -> setIcon(page -> icon());
		new_button -> setText(page -> title());
		new_button -> setTextAlignment(Qt::AlignHCenter);
		new_button -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}
}

/**
	Applique la configuration de toutes les pages
*/
void ConfigDialog::applyConf() {
	foreach(ConfigPage *page, pages) {
		page -> applyConf();
	}
	accept();
}

/**
	Ajoute une page au dialogue de configuration
*/
void ConfigDialog::addPage(ConfigPage *page) {
	if (!page || pages.contains(page)) return;
	pages << page;
	pages_widget -> addWidget(page);
}
