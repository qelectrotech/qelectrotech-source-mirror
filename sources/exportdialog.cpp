/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "exportdialog.h"
#include "shownkinds.h"

#include "conductorsegment.h"
#include "createdxf.h"
#include "dxfexport.h"
#include "exportpropertieswidget.h"
#include "factory/elementpicturefactory.h"
#include "qet.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "dxfpaintdevice.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/crossrefitem.h"
#include "qetgraphicsitem/diagramimageitem.h"
#include "qetgraphicsitem/diagramtextitem.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "qetgraphicsitem/terminal.h"
#include "qeticons.h"
#include "qetmessagebox.h"

#include <QGraphicsSimpleTextItem>
#include <QSvgGenerator>
#include <QtXml>
#include <cmath>
#include <utility>

/**
	Constructeur
	@param project Le projet a exporter
	@param parent Le Widget parent de ce dialogue
*/
ExportDialog::ExportDialog(
		QETProject *project,
		QWidget *parent) : QDialog(parent)
{
	if (!project) return;
	
	// recupere le projet a exporter
	project_ = project;
	
	// recupere les parametres d'export definis dans la configuration de l'application
	ExportProperties default_export_properties = ExportProperties::defaultExportProperties();
	
	// on utilise le repertoire du projet a exporter si possible
	if (!project_ -> filePath().isEmpty()) {
		default_export_properties.destination_directory.setPath(project_ -> currentDir());
	}
	
	// la taille minimale du dialogue est fixee
	setMinimumSize(800, 590);
	resize(minimumSize());
	setWindowTitle(tr("Exporter les folios du projet", "window title"));

	// options d'export, dans le widget epw
	epw = new ExportPropertiesWidget(default_export_properties);
	
	// le dialogue comporte deux boutons
	buttons = new QDialogButtonBox(this);
	buttons -> setOrientation(Qt::Horizontal);
	buttons -> setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);
	QPushButton *export_button = buttons -> button(QDialogButtonBox::Save);
	export_button -> setText(tr("Exporter"));
	
	// disposition des elements

	QHBoxLayout *hLayout = new QHBoxLayout();
	hLayout -> addWidget(new QLabel(tr("Choisissez les folios que vous désirez exporter ainsi que leurs dimensions :")));
	selectAll   = new QPushButton();
	deSelectAll = new QPushButton();
	selectAll   -> setText(tr("Tout cocher"));
	deSelectAll -> setText(tr("Tout décocher"));
	hLayout -> addWidget(selectAll);
	hLayout -> addWidget(deSelectAll);
	connect(selectAll, &QPushButton::clicked, this, &ExportDialog::slot_selectAllClicked);
	connect(deSelectAll, &QPushButton::clicked, this, &ExportDialog::slot_deSelectAllClicked);


	QVBoxLayout *layout = new QVBoxLayout(this);
	layout -> addLayout(hLayout);
	layout -> addWidget(initDiagramsListPart(), 1);
	layout -> addWidget(epw);
	layout -> addWidget(buttons);
	
	// connexions signaux/slots
	connect(epw, &ExportPropertiesWidget::formatChanged, this, [this]() { slot_changeFilesExtension(); });
	connect(epw, &ExportPropertiesWidget::exportedAreaChanged, this, &ExportDialog::slot_changeUseBorder);
	connect(buttons, &QDialogButtonBox::accepted, this, &ExportDialog::slot_export);
	connect(buttons, &QDialogButtonBox::rejected, this, &ExportDialog::reject);
	
	// ajustement des extensions des fichiers
	slot_changeFilesExtension(true);

	QET::trackDialogGeometry(this);
}

/**
	Destructeur - ne fait rien
*/
ExportDialog::~ExportDialog()
{
	qDeleteAll(diagram_lines_);
}

