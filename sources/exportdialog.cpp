/*
	Copyright 2006-2014 The QElectroTech Team
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
#include <QSvgGenerator>
#include <cmath>
#include <QtXml>
#include "qeticons.h"
#include "qetmessagebox.h"
#include "exportpropertieswidget.h"
#include "qetdiagrameditor.h"
#include "createdxf.h"
#include "conductorsegment.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/diagramtextitem.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/customelement.h"
#include "qetgraphicsitem/elementtextitem.h"
#include "qetgraphicsitem/ghostelement.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/diagramimageitem.h"

/**
	Constructeur
	@param project Le projet a exporter
	@param parent Le Widget parent de ce dialogue
*/
ExportDialog::ExportDialog(QETProject *project, QWidget *parent) : QDialog(parent) {
	if (!project) return;
	
	// recupere le projet a exporter
	project_ = project;
	
	// recupere les parametres d'export definis dans la configuration de l'application
	ExportProperties default_export_properties = QETDiagramEditor::defaultExportProperties();
	
	// on utilise le repertoire du projet a exporter si possible
	if (!project_ -> filePath().isEmpty()) {
		default_export_properties.destination_directory = project_ -> currentDir();
	}
	
	// la taille minimale du dialogue est fixee
	setMinimumSize(800, 390);
	resize(minimumSize());
	setWindowTitle(tr("Exporter les sch\351mas du projet", "window title"));

	// options d'export, dans le widget epw
	epw = new ExportPropertiesWidget(default_export_properties);
	
	// le dialogue comporte deux boutons
	buttons = new QDialogButtonBox(this);
	buttons -> setOrientation(Qt::Horizontal);
	buttons -> setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);
	QPushButton *export_button = buttons -> button(QDialogButtonBox::Save);
	export_button -> setText(tr("Exporter"));
	
	// disposition des elements
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout -> addWidget(new QLabel(tr("Choisissez les sch\351mas que vous d\351sirez exporter ainsi que leurs dimensions :")));
	layout -> addWidget(initDiagramsListPart(), 1);
	layout -> addWidget(epw);
	layout -> addWidget(buttons);
	
	// connexions signaux/slots
	connect(epw,     SIGNAL(formatChanged()),       this, SLOT(slot_changeFilesExtension()));
	connect(epw,     SIGNAL(exportedAreaChanged()), this, SLOT(slot_changeUseBorder()));
	connect(buttons, SIGNAL(accepted()),            this, SLOT(slot_export()));
	connect(buttons, SIGNAL(rejected()),            this, SLOT(reject()));
	
	// ajustement des extensions des fichiers
	slot_changeFilesExtension(true);
}

/**
	Destructeur - ne fait rien
*/
ExportDialog::~ExportDialog() {
}

