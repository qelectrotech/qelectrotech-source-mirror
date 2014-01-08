/*
	Copyright 2006-2013 The QElectroTech Team
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
#include <QPainter>
#include "titleblocktemplate.h"
#include "titleblocktemplaterenderer.h"
#include "bordertitleblock.h"
#include "diagramposition.h"
#include "qetapp.h"
#include "math.h"
#include "createdxf.h"

/**
	Constructeur simple : construit une bordure en recuperant les dimensions
	dans la configuration de l'application.
	@param parent QObject parent de ce BorderTitleBlock
*/
BorderTitleBlock::BorderTitleBlock(QObject *parent) :
	QObject(parent)
{
	// at first, the internal titleblock template renderer uses the default titleblock template
	titleblock_template_renderer_ = new TitleBlockTemplateRenderer(this);
	titleblock_template_renderer_ -> setTitleBlockTemplate(QETApp::defaultTitleBlockTemplate());
	
	// disable the QPicture-based cache from Qt 4.8 to avoid rendering errors and crashes
	if (!QRegExp("4\\.[0-7]\\.").exactMatch(qVersion())) {
		titleblock_template_renderer_ -> setUseCache(false);
	}
	
	// dimensions par defaut du schema
	importBorder(BorderProperties());
	
	// contenu par defaut du cartouche
	importTitleBlock(TitleBlockProperties());
	
	display_titleblock_         = true;
	display_border_        = true;
	setFolioData(1, 1);
	updateRectangles();
}

/**
	Destructeur - ne fait rien
*/
BorderTitleBlock::~BorderTitleBlock() {
}

/**
	@return la hauteur du cartouche
*/
qreal BorderTitleBlock::titleBlockHeight() const {
	return(titleblock_template_renderer_ -> height());
}

/**
	@return Le nombre minimum de colonnes qu'un schema doit comporter
*/
int BorderTitleBlock::minNbColumns() {
	return(3);
}

/**
	@return la largeur minimale d'une colonne de schema
*/
qreal BorderTitleBlock::minColumnsWidth() {
	return(5.0);
}

/**
	@return Le nombre minimum de lignes qu'un schema doit comporter
*/
int BorderTitleBlock::minNbRows() {
	return(2);
}

/**
	@return la hauteur minimale d'une ligne de schema
*/
qreal BorderTitleBlock::minRowsHeight() {
	return(5.0);
}

/**
	Exports the title block current values to XML.
	@param xml_elmt the XML element attributes will be added to
*/
void BorderTitleBlock::titleBlockToXml(QDomElement &xml_elmt) {
	exportTitleBlock().toXml(xml_elmt);
}

/**
	Reads the title block values from XML.
	@param xml_elmt the XML element values will be read from
*/
void BorderTitleBlock::titleBlockFromXml(const QDomElement &xml_elmt) {
	TitleBlockProperties tbp;
	tbp.fromXml(xml_elmt);
	importTitleBlock(tbp);
}

/**
	Exports the border current settings to XML.
	@param xml_elmt the XML element attributes will be added to
*/
void BorderTitleBlock::borderToXml(QDomElement &xml_elmt) {
	xml_elmt.setAttribute("cols",        columnsCount());
	xml_elmt.setAttribute("colsize",     QString("%1").arg(columnsWidth()));
	xml_elmt.setAttribute("displaycols", columnsAreDisplayed() ? "true" : "false");
	
	xml_elmt.setAttribute("rows",        rowsCount());
	xml_elmt.setAttribute("rowsize",     QString("%1").arg(rowsHeight()));
	xml_elmt.setAttribute("displayrows", rowsAreDisplayed() ? "true" : "false");
	
	// attribut datant de la version 0.1 - laisse pour retrocompatibilite
	xml_elmt.setAttribute("height", QString("%1").arg(diagramHeight()));
}

