/*
	Copyright 2006-2026 The QElectroTech Team
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

/**
	@brief ElementEditionCommand::ElementEditionCommand
	Constructs an ElementEditionCommand,
	thus embedding the provided \a scene and \a view.
	@param scene
	@param view
	@param parent : Parent command
*/
ElementEditionCommand::ElementEditionCommand(ElementScene *scene,
					     ElementView *view,
					     QUndoCommand *parent):
	QUndoCommand(parent),
	m_scene(scene),
	m_view(view)
{
}

/**
	@brief ElementEditionCommand::ElementEditionCommand
	Constructs an ElementEditionCommand,
	thus embedding the provided \a scene and \a view.
	@param text : Text describing the effect of the command
	@param scene
	@param view
	@param parent : Parent command
*/
ElementEditionCommand::ElementEditionCommand(const QString &text,
					     ElementScene *scene,
					     ElementView *view,
					     QUndoCommand *parent):
	QUndoCommand(text, parent),
	m_scene(scene),
	m_view(view)
{
}

/**
	Destructor
*/
ElementEditionCommand::~ElementEditionCommand()
{
}

/**
	@return the element editor/scene the command should take place on
*/
ElementScene *ElementEditionCommand::elementScene() const
{
	return(m_scene);
}

/**
	Define \a scene as the element editor/scene the command should take place
*/
void ElementEditionCommand::setElementScene(ElementScene *scene) {
	m_scene = scene;
}

/**
	@return the view the effect of the command should be rendered on
*/
ElementView *ElementEditionCommand::elementView() const
{
	return(m_view);
}

/**
	Define \a view as the view the effect of the command should be rendered on
*/
void ElementEditionCommand::setElementView(ElementView *view) {
	m_view = view;
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
	const QList<QGraphicsItem *>& parts,
	QUndoCommand *parent
) :
	DeletePartsCommand(scene, parts.toVector(), parent)
{
	setText(QString(QObject::tr("couper des parties", "undo caption")));
}

/// Destructeur
CutPartsCommand::~CutPartsCommand()
{
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
	const QList<QGraphicsItem *>& parts,
	QUndoCommand *parent
) :
	ElementEditionCommand(QObject::tr("déplacement", "undo caption"), scene, nullptr, parent),
	movement(m),
	first_redo(true)
{
	moved_parts = parts;
}

/// Destructeur
MovePartsCommand::~MovePartsCommand()
{
}

/// Annule le deplacement
void MovePartsCommand::undo()
{
	foreach(QGraphicsItem *qgi, moved_parts) qgi -> moveBy(-movement.x(), -movement.y());
}

/// Refait le deplacement
void MovePartsCommand::redo()
{
	// le premier appel a redo, lors de la construction de l'objet, ne doit pas se faire
	if (first_redo) {
		first_redo = false;
		return;
	}
	foreach(QGraphicsItem *qgi, moved_parts) qgi -> moveBy(movement.x(), movement.y());
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
	ElementEditionCommand(QObject::tr("modification noms", "undo caption"), element_scene, nullptr, parent),
	names_before(before),
	names_after(after)
{
}

/// Destructeur
ChangeNamesCommand::~ChangeNamesCommand()
{
}

/// Annule le changement
void ChangeNamesCommand::undo()
{
	auto data = m_scene->elementData();
	data.m_names_list = names_before;
	m_scene->setElementData(data);
}

/// Refait le changement
void ChangeNamesCommand::redo()
{
	auto data = m_scene->elementData();
	data.m_names_list = names_after;
	m_scene->setElementData(data);
}

/**
	Constructeur
	@param elmt ElementScene concernee
	@param o Option decrivant le type de traitement applique aux zValues des parties de l'element
	@param parent QUndoCommand parent
*/
ChangeZValueCommand::ChangeZValueCommand(
	ElementScene *elmt,
	QET::DepthOption o,
	QUndoCommand *parent
) :
	ElementEditionCommand(elmt, nullptr, parent),
	m_option(o)
{
	// retrieve all primitives but terminals
	QList<QGraphicsItem *> items_list = m_scene -> zItems(ElementScene::SortByZValue | ElementScene::SelectedOrNot);

	// prend un snapshot des zValues
	foreach(QGraphicsItem *qgi, items_list) undo_hash.insert(qgi, qgi -> zValue());

	// choisit le nom en fonction du traitement
	if (m_option == QET::BringForward) {
		setText(QObject::tr("amener au premier plan", "undo caption"));
		applyBringForward(items_list);
	} else if (m_option == QET::Raise) {
		setText(QObject::tr("rapprocher", "undo caption"));
		applyRaise(items_list);
	} else if (m_option == QET::Lower) {
		setText(QObject::tr("éloigner", "undo caption"));
		applyLower(items_list);
	} else if (m_option == QET::SendBackward) {
		setText(QObject::tr("envoyer au fond", "undo caption"));
		applySendBackward(items_list);
	}
}

/// Destructeur
ChangeZValueCommand::~ChangeZValueCommand()
{
}

/// Annule les changements de zValue
void ChangeZValueCommand::undo()
{
	foreach(QGraphicsItem *qgi, undo_hash.keys()) qgi -> setZValue(undo_hash[qgi]);
}

/// Refait les changements de zValue
void ChangeZValueCommand::redo()
{
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
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)	// ### Qt 6: remove
				my_items_list.swap(i, i + 1);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 5.13 or later")
#endif
				my_items_list.swapItemsAt(i, i + 1);
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)	// ### Qt 6: remove
				my_items_list.swap(i, i - 1);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 5.13 or later")