/**
	@return le nombre de schemas coches (donc a exporter)
*/
int ExportDialog::diagramsToExportCount() const {
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
QWidget *ExportDialog::initDiagramsListPart() {
	preview_mapper_   = new QSignalMapper(this);
	width_mapper_     = new QSignalMapper(this);
	height_mapper_    = new QSignalMapper(this);
	ratio_mapper_     = new QSignalMapper(this);
	reset_mapper_     = new QSignalMapper(this);
	clipboard_mapper_ = new QSignalMapper(this);
	
	connect(preview_mapper_,   SIGNAL(mapped(int)), this, SLOT(slot_previewDiagram(int)));
	connect(width_mapper_,     SIGNAL(mapped(int)), this, SLOT(slot_correctHeight(int)));
	connect(height_mapper_,    SIGNAL(mapped(int)), this, SLOT(slot_correctWidth(int)));
	connect(ratio_mapper_,     SIGNAL(mapped(int)), this, SLOT(slot_keepRatioChanged(int)));
	connect(reset_mapper_,     SIGNAL(mapped(int)), this, SLOT(slot_resetSize(int)));
	connect(clipboard_mapper_, SIGNAL(mapped(int)), this, SLOT(slot_exportToClipBoard(int)));
	
	diagrams_list_layout_ = new QGridLayout();
	
	int line_count = 0;
	diagrams_list_layout_ -> addWidget(new QLabel(tr("Sch\351ma")),        line_count, 1, Qt::AlignHCenter | Qt::AlignVCenter);
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
		connect(diagram_line -> must_export, SIGNAL(toggled(bool)), this, SLOT(slot_checkDiagramsCount()));
		
		// mappings et signaux pour la gestion des dimensions du schema
		width_mapper_  -> setMapping(diagram_line -> width,      line_count);
		height_mapper_ -> setMapping(diagram_line -> height,     line_count);
		ratio_mapper_  -> setMapping(diagram_line -> keep_ratio, line_count);
		reset_mapper_  -> setMapping(diagram_line -> reset_size, line_count);
		connect(diagram_line -> width,      SIGNAL(valueChanged(int)), width_mapper_,  SLOT(map()));
		connect(diagram_line -> height,     SIGNAL(valueChanged(int)), height_mapper_, SLOT(map()));
		connect(diagram_line -> keep_ratio, SIGNAL(toggled(bool)),     ratio_mapper_,  SLOT(map()));
		connect(diagram_line -> reset_size, SIGNAL(clicked(bool)),     reset_mapper_,  SLOT(map()));
		
		// mappings et signaux pour l'apercu du schema
		preview_mapper_ -> setMapping(diagram_line -> preview, line_count);
		connect(diagram_line -> preview, SIGNAL(clicked(bool)), preview_mapper_, SLOT(map()));
		
		// mappings et signaux pour l'export du schema vers le presse-papier
		clipboard_mapper_ -> setMapping(diagram_line -> clipboard, line_count);
		connect(diagram_line -> clipboard, SIGNAL(clicked(bool)), clipboard_mapper_, SLOT(map()));
	}
	
	QWidget *widget_diagrams_list = new QWidget();
	widget_diagrams_list -> setLayout(diagrams_list_layout_);
	
	QScrollArea *scroll_diagrams_list = new QScrollArea();
	scroll_diagrams_list -> setWidget(widget_diagrams_list);
	
	return(scroll_diagrams_list);
}

/**
	@param diagram Un schema
	@return le rapport largeur / hauteur du schema
*/
qreal ExportDialog::diagramRatio(Diagram *diagram) {
	QSize diagram_size = diagramSize(diagram);
	qreal diagram_ratio = (qreal)diagram_size.width() / (qreal)diagram_size.height();
	return(diagram_ratio);
}

