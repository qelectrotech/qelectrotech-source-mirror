/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "qetprintpreviewdialog.h"
#include "diagramschooser.h"
#include "exportproperties.h"
#include "exportpropertieswidget.h"
#include "qeticons.h"

#include <QPrintPreviewWidget>
#include <QPageSetupDialog>

/**
	Constructeur
	@param project Projet a imprimer
	@param printer Imprimante a utiliser pour
	@param widget  Widget parent
	@param f       Flags passes au constructeur de QDialog puis QWidget
*/
QETPrintPreviewDialog::QETPrintPreviewDialog(QETProject *project, QPrinter *printer, QWidget *widget, Qt::WindowFlags f) :
	QDialog(widget, f),
	project_(project),
	printer_(printer)
{
	setWindowTitle(tr("QElectroTech : Aperçu avant impression"));
	build();
	
	connect(preview_,       SIGNAL(paintRequested(QPrinter *)), this,     SLOT(requestPaint(QPrinter *)));
	connect(diagrams_list_, SIGNAL(selectionChanged()),         preview_, SLOT(updatePreview()));
	connect(diagrams_list_, SIGNAL(selectionChanged()),         this,     SLOT(checkDiagramsCount()));
	
	setWindowState(windowState() |  Qt::WindowMaximized);
}

/**
	Destructeur
*/
QETPrintPreviewDialog::~QETPrintPreviewDialog() {
}

/**
	@return le widget permettant de choisir les schemas a imprimer.
*/
DiagramsChooser *QETPrintPreviewDialog::diagramsChooser() {
	return(diagrams_list_);
}

/**
	@return true si l'option "Adapter le schema a la page" est activee
*/
bool QETPrintPreviewDialog::fitDiagramsToPages() const {
	return(fit_diagram_to_page_ -> isChecked());
}

/**
	@return les options de rendu definies par l'utilisateur
*/
ExportProperties QETPrintPreviewDialog::exportProperties() const {
	return(render_properties_ -> exportProperties());
}

/**
	Passe a la premiere page
*/
void QETPrintPreviewDialog::firstPage() {
	preview_ -> setCurrentPage(1);
}

/**
	Passe a la page precedente
*/
void QETPrintPreviewDialog::previousPage() {
	int preview_previous_page = preview_ -> currentPage() - 1;
	preview_ -> setCurrentPage(qMax(preview_previous_page, 0));
}

/**
	Passe a la page suivante
*/
void QETPrintPreviewDialog::nextPage() {
	int preview_next_page = preview_ -> currentPage() + 1;
	preview_ -> setCurrentPage(qMin(preview_next_page, preview_ -> pageCount()));
}

/**
	Passe a la derniere page
*/
void QETPrintPreviewDialog::lastPage() {
	preview_ -> setCurrentPage(preview_ -> pageCount());
}

/**
	Copnfigure la mise en page
*/
void QETPrintPreviewDialog::pageSetup() {
	QPageSetupDialog page_setup_dialog(printer_, this);
	if (page_setup_dialog.exec() == QDialog::Accepted) {
		preview_ -> updatePreview();
		updateZoomList();
	}
}

/**
	Utilise ou non toute la page sans teni compte des marges
	@param full_page true pour utiliser toute la page, false sinon
*/
void QETPrintPreviewDialog::useFullPage(bool full_page) {
	printer_ -> setFullPage(full_page);
	preview_ -> updatePreview();
	updateZoomList();
}

/**
	Fait tenir ou non chaque schema sur une page
	@param fit_diagram true pour adapter chaque schema sur une page, false sinon
*/
void QETPrintPreviewDialog::fitDiagramToPage(bool fit_diagram) {
	Q_UNUSED(fit_diagram);
	preview_ -> updatePreview();
	updateZoomList();
}

/**
	Effectue l'action "zoom avant" sur l'apercu avant impression
*/
void QETPrintPreviewDialog::zoomIn() {
	preview_ -> zoomIn(4.0/3.0);
	updateZoomList();
}

/**
	Effectue l'action "zoom arriere" sur l'apercu avant impression
*/
void QETPrintPreviewDialog::zoomOut() {
	preview_ -> zoomOut(4.0/3.0);
	updateZoomList();
}

/**
	Selectionne tous les schemas
*/
void QETPrintPreviewDialog::selectAllDiagrams() {
	diagrams_list_ -> setSelectedAllDiagrams(true);
}

/**
	Deselectionne tous les schemas
*/
void QETPrintPreviewDialog::selectNoDiagram() {
	diagrams_list_ -> setSelectedAllDiagrams(false);
}

