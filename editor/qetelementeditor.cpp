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
#include "qetelementeditor.h"
#include "qetapp.h"
#include "elementscene.h"
#include "elementview.h"
#include "customelementpart.h"
#include "newelementwizard.h"
#include "elementitemeditor.h"

/**
	Constructeur
	@param parent QWidget parent
*/
QETElementEditor::QETElementEditor(QWidget *parent) :
	QMainWindow(parent),
	read_only(false),
	min_title(tr("QElectroTech - \311diteur d'\351l\351ment")),
	_filename(QString())
{
	setWindowTitle(min_title);
	setWindowIcon(QIcon(":/ico/qet.png"));
	
	setupInterface();
	setupActions();
	setupMenus();
	
	// la fenetre est maximisee par defaut
	setMinimumSize(QSize(500, 350));
	setWindowState(Qt::WindowMaximized);
	
	// lecture des parametres
	readSettings();
	
	// affichage
	show();
}

/// Destructeur
QETElementEditor::~QETElementEditor() {
}

/**
	Met en place les actions
*/
void QETElementEditor::setupActions() {
	new_element   = new QAction(QIcon(":/ico/new.png"),          tr("&Nouveau"),                    this);
	open          = new QAction(QIcon(":/ico/open.png"),         tr("&Ouvrir"),                     this);
	save          = new QAction(QIcon(":/ico/save.png"),         tr("&Enregistrer"),                this);
	save_as       = new QAction(QIcon(":/ico/saveas.png"),       tr("Enregistrer sous"),            this);
	reload        = new QAction(QIcon(":/ico/reload.png"),       tr("Recharger"),                   this);
	quit          = new QAction(QIcon(":/ico/exit.png"),         tr("&Quitter"),                    this);
	selectall     = new QAction(                                 tr("Tout s\351lectionner"),        this);
	deselectall   = new QAction(                                 tr("D\351s\351lectionner tout"),   this);
	inv_select    = new QAction(                                 tr("Inverser la s\351lection"),    this);
	edit_delete   = new QAction(QIcon(":/ico/delete.png"),       tr("&Supprimer"),                  this);
	edit_size_hs  = new QAction(                                 tr("\311diter la taille et le point de saisie"), this);
	edit_names    = new QAction(                                 tr("\311diter les noms"),          this);
	edit_ori      = new QAction(QIcon(":/ico/orientations.png"), tr("\311diter les orientations"),  this);
	edit_raise    = new QAction(QIcon(":/ico/raise.png"),        tr("Rapprocher"),                  this);
	edit_lower    = new QAction(QIcon(":/ico/lower.png"),        tr("\311loigner"),                 this);
	edit_backward = new QAction(QIcon(":/ico/send_backward.png"),tr("Envoyer au fond"),             this);
	edit_forward  = new QAction(QIcon(":/ico/bring_forward.png"),tr("Amener au premier plan"),      this);
	move          = new QAction(QIcon(":/ico/select.png"),       tr("D\351placer un objet"),        this);
	add_line      = new QAction(QIcon(":/ico/line.png"),         tr("Ajouter une ligne"),           this);
	add_ellipse   = new QAction(QIcon(":/ico/ellipse.png"),      tr("Ajouter une ellipse"),         this);
	add_circle    = new QAction(QIcon(":/ico/circle.png"),       tr("Ajouter un cercle"),           this);
	add_polygon   = new QAction(QIcon(":/ico/polygon.png"),      tr("Ajouter un polygone"),         this);
	add_text      = new QAction(QIcon(":/ico/text.png"),         tr("Ajouter du texte"),            this);
	add_arc       = new QAction(QIcon(":/ico/arc.png"),          tr("Ajouter un arc de cercle"),    this);
	add_terminal  = new QAction(QIcon(":/ico/terminal.png"),     tr("Ajouter une borne"),           this);
	add_textfield = new QAction(QIcon(":/ico/textfield.png"),    tr("Ajouter un champ de texte"),   this);
	
	undo = ce_scene -> undoStack().createUndoAction(this, tr("Annuler"));
	redo = ce_scene -> undoStack().createRedoAction(this, tr("Refaire"));
	undo -> setIcon(QIcon(":/ico/undo.png"));
	redo -> setIcon(QIcon(":/ico/redo.png"));
	undo -> setShortcuts(QKeySequence::Undo);
	redo -> setShortcuts(QKeySequence::Redo);
	
	new_element       -> setShortcut(QKeySequence::New);
	open              -> setShortcut(QKeySequence::Open);
	save              -> setShortcut(QKeySequence::Save);
	reload            -> setShortcut(QKeySequence::Refresh);
	quit              -> setShortcut(QKeySequence(tr("Ctrl+Q")));
	selectall         -> setShortcut(QKeySequence::SelectAll);
	deselectall       -> setShortcut(QKeySequence(tr("Ctrl+Shift+A")));
	inv_select        -> setShortcut(QKeySequence(tr("Ctrl+I")));
	edit_delete       -> setShortcut(QKeySequence(tr("Suppr")));
	edit_names        -> setShortcut(QKeySequence(tr("Ctrl+E")));
	edit_size_hs      -> setShortcut(QKeySequence(tr("Ctrl+R")));
	edit_ori          -> setShortcut(QKeySequence(tr("Ctrl+T")));
	
	edit_raise        -> setShortcut(QKeySequence(tr("Ctrl+Shift+Up")));
	edit_lower        -> setShortcut(QKeySequence(tr("Ctrl+Shift+Down")));
	edit_backward     -> setShortcut(QKeySequence(tr("Ctrl+Shift+End")));
	edit_forward      -> setShortcut(QKeySequence(tr("Ctrl+Shift+Home")));
	
	connect(new_element,   SIGNAL(triggered()), this,     SLOT(slot_new()));
	connect(open,          SIGNAL(triggered()), this,     SLOT(slot_open()));
	connect(save,          SIGNAL(triggered()), this,     SLOT(slot_save()));
	connect(save_as,       SIGNAL(triggered()), this,     SLOT(slot_saveAs()));
	connect(reload,        SIGNAL(triggered()), this,     SLOT(slot_reload()));
	connect(quit,          SIGNAL(triggered()), this,     SLOT(close()));
	connect(selectall,     SIGNAL(triggered()), ce_scene, SLOT(slot_selectAll()));
	connect(deselectall,   SIGNAL(triggered()), ce_scene, SLOT(slot_deselectAll()));
	connect(inv_select,    SIGNAL(triggered()), ce_scene, SLOT(slot_invertSelection()));
	connect(edit_delete,   SIGNAL(triggered()), ce_scene, SLOT(slot_delete()));
	connect(edit_size_hs,  SIGNAL(triggered()), ce_scene, SLOT(slot_editSizeHotSpot()));
	connect(edit_names,    SIGNAL(triggered()), ce_scene, SLOT(slot_editNames()));
	connect(edit_ori,      SIGNAL(triggered()), ce_scene, SLOT(slot_editOrientations()));
	connect(edit_forward,  SIGNAL(triggered()), ce_scene, SLOT(slot_bringForward()));
	connect(edit_raise,    SIGNAL(triggered()), ce_scene, SLOT(slot_raise()));
	connect(edit_lower,    SIGNAL(triggered()), ce_scene, SLOT(slot_lower()));
	connect(edit_backward, SIGNAL(triggered()), ce_scene, SLOT(slot_sendBackward()));
	connect(move,          SIGNAL(triggered()), ce_scene, SLOT(slot_move()));
	connect(add_line,      SIGNAL(triggered()), ce_scene, SLOT(slot_addLine()));
	connect(add_ellipse,   SIGNAL(triggered()), ce_scene, SLOT(slot_addEllipse()));
	connect(add_circle,    SIGNAL(triggered()), ce_scene, SLOT(slot_addCircle()));
	connect(add_polygon,   SIGNAL(triggered()), ce_scene, SLOT(slot_addPolygon()));
	connect(add_text,      SIGNAL(triggered()), ce_scene, SLOT(slot_addText()));
	connect(add_arc,       SIGNAL(triggered()), ce_scene, SLOT(slot_addArc()));
	connect(add_terminal,  SIGNAL(triggered()), ce_scene, SLOT(slot_addTerminal()));
	connect(add_textfield, SIGNAL(triggered()), ce_scene, SLOT(slot_addTextField()));
	
	connect(move,          SIGNAL(triggered()), this, SLOT(slot_setRubberBandToView()));
	connect(add_line,      SIGNAL(triggered()), this, SLOT(slot_setNoDragToView()));
	connect(add_ellipse,   SIGNAL(triggered()), this, SLOT(slot_setNoDragToView()));
	connect(add_circle,    SIGNAL(triggered()), this, SLOT(slot_setNoDragToView()));
	connect(add_polygon,   SIGNAL(triggered()), this, SLOT(slot_setNoDragToView()));
	connect(add_text,      SIGNAL(triggered()), this, SLOT(slot_setNoDragToView()));
	connect(add_arc,       SIGNAL(triggered()), this, SLOT(slot_setNoDragToView()));
	connect(add_terminal,  SIGNAL(triggered()), this, SLOT(slot_setNoDragToView()));
	connect(add_textfield, SIGNAL(triggered()), this, SLOT(slot_setNoDragToView()));
	
	connect(ce_scene,      SIGNAL(needNormalMode()), this, SLOT(slot_setNormalMode()));
	
	move          -> setCheckable(true);
	add_line      -> setCheckable(true);
	add_ellipse   -> setCheckable(true);
	add_circle    -> setCheckable(true);
	add_polygon   -> setCheckable(true);
	add_text      -> setCheckable(true);
	add_arc       -> setCheckable(true);
	add_terminal  -> setCheckable(true);
	add_textfield -> setCheckable(true);
	
	parts = new QActionGroup(this);
	parts -> addAction(move);
	parts -> addAction(add_line);
	parts -> addAction(add_ellipse);
	parts -> addAction(add_circle);
	parts -> addAction(add_polygon);
	parts -> addAction(add_text);
	parts -> addAction(add_arc);
	parts -> addAction(add_textfield);
	parts -> addAction(add_terminal);
	parts -> setExclusive(true);
	
	parts_toolbar = new QToolBar(tr("Parties"), this);
	parts_toolbar -> setObjectName("parts");
	foreach (QAction *action, parts -> actions()) parts_toolbar -> addAction(action);
	move -> setChecked(true);
	parts_toolbar -> setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);
	
	/*
	QAction *xml_preview = new QAction(QIcon(":/ico/info.png"), tr("XML"), this);
	connect(xml_preview, SIGNAL(triggered()), this, SLOT(xmlPreview()));
	parts_toolbar -> addAction(xml_preview);
	*/
	
	addToolBar(Qt::LeftToolBarArea, parts_toolbar);
	
	connect(ce_scene, SIGNAL(selectionChanged()), this, SLOT(slot_updateInformations()));
	connect(ce_scene, SIGNAL(selectionChanged()), this, SLOT(slot_updateMenus()));
	connect(&(ce_scene -> undoStack()), SIGNAL(cleanChanged(bool)), this, SLOT(slot_updateMenus()));
	connect(&(ce_scene -> undoStack()), SIGNAL(cleanChanged(bool)), this, SLOT(slot_updateTitle()));
}

