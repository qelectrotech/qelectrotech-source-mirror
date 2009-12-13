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
#include "diagramview.h"
#include "diagram.h"
#include "customelement.h"
#include "ghostelement.h"
#include "conductor.h"
#include "diagramcommands.h"
#include "diagramposition.h"
#include "conductorpropertieswidget.h"
#include "elementtextitem.h"
#include "insetpropertieswidget.h"
#include "qetapp.h"
#include "qetproject.h"
#include "borderpropertieswidget.h"
#include "integrationmoveelementshandler.h"
#include "qetdiagrameditor.h"
#include "qeticons.h"
#include "qetmessagebox.h"

/**
	Constructeur
	@param diagram Schema a afficher ; si diagram vaut 0, un nouveau Diagram est utilise
	@param parent Le QWidget parent de cette vue de schema
*/
DiagramView::DiagramView(Diagram *diagram, QWidget *parent) : QGraphicsView(parent), is_adding_text(false) {
	setAttribute(Qt::WA_DeleteOnClose, true);
	setInteractive(true);
	
	// active l'antialiasing
	setRenderHint(QPainter::Antialiasing, true);
	setRenderHint(QPainter::TextAntialiasing, true);
	setRenderHint(QPainter::SmoothPixmapTransform, true);
	
	scene = diagram ? diagram : new Diagram(this);
	setScene(scene);
	scene -> undoStack().setClean();
	setWindowIcon(QET::Icons::QETLogo);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	setAlignment(Qt::AlignLeft | Qt::AlignTop);
	setSelectionMode();
	adjustSceneRect();
	updateWindowTitle();
	
	context_menu = new QMenu(this);
	paste_here = new QAction(QET::Icons::EditPaste, tr("Coller ici", "context menu action"), this);
	connect(paste_here, SIGNAL(triggered()), this, SLOT(pasteHere()));
	
	connect(scene, SIGNAL(selectionChanged()), this, SIGNAL(selectionChanged()));
	connect(scene, SIGNAL(readOnlyChanged(bool)), this, SLOT(applyReadOnly()));
	connect(&(scene -> border_and_inset), SIGNAL(borderChanged(QRectF, QRectF)), this, SLOT(adjustSceneRect()));
	connect(&(scene -> border_and_inset), SIGNAL(displayChanged()),              this, SLOT(adjustSceneRect()));
	connect(&(scene -> border_and_inset), SIGNAL(diagramTitleChanged(const QString &)), this, SLOT(updateWindowTitle()));
	connect(&(scene -> undoStack()), SIGNAL(cleanChanged(bool)), this, SLOT(updateWindowTitle()));
	
	connect(this, SIGNAL(aboutToAddElement()), this, SLOT(addDroppedElement()), Qt::QueuedConnection);
}

/**
	Destructeur
*/
DiagramView::~DiagramView() {
}

/**
	Selectionne tous les objets du schema
*/
void DiagramView::selectAll() {
	scene -> selectAll();
}

/**
	Deslectionne tous les objets selectionnes
*/
void DiagramView::selectNothing() {
	scene -> deselectAll();
}

/**
	Inverse l'etat de selection de tous les objets du schema
*/
void DiagramView::selectInvert() {
	scene -> invertSelection();
}

/**
	Supprime les composants selectionnes
*/
void DiagramView::deleteSelection() {
	if (scene -> isReadOnly()) return;
	DiagramContent removed_content = scene -> selectedContent();
	scene -> clearSelection();
	scene -> undoStack().push(new DeleteElementsCommand(scene, removed_content));
	adjustSceneRect();
}

/**
	Pivote les composants selectionnes
*/
void DiagramView::rotateSelection() {
	if (scene -> isReadOnly()) return;
	
	// recupere les elements et les champs de texte a pivoter
	QHash<Element *, QET::Orientation> elements_to_rotate;
	QList<DiagramTextItem *> texts_to_rotate;
	foreach (QGraphicsItem *item, scene -> selectedItems()) {
		if (Element *e = qgraphicsitem_cast<Element *>(item)) {
			elements_to_rotate.insert(e, e -> orientation().current());
		} else if (DiagramTextItem *dti = qgraphicsitem_cast<DiagramTextItem *>(item)) {
			texts_to_rotate << dti;
		} else if (ElementTextItem *eti = qgraphicsitem_cast<ElementTextItem *>(item)) {
			// on ne pivote un texte d'element que si son parent n'est pas selectionne
			if (eti -> parentItem() && !eti -> parentItem() -> isSelected()) {
				texts_to_rotate << eti;
			}
		}
	}
	
	// effectue les rotations s'il y a quelque chose a pivoter
	if (elements_to_rotate.isEmpty() && texts_to_rotate.isEmpty()) return;
	scene -> undoStack().push(new RotateElementsCommand(elements_to_rotate, texts_to_rotate));
}

