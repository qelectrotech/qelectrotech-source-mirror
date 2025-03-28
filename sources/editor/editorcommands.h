/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef EDITOR_COMMANDS_H
#define EDITOR_COMMANDS_H
#include "../editor/graphicspart/customelementpart.h"
#include "../qgimanager.h"
#include "elementcontent.h"
#include "elementscene.h"
#include "elementview.h"
#include "graphicspart/abstractpartellipse.h"
#include "graphicspart/customelementgraphicpart.h"
#include "graphicspart/customelementpart.h"
#include "graphicspart/partarc.h"
#include "graphicspart/partdynamictextfield.h"
#include "graphicspart/partellipse.h"
#include "graphicspart/partline.h"
#include "graphicspart/partpolygon.h"
#include "graphicspart/partrectangle.h"
#include "graphicspart/partterminal.h"
#include "graphicspart/parttext.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "UndoCommand/deletepartscommand.h"


/**
	@brief The ElementEditionCommand class
	ElementEditionCommand is the base class for all commands classes involved in
	the edition of an electrical element. It provides commonly required methods
	and attributes, such as accessors to the modified scene and view.
*/
class ElementEditionCommand : public QUndoCommand
{
		// constructors, destructor
	public:
		ElementEditionCommand(ElementScene * = nullptr,
				      ElementView * = nullptr,
				      QUndoCommand * = nullptr);
		ElementEditionCommand(const QString &,
				      ElementScene * = nullptr,
				      ElementView * = nullptr,
				      QUndoCommand * = nullptr);
		~ElementEditionCommand() override;
	private:
		ElementEditionCommand(const ElementEditionCommand &);
	
		// methods
	public:
		ElementScene *elementScene() const;
		void setElementScene(ElementScene *);
		ElementView *elementView() const;
		void setElementView(ElementView *);
	
		// attributes
	protected:
		/// Element editor/view/scene the command should take place on
		ElementScene *m_scene;
		ElementView *m_view;
};

/**
	This command cut primitives when editing an electrical element.
*/
class CutPartsCommand : public DeletePartsCommand {
	// constructors, destructor
	public:
	CutPartsCommand(ElementScene *, const QList<QGraphicsItem *>&, QUndoCommand * = nullptr);
	~CutPartsCommand() override;
	private:
	CutPartsCommand(const CutPartsCommand &);
};

/**
	This command moves primitives when editing an electrical element.
*/
class MovePartsCommand : public ElementEditionCommand {
	// constructors, destructor
	public:
	MovePartsCommand(const QPointF &, ElementScene *, const QList<QGraphicsItem *>&, QUndoCommand * = nullptr);
	~MovePartsCommand() override;
	private:
	MovePartsCommand(const MovePartsCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
	
	// attributes
	private:
	/// List of moved primitives
	QList<QGraphicsItem *> moved_parts;
	/// applied movement
	QPointF movement;
	/// Prevent the first call to redo()
	bool first_redo;
};

/**
	This command changes the translated names of an electrical element.
*/
class ChangeNamesCommand : public ElementEditionCommand {
	// constructors, destructor
	public:
	ChangeNamesCommand(ElementScene *, const NamesList &, const NamesList &, QUndoCommand * = nullptr);
	~ChangeNamesCommand() override;
	private:
	ChangeNamesCommand(const ChangeNamesCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
	
	// attributes
	private:
	/// List of former names
	NamesList names_before;
	/// List of new names
	NamesList names_after;
};

/**
	This command changes the zValue of a set of primitives when editing an
	electrical element.
*/
class ChangeZValueCommand : public ElementEditionCommand
{
		// constructors, destructor
	public:
	ChangeZValueCommand(ElementScene *, QET::DepthOption , QUndoCommand * = nullptr);
	~ChangeZValueCommand() override;
	private:
	ChangeZValueCommand(const ChangeZValueCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
	
	private:
	void applyBringForward(const QList<QGraphicsItem *> &);
	void applyRaise(const QList<QGraphicsItem *> &);
	void applyLower(const QList<QGraphicsItem *> &);
	void applySendBackward(const QList<QGraphicsItem *> &);
	
	// attributes
	private:
	/// associates impacted primitives with their former zValues
	QHash<QGraphicsItem *, qreal> undo_hash;
	/// associates impacted primitives with their new zValues
	QHash<QGraphicsItem *, qreal> redo_hash;
	/// kind of treatment to apply
	QET::DepthOption m_option;
};

/**
	This command changes extra information carried by an electrical element.
*/
class ChangeInformationsCommand : public ElementEditionCommand {
	// constructors, destructor
	public:
	ChangeInformationsCommand(ElementScene *, const QString &, const QString &, QUndoCommand * = nullptr);
	~ChangeInformationsCommand() override;
	private:
	ChangeInformationsCommand(const ChangeInformationsCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
	
	// attributes
	private:
	/// Former information
	QString old_informations_;
	/// New information
	QString new_informations_;
};

/**
	This command scales primitives when editing an electrical element.
*/
class ScalePartsCommand : public ElementEditionCommand {
	// constructors, destructor
	public:
	ScalePartsCommand(ElementScene * = nullptr, QUndoCommand * = nullptr);
	~ScalePartsCommand() override;
	private:
	ScalePartsCommand(const ScalePartsCommand &);
	
