#include "newelementwizard.h"
#include "elementscategorieswidget.h"
#include "elementscategorieslist.h"
#include "nameslist.h"
#include "diagram.h"

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
	element_names = new NamesList();
	QHash<QString, QString> hash_name;
	hash_name.insert(QLocale::system().name().left(2), tr("Nom du nouvel \351l\351ment"));
	element_names -> setNames(hash_name);
	step3_layout -> addWidget(element_names);
	step3 -> setLayout(step3_layout);
}

/**
	Met en place l'etape 4 : Dimensions et Hotspot
*/
void NewElementWizard::buildStep4() {
	step4 = new QWidget(this);
	sb_width = new QSpinBox();
	sb_width -> setMinimum(1);
	sb_width -> setValue(3);
	sb_width -> setSuffix(tr(" \32710 px"));
	sb_height = new QSpinBox();
	sb_height -> setMinimum(1);
	sb_height -> setValue(7);
	sb_height -> setSuffix(tr(" \32710 px"));
	
	sb_hotspot_x = new QSpinBox();
	sb_hotspot_x -> setValue(15);
	sb_hotspot_x -> setSuffix(tr(" px"));
	sb_hotspot_y = new QSpinBox();
	sb_hotspot_y -> setValue(35);
	sb_hotspot_y -> setSuffix(tr(" px"));
	
	diagram_scene = new Diagram();
	diagram_scene -> border_and_inset.setNbColumns(4);
	diagram_scene -> border_and_inset.setColumnsHeight(140);
	diagram_scene -> border_and_inset.displayInset(false);
	diagram_view = new QGraphicsView(diagram_scene);
	diagram_view -> setMaximumSize(
		static_cast<int>((5 * diagram_scene -> border_and_inset.columnsWidth()) + (3 * MARGIN)),
		300
	);
	diagram_view -> setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	diagram_view -> setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	diagram_view -> setAlignment(Qt::AlignLeft | Qt::AlignTop);
	
	connect(sb_width,     SIGNAL(valueChanged(int)), this, SLOT(updateHotspotLimits()));
	connect(sb_height,    SIGNAL(valueChanged(int)), this, SLOT(updateHotspotLimits()));
	connect(sb_width,     SIGNAL(valueChanged(int)), this, SLOT(updateScene()));
	connect(sb_height,    SIGNAL(valueChanged(int)), this, SLOT(updateScene()));
	connect(sb_hotspot_x, SIGNAL(valueChanged(int)), this, SLOT(updateScene()));
	connect(sb_hotspot_y, SIGNAL(valueChanged(int)), this, SLOT(updateScene()));
	
	QGridLayout *grid_layout = new QGridLayout();
	grid_layout -> addWidget(new QLabel(tr("<span style=\"text-decoration:underline;\">Dimensions</span>")),   0, 0);
	grid_layout -> addWidget(new QLabel(tr("Largeur :")),      1, 0);
	grid_layout -> addWidget(sb_width,                         1, 1);
	grid_layout -> addWidget(new QLabel(tr("Hauteur :")),      2, 0);
	grid_layout -> addWidget(sb_height,                        2, 1);
	grid_layout -> addWidget(new QLabel(tr("<span style=\"text-decoration:underline;\">Hotspot</span>")),      3, 0);
	grid_layout -> addWidget(new QLabel(tr("Abscisse :")),     4, 0);
	grid_layout -> addWidget(sb_hotspot_x,                     4, 1);
	grid_layout -> addWidget(new QLabel(tr("Ordonn\351e :")),  5, 0);
	grid_layout -> addWidget(sb_hotspot_y,                     5, 1);
	
	QHBoxLayout *step4_hlayout = new QHBoxLayout();
	
	step4_hlayout -> addLayout(grid_layout);
	step4_hlayout -> addWidget(diagram_view);
	
	
	QVBoxLayout *step4_layout = new QVBoxLayout();
	step4_layout -> addWidget(new QLabel(tr("\311tape 4/5 : Saisissez les dimensions du nouvel \351l\351ment ainsi\n que la position du hotspot (point de saisie de l'\351l\351ment \340 la souris)")));
	step4_layout -> addLayout(step4_hlayout);
	step4 -> setLayout(step4_layout);
	
	updateScene();
	updateHotspotLimits();
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
	
	return(true);
}

bool NewElementWizard::validStep3() {
	return(element_names -> checkOneName());
}

bool NewElementWizard::validStep4() {
	return(true);
}

bool NewElementWizard::validStep5() {
	return(true);
}

void NewElementWizard::updateScene() {
	foreach (QGraphicsItem *qgi, diagram_scene -> items()) {
		diagram_scene -> removeItem(qgi);
		delete qgi;
	}
	int elmt_width  = sb_width  -> value() * 10;
	int elmt_height = sb_height -> value() * 10;
	int hotspot_x   = sb_hotspot_x -> value();
	int hotspot_y   = sb_hotspot_y -> value();
	int margin_x = 10;
	int margin_y = 30;
	diagram_scene -> addRect(QRectF(margin_x, margin_y, sb_width -> value() * 10.0, sb_height -> value() * 10.0));
	QPen hotspot_pen(Qt::red);
	QGraphicsLineItem *line_hotspot_x = diagram_scene -> addLine(
		QLine(
			margin_x,
			margin_y + hotspot_y,
			margin_x + elmt_width,
			margin_y + hotspot_y
		),
		hotspot_pen
	);
	QGraphicsLineItem *line_hotspot_y = diagram_scene -> addLine(
		QLine(
			margin_x + hotspot_x,
			margin_y,
			margin_x + hotspot_x,
			margin_y + elmt_height
		),
		hotspot_pen
	);
	line_hotspot_x -> setZValue(10);
	line_hotspot_y -> setZValue(10);
	diagram_scene -> setSceneRect(QRect(0, 0, elmt_width + 10, elmt_height + 10));
}

void NewElementWizard::updateHotspotLimits() {
	sb_hotspot_x -> setMaximum(sb_width  -> value() * 10);
	sb_hotspot_y -> setMaximum(sb_height -> value() * 10);
}
