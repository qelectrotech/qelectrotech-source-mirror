/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "elementspanelwidget.h"
#include "qetelementeditor.h"
#include "elementscategoryeditor.h"
#include "qetapp.h"
#include "qetproject.h"
#include "diagram.h"
#include "qeticons.h"
#include "templatedeleter.h"

/*
	Lorsque le flag ENABLE_PANEL_WIDGET_DND_CHECKS est defini, le panel
	effectue des verifications lors des drag'n drop d'elements et categories.
	Par exemple, il verifie qu'une categorie cible est accessible en ecriture
	avant d'y autoriser le drop d'un element.
	Supprimer ce flag permet de tester le comportement des fonctions de gestion
	des items (copy, move, etc.).
*/
#define ENABLE_PANEL_WIDGET_DND_CHECKS

/**
	Constructeur
	@param parent Le QWidget parent de ce widget
*/
ElementsPanelWidget::ElementsPanelWidget(QWidget *parent) : QWidget(parent) {
	// initalise le panel d'elements
	elements_panel = new ElementsPanel(this);
	
	// initialise les actions
	open_directory        = new QAction(QET::Icons::DocumentOpen,              tr("Ouvrir le dossier correspondant"),     this);
	copy_path             = new QAction(QET::Icons::IC_CopyFile,                  tr("Copier le chemin"),                    this);
	reload                = new QAction(QET::Icons::ViewRefresh,               tr("Recharger les collections"),           this);
	prj_activate          = new QAction(QET::Icons::ProjectFile,               tr("Basculer vers ce projet"),             this);
	prj_close             = new QAction(QET::Icons::DocumentClose,             tr("Fermer ce projet"),                    this);
	prj_edit_prop         = new QAction(QET::Icons::DialogInformation,         tr("Propriétés du projet"),          this);
	prj_prop_diagram      = new QAction(QET::Icons::DialogInformation,         tr("Propriétés du folio"),       this);
	prj_add_diagram       = new QAction(QET::Icons::DiagramAdd,                tr("Ajouter un folio"),                this);
	prj_del_diagram       = new QAction(QET::Icons::DiagramDelete,             tr("Supprimer ce folio"),              this);
	prj_move_diagram_up   = new QAction(QET::Icons::GoUp,                      tr("Remonter ce folio"),               this);
	prj_move_diagram_down = new QAction(QET::Icons::GoDown,                    tr("Abaisser ce folio"),               this);
	prj_move_diagram_upx10   = new QAction(QET::Icons::GoUp,                   tr("Remonter ce folio x10"),           this);
	prj_move_diagram_top   = new QAction(QET::Icons::GoUp,                     tr("Remonter ce folio au debut"),               this);
	prj_move_diagram_downx10 = new QAction(QET::Icons::GoDown,                 tr("Abaisser ce folio x10"),           this);
	tbt_add               = new QAction(QET::Icons::TitleBlock,                tr("Nouveau modèle"),                   this);
	tbt_edit              = new QAction(QET::Icons::TitleBlock,                tr("Éditer ce modèle"),              this);
	tbt_remove            = new QAction(QET::Icons::TitleBlock,                tr("Supprimer ce modèle"),              this);

	reload            -> setShortcut(Qt::Key_F5);

	// initialise le champ de texte pour filtrer avec une disposition horizontale
	filter_textfield = new QLineEdit(this);
	filter_textfield -> setClearButtonEnabled(true);
	filter_textfield -> setPlaceholderText(tr("Filtrer"));


	context_menu = new QMenu(this);
	
	connect(open_directory,        SIGNAL(triggered()), this,           SLOT(openDirectoryForSelectedItem()));
	connect(copy_path,             SIGNAL(triggered()), this,           SLOT(copyPathForSelectedItem()));
	connect(reload,                SIGNAL(triggered()), this,           SLOT(reloadAndFilter()));
	connect(prj_activate,          SIGNAL(triggered()), this,           SLOT(activateProject()));
	connect(prj_close,             SIGNAL(triggered()), this,           SLOT(closeProject()));
	connect(prj_edit_prop,         SIGNAL(triggered()), this,           SLOT(editProjectProperties()));
	connect(prj_prop_diagram,      SIGNAL(triggered()), this,           SLOT(editDiagramProperties()));
	connect(prj_add_diagram,       SIGNAL(triggered()), this,           SLOT(newDiagram()));
	connect(prj_del_diagram,       SIGNAL(triggered()), this,           SLOT(deleteDiagram()));
	connect(prj_move_diagram_up,   SIGNAL(triggered()), this,           SLOT(moveDiagramUp()));
	connect(prj_move_diagram_down, SIGNAL(triggered()), this,           SLOT(moveDiagramDown()));
	connect(prj_move_diagram_top,  SIGNAL(triggered()), this,           SLOT(moveDiagramUpTop()));
	connect(prj_move_diagram_upx10,   SIGNAL(triggered()), this,        SLOT(moveDiagramUpx10()));
	connect(prj_move_diagram_downx10, SIGNAL(triggered()), this,        SLOT(moveDiagramDownx10()));
	connect(tbt_add,               SIGNAL(triggered()), this,           SLOT(addTitleBlockTemplate()));
	connect(tbt_edit,              SIGNAL(triggered()), this,           SLOT(editTitleBlockTemplate()));
	connect(tbt_remove,            SIGNAL(triggered()), this,           SLOT(removeTitleBlockTemplate()));
	
	connect(filter_textfield,      SIGNAL(textChanged(const QString &)), this,             SLOT(filterEdited(const QString &)));
	
	connect(elements_panel,        SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(updateButtons()));
	connect(elements_panel,        SIGNAL(customContextMenuRequested(const QPoint &)),               this, SLOT(handleContextMenu(const QPoint &)));
	connect(elements_panel,        SIGNAL(requestForDiagram(Diagram*)),                              this, SIGNAL(requestForDiagram(Diagram*)));
	connect(
		elements_panel,
		SIGNAL(requestForTitleBlockTemplate(const TitleBlockTemplateLocation &)),
		QETApp::instance(),
		SLOT(openTitleBlockTemplate(const TitleBlockTemplateLocation &))
	);
	
	// disposition verticale
	QVBoxLayout *vlayout = new QVBoxLayout(this);
	vlayout -> setMargin(0);
	vlayout -> setSpacing(0);
	vlayout -> addWidget(filter_textfield);
	vlayout -> addWidget(elements_panel);
	vlayout -> setStretchFactor(elements_panel, 75000);
	setLayout(vlayout);
}