/**
	Met en place les menus.
*/
void QETElementEditor::setupMenus() {
	file_menu    = new QMenu(tr("Fichier"),    this);
	edit_menu    = new QMenu(tr("\311dition"), this);
	display_menu = new QMenu(tr("Affichage"),  this);
	tools_menu   = new QMenu(tr("Outils"),     this);
	help_menu    = new QMenu(tr("Aide"),       this);
	
	file_menu    -> setTearOffEnabled(true);
	edit_menu    -> setTearOffEnabled(true);
	display_menu -> setTearOffEnabled(true);
	tools_menu   -> setTearOffEnabled(true);
	help_menu    -> setTearOffEnabled(true);
	
	file_menu    -> addAction(new_element);
	file_menu    -> addAction(open);
	file_menu    -> addAction(save);
	file_menu    -> addAction(save_as);
	file_menu    -> addSeparator();
	file_menu    -> addAction(reload);
	file_menu    -> addSeparator();
	file_menu    -> addAction(quit);
	
	edit_menu -> addAction(undo);
	edit_menu -> addAction(redo);
	edit_menu -> addSeparator();
	edit_menu -> addAction(selectall);
	edit_menu -> addAction(deselectall);
	edit_menu -> addAction(inv_select);
	edit_menu -> addSeparator();
	edit_menu -> addAction(edit_delete);
	edit_menu -> addSeparator();
	edit_menu -> addAction(edit_names);
	edit_menu -> addAction(edit_size_hs);
	edit_menu -> addAction(edit_ori);
	edit_menu -> addSeparator();
	edit_menu -> addAction(edit_forward);
	edit_menu -> addAction(edit_raise);
	edit_menu -> addAction(edit_lower);
	edit_menu -> addAction(edit_backward);
	
	// menu Affichage > Afficher
	QMenu *display_toolbars = createPopupMenu();
	display_toolbars -> setTearOffEnabled(true);
	display_toolbars -> setTitle(tr("Afficher"));
	display_menu -> addMenu(display_toolbars);
	
	menuBar() -> addMenu(file_menu);
	menuBar() -> addMenu(edit_menu);
	menuBar() -> addMenu(display_menu);
	
	/*
	menuBar() -> addMenu(tools_menu);
	menuBar() -> addMenu(help_menu);
	*/
}