/**
	Accepte ou refuse le drag'n drop en fonction du type de donnees entrant
	@param e le QDragEnterEvent correspondant au drag'n drop tente
*/
void DiagramView::dragEnterEvent(QDragEnterEvent *e) {
	if (e -> mimeData() -> hasFormat("application/x-qet-element-uri")) {
		e -> acceptProposedAction();
	} else {
		e -> ignore();
	}
}

/**
	Gere les dragleaveevent
	@param e le QDragEnterEvent correspondant au drag'n drop sortant
*/
void DiagramView::dragLeaveEvent(QDragLeaveEvent *e) {
	Q_UNUSED(e);
}

/**
	Accepte ou refuse le drag'n drop en fonction du type de donnees entrant
	@param e le QDragMoveEvent correspondant au drag'n drop tente
*/
void DiagramView::dragMoveEvent(QDragMoveEvent *e) {
	if (e -> mimeData() -> hasFormat("text/plain")) e -> acceptProposedAction();
	else e-> ignore();
}

/**
	Gere les depots (drop) acceptes sur le schema. Cette methode emet le signal
	aboutToAddElement si l'element depose est accessible.
	@param e le QDropEvent correspondant au drag'n drop effectue
*/
void DiagramView::dropEvent(QDropEvent *e) {
	// recupere l'emplacement de l'element depuis le drag'n drop
	QString elmt_path = e -> mimeData() -> text();
	ElementsLocation location(ElementsLocation::locationFromString(elmt_path));
	
	// verifie qu'il existe un element correspondant a cet emplacement
	ElementsCollectionItem *dropped_item = QETApp::collectionItem(location);
	if (!dropped_item) return;
	
	next_location_ = location;
	next_position_ = e-> pos();
	
	emit(aboutToAddElement());
}

/**
	Passe le Diagram en mode visualisation
*/
void DiagramView::setVisualisationMode() {
	setDragMode(ScrollHandDrag);
	applyReadOnly();
	setInteractive(false);
	emit(modeChanged());
}

/**
	Passe le Diagram en mode Selection
*/
void DiagramView::setSelectionMode() {
	setDragMode(RubberBandDrag);
	setInteractive(true);
	applyReadOnly();
	emit(modeChanged());
}

/**
	Agrandit le schema (+33% = inverse des -25 % de zoomMoins())
*/
void DiagramView::zoomIn() {
	scale(4.0/3.0, 4.0/3.0);
	adjustGridToZoom();
}

/**
	Retrecit le schema (-25% = inverse des +33 % de zoomPlus())
*/
void DiagramView::zoomOut() {
	scale(0.75, 0.75);
	adjustGridToZoom();
}

/**
	Agrandit ou rectrecit le schema de facon a ce que tous les elements du
	schema soient visibles a l'ecran. S'il n'y a aucun element sur le schema,
	le zoom est reinitialise
*/
void DiagramView::zoomFit() {
	adjustSceneRect();
	fitInView(sceneRect(), Qt::KeepAspectRatio);
	adjustGridToZoom();
}

/**
	Reinitialise le zoom
*/
void DiagramView::zoomReset() {
	resetMatrix();
	adjustGridToZoom();
}

/**
	Copie les elements selectionnes du schema dans le presse-papier puis les supprime
*/
void DiagramView::cut() {
	copy();
	DiagramContent cut_content = scene -> selectedContent();
	scene -> clearSelection();
	scene -> undoStack().push(new CutDiagramCommand(scene, cut_content));
}

/**
	Copie les elements selectionnes du schema dans le presse-papier
*/
void DiagramView::copy() {
	QClipboard *presse_papier = QApplication::clipboard();
	QString contenu_presse_papier = scene -> toXml(false).toString(4);
	if (presse_papier -> supportsSelection()) presse_papier -> setText(contenu_presse_papier, QClipboard::Selection);
	presse_papier -> setText(contenu_presse_papier);
}