/**
	@param diagram Un schema
	@return les dimensions du schema, en tenant compte du type d'export : cadre
	ou elements
*/
QSize ExportDialog::diagramSize(Diagram *diagram) {
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
void ExportDialog::slot_correctWidth(int diagram_id) {
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
void ExportDialog::slot_correctHeight(int diagram_id) {
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
void ExportDialog::slot_keepRatioChanged(int diagram_id) {
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
void ExportDialog::slot_resetSize(int diagram_id) {
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
QImage ExportDialog::generateImage(Diagram *diagram, int width, int height, bool keep_aspect_ratio) {
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
void ExportDialog::generateSvg(Diagram *diagram, int width, int height, bool keep_aspect_ratio, QIODevice &io_device) {
	saveReloadDiagramParameters(diagram, true);
	
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
	svg_engine.setOutputDevice(&io_device);
	QPainter svg_painter(&svg_engine);
	picture.play(&svg_painter);
	
	saveReloadDiagramParameters(diagram, false);
}

/**
	Exporte le schema en DXF
	@param diagram Schema a exporter en DXF
	@param width  Largeur de l'export DXF
	@param height Hauteur de l'export DXF
	@param keep_aspect_ratio True pour conserver le ratio, false sinon
	@param io_device Peripherique de sortie pour le code DXF (souvent : un fichier)
*/
void ExportDialog::generateDxf(Diagram *diagram, int width, int height, bool keep_aspect_ratio, QString &file_path) {

	project_ -> setFilePath(file_path);

	width  -= 2*Diagram::margin;
	height -= 2*Diagram::margin;

	Createdxf::xScale = Createdxf::sheetWidth  / double(width);
	Createdxf::yScale = Createdxf::sheetHeight / double(height);

	Createdxf::dxfBegin(file_path);

	//Add project elements (lines, rectangles, circles, texts) to dxf file
	Createdxf::drawRectangle(file_path, 0, 0, double(width)*Createdxf::xScale, double(height)*Createdxf::yScale, 0);
	diagram -> border_and_titleblock.drawDxf(width, height, keep_aspect_ratio, file_path, 0);

	// Build the lists of elements.
	QList<Element *> list_elements;
	QList<Conductor *> list_conductors;
	QList<DiagramTextItem *> list_texts;
	QList<DiagramImageItem *> list_images;

	// Determine les elements a "XMLiser"
	foreach(QGraphicsItem *qgi, diagram -> items()) {
		if (Element *elmt = qgraphicsitem_cast<Element *>(qgi)) {
			list_elements << elmt;
		} else if (Conductor *f = qgraphicsitem_cast<Conductor *>(qgi)) {
			list_conductors << f;
		} else if (IndependentTextItem *iti = qgraphicsitem_cast<IndependentTextItem *>(qgi)) {
			list_texts << iti;
		} else if (DiagramImageItem *dii = qgraphicsitem_cast<DiagramImageItem *>(qgi)) {
			list_images << dii;
		}
	}

	//Draw elements
	foreach(Element *elmt, list_elements) {

		double rotation_angle = elmt -> orientation() * 90;

		qreal elem_pos_x = elmt -> pos().x();
		qreal elem_pos_y = elmt -> pos().y();// - (diagram -> margin / 2);

		qreal hotspot_x = (elem_pos_x) * Createdxf::xScale;
		qreal hotspot_y = Createdxf::sheetHeight - (elem_pos_y) * Createdxf::yScale;

		QList<ElementTextItem *> elmt_text = elmt -> texts();
		foreach(ElementTextItem *dti, elmt_text) {
			qreal fontSize = dti -> font().pointSizeF();
			if (fontSize < 0)
				fontSize = dti -> font().pixelSize();
			fontSize *= Createdxf::yScale;
			qreal x = elem_pos_x + dti -> pos().x();
			qreal y = elem_pos_y + dti -> pos().y();
			x *= Createdxf::xScale;
			y = Createdxf::sheetHeight - (y * Createdxf::yScale);// - fontSize;
			QPointF transformed_point = rotation_transformed(x, y, hotspot_x, hotspot_y, rotation_angle);
			x = transformed_point.x();
			y = transformed_point.y();
			QStringList lines = dti -> toPlainText().split('\n');
			y += (fontSize/2) * (lines.count()-1);
			foreach (QString line, lines) {
				qreal angle = 360 - (dti -> rotationAngle() + rotation_angle);
				if (line.size() > 0 && line != "_" )
					Createdxf::drawText(file_path, line, x, y, fontSize, angle, 0);

				angle += 1080;
				// coordinates for next line
				if (int(angle) % 360 == 0) // no rotation
					y -= fontSize*1.06;
				else if (int(angle - 180) % 360 == 0) // 180 degrees rotation
					y += fontSize*1.06;
				else if (int(angle - 270) % 360 == 0) // 270 degrees rotation
					x -= fontSize*1.06;
				else // ((angle - 90) % 360 == 0)  90 degrees rotation
					x += fontSize*1.06;
			}

		}

		QList<QLineF *> elmt_line = elmt -> lines();
		foreach(QLineF *line, elmt_line) {
			qreal x1 = (elem_pos_x + line -> p1().x()) * Createdxf::xScale;
			qreal y1 = Createdxf::sheetHeight - (elem_pos_y + line -> p1().y()) * Createdxf::yScale;
			QPointF transformed_point = rotation_transformed(x1, y1, hotspot_x, hotspot_y, rotation_angle);
			x1 = transformed_point.x();
			y1 = transformed_point.y();
			qreal x2 = (elem_pos_x + line -> p2().x()) * Createdxf::xScale;
			qreal y2 = Createdxf::sheetHeight - (elem_pos_y + line -> p2().y()) * Createdxf::yScale;
			transformed_point = rotation_transformed(x2, y2, hotspot_x, hotspot_y, rotation_angle);
			x2 = transformed_point.x();
			y2 = transformed_point.y();
			Createdxf::drawLine(file_path, x1, y1, x2, y2, 0);
		}

		QList<QRectF *> elmt_rectangle = elmt -> rectangles();
		foreach(QRectF *rect, elmt_rectangle) {
			qreal x1 = (elem_pos_x + rect -> bottomLeft().x()) * Createdxf::xScale;
			qreal y1 = Createdxf::sheetHeight - (elem_pos_y + rect -> bottomLeft().y()) * Createdxf::yScale;
			qreal w = rect -> width() * Createdxf::xScale;
			qreal h = rect -> height() * Createdxf::yScale;
			// opposite corner
			qreal x2 = x1 + w;
			qreal y2 = y1 + h;
			QPointF transformed_point = rotation_transformed(x1, y1, hotspot_x, hotspot_y, rotation_angle);
			x1 = transformed_point.x();
			y1 = transformed_point.y();
			transformed_point = rotation_transformed(x2, y2, hotspot_x, hotspot_y, rotation_angle);
			x2 = transformed_point.x();
			y2 = transformed_point.y();
			qreal bottom_left_x = (x1 < x2) ? x1 : x2;
			qreal bottom_left_y = (y1 < y2) ? y1 : y2;
			w = (x1 < x2) ? x2-x1 : x1-x2;
			h = (y1 < y2) ? y2-y1 : y1-y2;
			Createdxf::drawRectangle(file_path, bottom_left_x, bottom_left_y, w, h, 0);
		}

		QList<QRectF *> elmt_circle = elmt -> circles();
		foreach(QRectF *circle_rect, elmt_circle) {
			qreal x1 = (elem_pos_x + circle_rect ->center().x()) * Createdxf::xScale;
			qreal y1 = Createdxf::sheetHeight - (elem_pos_y + circle_rect -> center().y()) * Createdxf::yScale;
			qreal r = circle_rect -> width() * Createdxf::xScale / 2;
			QPointF transformed_point = rotation_transformed(x1, y1, hotspot_x, hotspot_y, rotation_angle);
			x1 = transformed_point.x();
			y1 = transformed_point.y();
			Createdxf::drawCircle(file_path, r, x1, y1, 0);
		}

		QList<QVector<QPointF> *> elmt_polygon = elmt -> polygons();
		foreach(QVector<QPointF> *polygon, elmt_polygon) {
			if (polygon -> size() == 0)
				continue;
			qreal x1 = (elem_pos_x + polygon -> at(0).x()) * Createdxf::xScale;
			qreal y1 = Createdxf::sheetHeight - (elem_pos_y + polygon -> at(0).y()) * Createdxf::yScale;
			QPointF transformed_point = rotation_transformed(x1, y1, hotspot_x, hotspot_y, rotation_angle);
			x1 = transformed_point.x();
			y1 = transformed_point.y();
			for (int i = 1; i < polygon -> size(); ++i ) {
				qreal x2 = (elem_pos_x + polygon -> at(i).x()) * Createdxf::xScale;
				qreal y2 = Createdxf::sheetHeight - (elem_pos_y + polygon -> at(i).y()) * Createdxf::yScale;
				QPointF transformed_point = rotation_transformed(x2, y2, hotspot_x, hotspot_y, rotation_angle);
				x2 = transformed_point.x();
				y2 = transformed_point.y();
				Createdxf::drawLine(file_path, x1, y1, x2, y2, 0);
				x1 = x2;
				y1 = y2;
			}
		}

		// Draw arcs and ellipses
		QList<QVector<qreal> *> elmt_arc = elmt -> arcs();
		foreach(QVector<qreal> *arc, elmt_arc) {
			if (arc -> size() == 0)
				continue;
			qreal x = (elem_pos_x + arc -> at(0)) * Createdxf::xScale;
			qreal y = Createdxf::sheetHeight - (elem_pos_y + arc -> at(1)) * Createdxf::yScale;
			qreal w = arc -> at(2) * Createdxf::xScale;
			qreal h = arc -> at(3) * Createdxf::yScale;
			qreal startAngle = arc -> at(4);
			qreal spanAngle = arc -> at(5);
			drawDxfArcEllipse(file_path, x, y, w, h, startAngle, spanAngle, hotspot_x, hotspot_y, rotation_angle);
			/*
			// approximate this to center_x, center_y, radius, start angle and end angle.
			qreal center_x = x + w/2;
			qreal center_y = y - w/2;
			qreal radius = (w+h)/4;
			qreal endAngle = startAngle + spanAngle;
			QPointF transformed_point = rotation_transformed(center_x, center_y, hotspot_x, hotspot_y, rotation_angle);
			center_x = transformed_point.x();
			center_y = transformed_point.y();
			if (startAngle == 0 && spanAngle == 360)
				Createdxf::drawCircle(file_path, radius, center_x, center_y, 0);
			else {
				endAngle += rotation_angle;
				startAngle += rotation_angle;
				Createdxf::drawArc(file_path, center_x, center_y, radius, endAngle, startAngle, 0);
			}*/
		}
	}

	//Draw conductors
	foreach(Conductor *cond, list_conductors) {
		foreach(ConductorSegment *segment, cond -> segmentsList()) {
			qreal x1 = (segment -> firstPoint().x()) * Createdxf::xScale;
			qreal y1 = Createdxf::sheetHeight - (segment -> firstPoint().y() * Createdxf::yScale);
			qreal x2 = (segment -> secondPoint().x()) * Createdxf::xScale;
			qreal y2 = Createdxf::sheetHeight - (segment -> secondPoint().y() * Createdxf::yScale);
			Createdxf::drawLine(file_path, x1, y1, x2, y2, 0);
		}
		//Draw conductor text item
		ConductorTextItem *textItem = cond -> textItem();
		if (textItem) {
			qreal fontSize = textItem -> font().pointSizeF();
			if (fontSize < 0)
				fontSize = textItem -> font().pixelSize();
			fontSize *= Createdxf::yScale;
			qreal x = (textItem -> pos().x()) * Createdxf::xScale;
			qreal y = Createdxf::sheetHeight - (textItem -> pos().y() * Createdxf::yScale) - fontSize;
			QStringList lines = textItem->toPlainText().split('\n');
			foreach (QString line, lines) {
				qreal angle = 360 - (textItem -> rotationAngle());
				if (line.size() > 0 && line != "_" )
					Createdxf::drawText(file_path, line, x, y, fontSize, angle, 0 );

				angle += 1080;
				// coordinates for next line
				if (int(angle) % 360 == 0) // no rotation
					y -= fontSize*1.06;
				else if (int(angle - 180) % 360 == 0) // 180 degrees rotation
					y += fontSize*1.06;
				else if (int(angle - 270) % 360 == 0) // 270 degrees rotation
					x -= fontSize*1.06;
				else // ((angle - 90) % 360 == 0)  90 degrees rotation
					x += fontSize*1.06;
			}

		}
	}

	//Draw text items
	foreach(DiagramTextItem *dti, list_texts) {
		qreal fontSize = dti -> font().pointSizeF();
		if (fontSize < 0)
			fontSize = dti -> font().pixelSize();
		fontSize *= Createdxf::yScale;
		qreal x = (dti -> pos().x()) * Createdxf::xScale;
		qreal y = Createdxf::sheetHeight - (dti -> pos().y() * Createdxf::yScale) - fontSize*1.05;
		QStringList lines = dti -> toPlainText().split('\n');
		foreach (QString line, lines) {
			qreal angle = 360 - (dti -> rotationAngle());
			if (line.size() > 0 && line != "_" )
				Createdxf::drawText(file_path, line, x, y, fontSize, angle, 0);

			angle += 1080;
			// coordinates for next line
			if (int(angle) % 360 == 0) // no rotation
				y -= fontSize*1.06;
			else if (int(angle - 180) % 360 == 0) // 180 degrees rotation
				y += fontSize*1.06;
			else if (int(angle - 270) % 360 == 0) // 270 degrees rotation
				x -= fontSize*1.06;
			else // ((angle - 90) % 360 == 0)  90 degrees rotation
				x += fontSize*1.06;
		}
	}
	Createdxf::dxfEnd(file_path);
}

// approximate the ellipse to parts of circles.
void ExportDialog::drawDxfArcEllipse(QString file_path, qreal x, qreal y, qreal w, qreal h, qreal startAngle,
									 qreal spanAngle, qreal hotspot_x, qreal hotspot_y, qreal rotation_angle) {

	// vector of parts of arc (stored as a pair of startAngle and spanAngle) for each quadrant.
	QVector< QPair<qreal,qreal> > arc_parts_vector;

	if (spanAngle > 0) {
		qreal start = startAngle;
		qreal span;
		int i;
		for ( i = startAngle; i < startAngle+spanAngle; i++ ) {
			int absolute_theta = (i > 0) ? i : -i;
			if (absolute_theta == 0 || absolute_theta == 90 ||
				absolute_theta == 180 || absolute_theta == 270 ||
				absolute_theta == 360) {
				span = i - start;
				QPair<qreal, qreal> newPart(start,span);
				arc_parts_vector.push_back(newPart);
				start = i;
			}
		}
		if (start != i) {
			span = i - start;
			QPair<qreal, qreal> newPart(start,span);
			arc_parts_vector.push_back(newPart);
		}
	} else {
		qreal start = startAngle;
		qreal span;
		int i;
		for ( i = startAngle; i > startAngle+spanAngle; i-- ) {
			int absolute_theta = (i > 0) ? i : -i;
			if (absolute_theta == 0 || absolute_theta == 90 ||
				absolute_theta == 180 || absolute_theta == 270 ||
				absolute_theta == 360) {
				span = i - start;
				QPair<qreal, qreal> newPart(start,span);
				arc_parts_vector.push_back(newPart);
				start = i;
			}
		}
		if (start != i) {
			span = i - start;
			QPair<qreal, qreal> newPart(start,span);
			arc_parts_vector.push_back(newPart);
		}
	}

	for (int i = 0; i < arc_parts_vector.size(); i++) {

		QPair<qreal,qreal> arc = arc_parts_vector[i];
		if (arc.second == 0)
			continue;
		qreal arc_startAngle = arc.first * 3.142/180;
		qreal arc_spanAngle = arc.second * 3.142/180;

		qreal a = w/2;
		qreal b = h/2;

		qreal x1 = x + w/2 + a*cos(arc_startAngle);
		qreal y1 = y - h/2 + b*sin(arc_startAngle);
		qreal x2 = x + w/2 + a*cos(arc_startAngle + arc_spanAngle);
		qreal y2 = y - h/2 + b*sin(arc_startAngle + arc_spanAngle);


		qreal mid_ellipse_x = x + w/2 + a*cos(arc_startAngle + arc_spanAngle/2);
		qreal mid_ellipse_y = y - h/2 + b*sin(arc_startAngle + arc_spanAngle/2);
		qreal mid_line_x = (x1+x2)/2;
		qreal mid_line_y = (y1+y2)/2;

		qreal x3 = (mid_ellipse_x + mid_line_x)/2;
		qreal y3 = (mid_ellipse_y + mid_line_y)/2;

		// find circumcenter of points (x1,y1), (x3,y3) and (x2,y2)
		qreal a1 = 2*x2 - 2*x1;
		qreal b1 = 2*y2 - 2*y1;
		qreal c1 = x1*x1 + y1*y1 - x2*x2 - y2*y2;

		qreal a2 = 2*x3 - 2*x1;
		qreal b2 = 2*y3 - 2*y1;
		qreal c2 = x1*x1 + y1*y1 - x3*x3 - y3*y3;

		qreal center_x = (b1*c2 - b2*c1) / (a1*b2 - a2*b1);
		qreal center_y = (a1*c2 - a2*c1) / (b1*a2 - b2*a1);

		qreal radius = sqrt( (x1-center_x)*(x1-center_x) + (y1-center_y)*(y1-center_y) );

		if ( x1 > center_x && y1 > center_y )
			arc_startAngle = asin( (y1 - center_y) / radius );
		else if ( x1 > center_x && y1 < center_y )
			arc_startAngle = 3.142*2 - asin( (center_y - y1) / radius );
		else if ( x1 < center_x && y1 < center_y )
			arc_startAngle = 3.142 + asin( (center_y - y1) / radius );
		else
			arc_startAngle = 3.142 - asin( (y1 - center_y) / radius );

		qreal arc_endAngle;

		if ( x2 > center_x && y2 > center_y )
			arc_endAngle = asin( (y2 - center_y) / radius );
		else if ( x2 > center_x && y2 < center_y )
			arc_endAngle = 3.142*2 - asin( (center_y - y2) / radius );
		else if ( x2 < center_x && y2 < center_y )
			arc_endAngle = 3.142 + asin( (center_y - y2) / radius );
		else
			arc_endAngle = 3.142 - asin( (y2 - center_y) / radius );

		if (arc_endAngle < arc_startAngle) {
			qreal temp = arc_startAngle;
			arc_startAngle = arc_endAngle;
			arc_endAngle = temp;
		}

		QPointF transformed_point = rotation_transformed(center_x, center_y, hotspot_x, hotspot_y, rotation_angle);
		center_x = transformed_point.x();
		center_y = transformed_point.y();
		arc_endAngle *= 180/3.142;
		arc_startAngle *= 180/3.142;
		arc_endAngle -= rotation_angle;
		arc_startAngle -= rotation_angle;

		Createdxf::drawArc(file_path, center_x, center_y, radius, arc_startAngle, arc_endAngle, 0);
	}
}

QPointF ExportDialog::rotation_transformed(qreal px, qreal py , qreal origin_x, qreal origin_y, qreal angle) {

	angle *= -3.14159265 / 180;

	float s = sin(angle);
	float c = cos(angle);

	// Vector to rotate:
	qreal Vx = px - origin_x;
	qreal Vy = py - origin_y;

	// rotate vector
	float xnew = Vx * c - Vy * s;
	float ynew = Vx * s + Vy * c;

	return QPointF(xnew + origin_x, ynew + origin_y);
}



/**
	Slot effectuant les exports apres la validation du dialogue.
*/
void ExportDialog::slot_export() {
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
		QET::MessageBox::warning(
			this,
			tr("Noms des fichiers cibles", "message box title"),
			tr(
				"Vous devez entrer un nom de fichier non vide et unique pour chaque "
				"sch\351ma \340 exporter.",
				"message box content"
			)
		);
		return;
	}
	
	// verification #2 : un chemin vers un dossier doit avoir ete specifie
	
	QDir target_dir_path(epw -> exportProperties().destination_directory);
	if (!target_dir_path.exists()) {
		QET::MessageBox::warning(
			this,
			tr("Dossier non sp\351cifi\351", "message box title"),
			tr("Vous devez sp\351cifier le chemin du dossier dans lequel seront enregistr\351s les fichiers images.", "message box content"),
			QMessageBox::Ok
		);
		return;
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
		QET::MessageBox::critical(
			this,
			tr("Impossible d'\351crire dans ce fichier", "message box title"),
			QString(
				tr(
					"Il semblerait que vous n'ayez pas les permissions "
					"n\351cessaires pour \351crire dans le fichier %1.",
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
		generateDxf(
			diagram_line -> diagram,
			diagram_line -> width  -> value(),
			diagram_line -> height -> value(),
			diagram_line -> keep_ratio -> isChecked(),
			diagram_path
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
void ExportDialog::slot_changeUseBorder() {
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
void ExportDialog::slot_checkDiagramsCount() {
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
	preview_dialog.setWindowTitle(tr("Aper\347u"));
	preview_dialog.setWindowState(preview_dialog.windowState() | Qt::WindowMaximized);
	
	QGraphicsScene *preview_scene = new QGraphicsScene();
	preview_scene -> setBackgroundBrush(Qt::lightGray);
	QGraphicsView *preview_view = new QGraphicsView(preview_scene);
	preview_view -> setDragMode(QGraphicsView::ScrollHandDrag);
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
	connect(buttons, SIGNAL(accepted()), &preview_dialog, SLOT(accept()));
	
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
	if (diagram_title.isEmpty()) diagram_title = QObject::tr("Sch\351ma sans titre");
	QString diagram_filename = diagram -> title();
	if (diagram_filename.isEmpty()) diagram_filename = QObject::tr("schema");
	diagram_filename = QET::stringToFileName(diagram_filename);
	
	title_label = new QLabel(diagram_title);
	
	file_name = new QLineEdit();
	file_name -> setText(diagram_filename);
	file_name -> setMinimumWidth(180);
	
	width = new QSpinBox();
	width -> setRange(1, 10000);
	width -> setSuffix(tr("px"));
	width -> setValue(diagram_size.width());
	
	height = new QSpinBox();
	height -> setRange(1, 10000);
	height -> setSuffix(tr("px"));
	height -> setValue(diagram_size.height());
	
	x_label = new QLabel("\327");
	
	keep_ratio = new QPushButton();
	keep_ratio -> setCheckable(true);
	keep_ratio -> setChecked(true);
	keep_ratio -> setIcon(QET::Icons::ObjectLocked);
	keep_ratio -> setToolTip(QObject::tr("Conserver les proportions"));
	
	reset_size = new QPushButton();
	reset_size -> setIcon(QET::Icons::Start);
	reset_size -> setToolTip(QObject::tr("R\351initialiser les dimensions"));
	
	preview = new QPushButton();
	preview -> setIcon(QET::Icons::ZoomOriginal);
	preview -> setToolTip(QObject::tr("Aper\347u"));
	
	clipboard = new QPushButton();
	clipboard -> setIcon(QET::Icons::CopyFile);
	clipboard -> setToolTip(QObject::tr("Exporter vers le presse-papier"));
}

/**
	Destructeur
*/
ExportDialog::ExportDiagramLine::~ExportDiagramLine() {
}

/**
	@return un layout contenant les widgets necessaires a la gestion de la
	taille d'un schema avant son export.
*/
QBoxLayout *ExportDialog::ExportDiagramLine::sizeLayout() {
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
