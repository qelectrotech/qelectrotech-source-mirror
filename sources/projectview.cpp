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
#include "projectview.h"
#include "qetproject.h"
#include "configdialog.h"
#include "closediagramsdialog.h"
#include "projectconfigpages.h"
#include "diagramview.h"
#include "diagram.h"
#include "diagramprintdialog.h"
#include "exportdialog.h"
#include "qetapp.h"
#include "qettabwidget.h"
#include "qetelementeditor.h"
#include "interactivemoveelementshandler.h"
#include "borderpropertieswidget.h"
#include "titleblockpropertieswidget.h"
#include "conductorpropertieswidget.h"
#include "qeticons.h"
#include "qetmessagebox.h"
#include "qettabbar.h"
#include "qettemplateeditor.h"
#include "diagramfoliolist.h"

/**
	Constructeur
	@param project projet a visualiser
	@param parent Widget parent
*/
ProjectView::ProjectView(QETProject *project, QWidget *parent) :
	QWidget(parent),
	project_(0)
{
	initActions();
	initWidgets();
	initLayout();
	
	setProject(project);
}

/**
	Destructeur
	Supprime les DiagramView embarquees
*/
ProjectView::~ProjectView() {
	// qDebug() << "Suppression du ProjectView" << ((void *)this);
	foreach(int id, diagram_ids_.keys()) {
		DiagramView *diagram_view = diagram_ids_.take(id);
		delete diagram_view;
	}
}

/**
	@return le projet actuellement visualise par le ProjectView
*/
QETProject *ProjectView::project() {
	return(project_);
}

/**
	Definit le projet visualise par le ProjectView. Ne fait rien si le projet a
	deja ete defini.
	@param project projet a visualiser
*/
void ProjectView::setProject(QETProject *project) {
	if (!project_) {
		project_ = project;
		connect(project_, SIGNAL(projectTitleChanged(QETProject *, const QString &)), this, SLOT(updateWindowTitle()));
		connect(project_, SIGNAL(projectModified    (QETProject *, bool)),            this, SLOT(updateWindowTitle()));
		connect(project_, SIGNAL(readOnlyChanged    (QETProject *, bool)),            this, SLOT(adjustReadOnlyState()));
		adjustReadOnlyState();
		loadDiagrams();
	}
}

/**
	@return la liste des schemas ouverts dans le projet
*/
QList<DiagramView *> ProjectView::diagrams() const {
	return(diagrams_);
}

/**
	@return A list containing child diagrams matching provided \a options.
*/
QList<Diagram *> ProjectView::getDiagrams(ProjectSaveOptions options) {
	QList<Diagram *> selection;
	if ((options & AllDiagrams) == AllDiagrams) {
		selection << project_ -> diagrams();
	} else {
		Diagram *current = 0;
		if (DiagramView *view = currentDiagram()) {
			current = view -> diagram();
		}
		if (options & CurrentDiagram) {
			if (current) selection << current;
		} else if (options & AllDiagramsButCurrent) {
			selection = project_ -> diagrams();
			selection.removeOne(current);
		}
	}
	
	if (options & ModifiedDiagramsOnly) {
		foreach (Diagram *diagram, selection) {
			if (!diagram -> undoStack().isClean() || !diagram -> wasWritten()) continue;
			selection.removeOne(diagram);
		}
	}
	
	return(selection);
}

/**
	@return le schema actuellement active
*/
DiagramView *ProjectView::currentDiagram() const {
	int current_tab_index = tabs_ -> currentIndex();
	return(diagram_ids_[current_tab_index]);
}

/**
	Gere la fermeture du schema.
	@param qce Le QCloseEvent decrivant l'evenement
*/
void ProjectView::closeEvent(QCloseEvent *qce) {
	bool can_close_project = tryClosing();
	if (can_close_project) {
		qce -> accept();
		emit(projectClosed(this));
	} else {
		qce -> ignore();
	}
}

