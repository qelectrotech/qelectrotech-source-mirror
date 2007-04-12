#include "exportdialog.h"

/**
	Constructeur
	@param dia Le schema a exporter
	@param parent Le Widget parent de ce dialogue
*/
ExportDialog::ExportDialog(Diagram *dia, QWidget *parent) : QDialog(parent) {
	if (!dia) return;
	
	// recupere le schema a exporter, sa taille et ses proportions
	diagram = dia;
	diagram_size = diagram -> imageSize();
	diagram_ratio = (qreal)diagram_size.width() / (qreal)diagram_size.height();
	dontchangewidth = dontchangeheight = false;
	
	// la taille du dialogue est fixee
	setFixedSize(800, 360);
	setWindowTitle(tr("Exporter"));
	
	// le dialogue comporte deux boutons
	buttons = new QDialogButtonBox(this);
	buttons -> setOrientation(Qt::Horizontal);
	buttons -> setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Save);
	
	// disposition des elements
	QGridLayout *layout = new QGridLayout(this);
	setContentsMargins(0, 0, 5, 5);
	layout -> setMargin(0);
	layout -> setSpacing(0);
	layout -> setColumnMinimumWidth(0, 390);
	layout -> setColumnMinimumWidth(1, 400);
	layout -> addWidget(leftPart(),  0, 0);
	layout -> addWidget(rightPart(), 0, 1);
	layout -> addWidget(buttons,     1, 1);
	
	setTabOrder(keep_aspect_ratio, buttons);
	
	// connexions signaux/slots
	connect(button_browse,     SIGNAL(released()),         this, SLOT(slot_chooseAFile()));
	connect(width,             SIGNAL(valueChanged(int)),  this, SLOT(slot_correctHeight()));
	connect(keep_aspect_ratio, SIGNAL(stateChanged(int)),  this, SLOT(slot_correctHeight()));
	connect(height,            SIGNAL(valueChanged(int)),  this, SLOT(slot_correctWidth()));
	connect(buttons,           SIGNAL(accepted()),         this, SLOT(slot_check()));
	connect(buttons,           SIGNAL(rejected()),         this, SLOT(reject()));
	
	connect(draw_grid,         SIGNAL(stateChanged(int) ), this, SLOT(slot_refreshPreview()));
	connect(draw_border,       SIGNAL(stateChanged(int) ), this, SLOT(slot_refreshPreview()));
	connect(draw_inset,        SIGNAL(stateChanged(int) ), this, SLOT(slot_refreshPreview()));
	connect(draw_columns,      SIGNAL(stateChanged(int) ), this, SLOT(slot_refreshPreview()));
	connect(keep_colors,       SIGNAL(stateChanged(int) ), this, SLOT(slot_refreshPreview()));
	connect(width,             SIGNAL(valueChanged(int) ), this, SLOT(slot_refreshPreview()));
	connect(height,            SIGNAL(valueChanged(int) ), this, SLOT(slot_refreshPreview()));
	connect(export_border,     SIGNAL(toggled     (bool)), this, SLOT(slot_changeUseBorder()));
}

/**
	Destructeur - ne fait rien
*/
ExportDialog::~ExportDialog() {
}

/**
	Met en place la partie du dialogue dans lequel l'utilisateur entre les
	dimensions souhaitees de l'image.
	@return La QGroupBox permettant de regler les dimensions de l'image
*/
QGroupBox *ExportDialog::setupDimensionsGroupBox() {
	QGroupBox *groupbox_dimensions = new QGroupBox(tr("Dimensions"), this);
	QGridLayout *gridLayout = new QGridLayout(groupbox_dimensions);
	
	// hauteur
	gridLayout -> addWidget(new QLabel(tr("Hauteur :"), groupbox_dimensions), 2, 0, 1, 1);
	
	width = new QSpinBox(groupbox_dimensions);
	width -> setRange(1, 10000);
	width -> setValue(diagram_size.width());
	gridLayout -> addWidget(width, 0, 1, 1, 1);
	
	gridLayout -> addWidget(new QLabel(tr("px"), groupbox_dimensions), 0, 2, 1, 1);
	
	// largeur
	gridLayout -> addWidget(new QLabel(tr("Largeur :"), groupbox_dimensions), 0, 0, 1, 1);
	
	height = new QSpinBox(groupbox_dimensions);
	height -> setRange(1, 10000);
	height -> setValue(diagram_size.height());
	gridLayout -> addWidget(height, 2, 1, 1, 1);
	
	gridLayout -> addWidget(new QLabel(tr("px"), groupbox_dimensions), 2, 2, 1, 1);
	
	// conserver les proportions
	keep_aspect_ratio = new QCheckBox(tr("Conserver les proportions"), groupbox_dimensions);
	keep_aspect_ratio -> setChecked(true);
	
	gridLayout -> addWidget(keep_aspect_ratio, 1, 3, 1, 1);
	
	return(groupbox_dimensions);
}

