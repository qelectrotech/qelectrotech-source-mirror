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
#include "undocommand/deleteqgraphicsitemcommand.h"

class DiagramTextItem;
class Element;
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

		~AddItemCommand() override {
			m_diagram -> qgiManager().release(m_item);
		}

		void undo() override {
			m_diagram -> showMe();
			m_diagram -> removeItem(m_item);
			QUndoCommand::undo();
		}

		void redo() override {
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
	This command pastes some content onto a particular diagram.
*/
class PasteDiagramCommand : public QUndoCommand {
	// constructors, destructor
	public:
	PasteDiagramCommand(Diagram *, const DiagramContent &, QUndoCommand * = nullptr);
	~PasteDiagramCommand() override;
	private:
	PasteDiagramCommand(const PasteDiagramCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
	
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
class CutDiagramCommand : public DeleteQGraphicsItemCommand {
	// constructors, destructor
	public:
	CutDiagramCommand(Diagram *, const DiagramContent &, QUndoCommand * = nullptr);
	~CutDiagramCommand() override;
	private:
	CutDiagramCommand(const CutDiagramCommand &);
};

/**
	This command moves some content on a particular diagram.
*/
class MoveElementsCommand : public QUndoCommand {
		// constructors, destructor
	public:
		MoveElementsCommand(Diagram *, const DiagramContent &, const QPointF &m, QUndoCommand * = nullptr);
		~MoveElementsCommand() override;
	private:
		MoveElementsCommand(const MoveElementsCommand &);
	
		// methods
	public:
		void undo() override;
		void redo() override;
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
	MoveConductorsTextsCommand(Diagram *, QUndoCommand * = nullptr);
	~MoveConductorsTextsCommand() override;
	private:
	MoveConductorsTextsCommand(const MoveConductorsTextsCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
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
	ChangeDiagramTextCommand(DiagramTextItem *, const QString &before, const QString &after, QUndoCommand * = nullptr);
	~ChangeDiagramTextCommand() override;
	private:
	ChangeDiagramTextCommand(const ChangeDiagramTextCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
	
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
	This command changes a particular conductor.
*/
class ChangeConductorCommand : public QUndoCommand {
	// constructors, destructor
	public:
	ChangeConductorCommand(Conductor *, const ConductorProfile &, const ConductorProfile &, Qt::Corner, QUndoCommand * = nullptr);
	~ChangeConductorCommand() override;
	private:
	ChangeConductorCommand(const ChangeConductorCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
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
	ResetConductorCommand(const QHash<Conductor *, ConductorProfilesGroup> &, QUndoCommand * = nullptr);
	~ResetConductorCommand() override;
	private:
	ResetConductorCommand(const ResetConductorCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
	
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
	ChangeTitleBlockCommand(Diagram *, const TitleBlockProperties &, const TitleBlockProperties &, QUndoCommand * = nullptr);
	~ChangeTitleBlockCommand() override;
	private:
	ChangeTitleBlockCommand(const ChangeTitleBlockCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
	
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
	ChangeBorderCommand(Diagram *, const BorderProperties &, const BorderProperties &, QUndoCommand * = nullptr);
	~ChangeBorderCommand() override;
	private:
	ChangeBorderCommand(const ChangeBorderCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
	
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
