/*
	Copyright 2006-2007 Xavier Guerrin
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
#include "borderinset.h"
#include "qetapp.h"

/**
	Constructeur simple : construit une bordure de 15 colonnes de 50x500 avec
	un cartouche de 400x50.
	@param parent QObject parent de ce BorderInset
*/
BorderInset::BorderInset(QObject *parent) : QObject(parent) {
	nb_columns            = qMax(3, QETApp::settings().value("diagrameditor/defaultcols", 15).toInt());
	min_nb_columns        = 3;
	columns_width         = QETApp::settings().value("diagrameditor/defaultcolsize", 50.0).toDouble();
	columns_height        = QETApp::settings().value("diagrameditor/defaultheight", 500.0).toDouble();
	min_columns_height    = 80.0;
	inset_width           = nb_columns * columns_width;
	inset_height          = 50.0;
	columns_header_height = 20.0;
	display_inset         = true;
	display_columns       = true;
	display_border        = true;
	updateRectangles();
	
	bi_author   = QETApp::settings().value("diagrameditor/defaultauthor").toString();
	bi_title    = QETApp::settings().value("diagrameditor/defaulttitle").toString();
	bi_folio    = QETApp::settings().value("diagrameditor/defaultfolio").toString();
	bi_filename = QETApp::settings().value("diagrameditor/defaultfilename").toString();
	QString settings_date = QETApp::settings().value("diagrameditor/defaultdate").toString();
	if (settings_date == "now") bi_date = QDate::currentDate();
	else if (settings_date.isEmpty() || settings_date == "null") bi_date = QDate();
	else bi_date = QDate::fromString(settings_date, "yyyyMMdd");
}

/**
	Destructeur - ne fait rien
*/
BorderInset::~BorderInset() {
}

/**
	Methode recalculant les rectangles composant le cadre et le cartouche en
	fonction des attributs de taille
*/
void BorderInset::updateRectangles() {
	// rectangle delimitant le schema
	QRectF previous_border = border;
	border = QRectF(0, 0, nb_columns * columns_width, columns_height);
	if (border != previous_border) emit(borderChanged(previous_border, border));
	
	// rectangles relatifs au cartouche
	inset        = QRectF(border.bottomLeft().x(), border.bottomLeft().y(), inset_width, inset_height);
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
	border      .translate(x, y);
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
	if (display_border) qp -> drawRect(border);
	
	// dessine la numerotation des colonnes
	if (display_columns) {
		qp -> setBrush(Qt::white);
		for (int i = 1 ; i <= nb_columns ; ++ i) {
			QRectF numbered_rectangle = QRectF(
				border.topLeft().x() + ((i - 1) * columns_width),
				border.topLeft().y(),
				columns_width,
				columns_header_height
			);
			qp -> drawRect(numbered_rectangle);
			qp -> drawText(numbered_rectangle, Qt::AlignVCenter | Qt::AlignCenter, QString("%1").arg(i));
		}
	}
	
	// dessine le cartouche
	if (display_inset) {
		qp -> setBrush(Qt::white);
		qp -> drawRect(inset);
		
		qp -> drawRect(inset_author);
		qp -> drawText(inset_author, Qt::AlignVCenter | Qt::AlignLeft,   tr(" Auteur : ") + bi_author);
		
		qp -> drawRect(inset_date);
		qp -> drawText(inset_date,   Qt::AlignVCenter | Qt::AlignLeft,   tr(" Date : ") + bi_date.toString("dd/MM/yyyy"));
		
		qp -> drawRect(inset_title);
		qp -> drawText(inset_title,  Qt::AlignVCenter | Qt::AlignCenter, tr("Titre du document : ") + bi_title);
		
		qp -> drawRect(inset_file);
		qp -> drawText(inset_file,   Qt::AlignVCenter | Qt::AlignLeft,   tr(" Fichier : ") + bi_filename);
		
		qp -> drawRect(inset_folio);
		qp -> drawText(inset_folio,  Qt::AlignVCenter | Qt::AlignLeft,   tr(" Folio : ") + bi_folio);
	}
	
	qp -> restore();
	
	// annule la translation des rectangles
	border      .translate(-x, -y);
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
	++ nb_columns;
	setInsetWidth(nb_columns * columns_width);
	updateRectangles();
}

/**
	Enleve une colonne. Il doit rester au moins 3 colonnes.
*/
void BorderInset::removeColumn() {
	if (nb_columns == min_nb_columns) return;
	-- nb_columns;
	setInsetWidth(nb_columns * columns_width);
	updateRectangles();
}

/**
	Permet de changer le nombre de colonnes.
	Si ce nombre de colonnes est inferieur au minimum requis, cette fonction ne
	fait rien
	@param nb_c nouveau nombre de colonnes
	@see minNbColumns()
*/
void BorderInset::setNbColumns(int nb_c) {
	if (nb_c < min_nb_columns) return;
	nb_columns = nb_c;
	setInsetWidth(nb_columns * columns_width);
	updateRectangles();
}

/**
	Change la largeur des colonnes ; celle-ci doit rester comprise entre 10 et
	200px.
*/
void BorderInset::setColumnsWidth(const qreal &new_cw) {
	columns_width = qBound(10.0, new_cw, 200.0);
	updateRectangles();
}

/**
	Change la hauteur des en-tetes contenant les numeros de colonnes. Celle-ci
	doit rester comprise entre 5 et 50 px.
*/
void BorderInset::setColumnsHeaderHeight(const qreal &new_chh) {
	columns_header_height = qBound(5.0, new_chh, 50.0);
	updateRectangles();
}

/**
	Change la hauteur des colonnes (et donc du cadre). Cette hauteur doit
	rester superieure a la hauteur des en-tetes de colonnes + 20px.
*/
void BorderInset::setColumnsHeight(const qreal &new_ch) {
	columns_height = qMax(columns_header_height + min_columns_height, new_ch);
	updateRectangles();
}

/**
	Change la largeur du cartouche. Cette largeur doit rester comprise entre
	100px et la largeur du cartouche
*/
void BorderInset::setInsetWidth(const qreal &new_iw) {
	inset_width = qMax(100.0, qMin(nb_columns * columns_width, new_iw));
	updateRectangles();
}

/**
	Change la hauteur du cartouche. Cette largeur doit rester comprise entre
	20px et la hauteur du cartouche.
*/
void BorderInset::setInsetHeight(const qreal &new_ih) {
	inset_height = qMax(20.0, qMin(columns_height, new_ih));
	updateRectangles();
}

/**
	Ajuste la largeur du cartouche de facon a ce que celui-ci soit aussi large
	que le schema
*/
void BorderInset::adjustInsetToColumns() {
	setInsetWidth(nbColumn() * columnsWidth());
}
