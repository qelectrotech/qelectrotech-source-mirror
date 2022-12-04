/*
	Copyright 2006-2021 The QElectroTech Team
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

#include "diagram.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/elementtextitemgroup.h"
#include "qgimanager.h"

/**
	@brief PasteDiagramCommand::PasteDiagramCommand
	Constructor
	@param dia : diagram where we must paste
	@param c : content to paste
	@param parent : parent undo command
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
	@brief PasteDiagramCommand::~PasteDiagramCommand
	Destructor
*/
PasteDiagramCommand::~PasteDiagramCommand()
{
	diagram -> qgiManager().release(content.items(filter));
}

/**
	@brief PasteDiagramCommand::undo
	Undo this command
*/
void PasteDiagramCommand::undo()
{
	diagram -> showMe();

	foreach(QGraphicsItem *item, content.items(filter))
		diagram->removeItem(item);
}

/**
	@brief PasteDiagramCommand::redo
	Redo this command
*/
void PasteDiagramCommand::redo()
{
	diagram -> showMe();
	QSettings settings;

	if (first_redo)
	{
		first_redo = false;

		//this is the first paste, we do some actions for the new element
		const QList <Element *> elmts_list = content.m_elements;
		for (Element *e : elmts_list)
		{
			//make new uuid, because old uuid are the uuid of the copied element
			e -> newUuid();

			if (settings.value("diagramcommands/erase-label-on-copy", true).toBool())
			{
				//Reset the information about the label, the comment and location
				DiagramContext dc = e->elementInformations();
				dc.addValue("formula", "");
				dc.addValue("label", "");
				dc.addValue("comment", "");
				dc.addValue("location", "");
				e->setElementInformations(dc);
				
				//Reset the text of conductors
				const QList <Conductor *> conductors_list = content.m_conductors_to_move;
				for (Conductor *c : conductors_list)
				{
					ConductorProperties cp = c -> properties();
					cp.text = c->diagram() ? c -> diagram() -> defaultConductorProperties.text : "_";
					c -> setProperties(cp);
				}
			}
		}
	}
	else
	{
		const QList <QGraphicsItem *> qgis_list = content.items(filter);
		for (QGraphicsItem *item : qgis_list) {
			diagram->addItem(item);
		}
	}

	const QList <QGraphicsItem *> qgis_list = content.items();
	for (QGraphicsItem *qgi : qgis_list)
		qgi -> setSelected(true);
}

/**
	@brief CutDiagramCommand::CutDiagramCommand
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
	DeleteQGraphicsItemCommand(dia, content, parent)
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

/**
	@brief CutDiagramCommand::~CutDiagramCommand
	Destructeur
*/
CutDiagramCommand::~CutDiagramCommand()
{
}

/**
	@brief MoveElementsCommand::MoveElementsCommand
	Constructor
	@param dia diagram
	@param diagram_content diagram content (contain all items to be moved)
	@param m movement to applied
	@param parent parent undo command
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
		DiagramContent::Shapes |
		DiagramContent::ElementTextFields
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
	@brief MoveElementsCommand::~MoveElementsCommand
	Destructor
*/
MoveElementsCommand::~MoveElementsCommand()
{
	delete m_anim_group;
}

/**
	@brief MoveElementsCommand::undo
*/
void MoveElementsCommand::undo()
{
	diagram -> showMe();
	m_anim_group->setDirection(QAnimationGroup::Forward);
	m_anim_group->start();
	QUndoCommand::undo();
}

