/*
	Copyright 2006-2010 Xavier Guerrin
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
#include "diagramcommands.h"
#include "element.h"
#include "conductor.h"
#include "diagram.h"
#include "elementtextitem.h"
#include "independenttextitem.h"
#include "qgimanager.h"
#include "diagram.h"
#include "diagramtextitem.h"

/**
	Constructeur
	@param d Schema auquel on ajoute un element
	@param elmt Element ajoute
	@param p Position a laquelle l'element est ajoute
	@param parent QUndoCommand parent
*/
AddElementCommand::AddElementCommand(
	Diagram *d,
	Element *elmt,
	const QPointF &p,
	QUndoCommand *parent
) :
	QUndoCommand(QString(QObject::tr("ajouter 1 %1", "undo caption - %1 is an element name")).arg(elmt -> name()), parent),
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
	diagram -> removeElement(element);
}

/// Refait l'ajout
void AddElementCommand::redo() {
	diagram -> addElement(element);
	element -> setPos(position);
	element -> setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
}

/**
	Constructeur
	@param dia Schema auquel on ajoute du texte
	@param text Texte ajoute
	@param pos Position a laquelle le texte est ajoute
	@param parent QUndoCommand parent
*/
AddTextCommand::AddTextCommand(Diagram *dia, IndependentTextItem *text, const QPointF &pos, QUndoCommand *parent) :
	QUndoCommand(QObject::tr("Ajouter un champ de texte", "undo caption"), parent),
	textitem(text),
	diagram(dia),
	position(pos)
{
	diagram -> qgiManager().manage(textitem);
}

/// Destructeur
AddTextCommand::~AddTextCommand() {
	diagram -> qgiManager().release(textitem);
}

/// Annule l'ajout
void AddTextCommand::undo() {
	diagram -> removeIndependentTextItem(textitem);
}

/// Refait l'ajout
void AddTextCommand::redo() {
	diagram -> addIndependentTextItem(textitem);
	textitem -> setPos(position);
}

/**
	Constructeur
	@param d Schema auquel on ajoute un conducteur
	@param c Conducteur ajoute
	@param parent QUndoCommand parent
*/
AddConductorCommand::AddConductorCommand(
	Diagram *d,
	Conductor *c,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("ajouter un conducteur", "undo caption"), parent),
	conductor(c),
	diagram(d)
{
	diagram -> qgiManager().manage(conductor);
}

/// Destructeur
AddConductorCommand::~AddConductorCommand() {
	diagram -> qgiManager().release(conductor);
}

/// Annule l'ajout
void AddConductorCommand::undo() {
	diagram -> removeConductor(conductor);
}

/// Refait l'ajout
void AddConductorCommand::redo() {
	diagram -> addConductor(conductor);
}

/**
	Constructeur
	@param dia Schema dont on supprime des elements et conducteurs
	@param content Contenu supprime
	@param parent QUndoCommand parent
*/
DeleteElementsCommand::DeleteElementsCommand(
	Diagram *dia,
	const DiagramContent &content,
	QUndoCommand *parent
) :
	QUndoCommand(parent),
	removed_content(content),
	diagram(dia)
{
	setText(
		QString(
			QObject::tr(
				"supprimer %1",
				"undo caption - %1 is a sentence listing the removed content"
			)
		).arg(removed_content.sentence(DiagramContent::All))
	);
	diagram -> qgiManager().manage(removed_content.items(DiagramContent::All));
}

/// Destructeur
DeleteElementsCommand::~DeleteElementsCommand() {
	diagram -> qgiManager().release(removed_content.items(DiagramContent::All));
}

/// annule les suppressions
void DeleteElementsCommand::undo() {
	// remet les elements
	foreach(Element *e, removed_content.elements) {
		diagram -> addElement(e);
	}
	
	// remet les conducteurs
	foreach(Conductor *c, removed_content.conductors(DiagramContent::AnyConductor)) {
		diagram -> addConductor(c);
	}
	
	// remet les textes
	foreach(IndependentTextItem *t, removed_content.textFields) {
		diagram -> addIndependentTextItem(t);
	}
}