/**
	Met en place le dialogue
*/
void QETPrintPreviewDialog::build() {
	preview_ = new QPrintPreviewWidget(printer_);
	diagrams_label_       = new QLabel(tr("Schémas à imprimer :"));
	diagrams_list_        = new DiagramsChooser(project_);
	diagrams_select_all_  = new QPushButton(tr("Tout cocher"));
	diagrams_select_none_ = new QPushButton(tr("Tout décocher"));
	toggle_diagrams_list_ = new QAction(QET::Icons::Diagram,              tr("Cacher la liste des schémas"),            this);
	toggle_print_options_ = new QAction(QET::Icons::Configure,            tr("Cacher les options d'impression"),           this);
	adjust_width_         = new QAction(QET::Icons::ViewFitWidth,         tr("Ajuster la largeur"),                        this);
	adjust_page_          = new QAction(QET::Icons::ViewFitWindow,        tr("Ajuster la page"),                           this);
	zoom_out_             = new QAction(QET::Icons::ZoomOut,              tr("Zoom arrière"),                           this);
	zoom_box_             = new QComboBox(this);
	zoom_in_              = new QAction(QET::Icons::ZoomIn,               tr("Zoom avant"),                                this);
	landscape_            = new QAction(QET::Icons::PrintLandscape,       tr("Paysage"),                                   this);
	portrait_             = new QAction(QET::Icons::PrintPortrait,        tr("Portrait"),                                  this);
	first_page_           = new QAction(QET::Icons::ArrowLeftDouble,      tr("Première page"),                          this);
	previous_page_        = new QAction(QET::Icons::ArrowLeft,            tr("Page précédente"),                     this);
	next_page_            = new QAction(QET::Icons::ArrowRight,           tr("Page suivante"),                             this);
	last_page_            = new QAction(QET::Icons::ArrowRightDouble,     tr("Dernière page"),                          this);
	single_page_view_     = new QAction(QET::Icons::SinglePage,           tr("Afficher une seule page"),                   this);
	facing_pages_view_    = new QAction(QET::Icons::PrintTwoPages,        tr("Afficher deux pages"),                       this);
	all_pages_view_       = new QAction(QET::Icons::PrintAllPages,        tr("Afficher un aperçu de toutes les pages"), this);
	page_setup_           = new QAction(QET::Icons::DocumentPrintFrame,   tr("Mise en page"),                              this);
	
	toggle_diagrams_list_ -> setCheckable(true);
	toggle_diagrams_list_ -> setChecked(true);
	toggle_print_options_ -> setCheckable(true);
	toggle_print_options_ -> setChecked(true);
	
#ifdef Q_OS_WIN32
	/*
		Sous Windows, le QPageSetupDialog utilise le dialogue natif ; ce
		dernier ne peut gerer que les imprimantes physiques ("native
		printers" ).
		cf avertissement : QAbstractPageSetupDialog::QAbstractPageSetupDialog:
		Page setup dialog cannot be used on non-native printers
	*/
	if (!(printer_ -> outputFileName().isEmpty())) {
		page_setup_ -> setEnabled(false);
		page_setup_ -> setText(tr("Mise en page (non disponible sous Windows pour l'impression PDF/PS)"));
	}
#endif
	
	toolbar_ = new QToolBar();
	toolbar_ -> addAction(toggle_diagrams_list_);
	toolbar_ -> addAction(toggle_print_options_);
	toolbar_ -> addSeparator();
	toolbar_ -> addAction(adjust_width_);
	toolbar_ -> addAction(adjust_page_);
	toolbar_ -> addAction(zoom_out_);
	toolbar_ -> addWidget(zoom_box_);
	toolbar_ -> addAction(zoom_in_);
	toolbar_ -> addSeparator();
	toolbar_ -> addAction(landscape_);
	toolbar_ -> addAction(portrait_);
	toolbar_ -> addSeparator();
	toolbar_ -> addAction(first_page_);
	toolbar_ -> addAction(previous_page_);
	toolbar_ -> addAction(next_page_);
	toolbar_ -> addAction(last_page_);
	toolbar_ -> addSeparator();
	toolbar_ -> addAction(single_page_view_);
	toolbar_ -> addAction(facing_pages_view_);
	toolbar_ -> addAction(all_pages_view_);
	toolbar_ -> addSeparator();
	toolbar_ -> addAction(page_setup_);
	
	print_options_box_= new QGroupBox(tr("Options d'impression"));
	use_full_page_ = new QCheckBox(tr("Utiliser toute la feuille"));
	use_full_page_ -> setChecked(printer_ -> fullPage());
	use_full_page_label_ = new QLabel(tr(
		"Si cette option est cochée, les marges de la feuille seront "
		"ignorées et toute sa surface sera utilisée pour l'impression. "
		"Cela peut ne pas être supporté par votre imprimante."
	));
	use_full_page_label_ -> setWordWrap(true);
	use_full_page_label_ -> setContentsMargins(20, 0, 0, 0);
	fit_diagram_to_page_ = new QCheckBox(tr("Adapter le schéma à la page"));
	fit_diagram_to_page_label_ = new QLabel(tr(
		"Si cette option est cochée, le schéma sera agrandi ou "
		"rétréci de façon à remplir toute la surface imprimable "
		"d'une et une seule page."
	));
	fit_diagram_to_page_label_ -> setWordWrap(true);
	fit_diagram_to_page_label_ -> setContentsMargins(20, 0, 0, 0);
	fit_diagram_to_page_ -> setChecked(true);
	
	// recupere les parametres d'export definis dans la configuration de l'application
	ExportProperties default_print_properties = ExportProperties::defaultPrintProperties();
	
	render_properties_ = new ExportPropertiesWidget(default_print_properties);
	render_properties_ -> setPrintingMode(true);
	
	buttons_ = new QDialogButtonBox();
	buttons_ -> addButton(new QPushButton(QET::Icons::DocumentPrint, tr("Imprimer")), QDialogButtonBox::AcceptRole);
	buttons_ -> addButton(QDialogButtonBox::Cancel);
	
	connect(diagrams_select_all_,  SIGNAL(released()),    this,     SLOT(selectAllDiagrams()));
	connect(diagrams_select_none_, SIGNAL(released()),    this,     SLOT(selectNoDiagram()));
	connect(toggle_diagrams_list_, SIGNAL(toggled(bool)), this,     SLOT(setDiagramsListVisible(bool)));
	connect(toggle_print_options_, SIGNAL(toggled(bool)), this,     SLOT(setPrintOptionsVisible(bool)));
	connect(adjust_width_,         SIGNAL(triggered()),   preview_, SLOT(fitToWidth()));
	connect(adjust_page_,          SIGNAL(triggered()),   preview_, SLOT(fitInView()));
	connect(zoom_out_,             SIGNAL(triggered()),   this,     SLOT(zoomOut()));
	connect(zoom_in_,              SIGNAL(triggered()),   this,     SLOT(zoomIn()));
	connect(landscape_,            SIGNAL(triggered()),   preview_, SLOT(setLandscapeOrientation()));
	connect(portrait_,             SIGNAL(triggered()),   preview_, SLOT(setPortraitOrientation()));
	connect(first_page_,           SIGNAL(triggered()),   this,     SLOT(firstPage()));
	connect(previous_page_,        SIGNAL(triggered()),   this,     SLOT(previousPage()));
	connect(next_page_,            SIGNAL(triggered()),   this,     SLOT(nextPage()));
	connect(last_page_,            SIGNAL(triggered()),   this,     SLOT(lastPage()));
	connect(single_page_view_,     SIGNAL(triggered()),   preview_, SLOT(setSinglePageViewMode()));
	connect(facing_pages_view_,    SIGNAL(triggered()),   preview_, SLOT(setFacingPagesViewMode()));
	connect(all_pages_view_,       SIGNAL(triggered()),   preview_, SLOT(setAllPagesViewMode()));
	connect(page_setup_,           SIGNAL(triggered()),   this,     SLOT(pageSetup()));
	
	connect(use_full_page_,        SIGNAL(toggled(bool)), this, SLOT(useFullPage(bool)));
	connect(fit_diagram_to_page_,  SIGNAL(toggled(bool)), this, SLOT(fitDiagramToPage(bool)));
	
	connect(render_properties_,    SIGNAL(optionChanged()), preview_, SLOT(updatePreview()));
	
	connect(preview_,  SIGNAL(previewChanged()),         this, SLOT(updateZoomList()));
	connect(zoom_box_, SIGNAL(currentIndexChanged(int)), this, SLOT(updatePreviewZoom()));
	
	connect(buttons_, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons_, SIGNAL(rejected()), this, SLOT(reject()));
	
	hlayout0_ = new QHBoxLayout();
	vlayout0_ = new QVBoxLayout();
	vlayout1_ = new QVBoxLayout();
	vlayout2_ = new QVBoxLayout();
	
	vlayout1_ -> addWidget(use_full_page_);
	vlayout1_ -> addWidget(use_full_page_label_);
	vlayout1_ -> addWidget(fit_diagram_to_page_);
	vlayout1_ -> addWidget(fit_diagram_to_page_label_);
	print_options_box_ -> setLayout(vlayout1_);
	
	vlayout2_ -> addWidget(diagrams_label_);
	vlayout2_ -> addWidget(diagrams_list_);
	vlayout2_ -> addWidget(diagrams_select_all_);
	vlayout2_ -> addWidget(diagrams_select_none_);
	
	hlayout0_ -> addLayout(vlayout2_);
	hlayout0_ -> addWidget(preview_);
	
	vlayout0_ -> addWidget(toolbar_);
	vlayout0_ -> addLayout(hlayout0_);
	vlayout0_ -> addWidget(render_properties_);
	vlayout0_ -> addWidget(print_options_box_);
	vlayout0_ -> addWidget(buttons_);
	
	setLayout(vlayout0_);
	updateZoomList();
}

