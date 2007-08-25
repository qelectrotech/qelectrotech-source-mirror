#ifndef EDITOR_COMMANDS_H
#define EDITOR_COMMANDS_H
#include "customelementpart.h"
#include "elementscene.h"
#include "qgimanager.h"
#include <QtGui>
/**
	Cette classe represente l'action de supprimer une ou plusieurs
	parties lors de l'edition d'un element
*/
class DeletePartsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	DeletePartsCommand(ElementScene *, const QList<QGraphicsItem *>, QUndoCommand * = 0);
	virtual ~DeletePartsCommand();
	private:
	DeletePartsCommand(const DeletePartsCommand &);
	
	// methodes
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Liste des parties supprimees
	QList<QGraphicsItem *> deleted_parts;
	/// scene sur laquelle se produisent les actions
	ElementScene *editor_scene;
};

/**
	Cette classe represente l'action de deplacer une ou plusieurs
	parties lors de l'edition d'un element
*/
class MovePartsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	MovePartsCommand(const QPointF &, ElementScene *, const QList<QGraphicsItem *>, QUndoCommand * = 0);
	virtual ~MovePartsCommand();
	private:
	MovePartsCommand(const MovePartsCommand &);
	
	// methodes
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Liste des parties supprimees
	QList<QGraphicsItem *> moved_parts;
	/// scene sur laquelle se produisent les actions
	ElementScene *editor_scene;
	/// translation appliquee
	QPointF movement;
	/// booleen pour eviter d'appeler redo() lors de la construction de l'objet
	bool first_redo;
};

/**
	Cette classe represente l'action de deplacer une ou plusieurs
	parties lors de l'edition d'un element
*/
class AddPartCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	AddPartCommand(const QString &, ElementScene *, QGraphicsItem *, QUndoCommand * = 0);
	virtual ~AddPartCommand();
	private:
	AddPartCommand(const AddPartCommand &);
	
	// methodes
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Liste des parties supprimees
	QGraphicsItem *part;
	/// scene sur laquelle se produisent les actions
	ElementScene *editor_scene;
	/// booleen pour eviter d'appeler redo() lors de la construction de l'objet
	bool first_redo;
};

/**
	Cette classe represente l'action de modifier une propriete d'une partie
	lors de l'edition d'un element
*/
class ChangePartCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangePartCommand(const QString &, CustomElementPart *, const QString &, const QVariant &, const QVariant &, QUndoCommand * = 0);
	virtual ~ChangePartCommand();
	private:
	ChangePartCommand(const ChangePartCommand &);
	
	// methodes
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Partie modifiee
	CustomElementPart *cep;
	/// Propriete modifiee
	QString property;
	/// ancienne valeur
	QVariant old_value;
	/// nouvelle valeur
	QVariant new_value;
};
#endif
