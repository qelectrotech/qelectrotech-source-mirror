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
#include "diagramprintdialog.h"
#include <math.h>

/**
	Constructeur
	@param dia     Schema a imprimer
	@param printer Imprimante a utiliser
	@param parent  Widget parent du dialogue
*/
DiagramPrintDialog::DiagramPrintDialog(Diagram *dia, QWidget *parent) :
	QWidget(parent),
	diagram(dia),
	dialog(0)
{
	// initialise l'imprimante
	printer = new QPrinter();
	
	// orientation paysage par defaut
	printer -> setOrientation(QPrinter::Landscape);
}

/**
	Destructeur
*/
DiagramPrintDialog::~DiagramPrintDialog() {
	delete dialog;
	delete printer;
}

/**
	Definit le nom du PDF si l'utilisateur choisit une sortie vers un PDF
*/
void DiagramPrintDialog::setPDFName(const QString &name) {
	pdf_name = name;
}

/**
	@return le nom du PDF
*/
QString DiagramPrintDialog::PDFName() const {
	return(pdf_name);
}

/**
	Definit le nom du document
*/
void DiagramPrintDialog::setDocName(const QString &name) {
	doc_name = name;
}

/**
	@return le nom du document
*/
QString DiagramPrintDialog::docName() const {
	return(doc_name);
}

/**
	Execute le dialogue d'impression
*/
void DiagramPrintDialog::exec() {
	
	// affichage du dialogue d'impression standard
	QPrintDialog print_dialog(printer, parentWidget());
	print_dialog.setEnabledOptions(QAbstractPrintDialog::PrintToFile | QAbstractPrintDialog::PrintShowPageSize);
#ifndef Q_OS_WIN32
	if (!pdf_name.isEmpty()) printer -> setOutputFileName(pdf_name);
	if (!doc_name.isEmpty()) printer -> setDocName(doc_name);
#endif
	if (print_dialog.exec() == QDialog::Rejected) return;
	
	/*
		Apres l'execution de ce premier dialogue, on connait le format papier a
		utiliser, son orientation et on est sur que tout cela est supporte par
		l'imprimante.
		On peut donc en deduire le nombre de pages a imprimer
	*/
	
	// affichage d'un second dialogue, non standard, pour connaitre les pages a imprimer
	buildDialog();
	if (dialog -> exec() == QDialog::Rejected) return;
	
	// effectue l'impression en elle-meme
	print();
}

/**
	@param fullPage true pour utiliser toute la feuille dans le calcul
	@return Le nombre de pages necessaires pour imprimer le schema
	avec l'orientation et le format papier utilise dans l'imprimante en cours.
*/
int DiagramPrintDialog::pagesCount(bool fullpage) const {
	return(horizontalPagesCount(fullpage) * verticalPagesCount(fullpage));
}

/**
	@param fullPage true pour utiliser toute la feuille dans le calcul
	@return La largeur du "poster" en nombre de pages pour imprimer le schema
	avec l'orientation et le format papier utilise dans l'imprimante en cours.
*/
int DiagramPrintDialog::horizontalPagesCount(bool fullpage) const {
	// note : pageRect et Paper Rect tiennent compte de l'orientation du papier
	QRect printable_area = fullpage ? printer -> paperRect() : printer -> pageRect();
	QRect diagram_rect = diagram -> border().toRect();
	
	int h_pages_count = int(ceil(qreal(diagram_rect.width()) / qreal(printable_area.width())));
	return(h_pages_count);
}

/**
	@param fullPage true pour utiliser toute la feuille dans le calcul
	@return La largeur du "poster" en nombre de pages pour imprimer le schema
	avec l'orientation et le format papier utilise dans l'imprimante en cours.
*/
int DiagramPrintDialog::verticalPagesCount(bool fullpage) const {
	// note : pageRect et Paper Rect tiennent compte de l'orientation du papier
	QRect printable_area = fullpage ? printer -> paperRect() : printer -> pageRect();
	QRect diagram_rect = diagram -> border().toRect();
	
	int v_pages_count = int(ceil(qreal(diagram_rect.height()) / qreal(printable_area.height())));
	return(v_pages_count);
}

/**
	Construit un dialogue non standard pour demander les pages a imprimer a l'utilisateur
*/
void DiagramPrintDialog::buildDialog() {
	dialog = new QDialog(parentWidget());
	dialog -> setWindowTitle(tr("Options d'impression"));
	options_label = new QLabel();
	use_full_page = new QCheckBox(tr("Utiliser toute la feuille"));
	fit_diagram_to_page = new QCheckBox(tr("Adapter le sch\351ma \340 la page"));
	range_from_label = new QLabel(tr("Plage de "));
	start_page = new QSpinBox();
	to_label = new QLabel(tr(" \340 "));
	end_page = new QSpinBox();
	buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	
	QHBoxLayout *pages_layout = new QHBoxLayout();
	pages_layout -> addWidget(range_from_label);
	pages_layout -> addWidget(start_page);
	pages_layout -> addWidget(to_label);
	pages_layout -> addWidget(end_page);
	
	QVBoxLayout *dialog_layout = new QVBoxLayout(dialog);
	dialog_layout -> addWidget(options_label);
	dialog_layout -> addWidget(use_full_page);
	dialog_layout -> addWidget(fit_diagram_to_page);
	dialog_layout -> addLayout(pages_layout);
	dialog_layout -> addStretch();
	dialog_layout -> addWidget(buttons);
	
	connect(use_full_page,       SIGNAL(stateChanged(int)), this,   SLOT(updateDialog()));
	connect(fit_diagram_to_page, SIGNAL(stateChanged(int)), this,   SLOT(updateDialog()));
	connect(start_page,          SIGNAL(valueChanged(int)), this,   SLOT(checkStartPage()));
	connect(end_page,            SIGNAL(valueChanged(int)), this,   SLOT(checkEndPage()));
	connect(buttons,             SIGNAL(accepted()),        dialog, SLOT(accept()));
	connect(buttons,             SIGNAL(rejected()),        dialog, SLOT(reject()));
	
	updateDialog();
}

