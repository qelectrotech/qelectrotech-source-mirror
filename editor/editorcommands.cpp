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
	@param parts Liste des parties deplacees
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

/// Annule le deplacement
void AddPartCommand::undo() {
	editor_scene -> removeItem(part);
}

/// Refait le deplacement
void AddPartCommand::redo() {
	// le premier appel a redo, lors de la construction de l'objet, ne doit pas se faire
	if (first_redo) {
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