	// methods
	public:
	void undo() override;
	void redo() override;
	ElementScene *elementScene() const;
	void setScaledPrimitives(const QList<CustomElementPart *> &);
	QList<CustomElementPart *> scaledPrimitives() const;
	void setTransformation(const QRectF &, const QRectF &);
	QPair<QRectF, QRectF> transformation();
	
	protected:
	void scale(const QRectF &before, const QRectF &after);
	void adjustText();
	
	// attributes
	private:
	/// List of moved primitives
	QList<CustomElementPart *> scaled_primitives_;
	/// original rect items fit in
	QRectF original_rect_;
	/// new rect items should fit in
	QRectF new_rect_;
	/// Prevent the first call to redo()
	bool first_redo;
};

class changeElementDataCommand : public ElementEditionCommand
{
	public:
		changeElementDataCommand(ElementScene *scene,
								 ElementData old_data,
								 ElementData new_data,
								 QUndoCommand *parent = nullptr);
		~changeElementDataCommand() override {}

		void undo() override;
		void redo() override;

	private:
		ElementData m_old,
					m_new;
};

/**
	@brief The RotateSelectionInESCommand class
	Rotate the selected items in the element editor
*/

class RotateElementsCommand : public ElementEditionCommand
{

public:
	RotateElementsCommand(ElementScene *scene, QUndoCommand *parent=nullptr);
	void undo() override;
	void redo() override;

private:
	ElementScene *m_scene =nullptr;
	QList<QGraphicsItem*> m_items;

};

class RotateFineElementsCommand : public ElementEditionCommand
{

public:
	RotateFineElementsCommand(ElementScene *scene, QUndoCommand *parent=nullptr);
	void undo() override;
	void redo() override;

private:
	ElementScene *m_scene =nullptr;
	QList<QGraphicsItem*> m_items;

};

class MirrorElementsCommand : public ElementEditionCommand
{
public:
	MirrorElementsCommand(ElementScene *scene, QUndoCommand *parent=nullptr);
	void undo() override;
	void redo() override;
private:
	ElementScene *m_scene =nullptr;
	QList<QGraphicsItem*> m_items;
};

class FlipElementsCommand : public ElementEditionCommand
{
public:
	FlipElementsCommand(ElementScene *scene, QUndoCommand *parent=nullptr);
	void undo() override;
	void redo() override;
private:
	ElementScene *m_scene =nullptr;
	QList<QGraphicsItem*> m_items;
};

#endif
