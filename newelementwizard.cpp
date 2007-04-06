#include "newelementwizard.h"
#include "elementscategorieswidget.h"
#include "elementscategorieslist.h"

NewElementWizard::NewElementWizard(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f) {
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

NewElementWizard::~NewElementWizard() {
}

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

void NewElementWizard::next() {
	switch(current_state) {
		case Category:
			if (!validStep1()) return;
			current_state = Filename;
			step1 -> hide();
			step2 -> show();
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
	}
}

/**
	Met en place l'etape 1 : Categorie
*/
void NewElementWizard::buildStep1() {
	step1 = new QWidget(this);
	QVBoxLayout *step1_layout = new QVBoxLayout();
	step1_layout -> addWidget(new QLabel(tr("\311tape 1/5 : S\351lectionnez une cat\351gorie dans laquelle enregistrer le nouvel \351l\351ment.")));
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
	step2_layout -> addWidget(new QLabel(tr("\311tape 2/5 : Indiquez le nom du fichier dans lequel enregistrer le nouvel \351l\351ment.")));
	step2_layout -> addWidget(qle_filename = new QLineEdit());
	step2 -> setLayout(step2_layout);
}

/**
	Met en place l'etape 3 : Noms de l'element
*/
void NewElementWizard::buildStep3() {
	step3 = new QWidget(this);
	QVBoxLayout *step3_layout = new QVBoxLayout();
	step3_layout -> addWidget(new QLabel(tr("\311tape 3/5 : Indiquez le ou les noms de l'\351l\351ment.")));
	/// @todo
	step3 -> setLayout(step3_layout);
}

/**
	Met en place l'etape 4 : Dimensions et Hotspot
*/
void NewElementWizard::buildStep4() {
	step4 = new QWidget(this);
	sb_largeur = new QSpinBox();
	sb_largeur -> setMinimum(1);
	sb_hauteur = new QSpinBox();
	sb_hauteur -> setMinimum(1);
	
	QGridLayout *grid_layout = new QGridLayout();
	grid_layout -> addWidget(new QLabel(tr("Largeur :")),  0, 0);
	grid_layout -> addWidget(sb_largeur,                   0, 1);
	grid_layout -> addWidget(new QLabel(tr("\327 10 px")), 0, 2);
	grid_layout -> addWidget(new QLabel(tr("Hauteur :")),  1, 0);
	grid_layout -> addWidget(sb_hauteur,                   1, 1);
	grid_layout -> addWidget(new QLabel(tr("\327 10 px")), 1, 2);
	
	QVBoxLayout *step4_layout = new QVBoxLayout();
	step4_layout -> addWidget(new QLabel(tr("\311tape 4/5 : Saisissez la largeur et la hauteur du nouvel \351l\351ment")));
	step4_layout -> addLayout(grid_layout);
	step4 -> setLayout(step4_layout);
}

/**
	Met en place l'etape 5 : orientations possibles pour le nouvel element
*/
void NewElementWizard::buildStep5() {
	step5 = new QWidget(this);
	QVBoxLayout *step5_layout = new QVBoxLayout();
	step5_layout -> addWidget(new QLabel(tr("\311tape 5/5 : Indiquez les orientations possibles pour le nouvel \351l\351ment.")));
	/// @todo
	step5 -> setLayout(step5_layout);
}

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
	
	return(true);
}

bool NewElementWizard::validStep3() {
	return(true);
}

bool NewElementWizard::validStep4() {
	return(true);
}

bool NewElementWizard::validStep5() {
	return(true);
}