/**
	Reads the border settings from XML.
	@param xml_elmt the XML element values will be read from
*/
void BorderTitleBlock::borderFromXml(const QDomElement &xml_elmt) {
	bool ok;
	// columns count
	int cols_count = xml_elmt.attribute("cols").toInt(&ok);
	if (ok) setColumnsCount(cols_count);
	
	// columns width
	double cols_width = xml_elmt.attribute("colsize").toDouble(&ok);
	if (ok) setColumnsWidth(cols_width);
	
	// backward compatibility: diagrams saved with 0.1 version have a "height" attribute
	if (xml_elmt.hasAttribute("rows") && xml_elmt.hasAttribute("rowsize")) {
		// rows counts
		int rows_count = xml_elmt.attribute("rows").toInt(&ok);
		if (ok) setRowsCount(rows_count);
		
		// taille des lignes
		double rows_size = xml_elmt.attribute("rowsize").toDouble(&ok);
		if (ok) setRowsHeight(rows_size);
	} else {
		// hauteur du schema
		double height = xml_elmt.attribute("height").toDouble(&ok);
		if (ok) setDiagramHeight(height);
	}
	
	// rows and columns display
	displayColumns(xml_elmt.attribute("displaycols") != "false");
	displayRows(xml_elmt.attribute("displayrows") != "false");
	
	adjustTitleBlockToColumns();
}

/**
	@return les proprietes du cartouches
*/
TitleBlockProperties BorderTitleBlock::exportTitleBlock() {
	TitleBlockProperties ip;
	
	ip.author = author();
	ip.date = date();
	ip.title = title();
	ip.filename = fileName();
	ip.folio = folio();
	ip.template_name = titleBlockTemplateName();
	ip.context = additional_fields_;
	
	return(ip);
}

/**
	@param ip les nouvelles proprietes du cartouche
*/
void BorderTitleBlock::importTitleBlock(const TitleBlockProperties &ip) {
	setAuthor(ip.author);
	setDate(ip.date);
	setTitle(ip.title);
	setFileName(ip.filename);
	setFolio(ip.folio);
	additional_fields_ = ip.context;
	
	emit(needFolioData()); // Note: we expect additional data to be provided
	// through setFolioData(), which in turn calls updateDiagramContextForTitleBlock().
	emit(needTitleBlockTemplate(ip.template_name));
}

/**
	@return les proprietes de la bordure
*/
BorderProperties BorderTitleBlock::exportBorder() {
	BorderProperties bp;
	bp.columns_count = columnsCount();
	bp.columns_width = columnsWidth();
	bp.columns_header_height = columnsHeaderHeight();
	bp.display_columns = columnsAreDisplayed();
	bp.rows_count = rowsCount();
	bp.rows_height = rowsHeight();
	bp.rows_header_width = rowsHeaderWidth();
	bp.display_rows = rowsAreDisplayed();
	return(bp);
}

/**
	@param bp les nouvelles proprietes de la bordure
*/
void BorderTitleBlock::importBorder(const BorderProperties &bp) {
	setColumnsHeaderHeight(bp.columns_header_height);
	setColumnsCount(bp.columns_count);
	setColumnsWidth(bp.columns_width);
	displayColumns(bp.display_columns);
	setRowsHeaderWidth(bp.rows_header_width);
	setRowsCount(bp.rows_count);
	setRowsHeight(bp.rows_height);
	displayRows(bp.display_rows);
}

/**
	@return the titleblock template used to render the titleblock
	@see TitleBlockTemplateRenderer::titleBlockTemplate()
*/
const TitleBlockTemplate *BorderTitleBlock::titleBlockTemplate() {
	return(titleblock_template_renderer_ -> titleBlockTemplate());
}

/**
	@param titleblock_template The new titleblock template to use to render the titleblock
	@see TitleBlockTemplateRenderer::setTitleBlockTemplate()
*/
void BorderTitleBlock::setTitleBlockTemplate(const TitleBlockTemplate *titleblock_template) {
	titleblock_template_renderer_ -> setTitleBlockTemplate(titleblock_template);
}

/**
	@return The name of the template used to render the titleblock.
*/
QString BorderTitleBlock::titleBlockTemplateName() const {
	QString tbt_name = titleblock_template_renderer_ -> titleBlockTemplate() -> name();
	return((tbt_name == "default") ? "" : tbt_name);
}

