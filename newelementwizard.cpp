#include "newelementwizard.h"
#include "elementscategorieswidget.h"
#include "elementscategorieslist.h"
#include "nameslistwidget.h"
#include "orientationsetwidget.h"
#include "hotspoteditor.h"
#include "element.h"
#include "customelementeditor.h"

/**
	Constructeur
	@param parent QWidget parent de ce dialogue
	@param f flags pour le dialogue
*/
NewElementWizard::NewElementWizard(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f) {
	setFixedSize(480, 280);
	QVBoxLayout *dialog_layout = new QVBoxLayout();
	setLayout(dialog_layout);
	
	// chaines de caracteres utilisees sur les boutons
	str_next     = QString(tr("Suivant"));
	str_previous = QString(tr("Pr\351c\351dent"));
	str_finish   = QString(tr("Valider"));
	
	buildStep1();
	buildStep2();
	buildStep3();
	buildStep4();
	buildStep5();
	
	// dialogue itself
	setWindowTitle(tr("Cr\351er un nouvel \351l\351ment : Assistant"));
	
	dialog_layout -> addWidget(step1);
	dialog_layout -> addWidget(step2);
	dialog_layout -> addWidget(step3);
	dialog_layout -> addWidget(step4);
	dialog_layout -> addWidget(step5);
	step2 -> hide();
	step3 -> hide();
	step4 -> hide();
	step5 -> hide();
	
	// boutons
	QDialogButtonBox *boutons = new QDialogButtonBox(QDialogButtonBox::Cancel);
	button_previous = new QPushButton(str_previous, this);
	button_previous -> setEnabled(false);
	button_next = new QPushButton(str_next, this);
	boutons -> addButton(button_previous, QDialogButtonBox::ActionRole);
	boutons -> addButton(button_next,     QDialogButtonBox::ActionRole);
	connect(button_previous, SIGNAL(released()), this, SLOT(previous()));
	connect(button_next,     SIGNAL(released()), this, SLOT(next()));
	connect(boutons,         SIGNAL(rejected()), this, SLOT(accept()));
	dialog_layout -> addWidget(boutons);
	current_state = Category;
	
}

/**
	Destructeur
*/
NewElementWizard::~NewElementWizard() {
}

/**
	Passe a l'etape precedente
*/
void NewElementWizard::previous() {
	switch(current_state) {
		case Category:
			return;
		case Filename:
			current_state = Category;
			step1 -> show();
			step2 -> hide();
			button_previous -> setEnabled(false);
			break;
		case Names:
			current_state = Filename;
			step2 -> show();
			qle_filename -> setFocus();
			step3 -> hide();
			break;
		case Dimensions:
			current_state = Names;
			step3 -> show();
			step4 -> hide();
			button_next -> setText(str_next);
			break;
		case Orientations:
			current_state = Dimensions;
			step4 -> show();
			step5 -> hide();
			button_next -> setText(str_next);
	}
}

/**
	Passe a l'etape suivante
*/
void NewElementWizard::next() {
	switch(current_state) {
		case Category:
			if (!validStep1()) return;
			current_state = Filename;
			step1 -> hide();
			step2 -> show();
			qle_filename -> setFocus();
			button_previous -> setEnabled(true);
			break;
		case Filename:
			if (!validStep2()) return;
			current_state = Names;
			step2 -> hide();
			step3 -> show();
			break;
		case Names:
			if (!validStep3()) return;
			current_state = Dimensions;
			step3 -> hide();
			step4 -> show();
			break;
		case Dimensions:
			if (!validStep4()) return;
			current_state = Orientations;
			step4 -> hide();
			step5 -> show();
			button_next -> setText(str_finish);
			button_previous -> setEnabled(true);
			break;
		case Orientations:
			if (!validStep5()) return;
			createNewElement();
	}
}

/**
	Met en place l'etape 1 : Categorie
*/
void NewElementWizard::buildStep1() {
	step1 = new QWidget(this);
	QVBoxLayout *step1_layout = new QVBoxLayout();
	QLabel *explication = new QLabel(tr("\311tape 1/5 : S\351lectionnez une cat\351gorie dans laquelle enregistrer le nouvel \351l\351ment."));
	explication -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication -> setWordWrap(true);
	step1_layout -> addWidget(explication);
	categories_list = new ElementsCategoriesWidget();
	step1_layout -> addWidget(categories_list);
	step1 -> setLayout(step1_layout);
}