/**
	Met a jour les menus
*/
void QETElementEditor::slot_updateMenus() {
	bool selected_items = !ce_scene -> selectedItems().isEmpty();
	edit_delete   -> setEnabled(selected_items);
	edit_forward  -> setEnabled(selected_items);
	edit_raise    -> setEnabled(selected_items);
	edit_lower    -> setEnabled(selected_items);
	edit_backward -> setEnabled(selected_items);
	save -> setEnabled(!ce_scene -> undoStack().isClean());
}

/**
	Met a jour le titre de la fenetre
*/
void QETElementEditor::slot_updateTitle() {
	QString title = min_title;
	title += " - " + ce_scene -> names().name() + " ";
	if (_filename != QString()) {
		if (!ce_scene -> undoStack().isClean()) title += tr("[Modifi\351]");
		if (isReadOnly()) title += tr(" [lecture seule]");
	}
	setWindowTitle(title);
}

/**
	Met en place l'interface
*/
void QETElementEditor::setupInterface() {
	// editeur
	ce_scene = new ElementScene(this, this);
	ce_scene -> slot_move();
	ce_view = new ElementView(ce_scene, this);
	slot_setRubberBandToView();
	setCentralWidget(ce_view);
	
	// widget par defaut dans le QDockWidget
	default_informations = new QLabel();
	
	// panel sur le cote pour editer les parties
	tools_dock = new QDockWidget(tr("Informations"), this);
	tools_dock -> setObjectName("informations");
	tools_dock -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	tools_dock -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	tools_dock -> setMinimumWidth(290);
	addDockWidget(Qt::RightDockWidgetArea, tools_dock);
	QWidget *info_widget = new QWidget();
	info_widget -> setLayout(new QVBoxLayout(info_widget));
	tools_dock -> setWidget(info_widget);
	
	// panel sur le cote pour les annulations
	undo_dock = new QDockWidget(tr("Annulations"), this);
	undo_dock -> setObjectName("undo");
	undo_dock -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	undo_dock -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	undo_dock -> setMinimumWidth(290);
	addDockWidget(Qt::RightDockWidgetArea, undo_dock);
	QUndoView* undo_view = new QUndoView(&(ce_scene -> undoStack()), this);
	undo_view -> setEmptyLabel(tr("Aucune modification"));
	undo_dock -> setWidget(undo_view);
	
	// panel sur le cote pour la liste des parties
	parts_list = new QListWidget(this);
	parts_list -> setSelectionMode(QAbstractItemView::ExtendedSelection);
	connect(ce_scene,   SIGNAL(partsAdded()),           this, SLOT(slot_createPartsList()));
	connect(ce_scene,   SIGNAL(partsRemoved()),         this, SLOT(slot_createPartsList()));
	connect(ce_scene,   SIGNAL(partsZValueChanged()),   this, SLOT(slot_createPartsList()));
	connect(ce_scene,   SIGNAL(selectionChanged()),     this, SLOT(slot_updatePartsList()));
	connect(parts_list, SIGNAL(itemSelectionChanged()), this, SLOT(slot_updateSelectionFromPartsList()));
	parts_dock = new QDockWidget(tr("Parties"), this);
	parts_dock -> setObjectName("parts_list");
	parts_dock -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	parts_dock -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	parts_dock -> setMinimumWidth(290);
	tabifyDockWidget(undo_dock, parts_dock);
	parts_dock -> setWidget(parts_list);
	
	slot_updateInformations();
	slot_createPartsList();
	
	// barre d'etat
	statusBar() -> showMessage(tr("\311diteur d'\351l\351ments"));
}