/**
	Destructeur
*/
ElementsPanelWidget::~ElementsPanelWidget() {
}

/**
	Require the desktop environment to open the directory containing the file
	represented by the selected item, if any.
*/
void ElementsPanelWidget::openDirectoryForSelectedItem() {
	if (QTreeWidgetItem *qtwi = elements_panel -> currentItem()) {
		QString dir_path = elements_panel -> dirPathForItem(qtwi);
		if (!dir_path.isEmpty()) {
			QDesktopServices::openUrl(QUrl::fromLocalFile(dir_path));
		}
	}
}

/**
	Copy the full path to the file represented by the selected item to the
	clipboard.
*/
void ElementsPanelWidget::copyPathForSelectedItem() {
	if (QTreeWidgetItem *qtwi = elements_panel -> currentItem()) {
		QString file_path = elements_panel -> filePathForItem(qtwi);
		file_path = QDir::toNativeSeparators(file_path);
		if (!file_path.isEmpty()) {
			QApplication::clipboard() -> setText(file_path);
		}
	}
}

/**
	Recharge le panel d'elements
*/
void ElementsPanelWidget::reloadAndFilter() {
	// recharge tous les elements
	reload -> setEnabled(false);
	elements_panel -> reload(true);
	// reapplique le filtre
	if (!filter_textfield -> text().isEmpty()) {
		elements_panel -> filter(filter_textfield -> text());
	}
	reload -> setEnabled(true);
}

/**
	* Emit the requestForProject signal with te selected project
*/
void ElementsPanelWidget::activateProject() {
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForProject(selected_project));
	}
}

/**
	Emet le signal requestForProjectClosing avec le projet selectionne
*/
void ElementsPanelWidget::closeProject() {
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForProjectClosing(selected_project));
	}
}

