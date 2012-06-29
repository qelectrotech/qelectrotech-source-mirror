/*
	Copyright 2006-2012 Xavier Guerrin
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

/**
	Constructeur
	@param project projet a visualiser
	@param parent Widget parent
*/
ProjectView::ProjectView(QETProject *project, QWidget *parent) :
	QWidget(parent),
	project_(0)
{
	setObjectName("ProjectView");
	setWindowIcon(QET::Icons::ProjectFile);
	
	// construit le widget "fallback"
	fallback_widget_ = new QWidget();
	QVBoxLayout *fallback_widget_layout_ = new QVBoxLayout(fallback_widget_);
	QLabel *label_widget = new QLabel(tr("Ce projet ne contient aucun sch\351ma"));
	label_widget -> setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	fallback_widget_layout_ -> addWidget(label_widget);
	
	tabs_ = new QETTabWidget();
	tabs_ -> setMovable(true);
	connect(tabs_, SIGNAL(currentChanged(int)),   this, SLOT(tabChanged(int)));
	connect(tabs_, SIGNAL(tabDoubleClicked(int)), this, SLOT(tabDoubleClicked(int)));
	connect(tabs_, SIGNAL(firstTabInserted()),    this, SLOT(firstTabInserted()));
	connect(tabs_, SIGNAL(lastTabRemoved()),      this, SLOT(lastTabRemoved()));
	connect(tabs_, SIGNAL(tabMoved(int, int)),    this, SLOT(tabMoved(int, int)));
	
	layout_ = new QVBoxLayout(this);
#ifdef Q_WS_MAC
	layout_ -> setContentsMargins(0, 8, 0, 0);
#else
	layout_ -> setContentsMargins(0, 0, 0, 0);
#endif
	layout_ -> setSpacing(0);
	layout_ -> addWidget(fallback_widget_);
	layout_ -> addWidget(tabs_);
	
	fallback_widget_ -> setVisible(false);
	tabs_ -> setVisible(false);
	
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
	// si la vue n'est pas liee a un projet, on ferme directement
	if (!project_) {
		qce -> accept();
		emit(projectClosed(this));
		return;
	}
	
	// si le projet est comme neuf et n'est pas enregistre, on ferme directement
	if (!project_ -> projectWasModified() && project_ -> filePath().isEmpty()) {
		qce -> accept();
		emit(projectClosed(this));
		return;
	}
	
	bool can_close_project = true;
	if (!tryClosing()) {
		// l'utilisateur a refuse la fermeture du projet - on arrete la
		can_close_project = false;
	} else {
		// a ce stade, l'utilisateur a accepte la fermeture de tout le contenu du projet
		if (!project_ -> filePath().isEmpty()) {
			// si le projet a un chemin specifie, on l'enregistre et on le ferme
			can_close_project = project_ -> write();
		} else {
			// l'utilisateur n'enregistre pas son projet
			can_close_project = true;
		}
	}
	
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
	if (!tryClosingElementEditors()) {
		return(false);
	}
	
	if (!tryClosingDiagrams()) {
		return(false);
	}
	
	// a ce stade, l'utilisateur a accepte de fermer tous les editeurs
	// d'elements et tous les schemas
	// on regarde s'il reste du contenu dans le projet
	if (project_ -> projectWasModified() && project_ -> filePath().isEmpty()) {
		// si oui, on propose a l'utilisateur d'enregistrer le projet
		QMessageBox::StandardButton answer = QET::MessageBox::question(
			this,
			tr("Enregistrer le projet en cours ?", "message box title"),
			QString(tr("Voulez-vous enregistrer le projet ?", "message box content")),
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
			QMessageBox::Cancel
		);
		if (answer == QMessageBox::Cancel) {
			return(false);
		} else if (answer == QMessageBox::Yes) {
			return(save());
		}
	}
	
	return(true);
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
	Un projet comporte 0 a n schemas.
	Cette methode parcourt les schemas et demande a l'utilisateur s'il veut
	enregistrer les schemas modifies afin de les fermer. L'utilisateur peut
	refuser la fermeture d'un schema.
	Si un schema a ete ajoute sans jamais etre modifie, cette methode demande a
	l'utilisateur s'il souhaite l'enlever.
	@return true si tous les schemas peuvent etre fermes, false sinon
*/
bool ProjectView::tryClosingDiagrams() {
	if (!project_) return(true);
	
	foreach(DiagramView *diagram_view, diagrams()) {
		if (!diagram_view -> diagram() -> undoStack().isClean()) {
			// ce schema a ete modifie - on demande a l'utilisateur s'il veut l'enregistrer
			showDiagram(diagram_view -> diagram());
			QMessageBox::StandardButton answer = QET::MessageBox::question(
				this,
				tr("Enregistrer le sch\351ma en cours ?", "message box title"),
				QString(tr("Voulez-vous enregistrer le sch\351ma %1 ?", "message box content - %1 is a diagram title")).arg(diagram_view -> windowTitle()),
				QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
				QMessageBox::Cancel
			);
			if (answer == QMessageBox::Cancel) {
				return(false);
			} else if (answer == QMessageBox::Yes) {
				if (!save()) {
					return(false);
				}
			}
		} else if (!diagram_view -> diagram() -> wasWritten()) {
			// ce schema a ete ajoute mais pas modifie - on demande a l'utilisateur s'il veut le conserver
			showDiagram(diagram_view -> diagram());
			QMessageBox::StandardButton answer = QET::MessageBox::question(
				this,
				tr("Enregistrer le nouveau sch\351ma ?", "message box title"),
				tr("Ce sch\351ma a \351t\351 ajout\351 mais n'a \351t\351 ni modifi\351 ni enregistr\351. Voulez-vous le conserver ?", "message box content"),
				QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
				QMessageBox::Cancel
			);
			if (answer == QMessageBox::Cancel) {
				return(false);
			} else if (answer == QMessageBox::Yes) {
				if (!save()) {
					return(false);
				}
			} else if (answer == QMessageBox::No) {
				removeDiagram(diagram_view);
			}
		}
	}
	return(true);
}

/**
	Ajoute un nouveau schema au ProjectView
*/
void ProjectView::addNewDiagram() {
	if (project_ -> isReadOnly()) return;
	
	Diagram *new_diagram = project_ -> addNewDiagram();
	DiagramView *new_diagram_view = new DiagramView(new_diagram);
	addDiagram(new_diagram_view);
	showDiagram(new_diagram_view);
}

/**
	Ajoute un schema au ProjectView
	@param diagram Schema a ajouter
*/
void ProjectView::addDiagram(DiagramView *diagram) {
	if (!diagram) return;
	
	// verifie que le schema n'est pas deja present dans le projet
	if (diagram_ids_.values().contains(diagram)) return;
	
	// ajoute un nouvel onglet pour le nouveau schema
	tabs_ -> addTab(diagram, QET::Icons::Diagram, diagram -> title());
	diagrams_ << diagram;
	rebuildDiagramsMap();
	connect(diagram, SIGNAL(titleChanged(DiagramView *, const QString &)), this, SLOT(updateTabTitle(DiagramView *, const QString &)));
	connect(diagram, SIGNAL(findElementRequired(const ElementsLocation &)), this, SIGNAL(findElementRequired(const ElementsLocation &)));
	connect(diagram, SIGNAL(editElementRequired(const ElementsLocation &)), this, SIGNAL(editElementRequired(const ElementsLocation &)));
	connect(diagram, SIGNAL(editTitleBlockTemplate(const QString &, bool)), this, SLOT(editTitleBlockTemplateRequired(const QString &, bool)));
	
	// signale l'ajout du schema
	emit(diagramAdded(diagram));
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
	properties_dialog.setMinimumSize(786, 585);
	properties_dialog.setWindowTitle(tr("Propri\351t\351s du projet", "window title"));
	properties_dialog.addPage(new ProjectMainConfigPage(project_));
	properties_dialog.addPage(new ProjectNewDiagramConfigPage(project_));
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
	tabs_ -> tabBar() -> moveTab(diagram_view_position, diagram_view_position - 1);
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
	tabs_ -> tabBar() -> moveTab(diagram_view_position, diagram_view_position + 1);
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
	Enregistre le projet dans un fichier.
	@see filePath()
	@see setFilePath()
	@return true si l'enregistrement a reussi, false sinon
*/
bool ProjectView::save() {
	if (project_) {
		if (project_ -> filePath().isEmpty()) {
			// le projet n'est pas encore enregistre dans un fichier
			// save() equivaut alors a saveAs()
			return(saveAs());
		}
		// on enregistre le schema en cours
		if (DiagramView *current_view = currentDiagram()) {
			if (Diagram *diagram = current_view -> diagram()) {
				diagram -> write();
				updateWindowTitle();
				return(true);
			}
		} else {
			// s'il n'y a pas de schema, on appelle directement la methode write()
			project_ -> write();
		}
		return(true);
	}
	return(false);
}

/**
	Enregistre tous les schemas du projet.
	@see filePath()
	@see setFilePath()
	@return true si l'enregistrement a reussi, false sinon
*/
bool ProjectView::saveAll() {
	if (project_) {
		// on fait deja un appel a save
		if (!save()) {
			return(false);
		} else {
			// a ce stade, on suppose que l'on a un fichier, et que l'ecriture du schema en cours a reussi
			// on enregistre les schemas
			foreach(Diagram *diagram, project_ -> diagrams()) {
				diagram -> write();
			}
			updateWindowTitle();
			return(true);
		}
	}
	return(false);
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
	Demande un nom de fichier a l'utilisateur pour enregistrer le projet
	Si aucun nom n'est entre, elle renvoie faux.
	Si le nom ne se termine pas par l'extension .qet, celle-ci est ajoutee.
	Si l'enregistrement reussit, le nom du fichier est conserve et la fonction renvoie true.
	Sinon, faux est renvoye.
	@return true si l'enregistrement a reussi, false sinon
*/
bool ProjectView::saveAs() {
	// demande un nom de fichier a l'utilisateur pour enregistrer le projet
	QString filepath = QFileDialog::getSaveFileName(
		this,
		tr("Enregistrer sous", "dialog title"),
		project_ -> currentDir(),
		tr("Sch\351ma QElectroTech (*.qet)", "filetypes allowed when saving a diagram file")
	);
	
	// si aucun nom n'est entre, renvoie faux.
	if (filepath.isEmpty()) return(false);
	
	// si le nom ne se termine pas par l'extension .qet, celle-ci est ajoutee
	if (!filepath.endsWith(".qet", Qt::CaseInsensitive)) filepath += ".qet";
	
	// le fichier est assigne au projet
	project_ -> setFilePath(filepath);
	
	return(save());
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
	// on empeche l'utilisateur de deplacer les onglets
	tabs_ -> setMovable(!(project_ -> isReadOnly()));
	
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