/**
	Passe l'editeur d'element en mode selection : le pointeur deplace les
	elements selectionnes et il est possible d'utiliser un rectangle de selection.
*/
void QETElementEditor::slot_setRubberBandToView() {
	ce_view -> setDragMode(QGraphicsView::RubberBandDrag);
}

/**
	Passe l'editeur d'element en mode immobile (utilise pour la lecture seule)
*/
void QETElementEditor::slot_setNoDragToView() {
	ce_view -> setDragMode(QGraphicsView::NoDrag);
}

/**
	Passe l'editeur en mode normal
*/
void QETElementEditor::slot_setNormalMode() {
	if (!move -> isChecked()) move -> setChecked(true);
	ce_view -> setDragMode(QGraphicsView::RubberBandDrag);
	ce_scene -> slot_move();
}

/**
	Met a jour la zone d'information et d'edition.
	Si plusieurs parties sont selectionnees, seul leur nombre est affiche.
	Sinon, le widget d'edition de la partie est insere.
	@see CustomElementPart::elementInformations()
*/
void QETElementEditor::slot_updateInformations() {
	QList<QGraphicsItem *> selected_qgis = ce_scene -> selectedItems();
	QList<CustomElementPart *> selected_parts;
	foreach(QGraphicsItem *qgi, selected_qgis) {
		if (CustomElementPart *cep = dynamic_cast<CustomElementPart *>(qgi)) {
			selected_parts.append(cep);
		}
	}
	
	// recupere le layout
	QLayout *layout = tools_dock -> widget() -> layout();
	
	// enleve les widgets deja presents
	QLayoutItem *qli;
	while ((qli = layout -> takeAt(0)) != 0) {
		if (qli -> widget()) {
			layout -> removeWidget(qli -> widget());
			qli -> widget() -> setParent(0);
		}
	}
	if (selected_parts.size() == 1) {
		// recupere le premier CustomElementPart et en ajoute le widget d'edition
		layout -> addWidget(selected_parts.first() -> elementInformations());
	} else {
		default_informations -> setText(
			selected_parts.size() ?
			QString("%1").arg(selected_parts.size()) + tr(" parties s\351lectionn\351es.") :
			tr("Aucune partie s\351lectionn\351e.")
		);
		layout -> addWidget(default_informations);
	}
}

