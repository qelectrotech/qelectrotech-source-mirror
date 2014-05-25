/*
	Copyright 2006-2014 The QElectroTech Team
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
#include <QtGui>
#include "borderproperties.h"
#include "qetgraphicsitem/conductor.h"
#include "conductorproperties.h"
#include "diagramcontent.h"
#include "titleblockproperties.h"
#include "qet.h"
#include "qetgraphicsitem/qetshapeitem.h"
class Diagram;
class DiagramTextItem;
class Element;
class ElementTextItem;
class IndependentTextItem;
class DiagramImageItem;

/**
	This command adds an element to a particular diagram.
*/
class AddElementCommand : public QUndoCommand {
	// constructors, destructor
	public:
	AddElementCommand(Diagram *, Element *, const QPointF &, QUndoCommand * = 0);
	virtual ~AddElementCommand();
	private:
	AddElementCommand(const AddElementCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// added element
	Element *element;
	/// diagram the element is added to
	Diagram *diagram;
	/// position of the element on the diagram
	QPointF position;
};

/**
	This command adds an independent (i.e. related to neither an element nor a
	conductor) text item to a particular diagram.
*/
class AddTextCommand : public QUndoCommand {
	// constructors, destructor
	public:
	AddTextCommand(Diagram *, IndependentTextItem *, const QPointF &, QUndoCommand * = 0);
	virtual ~AddTextCommand();
	private:
	AddTextCommand(const AddTextCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// added text item
	IndependentTextItem *textitem;
	/// diagram the text item is added to
	Diagram *diagram;
	/// position of the text item on the diagram
	QPointF position;
};

/**
  This command adds an image item to a particular diagram
*/
class AddImageCommand : public QUndoCommand {
	//constructors, destructor
	public:
	AddImageCommand (Diagram *, DiagramImageItem *, const QPointF &, QUndoCommand * = 0);
	virtual ~AddImageCommand();
	private:
	AddImageCommand(const AddImageCommand &);

	//methods
	public:
	virtual void undo();
	virtual void redo();

	// attributes
	private:
	/// added image item
	DiagramImageItem *imageitem;
	/// diagram the image item is added to
	Diagram *diagram;
	/// position of the image item on the diagram
	QPointF position;

};


/**
  This command adds an image item to a particular diagram
*/
class AddShapeCommand : public QUndoCommand {
	//constructors, destructor
	public:
	AddShapeCommand (Diagram *, QetShapeItem *, const QPointF &, QUndoCommand * = 0);
	virtual ~AddShapeCommand();
	private:
	AddShapeCommand(const AddShapeCommand &);

	//methods
	public:
	virtual void undo();
	virtual void redo();

	// attributes
	private:
	/// added shape item
	QetShapeItem *shapeitem;
	/// diagram the image item is added to
	Diagram *diagram;
	/// position of the image item on the diagram
	QPointF position;

};

/**
	This command adds a conductor to a particular diagram.
*/
class AddConductorCommand : public QUndoCommand {
	// constructors, destructor
	public:
	AddConductorCommand(Diagram *, Conductor *, QUndoCommand * = 0);
	virtual ~AddConductorCommand();
	private:
	AddConductorCommand(const AddConductorCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// added conductor
	Conductor *conductor;
	/// diagram the conductor is added to
	Diagram *diagram;
};

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
	virtual void addConductorTextItemMovement(ConductorTextItem *, const QPointF &, const QPointF &);
	
