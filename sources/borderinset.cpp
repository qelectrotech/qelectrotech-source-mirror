/*
	Copyright 2006-2010 Xavier Guerrin
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
#include "borderinset.h"
#include "diagramposition.h"
#include "qetapp.h"
#include "math.h"

/**
	Constructeur simple : construit une bordure en recuperant les dimensions
	dans la configuration de l'application.
	@param parent QObject parent de ce BorderInset
*/
BorderInset::BorderInset(QObject *parent) : QObject(parent) {
	// dimensions par defaut du schema
	importBorder(BorderProperties());
	
	// contenu par defaut du cartouche
	importInset(InsetProperties());
	
	// hauteur du cartouche
	inset_height          = 50.0;
	
	display_inset         = true;
	display_border        = true;
	setFolioData(1, 1);
	updateRectangles();
}

/**
	Destructeur - ne fait rien
*/
BorderInset::~BorderInset() {
}

/**
	@return Le nombre minimum de colonnes qu'un schema doit comporter
*/
int BorderInset::minNbColumns() {
	return(3);
}

/**
	@return la largeur minimale d'une colonne de schema
*/
qreal BorderInset::minColumnsWidth() {
	return(5.0);
}

/**
	@return Le nombre minimum de lignes qu'un schema doit comporter
*/
int BorderInset::minNbRows() {
	return(2);
}

/**
	@return la hauteur minimale d'une ligne de schema
*/
qreal BorderInset::minRowsHeight() {
	return(5.0);
}

/**
	@return les proprietes du cartouches
*/
InsetProperties BorderInset::exportInset() {
	InsetProperties ip;
	ip.author = bi_author;
	ip.date = bi_date;
	ip.title = bi_title;
	ip.folio = bi_folio;
	ip.filename = bi_filename;
	return(ip);
}

/**
	@param ip les nouvelles proprietes du cartouche
*/
void BorderInset::importInset(const InsetProperties &ip) {
	bi_author = ip.author;
	bi_date = ip.date;
	setTitle(ip.title);
	bi_folio = ip.folio;
	bi_filename = ip.filename;
	emit(needFolioData());
}

/**
	@return les proprietes de la bordure
*/
BorderProperties BorderInset::exportBorder() {
	BorderProperties bp;
	bp.columns_count = nbColumns();
	bp.columns_width = columnsWidth();
	bp.columns_header_height = columnsHeaderHeight();
	bp.display_columns = columnsAreDisplayed();
	bp.rows_count = nbRows();
	bp.rows_height = rowsHeight();
	bp.rows_header_width = rowsHeaderWidth();
	bp.display_rows = rowsAreDisplayed();
	return(bp);
}

/**
	@param bp les nouvelles proprietes de la bordure
*/
void BorderInset::importBorder(const BorderProperties &bp) {
	setColumnsHeaderHeight(bp.columns_header_height);
	setNbColumns(bp.columns_count);
	setColumnsWidth(bp.columns_width);
	displayColumns(bp.display_columns);
	setRowsHeaderWidth(bp.rows_header_width);
	setNbRows(bp.rows_count);
	setRowsHeight(bp.rows_height);
	displayRows(bp.display_rows);
}

/**
	@param di true pour afficher le cartouche, false sinon
*/
void BorderInset::displayInset(bool di) {
	bool change = (di != display_inset);
	display_inset = di;
	if (change) emit(displayChanged());
}

/**
	@param dc true pour afficher les entetes des colonnes, false sinon
*/
void BorderInset::displayColumns(bool dc) {
	bool change = (dc != display_columns);
	display_columns = dc;
	if (change) emit(displayChanged());
}

/**
	@param dr true pour afficher les entetes des lignes, false sinon
*/
void BorderInset::displayRows(bool dr) {
	bool change = (dr != display_rows);
	display_rows = dr;
	if (change) emit(displayChanged());
}

/**
	@param db true pour afficher la bordure du schema, false sinon
	Note : si l'affichage de la bordure est ainsi desactivee, les lignes et
	colonnes ne seront pas dessinees.
*/
void BorderInset::displayBorder(bool db) {
	bool change = (db != display_border);
	display_border  = db;
	if (change) emit(displayChanged());
}

/**
	Methode recalculant les rectangles composant le cadre et le cartouche en
	fonction des attributs de taille
*/
void BorderInset::updateRectangles() {
	// rectangle delimitant le schema
	QRectF previous_diagram = diagram;
	diagram = QRectF(0, 0, diagramWidth(), diagramHeight());
	if (diagram != previous_diagram) emit(borderChanged(previous_diagram, diagram));
	
	// rectangles relatifs au cartouche
	inset        = QRectF(diagram.bottomLeft().x(), diagram.bottomLeft().y(), inset_width, inset_height);
	inset_author = QRectF(inset.topLeft(), QSizeF(2.0 * inset_width / 9.0, 0.5 * inset_height));
	inset_date   = QRectF(inset_author.bottomLeft(), inset_author.size());
	inset_title  = QRectF(inset_author.topRight(), QSizeF(5.0 * inset_width / 9.0, inset_height));
	inset_file   = QRectF(inset_title.topRight(), inset_author.size());
	inset_folio  = QRectF(inset_file.bottomLeft(), inset_author.size());
}