/**
	Cette methode essaye de fermer successivement les editeurs d'element puis
	les schemas du projet. L'utilisateur peut refuser de fermer un schema ou un
	editeur.
	@return true si tout a pu etre ferme, false sinon
	@see tryClosingElementEditors()
	@see tryClosingDiagrams()
*/
bool ProjectView::tryClosing() {
	if (!project_) return(true);
	
	// First step: require external editors closing -- users may either cancel
	// the whole closing process or save (and therefore add) content into this
	// project. Of course, they may also discard them.
	if (!tryClosingElementEditors()) {
		return(false);
	}
	
	// Check how different the current situation is from a brand new, untouched project
	if (project_ -> filePath().isEmpty() && !project_ -> projectWasModified()) {
		return(true);
	}
	
	// Second step: users are presented with a dialog that enables them to
	// choose whether they want to:
	//   - cancel the closing process,
	//   - discard all modifications,
	//   - or specify what is to be saved, i.e. they choose whether they wants to
	// save/give up/remove diagrams considered as modified.
	int user_input = tryClosingDiagrams();
	if (user_input == QMessageBox::Cancel) {
		return(false); // the closing process was cancelled
	} else if (user_input == QMessageBox::Discard) {
		return(true); // all modifications were discarded
	}
	
	// Check how different the current situation is from a brand new, untouched project (yes , again)
	if (project_ -> filePath().isEmpty() && !project_ -> projectWasModified()) {
		return(true);
	}
	
	if (project_ -> filePath().isEmpty()) {
		QString filepath = askUserForFilePath();
		if (filepath.isEmpty()) return(false); // users may cancel the closing
	}
	QETResult result = project_ -> write();
	updateWindowTitle();
	if (!result.isOk()) emit(errorEncountered(result.errorMessage()));
	return(result.isOk());
}

/**
	Un projet comporte des elements integres. Cette methode ferme les editeurs
	d'elements associes a ce projet. L'utilisateur peut refuser la fermeture
	d'un editeur d'element.
	@return true si tous les editeurs d'element ont pu etre fermes, false sinon
*/
bool ProjectView::tryClosingElementEditors() {
	if (!project_) return(true);
	/*
		La QETApp permet d'acceder rapidement aux editeurs d'element
		editant un element du projet.
	*/
	QList<QETElementEditor *> editors = QETApp::elementEditors(project_);
	foreach(QETElementEditor *editor, editors) {
		if (!editor -> close()) return(false);
	}
	
	QList<QETTitleBlockTemplateEditor *> template_editors = QETApp::titleBlockTemplateEditors(project_);
	foreach(QETTitleBlockTemplateEditor *template_editor, template_editors) {
		if (!template_editor -> close()) return(false);
	}
	return(true);
}

/**
 * @brief ProjectView::tryClosingDiagrams
 * try to close this project, if diagram or project option are changed
 * a dialog ask if user want to save the modification.
 * @return the answer of dialog or discard if no change.
 */
int ProjectView::tryClosingDiagrams() {
	if (!project_) return(QMessageBox::Discard);

	if (!project()->projectOptionsWereModified() &&
		project()->undoStack()->isClean() &&
		!project()->filePath().isEmpty()) {
		// nothing was modified, and we have a filepath, i.e. everything was already
		// saved, i.e we can close the project right now
		return(QMessageBox::Discard);
	}

	QString title = project()->title();
	if (title.isEmpty()) title = "QElectroTech ";

	int close_dialog = QMessageBox::question(this, title,
								   tr("Le projet \340 \351t\351 modifi\351.\n"
									  "Voulez-vous enregistrer les modifications ?"),
								   QMessageBox::Save | QMessageBox::Discard
								   | QMessageBox::Cancel,
								   QMessageBox::Save);

	if (close_dialog == QMessageBox::Save) {
		saveDiagrams(project()->diagrams());
	}
	
	return(close_dialog);
}

/**
	Ask the user to provide a file path in which the currently edited project will
	be saved.
	@param assign When true, assign the provided filepath to the project through
	setFilePath(). Defaults to true.
	@return the file path, or an empty string if none were provided
*/
QString ProjectView::askUserForFilePath(bool assign) {
	// ask the user for a filepath in order to save the project
	QString filepath = QFileDialog::getSaveFileName(
		this,
		tr("Enregistrer sous", "dialog title"),
		project_ -> currentDir(),
		tr("Sch\351ma QElectroTech (*.qet)", "filetypes allowed when saving a diagram file")
	);
	
	// if no filepath is provided, return an empty string
	if (filepath.isEmpty()) return(filepath);
	
	// if the name does not end with the .qet extension, append it
	if (!filepath.endsWith(".qet", Qt::CaseInsensitive)) filepath += ".qet";
	
	if (assign) {
		// assign the provided filepath to the currently edited project
		project_ -> setFilePath(filepath);
	}
	
	return(filepath);
}