/**
	This slot may be used to inform this class that the given title block
	template has changed. The title block-dedicated rendering cache will thus be
	flushed.
	@param template_name Name of the title block template that has changed
*/
void BorderTitleBlock::titleBlockTemplateChanged(const QString &template_name) {
	if (titleBlockTemplateName() != template_name) return;
	titleblock_template_renderer_ -> invalidateRenderedTemplate();
}

/**
	This slot has to be used to inform this class that the given title block
	template is about to be removed and is no longer accessible. This class
	will either use the provided optional TitleBlockTemplate or the default
	title block provided by QETApp::defaultTitleBlockTemplate()
	@param template_name Name of the title block template that has changed
	@param new_template (Optional) title block template to use instead
*/
void BorderTitleBlock::titleBlockTemplateRemoved(const QString &removed_template_name, const TitleBlockTemplate *new_template) {
	if (titleBlockTemplateName() != removed_template_name) return;
	
	if (new_template) {
		setTitleBlockTemplate(new_template);
	} else {
		setTitleBlockTemplate(QETApp::defaultTitleBlockTemplate());
	}
}

/**
	@param di true pour afficher le cartouche, false sinon
*/
void BorderTitleBlock::displayTitleBlock(bool di) {
	bool change = (di != display_titleblock_);
	display_titleblock_ = di;
	if (change) emit(displayChanged());
}

/**
	@param dc true pour afficher les entetes des colonnes, false sinon
*/
void BorderTitleBlock::displayColumns(bool dc) {
	bool change = (dc != display_columns_);
	display_columns_ = dc;
	if (change) emit(displayChanged());
}

/**
	@param dr true pour afficher les entetes des lignes, false sinon
*/
void BorderTitleBlock::displayRows(bool dr) {
	bool change = (dr != display_rows_);
	display_rows_ = dr;
	if (change) emit(displayChanged());
}

/**
	@param db true pour afficher la bordure du schema, false sinon
	Note : si l'affichage de la bordure est ainsi desactivee, les lignes et
	colonnes ne seront pas dessinees.
*/
void BorderTitleBlock::displayBorder(bool db) {
	bool change = (db != display_border_);
	display_border_  = db;
	if (change) emit(displayChanged());
}

/**
	Methode recalculant les rectangles composant le cadre et le cartouche en
	fonction des attributs de taille
*/
void BorderTitleBlock::updateRectangles() {
	// rectangle delimitant le schema
	QRectF previous_diagram = diagram_rect_;
	diagram_rect_ = QRectF(0, 0, diagramWidth(), diagramHeight());
	if (diagram_rect_ != previous_diagram) emit(borderChanged(previous_diagram, diagram_rect_));
	
	// rectangles relatifs au cartouche
	titleblock_rect_ = QRectF(diagram_rect_.bottomLeft().x(), diagram_rect_.bottomLeft().y(), titleBlockWidth(), titleBlockHeight());
}