/**
	Importe les elements contenus dans le presse-papier dans le schema
	@param pos coin superieur gauche (en coordonnees de la scene) du rectangle
	englobant le contenu colle
	@param clipboard_mode Type de presse-papier a prendre en compte
*/
void DiagramView::paste(const QPointF &pos, QClipboard::Mode clipboard_mode) {
	if (scene -> isReadOnly()) return;
	
	QString texte_presse_papier = QApplication::clipboard() -> text(clipboard_mode);
	if ((texte_presse_papier).isEmpty()) return;
	
	QDomDocument document_xml;
	if (!document_xml.setContent(texte_presse_papier)) return;
	
	// objet pour recuperer le contenu ajoute au schema par le coller
	DiagramContent content_pasted;
	scene -> fromXml(document_xml, pos, false, &content_pasted);
	
	// si quelque chose a effectivement ete ajoute au schema, on cree un objet d'annulation
	if (content_pasted.count()) {
		scene -> clearSelection();
		scene -> undoStack().push(new PasteDiagramCommand(scene, content_pasted));
		adjustSceneRect();
	}
}

/**
	Colle le contenu du presse-papier sur le schema a la position de la souris
*/
void DiagramView::pasteHere() {
	paste(mapToScene(paste_here_pos));
}

/**
	Gere les clics et plus particulierement :
	 *  le clic du milieu (= coller pour X11)
	 *  le clic pour ajouter un champ de texte independant
*/
void DiagramView::mousePressEvent(QMouseEvent *e) {
	if (e -> buttons() == Qt::MidButton) {
		paste(mapToScene(e -> pos()), QClipboard::Selection);
	} else {
		if (!scene -> isReadOnly() && is_adding_text && e -> buttons() == Qt::LeftButton) {
			addDiagramTextAtPos(mapToScene(e -> pos()));
			is_adding_text = false;
		}
		QGraphicsView::mousePressEvent(e);
	}
}

/**
	Gere les actions liees a la rollette de la souris
	@param e QWheelEvent decrivant l'evenement rollette
*/
void DiagramView::wheelEvent(QWheelEvent *e) {
	// si la touche Ctrl est enfoncee, on zoome / dezoome
	if (e -> modifiers() & Qt::ControlModifier) {
		if (e -> delta() > 0) {
			zoomIn();
		} else {
			zoomOut();
		}
	} else {
		QAbstractScrollArea::wheelEvent(e);
	}
}

/**
	@return le titre de cette vue ; cela correspond au titre du schema
	visualise precede de la mention "Schema". Si le titre du schema est vide,
	la mention "Schema sans titre" est utilisee
	@see Diagram::title()
*/
QString DiagramView::title() const {
	QString view_title;
	QString diagram_title(scene -> title());
	if (diagram_title.isEmpty()) {
		view_title = tr("Sch\351ma sans titre");
	} else {
		view_title = QString(tr("Sch\351ma %1", "%1 is a diagram title")).arg(diagram_title);
	}
	return(view_title);
}

/**
	Edite les informations du schema.
*/
void DiagramView::editDiagramProperties() {
	if (scene -> isReadOnly()) return;
	
	// recupere le cartouche et les dimensions du schema
	InsetProperties  inset  = scene -> border_and_inset.exportInset();
	BorderProperties border = scene -> border_and_inset.exportBorder();
	
	// construit le dialogue
	QDialog popup(diagramEditor());
#ifdef Q_WS_MAC
	popup.setWindowFlags(Qt::Sheet);
#endif
	
	popup.setMinimumWidth(400);
	popup.setWindowTitle(tr("Propri\351t\351s du sch\351ma", "window title"));
	
	BorderPropertiesWidget *border_infos = new BorderPropertiesWidget(border, &popup);
	InsetPropertiesWidget  *inset_infos  = new InsetPropertiesWidget(inset, false, &popup);
	
	// boutons
	QDialogButtonBox boutons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(&boutons, SIGNAL(accepted()), &popup, SLOT(accept()));
	connect(&boutons, SIGNAL(rejected()), &popup, SLOT(reject()));
	
	// ajout dans une disposition verticale
	QVBoxLayout layout_v(&popup);
	layout_v.addWidget(border_infos);
	layout_v.addWidget(inset_infos);
	layout_v.addStretch();
	layout_v.addWidget(&boutons);
	// si le dialogue est accepte
	if (popup.exec() == QDialog::Accepted) {
		InsetProperties new_inset   = inset_infos  -> insetProperties();
		BorderProperties new_border = border_infos -> borderProperties();
		// s'il y a des modifications au cartouche
		if (new_inset != inset) {
			scene -> undoStack().push(new ChangeInsetCommand(scene, inset, new_inset));
		}
		
		// s'il y a des modifications aux dimensions du schema
		if (new_border != border) {
			scene -> undoStack().push(new ChangeBorderCommand(scene, border, new_border));
		}
	}
}

