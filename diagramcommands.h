#ifndef DIAGRAM_COMMANDS_H
#define DIAGRAM_COMMANDS_H
#include "diagram.h"
#include <QtGui>
/**
	Cette classe represente l'action d'ajouter un element au schema
*/
class AddElementCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	AddElementCommand(Diagram *, Element *, const QPointF &, QUndoCommand * = 0);
	virtual ~AddElementCommand();
	private:
	AddElementCommand(const AddElementCommand &);
	
	// methodes
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Element ajoute
	Element *element;
	/// schema sur lequel on ajoute l'element
	Diagram *diagram;
	/// position de l'element sur le schema
	QPointF position;
};

/**
	Cette classe represente l'action d'ajouter un conducteur au schema
*/
class AddConducerCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	AddConducerCommand(Diagram *, Conducer *, QUndoCommand * = 0);
	virtual ~AddConducerCommand();
	private:
	AddConducerCommand(const AddConducerCommand &);
	
	// methodes
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Conducteur ajoute
	Conducer *conducer;
	/// schema auquel on ajoute le conducteur
	Diagram *diagram;
};

/**
	Cette classe represente l'action de supprimer des elements et / ou
	conducteurs d'un schema
*/
class DeleteElementsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	DeleteElementsCommand(Diagram *, QSet<Element *>, QSet<Conducer *>, QUndoCommand * = 0);
	virtual ~DeleteElementsCommand();
	private:
	DeleteElementsCommand(const DeleteElementsCommand &);
	
	// methodes
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Liste des elements enleves
	QSet<Element *> removed_elements;
	/// List des conducteurs enleves
	QSet<Conducer *> removed_conducers;
	/// schema dont on supprime des elements et conducteurs
	Diagram *diagram;
};

/**
	Cette classe represente l'action de coller quelque chose sur un schema
*/
class PasteDiagramCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	PasteDiagramCommand(Diagram *, const QList<Element *> &, const QList<Conducer *> &, QUndoCommand * = 0);
	virtual ~PasteDiagramCommand();
	private:
	PasteDiagramCommand(const PasteDiagramCommand &);
	
	// methodes
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Elements ajoutes
	QList<Element *> elements;
	/// conducteurs ajoutes
	QList<Conducer *> conducers;
	/// schema sur lequel on colle les elements et conducteurs
	Diagram *diagram;
	/// booleen pour empecher le premier appel a redo
	bool first_redo;
};

/**
	Cette classe represente l'action de supprimer des elements et / ou
	conducteurs d'un schema
*/
class CutDiagramCommand : public DeleteElementsCommand {
	// constructeurs, destructeur
	public:
	CutDiagramCommand(Diagram *, QSet<Element *>, QSet<Conducer *>, QUndoCommand * = 0);
	virtual ~CutDiagramCommand();
	private:
	CutDiagramCommand(const CutDiagramCommand &);
};

#endif
