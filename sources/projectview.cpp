/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "diagramview.h"
#include "diagram.h"
#include "diagramprintdialog.h"
#include "exportdialog.h"
#include "qetapp.h"
#include "qetelementeditor.h"
#include "borderpropertieswidget.h"
#include "titleblockpropertieswidget.h"
#include "conductorpropertieswidget.h"
#include "qeticons.h"
#include "qetmessagebox.h"
#include "qettemplateeditor.h"
#include "diagramfoliolist.h"
#include "projectpropertiesdialog.h"
#include "xmlelementcollection.h"
#include "autoNum/assignvariables.h"

/**
	Constructeur
	@param project projet a visualiser
	@param parent Widget parent
*/
ProjectView::ProjectView(QETProject *project, QWidget *parent) :
	QWidget(parent),
	m_project(0)
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
	foreach(int id, m_diagram_ids.keys()) {
		DiagramView *diagram_view = m_diagram_ids.take(id);
		delete diagram_view;
	}
}

/**
	@return le projet actuellement visualise par le ProjectView
*/
QETProject *ProjectView::project() {
	return(m_project);
}

/**
	Definit le projet visualise par le ProjectView. Ne fait rien si le projet a
	deja ete defini.
	@param project projet a visualiser
*/
void ProjectView::setProject(QETProject *project) {
	if (!m_project) {
		m_project = project;
		connect(m_project, SIGNAL(projectTitleChanged(QETProject *, const QString &)),	this, SLOT(updateWindowTitle()));
		connect(m_project, SIGNAL(projectModified	(QETProject *, bool)),				this, SLOT(updateWindowTitle()));
		connect(m_project, SIGNAL(readOnlyChanged	(QETProject *, bool)),				this, SLOT(adjustReadOnlyState()));
		connect(m_project, SIGNAL(addAutoNumDiagram()),									this, SLOT(addNewDiagram()));
		adjustReadOnlyState();
		loadDiagrams();
	}
}

/**
	@return la liste des schemas ouverts dans le projet
*/
QList<DiagramView *> ProjectView::diagram_views() const {
	return(m_diagram_view_list);
}

/**
 * @brief ProjectView::currentDiagram
 * @return The current active diagram view or nullptr if there isn't diagramView in this project view.
 */
