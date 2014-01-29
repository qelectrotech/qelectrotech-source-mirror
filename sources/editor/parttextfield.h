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
#ifndef PART_TEXTFIELD_H
#define PART_TEXTFIELD_H
#include <QtGui>
#include "customelementpart.h"
class TextFieldEditor;
class QETElementEditor;
class ElementPrimitiveDecorator;
/**
	This class represents an editable text field which may be used to compose the
	drawing of an electrical element within the element editor. Users may specify
	a default value. The field will remain editable once the element is added onto
	a diagram. lorsque l'element sera pose sur un schema.
*/
class PartTextField : public QGraphicsTextItem, public CustomElementPart {
	Q_OBJECT
	
	// constructors, destructor
	public:
	PartTextField(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartTextField();
	
	private:
	PartTextField(const PartTextField &);
	
	// attributes
	bool follow_parent_rotations;
	
	// methods
	public:
	enum { Type = UserType + 1108 };
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		PartTextField.
		@return the QGraphicsItem type
	*/
	virtual int type() const { return Type; }
	virtual QString name() const { return(QObject::tr("champ de texte", "element part name")); }
	virtual QString xmlName() const { return(QString("input")); }
	void fromXml(const QDomElement &);
	const QDomElement toXml(QDomDocument &) const;
	qreal rotationAngle() const;
	void setRotationAngle(const qreal &);
	bool followParentRotations();
	void setFollowParentRotations(bool);
	virtual void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	virtual bool isUseless() const;
	virtual QRectF sceneGeometricRect() const;
	virtual void startUserTransformation(const QRectF &);
	virtual void handleUserTransformation(const QRectF &, const QRectF &);
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0 );
	
	virtual void setDecorator(ElementPrimitiveDecorator *);
	virtual bool singleItemPressEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *);
	virtual bool singleItemMoveEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *);
	virtual bool singleItemReleaseEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *);
	virtual bool singleItemDoubleClickEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *);
	
	public slots:
	void adjustItemPosition(int = 0);
	void setEditable(bool);
	void startEdition();
	void endEdition();
	
	protected:
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *);
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *);
	virtual bool sceneEventFilter(QGraphicsItem *, QEvent *);
	virtual void focusInEvent(QFocusEvent *);
	virtual void focusOutEvent(QFocusEvent *);
	virtual void keyPressEvent(QKeyEvent *);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
	virtual QVariant itemChange(GraphicsItemChange, const QVariant &);
	QRectF boundingRect() const;
	
	private:
	QPointF margin() const;
#ifdef QET_DEBUG_EDITOR_TEXTS
	void drawPoint(QPainter *, const QPointF &);
#endif
	QString previous_text;
	qreal real_font_size_;
	QPointF saved_point_;
	qreal saved_font_size_;
	QGraphicsItem *decorator_;
};
#endif