/**
	@return le nombre de schemas coches (donc a exporter)
*/
int ExportDialog::diagramsToExportCount() const
{
	int checked_diagrams_count = 0;
	foreach(ExportDiagramLine *diagram_line, diagram_lines_.values()) {
		if (diagram_line -> must_export -> isChecked()) ++ checked_diagrams_count;
	}
	return(checked_diagrams_count);
}

/**
	Met en place la liste des schemas
	@return Le widget representant la liste des schemas
*/
QWidget *ExportDialog::initDiagramsListPart()
{
	preview_mapper_   = new QSignalMapper(this);
	width_mapper_     = new QSignalMapper(this);
	height_mapper_    = new QSignalMapper(this);
	ratio_mapper_     = new QSignalMapper(this);
	reset_mapper_     = new QSignalMapper(this);
	clipboard_mapper_ = new QSignalMapper(this);
	
	connect(preview_mapper_, &QSignalMapper::mappedInt, this, &ExportDialog::slot_previewDiagram);
	connect(width_mapper_, &QSignalMapper::mappedInt, this, &ExportDialog::slot_correctHeight);
	connect(height_mapper_, &QSignalMapper::mappedInt, this, &ExportDialog::slot_correctWidth);
	connect(ratio_mapper_, &QSignalMapper::mappedInt, this, &ExportDialog::slot_keepRatioChanged);
	connect(reset_mapper_, &QSignalMapper::mappedInt, this, &ExportDialog::slot_resetSize);
	connect(clipboard_mapper_, &QSignalMapper::mappedInt, this, &ExportDialog::slot_exportToClipBoard);
	
	diagrams_list_layout_ = new QGridLayout();
	
	int line_count = 0;
	diagrams_list_layout_ -> addWidget(new QLabel(tr("Titre du folio")),        line_count, 1, Qt::AlignHCenter | Qt::AlignVCenter);
	diagrams_list_layout_ -> addWidget(new QLabel(tr("Nom de fichier")),   line_count, 2, Qt::AlignHCenter | Qt::AlignVCenter);
	diagrams_list_layout_ -> addWidget(new QLabel(tr("Dimensions")),       line_count, 3, Qt::AlignHCenter | Qt::AlignVCenter);
	
	// remplit la liste
	foreach (Diagram *diagram, project_ -> diagrams()) {
		++ line_count;
		ExportDiagramLine *diagram_line = new ExportDiagramLine(diagram, diagramSize(diagram));
		diagram_lines_.insert(line_count, diagram_line);
		diagrams_list_layout_ -> addWidget(diagram_line -> must_export,    line_count, 0);
		diagrams_list_layout_ -> addWidget(diagram_line -> title_label,    line_count, 1);
		diagrams_list_layout_ -> addWidget(diagram_line -> file_name,      line_count, 2);
		diagrams_list_layout_ -> addLayout(diagram_line -> sizeLayout(),   line_count, 3);
		
		// si on decoche tous les schemas, on desactive le bouton "Exporter"
		connect(diagram_line -> must_export, &QCheckBox::toggled, this, &ExportDialog::slot_checkDiagramsCount);
		
		// mappings et signaux pour la gestion des dimensions du schema
		width_mapper_  -> setMapping(diagram_line -> width,      line_count);
		height_mapper_ -> setMapping(diagram_line -> height,     line_count);
		ratio_mapper_  -> setMapping(diagram_line -> keep_ratio, line_count);
		reset_mapper_  -> setMapping(diagram_line -> reset_size, line_count);
		connect(diagram_line -> width, qOverload<int>(&QSpinBox::valueChanged), width_mapper_, qOverload<>(&QSignalMapper::map));
		connect(diagram_line -> height, qOverload<int>(&QSpinBox::valueChanged), height_mapper_, qOverload<>(&QSignalMapper::map));
		connect(diagram_line -> keep_ratio, &QPushButton::toggled, ratio_mapper_, qOverload<>(&QSignalMapper::map));
		connect(diagram_line -> reset_size, &QPushButton::clicked, reset_mapper_, qOverload<>(&QSignalMapper::map));
		
		// mappings et signaux pour l'apercu du schema
		preview_mapper_ -> setMapping(diagram_line -> preview, line_count);
		connect(diagram_line -> preview, &QPushButton::clicked, preview_mapper_, qOverload<>(&QSignalMapper::map));
		
		// mappings et signaux pour l'export du schema vers le presse-papier
		clipboard_mapper_ -> setMapping(diagram_line -> clipboard, line_count);
		connect(diagram_line -> clipboard, &QPushButton::clicked, clipboard_mapper_, qOverload<>(&QSignalMapper::map));
	}
	
	QWidget *widget_diagrams_list = new QWidget();
	widget_diagrams_list -> setLayout(diagrams_list_layout_);
	
	QScrollArea *scroll_diagrams_list = new QScrollArea();
	scroll_diagrams_list -> setWidget(widget_diagrams_list);
	
	return(scroll_diagrams_list);
}

