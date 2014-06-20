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
#include "editorcommands.h"

/**
	Constructs an ElementEditionCommand, thus embedding the provided \a scene and \a view.
	@param parent Parent command
*/
ElementEditionCommand::ElementEditionCommand(ElementScene *scene, ElementView *view, QUndoCommand *parent):
	QUndoCommand(parent),
	editor_scene_(scene),
	editor_view_(view)
{
}

/**
	Constructs an ElementEditionCommand, thus embedding the provided \a scene and \a view.
	@param text Text describing the effect of the command
	@param parent Parent command
*/
ElementEditionCommand::ElementEditionCommand(const QString &text, ElementScene *scene, ElementView *view, QUndoCommand *parent):
	QUndoCommand(text, parent),
	editor_scene_(scene),
	editor_view_(view)
{
}

/**
	Destructor
*/
ElementEditionCommand::~ElementEditionCommand() {
}

/**
	@return the element editor/scene the command should take place on
*/
ElementScene *ElementEditionCommand::elementScene() const {
	return(editor_scene_);
}

/**
	Define \a scene as the element editor/scene the command should take place
*/
void ElementEditionCommand::setElementScene(ElementScene *scene) {
	editor_scene_ = scene;
}

/**
	@return the view the effect of the command should be rendered on
*/
ElementView *ElementEditionCommand::elementView() const {
	return(editor_view_);
}

/**
	Define \a view as the view the effect of the command should be rendered on
*/
void ElementEditionCommand::setElementView(ElementView *view) {
	editor_view_ = view;
}

/*** DeletePartsCommand ***/
/**
	Constructeur
	@param scene ElementScene concernee
	@param parts Liste des parties supprimees
	@param parent QUndoCommand parent
*/
DeletePartsCommand::DeletePartsCommand(
	ElementScene *scene,
	const QList<QGraphicsItem *> parts,
	QUndoCommand *parent
) :
	ElementEditionCommand(QObject::tr("suppression", "undo caption"), scene, 0, parent),
	deleted_parts(parts)
{
	foreach(QGraphicsItem *qgi, deleted_parts) {
		editor_scene_ -> qgiManager().manage(qgi);
	}
}

/// Destructeur : detruit egalement les parties supprimees
DeletePartsCommand::~DeletePartsCommand() {
	foreach(QGraphicsItem *qgi, deleted_parts) {
		editor_scene_ -> qgiManager().release(qgi);
	}
}

/// Restaure les parties supprimees
void DeletePartsCommand::undo() {
	editor_scene_ -> blockSignals(true);
	foreach(QGraphicsItem *qgi, deleted_parts) {
		editor_scene_ -> addItem(qgi);
	}
	editor_scene_ -> blockSignals(false);
}

/// Supprime les parties
void DeletePartsCommand::redo() {
	editor_scene_ -> blockSignals(true);
	foreach(QGraphicsItem *qgi, deleted_parts) {
		editor_scene_ -> removeItem(qgi);
	}
	editor_scene_ -> blockSignals(false);
}

/*** CutPartsCommand ***/
/**
	Constructeur
	@param view ElementView concernee
	@param c Liste des parties collees
	@param parent QUndoCommand parent
*/
PastePartsCommand::PastePartsCommand(
	ElementView *view,
	const ElementContent &c,
	QUndoCommand *parent
) :
	ElementEditionCommand(view ? view -> scene() : 0, view, parent),
	content_(c),
	uses_offset(false),
	first_redo(true)
{
	setText(QObject::tr("coller"));
	editor_scene_ -> qgiManager().manage(content_);
}

/// Destructeur
PastePartsCommand::~PastePartsCommand() {
	editor_scene_ -> qgiManager().release(content_);
}

/// annule le coller
void PastePartsCommand::undo() {
	// enleve les parties
	editor_scene_ -> blockSignals(true);
	foreach(QGraphicsItem *part, content_) {
		editor_scene_ -> removeItem(part);
	}
	editor_scene_ -> blockSignals(false);
	if (uses_offset) {
		editor_view_ -> offset_paste_count_    = old_offset_paste_count_;
		editor_view_ -> start_top_left_corner_ = old_start_top_left_corner_;
	}
	editor_view_ -> adjustSceneRect();
}

