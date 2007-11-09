#include "diagramview.h"
#include "diagram.h"
#include "customelement.h"
#include "exportdialog.h"
#include "conductor.h"
#include "diagramcommands.h"
#include "conductorpropertieswidget.h"

/**
	Constructeur
	@param parent Le QWidegt parent de cette vue de schema
*/
DiagramView::DiagramView(QWidget *parent) : QGraphicsView(parent), is_adding_text(false) {
	setInteractive(true);
	setCacheMode(QGraphicsView::CacheBackground);
	setOptimizationFlags(QGraphicsView::DontSavePainterState|QGraphicsView::DontAdjustForAntialiasing);
	
	// active l'antialiasing
	setRenderHint(QPainter::Antialiasing, true);
	setRenderHint(QPainter::TextAntialiasing, true);
	setRenderHint(QPainter::SmoothPixmapTransform, true);
	
	setScene(scene = new Diagram(this));
	scene -> undoStack().setClean();
	setDragMode(RubberBandDrag);
	setAcceptDrops(true);
	setWindowIcon(QIcon(":/ico/qet-16.png"));
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	setAlignment(Qt::AlignLeft | Qt::AlignTop);
	adjustSceneRect();
	updateWindowTitle();
	
	context_menu = new QMenu(this);
	paste_here = new QAction(QIcon(":/ico/paste.png"), tr("Coller ici"), this);
	connect(paste_here, SIGNAL(triggered()), this, SLOT(pasteHere()));
	
	connect(scene, SIGNAL(selectionEmptinessChanged()), this, SLOT(slot_selectionChanged()));
	connect(&(scene -> border_and_inset), SIGNAL(borderChanged(QRectF, QRectF)), this, SLOT(adjustSceneRect()));
	connect(&(scene -> undoStack()), SIGNAL(cleanChanged(bool)), this, SLOT(updateWindowTitle()));
}

/**
	Destructeur
*/
DiagramView::~DiagramView() {
}

/**
	appelle la methode select sur tous les elements de la liste d'elements
*/
void DiagramView::selectAll() {
	if (scene -> items().isEmpty()) return;
	QPainterPath path;
	path.addRect(scene -> itemsBoundingRect());
	scene -> setSelectionArea(path);
}

/**
	appelle la methode deselect sur tous les elements de la liste d'elements
*/
void DiagramView::selectNothing() {
	if (scene -> items().isEmpty()) return;
	scene -> clearSelection();
}

/**
	Inverse l'etat de selection de tous les elements de la liste d'elements
 */
void DiagramView::selectInvert() {
	if (scene -> items().isEmpty()) return;
	foreach (QGraphicsItem *item, scene -> items()) item -> setSelected(!item -> isSelected());
}

/**
	Supprime les composants selectionnes
*/
void DiagramView::deleteSelection() {
	// si un item a le focus et que ce slot est appele, c'est sans doute parce
	// que la touche suppr a ete enfoncee pour effacer une lettre et non la
	// selection
	if (scene -> focusItem()) return;

	DiagramContent removed_content = scene -> selectedContent();
	scene -> clearSelection();
	scene -> undoStack().push(new DeleteElementsCommand(scene, removed_content));
}

/**
	Pivote les composants selectionnes
*/
void DiagramView::rotateSelection() {
	QHash<Element *, QET::Orientation> elements_to_rotate;
	foreach (QGraphicsItem *item, scene -> selectedItems()) {
		if (Element *e = qgraphicsitem_cast<Element *>(item)) {
			elements_to_rotate.insert(e, e -> orientation().current());
		}
	}
	if (elements_to_rotate.isEmpty()) return;
	scene -> undoStack().push(new RotateElementsCommand(elements_to_rotate));
}

/**
	accepte ou refuse le drag'n drop en fonction du type de donnees entrant
	@param e le QDragEnterEvent correspondant au drag'n drop tente
*/
void DiagramView::dragEnterEvent(QDragEnterEvent *e) {
	if (e -> mimeData() -> hasFormat("text/plain")) e -> acceptProposedAction();
	else e-> ignore();
}

/**
	gere les dragleaveevent
	@param e le QDragEnterEvent correspondant au drag'n drop sortant
*/
void DiagramView::dragLeaveEvent(QDragLeaveEvent *) {
}

/**
	accepte ou refuse le drag'n drop en fonction du type de donnees entrant
	@param e le QDragMoveEvent correspondant au drag'n drop tente
*/
void DiagramView::dragMoveEvent(QDragMoveEvent *e) {
	if (e -> mimeData() -> hasFormat("text/plain")) e -> acceptProposedAction();
	else e-> ignore();
}

