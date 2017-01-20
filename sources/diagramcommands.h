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
#ifndef DIAGRAM_COMMANDS_H
#define DIAGRAM_COMMANDS_H

#include "borderproperties.h"
#include "qetgraphicsitem/conductor.h"
#include "diagramcontent.h"
#include "titleblockproperties.h"
#include "qet.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "conductorprofile.h"
#include "diagram.h"

class DiagramTextItem;
class Element;
class ElementTextItem;
class IndependentTextItem;
class DiagramImageItem;
class QetGraphicsItem;

/**
 * @brief The AddItemCommand class
 * This command add an item in a diagram
 * The item to add is template, but must be QGraphicsItem or derived.
 */
template <typename QGI>
class AddItemCommand : public QUndoCommand {
	public:
		AddItemCommand(QGI item, Diagram *diagram, const QPointF &pos = QPointF(), QUndoCommand *parent = nullptr) :
			QUndoCommand (parent),
			m_item (item),
			m_diagram (diagram),
			m_pos(pos)
		{
			setText(QObject::tr("Ajouter ") + itemText(item));
			m_diagram -> qgiManager().manage(m_item);
		}

		virtual ~AddItemCommand() {
			m_diagram -> qgiManager().release(m_item);
		}

		virtual void undo() {
			m_diagram -> showMe();
			m_diagram -> removeItem(m_item);
			QUndoCommand::undo();
		}

		virtual void redo() {
			m_diagram -> showMe();
			m_diagram -> addItem(m_item);
			m_item    -> setPos(m_pos);
			QUndoCommand::redo();
		}

	private:
		QGI m_item;
		Diagram *m_diagram;
		QPointF m_pos;
};

//Return a string to describe a QGraphicsItem
QString itemText(const QetGraphicsItem     *item);
QString itemText(const IndependentTextItem *item);
QString itemText(const Conductor           *item);

/**
	This command removes content from a particular diagram.
*/
class DeleteElementsCommand : public QUndoCommand {
	// constructors, destructor
	public:
	DeleteElementsCommand(Diagram *, const DiagramContent &, QUndoCommand * = 0);
	virtual ~DeleteElementsCommand();
	private:
	DeleteElementsCommand(const DeleteElementsCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// removed content
	DiagramContent removed_content;
	/// diagram which the content is removed from
	Diagram *diagram;
	/// keep linked element for each removed element linked to other element.
	QHash <Element *, QList<Element *> > m_link_hash;
};

/**
	This command pastes some content onto a particular diagram.
*/
class PasteDiagramCommand : public QUndoCommand {
	// constructors, destructor
	public:
	PasteDiagramCommand(Diagram *, const DiagramContent &, QUndoCommand * = 0);
	virtual ~PasteDiagramCommand();
	private:
	PasteDiagramCommand(const PasteDiagramCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// pasted content
	DiagramContent content;
	/// diagram content is pasted onto
	Diagram *diagram;
	/// filter stating what kinds of items should be pasted
	int filter;
	/// prevent the first call to redo()
	bool first_redo;
};

/**
	This command cuts content from a particular diagram.
*/
class CutDiagramCommand : public DeleteElementsCommand {
	// constructors, destructor
	public:
	CutDiagramCommand(Diagram *, const DiagramContent &, QUndoCommand * = 0);
	virtual ~CutDiagramCommand();
	private:
	CutDiagramCommand(const CutDiagramCommand &);
};

/**
	This command moves some content on a particular diagram.
*/
class MoveElementsCommand : public QUndoCommand {
		// constructors, destructor
	public:
		MoveElementsCommand(Diagram *, const DiagramContent &, const QPointF &m, QUndoCommand * = 0);
		virtual ~MoveElementsCommand();
	private:
		MoveElementsCommand(const MoveElementsCommand &);
	
		// methods
	public:
		virtual void undo();
		virtual void redo();
		virtual void move(const QPointF &);

	private:
		void setupAnimation (QObject * target, const QByteArray &propertyName, const QVariant start, const QVariant end);
	
