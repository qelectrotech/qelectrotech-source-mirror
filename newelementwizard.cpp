#include "newelementwizard.h"
#include "elementscategorieswidget.h"

NewElementWizard::NewElementWizard(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f) {
	QVBoxLayout *dialog_layout = new QVBoxLayout();
	setLayout(dialog_layout);
	
	// 1ere etape : dimensions
	etape1 = new QWidget(this);
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
	
	QVBoxLayout *etape1_layout = new QVBoxLayout();
	etape1_layout -> addWidget(new QLabel(tr("Saisissez la largeur et la hauteur du nouvel \351l\351ment")));
	etape1_layout -> addLayout(grid_layout);
	etape1 -> setLayout(etape1_layout);
	
	// 2eme etape : Categorie
	etape2 = new QWidget(this);
	QVBoxLayout *etape2_layout = new QVBoxLayout();
	etape2_layout -> addWidget(new QLabel(tr("S\351lectionnez une cat\351gorie dans laquelle enregistrer le nouvel \351l\351ment.")));
	categories_list = new ElementsCategoriesWidget();
	etape2_layout -> addWidget(categories_list);
	etape2 -> setLayout(etape2_layout);
	
	
	// dialogue itself
	setWindowTitle(tr("Cr\351er un nouvel \351l\351ment : Assistant"));
	
	dialog_layout -> addWidget(etape1);
	dialog_layout -> addWidget(etape2);
	etape2 -> hide();
	
	// boutons
	QDialogButtonBox *boutons = new QDialogButtonBox(QDialogButtonBox::Cancel);
	button_previous = new QPushButton(tr("Pr\351c\351dent"), this);
	button_previous -> setEnabled(false);
	button_next = new QPushButton(tr("Suivant"), this);
	boutons -> addButton(button_previous, QDialogButtonBox::ActionRole);
	boutons -> addButton(button_next,     QDialogButtonBox::ActionRole);
	connect(button_previous, SIGNAL(released()), this, SLOT(previous()));
	connect(button_next,     SIGNAL(released()), this, SLOT(next()));
	connect(boutons,         SIGNAL(rejected()), this, SLOT(accept()));
	dialog_layout -> addWidget(boutons);
	current_state = Dimensions;
	
}

NewElementWizard::~NewElementWizard() {
}

void NewElementWizard::previous() {
	switch(current_state) {
		case Dimensions:
			return;
		case Category:
			current_state = Dimensions;
			etape1 -> show();
			etape2 -> hide();
			button_next -> setText(tr("Suivant"));
			button_previous -> setEnabled(false);
	}
}

void NewElementWizard::next() {
	switch(current_state) {
		case Dimensions:
			current_state = Category;
			etape1 -> hide();
			etape2 -> show();
			button_next -> setText(tr("Valider"));
			button_previous -> setEnabled(true);
			break;
		case Category:
			valid();
	}
}

void NewElementWizard::valid() {
	return;
	switch(current_state) {
		case Dimensions:
			
		case Category:
			current_state = Dimensions;
			button_next -> setText(tr("Suivant"));
	}
}
