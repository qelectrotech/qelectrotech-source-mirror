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
	QUndoCommand(QObject::tr("suppression"), parent),
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
	foreach(QGraphicsItem *qgi, deleted_parts) {
		editor_scene -> addItem(qgi);
	}
}

/// Supprime les parties
void DeletePartsCommand::redo() {
	foreach(QGraphicsItem *qgi, deleted_parts) {
		editor_scene -> removeItem(qgi);
	}
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
	QUndoCommand(QObject::tr("d\351placement"), parent),
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
	QUndoCommand(QObject::tr("ajout ") + name, parent),
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
	QUndoCommand(QObject::tr("modification ") + name, parent),
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
	QUndoCommand(QObject::tr("modification points polygone"), parent),
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
	QUndoCommand(QObject::tr("modification dimensions/hotspot"), parent),
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
	QUndoCommand(QObject::tr("modification noms"), parent),
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
	@param after Orientationss apres changement
	@param parent QUndoCommand parent
*/
ChangeOrientationsCommand::ChangeOrientationsCommand(
	ElementScene *element_scene,
	const OrientationSet &before,
	const OrientationSet &after,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("modification orientations"), parent),
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

