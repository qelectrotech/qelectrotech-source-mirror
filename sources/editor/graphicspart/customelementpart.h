/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef CUSTOM_ELEMENT_PART_H
#define CUSTOM_ELEMENT_PART_H

#include "qet.h"

class CustomElement;
class ElementPrimitiveDecorator;
class ElementScene;
class QETElementEditor;
class QUndoStack;
class QGraphicsItem;
class QGraphicsSceneMouseEvent;

/**
	This abstract class represents a primitive of the visual representation of an
	electrical element. The Element, FixedElement and CustomElement classes do not
	embed its attributes and methods in order to remain lightweight; indeed, there
	is no point for those classes to store their visual representation with
	anything more complex than a QImage.
*/
class CustomElementPart {
	// constructors, destructor
	public:
	/**
		Constructor
		@param editor Element editor this primitive is attached to
	*/
	CustomElementPart(QETElementEditor *editor) : element_editor(editor) {}
	/// Destructor
	virtual ~CustomElementPart() {}
	
	private:
	CustomElementPart(const CustomElementPart &);
	
	// attributes
	private:
	QETElementEditor *element_editor;
	
	// methods
	public:
	/**
		Load the primitive from an XML element that describes it
	*/
	virtual void fromXml(const QDomElement &) = 0;
	/**
		Export the primitive as an XML element
	*/
	virtual const QDomElement toXml(QDomDocument &) const = 0;
	/**
		Set a specific property of the primitive
	*/
	virtual void setProperty(const char *name, const QVariant &value) = 0;
	/**
		Get the current value of a specific primitive property
	*/
	virtual QVariant property(const char *name) const = 0;
	/**
		@return whether the primitive appears to be useless (e.g. 0-length line)
		Typically, useless primitives are discarded when saving the element.
	*/
	virtual bool isUseless() const = 0;
	virtual QRectF sceneGeometricRect() const = 0;
	/**
		Inform this part a user-induced transformation is about to begin. This method can be used to save data required by handleUserTransformation().
	*/
	virtual void startUserTransformation(const QRectF &) = 0;
	/**
		Make this part fit into the provided rectangle.
	*/
	virtual void handleUserTransformation(const QRectF &, const QRectF &) = 0;
	/// @return a pointer to the parent element editor
	virtual QETElementEditor *elementEditor() const;
	/**
		Call the updateCurrentPartEditor() slot of the editor
		@see QETElementEditor::updateCurrentPartEditor()
	*/
	virtual void updateCurrentPartEditor() const;
	/// @return a pointer to the parent editing scene
	virtual ElementScene *elementScene() const;
	/// @return the element editor undo stack
	virtual QUndoStack &undoStack() const;
	/// @return the name of the primitive
	virtual QString name() const = 0;
	/// @return the name that will be used as XML tag when exporting the primitive
	virtual QString xmlName() const = 0;
	
	virtual QGraphicsItem *toItem();
	
	virtual void setDecorator(ElementPrimitiveDecorator *);
	virtual QET::ScalingMethod preferredScalingMethod() const;
	virtual bool singleItemPressEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *);
	virtual bool singleItemMoveEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *);
	virtual bool singleItemReleaseEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *);
	virtual bool singleItemDoubleClickEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *);
	
	protected:
	QList<QPointF> mapPoints(const QRectF &, const QRectF &, const QList<QPointF> &);
};
#endif