/**
	Emet le signal requestForProjectPropertiesEdition avec le projet selectionne
*/
void ElementsPanelWidget::editProjectProperties() {
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForProjectPropertiesEdition(selected_project));
	}
}

/**
	Emet le signal requestForDiagramPropertiesEdition avec le schema selectionne
*/
void ElementsPanelWidget::editDiagramProperties() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramPropertiesEdition(selected_diagram));
	}
}

/**
	Emet le signal requestForNewDiagram avec le projet selectionne
*/
void ElementsPanelWidget::newDiagram() {
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForNewDiagram(selected_project));
	}
}

/**
	Emet le signal requestForDiagramDeletion avec le schema selectionne
*/
void ElementsPanelWidget::deleteDiagram() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramDeletion(selected_diagram));
	}
}

/**
	Emet le signal requestForDiagramMoveUpTop avec le schema selectionne
+ */
void ElementsPanelWidget::moveDiagramUpTop() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramMoveUpTop(selected_diagram));
	}
}



/**
	Emet le signal requestForDiagramMoveUp avec le schema selectionne
*/
void ElementsPanelWidget::moveDiagramUp() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramMoveUp(selected_diagram));
	}
}

/**
	Emet le signal requestForDiagramMoveDown avec le schema selectionne
*/
void ElementsPanelWidget::moveDiagramDown() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramMoveDown(selected_diagram));
	}
}

/**
	Emet le signal requestForDiagramMoveUpx10 avec le schema selectionne
*/
void ElementsPanelWidget::moveDiagramUpx10() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramMoveUpx10(selected_diagram));
	}
}

/**
	Emet le signal requestForDiagramMoveDownx10 avec le schema selectionne
*/
void ElementsPanelWidget::moveDiagramDownx10() {
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramMoveDownx10(selected_diagram));
	}
}


/**
	Opens a template editor to create a new title block template.
*/
void ElementsPanelWidget::addTitleBlockTemplate() {
	QTreeWidgetItem *current_item = elements_panel -> currentItem();
	if (!current_item) return;
	
	if (current_item -> type() == QET::TitleBlockTemplatesCollection) {
		QETApp::instance() -> openTitleBlockTemplate(
			elements_panel -> templateLocationForItem(current_item)
		);
	}
}

/**
	Opens an editor to edit the currently selected title block template, if any.
*/
void ElementsPanelWidget::editTitleBlockTemplate() {
	QTreeWidgetItem *current_item = elements_panel -> currentItem();
	if (current_item && current_item -> type() == QET::TitleBlockTemplate) {
		QETApp::instance() -> openTitleBlockTemplate(
			elements_panel -> templateLocationForItem(current_item)
		);
	}
}

/**
	Delete the currently selected title block template, if any.
*/
void ElementsPanelWidget::removeTitleBlockTemplate() {
	QTreeWidgetItem *current_item = elements_panel -> currentItem();
	if (current_item && current_item -> type() == QET::TitleBlockTemplate) {
		TitleBlockTemplateDeleter(
			elements_panel -> templateLocationForItem(current_item),
			this
		).exec();
	}
}