/**
	@return the QETResult object to be returned when it appears this project
	view is not associated to any project.
*/
QETResult ProjectView::noProjectResult() const {
	QETResult no_project(tr("aucun projet affich\351", "error message"), false);
	return(no_project);
}

/**
	Ajoute un nouveau schema au ProjectView
*/
void ProjectView::addNewDiagram() {
	if (project_ -> isReadOnly()) return;
	
	Diagram *new_diagram = project_ -> addNewDiagram();
	DiagramView *new_diagram_view = new DiagramView(new_diagram);
	addDiagram(new_diagram_view);

	if (project_ -> diagrams().size() % 58 == 1 && project_ -> getFolioSheetsQuantity() != 0)
		addNewDiagramFolioList();
	showDiagram(new_diagram_view);
}

void ProjectView::addNewDiagramFolioList() {
	if (project_ -> isReadOnly()) return;

	Diagram *new_diagram = project_ -> addNewDiagramFolioList();
	DiagramView *new_diagram_view = new DiagramView(new_diagram);
	addDiagram(new_diagram_view, true);
	showDiagram(new_diagram_view);
}

/**
	Ajoute un schema au ProjectView
	@param diagram Schema a ajouter
*/
/**
 * @brief ProjectView::addDiagram
 * Add new digram to this project view
 * @param diagram added diagram
 * @param front: true add page at front
 *				 false add page at back
 */
void ProjectView::addDiagram(DiagramView *diagram, bool front) {
	if (!diagram) return;

	// check diagram isn't present in the project
	if (diagram_ids_.values().contains(diagram)) return;
	
	// Add new tab for the diagram
	tabs_ -> addTab(diagram, QET::Icons::Diagram, diagram -> title());
	diagram -> setFrameStyle(QFrame::Plain | QFrame::NoFrame);

	diagrams_ << diagram;

	rebuildDiagramsMap();
	connect(diagram, SIGNAL(showDiagram(Diagram*)), this, SLOT(showDiagram(Diagram*)));
	connect(diagram, SIGNAL(titleChanged(DiagramView *, const QString &)), this, SLOT(updateTabTitle(DiagramView *, const QString &)));
	connect(diagram, SIGNAL(findElementRequired(const ElementsLocation &)), this, SIGNAL(findElementRequired(const ElementsLocation &)));
	connect(diagram, SIGNAL(editElementRequired(const ElementsLocation &)), this, SIGNAL(editElementRequired(const ElementsLocation &)));
	connect(diagram, SIGNAL(editTitleBlockTemplate(const QString &, bool)), this, SLOT(editTitleBlockTemplateRequired(const QString &, bool)));
	
	// signal diagram was added
	emit(diagramAdded(diagram));
	// move tab to front if wanted
	if (front) {
		tabs_->moveTab(tabs_->count()-1, project_ -> getFolioSheetsQuantity()-1);
		//diagram -> diagram() -> project() -> setFolioSheetsQuantity(true);
	}
}

/**
	Enleve un schema du ProjectView
	@param diagram_view Schema a enlever
*/
void ProjectView::removeDiagram(DiagramView *diagram_view) {
	if (!diagram_view) return;
	if (project_ -> isReadOnly()) return;
	
	// verifie que le schema est bien present dans le projet
	if (!diagram_ids_.values().contains(diagram_view)) return;
	
	// demande confirmation a l'utilisateur
	if (
		diagram_view -> diagram() -> wasWritten() ||\
		!diagram_view -> diagram() -> undoStack().isClean()
	) {
		int answer = QET::MessageBox::question(
			this,
			tr("Supprimer le sch\351ma ?", "message box title"),
			tr("\312tes-vous s\373r de vouloir supprimer ce sch\351ma du projet ? Ce changement est irr\351versible.", "message box content"),
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
			QMessageBox::No
		);
		if (answer != QMessageBox::Yes) {
			return;
		}
	}
	
	// notifie le reste du monde que le DiagramView va disparaitre
	emit(diagramAboutToBeRemoved(diagram_view));
	
	// enleve le DiagramView des onglets
	int diagram_tab_id = diagram_ids_.key(diagram_view);
	tabs_ -> removeTab(diagram_tab_id);
	diagrams_.removeAll(diagram_view);
	rebuildDiagramsMap();
	
	// supprime le DiagramView, puis le Diagram
	project_ -> removeDiagram(diagram_view -> diagram());
	delete diagram_view;
	
	// signale le retrait du schema
	emit(diagramRemoved(diagram_view));
	
	// rend definitif le retrait du schema
	project_ -> write();
}