/**
	@return true s'il y a des elements selectionnes sur le schema, false sinon
*/
bool DiagramView::hasSelectedItems() {
	return(scene -> selectedItems().size() > 0);
}

/**
	Ajoute une colonne au schema.
*/
void DiagramView::addColumn() {
	if (scene -> isReadOnly()) return;
	BorderProperties old_bp = scene -> border_and_inset.exportBorder();
	BorderProperties new_bp = scene -> border_and_inset.exportBorder();
	new_bp.columns_count += 1;
	scene -> undoStack().push(new ChangeBorderCommand(scene, old_bp, new_bp));
}

/**
	Enleve une colonne au schema.
*/
void DiagramView::removeColumn() {
	if (scene -> isReadOnly()) return;
	BorderProperties old_bp = scene -> border_and_inset.exportBorder();
	BorderProperties new_bp = scene -> border_and_inset.exportBorder();
	new_bp.columns_count -= 1;
	scene -> undoStack().push(new ChangeBorderCommand(scene, old_bp, new_bp));
}

/**
	Agrandit le schema en hauteur
*/
void DiagramView::addRow() {
	if (scene -> isReadOnly()) return;
	BorderProperties old_bp = scene -> border_and_inset.exportBorder();
	BorderProperties new_bp = scene -> border_and_inset.exportBorder();
	new_bp.rows_count += 1;
	scene -> undoStack().push(new ChangeBorderCommand(scene, old_bp, new_bp));
}

/**
	Retrecit le schema en hauteur
*/
void DiagramView::removeRow() {
	if (scene -> isReadOnly()) return;
	BorderProperties old_bp = scene -> border_and_inset.exportBorder();
	BorderProperties new_bp = scene -> border_and_inset.exportBorder();
	new_bp.rows_count -= 1;
	scene -> undoStack().push(new ChangeBorderCommand(scene, old_bp, new_bp));
}

/**
	Ajuste le sceneRect (zone du schema visualisee par le DiagramView) afin que
	celui inclut a la fois les elements dans et en dehors du cadre et le cadre
	lui-meme.
*/
void DiagramView::adjustSceneRect() {
	QRectF old_scene_rect = sceneRect();
	
	// rectangle delimitant l'ensemble des elements
	QRectF elements_bounding_rect = scene -> itemsBoundingRect();
	
	// rectangle contenant le cadre = colonnes + cartouche
	QRectF border_bounding_rect = scene -> border().adjusted(-Diagram::margin, -Diagram::margin, Diagram::margin, Diagram::margin);
	
	// ajuste la sceneRect
	QRectF new_scene_rect = elements_bounding_rect.united(border_bounding_rect);
	setSceneRect(new_scene_rect);
	
	// met a jour la scene
	scene -> update(old_scene_rect.united(new_scene_rect));
}

/**
	Met a jour le titre du widget
*/
void DiagramView::updateWindowTitle() {
	QString view_title(title());
	
	// verifie si le document a ete modifie
	bool modified_diagram = !(scene -> undoStack().isClean());
	
	// specifie le titre du widget
	setWindowTitle(view_title + " [*]");
	setWindowModified(modified_diagram);
	
	// emet le signal titleChanged en ajoutant manuellement [*] si le schema a ete modifie
	QString emitted_title = view_title;
	if (modified_diagram) emitted_title += " [*]";
	emit(titleChanged(this, emitted_title));
}

/**
	Active ou desactive le dessin de grille selon la quantite de pixels affichee
*/
void DiagramView::adjustGridToZoom() {
	QRectF viewed_scene = viewedSceneRect();
	scene -> setDisplayGrid(viewed_scene.width() < 2000 || viewed_scene.height() < 2000);
}