void ExportDialog::slot_selectAllClicked()
{
	foreach (ExportDiagramLine *diagramLine, diagram_lines_) {
		diagramLine -> must_export -> setChecked(true);
	}
}

void ExportDialog::slot_deSelectAllClicked()
{
	foreach (ExportDiagramLine *diagramLine, diagram_lines_) {
		diagramLine -> must_export -> setChecked(false);
	}
}



/**
	@param diagram Un schema
	@return le rapport largeur / hauteur du schema
*/
qreal ExportDialog::diagramRatio(Diagram *diagram)
{
	QSize diagram_size = diagramSize(diagram);
	qreal diagram_ratio = (qreal)diagram_size.width() / (qreal)diagram_size.height();
	return(diagram_ratio);
}

/**
	@param diagram Un schema
	@return les dimensions du schema, en tenant compte du type d'export : cadre
	ou elements
*/
QSize ExportDialog::diagramSize(Diagram *diagram)
{
	// sauvegarde le parametre useBorder du schema
	bool state_useBorder = diagram -> useBorder();
	
	// applique le useBorder adequat et calcule le ratio
	diagram -> setUseBorder(epw -> exportProperties().exported_area == QET::BorderArea);
	QSize diagram_size = diagram -> imageSize();
	
	// restaure le parametre useBorder du schema
	diagram -> setUseBorder(state_useBorder);
	
	return(diagram_size);
}

/**
	Cette methode ajuste la largeur d'un des schemas a exporter en fonction de
	sa hauteur si et seulement si l'option "Conserver les proportions" est
	activee pour ce schema.
	@param diagram_id numero du schema concerne
*/
void ExportDialog::slot_correctWidth(int diagram_id)
{
	// recupere l'ExportDiagramLine concernee
	ExportDialog::ExportDiagramLine *current_diagram = diagram_lines_[diagram_id];
	if (!current_diagram) return;
	
	// ne fait rien si l'option "Conserver les proportions" n'est pas activee
	if (!(current_diagram -> keep_ratio -> isChecked())) return;
	
	// recupere les proportions du schema
	qreal diagram_ratio = diagramRatio(current_diagram -> diagram);
	
	// ajuste la largeur
	current_diagram -> width -> blockSignals(true);
	current_diagram -> width -> setValue(qRound(current_diagram -> height -> value() * diagram_ratio));
	current_diagram -> width -> blockSignals(false);
}

/**
	Cette methode ajuste la hauteur d'un des schemas a exporter en fonction de
	sa largeur si et seulement si l'option "Conserver les proportions" est
	activee pour ce schema.
	@param diagram_id numero du schema concerne
*/
void ExportDialog::slot_correctHeight(int diagram_id)
{
	// recupere l'ExportDiagramLine concernee
	ExportDialog::ExportDiagramLine *current_diagram = diagram_lines_[diagram_id];
	if (!current_diagram) return;
	
	// ne fait rien si l'option "Conserver les proportions" n'est pas activee
	if (!(current_diagram -> keep_ratio -> isChecked())) return;
	
	// recupere les proportions du schema
	qreal diagram_ratio = diagramRatio(current_diagram -> diagram);
	
	// ajuste la hauteur
	current_diagram -> height -> blockSignals(true);
	current_diagram -> height -> setValue(qRound(current_diagram -> width -> value() / diagram_ratio));
	current_diagram -> height -> blockSignals(false);
}