#endif
				my_items_list.swapItemsAt(i, i - 1);
#endif
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
	@param old_infos Informations complementaires precedentes
	@param new_infos Nouvelles informations complementaires
	@param parent QUndoCommand parent
*/
ChangeInformationsCommand::ChangeInformationsCommand(ElementScene *elmt, const QString &old_infos, const QString &new_infos, QUndoCommand *parent) :
	ElementEditionCommand(QObject::tr("modification informations complementaires", "undo caption"), elmt, nullptr, parent),
	old_informations_(old_infos),
	new_informations_(new_infos)
{
}

/// Destructeur
ChangeInformationsCommand::~ChangeInformationsCommand()
{
}

/// Annule le changement d'autorisation pour les connexions internes
void ChangeInformationsCommand::undo()
{
	auto data = m_scene->elementData();
	data.m_drawing_information = old_informations_;
	m_scene->setElementData(data);
}

/// Refait le changement d'autorisation pour les connexions internes
void ChangeInformationsCommand::redo()
{
	auto data = m_scene->elementData();
	data.m_drawing_information = new_informations_;
	m_scene->setElementData(data);
}

/**
	Constructor
	@param scene Modified ElementScene
	@param parent Parent QUndoCommand
*/
ScalePartsCommand::ScalePartsCommand(ElementScene *scene, QUndoCommand * parent) :
	ElementEditionCommand(scene, nullptr, parent),
	first_redo(true)
{}

/**
	Destructor
*/
ScalePartsCommand::~ScalePartsCommand()
{
}

/**
	Undo the scaling operation
*/
void ScalePartsCommand::undo()
{
	scale(new_rect_, original_rect_);
}

/**
	Redo the scaling operation
*/
void ScalePartsCommand::redo()
{
	if (first_redo) {
		first_redo = false;
		return;
	}
	scale(original_rect_, new_rect_);
}

/**
	@return the element editor/scene the command should take place on
*/
ElementScene *ScalePartsCommand::elementScene() const
{
	return(m_scene);
}

/**
	Set \a primitives as the list of primitives to be scaled by this command
*/
void ScalePartsCommand::setScaledPrimitives(const QList<CustomElementPart *> &primitives) {
	scaled_primitives_ = primitives;
	adjustText();
}

/**
	@return the list of primitives to be scaled by this command
*/
QList<CustomElementPart *> ScalePartsCommand::scaledPrimitives() const
{
	return(scaled_primitives_);
}

/**
	@brief ScalePartsCommand::setTransformation
	Define the transformation applied by this command
	@param original_rect :
	Bounding rectangle for all scaled primitives before the operation
	@param new_rect :
	Bounding rectangle for all scaled primitives after the operation
 */