/**
	Met en place la partie du dialogue dans lequel l'utilisateur entre les
	options souhaitees de l'image.
	@return La QGroupBox permettant de regler les options de l'image
*/
QGroupBox *ExportDialog::setupOptionsGroupBox() {
	QGroupBox *groupbox_options = new QGroupBox(tr("Options"), this);
	QGridLayout *optionshlayout = new QGridLayout(groupbox_options);
	
	// Choix de la zone du schema a exporter
	QButtonGroup *exported_content_choices = new QButtonGroup(groupbox_options);
	export_border = new QRadioButton(tr("Exporter le cadre"), groupbox_options);
	optionshlayout -> addWidget(export_border, 0, 0);
	exported_content_choices -> addButton(export_border);
	export_elements = new QRadioButton(tr("Exporter les \351l\351ments"), groupbox_options);
	optionshlayout -> addWidget(export_elements, 0, 1);
	exported_content_choices -> addButton(export_elements);
	(diagram -> useBorder() ? export_border : export_elements) -> setChecked(true);
	
	// dessiner la grille
	draw_grid = new QCheckBox(tr("Dessiner la grille"), groupbox_options);
	optionshlayout -> addWidget(draw_grid, 1, 0);
	draw_grid -> setChecked(diagram -> displayGrid());
	
	// dessiner le cadre
	draw_border = new QCheckBox(tr("Dessiner le cadre"), groupbox_options);
	optionshlayout -> addWidget(draw_border, 1, 1);
	draw_border -> setChecked(diagram -> border_and_inset.borderIsDisplayed());
	
	// dessiner le cartouche
	draw_inset = new QCheckBox(tr("Dessiner le cartouche"), groupbox_options);
	optionshlayout -> addWidget(draw_inset, 2, 0);
	draw_inset -> setChecked(diagram -> border_and_inset.insetIsDisplayed());
	
	// dessiner les colonnes
	draw_columns = new QCheckBox(tr("Dessiner les colonnes"), groupbox_options);
	optionshlayout -> addWidget(draw_columns, 2, 1);
	draw_columns -> setChecked(diagram -> border_and_inset.columnsAreDisplayed());
	
	// Conserver les couleurs
	keep_colors = new QCheckBox(tr("Conserver les couleurs"), groupbox_options);
	optionshlayout -> addWidget(keep_colors, 3, 0);
	
	return(groupbox_options);
}