/**
	@return le rectangle du schema (classe Diagram) visualise par ce DiagramView
*/
QRectF DiagramView::viewedSceneRect() const {
	// recupere la taille du widget viewport
	QSize viewport_size = viewport() -> size();
	
	// recupere la transformation viewport -> scene
	QTransform view_to_scene   = viewportTransform().inverted();
	
	// mappe le coin superieur gauche et le coin inferieur droit de la viewport sur la scene
	QPointF scene_left_top     = view_to_scene.map(QPointF(0.0, 0.0));
	QPointF scene_right_bottom = view_to_scene.map(QPointF(viewport_size.width(), viewport_size.height()));
	
	// en deduit le rectangle visualise par la scene
	return(QRectF(scene_left_top, scene_right_bottom));
}

/**
	Cette methode permet de determiner s'il faut ou non integrer au projet un
	element dont on connait l'emplacement.
	L'element droppe est integre a la collection du projet :
	  * s'il appartient a un autre projet, quelque soit la specification de
	  l'utilisateur a ce propos ;
	  * s'il appartient a la collection commune ou a la collection
	  personnelle ET que l'utilisateur a autorise l'integration automatique
	  des elements dans les projets.
	@param location Emplacement de l'element
	@return true si l'element doit etre integre, false sinon
	
*/
bool DiagramView::mustIntegrateElement(const ElementsLocation &location) const {
	// l'utilisateur a-t-il autorise l'integration automatique des elements dans les projets ?
	bool auto_integration_enabled = QETApp::settings().value("diagrameditor/integrate-elements", true).toBool();
	
	// l'element appartient-il a un projet et si oui, est-ce un autre projet ?
	bool elmt_from_project = location.project();
	bool elmt_from_another_project = elmt_from_project && location.project() != scene -> project();
	
	// faut-il integrer l'element ?
	bool must_integrate_element = (elmt_from_another_project || (auto_integration_enabled && !elmt_from_project));
	
	return(must_integrate_element);
}

/**
	@param location Emplacement de l'element a ajouter sur le schema
	@param pos Position (dans les coordonnees de la vue) a laquelle l'element sera ajoute
*/
bool DiagramView::addElementAtPos(const ElementsLocation &location, const QPoint &pos) {
	// construit une instance de l'element correspondant a l'emplacement
	int state;
	Element *el = new CustomElement(location, 0, 0, &state);
	if (state) {
		delete el;
		return(false);
	}
	
	// pose de l'element sur le schema
	diagram() -> undoStack().push(new AddElementCommand(diagram(), el, mapToScene(pos)));
	return(true);
}

/**
	Fait en sorte que le schema ne soit editable que s'il n'est pas en lecture
	seule
*/
void DiagramView::applyReadOnly() {
	if (!scene) return;
	
	bool is_writable = !scene -> isReadOnly();
	setInteractive(is_writable);
	setAcceptDrops(is_writable);
}

/**
	Edite les proprietes des objets selectionnes
*/
void DiagramView::editSelectionProperties() {
	// recupere la selection
	DiagramContent selection = scene -> selectedContent();
	
	// s'il n'y a rien de selectionne, cette methode ne fait rien
	int selected_items_count = selection.count(DiagramContent::All | DiagramContent::SelectedOnly);
	if (!selected_items_count) return;
	
	// si la selection ne comprend qu'un seul objet, on l'edite via un dialogue approprie
	if (selected_items_count == 1) {
		// cas d'un conducteur selectionne
		QList<Conductor *> selected_conductors = selection.conductors(DiagramContent::AnyConductor | DiagramContent::SelectedOnly);
		if (selected_conductors.count() == 1) {
			editConductor(selected_conductors.at(0));
			return;
		}
		
		// cas d'un element selectionne
		if (selection.elements.count() == 1) {
			editElement(selection.elements.at(0));
			return;
		}
		
		// cas d'un champ de texte selectionne : pour le moment, on traite comme une selection multiple
	}
	
	// sinon on affiche un simple listing des elements selectionnes
	QET::MessageBox::information(
		this,
		tr("Propri\351t\351s de la s\351lection"),
		QString(
			tr(
				"La s\351lection contient %1.",
				"%1 is a sentence listing the selected objects"
			)
		).arg(selection.sentence(DiagramContent::All | DiagramContent::SelectedOnly))
	);
}