/**
	Prend en compte le fait qu'il faut desormais conserver ou non les
	proportions d'un des schemas
	@param diagram_id numero du schema concerne
*/
void ExportDialog::slot_keepRatioChanged(int diagram_id)
{
	// recupere l'ExportDiagramLine concernee
	ExportDialog::ExportDiagramLine *current_diagram = diagram_lines_[diagram_id];
	if (!current_diagram) return;
	
	// gere l'icone du bouton "Conserver les proportions"
	if (current_diagram -> keep_ratio -> isChecked()) {
		current_diagram -> keep_ratio -> setIcon(QET::Icons::ObjectLocked);
	} else {
		current_diagram -> keep_ratio -> setIcon(QET::Icons::ObjectUnlocked);
	}
	
	// ne fait rien si l'option "Conserver les proportions" n'est pas activee
	if (!(current_diagram -> keep_ratio -> isChecked())) return;
	
	// au contraire, si elle est activee, ajuste la hauteur en fonction de la largeur
	slot_correctHeight(diagram_id);
}

/**
	Reinitialise les dimensions d'un des schemas
	@param diagram_id numero du schema concerne
*/
void ExportDialog::slot_resetSize(int diagram_id)
{
	// recupere l'ExportDiagramLine concernee
	ExportDialog::ExportDiagramLine *current_diagram = diagram_lines_[diagram_id];
	if (!current_diagram) return;
	
	// recupere la taille du schema
	QSize diagram_size = diagramSize(current_diagram -> diagram);
	
	// reinitialise les champs largeur et hauteur
	current_diagram -> width  -> blockSignals(true);
	current_diagram -> height -> blockSignals(true);
	current_diagram -> width  -> setValue(diagram_size.width());
	current_diagram -> height -> setValue(diagram_size.height());
	current_diagram -> width  -> blockSignals(false);
	current_diagram -> height -> blockSignals(false);
}

/**
	Genere l'image a exporter
	@param diagram Schema a exporter en SVG
	@param width  Largeur de l'export
	@param height Hauteur de l'export
	@param keep_aspect_ratio True pour conserver le ratio, false sinon
	@return l'image a exporter
*/
QImage ExportDialog::generateImage(
		Diagram *diagram,
		int width,
		int height,
		bool keep_aspect_ratio)
{
	saveReloadDiagramParameters(diagram, true);
	
	QImage image(width, height, QImage::Format_RGB32);
	diagram -> toPaintDevice(
		image,
		width,
		height,
		keep_aspect_ratio ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio
	);
	
	saveReloadDiagramParameters(diagram, false);
	
	return(image);
}

/**
	Sauve ou restaure les parametres du schema
	@param diagram Schema dont on sauve ou restaure les parametres
	@param save true pour memoriser les parametres du schema et appliquer ceux
	definis par le formulaire, false pour restaurer les parametres
*/
void ExportDialog::saveReloadDiagramParameters(Diagram *diagram, bool save) {
	static ExportProperties state_exportProperties;
	
	if (save) {
		// memorise les parametres relatifs au schema tout en appliquant les nouveaux
		state_exportProperties = diagram -> applyProperties(epw -> exportProperties());
	} else {
		// restaure les parametres relatifs au schema
		diagram -> applyProperties(state_exportProperties);
	}
}