DiagramView *ProjectView::currentDiagram() const {
	int current_tab_index = m_tab -> currentIndex();
	if (current_tab_index == -1)
		return nullptr;
	return(m_diagram_ids[current_tab_index]);
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
	@brief change current diagramview to next folio
*/
void ProjectView::changeTabDown(){
	DiagramView *nextDiagramView = this->nextDiagram();
	if (nextDiagramView!=NULL){
		rebuildDiagramsMap();
		m_tab -> setCurrentWidget(nextDiagramView);
	}
}

/**
	@return next folio of current diagramview
*/
DiagramView *ProjectView::nextDiagram() {
	int current_tab_index = m_tab -> currentIndex();
	int next_tab_index = current_tab_index + 1;	//get next tab index
	if (next_tab_index<m_diagram_ids.count()) //if next tab index >= greatest tab the last tab is activated so no need to change tab.
		return(m_diagram_ids[next_tab_index]);
	else
		return NULL;
}

/**
	@brief change current diagramview to previous tab
*/
void ProjectView::changeTabUp(){
	DiagramView *previousDiagramView = this->previousDiagram();
	if (previousDiagramView!=NULL){
		rebuildDiagramsMap();
		m_tab -> setCurrentWidget(previousDiagramView);
	}
}

/**
	@return previous folio of current diagramview
*/
DiagramView *ProjectView::previousDiagram() {
	int current_tab_index = m_tab -> currentIndex();
	int previous_tab_index = current_tab_index - 1;	//get previous tab index
	if (previous_tab_index>=0) //if previous tab index = 0 then the first tab is activated so no need to change tab.
		return(m_diagram_ids[previous_tab_index]);
	else
		return NULL;
}

/**
	@brief change current diagramview to last tab
*/
void ProjectView::changeLastTab(){
	DiagramView *lastDiagramView = this->lastDiagram();
	m_tab->setCurrentWidget(lastDiagramView);
}

/**
	@return last folio of current project
*/
DiagramView *ProjectView::lastDiagram(){
	return(m_diagram_ids.last());
}

/**
	@brief change current diagramview to first tab
*/
void ProjectView::changeFirstTab(){
	DiagramView *firstDiagramView = this->firstDiagram();
	m_tab->setCurrentWidget(firstDiagramView);
}

/**
	@return first folio of current project
*/
DiagramView *ProjectView::firstDiagram(){
	return(m_diagram_ids.first());
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
	if (!m_project) return(true);

	// First step: require external editors closing -- users may either cancel
	// the whole closing process or save (and therefore add) content into this
	// project. Of course, they may also discard them.
	if (!tryClosingElementEditors()) {
		return(false);
	}

	// Check how different the current situation is from a brand new, untouched project
	if (m_project -> filePath().isEmpty() && !m_project -> projectWasModified()) {
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
	if (m_project -> filePath().isEmpty() && !m_project -> projectWasModified()) {
		return(true);
	}

	if (m_project -> filePath().isEmpty()) {
		QString filepath = askUserForFilePath();
		if (filepath.isEmpty()) return(false); // users may cancel the closing
	}
	QETResult result = m_project -> write();
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
	if (!m_project) return(true);
	/*
		La QETApp permet d'acceder rapidement aux editeurs d'element
		editant un element du projet.
	*/
	QList<QETElementEditor *> editors = QETApp::elementEditors(m_project);
	foreach(QETElementEditor *editor, editors) {
		if (!editor -> close()) return(false);
	}

	QList<QETTitleBlockTemplateEditor *> template_editors = QETApp::titleBlockTemplateEditors(m_project);
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
	if (!m_project) return(QMessageBox::Discard);

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
								   tr("Le projet à été modifié.\n"
									  "Voulez-vous enregistrer les modifications ?"),
								   QMessageBox::Save | QMessageBox::Discard
								   | QMessageBox::Cancel,
								   QMessageBox::Save);

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
		m_project -> currentDir(),
		tr("Projet QElectroTech (*.qet)", "filetypes allowed when saving a project file")
	);

	// if no filepath is provided, return an empty string
	if (filepath.isEmpty()) return(filepath);

	// if the name does not end with the .qet extension, append it
	if (!filepath.endsWith(".qet", Qt::CaseInsensitive)) filepath += ".qet";

	if (assign) {
		// assign the provided filepath to the currently edited project
		m_project -> setFilePath(filepath);
	}

	return(filepath);
}

/**
	@return the QETResult object to be returned when it appears this project
	view is not associated to any project.
*/
QETResult ProjectView::noProjectResult() const {
	QETResult no_project(tr("aucun projet affiché", "error message"), false);
	return(no_project);
}

/**
 * @brief ProjectView::addNewDiagram
 * Add new diagram to project view
 */
void ProjectView::addNewDiagram() {
	if (m_project -> isReadOnly()) return;

	Diagram *new_diagram = m_project -> addNewDiagram();
	DiagramView *new_diagram_view = new DiagramView(new_diagram);
	addDiagram(new_diagram_view);

	if (m_project -> diagrams().size() % 58 == 1 && m_project -> getFolioSheetsQuantity() != 0)
		addNewDiagramFolioList();
	showDiagram(new_diagram_view);
}

/**
 * @brief ProjectView::addNewDiagramFolioList
 * Add new diagram folio list to project
 */
void ProjectView::addNewDiagramFolioList() {
	if (m_project -> isReadOnly()) return;
	int i = 1; //< Each new diagram is added  to the end of the project.
			   //< We use @i to move the folio list at second position in the project
	foreach (Diagram *d, m_project -> addNewDiagramFolioList()) {
		DiagramView *new_diagram_view = new DiagramView(d);
		addDiagram(new_diagram_view);
		showDiagram(new_diagram_view);
		m_tab->tabBar()->moveTab(diagram_views().size()-1, i);
		i++;
	}
}

/**
 * @brief ProjectView::addDiagram
 * Add diagram view to this project view
 * @param diagram_view
 */