/**
	Affiche des informations sur un element
	@param element Element a afficher
*/
void DiagramView::editElement(Element *element) {
	if (!element) return;
	
	CustomElement *custom_element = qobject_cast<CustomElement *>(element);
	GhostElement  *ghost_element  = qobject_cast<GhostElement  *>(element);
	
	// type de l'element
	QString description_string;
	if (ghost_element) {
		description_string += tr("\311l\351ment manquant");
	} else {
		description_string += tr("\311l\351ment");
	}
	description_string += "\n";
	
	// nom,  nombre de bornes, dimensions
	description_string += QString(tr("Nom\240: %1\n")).arg(element -> name());
	description_string += QString(tr("Position\240: %1\n")).arg(scene -> convertPosition(element -> scenePos()).toString());
	description_string += QString(tr("Dimensions\240: %1\327%2\n")).arg(element -> size().width()).arg(element -> size().height());
	description_string += QString(tr("Bornes\240: %1\n")).arg(element -> terminals().count());
	description_string += QString(tr("Connexions internes\240: %1\n")).arg(element -> internalConnections() ? tr("Autoris\351es") : tr("Interdites"));
	description_string += QString(tr("Champs de texte\240: %1\n")).arg(element -> texts().count());
	
	if (custom_element) {
		description_string += QString(tr("Emplacement\240: %1\n")).arg(custom_element -> location().toString());
	}
	
	// titre et boutons du dialogue
	QString description_title = tr("Propri\351t\351s de l'\351l\351ment s\351lectionn\351");
	QPushButton *find_in_panel = new QPushButton(tr("Retrouver dans le panel"));
	QPushButton *edit_element = new QPushButton(tr("\311diter l'\351l\351ment"));
	edit_element->setIcon(QET::Icons::ElementEdit);
	
	// dialogue en lui-meme
	QMessageBox edit_element_dialog(diagramEditor());
#ifdef Q_WS_MAC
	edit_element_dialog.setWindowFlags(Qt::Sheet);
#endif
	edit_element_dialog.setIcon(QMessageBox::Information);
	edit_element_dialog.setWindowTitle(description_title);
	edit_element_dialog.setText(description_title);
	edit_element_dialog.setInformativeText(description_string);
	edit_element_dialog.addButton(find_in_panel, QMessageBox::ApplyRole);
	edit_element_dialog.addButton(edit_element, QMessageBox::ApplyRole);
	edit_element_dialog.addButton(QMessageBox::Ok);
	edit_element_dialog.setDefaultButton(QMessageBox::Ok);
	edit_element_dialog.setEscapeButton(QMessageBox::Ok);
	edit_element_dialog.exec();
	
	// Permet de trouver l'element dans la collection
	if (edit_element_dialog.clickedButton() == find_in_panel) {
		emit(findElementRequired(custom_element -> location()));
	}
	
	// Trouve l'element dans la collection et l'edite
	if (edit_element_dialog.clickedButton() == edit_element) {
		emit(findElementRequired(custom_element -> location()));
		emit(editElementRequired(custom_element -> location()));
	}
}

/**
	Affiche un dialogue permettant d'editer le conducteur selectionne.
	Ne fait rien s'il y a 0 ou plusieurs conducteurs selectionnes.
*/
void DiagramView::editConductor() {
	QList<Conductor *> selected_conductors(scene -> selectedConductors().toList());
	
	// on ne peut editer qu'un conducteur a la fois
	if (selected_conductors.count() != 1) return;
	Conductor *edited_conductor = selected_conductors.first();
	
	editConductor(edited_conductor);
}

/**
	Edite le conducteur passe en parametre
	@param edited_conductor Conducteur a editer
*/
void DiagramView::editConductor(Conductor *edited_conductor) {
	if (scene -> isReadOnly()) return;
	if (!edited_conductor) return;
	
	// initialise l'editeur de proprietes pour le conducteur
	ConductorProperties old_properties = edited_conductor -> properties();
	ConductorPropertiesWidget *cpw = new ConductorPropertiesWidget(old_properties);
	
	// l'insere dans un dialogue
	QDialog conductor_dialog(diagramEditor());
#ifdef Q_WS_MAC
	conductor_dialog.setWindowFlags(Qt::Sheet);
#endif
	conductor_dialog.setWindowTitle(tr("\311diter les propri\351t\351s d'un conducteur", "window title"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&conductor_dialog);
	dialog_layout -> addWidget(cpw);
	dialog_layout -> addStretch();
	QDialogButtonBox *dbb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dbb);
	connect(dbb, SIGNAL(accepted()), &conductor_dialog, SLOT(accept()));
	connect(dbb, SIGNAL(rejected()), &conductor_dialog, SLOT(reject()));
	
	// execute le dialogue et met a jour le conducteur
	if (conductor_dialog.exec() == QDialog::Accepted) {
		// recupere les nouvelles proprietes
		ConductorProperties new_properties = cpw -> conductorProperties();
		
		if (new_properties != old_properties) {
			// initialise l'objet UndoCommand correspondant
			ChangeConductorPropertiesCommand *ccpc = new ChangeConductorPropertiesCommand(edited_conductor);
			ccpc -> setOldSettings(old_properties);
			ccpc -> setNewSettings(new_properties);
			diagram() -> undoStack().push(ccpc);
		}
	}
}