/**
	Met a jour les boutons afin d'assurer la coherence de l'interface
*/
void ElementsPanelWidget::updateButtons() {
	QTreeWidgetItem *current_item = elements_panel -> currentItem();
	int current_type = elements_panel -> currentItemType();
	
	if (current_type == QET::Project) {
		bool is_writable = !(elements_panel -> selectedProject() -> isReadOnly());
		prj_add_diagram -> setEnabled(is_writable);
	} else if (current_type == QET::Diagram) {
		Diagram    *selected_diagram         = elements_panel -> selectedDiagram();
		QETProject *selected_diagram_project = selected_diagram -> project();
		
		bool is_writable           = !(selected_diagram_project -> isReadOnly());
		int project_diagrams_count = selected_diagram_project -> diagrams().count();
		int diagram_position       = selected_diagram_project -> diagrams().indexOf(selected_diagram);
		
		prj_del_diagram       -> setEnabled(is_writable);
		prj_move_diagram_up   -> setEnabled(is_writable && diagram_position > 0);
		prj_move_diagram_down -> setEnabled(is_writable && diagram_position < project_diagrams_count - 1);
		prj_move_diagram_top   -> setEnabled(is_writable && diagram_position > 0);
		prj_move_diagram_upx10   -> setEnabled(is_writable && diagram_position > 10);
		prj_move_diagram_downx10 -> setEnabled(is_writable && diagram_position < project_diagrams_count - 10);
	} else if (current_type == QET::TitleBlockTemplatesCollection) {
		TitleBlockTemplateLocation location = elements_panel -> templateLocationForItem(current_item);
		tbt_add    -> setEnabled(!location.isReadOnly());
		tbt_edit   -> setEnabled(false); // would not make sense
		tbt_remove -> setEnabled(false); // would not make sense
	} else if (current_type == QET::TitleBlockTemplate) {
		QTreeWidgetItem *item = elements_panel -> currentItem();
		TitleBlockTemplateLocation location = elements_panel -> templateLocationForItem(item);
		tbt_add    -> setEnabled(false); // would not make sense
		tbt_edit   -> setEnabled(true); // the tbt editor has a read-only mode
		// deleting a tbt requires its parent collection to be writable
		tbt_remove -> setEnabled(location.parentCollection() && !(location.parentCollection() -> isReadOnly()));
	}
}

/**
	Gere le menu contextuel du panel d'elements
	@param pos Position ou le menu contextuel a ete demande
*/
void ElementsPanelWidget::handleContextMenu(const QPoint &pos) {
	// recupere l'item concerne par l'evenement ainsi que son chemin
	QTreeWidgetItem *item = elements_panel -> itemAt(pos);
	if (!item) return;
	
	updateButtons();
	context_menu -> clear();
	context_menu->addAction(reload);
	
	QString dir_path = elements_panel -> dirPathForItem(item);
	if (!dir_path.isEmpty()) {
		context_menu -> addAction(open_directory);
		context_menu -> addAction(copy_path);
		context_menu -> addSeparator();
	}
	
	switch(item -> type()) {
		case QET::Project:
			context_menu -> addAction(prj_activate);
			context_menu -> addAction(prj_edit_prop);
			context_menu -> addAction(prj_add_diagram);
			context_menu -> addAction(prj_close);
			break;
		case QET::Diagram:
			context_menu -> addAction(prj_prop_diagram);
			context_menu -> addAction(prj_del_diagram);
			context_menu -> addAction(prj_move_diagram_top);
			context_menu -> addAction(prj_move_diagram_upx10);
			context_menu -> addAction(prj_move_diagram_up);
			context_menu -> addAction(prj_move_diagram_down);
			context_menu -> addAction(prj_move_diagram_downx10);
			break;
		case QET::TitleBlockTemplatesCollection:
			context_menu -> addAction(tbt_add);
			break;
		case QET::TitleBlockTemplate:
			context_menu -> addAction(tbt_edit);
			context_menu -> addAction(tbt_remove);
			break;
	}
	
	// affiche le menu
	if (!context_menu -> isEmpty()) {
		context_menu -> popup(mapToGlobal(elements_panel -> mapTo(this, pos + QPoint(2, 2))));
	}
}

void ElementsPanelWidget::filterEdited(const QString &next_text) {
	if (previous_filter_.isEmpty() && next_text.length() == 1) {
		// the field is not empty anymore: begin filtering
		elements_panel -> filter(next_text, QET::BeginFilter);
	} else if (!previous_filter_.isEmpty() && next_text.isEmpty()) {
		// the field is now empty again: end of filtering
		elements_panel -> filter(QString(), QET::EndFilter);
	} else {
		// regular filtering
		elements_panel -> filter(next_text, QET::RegularFilter);
	}
	previous_filter_ = next_text;
}

/**
	Treat key press event inside elements panel widget
*/
void ElementsPanelWidget::keyPressEvent   (QKeyEvent *e) {
	switch(e -> key()) {
				case Qt::Key_Delete: //delete diagram through elements panel widget
					if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
						emit(requestForDiagramDeletion(selected_diagram));
					}
				}
	return;
}