/**
	Dessine le cadre et le cartouche
	@param qp QPainter a utiliser pour dessiner le cadre et le cartouche
	@param x  Abscisse du cadre
	@param y  Ordonnee du cadre
*/
void BorderInset::draw(QPainter *qp, qreal x, qreal y) {
	// translate tous les rectangles
	diagram     .translate(x, y);
	inset       .translate(x, y);
	inset_author.translate(x, y);
	inset_date  .translate(x, y);
	inset_title .translate(x, y);
	inset_file  .translate(x, y);
	inset_folio .translate(x, y);
	
	// prepare le QPainter
	qp -> save();
	qp -> setPen(Qt::black);
	qp -> setBrush(Qt::NoBrush);
	
	// dessine le cadre
	if (display_border) qp -> drawRect(diagram);
	
	qp -> setFont(QETApp::diagramTextsFont());
	
	// dessine la case vide qui apparait des qu'il y a un entete
	if (display_border && (display_columns || display_rows)) {
		qp -> setBrush(Qt::white);
		QRectF first_rectangle(
			diagram.topLeft().x(),
			diagram.topLeft().y(),
			rows_header_width,
			columns_header_height
		);
		qp -> drawRect(first_rectangle);
	}
	
	// dessine la numerotation des colonnes
	if (display_border && display_columns) {
		for (int i = 1 ; i <= nb_columns ; ++ i) {
			QRectF numbered_rectangle = QRectF(
				diagram.topLeft().x() + (rows_header_width + ((i - 1) * columns_width)),
				diagram.topLeft().y(),
				columns_width,
				columns_header_height
			);
			qp -> drawRect(numbered_rectangle);
			qp -> drawText(numbered_rectangle, Qt::AlignVCenter | Qt::AlignCenter, QString("%1").arg(i));
		}
	}
	
	// dessine la numerotation des lignes
	if (display_border && display_rows) {
		QString row_string("A");
		for (int i = 1 ; i <= nb_rows ; ++ i) {
			QRectF lettered_rectangle = QRectF(
				diagram.topLeft().x(),
				diagram.topLeft().y() + (columns_header_height + ((i - 1) * rows_height)),
				rows_header_width,
				rows_height
			);
			qp -> drawRect(lettered_rectangle);
			qp -> drawText(lettered_rectangle, Qt::AlignVCenter | Qt::AlignCenter, row_string);
			row_string = incrementLetters(row_string);
		}
	}
	
	// dessine le cartouche
	if (display_inset) {
		qp -> setBrush(Qt::white);
		qp -> drawRect(inset);
		
		qp -> drawRect(inset_author);
		qp -> drawText(inset_author, Qt::AlignVCenter | Qt::AlignLeft,   QString(tr(" Auteur : %1", "inset content")).arg(bi_author));
		
		qp -> drawRect(inset_date);
		qp -> drawText(inset_date,   Qt::AlignVCenter | Qt::AlignLeft,   QString(tr(" Date : %1", "inset content")).arg(bi_date.toString("dd/MM/yyyy")));
		
		qp -> drawRect(inset_title);
		qp -> drawText(inset_title,  Qt::AlignVCenter | Qt::AlignCenter, QString("%1").arg(bi_title));
		
		qp -> drawRect(inset_file);
		qp -> drawText(inset_file,   Qt::AlignVCenter | Qt::AlignLeft,   QString(tr(" Fichier : %1", "inset content")).arg(bi_filename));
		
		qp -> drawRect(inset_folio);
		qp -> drawText(inset_folio,  Qt::AlignVCenter | Qt::AlignLeft,   QString(tr(" Folio : %1", "inset content")).arg(bi_final_folio));
	}
	
	qp -> restore();
	
	// annule la translation des rectangles
	diagram     .translate(-x, -y);
	inset       .translate(-x, -y);
	inset_author.translate(-x, -y);
	inset_date  .translate(-x, -y);
	inset_title .translate(-x, -y);
	inset_file  .translate(-x, -y);
	inset_folio .translate(-x, -y);
}

/**
	Ajoute une colonne.
*/
void BorderInset::addColumn() {
	setNbColumns(nbColumns() + 1);
}

/**
	Enleve une colonne sans passer sous le minimum requis.
	@see minNbColumns()
*/
void BorderInset::removeColumn() {
	setNbColumns(nbColumns() - 1);
}

