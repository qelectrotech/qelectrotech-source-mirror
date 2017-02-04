/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "diagram.h"
#include "qetgraphicsitem/elementtextitem.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qgimanager.h"
#include "diagram.h"
#include "qetgraphicsitem/diagramtextitem.h"
#include "qetgraphicsitem/diagramimageitem.h"
#include <QPropertyAnimation>

QString itemText(const QetGraphicsItem *item) {
	return item->name();
}

QString itemText(const IndependentTextItem *item) {
	Q_UNUSED(item);
	return QObject::tr("un champ texte");
}

QString itemText(const Conductor *item) {
	Q_UNUSED(item);
	return QObject::tr("un conducteur");
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

/**
 * @brief DeleteElementsCommand::undo
 * Undo this command
 */
void DeleteElementsCommand::undo()
{
	diagram -> showMe();

	for (QGraphicsItem *item: removed_content.items())
		diagram->addItem(item);

		//We relink element after every element was added to diagram
	for (Element *e: removed_content.elements)
		for (Element *elmt: m_link_hash[e])
				e -> linkToElement(elmt);
}

/**
 * @brief DeleteElementsCommand::redo
 * Redo the delete command
 */
void DeleteElementsCommand::redo()
{
	diagram -> showMe();

	for (Conductor *c: removed_content.conductors(DiagramContent::AnyConductor))
	{
			//If option one text per folio is enable, and the text item of
			//current conductor is visible (that mean the conductor have the single displayed text)
			//We call adjustTextItemPosition to other conductor at the same potential to keep
			//a visible text on this potential.
		if (diagram -> defaultConductorProperties.m_one_text_per_folio && c -> textItem() -> isVisible())
		{
			QList <Conductor *> conductor_list;
			conductor_list << c -> relatedPotentialConductors(false).toList();
			if (conductor_list.count())
				conductor_list.first() -> calculateTextItemPosition();
		}
	}
	
	for (Element *e: removed_content.elements)
	{
			//Get linked element, for relink it at undo
		if (!e->linkedElements().isEmpty())
			m_link_hash.insert(e, e->linkedElements());
	}

	for (QGraphicsItem *item: removed_content.items())
		diagram->removeItem(item);
}

/**
 * @brief PasteDiagramCommand::PasteDiagramCommand
 * Constructor
 * @param dia : diagram where we must to paste
 * @param c : content to past
 * @param parent : parent undo command
 */
PasteDiagramCommand::PasteDiagramCommand( Diagram *dia, const DiagramContent &c, QUndoCommand *parent) :
	QUndoCommand(parent),
	content(c),
	diagram(dia),
	filter(DiagramContent::Elements|DiagramContent::TextFields|DiagramContent::Images|DiagramContent::ConductorsToMove | DiagramContent::Shapes),
	first_redo(true)
{
	setText(QObject::tr("coller %1", "undo caption - %1 is a sentence listing the content to paste").arg(content.sentence(filter)));
	diagram -> qgiManager().manage(content.items(filter));
}

/**
 * @brief PasteDiagramCommand::~PasteDiagramCommand
 * Destructor
 */
PasteDiagramCommand::~PasteDiagramCommand() {
	diagram -> qgiManager().release(content.items(filter));
}

/**
 * @brief PasteDiagramCommand::undo
 * Undo this command
 */
void PasteDiagramCommand::undo()
{
	diagram -> showMe();

	for (QGraphicsItem *item: content.items(filter))
		diagram->removeItem(item);
}

/**
 * @brief PasteDiagramCommand::redo
 * Redo this commnand
 */
void PasteDiagramCommand::redo()
{
	diagram -> showMe();
	QSettings settings;

	if (first_redo) {
		first_redo = false;

			//this is the first paste, we do some actions for the new element
		for (Element *e: content.elements) {
				//make new uuid, because old uuid are the uuid of the copied element
			e -> newUuid();

				//Reset the text of report element
			if (e -> linkType() & Element::AllReport) {

					//Befor commit 3559 there isn't text field tagged label,
					//so if not found we take the first text field
				if (ElementTextItem *eti = e->taggedText("label"))
					eti->setPlainText("/");
				else if (e->texts().size())
					e->texts().first()->setPlainText("/");

				if (ElementTextItem *eti = e->taggedText("function"))
					eti->setPlainText("_");
				if (ElementTextItem *eti = e->taggedText("tension-protocol"))
					eti->setPlainText("_");
			}
				if (settings.value("diagramcommands/save-label", true).toBool()){
				//Reset the information about the label, the comment and location
				e -> rElementInformations().addValue("label", "");
				e -> rElementInformations().addValue("comment", "");
				e -> rElementInformations().addValue("location", "");

				//Reset the text field tagged "label
				if (ElementTextItem *eti = e ->taggedText("label"))
				eti -> setPlainText("_");
			
				//Reset the text of conductors
				for (Conductor *c: content.conductorsToMove) {
					ConductorProperties cp = c -> properties();
					cp.text = c->diagram() ? c -> diagram() -> defaultConductorProperties.text : "_";
					c -> setProperties(cp);
			}
		}
}
	}
	else {
		for (QGraphicsItem *item: content.items(filter)) {
			diagram->item_paste = true;
			diagram->addItem(item);
			diagram->item_paste = false;
		}
	}

	for (QGraphicsItem *qgi: content.items())
		qgi -> setSelected(true);
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
 * @brief MoveElementsCommand::MoveElementsCommand
 * Constructor
 * @param dia diagram
 * @param diagram_content diagram content (contain all items to be moved)
 * @param m movement to applied
 * @param parent parent undo command
 */
MoveElementsCommand::MoveElementsCommand(
	Diagram *dia,
	const DiagramContent &diagram_content,
	const QPointF &m,
	QUndoCommand *parent
) :
	QUndoCommand    (parent),
	diagram         (dia),
	content_to_move (diagram_content),
	movement        (m),
	m_anim_group    (nullptr),
	first_redo      (true)
{
	QString moved_content_sentence = content_to_move.sentence(
		DiagramContent::Elements |
		DiagramContent::TextFields |
		DiagramContent::ConductorsToUpdate |
		DiagramContent::ConductorsToMove |
		DiagramContent::Images |
		DiagramContent::Shapes
	);
	
	setText(
		QString(
			QObject::tr(
				"déplacer %1",
				"undo caption - %1 is a sentence listing the moved content"
			).arg(moved_content_sentence)
		)
	);
}

/**
 * @brief MoveElementsCommand::~MoveElementsCommand
 * Destructor
 */
MoveElementsCommand::~MoveElementsCommand() {
	delete m_anim_group;
}

/**
 * @brief MoveElementsCommand::undo
 */
void MoveElementsCommand::undo() {
	diagram -> showMe();
	m_anim_group->setDirection(QAnimationGroup::Forward);
	m_anim_group->start();
	QUndoCommand::undo();
}

/**
 * @brief MoveElementsCommand::redo
 */
void MoveElementsCommand::redo() {
	diagram -> showMe();
	if (first_redo) {
		first_redo = false;
		move(-movement);
	}
	else {
		m_anim_group->setDirection(QAnimationGroup::Backward);
		m_anim_group->start();
	}
	QUndoCommand::redo();
}

/**
 * @brief MoveElementsCommand::move
 * Move item and conductor to @actual_movement
 * @param actual_movement movement to be applied
 */
void MoveElementsCommand::move(const QPointF &actual_movement) {
	typedef DiagramContent dc;

	//Move every movable item, except conductor
	for (QGraphicsItem *qgi: content_to_move.items(dc::Elements | dc::TextFields | dc::Images | dc::Shapes)) {
		//If curent item have parent, and parent item is in content_to_move
		//we don't apply movement to this item, because this item will be moved by is parent.
		if (qgi->parentItem()) {
			if (content_to_move.items().contains(qgi->parentItem()))
					continue;
		}
		if(qgi->toGraphicsObject()) {
			setupAnimation(qgi->toGraphicsObject(), "pos", qgi->pos(), qgi->pos() + actual_movement);
		}
		else qgi -> setPos(qgi->pos() + actual_movement);
	}
	
	// Move some conductors
	for (Conductor *conductor: content_to_move.conductorsToMove) {
		setupAnimation(conductor, "pos", conductor->pos(), conductor->pos() + actual_movement);
	}
	
	// Recalcul the path of other conductor
	for (Conductor *conductor: content_to_move.conductorsToUpdate) {
		setupAnimation(conductor, "animPath", 1, 1);
	}
}

/**
 * @brief MoveElementsCommand::setupAnimation
 * Set up the animation for this undo command
 * @param target object to anim
 * @param propertyName property to animate
 * @param start value at start
 * @param end value at end
 */
void MoveElementsCommand::setupAnimation(QObject *target, const QByteArray &propertyName, const QVariant start, const QVariant end) {
	//create animation group if not yet.
	if (m_anim_group == nullptr) m_anim_group = new QParallelAnimationGroup();
	QPropertyAnimation *animation = new QPropertyAnimation(target, propertyName);
	animation->setDuration(300);
	animation->setStartValue(start);
	animation->setEndValue(end);
	animation->setEasingCurve(QEasingCurve::OutQuad);
	m_anim_group->addAnimation(animation);
}

/**
	Constructeur
	@param diagram Schema sur lequel on deplace des champs de texte
	@param texts Textes deplaces : chaque ConductorTextItem est associe a un
	couple de position : avant et apres le deplacement
	@param m translation subie par les elements
	@param parent QUndoCommand parent
*/
MoveConductorsTextsCommand::MoveConductorsTextsCommand(
	Diagram *diagram,
	QUndoCommand *parent
) :
	QUndoCommand(parent),
	diagram(diagram),
	first_redo(true)
{
}

/// Destructeur
MoveConductorsTextsCommand::~MoveConductorsTextsCommand() {
}

/// annule le deplacement
void MoveConductorsTextsCommand::undo() {
	diagram -> showMe();
	for (ConductorTextItem *cti: texts_to_move_.keys()) {
		QPointF movement = texts_to_move_[cti].first;
		bool was_already_moved = texts_to_move_[cti].second;
		
		cti -> forceMovedByUser(was_already_moved);
		if (was_already_moved) {
			cti -> setPos(cti -> pos() - movement);
		}
	}
}

/// refait le deplacement
void MoveConductorsTextsCommand::redo() {
	diagram -> showMe();
	if (first_redo) {
		first_redo = false;
	} else {
		for (ConductorTextItem *cti: texts_to_move_.keys()) {
			QPointF movement = texts_to_move_[cti].first;
			
			cti -> forceMovedByUser(true);
			cti -> setPos(cti -> pos() + movement);
		}
	}
}

/**
	Ajout un mouvement de champ de texte a cet objet
	@param text_item Champ de texte deplace ; si celui-ci est deja connu de l'objet d'annulation, il sera ignore
	@param old_pos Position du champ de texte avant le mouvement
	@param new_pos Position du champ de texte apres le mouvement
	@param alread_moved true si le champ de texte etait deja a une position personnalisee par l'utilisateur, false sinon
*/
void MoveConductorsTextsCommand::addTextMovement(ConductorTextItem *text_item, const QPointF &old_pos, const QPointF &new_pos, bool already_moved) {
	// si le champ de texte est deja connu de l'objet d'annulation, il sera ignore
	if (texts_to_move_.contains(text_item)) return;
	
	// on memorise le champ de texte, en l'associant au mouvement effectue et a son etat avant le deplacement
	texts_to_move_.insert(text_item, qMakePair(new_pos - old_pos, already_moved));
	
	// met a jour la description de l'objet d'annulation
	regenerateTextLabel();
}

/**
	Genere la description de l'objet d'annulation
*/
void MoveConductorsTextsCommand::regenerateTextLabel() {
	QString moved_content_sentence = QET::ElementsAndConductorsSentence(0, 0, texts_to_move_.count());
	
	setText(
		QString(
			QObject::tr(
				"déplacer %1",
				"undo caption - %1 is a sentence listing the moved content"
			).arg(moved_content_sentence)
		)
	);
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
	first_redo(true),
	diagram(dti->diagram())
{
}

/// destructeur
ChangeDiagramTextCommand::~ChangeDiagramTextCommand() {
}

/// annule la modification de texte
void ChangeDiagramTextCommand::undo() {
	diagram -> showMe();
	text_item -> setHtml(text_before);
}

/// refait la modification de texte
void ChangeDiagramTextCommand::redo() {
	diagram -> showMe();
	if (first_redo) {
		first_redo = false;
	} else {
		text_item -> setHtml(text_after);
	}
}

/**
	Constructeur
	@param elements Elements a pivoter associes a leur orientation d'origine
	@param texts Textes a pivoter
	@param parent QUndoCommand parent
*/
RotateElementsCommand::RotateElementsCommand(const QList<Element *> &elements, const QList<DiagramTextItem *> &texts, const QList<DiagramImageItem *> &images, QUndoCommand *parent) :
	QUndoCommand(parent),
	elements_to_rotate(elements),
	texts_to_rotate(texts),
	images_to_rotate(images),
	applied_rotation_angle_(90.0)
{
	if(elements_to_rotate.size()) diagram = elements_to_rotate.first()->diagram();
	else if (texts_to_rotate.size()) diagram = texts_to_rotate.first()->diagram();
	else if (images_to_rotate.size()) diagram = images_to_rotate.first()->diagram();

	setText(
		QString(
			QObject::tr(
				"pivoter %1",
				"undo caption - %1 is a sentence listing the rotated content"
			)
		).arg(QET::ElementsAndConductorsSentence(elements.count(), 0, texts.count(), images.count()))
	);
}

/// Destructeur
RotateElementsCommand::~RotateElementsCommand() {
}

/// defait le pivotement
void RotateElementsCommand::undo() {
	diagram -> showMe();
	for (Element *e: elements_to_rotate) {
		e -> rotateBy(-applied_rotation_angle_);
	}
	for (DiagramTextItem *dti: texts_to_rotate) {
		//ConductorTextItem have a default rotation angle, we apply a specific treatment
		if (ConductorTextItem *cti = qgraphicsitem_cast<ConductorTextItem *>(dti)) {
			cti -> forceRotateByUser(previous_rotate_by_user_[cti]);
			(cti -> wasRotateByUser()) ? cti -> rotateBy(-applied_rotation_angle_) :
										 cti -> parentConductor() -> calculateTextItemPosition();
		}
		else {dti -> rotateBy(-applied_rotation_angle_);}
	}
	for (DiagramImageItem *dii: images_to_rotate) dii -> rotateBy(-applied_rotation_angle_);
}

/// refait le pivotement
void RotateElementsCommand::redo() {
	diagram -> showMe();
	for (Element *e: elements_to_rotate) {
		e -> rotateBy(applied_rotation_angle_);
	}
	for (DiagramTextItem *dti: texts_to_rotate) {
		//we grab the previous rotation by user of each ConductorTextItem
		if (ConductorTextItem *cti = qgraphicsitem_cast<ConductorTextItem *>(dti)) {
			previous_rotate_by_user_.insert(cti, cti -> wasRotateByUser());
			cti -> forceRotateByUser(true);
		}
		dti -> rotateBy(applied_rotation_angle_);
	}
	for (DiagramImageItem *dii: images_to_rotate) dii -> rotateBy(applied_rotation_angle_);
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
	applied_rotation_angle_(applied_rotation),
	diagram(previous_state.key(0)->diagram())
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
	applied_rotation_angle_(applied_rotation),
	diagram(texts.first()->diagram())
{
	for (DiagramTextItem *text: texts) {
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
	diagram -> showMe();
	for (DiagramTextItem *text: texts_to_rotate.keys()) {
		if (ConductorTextItem *cti = qgraphicsitem_cast<ConductorTextItem *>(text))
			cti -> forceRotateByUser(previous_rotate_by_user_[cti]);
		text -> setRotationAngle(texts_to_rotate[text]);
	}
}

/**
	Applique l'angle de rotation aux textes
*/
void RotateTextsCommand::redo() {
	diagram -> showMe();
	for (DiagramTextItem *text: texts_to_rotate.keys()) {
		if (ConductorTextItem *cti = qgraphicsitem_cast<ConductorTextItem *>(text)) {
			//we grab the previous rotation by user of each ConductorTextItem
			previous_rotate_by_user_.insert(cti, cti -> wasRotateByUser());
			cti -> forceRotateByUser(true);
		}
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
				"orienter %1 à %2°",
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
	first_redo(true),
	diagram (c->diagram())
{
}

/// Destructeur
ChangeConductorCommand::~ChangeConductorCommand() {
}

/// Annule la modification du conducteur
void ChangeConductorCommand::undo() {
	diagram -> showMe();
	conductor -> setProfile(old_profile, path_type);
	conductor -> textItem() -> setPos(text_pos_before_mov_);
}

/// Refait la modification du conducteur
void ChangeConductorCommand::redo() {
	diagram -> showMe();
	if (first_redo) {
		first_redo = false;
	} else {
		conductor -> setProfile(new_profile, path_type);
		conductor -> textItem() -> setPos(text_pos_after_mov_);
	}
}

/**
	Integre dans cet objet d'annulation le repositionnement du champ de texte
	du conducteur
	@param pos_before Position du texte avant la modification du conducteur
	@param pos_after  Position du texte apres la modification du conducteur
*/
void ChangeConductorCommand::setConductorTextItemMove(const QPointF &pos_before, const QPointF &pos_after) {
	text_pos_before_mov_ = pos_before;
	text_pos_after_mov_  = pos_after;
}

/**
 * @brief ResetConductorCommand::ResetConductorCommand
 * @param cp
 * @param parent
 */
ResetConductorCommand::ResetConductorCommand(
	const QHash<Conductor *, ConductorProfilesGroup> &cp,
	QUndoCommand *parent
) :
	QUndoCommand(parent),
	conductors_profiles(cp),
	diagram(cp.keys().first()->diagram())
{
	setText(
		QObject::tr(
			"Réinitialiser %1",
			"undo caption - %1 is a sentence listing the reset content"
		).arg(QET::ElementsAndConductorsSentence(0, cp.count()))
	);
}

/**
 * @brief ResetConductorCommand::~ResetConductorCommand
 */
ResetConductorCommand::~ResetConductorCommand() {
}

/**
 * @brief ResetConductorCommand::undo
 */
void ResetConductorCommand::undo() {
	diagram -> showMe();
	for (Conductor *c: conductors_profiles.keys()) {
		c -> setProfiles(conductors_profiles[c]);
	}
}

/**
 * @brief ResetConductorCommand::redo
 */
void ResetConductorCommand::redo() {
	diagram -> showMe();
	for (Conductor *c: conductors_profiles.keys()) {
		c -> textItem() -> forceMovedByUser  (false);
		c -> textItem() -> forceRotateByUser (false);
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
ChangeTitleBlockCommand::ChangeTitleBlockCommand(
	Diagram *d,
	const TitleBlockProperties &old_ip,
	const TitleBlockProperties &new_ip,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("modifier le cartouche", "undo caption"), parent),
	diagram(d),
	old_titleblock(old_ip),
	new_titleblock(new_ip)
{
}

/// Destructeur
ChangeTitleBlockCommand::~ChangeTitleBlockCommand() {
}

/// Annule la modification de cartouche
void ChangeTitleBlockCommand::undo() {
	diagram -> showMe();
	diagram -> border_and_titleblock.importTitleBlock(old_titleblock);
	diagram -> invalidate(diagram -> border_and_titleblock.borderAndTitleBlockRect());
}

/// Refait la modification de cartouche
void ChangeTitleBlockCommand::redo() {
	diagram -> showMe();
	diagram -> border_and_titleblock.importTitleBlock(new_titleblock);
	diagram -> invalidate(diagram -> border_and_titleblock.borderAndTitleBlockRect());
}

/**
	Constructeur
	@param dia Schema modifie
	@param old_bp Anciennes proprietes du cadre du schema
	@param new_bp Nouvelles proprietes du cadre du schema
	@param parent QUndoCommand parent
*/
ChangeBorderCommand::ChangeBorderCommand(Diagram *dia, const BorderProperties &old_bp, const BorderProperties &new_bp, QUndoCommand *parent) :
    QUndoCommand(QObject::tr("modifier les dimensions du folio", "undo caption"), parent),
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
	diagram -> showMe();
	diagram -> border_and_titleblock.importBorder(old_properties);
}

/// Refait les changements apportes au schema
void ChangeBorderCommand::redo() {
	diagram -> showMe();
	diagram -> border_and_titleblock.importBorder(new_properties);
}