/**
	Affiche le code XML correspondant a l'element dans son etat actuel dans
	une boite de dialogue.
*/
void QETElementEditor::xmlPreview() {
	QMessageBox::information(
		this,
		"Export XML",
		ce_scene -> toXml().toString(4)
	);
}

/**
	Charge un fichier
	@param filepath Chemin du fichier a charger
*/
void QETElementEditor::fromFile(const QString &filepath) {
	bool state = true;
	QString error_message;
	
	// le fichier doit exister
	QFileInfo infos_file(filepath);
	if (!infos_file.exists() || !infos_file.isFile()) {
		state = false;
		error_message = tr("Le fichier ") + filepath + tr(" n'existe pas.");
	}
	
	// le fichier doit etre lisible
	QFile file(filepath);
	if (state) {
		if (!file.open(QIODevice::ReadOnly)) {
			state = false;
			error_message = tr("Impossible d'ouvrir le fichier ") + filepath + ".";
		}
	}
	
	// le fichier doit etre un document XML
	QDomDocument document_xml;
	if (state) {
		if (!document_xml.setContent(&file)) {
			state = false;
			error_message = tr("Ce fichier n'est pas un document XML valide");
		}
		file.close();
	}
	
	if (!state) {
		QMessageBox::critical(this, tr("Erreur"), error_message);
		return;
	}
	
	// chargement de l'element
	ce_scene -> fromXml(document_xml);
	slot_createPartsList();
	
	// gestion de la lecture seule
	if (!infos_file.isWritable()) {
		QMessageBox::warning(
			this,
			tr("\311dition en lecture seule"),
			tr("Vous n'avez pas les privil\350ges n\351cessaires pour modifier cet \351lement. Il sera donc ouvert en lecture seule.")
		);
		setReadOnly(true);
	}
	
	// memorise le fichier
	setFileName(filepath);
}