	// attributes
	private:
	/// diagram the movement takes place on.
	Diagram *diagram;
	/// moved content
	DiagramContent content_to_move;
	/// applied movement
	QPointF movement;
	///animation group
	QParallelAnimationGroup *m_anim_group;
	/// prevent the first call to redo()
	bool first_redo;
};

/**
	This command moves text items related to conductors on a particular
	diagram.
*/
class MoveConductorsTextsCommand : public QUndoCommand {
	// constructors, destructor
	public:
	MoveConductorsTextsCommand(Diagram *, QUndoCommand * = 0);
	virtual ~MoveConductorsTextsCommand();
	private:
	MoveConductorsTextsCommand(const MoveConductorsTextsCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	virtual void addTextMovement(ConductorTextItem *, const QPointF &, const QPointF &, bool = false);
	
	private:
	void regenerateTextLabel();
	
	// attributes
	private:
	/// diagram the movement takes place on.
	Diagram *diagram;
	/// text items to be moved
	QHash<ConductorTextItem *, QPair<QPointF, bool> > texts_to_move_;
	/// prevent the first call to redo()
	bool first_redo;
};

/**
	This commad modifies a text item.
*/
class ChangeDiagramTextCommand : public QUndoCommand {
	// constructors, destructor
	public:
	ChangeDiagramTextCommand(DiagramTextItem *, const QString &before, const QString &after, QUndoCommand * = 0);
	virtual ~ChangeDiagramTextCommand();
	private:
	ChangeDiagramTextCommand(const ChangeDiagramTextCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// modified text item
	DiagramTextItem *text_item;
	/// former text
	QString text_before;
	/// new text
	QString text_after;
	/// prevent the first call to redo()
	bool first_redo;
	Diagram *diagram;
};

/**
	This command rotates several elements or text items by a particular angle.
*/
class RotateElementsCommand : public QUndoCommand {
	// constructors, destructor
	public:
	RotateElementsCommand(const QList<Element *> &elements, const QList<DiagramTextItem *> &, const QList<DiagramImageItem *> &, QUndoCommand * = 0);
	virtual ~RotateElementsCommand();
	private:
	RotateElementsCommand(const RotateElementsCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// hold rotated elements along with their former orientation
	QList<Element *> elements_to_rotate;
	/// text items to be rotated
	QList<DiagramTextItem *> texts_to_rotate;
	/// images item to be rotated
	QList<DiagramImageItem *> images_to_rotate;
	/// angle of rotation to be applied to text items
	qreal applied_rotation_angle_;
	/// previous state of each conductor text item
	QHash<ConductorTextItem *, bool> previous_rotate_by_user_;
	Diagram *diagram;
};

/**
	This command directs several text items to a same particular angle of
	rotation.
*/
class RotateTextsCommand : public QUndoCommand {
	// constructors, destructor
	public:
	RotateTextsCommand(const QHash<DiagramTextItem *, double> &, double, QUndoCommand * = 0);
	RotateTextsCommand(const QList<DiagramTextItem *> &,         double, QUndoCommand * = 0);
	virtual ~RotateTextsCommand();
	private:
	RotateTextsCommand(const RotateTextsCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	private:
	void defineCommandName();
	
	// attributes
	private:
	/// hold rotated text items along with their former angle of rotation
	QHash<DiagramTextItem *, double> texts_to_rotate;
	/// angle of rotation of all text items after the command
	double applied_rotation_angle_;
	/// previous state of each conductor text item
	QHash<ConductorTextItem *, bool> previous_rotate_by_user_;
	Diagram *diagram;
};

/**
	This command changes a particular conductor.
*/
class ChangeConductorCommand : public QUndoCommand {
	// constructors, destructor
	public:
	ChangeConductorCommand(Conductor *, const ConductorProfile &, const ConductorProfile &, Qt::Corner, QUndoCommand * = 0);
	virtual ~ChangeConductorCommand();
	private:
	ChangeConductorCommand(const ChangeConductorCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	virtual void setConductorTextItemMove(const QPointF &, const QPointF &);
	
	// attributes
	private:
	/// changed conductor
	Conductor *conductor;
	/// profile before the change
	ConductorProfile old_profile;
	/// profile after the change
	ConductorProfile new_profile;
	/// Path type of the modified conductor
	Qt::Corner path_type;
	/// position of the text item before the change
	QPointF text_pos_before_mov_;
	/// position of the text item after the change
	QPointF text_pos_after_mov_;
	/// prevent the first call to redo()
	bool first_redo;
	Diagram *diagram;
};

/**
	This command resets conductor paths.
*/
class ResetConductorCommand : public QUndoCommand {
	// constructors, destructor
	public:
	ResetConductorCommand(const QHash<Conductor *, ConductorProfilesGroup> &, QUndoCommand * = 0);
	virtual ~ResetConductorCommand();
	private:
	ResetConductorCommand(const ResetConductorCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// impacted conductors along with their former profiles
	QHash<Conductor *, ConductorProfilesGroup> conductors_profiles;
	Diagram *diagram;
};

/**
	This command changes the title block properties for a particular diagram.
*/
class ChangeTitleBlockCommand : public QUndoCommand {
	// constructors, destructor
	public:
	ChangeTitleBlockCommand(Diagram *, const TitleBlockProperties &, const TitleBlockProperties &, QUndoCommand * = 0);
	virtual ~ChangeTitleBlockCommand();
	private:
	ChangeTitleBlockCommand(const ChangeTitleBlockCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// modified diagram
	Diagram *diagram;
	/// properties before the change
	TitleBlockProperties old_titleblock;
	/// properties after the change
	TitleBlockProperties new_titleblock;
};

/**
	This command changes the border properties of a particular diagram.
*/
class ChangeBorderCommand : public QUndoCommand {
	// constructors, destructor
	public:
	ChangeBorderCommand(Diagram *, const BorderProperties &, const BorderProperties &, QUndoCommand * = 0);
	virtual ~ChangeBorderCommand();
	private:
	ChangeBorderCommand(const ChangeBorderCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// modified diagram
	Diagram *diagram;
	public:
	/// properties before the change
	BorderProperties old_properties;
	/// properties after the change
	BorderProperties new_properties;
};

#endif