/**
	Enleve un schema du ProjectView
	@param diagram Schema a enlever
*/
void ProjectView::removeDiagram(Diagram *diagram) {
	if (!diagram) return;
	
	if (DiagramView *diagram_view = findDiagram(diagram)) {
		removeDiagram(diagram_view);
	}
}

/**
	Active l'onglet adequat pour afficher le schema passe en parametre
	@param diagram Schema a afficher
*/
void ProjectView::showDiagram(DiagramView *diagram) {
	if (!diagram) return;
	tabs_ -> setCurrentWidget(diagram);
}

/**
	Active l'onglet adequat pour afficher le schema passe en parametre
	@param diagram Schema a afficher
*/
void ProjectView::showDiagram(Diagram *diagram) {
	if (!diagram) return;
	if (DiagramView *diagram_view = findDiagram(diagram)) {
		tabs_ -> setCurrentWidget(diagram_view);
	}
}

/**
	Enable the user to edit properties of the current project through a
	configuration dialog.
*/
void ProjectView::editProjectProperties() {
	if (!project_) return;
	
	ConfigDialog properties_dialog(parentWidget());
	properties_dialog.setWindowTitle(tr("Propri\351t\351s du projet", "window title"));
	properties_dialog.addPage(new ProjectMainConfigPage       (project_));
	properties_dialog.addPage(new ProjectNewDiagramConfigPage (project_));
	properties_dialog.addPage(new ProjectAutoNumConfigPage    (project_));
	properties_dialog.exec();
}

/**
	Edite les proprietes du schema courant
*/
void ProjectView::editCurrentDiagramProperties() {
	editDiagramProperties(currentDiagram());
}

/**
	Edite les proprietes du schema diagram_view
*/
void ProjectView::editDiagramProperties(DiagramView *diagram_view) {
	if (!diagram_view) return;
	showDiagram(diagram_view);
	diagram_view -> editDiagramProperties();
}

/**
	Edite les proprietes du schema diagram
*/
void ProjectView::editDiagramProperties(Diagram *diagram) {
	editDiagramProperties(findDiagram(diagram));
}

/**
	Deplace le schema diagram_view vers le haut / la gauche
*/
void ProjectView::moveDiagramUp(DiagramView *diagram_view) {
	if (!diagram_view) return;
	
	int diagram_view_position = diagram_ids_.key(diagram_view);
	if (!diagram_view_position) {
		// le schema est le premier du projet
		return;
	}
	tabs_ -> moveTab(diagram_view_position, diagram_view_position - 1);
}

/**
	Deplace le schema diagram vers le haut / la gauche
*/
void ProjectView::moveDiagramUp(Diagram *diagram) {
	moveDiagramUp(findDiagram(diagram));
}

/**
	Deplace le schema diagram_view vers le bas / la droite
*/
void ProjectView::moveDiagramDown(DiagramView *diagram_view) {
	if (!diagram_view) return;
	
	int diagram_view_position = diagram_ids_.key(diagram_view);
	if (diagram_view_position + 1 == diagram_ids_.count()) {
		// le schema est le dernier du projet
		return;
	}
	tabs_ -> moveTab(diagram_view_position, diagram_view_position + 1);
}

/**
	Deplace le schema diagram vers le bas / la droite
*/
void ProjectView::moveDiagramDown(Diagram *diagram) {
	moveDiagramDown(findDiagram(diagram));
}

/**
	Ce slot demarre un dialogue permettant a l'utilisateur de parametrer et de
	lancer l'impression de toute ou partie du projet.
*/
void ProjectView::printProject() {
	if (!project_) return;
	
	// transforme le titre du projet en nom utilisable pour le document
	QString doc_name;
	if (!(project_ -> title().isEmpty())) {
		doc_name = project_ -> title();
	} else if (!project_ -> filePath().isEmpty()) {
		doc_name = QFileInfo(project_ -> filePath()).baseName();
	}
	doc_name = QET::stringToFileName(doc_name);
	if (doc_name.isEmpty()) {
		doc_name = tr("projet", "string used to generate a filename");
	}
	
	// recupere le dossier contenant le fichier courant
	QString dir_path = project_ -> currentDir();
	
	// determine un chemin pour le pdf / ps
	QString file_name = QDir::toNativeSeparators(QDir::cleanPath(dir_path + "/" + doc_name));
	
	DiagramPrintDialog print_dialog(project_, this);
	print_dialog.setDocName(doc_name);
	print_dialog.setFileName(file_name);
	print_dialog.exec();
}

