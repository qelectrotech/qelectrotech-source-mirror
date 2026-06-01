/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "diagram.h"
#include "editor/ui/qetelementeditor.h"
#include "elementscategoryeditor.h"
#include "qetapp.h"
#include "qeticons.h"
#include "qetproject.h"
#include "titleblock/templatedeleter.h"
#include <QFileInfo>
#include <QMessageBox>
#include "element.h"

/*
	When the ENABLE_PANEL_WIDGET_DND_CHECKS flag is set, the panel
	performs checks during drag'n drop of items and categories.
	For example, it checks that a target category is writable
	before authorizing the drop of an element.
	Removing this flag allows you to test the behavior of management functions
	items (copy, move, etc.).

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
	open_directory           = new QAction(QET::Icons::FolderOpen,             tr("Ouvrir le dossier correspondant"),     this);
	copy_path                = new QAction(QET::Icons::IC_CopyFile,            tr("Copier le chemin"),                    this);
	prj_activate             = new QAction(QET::Icons::ProjectFile,            tr("Basculer vers ce projet"),             this);
	prj_close                = new QAction(QET::Icons::DocumentClose,          tr("Fermer ce projet"),                    this);
	prj_edit_prop            = new QAction(QET::Icons::DialogInformation,      tr("Propriétés du projet"),          this);
	prj_prop_diagram         = new QAction(QET::Icons::DialogInformation,      tr("Propriétés du folio"),       this);
	prj_add_diagram          = new QAction(QET::Icons::DiagramAdd,              tr("Ajouter un folio"),                this);
	prj_duplicate_diagram   = new QAction(QET::Icons::IC_CopyFile,              tr("Copier et coller"),               this);
	prj_del_diagram          = new QAction(QET::Icons::DiagramDelete,          tr("Supprimer ce folio"),              this);
	prj_move_diagram_up      = new QAction(QET::Icons::GoUp,                   tr("Remonter ce folio"),               this);
	prj_move_diagram_down    = new QAction(QET::Icons::GoDown,                 tr("Abaisser ce folio"),               this);
	prj_move_diagram_upx10   = new QAction(QET::Icons::GoUpDouble,             tr("Remonter ce folio x10"),           this);
	prj_move_diagram_upx100  = new QAction(QET::Icons::GoUpDouble,             tr("Remonter ce folio x100"),           this);
	prj_move_diagram_top     = new QAction(QET::Icons::GoTop,                  tr("Remonter ce folio au debut"),               this);
	prj_move_diagram_downx10 = new QAction(QET::Icons::GoDownDouble,           tr("Abaisser ce folio x10"),           this);
	prj_move_diagram_downx100 = new QAction(QET::Icons::GoDownDouble,           tr("Abaisser ce folio x100"),           this);
	tbt_add               = new QAction(QET::Icons::TitleBlock,                tr("Nouveau modèle"),                   this);
	tbt_edit              = new QAction(QET::Icons::TitleBlock,                tr("Éditer ce modèle"),              this);
	tbt_remove            = new QAction(QET::Icons::TitleBlock,                tr("Supprimer ce modèle"),              this);


	prj_del_diagram -> setShortcut(QKeySequence(Qt::Key_Delete));
	prj_move_diagram_up -> setShortcut(QKeySequence(Qt::Key_F3));
	prj_move_diagram_down -> setShortcut(QKeySequence(Qt::Key_F4));
	prj_move_diagram_top -> setShortcut(QKeySequence(Qt::Key_F5));
	prj_move_diagram_downx10 -> setShortcut(QKeySequence(Qt::Key_F6));
	prj_move_diagram_downx100 -> setShortcut(QKeySequence(Qt::Key_F7));
	prj_move_diagram_upx10 -> setShortcut(QKeySequence(Qt::Key_F8));
	prj_move_diagram_upx100 -> setShortcut(QKeySequence(Qt::Key_F9));



	// initialise le champ de texte pour filtrer avec une disposition horizontale
	filter_textfield = new QLineEdit(this);
	filter_textfield -> setClearButtonEnabled(true);
	filter_textfield -> setPlaceholderText(tr("Filtrer"));


	context_menu = new QMenu(this);

	connect(open_directory,        SIGNAL(triggered()), this,           SLOT(openDirectoryForSelectedItem()));
	connect(copy_path,             SIGNAL(triggered()), this,           SLOT(copyPathForSelectedItem()));
	connect(prj_activate,          SIGNAL(triggered()), this,           SLOT(activateProject()));
	connect(prj_close,             SIGNAL(triggered()), this,           SLOT(closeProject()));
	connect(prj_edit_prop,         SIGNAL(triggered()), this,           SLOT(editProjectProperties()));
	connect(prj_prop_diagram,      SIGNAL(triggered()), this,           SLOT(editDiagramProperties()));
	connect(prj_add_diagram,       SIGNAL(triggered()), this,           SLOT(newDiagram()));
	connect(prj_del_diagram,       SIGNAL(triggered()), this,           SLOT(deleteDiagram()));
	connect(prj_duplicate_diagram, SIGNAL(triggered()), this,           SLOT(duplicateDiagram()));
	connect(prj_move_diagram_up,   SIGNAL(triggered()), this,           SLOT(moveDiagramUp()));
	connect(prj_move_diagram_down, SIGNAL(triggered()), this,           SLOT(moveDiagramDown()));
	connect(prj_move_diagram_top,  SIGNAL(triggered()), this,           SLOT(moveDiagramUpTop()));
	connect(prj_move_diagram_upx10,   SIGNAL(triggered()), this,        SLOT(moveDiagramUpx10()));
	connect(prj_move_diagram_upx100,  SIGNAL(triggered()), this,        SLOT(moveDiagramUpx100()));
	connect(prj_move_diagram_downx10, SIGNAL(triggered()), this,        SLOT(moveDiagramDownx10()));
	connect(prj_move_diagram_downx100,SIGNAL(triggered()), this,        SLOT(moveDiagramDownx100()));
	connect(tbt_add,               SIGNAL(triggered()), this,           SLOT(addTitleBlockTemplate()));
	connect(tbt_edit,              SIGNAL(triggered()), this,           SLOT(editTitleBlockTemplate()));
	connect(tbt_remove,            SIGNAL(triggered()), this,           SLOT(removeTitleBlockTemplate()));

	connect(filter_textfield,      SIGNAL(textChanged(const QString &)), this,             SLOT(filterEdited(const QString &)));

	connect(elements_panel,        SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(updateButtons()));
	connect(elements_panel,        SIGNAL(customContextMenuRequested(const QPoint &)),               this, SLOT(handleContextMenu(const QPoint &)));
	connect(
		elements_panel,
		SIGNAL(requestForTitleBlockTemplate(const TitleBlockTemplateLocation &)),
		QETApp::instance(),
		SLOT(openTitleBlockTemplate(const TitleBlockTemplateLocation &))
	);

		// manage double click on TreeWidgetItem
	connect(elements_panel, SIGNAL(requestForProjectPropertiesEdition()), this, SLOT(editProjectProperties()) );
	connect(elements_panel, SIGNAL(requestForDiagramPropertiesEdition()), this, SLOT(editDiagramProperties()) );
		// manage project activation
	connect(elements_panel, SIGNAL(requestForProject(QETProject*)), this, SIGNAL(requestForProject(QETProject*)));

	// disposition verticale
	QVBoxLayout *vlayout = new QVBoxLayout(this);
	vlayout -> setContentsMargins(0,0,0,0);
	vlayout -> setSpacing(0);
	vlayout -> addWidget(filter_textfield);
	vlayout -> addWidget(elements_panel);
	vlayout -> setStretchFactor(elements_panel, 75000);
	setLayout(vlayout);
}

/**
	Destructeur
*/
ElementsPanelWidget::~ElementsPanelWidget()
{
}