/**
	Met en place la partie gauche du dialogue
	@return Le widget representant la moitie gauche du dialogue
*/
QWidget *ExportDialog::leftPart() {
	QWidget *retour = new QWidget();
	
	// la partie gauche du dialogue est un empilement vertical d'elements
	QVBoxLayout *vboxLayout = new QVBoxLayout(retour);
	
	/* le dialogue comprend une ligne permettant d'indiquer un chemin de fichier (hboxLayout) */
	QHBoxLayout *hboxLayout = new QHBoxLayout();
	hboxLayout -> addWidget(new QLabel(tr("Nom de fichier :"), this));
	hboxLayout -> addWidget(filename = new QLineEdit(this));
	hboxLayout -> addWidget(button_browse = new QPushButton(tr("Parcourir"), this));
	
	vboxLayout -> addLayout(hboxLayout);
	
	/* une ligne permettant de choisir le format (hboxLayout1) */
	QHBoxLayout *hboxLayout1 = new QHBoxLayout();
	hboxLayout1 -> addWidget(new QLabel(tr("Format :"), this));
	hboxLayout1 -> addWidget(format = new QComboBox(this));
	format -> addItem(tr("PNG (*.png)"),    "PNG");
	format -> addItem(tr("JPEG (*.jpg)"),   "JPG");
	format -> addItem(tr("Bitmap (*.bmp)"), "BMP");
	
	vboxLayout -> addLayout(hboxLayout1);
	
	/* un cadre permettant de specifier les dimensions de l'image finale */
	vboxLayout -> addWidget(setupDimensionsGroupBox());
	
	/* un cadre permettant de specifier les options de l'image finale */
	vboxLayout -> addWidget(setupOptionsGroupBox());
	
	// ordre des input selectionnes avec la touche tab
	setTabOrder(filename, button_browse);
	setTabOrder(button_browse, format);
	setTabOrder(format, width);
	setTabOrder(width, height);
	setTabOrder(height, keep_aspect_ratio);
	
	return(retour);
}

/**
	Met en place la partie droite du dialogue
	@return Le widget representant la moitie droite du dialogue
*/
QWidget *ExportDialog::rightPart() {
	QWidget *retour = new QWidget();
	QHBoxLayout *hboxlayout0 = new QHBoxLayout(retour);
	
	
	// la partie droite contient une GroupBox intitulee "Apercu"
	QGroupBox *groupbox_preview = new QGroupBox(tr("Aper\347u"), this);
	groupbox_preview -> setFixedWidth(390);
	QHBoxLayout *hboxlayout1 = new QHBoxLayout(groupbox_preview);
	hboxlayout1 -> setMargin(0);
	
	// cette GroupBox contient l'apercu
	preview_scene = new QGraphicsScene();
	preview_view = new QGraphicsView(preview_scene, groupbox_preview);
	hboxlayout1 -> addWidget(preview_view);
	
	// genere le premier apercu
	slot_refreshPreview();
	
	hboxlayout0 -> addWidget(groupbox_preview);
	return(retour);
}

/**
	Slot corrigeant la largeur (typiquement lors d'un changement de la hauteur)
*/
void ExportDialog::slot_correctWidth() {
	if (!keep_aspect_ratio -> isChecked() || dontchangewidth) return;
	dontchangeheight = true;
	width -> setValue(qRound(height -> value() * diagram_ratio));
	dontchangeheight = false;
}

/**
	Slot corrigeant la hauteur (typiquement lors d'un changement de la largeur)
*/
void ExportDialog::slot_correctHeight() {
	if (!keep_aspect_ratio -> isChecked() || dontchangeheight) return;
	dontchangewidth = true;
	height -> setValue(qRound(width -> value() / diagram_ratio));
	dontchangewidth = false;
}

/**
	Slot demandant a l'utilisateur de choisir un fichier
*/
void ExportDialog::slot_chooseAFile() {
	QString user_file = QFileDialog::getSaveFileName(
		this,
		tr("Exporter vers le fichier"),
		QDir::homePath(),
		tr("Images (*.png *.bmp *.jpg)")
	);
	if (user_file != "") {
		diagram_path = user_file;
		filename -> setText(diagram_path);
	}
}