	// attributes
	private:
	/// diagram the movement takes place on.
	Diagram *diagram;
	/// moved content
	DiagramContent content_to_move;
	/// applied movement
	QPointF movement;
	/**
		Moving elements impacts their conductors: either they are moved, or their path
		needs to be generated again, which in turn tends to move their child text
		items. This attribute holds both new and previous positions for each moved
		text item.
	*/
	QHash<ConductorTextItem *, QPair<QPointF, QPointF> > moved_conductor_texts_;
	/// prevent the first call to redo()
	bool first_redo;
};

/**
	This command moves text items related to elements on a particular diagram.
*/
class MoveElementsTextsCommand : public QUndoCommand {
	// constructors, destructor
	public:
	MoveElementsTextsCommand(Diagram *, const QSet<ElementTextItem *> &, const QPointF &m, QUndoCommand * = 0);
	virtual ~MoveElementsTextsCommand();
	private:
	MoveElementsTextsCommand(const MoveElementsTextsCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	virtual void move(const QPointF &);
	
	// attributes
	private:
	/// diagram the movement takes place on.
	Diagram *diagram;
	/// text items to be moved
	QSet<ElementTextItem *> texts_to_move;
	/// applied movement
	QPointF movement;
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

/**
	This command changes the properties for a particular conductor.
*/
class ChangeConductorPropertiesCommand : public QUndoCommand {
	// constructors, destructor
	public:
	ChangeConductorPropertiesCommand(Conductor *, QUndoCommand * = 0);
	virtual ~ChangeConductorPropertiesCommand();
	private:
	ChangeConductorPropertiesCommand(const ChangeConductorPropertiesCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	virtual void setOldSettings(const ConductorProperties &);
	virtual void setNewSettings(const ConductorProperties &);
	
	// attributes
	private:
	/// modified conductor
	Conductor *conductor;
	/// properties before the change
	ConductorProperties old_properties;
	/// properties after the change
	ConductorProperties new_properties;
	/// track whether pre-change properties were set
	bool old_settings_set;
	/// track whether post-change properties were set
	bool new_settings_set;
	Diagram *diagram;
};

/**
	This command changes the properties for several conductors.
*/
class ChangeSeveralConductorsPropertiesCommand : public QUndoCommand {
	// constructors, destructor
	public:
	ChangeSeveralConductorsPropertiesCommand(QList<Conductor *>, QUndoCommand * = 0);
	virtual ~ChangeSeveralConductorsPropertiesCommand();
	private:
	ChangeSeveralConductorsPropertiesCommand(const ChangeSeveralConductorsPropertiesCommand &);

	// methods
	public:
	virtual void undo();
	virtual void redo();
	virtual void setOldSettings(const QList<ConductorProperties> &);
	virtual void setNewSettings(const QList<ConductorProperties> &);
	virtual void setNewSettings(const ConductorProperties &);

	// attributes
	private:
	/// modified conductor
	QList<Conductor *> conductors;
	/// properties before the change
	QList <ConductorProperties> old_properties;
	/// properties after the change
	QList <ConductorProperties> new_properties;
	///  single properties for each conductor
	ConductorProperties single_new_properties;
	/// track whether pre-change properties were set
	bool old_settings_set;
	/// track whether post-change properties were set
	bool new_settings_set;
	Diagram *diagram;
};

class ImageResizerCommand : public QUndoCommand {
	//constructor and destructor
	public:
	ImageResizerCommand (DiagramImageItem *image, qreal &old_, qreal &new_, QUndoCommand *parent = 0);
	virtual ~ImageResizerCommand();

	//methods
	public:
	virtual void undo();
	virtual void redo();

	//attributes
	private:
	DiagramImageItem *image_;
	qreal old_size, new_size;
	Diagram *diagram;
};


class ChangeShapeStyleCommand : public QUndoCommand {
	//constructor and destructor
	public:
	ChangeShapeStyleCommand (QetShapeItem *shape, Qt::PenStyle &old_, Qt::PenStyle &new_, QUndoCommand *parent = 0);
	virtual ~ChangeShapeStyleCommand();

	//methods
	public:
	virtual void undo();
	virtual void redo();

	//attributes
	private:
	QetShapeItem *shape_;
	Qt::PenStyle old_style, new_style;
	Diagram *diagram;
};

class ChangeShapeScaleCommand : public QUndoCommand {
	//constructor and destructor
	public:
	ChangeShapeScaleCommand (QetShapeItem *shape, double scale_factor, QUndoCommand *parent = 0);
	virtual ~ChangeShapeScaleCommand();

	//methods
	public:
	virtual void undo();
	virtual void redo();

	//attributes
	private:
	QetShapeItem *shape_;
	double factor;
	Diagram *diagram;
};

class LinkElementsCommand : public QUndoCommand {
	public:
	// constructor destructor
	LinkElementsCommand (Element *elmt1, Element *elmt2, QUndoCommand *parent = 0);
	LinkElementsCommand (Element *elmt1, QList <Element *> &elmtList, QUndoCommand *parent = 0);
	virtual ~LinkElementsCommand();
	//methods
	virtual void undo();
	virtual void redo();

	private:
	//attributes
	Diagram *diagram_;
	Element *element_;
	QList <Element *> elmt_list;
	QList <Element *> previous_linked;
	bool first_redo;
};

class unlinkElementsCommand : public QUndoCommand {
	public:
	//constructor destructor
	unlinkElementsCommand (Element *elmt1, Element *elmt2 = 0, QUndoCommand *parent = 0);
	unlinkElementsCommand (Element *elmt1, QList <Element *> &elmtList, QUndoCommand *parent = 0);
	virtual ~unlinkElementsCommand();
	//methods
	virtual void undo();
	virtual void redo();

	private:
	//attributes
	Diagram *diagram_;
	Element *element_;
	QList <Element *> elmt_list;
};

#endif
