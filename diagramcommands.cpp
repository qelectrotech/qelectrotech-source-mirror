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
	QUndoCommand(QObject::tr("ajouter 1 ") + elmt -> nom(), parent),
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
	QUndoCommand(QObject::tr("ajouter un conducteur"), parent),
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
	@param dia Schema dont on supprime des elements et conducteurs
	@param elements Elements supprimes
	@param conducers Conducteurs supprimes
	@param parent QUndoCommand parent
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
	setText(QObject::tr("supprimer ") + QET::ElementsAndConducersSentence(removed_elements.count(), removed_conducers.count()));
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

/**
	Constructeur
	@param dia Schema sur lequel on colle les elements et conducteurs
	@param e Elements colles sur le schema
	@param c Conducteurs colles sur le schema
	@param parent QUndoCommand parent
*/
PasteDiagramCommand::PasteDiagramCommand(
	Diagram *dia,
	const QList<Element *> &e,
	const QList<Conducer *> &c,
	QUndoCommand *parent
) :
	QUndoCommand(parent),
	elements(e),
	conducers(c),
	diagram(dia),
	first_redo(true)
{
	setText(QObject::tr("coller ") + QET::ElementsAndConducersSentence(elements.count(), conducers.count()));
	foreach(QGraphicsItem *qgi, elements)  diagram -> qgiManager().manage(qgi);
	foreach(QGraphicsItem *qgi, conducers) diagram -> qgiManager().manage(qgi);
}

/// Destructeur
PasteDiagramCommand::~PasteDiagramCommand() {
	foreach(QGraphicsItem *qgi, elements)  diagram -> qgiManager().release(qgi);
	foreach(QGraphicsItem *qgi, conducers) diagram -> qgiManager().release(qgi);
}

/// annule le coller
void PasteDiagramCommand::undo() {
	// enleve les conducteurs
	foreach(Conducer *c, conducers) {
		c -> terminal1 -> removeConducer(c);
		c -> terminal2 -> removeConducer(c);
		diagram -> removeItem(c);
	}
	
	// enleve les elements
	foreach(Element *e, elements)  diagram -> removeItem(e);
}

/// refait le coller
void PasteDiagramCommand::redo() {
	if (first_redo) first_redo = false;
	else {
		// pose les elements
		foreach(Element *e, elements)  diagram -> addItem(e);
		
		// pose les conducteurs
		foreach(Conducer *c, conducers) {
			diagram -> addItem(c);
			c -> terminal1 -> addConducer(c);
			c -> terminal2 -> addConducer(c);
		}
	}
	foreach(Element *e, elements)   e -> setSelected(true);
	foreach(Conducer *c, conducers) c -> setSelected(true);
}

/**
	Constructeur
	@param dia Schema dont on supprime des elements et conducteurs
	@param elements Elements supprimes
	@param conducers Conducteurs supprimes
	@param parent QUndoCommand parent
*/
CutDiagramCommand::CutDiagramCommand(
	Diagram *dia,
	QSet<Element *> elements,
	QSet<Conducer *> conducers,
	QUndoCommand *parent
) : 
	DeleteElementsCommand(dia, elements, conducers, parent)
{
	setText(QObject::tr("couper ") + QET::ElementsAndConducersSentence(elements.count(), conducers.count()));
}

/// Destructeur
CutDiagramCommand::~CutDiagramCommand() {
}

/**
	Constructeur
	@param dia Schema sur lequel on deplace des elements
	@param move_elements Elements a deplacer
	@param move_conducers Conducteurs a deplacer
	@param modify_conducers Conducteurs a mettre a jour
	@param m translation subie par les elements
	@param parent QUndoCommand parent
*/
MoveElementsCommand::MoveElementsCommand(
	Diagram *dia,
	const QSet<Element *> &move_elements,
	const QSet<Conducer *> &move_conducers,
	const QHash<Conducer *, Terminal *> &modify_conducers,
	const QPointF &m,
	QUndoCommand *parent
) :
	QUndoCommand(parent),
	diagram(dia),
	elements_to_move(move_elements),
	conducers_to_move(move_conducers),
	conducers_to_update(modify_conducers),
	movement(m)
{
	setText(QObject::tr("d\351placer ") + QET::ElementsAndConducersSentence(elements_to_move.count(), conducers_to_move.count()));
}

/// Destructeur
MoveElementsCommand::~MoveElementsCommand() {
}

/// annule le deplacement
void MoveElementsCommand::undo() {
	move(-movement);
}