/// refait les suppressions
void DeleteElementsCommand::redo() {
	// enleve les conducteurs
	foreach(Conductor *c, removed_content.conductors(DiagramContent::AnyConductor)) {
		diagram -> removeConductor(c);
	}
	
	// enleve les elements
	foreach(Element *e, removed_content.elements) {
		diagram -> removeElement(e);
	}
	
	// enleve les textes
	foreach(IndependentTextItem *t, removed_content.textFields) {
		diagram -> removeIndependentTextItem(t);
	}
}

/**
	Constructeur
	@param dia Schema sur lequel on colle les elements et conducteurs
	@param c Contenu a coller sur le schema
	@param parent QUndoCommand parent
*/
PasteDiagramCommand::PasteDiagramCommand(
	Diagram *dia,
	const DiagramContent &c,
	QUndoCommand *parent
) :
	QUndoCommand(parent),
	content(c),
	diagram(dia),
	filter(DiagramContent::Elements|DiagramContent::TextFields|DiagramContent::ConductorsToMove),
	first_redo(true)
{
	
	setText(
		QString(
			QObject::tr(
				"coller %1",
				"undo caption - %1 is a sentence listing the content to paste"
			).arg(content.sentence(filter))
		)
	);
	diagram -> qgiManager().manage(content.items(filter));
}

/// Destructeur
PasteDiagramCommand::~PasteDiagramCommand() {
	diagram -> qgiManager().release(content.items(filter));
}

/// annule le coller
void PasteDiagramCommand::undo() {
	// enleve les conducteurs
	foreach(Conductor *c, content.conductorsToMove) diagram -> removeConductor(c);
	
	// enleve les elements
	foreach(Element *e, content.elements) diagram -> removeElement(e);
	
	// enleve les textes
	foreach(IndependentTextItem *t, content.textFields) diagram -> removeIndependentTextItem(t);
}

/// refait le coller
void PasteDiagramCommand::redo() {
	if (first_redo) first_redo = false;
	else {
		// pose les elements
		foreach(Element *e, content.elements)  diagram -> addElement(e);
		
		// pose les conducteurs
		foreach(Conductor *c, content.conductorsToMove) diagram -> addConductor(c);
		
		// pose les textes
		foreach(IndependentTextItem *t, content.textFields) diagram -> addIndependentTextItem(t);
	}
	foreach(Element *e, content.elements) e -> setSelected(true);
	foreach(Conductor *c, content.conductorsToMove) c -> setSelected(true);
	foreach(IndependentTextItem *t, content.textFields) t -> setSelected(true);
}

/**
	Constructeur
	@param dia Schema dont on coupe des elements et conducteurs
	@param content Contenu coupe
	@param parent QUndoCommand parent
*/
CutDiagramCommand::CutDiagramCommand(
	Diagram *dia,
	const DiagramContent &content,
	QUndoCommand *parent
) : 
	DeleteElementsCommand(dia, content, parent)
{
	setText(
		QString(
			QObject::tr(
				"couper %1",
				"undo caption - %1 is a sentence listing the content to cut"
			).arg(content.sentence(DiagramContent::All))
		)
	);
}

/// Destructeur
CutDiagramCommand::~CutDiagramCommand() {
}

/**
	Constructeur
	@param dia Schema sur lequel on deplace des elements
	@param diagram_content Contenu a deplacer
	@param m translation subie par les elements
	@param parent QUndoCommand parent
*/
MoveElementsCommand::MoveElementsCommand(
	Diagram *dia,
	const DiagramContent &diagram_content,
	const QPointF &m,
	QUndoCommand *parent
) :
	QUndoCommand(parent),
	diagram(dia),
	content_to_move(diagram_content),
	movement(m),
	first_redo(true)
{
	QString moved_content_sentence = content_to_move.sentence(
		DiagramContent::Elements |
		DiagramContent::TextFields |
		DiagramContent::ConductorsToUpdate |
		DiagramContent::ConductorsToMove
	);
	
	setText(
		QString(
			QObject::tr(
				"d\351placer %1",
				"undo caption - %1 is a sentence listing the moved content"
			).arg(moved_content_sentence)
		)
	);
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
	foreach(Element *element, content_to_move.elements) {
		element -> setPos(element -> pos() + actual_movement);
	}
	
	// deplace certains conducteurs
	foreach(Conductor *conductor, content_to_move.conductorsToMove) {
		conductor -> setPos(conductor -> pos() + actual_movement);
	}
	
	// recalcule les autres conducteurs
	foreach(Conductor *conductor, content_to_move.conductorsToUpdate.keys()) {
		conductor -> updateWithNewPos(
			QRectF(),
			content_to_move.conductorsToUpdate[conductor],
			content_to_move.conductorsToUpdate[conductor] -> dockConductor()
		);
	}
	
	// deplace les textes
	foreach(DiagramTextItem *text, content_to_move.textFields) {
		text -> setPos(text -> pos() + actual_movement);
	}
}