/**
	Exporte le schema en SVG
	@param diagram Schema a exporter en SVG
	@param width  Largeur de l'export SVG
	@param height Hauteur de l'export SVG
	@param keep_aspect_ratio True pour conserver le ratio, false sinon
	@param io_device Peripherique de sortie pour le code SVG (souvent : un fichier)
*/
void ExportDialog::generateSvg(
		Diagram *diagram,
		int width,
		int height,
		bool keep_aspect_ratio,
		QIODevice &io_device)
{
	saveReloadDiagramParameters(diagram, true);

	// set the transparency for the SVG-Background:
	if (epw->exportProperties().draw_bg_transparent == true) {
		diagram->background_color.setAlpha(0);
		} else {
		diagram->background_color.setAlpha(255);
		}
	
	// genere une QPicture a partir du schema
	QPicture picture;
	diagram -> toPaintDevice(
		picture,
		width,
		height,
		keep_aspect_ratio ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio
	);
	
	// "joue" la QPicture sur un QSvgGenerator
	QSvgGenerator svg_engine;
	svg_engine.setSize(QSize(width, height));
	svg_engine.setViewBox(QRect(0, 0, width*0.75, height*0.75));
	svg_engine.setOutputDevice(&io_device);
	QPainter svg_painter(&svg_engine);
	picture.play(&svg_painter);
	
	saveReloadDiagramParameters(diagram, false);
}


/**
	Slot effectuant les exports apres la validation du dialogue.
*/
void ExportDialog::slot_export()
{
	// recupere la liste des schemas a exporter
	QList<ExportDiagramLine *> diagrams_to_export;
	foreach(ExportDiagramLine *diagram_line, diagram_lines_.values()) {
		if (diagram_line -> must_export -> isChecked()) {
			diagrams_to_export << diagram_line;
		}
	}
	
	// verification #1 : chaque schema coche doit avoir un nom de fichier distinct
	QSet<QString> filenames;
	foreach(ExportDiagramLine *diagram_line, diagrams_to_export) {
		QString diagram_file = diagram_line -> file_name -> text();
		if (!diagram_file.isEmpty()) {
			filenames << diagram_file;
		}
	}
	if (filenames.count() != diagrams_to_export.count()) {
		QET::QetMessageBox::warning(
			this,
			tr("Noms des fichiers cibles", "message box title"),
			tr(
				"Vous devez entrer un nom de fichier non vide et unique pour chaque "
				"folio à exporter.",
				"message box content"
			)
		);
		return;
	}
	
	// verification #2 : un chemin vers un dossier doit avoir ete specifie
	
	QDir target_dir_path(epw -> exportProperties().destination_directory);
	if (!target_dir_path.exists()) {
		QET::QetMessageBox::warning(
			this,
			tr("Dossier non spécifié", "message box title"),
			tr("Vous devez spécifier le chemin du dossier dans lequel seront enregistrés les fichiers images.", "message box content"),
			QMessageBox::Ok
		);
		return;
	}
	
	// Warn once, up front, rather than per-diagram: this DXF dialect
	// (AC1006, AutoCAD R10) has no raster image representation at all
	// (IMAGE/IMAGEDEF wasn't introduced until R2000, over a decade
	// later, and even there the picture is never embedded, only
	// referenced by external file path) -- so any image ends up as a
	// placeholder rectangle outline instead (see DxfPaintEngine::
	// drawPixmap()), with its position/size/rotation/skew preserved but
	// not its actual content.
	if (epw -> exportProperties().format.compare(QLatin1String("dxf"), Qt::CaseInsensitive) == 0)
	{
		bool any_images = false;
		for (ExportDiagramLine *diagram_line : std::as_const(diagrams_to_export))
		{
			for (QGraphicsItem *item : diagram_line->diagram->items())
			{
					//A picture hidden by View > Show is not exported
				if (qgraphicsitem_cast<DiagramImageItem *>(item)
						&& !ShownKinds::isHidden(item)) {
					any_images = true;
					break;
				}
			}
			if (any_images) break;
		}

		if (any_images)
		{
			QET::QetMessageBox::warning(
				this,
				tr("Images non incluses dans l'export DXF", "message box title"),
				tr("Le format DXF utilisé ici (AC1006) ne permet pas d'inclure d'image. "
				   "Les images seront représentées uniquement par un rectangle de contour "
				   "(position, taille, rotation et inclinaison conservées), sans le contenu de l'image.",
				   "message box content")
			);
		}
	}
	
	// exporte chaque schema a exporter
	foreach(ExportDiagramLine *diagram_line, diagrams_to_export) {
		exportDiagram(diagram_line);
	}
	
	// fermeture du dialogue
	accept();
}