/**
	Reinitialise le profil des conducteurs selectionnes
*/
void DiagramView::resetConductors() {
	if (scene -> isReadOnly()) return;
	// recupere les conducteurs selectionnes
	QSet<Conductor *> selected_conductors = scene -> selectedConductors();
	
	// repere les conducteurs modifies (= profil non nul)
	QHash<Conductor *, ConductorProfilesGroup> conductors_and_profiles;
	foreach(Conductor *conductor, selected_conductors) {
		ConductorProfilesGroup profile = conductor -> profiles();
		if (
			!profile[Qt::TopLeftCorner].isNull() ||\
			!profile[Qt::TopRightCorner].isNull() ||\
			!profile[Qt::BottomLeftCorner].isNull() ||\
			!profile[Qt::BottomRightCorner].isNull()
		) {
			conductors_and_profiles.insert(conductor, profile);
		}
	}
	
	if (conductors_and_profiles.isEmpty()) return;
	scene -> undoStack().push(new ResetConductorCommand(conductors_and_profiles));
}

/**
	Lance un dialogue permettant de modifier les proprietes par defaut des
	futurs nouveaux conducteurs
*/
void DiagramView::editDefaultConductorProperties() {
	if (scene -> isReadOnly()) return;
	// initialise l'editeur de proprietes pour le conducteur
	ConductorPropertiesWidget *cpw = new ConductorPropertiesWidget(scene -> defaultConductorProperties);
	
	// l'insere dans un dialogue
	QDialog conductor_dialog(diagramEditor());
#ifdef Q_WS_MAC
	conductor_dialog.setWindowFlags(Qt::Sheet);
#endif
	
	conductor_dialog.setWindowTitle(tr("\311diter les propri\351t\351s par d\351faut des conducteurs", "window title"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&conductor_dialog);
	dialog_layout -> addWidget(cpw);
	QDialogButtonBox *dbb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dbb);
	connect(dbb, SIGNAL(accepted()), &conductor_dialog, SLOT(accept()));
	connect(dbb, SIGNAL(rejected()), &conductor_dialog, SLOT(reject()));
	
	// execute le dialogue et met a jour le conducteur
	if (conductor_dialog.exec() == QDialog::Accepted) {
		scene -> defaultConductorProperties = cpw -> conductorProperties();
	}
}

/**
	Gere les evenements de la DiagramView
	@param e Evenement
*/
bool DiagramView::event(QEvent *e) {
	// fait en sorte que les raccourcis clavier arrivent prioritairement sur la
	// vue plutot que de remonter vers les QMenu / QAction
	if (e -> type() == QEvent::ShortcutOverride && scene -> focusItem()) {
		e -> accept();
		return(true);
	}
	return(QGraphicsView::event(e));
}

/**
	Passe le DiagramView en mode "ajout de texte". Un clic cree alors un
	nouveau champ de texte.
*/
void DiagramView::addText() {
	if (scene -> isReadOnly()) return;
	is_adding_text = true;
}

/**
	Cree un nouveau champ de texte et le place a la position pos
	en gerant l'annulation ; enfin, le signal textAdded est emis.
	@param pos Position du champ de texte ajoute
	@return le champ de texte ajoute
*/
DiagramTextItem *DiagramView::addDiagramTextAtPos(const QPointF &pos) {
	// cree un nouveau champ de texte
	DiagramTextItem *dti = new DiagramTextItem();
	dti -> setPlainText("_");
	dti -> previous_text = "_";

	// le place a la position pos en gerant l'annulation
	scene -> undoStack().push(new AddTextCommand(scene, dti, pos));
	adjustSceneRect();

	// emet le signal textAdded
	emit(textAdded(false));

	return(dti);
}