/**
	Constructeur
	@param diagram Schema sur lequel on deplace des champs de texte
	@param texts Liste des textes deplaces
	@param m translation subie par les elements
	@param parent QUndoCommand parent
*/
MoveElementsTextsCommand::MoveElementsTextsCommand(
	Diagram *diagram,
	const QSet<ElementTextItem *> &texts,
	const QPointF &m,
	QUndoCommand *parent
) :
	QUndoCommand(parent),
	diagram(diagram),
	texts_to_move(texts),
	movement(m),
	first_redo(true)
{
	QString moved_content_sentence = QET::ElementsAndConductorsSentence(0, 0, texts_to_move.count());
	
	setText(
		QString(
			QObject::tr(
				"d\351placer %1",
				"undo caption - %1 is a sentence listing the moved content"
			).arg(moved_content_sentence)
		)
	);
}

/// Destructeur
MoveElementsTextsCommand::~MoveElementsTextsCommand() {
}

/// annule le deplacement
void MoveElementsTextsCommand::undo() {
	move(-movement);
}

/// refait le deplacement
void MoveElementsTextsCommand::redo() {
	if (first_redo) first_redo = false;
	else move(movement);
}

/**
	deplace les elements et conducteurs
	@param actual_movement translation a effectuer sur les elements et conducteurs
*/
void MoveElementsTextsCommand::move(const QPointF &actual_movement) {
	// deplace les textes
	foreach(ElementTextItem *text, texts_to_move) {
		QPointF applied_movement = text -> mapMovementToParent(text -> mapMovementFromScene(actual_movement));
		text -> setPos(text -> pos() + applied_movement);
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
	QUndoCommand(QObject::tr("modifier le texte", "undo caption"), parent),
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
}

/// refait la modification de texte
void ChangeDiagramTextCommand::redo() {
	if (first_redo) {
		first_redo = false;
	} else {
		text_item -> setPlainText(text_after);
	}
}

/**
	Constructeur
	@param elements Elements a pivoter associes a leur orientation d'origine
	@param texts Textes a pivoter
	@param parent QUndoCommand parent
*/
RotateElementsCommand::RotateElementsCommand(const QHash<Element *, QET::Orientation> &elements, const QList<DiagramTextItem *> &texts, QUndoCommand *parent) :
	QUndoCommand(parent),
	elements_to_rotate(elements),
	texts_to_rotate(texts),
	applied_rotation_angle_(-90.0)
{
	setText(
		QString(
			QObject::tr(
				"pivoter %1",
				"undo caption - %1 is a sentence listing the rotated content"
			)
		).arg(QET::ElementsAndConductorsSentence(elements.count(), 0, texts.count()))
	);
}

/// Destructeur
RotateElementsCommand::~RotateElementsCommand() {
}

/// defait le pivotement
void RotateElementsCommand::undo() {
	foreach(Element *e, elements_to_rotate.keys()) {
		e -> setOrientation(elements_to_rotate[e]);
	}
	foreach(DiagramTextItem *dti, texts_to_rotate) {
		dti -> rotateBy(-applied_rotation_angle_);
	}
}

/// refait le pivotement
void RotateElementsCommand::redo() {
	foreach(Element *e, elements_to_rotate.keys()) {
		e -> setOrientation(e -> orientation().next());
		e -> update();
	}
	foreach(DiagramTextItem *dti, texts_to_rotate) {
		dti -> rotateBy(applied_rotation_angle_);
	}
}

/**
	@return l'angle de rotation applique aux textes
*/
qreal RotateElementsCommand::appliedRotationAngle() const {
	return(applied_rotation_angle_);
}

/**
	@param angle l'angle de rotation a appliquer aux textes
*/
void RotateElementsCommand::setAppliedRotationAngle(const qreal &angle) {
	applied_rotation_angle_ = QET::correctAngle(angle);
}

/**
	Constructeur
	@param previous_state Hash associant les textes impactes par l'action et leur angle de rotation avant l'action
	@param applied_rotation Nouvel angle de rotation, a appliquer au textes concernes
	@param parent QUndoCommand parent
*/
RotateTextsCommand::RotateTextsCommand(const QHash<DiagramTextItem *, double> &previous_state, double applied_rotation, QUndoCommand *parent) :
	QUndoCommand(parent),
	texts_to_rotate(previous_state),
	applied_rotation_angle_(applied_rotation)
{
	defineCommandName();
}

/**
	Constructeur
	@param texts Liste des textes impactes par l'action. L'objet retiendra leur angle de rotation au moment de sa construction.
	@param applied_rotation Nouvel angle de rotation, a appliquer au textes concernes
	@param parent QUndoCommand parent
*/
RotateTextsCommand::RotateTextsCommand(const QList<DiagramTextItem *> &texts, double applied_rotation, QUndoCommand *parent) :
	QUndoCommand(parent),
	applied_rotation_angle_(applied_rotation)
{
	foreach(DiagramTextItem *text, texts) {
		texts_to_rotate.insert(text, text -> rotationAngle());
	}
	defineCommandName();
}

/**
	Destructeur
*/
RotateTextsCommand::~RotateTextsCommand() {
}

/**
	Annule la rotation des textes
*/
void RotateTextsCommand::undo() {
	foreach(DiagramTextItem *text, texts_to_rotate.keys()) {
		text -> setRotationAngle(texts_to_rotate[text]);
	}
}

/**
	Applique l'angle de rotation aux textes
*/
void RotateTextsCommand::redo() {
	foreach(DiagramTextItem *text, texts_to_rotate.keys()) {
		text -> setRotationAngle(applied_rotation_angle_);
	}
}

/**
	Definit le nom de la commande d'annulation
*/
void RotateTextsCommand::defineCommandName() {
	setText(
		QString(
			QObject::tr(
				"orienter %1 \340 %2\260",
				"undo caption - %1 looks like '42 texts', %2 is a rotation angle"
			)
		).arg(QET::ElementsAndConductorsSentence(0, 0, texts_to_rotate.count()))
		.arg(applied_rotation_angle_)
	);
}

/**
	Constructeur
	@param c Conducteur modifie
	@param old_p ancien profil du conducteur
	@param new_p nouveau profil du conducteur
	@param path_t Trajectoire du trajet modifie
	@param parent QUndoCommand parent
*/
ChangeConductorCommand::ChangeConductorCommand(
	Conductor *c,
	const ConductorProfile &old_p,
	const ConductorProfile &new_p,
	Qt::Corner path_t,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("modifier un conducteur", "undo caption"), parent),
	conductor(c),
	old_profile(old_p),
	new_profile(new_p),
	path_type(path_t),
	first_redo(true)
{
}

/// Destructeur
ChangeConductorCommand::~ChangeConductorCommand() {
}

/// Annule la modification du conducteur
void ChangeConductorCommand::undo() {
	conductor -> setProfile(old_profile, path_type);
}

/// Refait la modification du conducteur
void ChangeConductorCommand::redo() {
	if (first_redo) first_redo = false;
	else conductor -> setProfile(new_profile, path_type);
}

/**
	Constructeur
	@param cp Conducteurs reinitialises, associes a leur ancien profil
	@param parent QUndoCommand parent
*/
ResetConductorCommand::ResetConductorCommand(
	const QHash<Conductor *, ConductorProfilesGroup> &cp,
	QUndoCommand *parent
) :
	QUndoCommand(parent),
	conductors_profiles(cp)
{
	setText(
		QObject::tr(
			"R\351initialiser %1",
			"undo caption - %1 is a sentence listing the reset content"
		).arg(QET::ElementsAndConductorsSentence(0, cp.count()))
	);
}

/// Destructeur
ResetConductorCommand::~ResetConductorCommand() {
}

/// Annule la reinitialisation des conducteurs
void ResetConductorCommand::undo() {
	foreach(Conductor *c, conductors_profiles.keys()) {
		c -> setProfiles(conductors_profiles[c]);
	}
}

/// Refait la reinitialisation des conducteurs
void ResetConductorCommand::redo() {
	foreach(Conductor *c, conductors_profiles.keys()) {
		c -> setProfiles(ConductorProfilesGroup());
	}
}

/**
	Constructeur
	@param d Schema dont on modifie le cartouche
	@param old_ip Anciennes proprietes du cartouche
	@param new_ip Nouvelles proprietes du cartouche
	@param parent QUndoCommand parent
*/
ChangeInsetCommand::ChangeInsetCommand(
	Diagram *d,
	const InsetProperties &old_ip,
	const InsetProperties &new_ip,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("modifier le cartouche", "undo caption"), parent),
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
	diagram -> invalidate(diagram -> border());
}