/**
	Ce slot prive emet le signal paintRequested avec :
	  * la liste des schemas a imprimer / selectionnes
	  * un booleen indiquant s'il faut adapter les schemas aux pages ou non
	  * l'imprimante a utiliser
*/
void QETPrintPreviewDialog::requestPaint(QPrinter *printer) {
	emit(
		paintRequested(
			diagrams_list_ -> selectedDiagrams(),
			fit_diagram_to_page_ -> isChecked(),
			render_properties_ -> exportProperties(),
			printer
		)
	);
}

/**
	Ce slot prive verifie que le nombre de schemas a imprimer est bien superieur
	a 0 et active ou desactive le bouton "Imprimer" en consequence.
*/
void QETPrintPreviewDialog::checkDiagramsCount() {
	int diagrams_count = diagrams_list_ -> selectedDiagrams().count();
	
	// desactive le premier bouton de la liste (= le bouton "Imprimer")
	QList<QAbstractButton *> buttons = buttons_ -> buttons();
	if (buttons.count()) buttons[0] -> setEnabled(diagrams_count);
}

/**
	Ce slot prive affiche ou cache la liste des schemas
	@param display true pour affiche la liste des schemas, false pour la cacher
*/
void QETPrintPreviewDialog::setDiagramsListVisible(bool display) {
	diagrams_label_ -> setVisible(display);
	diagrams_list_  -> setVisible(display);
	diagrams_select_all_ -> setVisible(display);
	diagrams_select_none_ -> setVisible(display);
	
	if (display) {
		toggle_diagrams_list_ -> setText(tr("Cacher la liste des schémas"));
	} else {
		toggle_diagrams_list_ -> setText(tr("Afficher la liste des schémas"));
	}
}