/// refait le deplacement
void MoveElementsCommand::redo() {
	if (first_redo) first_redo = false;
	else move(movement);
}

/**
	deplace les elements et conducteurs
	@param actual_movement translation a effectuer sur les elements et conducteurs
*/
void MoveElementsCommand::move(const QPointF &actual_movement) {
	// deplace les elements
	foreach(Element *element, elements_to_move) {
		element -> setPos(element -> pos() + actual_movement);
	}
	
	// deplace certains conducteurs
	foreach(Conducer *conducer, conducers_to_move) {
		conducer -> setPos(conducer -> pos() + actual_movement);
	}
	
	// recalcule les autres conducteurs
	foreach(Conducer *conducer, conducers_to_update.keys()) {
		conducer -> updateWithNewPos(QRectF(), conducers_to_update[conducer], conducers_to_update[conducer] -> amarrageConducer());
	}
}

/**
	Constructeur
	@param dti Champ de texte modifie
	@param before texte avant
	@param after texte apres
	@param parent QUndoCommand parent
*/
ChangeDiagramTextCommand::ChangeDiagramTextCommand(
	DiagramTextItem *dti,
	const QString &before,
	const QString &after,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("modifier le texte"), parent),
	text_item(dti),
	text_before(before),
	text_after(after),
	first_redo(true)
{
}

/// destructeur
ChangeDiagramTextCommand::~ChangeDiagramTextCommand() {
}

/// annule la modification de texte
void ChangeDiagramTextCommand::undo() {
	text_item -> setPlainText(text_before);
	text_item -> previous_text = text_before;
}

/// refait la modification de texte
void ChangeDiagramTextCommand::redo() {
	if (first_redo) first_redo = false;
	else {
		text_item -> setPlainText(text_after);
		text_item -> previous_text = text_after;
	}
}

/**
	Constructeur
	@param elements Elements a pivoter associes a leur orientation d'origine
	@param parent QUndoCommand parent
*/
RotateElementsCommand::RotateElementsCommand(const QHash<Element *, QET::Orientation> &elements, QUndoCommand *parent) :
	QUndoCommand(QObject::tr("pivoter ") + QET::ElementsAndConducersSentence(elements.count(), 0), parent),
	elements_to_rotate(elements)
{
}

/// Destructeur
RotateElementsCommand::~RotateElementsCommand() {
}

/// defait le pivotement
void RotateElementsCommand::undo() {
	foreach(Element *e, elements_to_rotate.keys()) {
		e -> setOrientation(elements_to_rotate[e]);
	}
}

/// refait le pivotement
void RotateElementsCommand::redo() {
	foreach(Element *e, elements_to_rotate.keys()) {
		e -> setOrientation(e -> orientation().next());
		e -> update();
	}
}

/**
	Constructeur
	@param c Conducteur modifie
	@param old_p ancien profil du conducteur
	@param new_p nouveau profil du conducteur
	@param parent QUndoCommand parent
*/
ChangeConducerCommand::ChangeConducerCommand(
	Conducer *c,
	const ConducerProfile &old_p,
	const ConducerProfile &new_p,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("modifier un conducteur"), parent),
	conducer(c),
	old_profile(old_p),
	new_profile(new_p),
	first_redo(true)
{
}

/// Destructeur
ChangeConducerCommand::~ChangeConducerCommand() {
}

/// Annule la modification du conducteur
void ChangeConducerCommand::undo() {
	conducer -> setProfile(old_profile);
}

/// Refait la modification du conducteur
void ChangeConducerCommand::redo() {
	if (first_redo) first_redo = false;
	else conducer -> setProfile(new_profile);
}

/**
	Constructeur
	@param d Schema dont on modifie le cartouche
	@param old_ip Anciennes proprietes du cartouches
	@param new_ip Nouvelles proprietes du cartouches
	@param parent QUndoCommand parent
*/
ChangeInsetCommand::ChangeInsetCommand(
	Diagram *d,
	const InsetProperties &old_ip,
	const InsetProperties &new_ip,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("modifier le cartouche"), parent),
	diagram(d),
	old_inset(old_ip),
	new_inset(new_ip)
{
}

/// Destructeur
ChangeInsetCommand::~ChangeInsetCommand() {
}

/// Annule la modification de cartouche
void ChangeInsetCommand::undo() {
	diagram -> border_and_inset.importInset(old_inset);
}

/// Refait la modification de cartouche
void ChangeInsetCommand::redo() {
	diagram -> border_and_inset.importInset(new_inset);
}