/// refait le coller
void PastePartsCommand::redo() {
	if (first_redo) first_redo = false;
	else {
		// pose les parties
		editor_scene_ -> blockSignals(true);
		foreach(QGraphicsItem *part, content_) {
			editor_scene_ -> addItem(part);
		}
		editor_scene_ -> blockSignals(false);
		if (uses_offset) {
			editor_view_ -> offset_paste_count_    = new_offset_paste_count_;
			editor_view_ -> start_top_left_corner_ = new_start_top_left_corner_;
		}
	}
	editor_scene_ -> slot_select(content_);
	editor_view_ -> adjustSceneRect();
}

/**
	Indique a cet objet d'annulation que le c/c a annuler ou refaire etait un
	c/c avec decalage ; il faut plus d'informations pour annuler ce type de
	collage.
*/
void PastePartsCommand::setOffset(int old_offset_pc, const QPointF &old_start_tlc, int new_offset_pc, const QPointF &new_start_tlc) {
	old_offset_paste_count_    = old_offset_pc;
	old_start_top_left_corner_ = old_start_tlc;
	new_offset_paste_count_    = new_offset_pc;
	new_start_top_left_corner_ = new_start_tlc;
	uses_offset = true;
}

/*** CutPartsCommand ***/
/**
	Constructeur
	@param scene ElementScene concernee
	@param parts Liste des parties coupees
	@param parent QUndoCommand parent
*/
CutPartsCommand::CutPartsCommand(
	ElementScene *scene,
	const QList<QGraphicsItem *> parts,
	QUndoCommand *parent
) :
	DeletePartsCommand(scene, parts, parent)
{
	setText(QString(QObject::tr("couper des parties", "undo caption")));
}

/// Destructeur
CutPartsCommand::~CutPartsCommand() {
}

/*** MovePartsCommand ***/
/**
	Constructeur
	@param m Mouvement sous forme de QPointF
	@param scene ElementScene concernee
	@param parts Liste des parties deplacees
	@param parent QUndoCommand parent
*/
MovePartsCommand::MovePartsCommand(
	const QPointF &m,
	ElementScene *scene,
	const QList<QGraphicsItem *> parts,
	QUndoCommand *parent
) :
	ElementEditionCommand(QObject::tr("d\351placement", "undo caption"), scene, 0, parent),
	movement(m),
	first_redo(true)
{
	moved_parts = parts;
}

/// Destructeur
MovePartsCommand::~MovePartsCommand() {
}

/// Annule le deplacement
void MovePartsCommand::undo() {
	foreach(QGraphicsItem *qgi, moved_parts) qgi -> moveBy(-movement.x(), -movement.y());
}

/// Refait le deplacement
void MovePartsCommand::redo() {
	// le premier appel a redo, lors de la construction de l'objet, ne doit pas se faire
	if (first_redo) {
		first_redo = false;
		return;
	}
	foreach(QGraphicsItem *qgi, moved_parts) qgi -> moveBy(movement.x(), movement.y());
}

/*** AddPartCommand ***/
/**
	Constructeur
	@param name Nom de la partie ajoutee
	@param scene ElementScene concernee
	@param p partie ajoutee
	@param parent QUndoCommand parent
*/
AddPartCommand::AddPartCommand(
	const QString &name,
	ElementScene *scene,
	QGraphicsItem *p,
	QUndoCommand *parent
) :
	ElementEditionCommand(QString(QObject::tr("ajout %1", "undo caption")).arg(name), scene, 0, parent),
	part(p),
	first_redo(true)
{
	editor_scene_ -> qgiManager().manage(part);
}

/// Destructeur
AddPartCommand::~AddPartCommand() {
	editor_scene_ -> qgiManager().release(part);
}

/// Annule l'ajout
void AddPartCommand::undo() {
	editor_scene_ -> removeItem(part);
}

/// Refait l'ajout
void AddPartCommand::redo() {
	// le premier appel a redo, lors de la construction de l'objet, ne doit pas se faire
	if (first_redo) {
		if (!part -> zValue()) {
			// the added part has no specific zValue already defined, we put it
			// above existing items (but still under terminals)
			QList<QGraphicsItem *> existing_items = editor_scene_ -> zItems(ElementScene::SortByZValue | ElementScene::SelectedOrNot);
			qreal z = existing_items.count() ? existing_items.last() -> zValue() + 1 : 1;
			part -> setZValue(z);
		}
		editor_scene_ -> clearSelection();
		part -> setSelected(true);
		first_redo = false;
		return;
	}
	editor_scene_ -> addItem(part);
}