/**
	Exporte un schema
	@param diagram_line La ligne decrivant le schema a exporter et la maniere
	de l'exporter
*/
void ExportDialog::exportDiagram(ExportDiagramLine *diagram_line) {
	ExportProperties export_properties(epw -> exportProperties());
	
	// recupere le format a utiliser (acronyme et extension)
	QString format_acronym = export_properties.format;
	QString format_extension = "." + format_acronym.toLower();
	
	// determine le nom de fichier a utiliser
	QString diagram_path = diagram_line -> file_name -> text();
	
	// determine le chemin du fichier
	QDir target_dir_path(export_properties.destination_directory);
	diagram_path = target_dir_path.absoluteFilePath(diagram_path);
	
	// recupere des informations sur le fichier specifie
	QFileInfo file_infos(diagram_path);
	
	// verifie qu'il est possible d'ecrire dans le fichier en question
	if (file_infos.exists() && !file_infos.isWritable()) {
		QET::QetMessageBox::critical(
			this,
			tr("Impossible d'écrire dans ce fichier", "message box title"),
			QString(
				tr(
					"Il semblerait que vous n'ayez pas les permissions "
					"nécessaires pour écrire dans le fichier %1.",
					"message box content"
				)
			).arg(diagram_path),
			QMessageBox::Ok
		);
		return;
	}
	
	// ouvre le fichier
	QFile target_file(diagram_path);
	
	// enregistre l'image dans le fichier
	if (format_acronym == "SVG") {
		generateSvg(
			diagram_line -> diagram,
			diagram_line -> width  -> value(),
			diagram_line -> height -> value(),
			diagram_line -> keep_ratio -> isChecked(),
			target_file
		);
	} else if (format_acronym == "DXF") {
		DxfExport::write(
			diagram_line -> diagram,
			diagram_line -> width  -> value(),
			diagram_line -> height -> value(),
			diagram_path,
			export_properties
		);
	} else {
		QImage image = generateImage(
			diagram_line -> diagram,
			diagram_line -> width  -> value(),
			diagram_line -> height -> value(),
			diagram_line -> keep_ratio -> isChecked()
		);
		image.save(&target_file, format_acronym.toUtf8().data());
	}
	target_file.close();
}

/**
	Slot appele lorsque l'utilisateur change la zone du schema qui doit etre
	exportee. Il faut alors ajuster les dimensions des schemas.
*/
void ExportDialog::slot_changeUseBorder()
{
	// parcourt les schemas a exporter
	foreach(int diagram_id, diagram_lines_.keys()) {
		ExportDiagramLine *diagram_line = diagram_lines_[diagram_id];
		
		// corrige les dimensions des schemas dont il faut preserver le ratio
		if (diagram_line -> keep_ratio -> isChecked()) {
			slot_correctHeight(diagram_id);
		}
	}
}

/**
	Ce slot est appele quand un schema a ete coche ou decoche.
	Il active ou desactive le bouton "Exporter" en fonction du nombre de
	schemas coches, et il garde au plus un schema coche si on exporte vers
	le presse-papier.
*/
void ExportDialog::slot_checkDiagramsCount()
{
	QPushButton *export_button = buttons -> button(QDialogButtonBox::Save);
	export_button -> setEnabled(diagramsToExportCount());
}