/**
	Exporte le schema.
*/
void ProjectView::exportProject() {
	if (!project_) return;
	
	ExportDialog ed(project_, parentWidget());
#ifdef Q_WS_MAC
	ed.setWindowFlags(Qt::Sheet);
#endif
	ed.exec();
}

/**
	Save project properties along with all modified diagrams.
	@see filePath()
	@see setFilePath()
	@return a QETResult object reflecting the situation
*/
QETResult ProjectView::save() {
	return(doSave(AllDiagrams));
}

/**
	Ask users for a filepath in order to save the project.
	@param options May be used to specify what should be saved; defaults to
	all modified diagrams.
	@return a QETResult object reflecting the situation; note that a valid
	QETResult object is returned if the operation was cancelled.
*/
QETResult ProjectView::saveAs(ProjectSaveOptions options) {
	if (!project_) return(noProjectResult());
	
	QString filepath = askUserForFilePath();
	if (filepath.isEmpty()) return(QETResult());
	return(doSave(options));
}

/**
	Save the current diagram.
	@return A QETResult object reflecting the situation.
*/
QETResult ProjectView::saveCurrentDiagram() {
	return(doSave(CurrentDiagram));
}

/**
	Save project content according to \a options, then write the project file. May
	call saveAs if no filepath was provided before.
	@param options May be used to specify what should be saved (e.g. modified
	diagrams only).
	@return a QETResult object reflecting the situation; note that a valid
	QETResult object is returned if the operation was cancelled.
*/
QETResult ProjectView::doSave(ProjectSaveOptions options) {
	if (!project_) return(noProjectResult());
	
	if (project_ -> filePath().isEmpty()) {
		// The project has not been saved to a file yet,
		// so save() actually means saveAs().
		return(saveAs(options));
	}
	
	// look for diagrams matching the required save options
	saveDiagrams(getDiagrams(options));
	
	// write to file
	QETResult result = project_ -> write();
	updateWindowTitle();
	if (options == AllDiagrams) project()->undoStack()->clear();
	return(result);
}

/**
	Save \a diagrams without emitting the written() signal and without writing
	the project file itself.
*/
void ProjectView::saveDiagrams(const QList<Diagram *> &diagrams) {
	foreach (Diagram *diagram, diagrams) {
		// Diagram::write() emits the written() signal, which is connected
		// to QETProject::write() through QETProject::componentWritten().
		// We do not want to write the project immediately, so we block
		// this signal.
		diagram -> blockSignals(true);
		diagram -> write();
		diagram -> blockSignals(false);
	}
}