/**
	Constructeur
	@param name   nom de la propriete modifiee
	@param part   partie modifiee
	@param prop   propriete modifiee
	@param old_v  ancienne valeur
	@param new_v  nouvelle valeur
	@param parent qUndoCommand parent
*/
ChangePartCommand::ChangePartCommand(
	const QString &name,
	CustomElementPart *part,
	const char *prop,
	const QVariant &old_v,
	const QVariant &new_v,
	QUndoCommand *parent
) :
	ElementEditionCommand(QString(QObject::tr("modification %1", "undo caption")).arg(name), 0, 0, parent),
	cep(part),
	property(prop),
	old_value(old_v),
	new_value(new_v)
{
}

/// Destructeur
ChangePartCommand::~ChangePartCommand() {
}

/// Annule le changement
void ChangePartCommand::undo() {
	cep -> setProperty(property, old_value);
}

/// Refait le changement
void ChangePartCommand::redo() {
	cep -> setProperty(property, new_value);
}

/**
	Constructeur
	@param p Polygone edite
	@param o_points points avant le changement
	@param n_points points apres le changement
	@param parent QUndoCommand parent
*/
ChangePolygonPointsCommand::ChangePolygonPointsCommand(
	PartPolygon *p,
	const QVector<QPointF> &o_points,
	const QVector<QPointF> &n_points,
	QUndoCommand *parent
) :
	ElementEditionCommand(QObject::tr("modification points polygone", "undo caption"), 0, 0, parent),
	polygon(p),
	old_points(o_points),
	new_points(n_points)
{
}

/// Destructeur
ChangePolygonPointsCommand::~ChangePolygonPointsCommand() {
}

/// Annule le changement
void ChangePolygonPointsCommand::undo() {
	polygon -> setPolygon(old_points);
}

/// Refait le changement
void ChangePolygonPointsCommand::redo() {
	polygon -> setPolygon(new_points);
}

/**
	Constructeur
	@param element_scene Element edite
	@param before Listes des noms avant changement
	@param after Listes des noms apres changement
	@param parent QUndoCommand parent
*/
ChangeNamesCommand::ChangeNamesCommand(
	ElementScene *element_scene,
	const NamesList &before,
	const NamesList &after,
	QUndoCommand *parent
) :
	ElementEditionCommand(QObject::tr("modification noms", "undo caption"), element_scene, 0, parent),
	names_before(before),
	names_after(after)
{
}

/// Destructeur
ChangeNamesCommand::~ChangeNamesCommand() {
}

/// Annule le changement
void ChangeNamesCommand::undo() {
	editor_scene_ -> setNames(names_before);
}

/// Refait le changement
void ChangeNamesCommand::redo() {
	editor_scene_ -> setNames(names_after);
}

/**
	Constructeur
	@param elmt ElementScene concernee
	@param o Option decrivant le type de traitement applique aux zValues des parties de l'element
	@param parent QUndoCommand parent
*/
ChangeZValueCommand::ChangeZValueCommand(
	ElementScene *elmt,
	ChangeZValueCommand::Option o,
	QUndoCommand *parent
) :
	ElementEditionCommand(elmt, 0, parent),
	option(o)
{
	// retrieve all primitives but terminals
	QList<QGraphicsItem *> items_list = editor_scene_ -> zItems(ElementScene::SortByZValue | ElementScene::SelectedOrNot);
	
	// prend un snapshot des zValues
	foreach(QGraphicsItem *qgi, items_list) undo_hash.insert(qgi, qgi -> zValue());
	
	// choisit le nom en fonction du traitement
	if (option == BringForward) {
		setText(QObject::tr("amener au premier plan", "undo caption"));
		applyBringForward(items_list);
	} else if (option == Raise) {
		setText(QObject::tr("rapprocher", "undo caption"));
		applyRaise(items_list);
	} else if (option == Lower) {
		setText(QObject::tr("\351loigner", "undo caption"));
		applyLower(items_list);
	} else if (option == SendBackward) {
		setText(QObject::tr("envoyer au fond", "undo caption"));
		applySendBackward(items_list);
	}
}