/**
	Require the desktop environment to open the directory containing the file
	represented by the selected item, if any.
*/
void ElementsPanelWidget::openDirectoryForSelectedItem()
{
	if (QTreeWidgetItem *qtwi = elements_panel -> currentItem()) {
		QString dir_path = elements_panel -> dirPathForItem(qtwi);
		if (!dir_path.isEmpty()) {
			QFileInfo fileInfo(dir_path);
			// Wenn der Pfad auf eine Datei (z.B. Makro) zeigt, isoliere den Ordnerpfad
			if (fileInfo.isFile()) {
				dir_path = fileInfo.absolutePath();
			}
			QDesktopServices::openUrl(QUrl::fromLocalFile(dir_path));
		}
	}
}

/**
	Copy the full path to the file represented by the selected item to the
	clipboard.
*/
void ElementsPanelWidget::copyPathForSelectedItem()
{
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
void ElementsPanelWidget::reloadAndFilter()
{
	// recharge tous les elements
	elements_panel -> reload();
	// reapplique le filtre
	if (!filter_textfield -> text().isEmpty()) {
		elements_panel -> filter(filter_textfield -> text());
	}
}

/**
	* Emit the requestForProject signal with the selected project
*/
void ElementsPanelWidget::activateProject()
{
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForProject(selected_project));
	}
}