/**
	@brief MoveElementsCommand::redo
*/
void MoveElementsCommand::redo()
{
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
	@brief MoveElementsCommand::move
	Move item and conductor to actual_movement
	@param actual_movement movement to be applied
*/
void MoveElementsCommand::move(const QPointF &actual_movement)
{
	typedef DiagramContent dc;

	//Move every movable items, except conductor
	for (QGraphicsItem *qgi : content_to_move.items(dc::Elements
							| dc::TextFields
							| dc::Images
							| dc::Shapes
							| dc::TextGroup
							| dc::ElementTextFields
							| dc::Tables))
	{
		//If current item has parent, and parent item is in content_to_move
		//we don't apply movement to this item, because this item will be moved by is parent.
		if (qgi->parentItem())
			if (content_to_move.items().contains(qgi->parentItem()))
					continue;
		
		if(qgi->toGraphicsObject())
			setupAnimation(qgi->toGraphicsObject(), "pos",
				       qgi->pos(), qgi->pos() + actual_movement);
		else if(qgi->type() == QGraphicsItemGroup::Type)
		{
			//ElementTextItemGroup is a QObject but not a QGraphicsObject
			if(ElementTextItemGroup *etig = dynamic_cast<ElementTextItemGroup *>(qgi))
				setupAnimation(etig, "pos", etig->pos(),
					       etig->pos() + actual_movement);
		}
		else qgi -> setPos(qgi->pos() + actual_movement);
	}
	
	// Move some conductors
	for (Conductor *conductor : content_to_move.m_conductors_to_move)
		setupAnimation(conductor, "pos", conductor->pos(),
			       conductor->pos() + actual_movement);
	
	// Recalcul the path of other conductor
	for (Conductor *conductor : content_to_move.m_conductors_to_update)
		setupAnimation(conductor, "animPath", 1, 1);
}

/**
	@brief MoveElementsCommand::setupAnimation
	Set up the animation for this undo command
	@param target object to anim
	@param propertyName property to animate
	@param start value at start
	@param end value at end
*/
void MoveElementsCommand::setupAnimation(QObject *target,
					 const QByteArray &propertyName,
					 const QVariant& start,
					 const QVariant& end) {
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
	@brief MoveConductorsTextsCommand::MoveConductorsTextsCommand
	Constructeur
	@param diagram Schema sur lequel on deplace des champs de texte
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

/**
	@brief MoveConductorsTextsCommand::~MoveConductorsTextsCommand
	Destructeur
*/
MoveConductorsTextsCommand::~MoveConductorsTextsCommand()
{
}

/**
	@brief MoveConductorsTextsCommand::undo
	annule le deplacement
*/
void MoveConductorsTextsCommand::undo()
{
	diagram -> showMe();
	foreach(ConductorTextItem *cti, texts_to_move_.keys()) {
		QPointF movement = texts_to_move_[cti].first;
		bool was_already_moved = texts_to_move_[cti].second;
		
		cti -> forceMovedByUser(was_already_moved);
		if (was_already_moved) {
			cti -> setPos(cti -> pos() - movement);
		}
	}
}

/**
	@brief MoveConductorsTextsCommand::redo
	refait le deplacement
*/
void MoveConductorsTextsCommand::redo()
{
	diagram -> showMe();
	if (first_redo) {
		first_redo = false;
	} else {
		foreach(ConductorTextItem *cti, texts_to_move_.keys()) {
			QPointF movement = texts_to_move_[cti].first;
			
			cti -> forceMovedByUser(true);
			cti -> setPos(cti -> pos() + movement);
		}
	}
}

/**
	@brief MoveConductorsTextsCommand::addTextMovement
	Ajout un mouvement de champ de texte a cet objet
	@param text_item Champ de texte deplace ;
	si celui-ci est deja connu de l'objet d'annulation,
	il sera ignore
	@param old_pos Position du champ de texte avant le mouvement
	@param new_pos Position du champ de texte apres le mouvement
	@param already_moved true si le champ de texte etait deja a une position
	personnalisee par l'utilisateur, false sinon
*/
void MoveConductorsTextsCommand::addTextMovement(ConductorTextItem *text_item,
						 const QPointF &old_pos,
						 const QPointF &new_pos,
						 bool already_moved) {
	// si le champ de texte est deja connu de l'objet d'annulation, il sera ignore
	if (texts_to_move_.contains(text_item)) return;
	
	// on memorise le champ de texte,
	//en l'associant au mouvement effectue et a son etat avant le deplacement
	texts_to_move_.insert(text_item, qMakePair(new_pos - old_pos, already_moved));
	
	// met a jour la description de l'objet d'annulation
	regenerateTextLabel();
}

/**
	@brief MoveConductorsTextsCommand::regenerateTextLabel
	Genere la description de l'objet d'annulation
*/
void MoveConductorsTextsCommand::regenerateTextLabel()
{
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
	@brief ChangeDiagramTextCommand::ChangeDiagramTextCommand
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

/**
	@brief ChangeDiagramTextCommand::~ChangeDiagramTextCommand
	destructeur
*/
ChangeDiagramTextCommand::~ChangeDiagramTextCommand()
{
}

/**
	@brief ChangeDiagramTextCommand::undo
	annule la modification de texte
*/
void ChangeDiagramTextCommand::undo()
{
	diagram -> showMe();
	text_item -> setHtml(text_before);
}

/**
	@brief ChangeDiagramTextCommand::redo
*/
void ChangeDiagramTextCommand::redo()
{
	diagram -> showMe();
	text_item->setHtml(text_after);
}

/**
	@brief ChangeConductorCommand::ChangeConductorCommand
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

/**
	@brief ChangeConductorCommand::~ChangeConductorCommand
	Destructeur
*/
ChangeConductorCommand::~ChangeConductorCommand()
{
}

/**
	@brief ChangeConductorCommand::undo
	Annule la modification du conducteur
*/
void ChangeConductorCommand::undo()
{
	diagram -> showMe();
	conductor -> setProfile(old_profile, path_type);
	conductor -> textItem() -> setPos(text_pos_before_mov_);
}

/**
	@brief ChangeConductorCommand::redo
	Refait la modification du conducteur
*/
void ChangeConductorCommand::redo()
{
	diagram -> showMe();
	if (first_redo) {
		first_redo = false;
	} else {
		conductor -> setProfile(new_profile, path_type);
		conductor -> textItem() -> setPos(text_pos_after_mov_);
	}
}

/**
	@brief ChangeConductorCommand::setConductorTextItemMove
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
	@brief ResetConductorCommand::ResetConductorCommand
	@param cp
	@param parent
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
	@brief ResetConductorCommand::~ResetConductorCommand
*/
ResetConductorCommand::~ResetConductorCommand()
{
}

/**
	@brief ResetConductorCommand::undo
*/
void ResetConductorCommand::undo()
{
	diagram -> showMe();
	foreach(Conductor *c, conductors_profiles.keys()) {
		c -> setProfiles(conductors_profiles[c]);
	}
}

/**
	@brief ResetConductorCommand::redo
*/
void ResetConductorCommand::redo()
{
	diagram -> showMe();
	foreach(Conductor *c, conductors_profiles.keys()) {
		c -> textItem() -> forceMovedByUser  (false);
		c -> textItem() -> forceRotateByUser (false);
		c -> setProfiles(ConductorProfilesGroup());
	}
}

/**
	@brief ChangeBorderCommand::ChangeBorderCommand
	Constructeur
	@param dia Schema modifie
	@param old_bp Anciennes proprietes du cadre du schema
	@param new_bp Nouvelles proprietes du cadre du schema
	@param parent QUndoCommand parent
*/
ChangeBorderCommand::ChangeBorderCommand(Diagram *dia,
					 const BorderProperties &old_bp,
					 const BorderProperties &new_bp,
					 QUndoCommand *parent) :
	QUndoCommand(
		QObject::tr("modifier les dimensions du folio", "undo caption"),
		parent),
	diagram(dia),
	old_properties(old_bp),
	new_properties(new_bp)
{
}

/**
	@brief ChangeBorderCommand::~ChangeBorderCommand
	Destructeur
*/
ChangeBorderCommand::~ChangeBorderCommand()
{
}

/**
	@brief ChangeBorderCommand::undo
	Annule les changements apportes au schema
*/
void ChangeBorderCommand::undo()
{
	diagram -> showMe();
	diagram -> border_and_titleblock.importBorder(old_properties);
}

/**
	@brief ChangeBorderCommand::redo
	Refait les changements apportes au schema
*/
void ChangeBorderCommand::redo()
{
	diagram -> showMe();
	diagram -> border_and_titleblock.importBorder(new_properties);
}