/**
	Allow the user to clean the project, which includes:
	  * deleting unused title block templates
	  * deleting unused elements
	  * deleting empty categories
	@return an integer value above zero if elements and/or categories were
	cleaned.
*/
int ProjectView::cleanProject() {
	if (!project_) return(0);
	
	// s'assure que le schema n'est pas en lecture seule
	if (project_ -> isReadOnly()) {
		QET::MessageBox::critical(
			this,
			tr("Projet en lecture seule", "message box title"),
			tr("Ce projet est en lecture seule. Il n'est donc pas possible de le nettoyer.", "message box content")
		);
		return(0);
	}
	
	// construit un petit dialogue pour parametrer le nettoyage
	QCheckBox *clean_tbt        = new QCheckBox(tr("Supprimer les mod\350les de cartouche inutilis\351s dans le projet"));
	QCheckBox *clean_elements   = new QCheckBox(tr("Supprimer les \351l\351ments inutilis\351s dans le projet"));
	QCheckBox *clean_categories = new QCheckBox(tr("Supprimer les cat\351gories vides"));
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	
	clean_tbt        -> setChecked(true);
	clean_elements   -> setChecked(true);
	clean_categories -> setChecked(true);
	
	QDialog clean_dialog(parentWidget());
#ifdef Q_WS_MAC
	clean_dialog.setWindowFlags(Qt::Sheet);
#endif
	
	clean_dialog.setWindowTitle(tr("Nettoyer le projet", "window title"));
	QVBoxLayout *clean_dialog_layout = new QVBoxLayout();
	clean_dialog_layout -> addWidget(clean_tbt);
	clean_dialog_layout -> addWidget(clean_elements);
	clean_dialog_layout -> addWidget(clean_categories);
	clean_dialog_layout -> addWidget(buttons);
	clean_dialog.setLayout(clean_dialog_layout);
	
	connect(buttons, SIGNAL(accepted()), &clean_dialog, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), &clean_dialog, SLOT(reject()));
	
	int clean_count = 0;
	if (clean_dialog.exec() == QDialog::Accepted) {
		if (clean_tbt -> isChecked()) {
			project_ -> cleanUnusedTitleBlocKTemplates();
		}
		if (clean_elements -> isChecked()) {
			InteractiveMoveElementsHandler *handler = new InteractiveMoveElementsHandler(this);
			project_ -> cleanUnusedElements(handler);
			delete handler;
			++ clean_count;
		}
		if (clean_categories -> isChecked()) {
			InteractiveMoveElementsHandler *handler = new InteractiveMoveElementsHandler(this);
			project_ -> cleanEmptyCategories(handler);
			delete handler;
			++ clean_count;
		}
	}
	return(clean_count);
}

/**
	Initialize actions for this widget.
*/
void ProjectView::initActions() {
	add_new_diagram_ = new QAction(QET::Icons::Add, tr("Ajouter un sch\351ma"), this);
	connect(add_new_diagram_, SIGNAL(triggered()), this, SLOT(addNewDiagram()));
}

/**
	Initialize child widgets for this widget.
*/
void ProjectView::initWidgets() {
	setObjectName("ProjectView");
	setWindowIcon(QET::Icons::ProjectFile);
	
	// initialize the "fallback" widget
	fallback_widget_ = new QWidget();
	fallback_label_ = new QLabel(
		tr(
			"Ce projet ne contient aucun sch\351ma",
			"label displayed when a project contains no diagram"
		)
	);
	fallback_label_ -> setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	
	// initialize tabs
	tabs_ = new QETTabWidget();
	tabs_ -> setMovable(true);
	
	QToolButton *add_new_diagram_button = new QToolButton();
	add_new_diagram_button -> setDefaultAction(add_new_diagram_);
	add_new_diagram_button -> setAutoRaise(true);
	tabs_ -> setCornerWidget(add_new_diagram_button, Qt::TopRightCorner);
	
	connect(tabs_, SIGNAL(currentChanged(int)),   this, SLOT(tabChanged(int)));
	connect(tabs_, SIGNAL(tabDoubleClicked(int)), this, SLOT(tabDoubleClicked(int)));
	connect(tabs_, SIGNAL(firstTabInserted()),    this, SLOT(firstTabInserted()));
	connect(tabs_, SIGNAL(lastTabRemoved()),      this, SLOT(lastTabRemoved()));
	connect(tabs_, SIGNAL(tabMoved(int, int)),    this, SLOT(tabMoved(int, int)));
	
	fallback_widget_ -> setVisible(false);
	tabs_ -> setVisible(false);
}

/**
	Initialize layout for this widget.
*/
void ProjectView::initLayout() {
	QVBoxLayout *fallback_widget_layout_ = new QVBoxLayout(fallback_widget_);
	fallback_widget_layout_ -> addWidget(fallback_label_);
	
	layout_ = new QVBoxLayout(this);
#ifdef Q_WS_MAC
	layout_ -> setContentsMargins(0, 8, 0, 0);
#else
	layout_ -> setContentsMargins(0, 0, 0, 0);
#endif
	layout_ -> setSpacing(0);
	layout_ -> addWidget(fallback_widget_);
	layout_ -> addWidget(tabs_);
}

/**
	Charge les schemas du projet
*/
void ProjectView::loadDiagrams() {
	if (!project_) return;
	
	setDisplayFallbackWidget(project_ -> diagrams().isEmpty());
	
	foreach(Diagram *diagram, project_ -> diagrams()) {
		DiagramView *sv = new DiagramView(diagram);
		addDiagram(sv);
	}
}