/**
	Emet le signal requestForProjectClosing avec le projet selectionne
*/
void ElementsPanelWidget::closeProject()
{
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForProjectClosing(selected_project));
	}
}

/**
	Emet le signal requestForProjectPropertiesEdition avec le projet selectionne
*/
void ElementsPanelWidget::editProjectProperties()
{
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForProjectPropertiesEdition(selected_project));
	}
}

/**
	Emet le signal requestForDiagramPropertiesEdition avec le schema selectionne
*/
void ElementsPanelWidget::editDiagramProperties()
{
	if (Diagram *selected_diagram = elements_panel -> selectedDiagram()) {
		emit(requestForDiagramPropertiesEdition(selected_diagram));
	}
}

/**
	Emet le signal requestForNewDiagram avec le projet selectionne
*/
void ElementsPanelWidget::newDiagram()
{
	if (QETProject *selected_project = elements_panel -> selectedProject()) {
		emit(requestForNewDiagram(selected_project));
	}
}

/**
 * Emet le signal requestForDiagramsDeletion avec les schemas selectionnes
 */
void ElementsPanelWidget::deleteDiagram()
{
	QList<Diagram *> diagrams_to_delete = elements_panel->selectedDiagrams();

	if (diagrams_to_delete.isEmpty()) return;

	emit(requestForDiagramsDeletion(diagrams_to_delete));

	elements_panel->reload();
}
/**
 * Emits the requestForDiagramMoveUpTop signal with all selected diagrams.
 */
void ElementsPanelWidget::moveDiagramUpTop() {
	QList<Diagram *> diagrams_to_move = elements_panel->selectedDiagrams();
	if (diagrams_to_move.isEmpty()) return;

	// Emit the entire list at once
	emit requestForDiagramMoveUpTop(diagrams_to_move);

	// Clear messy tree selection caused by moving items, then restore clean selection
	elements_panel->clearSelection();
	for (Diagram *d : diagrams_to_move) {
		if (auto item = elements_panel->getItemForDiagram(d)) item->setSelected(true);
	}
}

/**
 * Emits the requestForDiagramMoveUp signal with all selected diagrams.
 */
void ElementsPanelWidget::moveDiagramUp() {
	QList<Diagram *> diagrams_to_move = elements_panel->selectedDiagrams();
	if (diagrams_to_move.isEmpty()) return;

	// Emit the entire list at once
	emit requestForDiagramMoveUp(diagrams_to_move);

	// Clear messy tree selection caused by moving items, then restore clean selection
	elements_panel->clearSelection();
	for (Diagram *d : diagrams_to_move) {
		if (auto item = elements_panel->getItemForDiagram(d)) item->setSelected(true);
	}
}

/**
 * Emits the requestForDiagramMoveDown signal with all selected diagrams.
 */
void ElementsPanelWidget::moveDiagramDown() {
	QList<Diagram *> diagrams_to_move = elements_panel->selectedDiagrams();
	if (diagrams_to_move.isEmpty()) return;

	// Emit the entire list at once
	emit requestForDiagramMoveDown(diagrams_to_move);

	// Clear messy tree selection caused by moving items, then restore clean selection
	elements_panel->clearSelection();
	for (Diagram *d : diagrams_to_move) {
		if (auto item = elements_panel->getItemForDiagram(d)) item->setSelected(true);
	}
}

/**
 * Emits the requestForDiagramMoveUpx10 signal with all selected diagrams.
 */
void ElementsPanelWidget::moveDiagramUpx10() {
	QList<Diagram *> diagrams_to_move = elements_panel->selectedDiagrams();
	if (diagrams_to_move.isEmpty()) return;

	// Emit the entire list at once
	emit requestForDiagramMoveUpx10(diagrams_to_move);

	// Clear messy tree selection caused by moving items, then restore clean selection
	elements_panel->clearSelection();
	for (Diagram *d : diagrams_to_move) {
		if (auto item = elements_panel->getItemForDiagram(d)) item->setSelected(true);
	}
}