/**
	Met en place l'etape 2 : Nom du fichier
*/
void NewElementWizard::buildStep2() {
	step2 = new QWidget(this);
	QVBoxLayout *step2_layout = new QVBoxLayout();
	QLabel *explication1 = new QLabel(tr("\311tape 2/5 : Indiquez le nom du fichier dans lequel enregistrer le nouvel \351l\351ment."));
	explication1 -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication1 -> setWordWrap(true);
	QLabel *explication2 = new QLabel(tr("Vous n'\352tes pas oblig\351 de pr\351ciser l'extension *.elmt. Elle sera ajout\351e automatiquement."));
	explication2 -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication2 -> setWordWrap(true);
	step2_layout -> addWidget(explication1);
	qle_filename = new QLineEdit(tr("nouvel_element"));
	qle_filename -> selectAll();
	step2_layout -> addWidget(qle_filename);
	step2_layout -> addWidget(explication2);
	step2_layout -> addSpacing(100);
	step2 -> setLayout(step2_layout);
}

/**
	Met en place l'etape 3 : Noms de l'element
*/
void NewElementWizard::buildStep3() {
	step3 = new QWidget(this);
	QVBoxLayout *step3_layout = new QVBoxLayout();
	QLabel *explication = new QLabel(tr("\311tape 3/5 : Indiquez le ou les noms de l'\351l\351ment."));
	explication -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication -> setWordWrap(true);
	step3_layout -> addWidget(explication);
	element_names = new NamesListWidget();
	NamesList hash_name;
	hash_name.addName(QLocale::system().name().left(2), tr("Nom du nouvel \351l\351ment"));
	element_names -> setNames(hash_name);
	step3_layout -> addWidget(element_names);
	step3 -> setLayout(step3_layout);
}

/**
	Met en place l'etape 4 : Dimensions et Hotspot
*/
void NewElementWizard::buildStep4() {
	step4 = new QWidget(this);
	QVBoxLayout *step4_layout = new QVBoxLayout(step4);
	QLabel *explication = new QLabel(tr("\311tape 4/5 : Saisissez les dimensions du nouvel \351l\351ment ainsi que la position du hotspot (point de saisie de l'\351l\351ment \340 la souris) en consid\351rant que l'\351l\351ment est dans son orientation par d\351faut."));
	explication -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication -> setWordWrap(true);
	step4_layout -> addWidget(explication);
	hotspot_editor = new HotspotEditor();
	step4_layout -> addWidget(hotspot_editor, 0);
	step4_layout -> setSpacing(0);
}

/**
	Met en place l'etape 5 : orientations possibles pour le nouvel element
*/
void NewElementWizard::buildStep5() {
	step5 = new QWidget(this);
	
	QLabel *explication = new QLabel(tr("\311tape 5/5 : Indiquez les orientations possibles pour le nouvel \351l\351ment."));
	explication -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication -> setWordWrap(true);
	
	orientation_set = new OrientationSetWidget();
	
	QVBoxLayout *step5_layout = new QVBoxLayout();
	step5_layout -> addWidget(explication);
	step5_layout -> addWidget(orientation_set);
	step5_layout -> addSpacing(25);
	step5 -> setLayout(step5_layout);
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
	if (file_name == QString()) {
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
	Valide l'etape 3
	@return true si l'etape est validee, false sinon
*/
bool NewElementWizard::validStep3() {
	// il doit y avoir au moins un nom
	return(element_names -> checkOneName());
}

/**
	Valide l'etape 4
	@return true si l'etape est validee, false sinon
*/
bool NewElementWizard::validStep4() {
	// l'editeur de hotspot se charge deja de valider tout ca
	return(true);
}

/**
	Valide l'etape 5
	Cette fonction s'assure qu'il y ait toujours exactement une orientation
	"par defaut"
	@return true si l'etape est validee, false sinon
*/
bool NewElementWizard::validStep5() {
	// l'editeur d'orientations se charge deja de valider tout ca
	return(true);
}

/**
	Cree le nouvel element
*/
void NewElementWizard::createNewElement() {
	CustomElementEditor *edit_new_element = new CustomElementEditor(parentWidget());
	edit_new_element -> setSize(hotspot_editor -> elementSize());
	edit_new_element -> setHotspot(hotspot_editor -> hotspot());
	edit_new_element -> setNames(element_names -> names());
	edit_new_element -> setOrientations(orientation_set -> orientationSet());
	edit_new_element -> setFileName(chosen_file);
	edit_new_element -> show();
	accept();
}