/**
	Assure la coherence du dialogue
*/
void DiagramPrintDialog::updateDialog() {
	int pages_count;
	// si on adapte le schema a la page, alors il n'y a qu'une page a imprimer
	if (fit_diagram_to_page -> isChecked()) {
		pages_count = 1;
	} else {
		pages_count = pagesCount(use_full_page -> isChecked());
	}
	options_label -> setText(tr("Nombre total de pages : ") + QString("%1").arg(pages_count));
	setPagesRangeVisible(pages_count > 1);
	start_page -> setRange(1, pages_count);
	end_page   -> setRange(1, pages_count);
	end_page   -> setValue(pages_count);
}

/**
	S'assure que la premiere page ne soit pas superieure a la derniere page
*/
void DiagramPrintDialog::checkStartPage() {
	if (start_page -> value() > end_page -> value()) {
		start_page -> blockSignals(true);
		start_page -> setValue(end_page -> value());
		start_page -> blockSignals(false);
	}
}

/**
	S'assure que la derniere page ne soit pas inferieure a la premiere page
*/
void DiagramPrintDialog::checkEndPage() {
	if (end_page -> value() < start_page -> value()) {
		end_page -> blockSignals(true);
		end_page -> setValue(start_page -> value());
		end_page -> blockSignals(false);
	}
}

/**
	@param visible true pour afficher les pages, false sinon
*/
void DiagramPrintDialog::setPagesRangeVisible(bool visible) {
	range_from_label -> setVisible(visible);
	start_page       -> setVisible(visible);
	to_label         -> setVisible(visible);
	end_page         -> setVisible(visible);
}

/**
	Effectue l'impression elle-meme
*/
void DiagramPrintDialog::print() {
	// recupere les informations collectees dans le second dialogue
	bool full_page = use_full_page -> isChecked();
	bool fit_page  = fit_diagram_to_page -> isChecked();
	int first_page = start_page -> value();
	int last_page  = end_page -> value();
	
	// parametre l'imprimante
	printer -> setFullPage(full_page);
	
	// QPainter utiliser pour effectuer le rendu
	QPainter qp(printer);
	
	// impression physique (!= fichier PDF)
	if (printer -> outputFileName().isEmpty()) {
		// utiliser cette condition pour agir differemment en cas d'impression physique
	}
	
	diagram -> setDisplayGrid(false);
	diagram -> setDrawTerminals(false);
	
	if (fit_page) {
		// impression adaptee sur une seule page
		diagram -> render(&qp, QRectF(), diagram -> border(), Qt::KeepAspectRatio);
	} else {
		// impression sur une ou plusieurs pages
		QRect diagram_rect = diagram -> border().toRect();
		QRect printed_area = full_page ? printer -> paperRect() : printer -> pageRect();
		int used_width  = printed_area.width();
		int used_height = printed_area.height();
		int h_pages_count = horizontalPagesCount(full_page);
		int v_pages_count = verticalPagesCount(full_page);
		
		QVector< QVector< QRect > > pages_grid;
		// le schema est imprime sur une matrice de feuilles
		// parcourt les lignes de la matrice
		int y_offset = 0;
		for (int i = 0 ; i < v_pages_count ; ++ i) {
			pages_grid << QVector< QRect >();
			
			// parcourt les feuilles de la ligne
			int x_offset = 0;
			for (int j = 0 ; j < h_pages_count ; ++ j) {
				pages_grid.last() << QRect(
					QPoint(x_offset, y_offset),
					QSize(
						qMin(used_width,  diagram_rect.width()  - x_offset),
						qMin(used_height, diagram_rect.height() - y_offset)
					)
				);
				x_offset += used_width;
			}
			
			y_offset += used_height;
		}
		
		// ne retient que les pages a imprimer
		QVector<QRect> pages_to_print;
		for (int i = 0 ; i < v_pages_count ; ++ i) {
			for (int j = 0 ; j < h_pages_count ; ++ j) {
				int page_number = (i * h_pages_count) + j + 1;
				if (page_number >= first_page && page_number <= last_page) {
					pages_to_print << pages_grid.at(i).at(j);
				}
			}
		}
		
		// parcourt les pages pour impression
		for (int i = 0 ; i < pages_to_print.count() ; ++ i) {
			QRect current_rect(pages_to_print.at(i));
			diagram -> render(
				&qp,
				QRect(QPoint(0,0), current_rect.size()),
				current_rect.translated(diagram_rect.topLeft()),
				Qt::KeepAspectRatio
			);
			if (i != pages_to_print.count() - 1) {
				printer -> newPage();
			}
		}
	}
	diagram -> setDrawTerminals(true);
	diagram -> setDisplayGrid(true);
}