void ProjectView::addDiagram(DiagramView *diagram_view)
{
	if (!diagram_view) 
		return;

		//Check if diagram isn't present in the project
	if (m_diagram_ids.values().contains(diagram_view))
		return;

		// Add new tab for the diagram
	m_tab->addTab(diagram_view, QET::Icons::Diagram, diagram_view -> title());
	diagram_view->setFrameStyle(QFrame::Plain | QFrame::NoFrame);

	m_diagram_view_list << diagram_view;

	rebuildDiagramsMap();
	updateTabTitle(diagram_view);
	
	connect(diagram_view, SIGNAL(showDiagram(Diagram*)), this, SLOT(showDiagram(Diagram*)));
	connect(diagram_view, SIGNAL(titleChanged(DiagramView *, const QString &)), this, SLOT(updateTabTitle(DiagramView *)));
	connect(diagram_view, SIGNAL(findElementRequired(const ElementsLocation &)), this, SIGNAL(findElementRequired(const ElementsLocation &)));
	connect(diagram_view, SIGNAL(editElementRequired(const ElementsLocation &)), this, SIGNAL(editElementRequired(const ElementsLocation &)));
	connect(&diagram_view->diagram()->border_and_titleblock , &BorderTitleBlock::titleBlockFolioChanged, [this, diagram_view]() {this->updateTabTitle(diagram_view);});

		// signal diagram view was added
	emit(diagramAdded(diagram_view));
}

/**
	Enleve un schema du ProjectView
	@param diagram_view Schema a enlever
*/
void ProjectView::removeDiagram(DiagramView *diagram_view) {
	if (!diagram_view) return;
	if (m_project -> isReadOnly()) return;

	// verifie que le schema est bien present dans le projet
	if (!m_diagram_ids.values().contains(diagram_view)) return;


	//Ask confirmation to user.
	int answer = QET::QetMessageBox::question(
		this,
		tr("Supprimer le folio ?", "message box title"),
		tr("Êtes-vous sûr  de vouloir supprimer ce folio du projet ? Ce changement est irréversible.", "message box content"),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
		QMessageBox::No
	);
	if (answer != QMessageBox::Yes) {
		return;
	}

	// enleve le DiagramView des onglets
	int diagram_tab_id = m_diagram_ids.key(diagram_view);
	m_tab -> removeTab(diagram_tab_id);
	m_diagram_view_list.removeAll(diagram_view);
	rebuildDiagramsMap();

	// supprime le DiagramView, puis le Diagram
	m_project -> removeDiagram(diagram_view -> diagram());
	delete diagram_view;

	// signale le retrait du schema
	emit(diagramRemoved(diagram_view));

	// rend definitif le retrait du schema
	m_project -> write();
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
	m_tab -> setCurrentWidget(diagram);
}

/**
	Active l'onglet adequat pour afficher le schema passe en parametre
	@param diagram Schema a afficher
*/
void ProjectView::showDiagram(Diagram *diagram) {
	if (!diagram) return;
	if (DiagramView *diagram_view = findDiagram(diagram)) {
		m_tab -> setCurrentWidget(diagram_view);
	}
}

/**
	Enable the user to edit properties of the current project through a
	configuration dialog.
*/
void ProjectView::editProjectProperties() {
	if (!m_project) return;
	ProjectPropertiesDialog dialog(m_project, parentWidget());
	dialog.exec();
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

	int diagram_view_position = m_diagram_ids.key(diagram_view);
	if (!diagram_view_position) {
		// le schema est le premier du projet
		return;
	}
	m_tab -> tabBar() -> moveTab(diagram_view_position, diagram_view_position - 1);
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

	int diagram_view_position = m_diagram_ids.key(diagram_view);
	if (diagram_view_position + 1 == m_diagram_ids.count()) {
		// le schema est le dernier du projet
		return;
	}
	m_tab -> tabBar() -> moveTab(diagram_view_position, diagram_view_position + 1);
}

/**
	Deplace le schema diagram vers le bas / la droite
*/
void ProjectView::moveDiagramDown(Diagram *diagram) {
	moveDiagramDown(findDiagram(diagram));
}

/*
 * Deplace le schema diagram_view vers le haut / la gauche en position 0
 */
void ProjectView::moveDiagramUpTop(DiagramView *diagram_view)
{
	if (!diagram_view) return;

	int diagram_view_position = m_diagram_ids.key(diagram_view);
	if (!diagram_view_position) {
		// le schema est le premier du projet
		return;
	}
	m_tab -> tabBar() -> moveTab(diagram_view_position, (diagram_views().size(), 0));
}