void ScalePartsCommand::setTransformation(const QRectF &original_rect,
					  const QRectF &new_rect) {
	original_rect_ = original_rect;
	new_rect_ = new_rect;
}

/**
	@return the transformation applied by this command. The returned rectangles
	are the bounding rectangles for all scaled primitives respectively before
	and after the operation.
*/
QPair<QRectF, QRectF> ScalePartsCommand::transformation()
{
	return(QPair<QRectF, QRectF>(original_rect_, new_rect_));
}

/**
	Apply the scaling operation from \a before to \a after.
*/
void ScalePartsCommand::scale(const QRectF &before, const QRectF &after) {
	if (!scaled_primitives_.count()) return;
	if (before == after) return;
	if (!before.width() || !before.height()) return; // cowardly flee division by zero FIXME?

	foreach (CustomElementPart *part_item, scaled_primitives_) {
		part_item -> startUserTransformation(before);
		part_item -> handleUserTransformation(before, after);
	}
}

/**
	Generate the text describing what this command does exactly.
*/
void ScalePartsCommand::adjustText()
{
	if (scaled_primitives_.count() == 1) {
		setText(QObject::tr("redimensionnement %1", "undo caption -- %1 is the resized primitive type name").arg(scaled_primitives_.first() -> name()));
	} else {
		setText(QObject::tr("redimensionnement de %1 primitives", "undo caption -- %1 always > 1").arg(scaled_primitives_.count()));
	}
}

/**
 * @brief changeElementDataCommand::changeElementDataCommand
 * Change the properties of the drawn element
 * @param scene : scene belonging to the property
 * @param old_data : old data
 * @param new_data : new data
 * @param parent : parent undo command
 */
changeElementDataCommand::changeElementDataCommand(ElementScene *scene,
												   ElementData old_data,
												   ElementData new_data,
												   QUndoCommand *parent) :
	ElementEditionCommand(scene, nullptr, parent),
	m_old(old_data),
	m_new(new_data)
{
	setText(QObject::tr("Modifier les propriétées de l'élément"));
}

void changeElementDataCommand::undo() {
	m_scene->setElementData(m_old);
	QUndoCommand::undo();
}

void changeElementDataCommand::redo() {
	m_scene->setElementData(m_new);
	QUndoCommand::redo();
}

RotateElementsCommand::RotateElementsCommand(ElementScene *scene, QUndoCommand *parent) :
ElementEditionCommand(QObject::tr("Pivoter la selection", "undo caption"), scene, nullptr, parent)
{
	m_items = scene->selectedItems();
}

/**
	@brief RotateElementsCommand::undo
*/
void RotateElementsCommand::undo()
{
	for (QGraphicsItem *item : m_items)
	{
		if (item->type() == PartTerminal::Type) {
			PartTerminal* term = qgraphicsitem_cast<PartTerminal*>(item);
                    if(m_items.size() == 1) {
                            term->previousOrientation();
                    } else {
			term->setRotation(term->rotation()-90);
                    }
		}
		else if (item->type() == PartRectangle::Type) {
			PartRectangle* rect = qgraphicsitem_cast<PartRectangle*>(item);
			rect->setRotation(rect->rotation()-90);
		}
		else if (item->type() == PartArc::Type) {
			PartArc* arc = qgraphicsitem_cast<PartArc*>(item);
			arc->setRotation(arc->rotation()-90);
		}
		else if (item->type() == PartEllipse::Type) {
			PartEllipse* ellipse = qgraphicsitem_cast<PartEllipse*>(item);
			ellipse->setRotation(ellipse->rotation()-90);
		}
		else if (item->type() == PartLine::Type) {
			PartLine* line = qgraphicsitem_cast<PartLine*>(item);
			line->setRotation(line->rotation()-90);
		}
		else if (item->type() == PartPolygon::Type) {
			PartPolygon* poly = qgraphicsitem_cast<PartPolygon*>(item);
			poly->setRotation(poly->rotation()-90);
		}
		else if (item->type() == PartText::Type) {
			PartText* text = qgraphicsitem_cast<PartText*>(item);
			text->setRotation(text->rotation()-90);
		}
		else if (item->type() == PartDynamicTextField::Type) {
			PartDynamicTextField* dyntext = qgraphicsitem_cast<PartDynamicTextField*>(item);
			dyntext->setRotation(dyntext->rotation()-90);
		}
		else {
			item->setRotation(item->rotation()-90);
		}
	}
}