/**
	Ce slot prive affiche ou cache les options d'impression
	@param display true pour affiche les options d'impression, false pour les
	cacher
*/
void QETPrintPreviewDialog::setPrintOptionsVisible(bool display) {
	print_options_box_ -> setVisible(display);
	render_properties_ -> setVisible(display);
	
	if (display) {
		toggle_print_options_ -> setText(tr("Cacher les options d'impression"));
	} else {
		toggle_print_options_ -> setText(tr("Afficher les options d'impression"));
	}
}

/**
	Met a jour la liste des zooms disponibles
*/
void QETPrintPreviewDialog::updateZoomList() {
	// recupere le zooom courant
	qreal current_zoom = preview_ -> zoomFactor();
	bool current_zoom_is_not_null = bool(int(current_zoom * 100.0));
	
	// liste des zooms par defaut
	QList<qreal> zooms_real;
	zooms_real << 0.25 << 0.5 << 0.75 << 1.0 << 1.5 << 2.0 << 4.0 << 8.0;
	
	// ajout du zoom en cours
	if (current_zoom_is_not_null && (!zooms_real.contains(current_zoom))) {
		zooms_real << current_zoom;
		qSort(zooms_real.begin(), zooms_real.end());
	}
	
	// remplissage de la liste deroulante
	int current_zoom_index = -1;
	zoom_box_ -> blockSignals(true);
	zoom_box_ -> clear();
	foreach (qreal z, zooms_real) {
		zoom_box_ -> addItem(QString(tr("%1 %")).arg(z * 100.0, 0, 'f', 2), z);
		if (z == current_zoom) current_zoom_index = zoom_box_ -> count() - 1;
	}
	zoom_box_ -> setCurrentIndex(current_zoom_index);
	zoom_box_ -> blockSignals(false);
}

/**
	Change le zoom de l'apercu en fonctiopn du contenu du zoom selectionne
*/
void QETPrintPreviewDialog::updatePreviewZoom() {
	preview_ -> setZoomFactor(
		zoom_box_ -> itemData(zoom_box_ -> currentIndex()).toDouble()
	);
	updateZoomList();
}