/**
	Dessine le cadre et le cartouche
	@param qp QPainter a utiliser pour dessiner le cadre et le cartouche
	@param x  Abscisse du cadre
	@param y  Ordonnee du cadre
*/
void BorderTitleBlock::draw(QPainter *qp, qreal x, qreal y) {
	// translate tous les rectangles
	diagram_rect_     .translate(x, y);
	titleblock_rect_       .translate(x, y);
	
	// prepare le QPainter
	qp -> save();
	qp -> setPen(Qt::black);
	qp -> setBrush(Qt::NoBrush);
	
	// dessine le cadre
	if (display_border_) qp -> drawRect(diagram_rect_);
	
	qp -> setFont(QETApp::diagramTextsFont());
	
	// dessine la case vide qui apparait des qu'il y a un entete
	if (display_border_ && (display_columns_ || display_rows_)) {
		qp -> setBrush(Qt::white);
		QRectF first_rectangle(
			diagram_rect_.topLeft().x(),
			diagram_rect_.topLeft().y(),
			rows_header_width_,
			columns_header_height_
		);
		qp -> drawRect(first_rectangle);
	}
	
	// dessine la numerotation des colonnes
	if (display_border_ && display_columns_) {
		for (int i = 1 ; i <= columns_count_ ; ++ i) {
			QRectF numbered_rectangle = QRectF(
				diagram_rect_.topLeft().x() + (rows_header_width_ + ((i - 1) * columns_width_)),
				diagram_rect_.topLeft().y(),
				columns_width_,
				columns_header_height_
			);
			qp -> drawRect(numbered_rectangle);
			qp -> drawText(numbered_rectangle, Qt::AlignVCenter | Qt::AlignCenter, QString("%1").arg(i));
		}
	}
	
	// dessine la numerotation des lignes
	if (display_border_ && display_rows_) {
		QString row_string("A");
		for (int i = 1 ; i <= rows_count_ ; ++ i) {
			QRectF lettered_rectangle = QRectF(
				diagram_rect_.topLeft().x(),
				diagram_rect_.topLeft().y() + (columns_header_height_ + ((i - 1) * rows_height_)),
				rows_header_width_,
				rows_height_
			);
			qp -> drawRect(lettered_rectangle);
			qp -> drawText(lettered_rectangle, Qt::AlignVCenter | Qt::AlignCenter, row_string);
			row_string = incrementLetters(row_string);
		}
	}
	
	// render the titleblock, using the TitleBlockTemplate object
	if (display_titleblock_) {
		qp -> translate(titleblock_rect_.topLeft());
		titleblock_template_renderer_ -> render(qp, titleblock_rect_.width());
		qp -> translate(-titleblock_rect_.topLeft());
	}
	
	qp -> restore();
	
	// annule la translation des rectangles
	diagram_rect_     .translate(-x, -y);
	titleblock_rect_       .translate(-x, -y);
}

void BorderTitleBlock::drawDxf(int width, int height, bool keep_aspect_ratio, QString &file_path, int color) {

	// Transform to DXF scale.
	columns_header_height_ *= Createdxf::yScale;
	rows_height_           *= Createdxf::yScale;
	rows_header_width_     *= Createdxf::xScale;
	columns_width_         *= Createdxf::xScale;

	// dessine la case vide qui apparait des qu'il y a un entete
	if (display_border_ &&
		(display_columns_ ||
		 display_rows_)
		) {
		Createdxf::drawRectangle(
			file_path,
			double(diagram_rect_.topLeft().x()) * Createdxf::xScale,
			Createdxf::sheetHeight - double(diagram_rect_.topLeft().y()) * Createdxf::yScale - columns_header_height_,
			rows_header_width_,
			columns_header_height_,
			color
		);
	}

	// dessine la numerotation des colonnes
	if (display_border_ &&
		display_columns_) {
		for (int i = 1 ; i <= columns_count_ ; ++ i) {
			double xCoord = diagram_rect_.topLeft().x() +
					(rows_header_width_ + ((i - 1) *
					 columns_width_));
			double yCoord = Createdxf::sheetHeight - diagram_rect_.topLeft().y() - columns_header_height_;
			double recWidth = columns_width_;
			double recHeight = columns_header_height_;
			Createdxf::drawRectangle(file_path, xCoord, yCoord, recWidth, recHeight, color);
			Createdxf::drawTextAligned(file_path, QString::number(i), xCoord,
									   yCoord + recHeight*0.5, recHeight*0.7, 0, 0, 1, 2, xCoord+recWidth/2, color, 0);
		}
	}

	// dessine la numerotation des lignes

	if (display_border_ && display_rows_) {
		QString row_string("A");
		for (int i = 1 ; i <= rows_count_ ; ++ i) {
			double xCoord = diagram_rect_.topLeft().x() * Createdxf::xScale;
			double yCoord = Createdxf::sheetHeight - diagram_rect_.topLeft().y()*Createdxf::yScale
							- (columns_header_height_ + ((i - 1) * rows_height_))
							- rows_height_;
			double recWidth = rows_header_width_;
			double recHeight = rows_height_;
			Createdxf::drawRectangle(file_path, xCoord, yCoord, recWidth, recHeight, color);
			Createdxf::drawTextAligned(file_path, row_string, xCoord,
									   yCoord + recHeight*0.5, recWidth*0.7, 0, 0, 1, 2, xCoord+recWidth/2, color, 0);
			row_string = incrementLetters(row_string);
		}
	}

	// render the titleblock, using the TitleBlockTemplate object
	if (display_titleblock_) {
		//qp -> translate(titleblock_rect_.topLeft());
		titleblock_template_renderer_ -> renderDxf(titleblock_rect_, titleblock_rect_.width(), file_path, color);
		//qp -> translate(-titleblock_rect_.topLeft());
	}

	// Transform back to QET scale
	columns_header_height_ /= Createdxf::yScale;
	rows_height_		   /= Createdxf::yScale;
	rows_header_width_     /= Createdxf::xScale;
	columns_width_         /= Createdxf::xScale;

}