/**
	Enregistre l'element vers un fichier
	@param fn Chemin du fichier a enregistrer
	@return true en cas de reussite, false sinon
*/
bool QETElementEditor::toFile(const QString &fn) {
	QFile file(fn);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'ecrire dans ce fichier"));
		return(false);
	}
	QTextStream out(&file);
	out.setCodec("UTF-8");
	out << ce_scene -> toXml().toString(4);
	file.close();
	return(true);
}

/**
	specifie si l'editeur d'element doit etre en mode lecture seule
	@param ro true pour activer le mode lecture seule, false pour le desactiver
*/
void QETElementEditor::setReadOnly(bool ro) {
	read_only = ro;
	// active / desactive les actions
	foreach (QAction *action, parts -> actions()) action -> setEnabled(!ro);
	
	// active / desactive les interactions avec la scene
	ce_view -> setInteractive(!ro);
	
	// active / desactive l'edition de la taille, du hotspot, des noms et des orientations
	selectall    -> setEnabled(!ro);
	deselectall  -> setEnabled(!ro);
	inv_select   -> setEnabled(!ro);
	undo         -> setEnabled(!ro);
	redo         -> setEnabled(!ro);
	edit_delete  -> setEnabled(!ro);
	edit_size_hs -> setEnabled(!ro);
	edit_names   -> setEnabled(!ro);
	edit_ori     -> setEnabled(!ro);
	parts_list   -> setEnabled(!ro);
}

/**
	@return true si l'editeur d'element est en mode lecture seule
*/
bool QETElementEditor::isReadOnly() const {
	return(read_only);
}

/**
	Lance l'assistant de creation d'un nouvel element.
*/
void QETElementEditor::slot_new() {
	NewElementWizard new_element_wizard;
	new_element_wizard.exec();
}