/*
 * Deplace le schema diagram vers le haut / la gauche en position 0
 */
void ProjectView::moveDiagramUpTop(Diagram *diagram)
{
	moveDiagramUpTop(findDiagram(diagram));
}

/**
	Deplace le schema diagram_view vers le haut / la gauche x10
*/
void ProjectView::moveDiagramUpx10(DiagramView *diagram_view) {
	if (!diagram_view) return;

	int diagram_view_position = m_diagram_ids.key(diagram_view);
	if (!diagram_view_position) {
		// le schema est le premier du projet
		return;
	}
	m_tab -> tabBar() -> moveTab(diagram_view_position, diagram_view_position - 10);
}

/**
	Deplace le schema diagram vers le haut / la gauche x10
*/
void ProjectView::moveDiagramUpx10(Diagram *diagram) {
	moveDiagramUpx10(findDiagram(diagram));
}

/**
	Deplace le schema diagram_view vers le bas / la droite x10
*/
void ProjectView::moveDiagramDownx10(DiagramView *diagram_view) {
	if (!diagram_view) return;

	int diagram_view_position = m_diagram_ids.key(diagram_view);
	if (diagram_view_position + 1 == m_diagram_ids.count()) {
		// le schema est le dernier du projet
		return;
	}
	m_tab -> tabBar() -> moveTab(diagram_view_position, diagram_view_position + 10);
}

/**
	Deplace le schema diagram vers le bas / la droite x10
*/
void ProjectView::moveDiagramDownx10(Diagram *diagram) {
	moveDiagramDownx10(findDiagram(diagram));
}

/**
	Ce slot demarre un dialogue permettant a l'utilisateur de parametrer et de
	lancer l'impression de toute ou partie du projet.
*/
void ProjectView::printProject() {
	if (!m_project) return;

	// transforme le titre du projet en nom utilisable pour le document
	QString doc_name;
	if (!(m_project -> title().isEmpty())) {
		doc_name = m_project -> title();
	} else if (!m_project -> filePath().isEmpty()) {
		doc_name = QFileInfo(m_project -> filePath()).baseName();
	}
	doc_name = QET::stringToFileName(doc_name);
	if (doc_name.isEmpty()) {
		doc_name = tr("projet", "string used to generate a filename");
	}

	// recupere le dossier contenant le fichier courant
	QString dir_path = m_project -> currentDir();

	// determine un chemin pour le pdf / ps
	QString file_name = QDir::toNativeSeparators(QDir::cleanPath(dir_path + "/" + doc_name));

	DiagramPrintDialog print_dialog(m_project, this);
	print_dialog.setDocName(doc_name);
	print_dialog.setFileName(file_name);
	print_dialog.exec();
}

/**
	Exporte le schema.
*/
void ProjectView::exportProject() {
	if (!m_project) return;

	ExportDialog ed(m_project, parentWidget());
#ifdef Q_OS_MAC
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
	return(doSave());
}

/**
	Ask users for a filepath in order to save the project.
	@param options May be used to specify what should be saved; defaults to
	all modified diagrams.
	@return a QETResult object reflecting the situation; note that a valid
	QETResult object is returned if the operation was cancelled.
*/
QETResult ProjectView::saveAs()
{
	if (!m_project) return(noProjectResult());

	QString filepath = askUserForFilePath();
	if (filepath.isEmpty()) return(QETResult());
	return(doSave());
}

