/*
	Copyright 2006-2009 Xavier Guerrin
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
#include "qetprintpreviewdialog.h"
#include <math.h>
#include "diagramschooser.h"
#include "qeticons.h"
#include "qetmessagebox.h"

/**
	Constructeur
	@param dia     Schema a imprimer
	@param printer Imprimante a utiliser
	@param parent  Widget parent du dialogue
*/
DiagramPrintDialog::DiagramPrintDialog(QETProject *project, QWidget *parent) :
	QWidget(parent),
	project_(project),
	dialog_(0)
{
	// initialise l'imprimante
	printer_ = new QPrinter();
	
	// orientation paysage par defaut
	printer_ -> setOrientation(QPrinter::Landscape);
}

/**
	Destructeur
*/
DiagramPrintDialog::~DiagramPrintDialog() {
	delete dialog_;
	delete printer_;
}

/**
	Definit le nom du PDF si l'utilisateur choisit une sortie vers un PDF
*/
void DiagramPrintDialog::setFileName(const QString &name) {
	file_name_ = name;
}

/**
	@return le nom du PDF
*/
QString DiagramPrintDialog::fileName() const {
	return(file_name_);
}

/**
	Definit le nom du document
*/
void DiagramPrintDialog::setDocName(const QString &name) {
	doc_name_ = name;
}

/**
	@return le nom du document
*/
QString DiagramPrintDialog::docName() const {
	return(doc_name_);
}

/**
	@param diagram Un schema
	@return le rectangle a imprimer pour ce schema
*/
QRect DiagramPrintDialog::diagramRect(Diagram *diagram) const {
	if (!diagram) return(QRect());
	
	// ajuste la bordure du schema d'un pixel (epaisseur du trait)
	QRect diagram_rect = diagram -> border().adjusted(0.0, 0.0, 1.0, 1.0).toAlignedRect();
	
	return(diagram_rect);
}

/**
	Execute le dialogue d'impression
*/
void DiagramPrintDialog::exec() {
	
	// prise en compte du nom du document
	if (!doc_name_.isEmpty()) printer_ -> setDocName(doc_name_);
	
	// affichage d'un premier dialogue demandant a l'utilisateur le type
	// d'impression qu'il souhaite effectuer
	buildPrintTypeDialog();
	if (dialog_ -> exec() == QDialog::Rejected) return;
	
	// parametrage de l'imprimante en fonction du type d'impression choisi
	if (printer_choice_ -> isChecked()) {
		// affichage du dialogue d'impression standard pour parametrer l'imprimante
		QPrintDialog print_dialog(printer_, parentWidget());
#ifdef Q_WS_MAC
		print_dialog.setWindowFlags(Qt::Sheet);
#endif
		print_dialog.setWindowTitle(tr("Options d'impression", "window title"));
		print_dialog.setEnabledOptions(QAbstractPrintDialog::PrintShowPageSize);
		if (print_dialog.exec() == QDialog::Rejected) return;
	} else if (pdf_choice_ -> isChecked()) {
		printer_ -> setOutputFormat(QPrinter::PdfFormat);
		printer_ -> setOutputFileName(filepath_field_ -> text());
	} else {
		printer_ -> setOutputFormat(QPrinter::PostScriptFormat);
		printer_ -> setOutputFileName(filepath_field_ -> text());
	}
	
	// Apercu avant impression
	QETPrintPreviewDialog preview_dialog(project_, printer_, parentWidget());
	connect(&preview_dialog, SIGNAL(paintRequested(const QList<Diagram *> &, bool, QPrinter *)), this, SLOT(print(const QList<Diagram *> &, bool, QPrinter *)));
	DiagramsChooser *dc = preview_dialog.diagramsChooser();
	dc -> setSelectedAllDiagrams();
	if (preview_dialog.exec() == QDialog::Rejected) return;
	
	// effectue l'impression en elle-meme
	print(dc -> selectedDiagrams(), preview_dialog.fitDiagramsToPages(), printer_);
}

/**
	@param fullPage true pour utiliser toute la feuille dans le calcul
	@return Le nombre de pages necessaires pour imprimer le schema
	avec l'orientation et le format papier utilise dans l'imprimante en cours.
*/
int DiagramPrintDialog::pagesCount(Diagram *diagram, bool fullpage) const {
	return(horizontalPagesCount(diagram, fullpage) * verticalPagesCount(diagram, fullpage));
}

/**
	@param fullPage true pour utiliser toute la feuille dans le calcul
	@return La largeur du "poster" en nombre de pages pour imprimer le schema
	avec l'orientation et le format papier utilise dans l'imprimante en cours.
*/
int DiagramPrintDialog::horizontalPagesCount(Diagram *diagram, bool fullpage) const {
	// note : pageRect et Paper Rect tiennent compte de l'orientation du papier
	QRect printable_area = fullpage ? printer_ -> paperRect() : printer_ -> pageRect();
	QRect diagram_rect = diagramRect(diagram);
	
	int h_pages_count = int(ceil(qreal(diagram_rect.width()) / qreal(printable_area.width())));
	return(h_pages_count);
}