/**
	Ajoute une colonne.
*/
void BorderTitleBlock::addColumn() {
	setColumnsCount(columnsCount() + 1);
}

/**
	Enleve une colonne sans passer sous le minimum requis.
	@see minNbColumns()
*/
void BorderTitleBlock::removeColumn() {
	setColumnsCount(columnsCount() - 1);
}

/**
	Ajoute une ligne.
*/
void BorderTitleBlock::addRow() {
	setRowsCount(rowsCount() + 1);
}

/**
	Enleve une ligne sans passer sous le minimum requis.
	@see minNbRows()
*/
void BorderTitleBlock::removeRow() {
	setRowsCount(rowsCount() - 1);
}

/**
	Permet de changer le nombre de colonnes.
	Si ce nombre de colonnes est inferieur au minimum requis, c'est ce minimum
	qui est utilise.
	@param nb_c nouveau nombre de colonnes
	@see minNbColumns()
*/
void BorderTitleBlock::setColumnsCount(int nb_c) {
	if (nb_c == columnsCount()) return;
	columns_count_ = qMax(minNbColumns(), nb_c);
	setTitleBlockWidth(diagramWidth());
}

/**
	Change la largeur des colonnes.
	Si la largeur indiquee est inferieure au minimum requis, c'est ce minimum
	qui est utilise.
	@param new_cw nouvelle largeur des colonnes
	@see minColumnsWidth()
*/
void BorderTitleBlock::setColumnsWidth(const qreal &new_cw) {
	if (new_cw == columnsWidth()) return;
	columns_width_ = qMax(minColumnsWidth(), new_cw);
	setTitleBlockWidth(diagramWidth());
}

/**
	Change la hauteur des en-tetes contenant les numeros de colonnes. Celle-ci
	doit rester comprise entre 5 et 50 px.
	@param new_chh nouvelle hauteur des en-tetes de colonnes
*/
void BorderTitleBlock::setColumnsHeaderHeight(const qreal &new_chh) {
	columns_header_height_ = qBound(qreal(5.0), new_chh, qreal(50.0));
	updateRectangles();
}

/**
	Permet de changer le nombre de lignes.
	Si ce nombre de lignes est inferieur au minimum requis, cette fonction ne
	fait rien
	@param nb_r nouveau nombre de lignes
	@see minNbRows()
*/
void BorderTitleBlock::setRowsCount(int nb_r) {
	if (nb_r == rowsCount()) return;
	rows_count_ = qMax(minNbRows(), nb_r);
	setTitleBlockWidth(diagramWidth());
	updateRectangles();
}

/**
	Change la hauteur des lignes.
	Si la hauteur indiquee est inferieure au minimum requis, c'est ce minimum
	qui est utilise.
	@param new_rh nouvelle hauteur des lignes
	@see minRowsHeight()
*/
void BorderTitleBlock::setRowsHeight(const qreal &new_rh) {
	if (new_rh == rowsHeight()) return;
	rows_height_ = qMax(minRowsHeight(), new_rh);
	updateRectangles();
}

/**
	Change la largeur des en-tetes contenant les numeros de lignes. Celle-ci
	doit rester comprise entre 5 et 50 px.
	@param new_rhw nouvelle largeur des en-tetes des lignes
*/
void BorderTitleBlock::setRowsHeaderWidth(const qreal &new_rhw) {
	rows_header_width_ = qBound(qreal(5.0), new_rhw, qreal(50.0));
	updateRectangles();
}