/**
	Modifie les extensions des fichiers en fonction du format selectionne
	@param force_extension true pour ajouter l'extension si elle n'est pas
	presente, false pour se contenter de la modifier si elle est incorrecte.
*/
void ExportDialog::slot_changeFilesExtension(bool force_extension) {
	// recupere le format a utiliser (acronyme et extension)
	QString format_acronym = epw -> exportProperties().format;
	QString format_extension = "." + format_acronym.toLower();

	// set maximum width / height according limitations in QPainter
	if ((format_extension == ".bmp") ||
	    (format_extension == ".jpg") ||
	    (format_extension == ".png")) {
		foreach (auto line, diagram_lines_.values() ) {
			line->width ->setRange(1, RasterMaxSize);
			line->height->setRange(1, RasterMaxSize);
		}
	} else {
		foreach (auto line, diagram_lines_.values() ) {
			line->width ->setRange(1, GeneralMaxSize);
			line->height->setRange(1, GeneralMaxSize);
		}
	}

	// parcourt les schemas a exporter
	foreach(ExportDiagramLine *diagram_line, diagram_lines_.values()) {
		QString diagram_filename = diagram_line -> file_name -> text();
		
		// cas 1 : l'extension est presente et correcte : on ne fait rien
		if (diagram_filename.endsWith(format_extension, Qt::CaseInsensitive)) {
			continue;
		}
		
		QFileInfo diagram_filename_info(diagram_filename);
		// cas 2 : l'extension est absente
		if (diagram_filename_info.suffix().isEmpty()) {
			if (force_extension) {
				diagram_filename = diagram_filename_info.completeBaseName() + format_extension;
			}
		} else {
			// cas 3 : l'extension est presente mais erronee
			diagram_filename = diagram_filename_info.completeBaseName() + format_extension;
		}
		
		diagram_line -> file_name -> setText(diagram_filename);
	}
}

/**
	Cette methode fait apparaitre un dialogue permettant de previsualiser un
	des schemas a exporter
	@param diagram_id numero du schema a previsualiser
*/
void ExportDialog::slot_previewDiagram(int diagram_id) {
	// recupere l'ExportDiagramLine concernee
	ExportDialog::ExportDiagramLine *current_diagram = diagram_lines_[diagram_id];
	if (!current_diagram) return;
	
	// initialise un dialogue
	QDialog preview_dialog;
	preview_dialog.setWindowTitle(tr("Aperçu"));
	preview_dialog.setWindowState(preview_dialog.windowState() | Qt::WindowMaximized);
	
	QGraphicsScene *preview_scene = new QGraphicsScene();
	preview_scene -> setBackgroundBrush(Qt::lightGray);
	QGraphicsView *preview_view = new QGraphicsView(preview_scene);
	preview_view -> setDragMode(QGraphicsView::ScrollHandDrag);
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
	connect(buttons, &QDialogButtonBox::accepted, &preview_dialog, &QDialog::accept);
	
	QVBoxLayout *vboxlayout1 = new QVBoxLayout();
	vboxlayout1 -> addWidget(preview_view);
	vboxlayout1 -> addWidget(buttons);
	preview_dialog.setLayout(vboxlayout1);
	
	// genere le nouvel apercu
	QImage preview_image = generateImage(
		current_diagram -> diagram,
		current_diagram -> width  -> value(),
		current_diagram -> height -> value(),
		current_diagram -> keep_ratio -> isChecked()
	);
	
	// nettoie l'apercu
	foreach (QGraphicsItem *qgi, preview_scene -> items()) {
		preview_scene -> removeItem(qgi);
		delete qgi;
	}
	
	// ajoute le nouvel apercu
	QGraphicsPixmapItem *qgpi = new QGraphicsPixmapItem(QPixmap::fromImage(preview_image));
	preview_scene -> addItem(qgpi);
	preview_scene -> setSceneRect(QRectF(0.0, 0.0, preview_image.width(), preview_image.height()));
	
	// montre l'apercu
	preview_dialog.exec();
}