/**
	Met a jour le titre du ProjectView
*/
void ProjectView::updateWindowTitle() {
	QString title;
	if (project_) {
		title = project_ -> pathNameTitle();
	} else {
		title = tr("Projet", "window title for a project-less ProjectView");
	}
	setWindowTitle(title);
}

/**
	Effectue les actions necessaires lorsque le projet visualise entre ou sort
	du mode lecture seule.
*/
void ProjectView::adjustReadOnlyState() {
	bool editable = !(project_ -> isReadOnly());
	
	// prevent users from moving existing diagrams
	tabs_ -> setMovable(editable);
	// prevent users from adding new diagrams
	add_new_diagram_ -> setEnabled(editable);
	
	// on met a jour le titre du widget, qui reflete l'etat de lecture seule
	updateWindowTitle();
}

/**
	Met a jour le titre d'un onglet
	@param diagram Schema
	@param diagram_title Titre du schema
*/
void ProjectView::updateTabTitle(DiagramView *diagram, const QString &diagram_title) {
	int diagram_tab_id = diagram_ids_.key(diagram, -1);
	if (diagram_tab_id != -1) {
		tabs_ -> setTabText(diagram_tab_id, diagram_title);
	}
}

/**
	@param from Index de l'onglet avant le deplacement
	@param to   Index de l'onglet apres le deplacement
*/
void ProjectView::tabMoved(int from, int to) {
	if (!project_) return;
	
	// signale au QETProject le changement d'ordre des schemas
	project_ -> diagramOrderChanged(from, to);
	
	// reconstruit la liste associant les index des onglets aux schemas
	rebuildDiagramsMap();
	
	// emet un signal pour informer le reste du monde que l'ordre des schemas a change
	emit(diagramOrderChanged(this, from, to));
}

/**
	Require the edition of the \a template_name title blocke template.
	@param template_name Name of the tempalte to be edited
	@param duplicate If true, this methd will ask the user for a template name
	in order to duplicate the \a template_name template
*/
void ProjectView::editTitleBlockTemplateRequired(const QString &template_name, bool duplicate) {
	if (!project_) return;
	emit(
		editTitleBlockTemplate(
			project_ -> embeddedTitleBlockTemplatesCollection() -> location(template_name),
			duplicate
		)
	);
}

/**
	@param diagram Schema a trouver
	@return le DiagramView correspondant au schema passe en parametre, ou 0 si
	le schema n'est pas trouve
*/
DiagramView *ProjectView::findDiagram(Diagram *diagram) {
	foreach(DiagramView *diagram_view, diagrams()) {
		if (diagram_view -> diagram() == diagram) {
			return(diagram_view);
		}
	}
	return(0);
}

/**
	Reconstruit la map associant les index des onglets avec les DiagramView
*/
void ProjectView::rebuildDiagramsMap() {
	// vide la map
	diagram_ids_.clear();
	
	foreach(DiagramView *diagram_view, diagrams_) {
		int dv_idx = tabs_ -> indexOf(diagram_view);
		if (dv_idx == -1) continue;
		diagram_ids_.insert(dv_idx, diagram_view);
	}
}

/**
	Gere les changements d'onglets
	@param tab_id Index de l'onglet actif
*/
void ProjectView::tabChanged(int tab_id) {
	emit(diagramActivated(diagram_ids_[tab_id]));
}

/**
	Gere le double-clic sur un onglet : edite les proprietes du schema
	@param tab_id Index de l'onglet concerne
*/
void ProjectView::tabDoubleClicked(int tab_id) {
	// repere le schema concerne
	DiagramView *diagram_view = diagram_ids_[tab_id];
	if (!diagram_view) return;
	
	diagram_view -> editDiagramProperties();
}

/**
	Gere le fait que le premier schema d'un projet soit insere
*/
void ProjectView::firstTabInserted() {
	setDisplayFallbackWidget(false);
}

/**
	Gere le fait que le dernier schema d'un projet soit enleve
*/
void ProjectView::lastTabRemoved() {
	setDisplayFallbackWidget(true);
}

/**
	@param fallback true pour afficher le widget de fallback, false pour
	afficher les onglets.
	Le widget de Fallback est le widget affiche lorsque le projet ne comporte
	aucun schema.
*/
void ProjectView::setDisplayFallbackWidget(bool fallback) {
	fallback_widget_ -> setVisible(fallback);
	tabs_ -> setVisible(!fallback);
}