/// Refait la modification de cartouche
void ChangeInsetCommand::redo() {
	diagram -> border_and_inset.importInset(new_inset);
	diagram -> invalidate(diagram -> border());
}

/**
	Constructeur
	@param dia Schema modifie
	@param old_bp Anciennes proprietes du cadre du schema
	@param new_bp Nouvelles proprietes du cadre du schema
	@param parent QUndoCommand parent
*/
ChangeBorderCommand::ChangeBorderCommand(Diagram *dia, const BorderProperties &old_bp, const BorderProperties &new_bp, QUndoCommand *parent) :
	QUndoCommand(QObject::tr("modifier les dimensions du sch\351ma", "undo caption"), parent),
	diagram(dia),
	old_properties(old_bp),
	new_properties(new_bp)
{
}

/// Destructeur
ChangeBorderCommand::~ChangeBorderCommand() {
}

/// Annule les changements apportes au schema
void ChangeBorderCommand::undo() {
	diagram -> border_and_inset.importBorder(old_properties);
}

/// Refait les changements apportes au schema
void ChangeBorderCommand::redo() {
	diagram -> border_and_inset.importBorder(new_properties);
}

/**
	Constructeur
	@param c Le conducteur dont on modifie les proprietes
	@param parent QUndoCommand parent
*/
ChangeConductorPropertiesCommand::ChangeConductorPropertiesCommand(Conductor *c, QUndoCommand *parent) :
	QUndoCommand(QObject::tr("modifier les propri\351t\351s d'un conducteur", "undo caption"), parent),
	conductor(c),
	old_settings_set(false),
	new_settings_set(false)
{
}

/// Destructeur
ChangeConductorPropertiesCommand::~ChangeConductorPropertiesCommand() {
}

/// definit l'ancienne configuration
void ChangeConductorPropertiesCommand::setOldSettings(const ConductorProperties &properties) {
	old_properties = properties;
	old_settings_set = true;
}

/// definit la nouvelle configuration
void ChangeConductorPropertiesCommand::setNewSettings(const ConductorProperties &properties) {
	new_properties = properties;
	new_settings_set = true;
}

/**
	Annule les changements - Attention : les anciens et nouveaux parametres
	doivent avoir ete definis a l'aide de setNewSettings et setOldSettings
*/
void ChangeConductorPropertiesCommand::undo() {
	if (old_settings_set && new_settings_set) {
		conductor -> setProperties(old_properties);
		conductor -> update();
	}
}

/**
	Refait les changements - Attention : les anciens et nouveaux parametres
	doivent avoir ete definis a l'aide de setNewSettings et setOldSettings
*/
void ChangeConductorPropertiesCommand::redo() {
	if (old_settings_set && new_settings_set) {
		conductor -> setProperties(new_properties);
		conductor -> update();
	}
}