/**
	Ouvre un fichier
*/
void QETElementEditor::slot_open() {
	// demande un nom de fichier a ouvrir a l'utilisateur
	QString user_filename = QFileDialog::getOpenFileName(
		this,
		tr("Ouvrir un fichier"),
		_filename.isEmpty() ? QETApp::customElementsDir() : QDir(_filename).absolutePath(),
		tr("\311l\351ments QElectroTech (*.elmt);;Fichiers XML (*.xml);;Tous les fichiers (*)")
	);
	if (user_filename.isEmpty()) return;
	QETElementEditor *cee = new QETElementEditor();
	cee -> fromFile(user_filename);
	cee -> show();
}

/**
	Recharge l'element edite
*/
void QETElementEditor::slot_reload() {
	// impossible de recharger un element non enregistre
	if (_filename.isEmpty()) return;
	
	// s'il ya des modifications, on demande a l'utilisateur s'il est certain
	// de vouloir recharger
	if (!ce_scene -> undoStack().isClean()) {
		QMessageBox::StandardButton answer = QMessageBox::question(
			this,
			tr("Recharger l'\351l\351ment"),
			tr("Vous avez efffectu\351 des modifications sur cet \351l\351ment. Si vous le rechargez, ces modifications seront perdues. Voulez-vous vraiment recharger l'\351l\351ment ?"),
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
			QMessageBox::Cancel
		);
		if (answer != QMessageBox::Yes) return;
	}
	
	// recharge l'element
	ce_scene -> reset();
	fromFile(_filename);
}

/**
	Enregistre l'element en cours d'edition.
	Si le nom du fichier en cours n'est pas connu, cette methode equivaut a
	l'action "Enregistrer sous"
	@see slot_saveAs()
*/
bool QETElementEditor::slot_save() {
	// si on ne connait pas le nom du fichier en cours, enregistrer revient a enregistrer sous
	if (_filename.isEmpty()) return(slot_saveAs());
	// sinon on enregistre dans le nom de fichier connu
	bool result_save = toFile(_filename);
	if (result_save) ce_scene -> undoStack().setClean();
	return(result_save);
}

/**
	Demande un nom de fichier a l'utilisateur et enregistre l'element
*/
bool QETElementEditor::slot_saveAs() {
	// demande un nom de fichier a l'utilisateur pour enregistrer l'element
	QString fn = QFileDialog::getSaveFileName(
		this,
		tr("Enregistrer sous"),
		_filename.isEmpty() ? QETApp::customElementsDir() : QDir(_filename).absolutePath(),
		tr("\311l\351ments QElectroTech (*.elmt)")
	);
	// si aucun nom n'est entre, renvoie faux.
	if (fn.isEmpty()) return(false);
	// si le nom ne se termine pas par l'extension .elmt, celle-ci est ajoutee
	if (!fn.endsWith(".elmt", Qt::CaseInsensitive)) fn += ".elmt";
	// tente d'enregistrer le fichier
	bool result_save = toFile(fn);
	// si l'enregistrement reussit, le nom du fichier est conserve
	if (result_save) {
		setFileName(fn);
		ce_scene -> undoStack().setClean();
	}
	// retourne un booleen representatif de la reussite de l'enregistrement
	return(result_save);
}

/**
	@return true si l'element peut etre ferme.
	Un element peut etre ferme s'il ne comporte aucune modification.
	Si l'element comporte des modifications, la question est posee a
	l'utilisateur.
*/
bool QETElementEditor::canClose() {
	if (ce_scene -> undoStack().isClean()) return(true);
	// demande d'abord a l'utilisateur s'il veut enregistrer l'element en cours
	QMessageBox::StandardButton answer = QMessageBox::question(
		this,
		tr("Enregistrer l'\351l\351ment en cours ?"),
		tr("Voulez-vous enregistrer l'\351l\351ment ") + ce_scene -> names().name() + tr(" ?"),
		QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
		QMessageBox::Cancel
	);
	bool result;
	switch(answer) {
		case QMessageBox::Cancel: result = false;         break; // l'utilisateur annule : echec de la fermeture
		case QMessageBox::Yes:    result = slot_save();   break; // l'utilisateur dit oui : la reussite depend de l'enregistrement
		default:                  result = true;                 // l'utilisateur dit non ou ferme le dialogue: c'est reussi
	}
	return(result);
}