/**
	@brief RotateElementsCommand::redo
*/
void RotateElementsCommand::redo()
{
	for (QGraphicsItem *item : m_items)
	{
		if (item->type() == PartTerminal::Type) {
			PartTerminal* term = qgraphicsitem_cast<PartTerminal*>(item);
                    if (m_items.size() == 1) {
                            term->nextOrientation();
                    } else {
			term->setRotation(term->rotation()+90);
                    }
		}
		else if (item->type() == PartRectangle::Type) {
			PartRectangle* rect = qgraphicsitem_cast<PartRectangle*>(item);
			rect->setRotation(rect->rotation()+90);
		}
		else if (item->type() == PartArc::Type) {
			PartArc* arc = qgraphicsitem_cast<PartArc*>(item);
			arc->setRotation(arc->rotation()+90);
		}
		else if (item->type() == PartEllipse::Type) {
			PartEllipse* ellipse = qgraphicsitem_cast<PartEllipse*>(item);
			ellipse->setRotation(ellipse->rotation()+90);
		}
		else if (item->type() == PartLine::Type) {
			PartLine* line = qgraphicsitem_cast<PartLine*>(item);
			line->setRotation(line->rotation()+90);
		}
		else if (item->type() == PartPolygon::Type) {
			PartPolygon* poly = qgraphicsitem_cast<PartPolygon*>(item);
			poly->setRotation(poly->rotation()+90);
		}
		else if (item->type() == PartText::Type) {
			PartText* text = qgraphicsitem_cast<PartText*>(item);
			text->setRotation(text->rotation()+90);
		}
		else if (item->type() == PartDynamicTextField::Type) {
			PartDynamicTextField* dyntext = qgraphicsitem_cast<PartDynamicTextField*>(item);
			dyntext->setRotation(dyntext->rotation()+90);
		}
		else {
			item->setRotation(item->rotation()+90);
		}
	}
}


RotateFineElementsCommand::RotateFineElementsCommand(ElementScene *scene, QUndoCommand *parent) :
ElementEditionCommand(QObject::tr("Pivoter la selection", "undo caption"), scene, nullptr, parent)
{
	m_items = scene->selectedItems();
}

/**
	@brief RotateFineElementsCommand::undo
*/
void RotateFineElementsCommand::undo()
{
	for (QGraphicsItem *item : m_items)
	{
		if (item->type() == PartLine::Type) {
			PartLine* line = qgraphicsitem_cast<PartLine*>(item);
			line->setRotation(line->rotation()-5);
		}
		else if (item->type() == PartPolygon::Type) {
			PartPolygon* poly = qgraphicsitem_cast<PartPolygon*>(item);
			poly->setRotation(poly->rotation()-5);
		}
		else if (item->type() == PartText::Type) {
			PartText* text = qgraphicsitem_cast<PartText*>(item);
			text->setRotation(text->rotation()-5);
		}
		else if (item->type() == PartDynamicTextField::Type) {
			PartDynamicTextField* dyntext = qgraphicsitem_cast<PartDynamicTextField*>(item);
			dyntext->setRotation(dyntext->rotation()-5);
		}
		else {
			//item->setRotation(item->rotation()-5);
		}
	}
}

/**
	@brief RotateFineElementsCommand::redo
*/
void RotateFineElementsCommand::redo()
{
	for (QGraphicsItem *item : m_items)
	{
		if (item->type() == PartLine::Type) {
			PartLine* line = qgraphicsitem_cast<PartLine*>(item);
			line->setRotation(line->rotation()+5);
		}
		else if (item->type() == PartPolygon::Type) {
			PartPolygon* poly = qgraphicsitem_cast<PartPolygon*>(item);
			poly->setRotation(poly->rotation()+5);
		}
		else if (item->type() == PartText::Type) {
			PartText* text = qgraphicsitem_cast<PartText*>(item);
			text->setRotation(text->rotation()+5);
		}
		else if (item->type() == PartDynamicTextField::Type) {
			PartDynamicTextField* dyntext = qgraphicsitem_cast<PartDynamicTextField*>(item);
			dyntext->setRotation(dyntext->rotation()+5);
		}
		else {
			//item->setRotation(item->rotation()+5);
		}
	}
}