/**
	gere les depots (drop) acceptes sur le Diagram
	@param e le QDropEvent correspondant au drag'n drop effectue
*/
void DiagramView::dropEvent(QDropEvent *e) {
	QString fichier = e -> mimeData() -> text();
	int etat;
	Element *el = new CustomElement(fichier, 0, 0, &etat);
	if (etat) delete el;
	else {
		diagram() -> undoStack().push(new AddElementCommand(diagram(), el, mapToScene(e -> pos())));
	}
}

/**
	Passe le Diagram en mode visualisation
*/
void DiagramView::setVisualisationMode() {
	setDragMode(ScrollHandDrag);
	setInteractive(false);
	emit(modeChanged());
}

/**
	Passe le Diagram en mode Selection
*/
void DiagramView::setSelectionMode() {
	setDragMode(RubberBandDrag);
	setInteractive(true);
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
	copie les elements selectionnes du schema dans le presse-papier puis les supprime
*/
void DiagramView::cut() {
	copy();
	DiagramContent removed_content = scene -> selectedContent();
	scene -> clearSelection();
	scene -> undoStack().push(new CutDiagramCommand(scene, scene -> selectedContent()));
}

/**
	copie les elements selectionnes du schema dans le presse-papier
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
	}
}

/// Colle le contenu du presse-papier sur le schema a la position de la souris
void DiagramView::pasteHere() {
	paste(mapToScene(paste_here_pos));
}

/**
	gere les clics et plus particulierement le clic du milieu (= coller pour X11)
*/
void DiagramView::mousePressEvent(QMouseEvent *e) {
	if (e -> buttons() == Qt::MidButton) {
		paste(mapToScene(e -> pos()), QClipboard::Selection);
	} else {
		if (is_adding_text && e -> buttons() == Qt::LeftButton) {
			DiagramTextItem *dti = new DiagramTextItem();
			dti -> setPlainText("_");
			dti -> previous_text = "_";
			scene -> undoStack().push(new AddTextCommand(scene, dti, e -> pos()));
			is_adding_text = false;
			emit(textAdded(false));
		}
		QGraphicsView::mousePressEvent(e);
	}
}

/**
	Ouvre un fichier *.qet dans cette DiagramView
	@param n_fichier Nom du fichier a ouvrir
	@param erreur Si le pointeur est specifie, cet entier est mis a 0 en cas de reussite de l'ouverture, 1 si le fichier n'existe pas, 2 si le fichier n'est pas lisible, 3 si le fichier n'est pas un element XML, 4 si l'ouverture du fichier a echoue pour une autre raison (c'est pas ca qui manque ^^)
	@return true si l'ouverture a reussi, false sinon
*/
bool DiagramView::open(QString n_fichier, int *erreur) {
	// verifie l'existence du fichier
	if (!QFileInfo(n_fichier).exists()) {
		if (erreur != NULL) *erreur = 1;
		return(false);
	}
	
	// ouvre le fichier
	QFile fichier(n_fichier);
	if (!fichier.open(QIODevice::ReadOnly)) {
		if (erreur != NULL) *erreur = 2;
		return(false);
	}
	
	// lit son contenu dans un QDomDocument
	QDomDocument document;
	if (!document.setContent(&fichier)) {
		if (erreur != NULL) *erreur = 3;
		fichier.close();
		return(false);
	}
	fichier.close();
	
	// construit le schema a partir du QDomDocument
	QDomDocument &doc = document;
	if (scene -> fromXml(doc)) {
		if (erreur != NULL) *erreur = 0;
		file_name = n_fichier;
		scene -> undoStack().setClean();
		updateWindowTitle();
		return(true);
	} else {
		if (erreur != NULL) *erreur = 4;
		return(false);
	}
}

/**
	Slot appele lorsque la selection change.
*/
void DiagramView::slot_selectionChanged() {
	emit(selectionChanged());
}

/**
	Gere la fermeture du schema.
	@param event Le QCloseEvent decrivant l'evenement
*/
void DiagramView::closeEvent(QCloseEvent *event) {
	bool retour;
	// si le schema est modifie
	if (!isWindowModified()) {
		retour = true;
	} else {
		// demande d'abord a l'utilisateur s'il veut enregistrer le schema en cours
		QMessageBox::StandardButton reponse = QMessageBox::question(
			this,
			tr("Enregistrer le sch\351ma en cours ?"),
			tr("Voulez-vous enregistrer le sch\351ma ") + windowTitle() + tr(" ?"),
			QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
			QMessageBox::Cancel
		);
		switch(reponse) {
			case QMessageBox::Cancel: retour = false;         break; // l'utilisateur annule : echec de la fermeture
			case QMessageBox::Yes:    retour = save(); break; // l'utilisateur dit oui : la reussite depend de l'enregistrement
			default:                  retour = true;                 // l'utilisateur dit non ou ferme le dialogue: c'est reussi
		}
	}
	if (retour) {
		event -> accept();
		delete this;
	} else event -> ignore();
	
}

/**
	Methode enregistrant le schema dans le dernier nom de fichier connu.
	Si aucun nom de fichier n'est connu, cette methode appelle la methode saveAs
	@return true si l'enregistrement a reussi, false sinon
*/
bool DiagramView::save() {
	if (file_name.isEmpty()) return(saveAs());
	else return(saveDiagramToFile(file_name));
}

/**
	Cette methode demande un nom de fichier a l'utilisateur pour enregistrer le schema
	Si aucun nom n'est entre, elle renvoie faux.
	Si le nom ne se termine pas par l'extension .qet, celle-ci est ajoutee.
	Si l'enregistrement reussit, le nom du fichier est conserve et la fonction renvoie true.
	Sinon, faux est renvoye.
	@return true si l'enregistrement a reussi, false sinon
*/
bool DiagramView::saveAs() {
	// demande un nom de fichier a l'utilisateur pour enregistrer le schema
	QString n_fichier = QFileDialog::getSaveFileName(
		this,
		tr("Enregistrer sous"),
		(file_name.isEmpty() ? QDir::homePath() : QDir(file_name)).absolutePath(),
		tr("Sch\351ma QElectroTech (*.qet)")
	);
	// si aucun nom n'est entre, renvoie faux.
	if (n_fichier.isEmpty()) return(false);
	// si le nom ne se termine pas par l'extension .qet, celle-ci est ajoutee
	if (!n_fichier.endsWith(".qet", Qt::CaseInsensitive)) n_fichier += ".qet";
	// tente d'enregistrer le fichier
	bool resultat_enregistrement = saveDiagramToFile(n_fichier);
	// si l'enregistrement reussit, le nom du fichier est conserve
	if (resultat_enregistrement) {
		file_name = n_fichier;
		updateWindowTitle();
	}
	// retourne un booleen representatif de la reussite de l'enregistrement
	return(resultat_enregistrement);
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
	Methode privee gerant l'enregistrement du fichier XML. S'il n'est pas possible
	d'ecrire dans le fichier, cette fonction affiche un message d'erreur et renvoie false.
	Autrement, elle renvoie true.
	@param n_fichier Nom du fichier dans lequel l'arbre XML doit etre ecrit
	@return true si l'enregistrement a reussi, false sinon
*/
bool DiagramView::saveDiagramToFile(QString &n_fichier) {
	QFile fichier(n_fichier);
	if (!fichier.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'ecrire dans ce fichier"));
		return(false);
	}
	QTextStream out(&fichier);
	out.setCodec("UTF-8");
	out << scene -> toXml().toString(4);
	fichier.close();
	scene -> undoStack().setClean();
	return(true);
}

/**
	Exporte le schema.
*/
void DiagramView::dialogExport() {
	ExportDialog ed(scene, this);
	ed.exec();
}

/**
	Imprime le schema.
*/
void DiagramView::dialogPrint() {
	// initialise l'acces a l'imprimante
	QPrinter qprin;
	qprin.setOutputFormat(QPrinter::PdfFormat);
	qprin.setOrientation(QPrinter::Landscape);
	qprin.setPageSize(QPrinter::A4);
	QPrintDialog qpd(&qprin, this);
	
	if (qpd.exec() == QDialog::Accepted) {
		QPainter qp(&qprin);
		// impression physique (!= fichier PDF)
		if (qprin.outputFileName().isEmpty()) {
			// lorsqu'on imprime en paysage sur imprimante reelle, il faut pivoter soi-meme le rendu
			if (qprin.orientation() == QPrinter::Landscape) {
				qp.rotate(90.0);
				qp.translate(0.0, -qprin.pageRect().height());
			}
		}
		scene -> setDisplayGrid(false);
		scene -> setDrawTerminals(false);
		scene -> render(&qp, QRectF(), scene -> border(), Qt::KeepAspectRatio);
		scene -> setDrawTerminals(true);
		scene -> setDisplayGrid(true);
	}
}

/**
	Edite les informations du schema.
*/
void DiagramView::dialogEditInfos() {
	// recupere le cartouche du schema
	InsetProperties inset = scene -> border_and_inset.exportInset();
	
	// recupere les dimensions du schema
	int columns_count_value  = scene -> border_and_inset.nbColumn();
	int columns_width_value  = qRound(scene -> border_and_inset.columnsWidth());
	int columns_height_value = qRound(scene -> border_and_inset.columnsHeight());
	
	// construit le dialogue
	QDialog popup;
	popup.setMinimumWidth(400);
	popup.setWindowTitle(tr("Propri\351t\351s du sch\351ma"));
	
	QGroupBox *diagram_size_box = new QGroupBox(tr("Dimensions du sch\351ma"), &popup);
	QGridLayout diagram_size_box_layout(diagram_size_box);
	
	QLabel *ds1 = new QLabel(tr("Colonnes :"));
	
	QSpinBox *columns_count  = new QSpinBox(diagram_size_box);
	columns_count -> setMinimum(scene -> border_and_inset.minNbColumns());
	columns_count -> setValue(columns_count_value);
	
	QSpinBox *columns_width  = new QSpinBox(diagram_size_box);
	columns_width -> setMinimum(1);
	columns_width -> setSingleStep(10);
	columns_width -> setValue(columns_width_value);
	columns_width -> setPrefix(tr("\327"));
	columns_width -> setSuffix(tr("px"));
	
	QLabel *ds2 = new QLabel(tr("Hauteur :"));
	
	QSpinBox *columns_height = new QSpinBox(diagram_size_box);
	columns_height -> setRange(qRound(scene -> border_and_inset.minColumnsHeight()), 10000);
	columns_height -> setSingleStep(80);
	columns_height -> setValue(columns_height_value);
	
	diagram_size_box_layout.addWidget(ds1,            0, 0);
	diagram_size_box_layout.addWidget(columns_count,  0, 1);
	diagram_size_box_layout.addWidget(columns_width,  0, 2);
	diagram_size_box_layout.addWidget(ds2,            1, 0);
	diagram_size_box_layout.addWidget(columns_height, 1, 1);
	
	QGroupBox *inset_infos = new QGroupBox(tr("Informations du cartouche"), &popup);
	
	QLineEdit *titre = new QLineEdit(inset.title, &popup);
	QLineEdit *auteur = new QLineEdit(inset.author, &popup);
	QDate date_diagram = QDate(inset.date);
	if (date_diagram.isNull() || !date_diagram.isValid()) date_diagram = QDate::currentDate();
	QDateEdit *date = new QDateEdit(date_diagram, &popup);
	date -> setCalendarPopup(true);
	QLineEdit *fichier = new QLineEdit(inset.filename, &popup);
	QLineEdit *folio = new QLineEdit(inset.folio, &popup);
	QGridLayout layout_champs(inset_infos);
	
	layout_champs.addWidget(new QLabel(tr("Titre : ")),   0, 0);
	layout_champs.addWidget(titre,                        0, 1);
	layout_champs.addWidget(new QLabel(tr("Auteur : ")),  1, 0);
	layout_champs.addWidget(auteur,                       1, 1);
	layout_champs.addWidget(new QLabel(tr("Date : ")),    2, 0);
	layout_champs.addWidget(date,                         2, 1);
	layout_champs.addWidget(new QLabel(tr("Fichier : ")), 3, 0);
	layout_champs.addWidget(fichier,                      3, 1);
	layout_champs.addWidget(new QLabel(tr("Folio : ")),   4, 0);
	layout_champs.addWidget(folio,                        4, 1);
	
	// boutons
	QDialogButtonBox boutons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(&boutons, SIGNAL(accepted()), &popup, SLOT(accept()));
	connect(&boutons, SIGNAL(rejected()), &popup, SLOT(accept()));
	
	// ajout dans une disposition verticale
	QVBoxLayout layout_v(&popup);
	layout_v.addWidget(diagram_size_box);
	layout_v.addWidget(inset_infos);
	layout_v.addWidget(&boutons);
	// si le dialogue est accepte
	if (popup.exec() == QDialog::Accepted) {
		InsetProperties new_inset;
		new_inset.title = titre -> text();
		new_inset.author = auteur -> text();
		new_inset.date = date -> date();
		new_inset.filename = fichier -> text();
		new_inset.folio = folio -> text();
		
		// s'il y a des modifications au cartouche
		if (new_inset != inset) {
			scene -> undoStack().push(new ChangeInsetCommand(scene, inset, new_inset));
		}
		
		// s'il y a des modifications
		if (
			columns_count_value  != columns_count  -> value() ||\
			columns_width_value  != columns_width  -> value() ||\
			columns_height_value != columns_height -> value()
		) {
			ChangeBorderCommand *cbc = new ChangeBorderCommand(scene);
			cbc -> columnsCountDifference  = columns_count  -> value() - columns_count_value;
			cbc -> columnsWidthDifference  = columns_width  -> value() - columns_width_value;
			cbc -> columnsHeightDifference = columns_height -> value() - columns_height_value;
			scene -> undoStack().push(cbc);
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
	ChangeBorderCommand *cbc = new ChangeBorderCommand(scene);
	cbc -> columnsCountDifference = 1;
	scene -> undoStack().push(cbc);
}

/**
	Enleve une colonne au schema.
*/
void DiagramView::removeColumn() {
	ChangeBorderCommand *cbc = new ChangeBorderCommand(scene);
	cbc -> columnsCountDifference = -1;
	scene -> undoStack().push(cbc);
}

/**
	Agrandit le schema en hauteur
*/
void DiagramView::expand() {
	ChangeBorderCommand *cbc = new ChangeBorderCommand(scene);
	cbc -> columnsHeightDifference = 80.0;
	scene -> undoStack().push(cbc);
}

/**
	Retrecit le schema en hauteur
*/
void DiagramView::shrink() {
	ChangeBorderCommand *cbc = new ChangeBorderCommand(scene);
	cbc -> columnsHeightDifference = -80.0;
	scene -> undoStack().push(cbc);
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
	QString window_title;
	if (file_name.isNull()) window_title += tr("nouveau sch\351ma");
	else window_title += file_name;
	window_title += "[*]";
	setWindowTitle(window_title);
	setWindowModified(!(scene -> undoStack().isClean()));
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
	Affiche un dialogue permettant d'editer le conducteur selectionne.
	Ne fait rien s'il y a 0 ou plusieurs conducteurs selectionnes.
*/
void DiagramView::editConductor() {
	QList<Conductor *> selected_conductors(scene -> selectedConductors().toList());
	
	// on ne peut editer qu'un conducteur a la fois
	if (selected_conductors.count() != 1) return;
	Conductor *edited_conductor = selected_conductors.first();
	
	// initialise l'editeur de proprietes pour le conducteur
	ConductorProperties old_properties = edited_conductor -> properties();
	ConductorPropertiesWidget *cpw = new ConductorPropertiesWidget(old_properties);
	
	// l'insere dans un dialogue
	QDialog conductor_dialog;
	conductor_dialog.setWindowTitle(tr("\311diter les propri\351t\351s d'un conducteur"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&conductor_dialog);
	dialog_layout -> addWidget(cpw);
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
	// initialise l'editeur de proprietes pour le conducteur
	ConductorPropertiesWidget *cpw = new ConductorPropertiesWidget(scene -> defaultConductorProperties);
	
	// l'insere dans un dialogue
	QDialog conductor_dialog;
	conductor_dialog.setWindowTitle(tr("\311diter les propri\351t\351s par d\351faut des conducteurs"));
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

void DiagramView::addText() {
	is_adding_text = true;
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
			context_menu -> addAction(qde -> expand_diagram);
			context_menu -> addAction(qde -> shrink_diagram);
		} else {
			context_menu -> addAction(qde -> cut);
			context_menu -> addAction(qde -> copy);
			context_menu -> addSeparator();
			context_menu -> addAction(qde -> delete_selection);
			context_menu -> addAction(qde -> rotate_selection);
			context_menu -> addSeparator();
			context_menu -> addAction(qde -> conductor_prop);
			context_menu -> addAction(qde -> conductor_reset);
		}
		
		// affiche le menu contextuel
		context_menu -> popup(e -> globalPos());
	}
	e -> accept();
}

/// @return l'editeur de schemas parent ou 0
QETDiagramEditor *DiagramView::diagramEditor() const {
	// remonte la hierarchie des widgets
	QWidget *w = const_cast<DiagramView *>(this);
	while (w -> parentWidget() && !w -> isWindow()) {
		w = w -> parentWidget();
	}
	// la fenetre est supposee etre un QETDiagramEditor
	return(qobject_cast<QETDiagramEditor *>(w));
}