/**
	Save project content, then write the project file. May
	call saveAs if no filepath was provided before.
	@return a QETResult object reflecting the situation; note that a valid
	QETResult object is returned if the operation was cancelled.
*/
QETResult ProjectView::doSave()
{
	if (!m_project) return(noProjectResult());

	if (m_project -> filePath().isEmpty()) {
		// The project has not been saved to a file yet,
		// so save() actually means saveAs().
		return(saveAs());
	}

	// write to file
	QETResult result = m_project -> write();
	updateWindowTitle();
	project()->undoStack()->clear();
	return(result);
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
	if (!m_project) return(0);

	// s'assure que le schema n'est pas en lecture seule
	if (m_project -> isReadOnly()) {
		QET::QetMessageBox::critical(
			this,
			tr("Projet en lecture seule", "message box title"),
			tr("Ce projet est en lecture seule. Il n'est donc pas possible de le nettoyer.", "message box content")
		);
		return(0);
	}

	// construit un petit dialogue pour parametrer le nettoyage
	QCheckBox *clean_tbt		= new QCheckBox(tr("Supprimer les modèles de cartouche inutilisés dans le projet"));
	QCheckBox *clean_elements   = new QCheckBox(tr("Supprimer les éléments inutilisés dans le projet"));
	QCheckBox *clean_categories = new QCheckBox(tr("Supprimer les catégories vides"));
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	clean_tbt		-> setChecked(true);
	clean_elements   -> setChecked(true);
	clean_categories -> setChecked(true);

	QDialog clean_dialog(parentWidget());
#ifdef Q_OS_MAC
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
	if (clean_dialog.exec() == QDialog::Accepted)
	{
		if (clean_tbt -> isChecked()) {
			m_project->embeddedTitleBlockTemplatesCollection()->deleteUnusedTitleBlocKTemplates();
		}
		if (clean_elements->isChecked()) {
			m_project->embeddedElementCollection()->cleanUnusedElement();
		}
		if (clean_categories->isChecked()) {
			m_project->embeddedElementCollection()->cleanUnusedDirectory();
		}
	}

	return(clean_count);
}

/**
	Initialize actions for this widget.
*/
void ProjectView::initActions() {
	add_new_diagram_ = new QAction(QET::Icons::AddFolio, tr("Ajouter un folio"), this);
	connect(add_new_diagram_, SIGNAL(triggered()), this, SLOT(addNewDiagram()));
}