/**
	Genere l'image a exporter
	@return l'image a exporter
*/
QImage ExportDialog::generateImage() {
	// memorise les parametres relatifs au schema
	bool state_drawBorder  = diagram -> border_and_inset.borderIsDisplayed();
	bool state_drawColumns = diagram -> border_and_inset.columnsAreDisplayed();
	bool state_drawInset   = diagram -> border_and_inset.insetIsDisplayed();
	bool state_drawGrid    = diagram -> displayGrid();
	bool state_useBorder   = diagram -> useBorder();
	
	// genere l'image
	diagram -> setUseBorder(export_border -> isChecked());
	diagram -> setDisplayGrid(draw_grid -> isChecked());
	diagram -> border_and_inset.displayBorder(draw_border -> isChecked());
	diagram -> border_and_inset.displayColumns(draw_columns -> isChecked());
	diagram -> border_and_inset.displayInset(draw_inset -> isChecked());
	QImage image = diagram -> toImage(
		width -> value(),
		height -> value(),
		keep_aspect_ratio -> isChecked() ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio
	);
	
	// convertit l'image en niveaux de gris si besoin
	if (!keep_colors -> isChecked()) {
		QVector<QRgb> ColorTab;
		for (int i = 0 ; i < 256 ; ++ i) ColorTab << qRgb(i, i, i);
		image = image.convertToFormat(QImage::Format_Indexed8, ColorTab, Qt::ThresholdDither);
	}
	
	// restaure les parametres relatifs au schema
	diagram -> border_and_inset.displayBorder(state_drawBorder);
	diagram -> border_and_inset.displayColumns(state_drawColumns);
	diagram -> border_and_inset.displayInset(state_drawInset);
	diagram -> setDisplayGrid(state_drawGrid);
	diagram -> setUseBorder(state_useBorder);
	
	return(image);
}

/**
	Slot effectuant les verifications necessaires apres la validation du
	dialogue.
*/
void ExportDialog::slot_check() {
	
	// verifie que le fichier a ete specifie
	if (diagram_path == "") {
		QMessageBox::information(
			this,
			tr("Fichier non sp\351cifi\351"),
			tr("Vous devez sp\351cifier le chemin du fichier dans lequel sera enregistr\351e l'image."),
			QMessageBox::Ok
		);
		return;
	}
	
	// recupere le format a utiliser (acronyme et extension)
	QString format_acronym = format -> itemData(format -> currentIndex()).toString();
	QString format_extension = "." + format_acronym.toLower();
	
	// corrige l'extension du fichier
	if (!diagram_path.endsWith(format_extension, Qt::CaseInsensitive)) diagram_path += format_extension;
	
	// recupere des informations sur le fichier specifie
	QFileInfo file_infos(diagram_path);
	
	// verifie qu'il est possible d'ecrire dans le fichier en question
	if (file_infos.exists() && !file_infos.isWritable()) {
		QMessageBox::critical(
			this,
			tr("Impossible d'\351crire dans ce fichier"),
			tr("Il semblerait que vous n'ayez pas les permissions n\351cessaires pour \351crire dans ce fichier.."),
			QMessageBox::Ok
		);
		return;
	}
	
	// ouvre le fichier
	QFile fichier(diagram_path);
	
	QImage image = generateImage();
	
	// enregistre l'image dans le fichier
	image.save(&fichier, format_acronym.toUtf8().data());
	fichier.close();
	
	// fermeture du dialogue
	accept();
}


/**
	Slot appele lorsque l'utilisateur change la zone du schema qui doit etre
	exportee. Il faut alors calculer le nouveau ratio et corriger les
	dimensions.
*/
void ExportDialog::slot_changeUseBorder() {
	// calcule le nouveau ratio
	bool state_useBorder   = diagram -> useBorder();
	diagram -> setUseBorder(export_border -> isChecked());
	diagram_size = diagram -> imageSize();
	diagram_ratio = (qreal)diagram_size.width() / (qreal)diagram_size.height();
	diagram -> setUseBorder(state_useBorder);
	
	// corrige les dimensions
	if (keep_aspect_ratio -> isChecked()) slot_correctHeight();
	
	// rafraichit l'apercu
	slot_refreshPreview();
}

void ExportDialog::slot_refreshPreview() {
	
	// genere le nouvel apercu
	QImage preview_image = generateImage();
	
	// nettoie l'apercu
	foreach (QGraphicsItem *qgi, preview_scene -> items()) {
		preview_scene -> removeItem(qgi);
		delete qgi;
	}
	
	// ajoute le nouvel apercu
	QGraphicsPixmapItem *qgpi = new QGraphicsPixmapItem(QPixmap::fromImage(preview_image));
	preview_scene -> addItem(qgpi);
	preview_scene -> setSceneRect(QRectF(0.0, 0.0, preview_image.width() + 15.0, preview_image.height() + 15.0));
}

