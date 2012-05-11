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
#include "editorcommands.h"

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
	QUndoCommand(QObject::tr("suppression", "undo caption"), parent),
	deleted_parts(parts),
	editor_scene(scene)
{
	foreach(QGraphicsItem *qgi, deleted_parts) {
		editor_scene -> qgiManager().manage(qgi);
	}
}

/// Destructeur : detruit egalement les parties supprimees
DeletePartsCommand::~DeletePartsCommand() {
	foreach(QGraphicsItem *qgi, deleted_parts) {
		editor_scene -> qgiManager().release(qgi);
	}
}

/// Restaure les parties supprimees
void DeletePartsCommand::undo() {
	editor_scene -> blockSignals(true);
	foreach(QGraphicsItem *qgi, deleted_parts) {
		editor_scene -> addItem(qgi);
	}
	editor_scene -> blockSignals(false);
}

/// Supprime les parties
void DeletePartsCommand::redo() {
	editor_scene -> blockSignals(true);
	foreach(QGraphicsItem *qgi, deleted_parts) {
		editor_scene -> removeItem(qgi);
	}
	editor_scene -> blockSignals(false);
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
	QUndoCommand(parent),
	content_(c),
	editor_view_(view),
	editor_scene_(view -> scene()),
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
	QUndoCommand(QObject::tr("d\351placement", "undo caption"), parent),
	movement(m),
	first_redo(true)
{
	moved_parts = parts;
	editor_scene  = scene;
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
	QUndoCommand(QString(QObject::tr("ajout %1", "undo caption")).arg(name), parent),
	part(p),
	editor_scene(scene),
	first_redo(true)
{
	editor_scene -> qgiManager().manage(part);
}

/// Destructeur
AddPartCommand::~AddPartCommand() {
	editor_scene -> qgiManager().release(part);
}

/// Annule l'ajout
void AddPartCommand::undo() {
	editor_scene -> removeItem(part);
}

/// Refait l'ajout
void AddPartCommand::redo() {
	// le premier appel a redo, lors de la construction de l'objet, ne doit pas se faire
	if (first_redo) {
		part -> setZValue(editor_scene -> items().count());
		editor_scene -> clearSelection();
		part -> setSelected(true);
		first_redo = false;
		return;
	}
	editor_scene -> addItem(part);
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
	const QString &prop,
	const QVariant &old_v,
	const QVariant &new_v,
	QUndoCommand *parent
) :
	QUndoCommand(QString(QObject::tr("modification %1", "undo caption")).arg(name), parent),
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
	QUndoCommand(QObject::tr("modification points polygone", "undo caption"), parent),
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
	@param size_1 Dimensions de l'element avant le changement
	@param size_2 Dimensions de l'element apres le changement
	@param hotspot_1 Point de saisie de l'element avant le changement
	@param hotspot_2 Point de saisie de l'element apres le changement
	@param o Eventuel decalage a appliquer aux parties de l'element edite
	@param parent QUndoCommand parent
*/
ChangeHotspotCommand::ChangeHotspotCommand(
	ElementScene *element_scene,
	const QSize  &size_1,
	const QSize  &size_2,
	const QPoint &hotspot_1,
	const QPoint &hotspot_2,
	const QPoint &o,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("modification dimensions/hotspot", "undo caption"), parent),
	element(element_scene),
	size_before(size_1),
	size_after(size_2),
	hotspot_before(hotspot_1),
	hotspot_after(hotspot_2),
	offset(o)
{
}

/// Destructeur
ChangeHotspotCommand::~ChangeHotspotCommand() {
}

/// Annule le changement
void ChangeHotspotCommand::undo() {
	QRectF sc(element -> sceneContent());
	
	element -> setWidth(size_before.width());
	element -> setHeight(size_before.height());
	element -> setHotspot(hotspot_before);
	if (!offset.isNull()) applyOffset(-offset);
	
	element -> update(element -> sceneContent().unite(sc));
}

/// Refait le changement
void ChangeHotspotCommand::redo() {
	QRectF sc(element -> sceneContent());
	
	element -> setWidth(size_after.width());
	element -> setHeight(size_after.height());
	element -> setHotspot(hotspot_after);
	if (!offset.isNull()) applyOffset(offset);
	
	element -> update(element -> sceneContent().unite(sc));
}

/**
	Applique une translation aux parties de l'element edite
	@param o Translation a appliquer
*/
void ChangeHotspotCommand::applyOffset(const QPointF &o) {
	foreach(QGraphicsItem *qgi, element -> items()) {
		qgi -> translate(o.x(), o.y());
	}
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
	QUndoCommand(QObject::tr("modification noms", "undo caption"), parent),
	names_before(before),
	names_after(after),
	element(element_scene)
{
}

/// Destructeur
ChangeNamesCommand::~ChangeNamesCommand() {
}

/// Annule le changement
void ChangeNamesCommand::undo() {
	element -> setNames(names_before);
}

/// Refait le changement
void ChangeNamesCommand::redo() {
	element -> setNames(names_after);
}

/**
	Constructeur
	@param element_scene Element edite
	@param before Orientations avant changement
	@param after Orientations apres changement
	@param parent QUndoCommand parent
*/
ChangeOrientationsCommand::ChangeOrientationsCommand(
	ElementScene *element_scene,
	const OrientationSet &before,
	const OrientationSet &after,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("modification orientations", "undo caption"), parent),
	ori_before(before),
	ori_after(after),
	element(element_scene)
{
}

/// Destructeur
ChangeOrientationsCommand::~ChangeOrientationsCommand() {
}

/// Annule le changement
void ChangeOrientationsCommand::undo() {
	element -> setOrientations(ori_before);
}

/// Refait le changement
void ChangeOrientationsCommand::redo() {
	element -> setOrientations(ori_after);
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
	QUndoCommand(parent),
	element(elmt),
	option(o)
{
	// recupere les parties de l'elements, sauf les bornes
	QList<QGraphicsItem *> items_list = element -> zItems();
	
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
	@param allow true pour que les connexions internes soient acceptees, false sinon
	@param parent QUndoCommand parent
*/
AllowInternalConnectionsCommand::AllowInternalConnectionsCommand(ElementScene *elmt, bool allow, QUndoCommand *parent) :
	QUndoCommand(QObject::tr("modification connexions internes", "undo caption"), parent),
	element(elmt),
	ic(allow)
{
}

/// Destructeur
AllowInternalConnectionsCommand::~AllowInternalConnectionsCommand() {
}

/// Annule le changement d'autorisation pour les connexions internes
void AllowInternalConnectionsCommand::undo() {
	element -> setInternalConnections(!ic);
}

/// Refait le changement d'autorisation pour les connexions internes
void AllowInternalConnectionsCommand::redo() {
	element -> setInternalConnections(ic);
}

/**
	Constructeur
	@param elmt ElementScene concernee
	@param old_infos Informations complementaires precedentes
	@param new_infos Nouvelles informations complementaires
	@param parent QUndoCommand parent
*/
ChangeInformationsCommand::ChangeInformationsCommand(ElementScene *elmt, const QString &old_infos, const QString &new_infos, QUndoCommand *parent) :
	QUndoCommand(QObject::tr("modification informations complementaires", "undo caption"), parent),
	element(elmt),
	old_informations_(old_infos),
	new_informations_(new_infos)
{
}

/// Destructeur
ChangeInformationsCommand::~ChangeInformationsCommand() {
}

/// Annule le changement d'autorisation pour les connexions internes
void ChangeInformationsCommand::undo() {
	element -> setInformations(old_informations_);
}

/// Refait le changement d'autorisation pour les connexions internes
void ChangeInformationsCommand::redo() {
	element -> setInformations(new_informations_);
}