/**
	Initialize child widgets for this widget.
*/
void ProjectView::initWidgets() {
	setObjectName("ProjectView");
	setWindowIcon(QET::Icons::ProjectFileGP);

	// initialize the "fallback" widget
	fallback_widget_ = new QWidget();
	fallback_label_ = new QLabel(
		tr(
			"Ce projet ne contient aucun folio",
			"label displayed when a project contains no diagram"
		)
	);
	fallback_label_ -> setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

	// initialize tabs
	m_tab = new QTabWidget(this);
	m_tab -> setMovable(true);

	QToolButton *add_new_diagram_button = new QToolButton;
	add_new_diagram_button -> setDefaultAction(add_new_diagram_);
	add_new_diagram_button -> setAutoRaise(true);
	m_tab -> setCornerWidget(add_new_diagram_button, Qt::TopRightCorner);

	connect(m_tab, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
	connect(m_tab, SIGNAL(tabBarDoubleClicked(int)), this, SLOT(tabDoubleClicked(int)));
	connect(m_tab->tabBar(), SIGNAL(tabMoved(int, int)), this, SLOT(tabMoved(int, int)));

	fallback_widget_ -> setVisible(false);
	m_tab -> setVisible(false);
}

/**
	Initialize layout for this widget.
*/
void ProjectView::initLayout() {
	QVBoxLayout *fallback_widget_layout_ = new QVBoxLayout(fallback_widget_);
	fallback_widget_layout_ -> addWidget(fallback_label_);

	layout_ = new QVBoxLayout(this);
#ifdef Q_OS_MAC
	layout_ -> setContentsMargins(0, 8, 0, 0);
#else
	layout_ -> setContentsMargins(0, 0, 0, 0);
#endif
	layout_ -> setSpacing(0);
	layout_ -> addWidget(fallback_widget_);
	layout_ -> addWidget(m_tab);
}


/**
 * @brief ProjectView::loadDiagrams
 * Load diagrams of project.
 * We create a diagram view for each diagram,
 * and add it to the project view.
 */
void ProjectView::loadDiagrams() {
	if (!m_project) return;

	setDisplayFallbackWidget(m_project -> diagrams().isEmpty());

	foreach(Diagram *diagram, m_project -> diagrams()) {
		DiagramView *sv = new DiagramView(diagram);
		addDiagram(sv);
	}

	this->currentDiagram()->diagram()->loadElmtFolioSeq();
	this->currentDiagram()->diagram()->loadCndFolioSeq();

	// If project have the folios list, move it at the beginning of the project
	if (m_project -> getFolioSheetsQuantity()) {
		for (int i = 0; i < m_project->getFolioSheetsQuantity(); i++)
		m_tab -> tabBar() -> moveTab(diagram_views().size()-1, + 1);
	}
}

/**
 * @brief ProjectView::updateWindowTitle
 * Update the project view title
 */
void ProjectView::updateWindowTitle() {
	QString title;
	if (m_project) {
		title = m_project -> pathNameTitle();
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
	bool editable = !(m_project -> isReadOnly());

	// prevent users from moving existing diagrams
	m_tab -> setMovable(editable);
	// prevent users from adding new diagrams
	add_new_diagram_ -> setEnabled(editable);

	// on met a jour le titre du widget, qui reflete l'etat de lecture seule
	updateWindowTitle();
}

/**
 * @brief ProjectView::updateTabTitle
 * Update the title of the tab which display the diagram view @diagram_view.
 * @param diagram : The diagram view.
 */
void ProjectView::updateTabTitle(DiagramView *diagram_view)
{
	int diagram_tab_id = m_diagram_ids.key(diagram_view, -1);
	
	if (diagram_tab_id != -1)
	{
		QSettings settings;
		QString title;
		Diagram *diagram = diagram_view->diagram();
		
		if (settings.value("genericpanel/folio", false).toBool())
		{
			QString formula = diagram->border_and_titleblock.folio();
			autonum::sequentialNumbers seq;
			title = autonum::AssignVariables::formulaToLabel(formula, seq, diagram);
		}
		else
			title = QString::number(diagram->folioIndex() + 1);
		
		title += " - ";
		title += diagram->title();
		m_tab->setTabText(diagram_tab_id ,title);
	}
}

/**
 * @brief ProjectView::updateAllTabsTitle
 * Update all tabs title
 */
void ProjectView::updateAllTabsTitle()
{
	for (DiagramView *dv : m_diagram_ids.values())
		updateTabTitle(dv);
}

/**
	@param from Index de l'onglet avant le deplacement
	@param to   Index de l'onglet apres le deplacement
*/
void ProjectView::tabMoved(int from, int to)
{
	if (!m_project)
		return;
	
	m_project->diagramOrderChanged(from, to);
	rebuildDiagramsMap();
	
		//Rebuild the title of each diagram in range from - to
	for (int i= qMin(from,to) ; i< qMax(from,to)+1 ; ++i)
	{
		DiagramView *dv = m_diagram_ids.value(i);
		updateTabTitle(dv);
	}
}

/**
	@param diagram Schema a trouver
	@return le DiagramView correspondant au schema passe en parametre, ou 0 si
	le schema n'est pas trouve
*/
DiagramView *ProjectView::findDiagram(Diagram *diagram) {
	foreach(DiagramView *diagram_view, diagram_views()) {
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
	m_diagram_ids.clear();

	foreach(DiagramView *diagram_view, m_diagram_view_list) {
		int dv_idx = m_tab -> indexOf(diagram_view);
		if (dv_idx == -1) continue;
		m_diagram_ids.insert(dv_idx, diagram_view);
	}
}

/**
 * @brief ProjectView::tabChanged
 * Manage the tab change.
 * If tab_id == -1 (there is no diagram opened),
 * we display the fallback widget.
 * @param tab_id
 */
void ProjectView::tabChanged(int tab_id) {
	if (tab_id == -1)
		setDisplayFallbackWidget(true);
	else if(m_tab->count() == 1)
		setDisplayFallbackWidget(false);

	emit(diagramActivated(m_diagram_ids[tab_id]));
	if (m_diagram_ids[tab_id] != nullptr)
		m_diagram_ids[tab_id]->diagram()->diagramActivated();
}

/**
	Gere le double-clic sur un onglet : edite les proprietes du schema
	@param tab_id Index de l'onglet concerne
*/
void ProjectView::tabDoubleClicked(int tab_id) {
	// repere le schema concerne
	DiagramView *diagram_view = m_diagram_ids[tab_id];
	if (!diagram_view) return;

	diagram_view -> editDiagramProperties();
}

/**
	@param fallback true pour afficher le widget de fallback, false pour
	afficher les onglets.
	Le widget de Fallback est le widget affiche lorsque le projet ne comporte
	aucun schema.
*/
void ProjectView::setDisplayFallbackWidget(bool fallback) {
	fallback_widget_ -> setVisible(fallback);
	m_tab -> setVisible(!fallback);
}