/**
	Permet de quitter l'editeur lors de la fermeture de la fenetre principale
	@param qce Le QCloseEvent correspondant a l'evenement de fermeture
*/
void QETElementEditor::closeEvent(QCloseEvent *qce) {
	if (canClose()) {
		writeSettings();
		setAttribute(Qt::WA_DeleteOnClose);
		qce -> accept();
	} else qce -> ignore();
}

/**
	Remplit la liste des parties
*/
void QETElementEditor::slot_createPartsList() {
	parts_list -> blockSignals(true);
	parts_list -> clear();
	QList<QGraphicsItem *> qgis = ce_scene -> zItems(true);
	for (int j = qgis.count() - 1 ; j >= 0 ; -- j) {
		QGraphicsItem *qgi = qgis[j];
		if (CustomElementPart *cep = dynamic_cast<CustomElementPart *>(qgi)) {
			QString part_desc = cep -> name();
			QListWidgetItem *qlwi = new QListWidgetItem(part_desc);
			QVariant v;
			v.setValue<QGraphicsItem *>(qgi);
			qlwi -> setData(42, v);
			parts_list -> addItem(qlwi);
			qlwi -> setSelected(qgi -> isSelected());
		}
	}
	parts_list -> blockSignals(false);
}

/**
	Met a jour la selection dans la liste des parties
*/
void QETElementEditor::slot_updatePartsList() {
	if (parts_list -> count() != ce_scene -> items().count()) {
		slot_createPartsList();
	} else {
		parts_list -> blockSignals(true);
		int i = 0;
		QList<QGraphicsItem *> items = ce_scene -> zItems(true);
		for (int j = items.count() - 1 ; j >= 0 ; -- j) {
			QGraphicsItem *qgi = items[j];
			QListWidgetItem *qlwi = parts_list -> item(i);
			if (qlwi) qlwi -> setSelected(qgi -> isSelected());
			++ i;
		}
		parts_list -> blockSignals(false);
	}
}

/**
	Met a jour la selection des parties de l'element a partir de la liste des
	parties
*/
void QETElementEditor::slot_updateSelectionFromPartsList() {
	ce_scene  -> blockSignals(true);
	parts_list -> blockSignals(true);
	for (int i = 0 ; i < parts_list -> count() ; ++ i) {
		QListWidgetItem *qlwi = parts_list -> item(i);
		QGraphicsItem *qgi = qlwi -> data(42).value<QGraphicsItem *>();
		if (qgi) {
			qgi -> setSelected(qlwi -> isSelected());
		}
	}
	parts_list -> blockSignals(false);
	ce_scene -> blockSignals(false);
}

/// Lit les parametres de l'editeur d'element
void QETElementEditor::readSettings() {
	QSettings &settings = QETApp::settings();
	
	// dimensions et position de la fenetre
	QVariant geometry = settings.value("elementeditor/geometry");
	if (geometry.isValid()) restoreGeometry(geometry.toByteArray());
	
	// etat de la fenetre (barres d'outils, docks...)
	QVariant state = settings.value("elementeditor/state");
	if (state.isValid()) restoreState(state.toByteArray());
}

/// Enregistre les parametres de l'editeur d'element
void QETElementEditor::writeSettings() {
	QSettings &settings = QETApp::settings();
	settings.setValue("elementeditor/geometry", saveGeometry());
	settings.setValue("elementeditor/state", saveState());
}