/**
	Cette methode essaye de se rapprocher le plus possible de la hauteur donnee
	en parametre en modifiant le nombre de lignes en cours.
*/
void BorderTitleBlock::setDiagramHeight(const qreal &height) {
	// taille des lignes a utiliser = rows_height
	setRowsCount(qRound(ceil(height / rows_height_)));
}

/**
	Change la largeur du cartouche. Cette largeur sera restreinte a celle du
	schema.
*/
void BorderTitleBlock::setTitleBlockWidth(const qreal &new_iw) {
	titleblock_width_ = qMin(diagramWidth(), new_iw);
	updateRectangles();
}


/**
	Ajuste la largeur du cartouche de facon a ce que celui-ci soit aussi large
	que le schema
*/
void BorderTitleBlock::adjustTitleBlockToColumns() {
	setTitleBlockWidth(diagramWidth());
}

/**
	@param pos Position cartesienne (ex : 10.3, 45.2) a transformer en position
	dans la grille (ex : B2)
	@return la position dans la grille correspondant a pos
*/
DiagramPosition BorderTitleBlock::convertPosition(const QPointF &pos) {
	// recupere le rectangle quadrille par les en-tetes
	QRectF grid_rect(
		rowsHeaderWidth(),
		columnsHeaderHeight(),
		diagramWidth(),
		diagramHeight()
	);
	
	if (!grid_rect.contains(pos)) {
		return(DiagramPosition("", 0));
	}
	
	QPointF relative_pos = pos - grid_rect.topLeft();
	int row_number    = int(ceil(relative_pos.x() / columnsWidth()));
	int column_number = int(ceil(relative_pos.y() / rowsHeight()));
	
	QString letter = "A";
	for (int i = 1 ; i < column_number ; ++ i) {
		letter = incrementLetters(letter);
	}
	
	return(DiagramPosition(letter, row_number));
}

/**
	Update the informations given to the titleblock template by regenerating a
	DiagramContext object.
	@param initial_context Base diagram context that will be overridden by
	diagram-wide values
*/
void BorderTitleBlock::updateDiagramContextForTitleBlock(const DiagramContext &initial_context) {
	// Our final DiagramContext is the initial one (which is supposed to bring
	// project-wide properties), overridden by the "additional fields" one...
	DiagramContext context = initial_context;
	foreach (QString key, additional_fields_.keys()) {
		context.addValue(key, additional_fields_[key]);
	}
	
	// ... overridden by the historical and/or dynamically generated fields
	context.addValue("author",      btb_author_);
	context.addValue("date",        btb_date_.toString("dd/MM/yyyy"));
	context.addValue("title",       btb_title_);
	context.addValue("filename",    btb_filename_);
	context.addValue("folio",       btb_final_folio_);
	context.addValue("folio-id",    folio_index_);
	context.addValue("folio-total", folio_total_);
	
	titleblock_template_renderer_ -> setContext(context);
}

QString BorderTitleBlock::incrementLetters(const QString &string) {
	if (string.isEmpty()) {
		return("A");
	} else {
		// separe les digits precedents du dernier digit
		QString first_digits(string.left(string.count() - 1));
		QChar last_digit(string.at(string.count() - 1));
		if (last_digit != 'Z') {
			// incremente le dernier digit
			last_digit = last_digit.toAscii() + 1;
			return(first_digits + QString(last_digit));
		} else {
			return(incrementLetters(first_digits) + "A");
		}
	}
}

/**
	@param index numero du schema (de 1 a total)
	@param total nombre total de schemas dans le projet
	@param project_properties Project-wide properties, to be merged with diagram-wide ones.
*/
void BorderTitleBlock::setFolioData(int index, int total, const DiagramContext &project_properties) {
	if (index < 1 || total < 1 || index > total) return;
	
	// memorise les informations
	folio_index_ = index;
	folio_total_ = total;
	
	// regenere le contenu du champ folio
	btb_final_folio_ = btb_folio_;
	btb_final_folio_.replace("%id",    QString::number(folio_index_));
	btb_final_folio_.replace("%total", QString::number(folio_total_));
	
	updateDiagramContextForTitleBlock(project_properties);
}