/**
	@param fullPage true pour utiliser toute la feuille dans le calcul
	@return La largeur du "poster" en nombre de pages pour imprimer le schema
	avec l'orientation et le format papier utilise dans l'imprimante en cours.
*/
int DiagramPrintDialog::verticalPagesCount(Diagram *diagram, bool fullpage) const {
	// note : pageRect et Paper Rect tiennent compte de l'orientation du papier
	QRect printable_area = fullpage ? printer_ -> paperRect() : printer_ -> pageRect();
	QRect diagram_rect = diagramRect(diagram);
	
	int v_pages_count = int(ceil(qreal(diagram_rect.height()) / qreal(printable_area.height())));
	return(v_pages_count);
}

/**
	Construit un dialogue non standard pour demander a l'utilisateur quelle type
	d'impression il souhaite effectuer : PDF, PS ou imprimante physique
*/
void DiagramPrintDialog::buildPrintTypeDialog() {
	// initialisation des widgets
	dialog_           = new QDialog(parentWidget());
#ifdef Q_WS_MAC
	dialog_ -> setWindowFlags(Qt::Sheet);
#endif
	
	printtype_label_  = new QLabel(tr("Quel type d'impression d\351sirez-vous effectuer ?"));
	printer_icon_     = new QLabel();
	pdf_icon_         = new QLabel();
	ps_icon_          = new QLabel();
	printtype_choice_ = new QButtonGroup();
	printer_choice_   = new QRadioButton(tr("Impression sur une imprimante physique",               "Print type choice"));
	pdf_choice_       = new QRadioButton(tr("Impression vers un fichier au format PDF",             "Print type choice"));
	ps_choice_        = new QRadioButton(tr("Impression vers un fichier au format PostScript (PS)", "Print type choice"));
	filepath_field_   = new QLineEdit();
	browse_button_    = new QPushButton("...");
	buttons_          = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	
	dialog_ -> setWindowTitle(tr("Choix du type d'impression"));
	printer_icon_ -> setPixmap(QET::Icons::Printer.pixmap(32, 32));
	pdf_icon_     -> setPixmap(QET::Icons::PDF.pixmap(32, 32));
	ps_icon_      -> setPixmap(QET::Icons::EPS.pixmap(32, 32));
	printtype_choice_ -> addButton(printer_choice_);
	printtype_choice_ -> addButton(pdf_choice_);
	printtype_choice_ -> addButton(ps_choice_);
	printer_choice_ -> setChecked(true);
	if (!file_name_.isEmpty()) filepath_field_ -> setText(file_name_ + ".pdf");
	
	// connexions signaux / slots
	connect(printer_choice_, SIGNAL(toggled(bool)), this,    SLOT(updatePrintTypeDialog()));
	connect(pdf_choice_,     SIGNAL(toggled(bool)), this,    SLOT(updatePrintTypeDialog()));
	connect(ps_choice_,      SIGNAL(toggled(bool)), this,    SLOT(updatePrintTypeDialog()));
	connect(browse_button_,  SIGNAL(clicked(bool)), this,    SLOT(browseFilePrintTypeDialog()));
	connect(buttons_,        SIGNAL(accepted()),    this,    SLOT(acceptPrintTypeDialog()));
	connect(buttons_,        SIGNAL(rejected()),    dialog_, SLOT(reject()));
	
	// organisation graphique
	glayout0_ = new QGridLayout();
	hlayout0_ = new QHBoxLayout();
	vlayout0_ = new QVBoxLayout();
	
	hlayout0_ -> addWidget(filepath_field_);
	hlayout0_ -> addWidget(browse_button_);
	glayout0_ -> addWidget(printer_icon_,   0, 0);
	glayout0_ -> addWidget(printer_choice_, 0, 1);
	glayout0_ -> addWidget(pdf_icon_,       1, 0);
	glayout0_ -> addWidget(pdf_choice_,     1, 1);
	glayout0_ -> addWidget(ps_icon_,        2, 0);
	glayout0_ -> addWidget(ps_choice_,      2, 1);
	glayout0_ -> addLayout(hlayout0_,       3, 1);
	
	vlayout0_ -> addWidget(printtype_label_);
	vlayout0_ -> addLayout(glayout0_);
	vlayout0_ -> addWidget(buttons_);
	
	dialog_ -> setLayout(vlayout0_);
	
	updatePrintTypeDialog();
}

/**
	Assure la coherence du dialogue permettant le choix du type d'impression
*/
void DiagramPrintDialog::updatePrintTypeDialog() {
	// imprime-t-on vers un fichier ?
	bool file_print = !(printer_choice_ -> isChecked());
	
	// on n'active le champ fichier que pour les impressions vers un fichier
	filepath_field_ -> setEnabled(file_print);
	browse_button_  -> setEnabled(file_print);
	
	// on corrige eventuellement l'extension du fichier deja selectionne
	if (file_print) {
		QString filepath = filepath_field_ -> text();
		if (!filepath.isEmpty()) {
			if (pdf_choice_ -> isChecked() && filepath.endsWith(".ps")) {
				QRegExp re("\\.ps$", Qt::CaseInsensitive);
				filepath.replace(re, ".pdf");
				filepath_field_ -> setText(filepath);
			} else if (ps_choice_ -> isChecked() && filepath.endsWith(".pdf")) {
				QRegExp re("\\.pdf$", Qt::CaseInsensitive);
				filepath.replace(re, ".ps");
				filepath_field_ -> setText(filepath);
			}
		}
	}
}