/**
 * Emits the requestForDiagramMoveUpx100 signal with all selected diagrams.
 */
void ElementsPanelWidget::moveDiagramUpx100() {
	QList<Diagram *> diagrams_to_move = elements_panel->selectedDiagrams();
	if (diagrams_to_move.isEmpty()) return;

	// Emit the entire list at once
	emit requestForDiagramMoveUpx100(diagrams_to_move);

	// Clear messy tree selection caused by moving items, then restore clean selection
	elements_panel->clearSelection();
	for (Diagram *d : diagrams_to_move) {
		if (auto item = elements_panel->getItemForDiagram(d)) item->setSelected(true);
	}
}

/**
 * Emits the requestForDiagramMoveDownx10 signal with all selected diagrams.
 */
void ElementsPanelWidget::moveDiagramDownx10() {
	QList<Diagram *> diagrams_to_move = elements_panel->selectedDiagrams();
	if (diagrams_to_move.isEmpty()) return;

	// Emit the entire list at once
	emit requestForDiagramMoveDownx10(diagrams_to_move);

	// Clear messy tree selection caused by moving items, then restore clean selection
	elements_panel->clearSelection();
	for (Diagram *d : diagrams_to_move) {
		if (auto item = elements_panel->getItemForDiagram(d)) item->setSelected(true);
	}
}

/**
 * Emits the requestForDiagramMoveDownx100 signal with all selected diagrams.
 */
void ElementsPanelWidget::moveDiagramDownx100() {
	QList<Diagram *> diagrams_to_move = elements_panel->selectedDiagrams();
	if (diagrams_to_move.isEmpty()) return;

	// Emit the entire list at once
	emit requestForDiagramMoveDownx100(diagrams_to_move);

	// Clear messy tree selection caused by moving items, then restore clean selection
	elements_panel->clearSelection();
	for (Diagram *d : diagrams_to_move) {
		if (auto item = elements_panel->getItemForDiagram(d)) item->setSelected(true);
	}
}