/**
	Ajoute une ligne.
*/
void BorderInset::addRow() {
	setNbRows(nbRows() + 1);
}

/**
	Enleve une ligne sans passer sous le minimum requis.
	@see minNbRows()
*/
void BorderInset::removeRow() {
	setNbRows(nbRows() - 1);
}

/**
	Permet de changer le nombre de colonnes.
	Si ce nombre de colonnes est inferieur au minimum requis, c'est ce minimum
	qui est utilise.
	@param nb_c nouveau nombre de colonnes
	@see minNbColumns()
*/
void BorderInset::setNbColumns(int nb_c) {
	if (nb_c == nbColumns()) return;
	nb_columns = qMax(minNbColumns(), nb_c);
	setInsetWidth(diagramWidth());
}

/**
	Change la largeur des colonnes.
	Si la largeur indiquee est inferieure au minimum requis, c'est ce minimum
	qui est utilise.
	@param new_cw nouvelle largeur des colonnes
	@see minColumnsWidth()
*/
void BorderInset::setColumnsWidth(const qreal &new_cw) {
	if (new_cw == columnsWidth()) return;
	columns_width = qMax(minColumnsWidth(), new_cw);
	setInsetWidth(diagramWidth());
}

/**
	Change la hauteur des en-tetes contenant les numeros de colonnes. Celle-ci
	doit rester comprise entre 5 et 50 px.
	@param new_chh nouvelle hauteur des en-tetes de colonnes
*/
void BorderInset::setColumnsHeaderHeight(const qreal &new_chh) {
	columns_header_height = qBound(qreal(5.0), new_chh, qreal(50.0));
	updateRectangles();
}

/**
	Permet de changer le nombre de lignes.
	Si ce nombre de lignes est inferieur au minimum requis, cette fonction ne
	fait rien
	@param nb_r nouveau nombre de lignes
	@see minNbRows()
*/
void BorderInset::setNbRows(int nb_r) {
	if (nb_r == nbRows()) return;
	nb_rows = qMax(minNbRows(), nb_r);
	setInsetWidth(diagramWidth());
	updateRectangles();
}

/**
	Change la hauteur des lignes.
	Si la hauteur indiquee est inferieure au minimum requis, c'est ce minimum
	qui est utilise.
	@param new_rh nouvelle hauteur des lignes
	@see minRowsHeight()
*/
void BorderInset::setRowsHeight(const qreal &new_rh) {
	if (new_rh == rowsHeight()) return;
	rows_height = qMax(minRowsHeight(), new_rh);
	updateRectangles();
}

/**
	Change la largeur des en-tetes contenant les numeros de lignes. Celle-ci
	doit rester comprise entre 5 et 50 px.
	@param new_rhw nouvelle largeur des en-tetes des lignes
*/
void BorderInset::setRowsHeaderWidth(const qreal &new_rhw) {
	rows_header_width = qBound(qreal(5.0), new_rhw, qreal(50.0));
	updateRectangles();
}

/**
	Cette methode essaye de se rapprocher le plus possible de la hauteur donnee
	en parametre en modifiant le nombre de lignes en cours.
*/
void BorderInset::setDiagramHeight(const qreal &height) {
	// taille des lignes a utiliser = rows_height
	setNbRows(qRound(ceil(height / rows_height)));
}

/**
	Change la largeur du cartouche. Cette largeur sera restreinte a celle du
	schema.
*/
void BorderInset::setInsetWidth(const qreal &new_iw) {
	inset_width = qMin(diagramWidth(), new_iw);
	updateRectangles();
}

/**
	Change la hauteur du cartouche. Cette hauteur doit rester comprise entre
	20px et la hauteur du schema.
*/
void BorderInset::setInsetHeight(const qreal &new_ih) {
	inset_height = qMax(qreal(20.0), qMin(diagramHeight(), new_ih));
	updateRectangles();
}

/**
	Ajuste la largeur du cartouche de facon a ce que celui-ci soit aussi large
	que le schema
*/
void BorderInset::adjustInsetToColumns() {
	setInsetWidth(diagramWidth());
}

/**
	@param pos Position cartesienne (ex : 10.3, 45.2) a transformer en position
	dans la grille (ex : B2)
	@return la position dans la grille correspondant a pos
*/
DiagramPosition BorderInset::convertPosition(const QPointF &pos) {
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

QString BorderInset::incrementLetters(const QString &string) {
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
*/
void BorderInset::setFolioData(int index, int total) {
	if (index < 1 || total < 1 || index > total) return;
	
	// memorise les informations
	folio_index_ = index;
	folio_total_ = total;
	
	// regenere le contenu du champ folio
	bi_final_folio = bi_folio;
	bi_final_folio.replace("%id",    QString::number(folio_index_));
	bi_final_folio.replace("%total", QString::number(folio_total_));
}