/**
	Verifie l'etat du dialogue permettant le choix du type d'impression lorsque
	l'utilisateur le valide.
*/
void DiagramPrintDialog::acceptPrintTypeDialog() {
	bool file_print = !(printer_choice_ -> isChecked());
	if (file_print) {
		// un fichier doit avoir ete entre
		if (filepath_field_ -> text().isEmpty()) {
			QET::MessageBox::information(
				parentWidget(),
				tr("Fichier manquant", "message box title"),
				tr("Vous devez indiquer le chemin du fichier PDF/PS \340 cr\351er.", "message box content")
			);
		} else dialog_ -> accept();
	} else {
		// une imprimante doit avoir ete selectionnee
		/// @todo
		dialog_ -> accept();
	}
}

/**
	Permet a l'utilisateur de choisir un fichier 
*/
void DiagramPrintDialog::browseFilePrintTypeDialog() {
	QString extension;
	QString filter;
	if (printer_choice_ -> isChecked()) return;
	else if (pdf_choice_ -> isChecked()) {
		extension = ".pdf";
		filter    = tr("Fichiers PDF (*.pdf)",       "file filter");
	}
	else if (ps_choice_ -> isChecked()) {
		extension = ".ps";
		filter    = tr("Fichiers PostScript (*.ps)", "file filter");
	}
	
	QString filepath = QFileDialog::getSaveFileName(
		parentWidget(),
		QString(),
		filepath_field_ -> text(),
		filter
	);
	
	if (!filepath.isEmpty()) {
		if (!filepath.endsWith(extension)) filepath += extension;
		filepath_field_ -> setText(filepath);
	}
}

/**
	Effectue l'impression elle-meme
	@param diagrams Schemas a imprimer
	@param fit_page Booleen indiquant s'il faut adapter les schemas aux pages
	ou non
	@param printer L'imprimante a utiliser
*/
void DiagramPrintDialog::print(const QList<Diagram *> &diagrams, bool fit_page, QPrinter */*printer*/) {
	//qDebug() << "Demande d'impression de " << diagrams.count() << "schemas.";
	
	// QPainter utiliser pour effectuer le rendu
	QPainter qp(printer_);
	
	// cas special : il n'y a aucun schema a imprimer
	if (!diagrams.count()) {
		qp.end();
		return;
	}
	
	// imprime les schemas
	for (int i = 0 ; i < diagrams.count() ; ++ i) {
		printDiagram(diagrams[i], fit_page, &qp, printer_);
		if (i != diagrams.count() - 1) {
			printer_ -> newPage();
		}
	}
}

/**
	Imprime un schema
	@param diagram Schema a imprimer
	@param fit_page True pour  adapter les schemas aux pages, false sinon
	@param qp QPainter a utiliser (deja initialise sur printer)
	@param printer Imprimante a utiliser
*/
void DiagramPrintDialog::printDiagram(Diagram *diagram, bool fit_page, QPainter *qp, QPrinter *printer) {
	//qDebug() << printer -> paperSize() << printer -> paperRect() << diagram -> title();
	// l'imprimante utilise-t-elle toute la feuille ?
	bool full_page = printer -> fullPage ();
	
	// impression physique (!= fichier PDF)
	if (printer -> outputFileName().isEmpty()) {
		// utiliser cette condition pour agir differemment en cas d'impression physique
	}
	
	diagram -> setDisplayGrid(false);
	diagram -> setDrawTerminals(false);
	
	if (fit_page) {
		// impression adaptee sur une seule page
		diagram -> render(qp, QRectF(), diagramRect(diagram), Qt::KeepAspectRatio);
	} else {
		// impression sur une ou plusieurs pages
		QRect diagram_rect = diagramRect(diagram);
		QRect printed_area = full_page ? printer -> paperRect() : printer -> pageRect();
		//qDebug() << "impression sur une ou plusieurs pages";
		//qDebug() << "  schema :" << diagram_rect;
		//qDebug() << "  page   :" << printed_area;
		
		int used_width  = printed_area.width();
		int used_height = printed_area.height();
		int h_pages_count = horizontalPagesCount(diagram, full_page);
		int v_pages_count = verticalPagesCount(diagram, full_page);
		
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
				//int page_number = (i * h_pages_count) + j + 1;
				//if (page_number >= first_page && page_number <= last_page) {
					pages_to_print << pages_grid.at(i).at(j);
				//}
			}
		}
		//qDebug() << "  " << pages_to_print.count() << " pages a imprimer :";
		
		// parcourt les pages pour impression
		for (int i = 0 ; i < pages_to_print.count() ; ++ i) {
			QRect current_rect(pages_to_print.at(i));
			//qDebug() << "    " << current_rect;
			diagram -> render(
				qp,
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