/**
	Cette methode exporte un schema vers le presse-papier
	@param diagram_id numero du schema a previsualiser
*/
void ExportDialog::slot_exportToClipBoard(int diagram_id) {
	// recupere l'ExportDiagramLine concernee
	ExportDialog::ExportDiagramLine *diagram_line = diagram_lines_[diagram_id];
	if (!diagram_line) return;
	
	// recupere le format a utiliser (acronyme et extension)
	QString format_acronym = epw -> exportProperties().format;
	
	QClipboard *clipboard = QApplication::clipboard();
	
	// enregistre l'image dans le fichier
	if (format_acronym == "SVG") {
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		generateSvg(
			diagram_line -> diagram,
			diagram_line -> width  -> value(),
			diagram_line -> height -> value(),
			diagram_line -> keep_ratio -> isChecked(),
			buffer
		);
		buffer.close();
		clipboard -> setText(ba);
	} else {
		QImage image = generateImage(
			diagram_line -> diagram,
			diagram_line -> width  -> value(),
			diagram_line -> height -> value(),
			diagram_line -> keep_ratio -> isChecked()
		);
		clipboard -> setImage(image);
	}
}

/**
	Constructeur
	@param dia Schema concerne, 
	@param diagram_size taille du schema tenant compte des parametres d'export
*/
ExportDialog::ExportDiagramLine::ExportDiagramLine(Diagram *dia, QSize diagram_size) {
	diagram = dia;
	must_export = new QCheckBox();
	must_export -> setChecked(true);
	
	// titre et nom de fichier du schema
	QString diagram_title = diagram -> title();
	QString diagram_index = QString::number(diagram -> folioIndex()+1);
	//QString diagram_folio_label = diagram -> border_and_titleblock.finalfolio();
	if (diagram_title.isEmpty()) diagram_title = QObject::tr("Folio sans titre");
	QString diagram_filename = diagram -> title();
	if (diagram_filename.isEmpty()) diagram_filename = QObject::tr("schema");
	diagram_filename = QET::stringToFileName(diagram_index % "_" % diagram_filename);
	
	title_label = new QLabel(diagram_title);
	
	file_name = new QLineEdit();
	file_name -> setText(diagram_filename);
	file_name -> setMinimumWidth(280);
	
	width = new QSpinBox();
	width -> setRange(1, GeneralMaxSize);
	width -> setSuffix(tr("px"));
	width -> setValue(diagram_size.width());
	
	height = new QSpinBox();
	height -> setRange(1, GeneralMaxSize);
	height -> setSuffix(tr("px"));
	height -> setValue(diagram_size.height());
	
	x_label = new QLabel("×");
	
	keep_ratio = new QPushButton();
	keep_ratio -> setCheckable(true);
	keep_ratio -> setChecked(true);
	keep_ratio -> setIcon(QET::Icons::ObjectLocked);
	keep_ratio -> setToolTip(QObject::tr("Conserver les proportions"));
	
	reset_size = new QPushButton();
	reset_size -> setIcon(QET::Icons::Start);
	reset_size -> setToolTip(QObject::tr("Réinitialiser les dimensions"));
	
	preview = new QPushButton();
	preview -> setIcon(QET::Icons::ZoomOriginal);
	preview -> setToolTip(QObject::tr("Aperçu"));
	
	clipboard = new QPushButton();
	clipboard -> setIcon(QET::Icons::IC_CopyFile);
	clipboard -> setToolTip(QObject::tr("Exporter vers le presse-papier"));
}

/**
	Destructeur
*/
ExportDialog::ExportDiagramLine::~ExportDiagramLine()
{
}

/**
	@return un layout contenant les widgets necessaires a la gestion de la
	taille d'un schema avant son export.
*/
QBoxLayout *ExportDialog::ExportDiagramLine::sizeLayout()
{
	QHBoxLayout *layout = new QHBoxLayout();
	layout -> addWidget(width);
	layout -> addWidget(x_label);
	layout -> addWidget(height);
	layout -> addWidget(keep_ratio);
	layout -> addWidget(reset_size);
	layout -> addWidget(preview);
	layout -> addWidget(clipboard);
	return(layout);
}