/**
	Opens a template editor to create a new title block template.
*/
void ElementsPanelWidget::addTitleBlockTemplate()
{
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
void ElementsPanelWidget::editTitleBlockTemplate()
{
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
void ElementsPanelWidget::removeTitleBlockTemplate()
{
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
void ElementsPanelWidget::updateButtons()
{
	QTreeWidgetItem *current_item = elements_panel -> currentItem();
	int current_type = elements_panel -> currentItemType();

	if (current_type == QET::Project) {
		bool is_writable = !(elements_panel -> selectedProject() -> isReadOnly());
		prj_add_diagram -> setEnabled(is_writable);
	} else if (current_type == QET::Diagram) {
		// Fetch ALL selected diagrams instead of just one
		QList<Diagram *> selected_diagrams = elements_panel -> selectedDiagrams();

		if (!selected_diagrams.isEmpty()) {
			QETProject *selected_diagram_project = selected_diagrams.first() -> project();
			bool is_writable           = !(selected_diagram_project -> isReadOnly());
			int project_diagrams_count = selected_diagram_project -> diagrams().count();

			// Find the highest (min) and lowest (max) index among the selection
			int min_position = project_diagrams_count;
			int max_position = -1;

			for (Diagram *diagram : selected_diagrams) {
				int pos = selected_diagram_project -> diagrams().indexOf(diagram);
				if (pos < min_position) min_position = pos;
				if (pos > max_position) max_position = pos;
			}

			prj_del_diagram           -> setEnabled(is_writable);
			prj_duplicate_diagram     -> setEnabled(is_writable);
			prj_move_diagram_up        -> setEnabled(is_writable && min_position > 0);
			prj_move_diagram_down     -> setEnabled(is_writable && max_position < project_diagrams_count - 1);
			prj_move_diagram_top      -> setEnabled(is_writable && min_position > 0);

			// Adjusted to >= to allow exactly 10 or 100 steps if space permits
			prj_move_diagram_upx10    -> setEnabled(is_writable && min_position > 10);
			prj_move_diagram_upx100   -> setEnabled(is_writable && min_position > 100);
			prj_move_diagram_downx10  -> setEnabled(is_writable && max_position < project_diagrams_count - 10);
			prj_move_diagram_downx100 -> setEnabled(is_writable && max_position < project_diagrams_count - 100);
		}
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
			context_menu -> addAction(prj_duplicate_diagram);
			context_menu -> addAction(prj_move_diagram_top);
			context_menu -> addAction(prj_move_diagram_upx10);
			context_menu -> addAction(prj_move_diagram_upx100);
			context_menu -> addAction(prj_move_diagram_up);
			context_menu -> addAction(prj_move_diagram_down);
			context_menu -> addAction(prj_move_diagram_downx10);
			context_menu -> addAction(prj_move_diagram_downx100);
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
 * Treat key press event inside elements panel widget
 */
/**
 * Treat key press event inside elements panel widget.
 * Respects the enabled/disabled state of the corresponding QActions.
 */
void ElementsPanelWidget::keyPressEvent(QKeyEvent *e) {
	switch(e->key()) {
		case Qt::Key_Delete:
			if (prj_del_diagram && prj_del_diagram->isEnabled()) {
				deleteDiagram();
			}
			break;
		case Qt::Key_F3:
			if (prj_move_diagram_up && prj_move_diagram_up->isEnabled()) {
				moveDiagramUp();
			}
			break;
		case Qt::Key_F4:
			if (prj_move_diagram_down && prj_move_diagram_down->isEnabled()) {
				moveDiagramDown();
			}
			break;
		case Qt::Key_F5:
			if (prj_move_diagram_top && prj_move_diagram_top->isEnabled()) {
				moveDiagramUpTop();
			}
			break;
		case Qt::Key_F6:
			if (prj_move_diagram_downx10 && prj_move_diagram_downx10->isEnabled()) {
				moveDiagramDownx10();
			}
			break;
		case Qt::Key_F7:
			if (prj_move_diagram_downx100 && prj_move_diagram_downx100->isEnabled()) {
				moveDiagramDownx100();
			}
			break;
		case Qt::Key_F8:
			if (prj_move_diagram_upx10 && prj_move_diagram_upx10->isEnabled()) {
				moveDiagramUpx10();
			}
			break;
		case Qt::Key_F9:
			if (prj_move_diagram_upx100 && prj_move_diagram_upx100->isEnabled()) {
				moveDiagramUpx100();
			}
			break;
		default:
			// Pass unhandled key events to the base class
			QWidget::keyPressEvent(e);
			break;
	}
}

/**
 * Duplicates the selected folios (pages) along with their content
 * and properties, and cleanly resolves cross-references.
 */
void ElementsPanelWidget::duplicateDiagram()
{
	QList<Diagram *> diagrams_to_duplicate = elements_panel->selectedDiagrams();
	if (diagrams_to_duplicate.isEmpty()) return;

	QETProject *project = diagrams_to_duplicate.first()->project();
	if (!project || project->isReadOnly()) return;

	for (Diagram *source_diagram : diagrams_to_duplicate) {

		Diagram *new_diagram = project->addNewDiagram();
		if (!new_diagram) continue;

		QString template_name = source_diagram->border_and_titleblock.titleBlockTemplateName();
		new_diagram->setTitleBlockTemplate(template_name);

		TitleBlockProperties tbp = source_diagram->border_and_titleblock.exportTitleBlock();
		new_diagram->border_and_titleblock.importTitleBlock(tbp);

		BorderProperties bp = source_diagram->border_and_titleblock.exportBorder();
		new_diagram->border_and_titleblock.importBorder(bp);

		for (QGraphicsItem *item : source_diagram->items()) {
			if (Element *elmt = dynamic_cast<Element *>(item)) {
				source_diagram->correctTextPos(elmt);
			}
		}

		QDomDocument doc = source_diagram->toXml();
		QDomElement diagram_elmt = doc.documentElement();

		for (QGraphicsItem *item : source_diagram->items()) {
			if (Element *elmt = dynamic_cast<Element *>(item)) {
				source_diagram->restoreText(elmt);
			}
		}

		new_diagram->fromXml(diagram_elmt, QPointF(0, 0), false, nullptr);

		for (QGraphicsItem *item : new_diagram->items()) {
			if (Element *elmt = dynamic_cast<Element *>(item)) {
				new_diagram->restoreText(elmt);
			}
		}
	}

	elements_panel->reload();
}
