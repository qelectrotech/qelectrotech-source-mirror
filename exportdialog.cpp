#include "exportdialog.h"

/**
	Constructeur
	@param dia Le schema a exporter
	@param parent Le Widget parent de ce dialogue
*/
ExportDialog::ExportDialog(Schema &dia, QWidget *parent) : QDialog(parent) {
	// recupere le schema a exporter, sa taille et ses proportions
	diagram = &dia;
	diagram_size = diagram -> imageSize();
	diagram_ratio = (qreal)diagram_size.width() / (qreal)diagram_size.height();
	
	// la taille du dialogue est fixee
	setFixedSize(400, 310);
	setWindowTitle(tr("Exporter"));
	
	// le dialogue est un empilement vertical d'elements
	QVBoxLayout *vboxLayout = new QVBoxLayout(this);
	
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
	
	/* et deux boutons */
	buttons = new QDialogButtonBox(this);
	buttons -> setOrientation(Qt::Horizontal);
	buttons -> setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Save);
	
	vboxLayout -> addWidget(buttons);
	
	// ordre des input selectionnes avec la touche tab
	setTabOrder(filename, button_browse);
	setTabOrder(button_browse, format);
	setTabOrder(format, width);
	setTabOrder(width, height);
	setTabOrder(height, keep_aspect_ratio);
	setTabOrder(keep_aspect_ratio, buttons);
	
	// connexions signaux/slots
	connect(button_browse,     SIGNAL(released()),        this, SLOT(slot_chooseAFile()));
	connect(width,             SIGNAL(valueChanged(int)), this, SLOT(slot_correctHeight()));
	connect(keep_aspect_ratio, SIGNAL(stateChanged(int)), this, SLOT(slot_correctHeight()));
	connect(height,            SIGNAL(valueChanged(int)), this, SLOT(slot_correctWidth()));
	connect(buttons,           SIGNAL(accepted()),        this, SLOT(slot_check()));
	connect(buttons,           SIGNAL(rejected()),        this, SLOT(reject()));
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
	QHBoxLayout *optionshlayout = new QHBoxLayout(groupbox_options);
	
	// exporter la grille
	export_grid = new QCheckBox(tr("Exporter la grille"), groupbox_options);
	optionshlayout -> addWidget(export_grid);
	
	// Conserver les couleurs
	keep_colors = new QCheckBox(tr("Conserver les couleurs"), groupbox_options);
	optionshlayout -> addWidget(keep_colors);
	
	return(groupbox_options);
}

void ExportDialog::slot_correctWidth() {
	if (!keep_aspect_ratio -> isChecked() || dontchangewidth) return;
	dontchangeheight = true;
	width -> setValue(qRound(height -> value() * diagram_ratio));
	dontchangeheight = false;
}

void ExportDialog::slot_correctHeight() {
	if (!keep_aspect_ratio -> isChecked() || dontchangeheight) return;
	dontchangewidth = true;
	height -> setValue(qRound(width -> value() / diagram_ratio));
	dontchangewidth = false;
}

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
	
	// genere l'image
	if (!export_grid -> isChecked()) diagram -> setAffichageGrille(false);
	QImage image = diagram -> toImage(width -> value(), height -> value(), keep_aspect_ratio -> isChecked());
	if (!export_grid -> isChecked()) diagram -> setAffichageGrille(true);
	
	// convertit l'image en niveaux de gris si besoin
	if (!keep_colors -> isChecked()) {
		QVector<QRgb> ColorTab;
		for (int i = 0 ; i < 256 ; ++ i) ColorTab << qRgb(i, i, i);
		image = image.convertToFormat(QImage::Format_Indexed8, ColorTab, Qt::ThresholdDither);
	}
	
	// enregistre l'image dans le fichier
	image.save(&fichier, format_acronym.toUtf8().data());
	fichier.close();
	
	// fermeture du dialogue
	accept();
}