/// Destructeur
ChangeZValueCommand::~ChangeZValueCommand() {
}

/// Annule les changements de zValue
void ChangeZValueCommand::undo() {
	foreach(QGraphicsItem *qgi, undo_hash.keys()) qgi -> setZValue(undo_hash[qgi]);
}

/// Refait les changements de zValue
void ChangeZValueCommand::redo() {
	foreach(QGraphicsItem *qgi, redo_hash.keys()) qgi -> setZValue(redo_hash[qgi]);
}

/**
	Amene les elements selectionnes au premier plan
	@param items_list Liste des elements (selectionnes et non selectionnes)
*/
void ChangeZValueCommand::applyBringForward(const QList<QGraphicsItem *> &items_list) {
	QList<QGraphicsItem *> non_selected_items = items_list;
	QList<QGraphicsItem *> selected_items;
	foreach(QGraphicsItem *qgi, non_selected_items) {
		if (qgi -> isSelected()) {
			selected_items << qgi;
			non_selected_items.removeAt(non_selected_items.indexOf(qgi));
		}
	}
	int z = 1;
	foreach(QGraphicsItem *qgi, non_selected_items) redo_hash.insert(qgi, z ++);
	foreach(QGraphicsItem *qgi,     selected_items) redo_hash.insert(qgi, z ++);
}

/**
	Remonte les elements selectionnes d'un plan
	@param items_list Liste des elements (selectionnes et non selectionnes)
*/
void ChangeZValueCommand::applyRaise(const QList<QGraphicsItem *> &items_list) {
	QList<QGraphicsItem *> my_items_list = items_list;
	
	for (int i = my_items_list.count() - 2 ; i >= 0 ; -- i) {
		if (my_items_list[i] -> isSelected()) {
			if (!my_items_list[i +1] -> isSelected()) {
				my_items_list.swap(i, i + 1);
			}
		}
	}
	int z = 1;
	foreach(QGraphicsItem *qgi, my_items_list) redo_hash.insert(qgi, z ++);
}

/**
	Descend les elements selectionnes d'un plan
	@param items_list Liste des elements (selectionnes et non selectionnes)
*/
void ChangeZValueCommand::applyLower(const QList<QGraphicsItem *> &items_list) {
	QList<QGraphicsItem *> my_items_list = items_list;
	
	for (int i = 1 ; i < my_items_list.count() ; ++ i) {
		if (my_items_list[i] -> isSelected()) {
			if (!my_items_list[i - 1] -> isSelected()) {
				my_items_list.swap(i, i - 1);
			}
		}
	}
	
	int z = 1;
	foreach(QGraphicsItem *qgi, my_items_list) redo_hash.insert(qgi, z ++);
}

/**
	Envoie les elements selectionnes au fond
	@param items_list Liste des elements (selectionnes et non selectionnes)
*/
void ChangeZValueCommand::applySendBackward(const QList<QGraphicsItem *> &items_list) {
	QList<QGraphicsItem *> non_selected_items = items_list;
	QList<QGraphicsItem *> selected_items;
	foreach(QGraphicsItem *qgi, non_selected_items) {
		if (qgi -> isSelected()) {
			selected_items << qgi;
			non_selected_items.removeAt(non_selected_items.indexOf(qgi));
		}
	}
	int z = 1;
	foreach(QGraphicsItem *qgi,     selected_items) redo_hash.insert(qgi, z ++);
	foreach(QGraphicsItem *qgi, non_selected_items) redo_hash.insert(qgi, z ++);
}

/**
	Constructeur
	@param elmt ElementScene concernee
	@param old_infos Informations complementaires precedentes
	@param new_infos Nouvelles informations complementaires
	@param parent QUndoCommand parent
*/
ChangeInformationsCommand::ChangeInformationsCommand(ElementScene *elmt, const QString &old_infos, const QString &new_infos, QUndoCommand *parent) :
	ElementEditionCommand(QObject::tr("modification informations complementaires", "undo caption"), elmt, 0, parent),
	old_informations_(old_infos),
	new_informations_(new_infos)
{
}

/// Destructeur
ChangeInformationsCommand::~ChangeInformationsCommand() {
}