/**
	Gere le menu contextuel
	@param e Evenement decrivant la demande de menu contextuel
*/
void DiagramView::contextMenuEvent(QContextMenuEvent *e) {
	if (QGraphicsItem *qgi = scene -> itemAt(mapToScene(e -> pos()))) {
		if (!qgi -> isSelected()) scene -> clearSelection();
		qgi -> setSelected(true);
	}
	
	if (QETDiagramEditor *qde = diagramEditor()) {
		context_menu -> clear();
		if (scene -> selectedItems().isEmpty()) {
			paste_here_pos = e -> pos();
			paste_here -> setEnabled(Diagram::clipboardMayContainDiagram());
			context_menu -> addAction(paste_here);
			context_menu -> addSeparator();
			context_menu -> addAction(qde -> infos_diagram);
			context_menu -> addAction(qde -> add_column);
			context_menu -> addAction(qde -> remove_column);
			context_menu -> addAction(qde -> add_row);
			context_menu -> addAction(qde -> remove_row);
		} else {
			context_menu -> addAction(qde -> cut);
			context_menu -> addAction(qde -> copy);
			context_menu -> addSeparator();
			context_menu -> addAction(qde -> conductor_reset);
			context_menu -> addSeparator();
			context_menu -> addAction(qde -> delete_selection);
			context_menu -> addAction(qde -> rotate_selection);
			context_menu -> addAction(qde -> selection_prop);
		}
		
		// affiche le menu contextuel
		context_menu -> popup(e -> globalPos());
	}
	e -> accept();
}

/**
	@return l'editeur de schemas parent ou 0
*/
QETDiagramEditor *DiagramView::diagramEditor() const {
	// remonte la hierarchie des widgets
	QWidget *w = const_cast<DiagramView *>(this);
	while (w -> parentWidget() && !w -> isWindow()) {
		w = w -> parentWidget();
	}
	// la fenetre est supposee etre un QETDiagramEditor
	return(qobject_cast<QETDiagramEditor *>(w));
}

/**
	Gere les double-clics sur le schema
*/
void DiagramView::mouseDoubleClickEvent(QMouseEvent *e) {
	BorderInset &bi = scene -> border_and_inset;
	
	// recupere le rectangle corespondant au cartouche
	QRectF inset_rect(
		Diagram::margin,
		Diagram::margin + bi.diagramHeight(),
		bi.insetWidth(),
		bi.insetHeight()
	);
	
	// recupere le rectangle correspondant aux en-tetes des colonnes
	QRectF columns_rect(
		Diagram::margin,
		Diagram::margin,
		bi.borderWidth(),
		bi.columnsHeaderHeight()
	);
	
	// recupere le rectangle correspondant aux en-tetes des lignes
	QRectF rows_rect(
		Diagram::margin,
		Diagram::margin,
		bi.rowsHeaderWidth(),
		bi.diagramHeight()
	);
	
	// coordonnees du clic par rapport au schema
	QPointF click_pos = viewportTransform().inverted().map(e -> pos());
	
	// detecte le double-clic sur le cartouche ou les colonnes
	if (QGraphicsItem *qgi = itemAt(e -> pos())) {
		if (Conductor *c = qgraphicsitem_cast<Conductor *>(qgi)) {
			editConductor(c);
		} else if (Element *element = qgraphicsitem_cast<Element *>(qgi)) {
			editElement(element);
		} else {
			QGraphicsView::mouseDoubleClickEvent(e);
		}
	} else if (inset_rect.contains(click_pos) || columns_rect.contains(click_pos) || rows_rect.contains(click_pos)) {
		// edite les proprietes du schema
		editDiagramProperties();
	} else {
		QGraphicsView::mouseDoubleClickEvent(e);
	}
}

/**
	Cette methode ajoute l'element deisgne par l'emplacement location a la
	position pos. Si necessaire, elle demande l'integration de l'element au
	projet.
	@see mustIntegrateElement
*/
void DiagramView::addDroppedElement() {
	ElementsLocation location = next_location_;
	QPoint pos = next_position_;
	
	if (!mustIntegrateElement(location)) {
		addElementAtPos(location, pos);
	} else {
		QString error_msg;
		IntegrationMoveElementsHandler *integ_handler = new IntegrationMoveElementsHandler(this);
		QString integ_path = scene -> project() -> integrateElement(location.toString(), integ_handler, error_msg);
		delete integ_handler;
		if (integ_path.isEmpty()) {
			qDebug() << "DiagramView::addDroppedElement : Impossible d'ajouter l'element. Motif : " << qPrintable(error_msg);
			return;
		}
		addElementAtPos(ElementsLocation::locationFromString(integ_path), pos);
	}
	adjustSceneRect();
}
