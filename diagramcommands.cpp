#include "diagramcommands.h"
#include "element.h"
#include "conducer.h"
#include "diagram.h"
#include "qgimanager.h"
/**
	Constructeur
	@param name Nom de la partie ajoutee
	@param parts Liste des parties deplacees
	@param parent QUndoCommand parent
*/
AddElementCommand::AddElementCommand(
	Diagram *d,
	Element *elmt,
	const QPointF &p,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("ajout ") + elmt -> nom(), parent),
	element(elmt),
	diagram(d),
	position(p)
{
	diagram -> qgiManager().manage(element);
}

/// Destructeur
AddElementCommand::~AddElementCommand() {
	diagram -> qgiManager().release(element);
}

/// Annule l'ajout
void AddElementCommand::undo() {
	diagram -> removeItem(element);
}

/// Refait l'ajout
void AddElementCommand::redo() {
	diagram -> addItem(element);
	element -> setPos(position);
	element -> setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
}

/**
	Constructeur
	@param d Schema auquel on ajoute un conducteur
	@param t1 Premiere borne du conducteur
	@param t2 Seconde borne du conducteur
	@param parent QUndoCommand parent
*/
AddConducerCommand::AddConducerCommand(
	Diagram *d,
	Conducer *c,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("ajout conducteur"), parent),
	conducer(c),
	diagram(d)
{
	diagram -> qgiManager().manage(conducer);
}

/// Destructeur
AddConducerCommand::~AddConducerCommand() {
	diagram -> qgiManager().release(conducer);
}

/// Annule l'ajout
void AddConducerCommand::undo() {
	// detache le conducteur sans le detruire
	conducer -> terminal1 -> removeConducer(conducer);
	conducer -> terminal2 -> removeConducer(conducer);
	diagram -> removeItem(conducer);
}

/// Refait l'ajout
void AddConducerCommand::redo() {
	diagram -> addItem(conducer);
}

/**
	Constructeur
*/
DeleteElementsCommand::DeleteElementsCommand(
	Diagram *dia,
	QSet<Element *> elements,
	QSet<Conducer *> conducers,
	QUndoCommand *parent
) :
	QUndoCommand(parent),
	removed_elements(elements),
	removed_conducers(conducers),
	diagram(dia)
{
	foreach(QGraphicsItem *qgi, removed_elements)  diagram -> qgiManager().manage(qgi);
	foreach(QGraphicsItem *qgi, removed_conducers) diagram -> qgiManager().manage(qgi);
}

/// Destructeur
DeleteElementsCommand::~DeleteElementsCommand() {
	foreach(QGraphicsItem *qgi, removed_elements)  diagram -> qgiManager().release(qgi);
	foreach(QGraphicsItem *qgi, removed_conducers) diagram -> qgiManager().release(qgi);
}

/// annule les suppressions
void DeleteElementsCommand::undo() {
	// remet les elements
	foreach(Element *e, removed_elements) {
		diagram -> addItem(e);
	}
	
	// remet les conducteurs
	foreach(Conducer *c, removed_conducers) {
		diagram -> addItem(c);
		c -> terminal1 -> addConducer(c);
		c -> terminal2 -> addConducer(c);
	}
}

/// refait les suppressions
void DeleteElementsCommand::redo() {
	// enleve les conducteurs
	foreach(Conducer *c, removed_conducers) {
		c -> terminal1 -> removeConducer(c);
		c -> terminal2 -> removeConducer(c);
		diagram -> removeItem(c);
	}
	
	// enleve les elements
	foreach(Element *e, removed_elements) {
		diagram -> removeItem(e);
	}
}