/// Annule le changement d'autorisation pour les connexions internes
void ChangeInformationsCommand::undo() {
	editor_scene_ -> setInformations(old_informations_);
}

/// Refait le changement d'autorisation pour les connexions internes
void ChangeInformationsCommand::redo() {
	editor_scene_ -> setInformations(new_informations_);
}

/**
	Constructor
	@param scene Modified ElementScene
	@param parent Parent QUndoCommand
*/
ScalePartsCommand::ScalePartsCommand(ElementScene *scene, QUndoCommand * parent) :
	ElementEditionCommand(scene, 0, parent),
	first_redo(true)
{
}

/**
	Destructor
*/
ScalePartsCommand::~ScalePartsCommand() {
}

/**
	Undo the scaling operation
*/
void ScalePartsCommand::undo() {
	scale(new_rect_, original_rect_);
}

/**
	Redo the scaling operation
*/
void ScalePartsCommand::redo() {
	if (first_redo) {
		first_redo = false;
		return;
	}
	scale(original_rect_, new_rect_);
}

/**
	@return the element editor/scene the command should take place on
*/
ElementScene *ScalePartsCommand::elementScene() const {
	return(editor_scene_);
}

/**
	Set \a primitives as the list of primitives to be scaled by this command
*/
void ScalePartsCommand::setScaledPrimitives(const QList<CustomElementPart *> &primitives) {
	scaled_primitives_ = primitives;
	adjustText();
}

/**
	@return the list of primitives to be scaled by this command
*/
QList<CustomElementPart *> ScalePartsCommand::scaledPrimitives() const {
	return(scaled_primitives_);
}

/**
	Define the transformation applied by this command
	@param original_rect Bounding rectangle for all scaled primitives before the operation
	@param original_rect Bounding rectangle for all scaled primitives after the operation
*/
void ScalePartsCommand::setTransformation(const QRectF &original_rect, const QRectF &new_rect) {
	original_rect_ = original_rect;
	new_rect_ = new_rect;
}

/**
	@return the transformation applied by this command. The returned rectangles
	are the bounding rectangles for all scaled primitives respectively before
	and after the operation.
*/
QPair<QRectF, QRectF> ScalePartsCommand::transformation() {
	return(QPair<QRectF, QRectF>(original_rect_, new_rect_));
}

/**
	Apply the scaling operation from \a before to \a after.
*/
void ScalePartsCommand::scale(const QRectF &before, const QRectF &after) {
	if (!scaled_primitives_.count()) return;
	if (before == after) return;
	if (!before.width() || !before.height()) return; // cowardly flee division by zero FIXME?
	
	foreach (CustomElementPart *part_item, scaled_primitives_) {
		part_item -> startUserTransformation(before);
		part_item -> handleUserTransformation(before, after);
	}
}

/**
	Generate the text describing what this command does exactly.
*/
void ScalePartsCommand::adjustText() {
	if (scaled_primitives_.count() == 1) {
		setText(QObject::tr("redimensionnement %1", "undo caption -- %1 is the resized primitive type name").arg(scaled_primitives_.first() -> name()));
	} else {
		setText(QObject::tr("redimensionnement de %1 primitives", "undo caption -- %1 always > 1").arg(scaled_primitives_.count()));
	}
}
/**
 * @brief ChangePropertiesCommand::ChangePropertiesCommand
 * Change the properties of the drawed element
 * @param scene: scene to belong the property
 * @param type: new type of element.
 * @param context: new info about type.
 * @param parent: parent undo
 */
ChangePropertiesCommand::ChangePropertiesCommand(ElementScene *scene, QString type, DiagramContext info, QUndoCommand *parent) :
	ElementEditionCommand(scene, 0, parent)
{
	m_type << scene->m_elmt_type << type;
	m_info << scene->m_elmt_kindInfo << info;
	setText(QObject::tr("Modifier les propri\351t\351s"));
}

ChangePropertiesCommand::~ChangePropertiesCommand() {}

void ChangePropertiesCommand::undo() {
	editor_scene_-> m_elmt_type = m_type.first();
	editor_scene_-> m_elmt_kindInfo = m_info.first();
}

void ChangePropertiesCommand::redo() {
	editor_scene_-> m_elmt_type = m_type.last();
	editor_scene_-> m_elmt_kindInfo = m_info.last();
}