MirrorElementsCommand::MirrorElementsCommand(ElementScene *scene, QUndoCommand *parent) :
ElementEditionCommand(QObject::tr("Miroir de sélection", "undo caption"), scene, nullptr, parent)
{
	m_items = scene->selectedItems();
}

/**
	@brief MirrorElementsCommand::redo
*/
void MirrorElementsCommand::redo()
{
	foreach (auto *item, m_items) {
		if (item->type() == PartText::Type) {
			PartText* staticText = qgraphicsitem_cast<PartText*>(item);
			staticText->mirror();
		} else if (item->type() == PartDynamicTextField::Type)  {
			PartDynamicTextField* dyntext = qgraphicsitem_cast<PartDynamicTextField*>(item);
			dyntext->mirror();
		} else if (item->type() == PartArc::Type) {
			PartArc* arc = qgraphicsitem_cast<PartArc*>(item);
			arc->mirror();
		} else if (item->type() == PartEllipse::Type) {
			PartEllipse* ellipse = qgraphicsitem_cast<PartEllipse*>(item);
			ellipse->mirror();
		} else if (item->type() == PartLine::Type) {
			PartLine* line = qgraphicsitem_cast<PartLine*>(item);
			line->mirror();
		} else if (item->type() == PartPolygon::Type) {
			PartPolygon* poly = qgraphicsitem_cast<PartPolygon*>(item);
			poly->mirror();
		} else if (item->type() == PartRectangle::Type) {
			PartRectangle* rect = qgraphicsitem_cast<PartRectangle*>(item);
			rect->mirror();
		} else if (item->type() == PartTerminal::Type) {
			PartTerminal* term = qgraphicsitem_cast<PartTerminal*>(item);
			term->mirror();
		}
	}
}
/**
	@brief MirrorElementsCommand::undo
*/
void MirrorElementsCommand::undo()
{
	redo();
}


FlipElementsCommand::FlipElementsCommand(ElementScene *scene, QUndoCommand *parent) :
ElementEditionCommand(QObject::tr("Retourner la sélection", "undo caption"), scene, nullptr, parent)
{
	m_items = scene->selectedItems();
}

/**
	@brief FlipElementsCommand::redo
*/
void FlipElementsCommand::redo()
{
	foreach (auto *item, m_items) {
		if (item->type() == PartText::Type) {
			PartText* staticText = qgraphicsitem_cast<PartText*>(item);
			staticText->flip();
		} else if (item->type() == PartDynamicTextField::Type) {
			PartDynamicTextField* dyntext = qgraphicsitem_cast<PartDynamicTextField*>(item);
			dyntext->flip();
		} else if (item->type() == PartArc::Type) {
			PartArc* arc = qgraphicsitem_cast<PartArc*>(item);
			arc->flip();
		} else if (item->type() == PartEllipse::Type) {
			PartEllipse* ellipse = qgraphicsitem_cast<PartEllipse*>(item);
			ellipse->flip();
		} else if (item->type() == PartLine::Type) {
			PartLine* line = qgraphicsitem_cast<PartLine*>(item);
			line->flip();
		} else if (item->type() == PartPolygon::Type) {
			PartPolygon* poly = qgraphicsitem_cast<PartPolygon*>(item);
			poly->flip();
		} else if (item->type() == PartRectangle::Type) {
			PartRectangle* rect = qgraphicsitem_cast<PartRectangle*>(item);
			rect->flip();
		} else if (item->type() == PartTerminal::Type) {
			PartTerminal* term = qgraphicsitem_cast<PartTerminal*>(item);
			term->flip();
		}
	}
}

/**
	@brief FlipElementsCommand::undo
*/
void FlipElementsCommand::undo()
{
	redo();
}
