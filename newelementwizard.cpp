#include "newelementwizard.h"
#include "elementscategorieswidget.h"
#include "elementscategorieslist.h"
#include "nameslistwidget.h"
#include "orientationsetwidget.h"
#include "hotspoteditor.h"
#include "element.h"
#include "qetelementeditor.h"

/**
	Constructeur
	@param parent QWidget parent de ce dialogue
	@param f flags pour le dialogue
*/
NewElementWizard::NewElementWizard(QWidget *parent, Qt::WindowFlags f) : QWizard(parent, f) {
	setPixmap(LogoPixmap, QPixmap(":/ico/qelectrotech.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	setWindowTitle(tr("Cr\351er un nouvel \351l\351ment : Assistant"));
	setButtonText(QWizard::NextButton, tr("&Suivant >"));
	addPage(buildStep1());
	addPage(buildStep2());
	addPage(buildStep3());
	addPage(buildStep4());
	addPage(buildStep5());
	setFixedSize(705, 325);
}

/**
	Destructeur
*/
NewElementWizard::~NewElementWizard() {
}

/**
	Met en place l'etape 1 : Categorie
*/
QWizardPage *NewElementWizard::buildStep1() {
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Category);
	page -> setTitle(tr("\311tape 1/5 : Cat\351gorie parente"));
	page -> setSubTitle(tr("S\351lectionnez une cat\351gorie dans laquelle enregistrer le nouvel \351l\351ment."));
	QVBoxLayout *layout = new QVBoxLayout();
	
	categories_list = new ElementsCategoriesWidget();
	layout -> addWidget(categories_list);
	
	page -> setLayout(layout);
	return(page);
}

/**
	Met en place l'etape 2 : Nom du fichier
*/
QWizardPage *NewElementWizard::buildStep2() {
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Filename);
	page -> setTitle(tr("\311tape 2/5 : Nom du fichier"));
	page -> setSubTitle(tr("Indiquez le nom du fichier dans lequel enregistrer le nouvel \351l\351ment."));
	QVBoxLayout *layout = new QVBoxLayout();
	
	qle_filename = new QLineEdit(tr("nouvel_element"));
	qle_filename -> selectAll();
	QLabel *explication2 = new QLabel(tr("Vous n'\352tes pas oblig\351 de pr\351ciser l'extension *.elmt. Elle sera ajout\351e automatiquement."));
	explication2 -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication2 -> setWordWrap(true);
	layout -> addWidget(qle_filename);
	layout -> addWidget(explication2);
	layout -> addSpacing(100);
	
	page -> setLayout(layout);
	return(page);
}

/**
	Met en place l'etape 3 : Noms de l'element
*/
QWizardPage *NewElementWizard::buildStep3() {
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Names);
	page -> setTitle(tr("\311tape 3/5 : Noms de l'\351l\351ment"));
	page -> setSubTitle(tr("Indiquez le ou les noms de l'\351l\351ment."));
	QVBoxLayout *layout = new QVBoxLayout();
	
	element_names = new NamesListWidget();
	NamesList hash_name;
	hash_name.addName(QLocale::system().name().left(2), tr("Nom du nouvel \351l\351ment"));
	element_names -> setNames(hash_name);
	layout -> addWidget(element_names);
	
	page -> setLayout(layout);
	return(page);
}

/**
	Met en place l'etape 4 : Dimensions et Hotspot
*/
QWizardPage *NewElementWizard::buildStep4() {
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Dimensions);
	page -> setTitle(tr("\311tape 4/5 : Dimensions et point de saisie"));
	page -> setSubTitle(tr("Saisissez les dimensions du nouvel \351l\351ment ainsi que la position du hotspot (point de saisie de l'\351l\351ment \340 la souris) en consid\351rant que l'\351l\351ment est dans son orientation par d\351faut."));
	QVBoxLayout *layout = new QVBoxLayout();
	
	hotspot_editor = new HotspotEditor();
	layout -> addWidget(hotspot_editor, 0);
	layout -> setSpacing(0);
	
	page -> setLayout(layout);
	return(page);
}

/**
	Met en place l'etape 5 : orientations possibles pour le nouvel element
*/
QWizardPage *NewElementWizard::buildStep5() {
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Orientations);
	page -> setTitle(tr("\311tape 5/5 : Orientations"));
	page -> setSubTitle(tr("Indiquez les orientations possibles pour le nouvel \351l\351ment."));
	QVBoxLayout *layout = new QVBoxLayout();
	
	orientation_set = new OrientationSetWidget();
	layout -> addWidget(orientation_set);
	layout -> addSpacing(25);
	
	page -> setLayout(layout);
	return(page);
}

/// @return true si l'etape en cours est validee, false sinon
bool NewElementWizard::validateCurrentPage() {
	WizardState wizard_state = static_cast<WizardState>(currentPage() -> property("WizardState").toInt());
	if      (wizard_state == Category) return(validStep1());
	else if (wizard_state == Filename) return(validStep2());
	else if (wizard_state == Names)    return(element_names -> checkOneName()); // il doit y avoir au moins un nom
	else if (wizard_state == Orientations) {
		createNewElement();
		return(true);
	}
	else return(true);
}

/**
	Valide l'etape 1
	@return true si l'etape est validee, false sinon
*/
bool NewElementWizard::validStep1() {
	// il doit y avoir une categorie selectionnee
	bool step1_ok = categories_list -> elementsCategoriesList().selectedCategoryPath() != QString();
	if (!step1_ok) {
		QMessageBox::critical(
			this,
			tr("Erreur"),
			tr("Vous devez s\351lectionner une cat\351gorie.")
		);
	}
	return(step1_ok);
}

/**
	Valide l'etape 2
	@return true si l'etape est validee, false sinon
*/
bool NewElementWizard::validStep2() {
	QString dir_path = categories_list -> elementsCategoriesList().selectedCategoryPath();
	QString file_name = qle_filename -> text();
	
	// un nom doit avoir ete entre
	if (file_name.isEmpty()) {
		QMessageBox::critical(
			this,
			tr("Erreur"),
			tr("Vous devez entrer un nom de fichier")
		);
		return(false);
	}
	
	if (!file_name.endsWith(".elmt")) file_name += ".elmt";
	
	// le fichier existe peut etre deja
	if (QFileInfo(dir_path + "/" + file_name).exists()) {
		QMessageBox::StandardButton answer = QMessageBox::question(
			this,
			"\311craser le fichier ?",
			"Le fichier existe d\351j\340. Souhaitez-vous l'\351craser ?",
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
			QMessageBox::No
		);
		return(answer == QMessageBox::Yes);
	}
	
	chosen_file = dir_path + "/" + file_name;
	return(true);
}

/**
	Cree le nouvel element
*/
void NewElementWizard::createNewElement() {
	QETElementEditor *edit_new_element = new QETElementEditor(parentWidget());
	edit_new_element -> setSize(hotspot_editor -> elementSize());
	edit_new_element -> setHotspot(hotspot_editor -> hotspot());
	edit_new_element -> setNames(element_names -> names());
	edit_new_element -> setOrientations(orientation_set -> orientationSet());
	edit_new_element -> setFileName(chosen_file);
	edit_new_element -> show();
}
