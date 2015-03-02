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
#ifndef PART_TEXTFIELD_H
#define PART_TEXTFIELD_H
#include <QtWidgets>
#include "customelementpart.h"
#include "qetapp.h"
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
	QString m_tagg;
	
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

	///PROPERTY
	virtual void setProperty(const char *name, const QVariant &value) {QGraphicsTextItem::setProperty(name, value);}
	virtual QVariant property(const char *name) const {return QGraphicsTextItem::property(name);}

	// displayed text
	Q_PROPERTY(QString text READ toPlainText WRITE setPlainText)
	// font size
	Q_PROPERTY(int   size READ size  WRITE setSize)
		int size() const {return font().pointSize();}
		void setSize (const int value) {setFont(QETApp::diagramTextsFont(value)); real_font_size_ = value;}
	// real size
	Q_PROPERTY(qreal real_size READ realSize WRITE setRealSize)
		qreal realSize() const {return real_font_size_;}
		void setRealSize(const qreal size) {real_font_size_ = size;}
	// angle of text
	Q_PROPERTY(qreal rotation_angle READ rotation WRITE setRotationAngle)
		void setRotationAngle(const qreal &angle) {setRotation(QET::correctAngle(angle));}
	// follow parent rotation
	Q_PROPERTY(bool rotate READ followParentRotations WRITE setFollowParentRotations)
		bool followParentRotations() const {return follow_parent_rotations;}
		void setFollowParentRotations(bool i) {follow_parent_rotations = i;}
	// tagg of text
	Q_PROPERTY(QString tagg READ tagg WRITE setTagg)
		QString tagg() const {return m_tagg;}
		void setTagg(const QString &tagg) {m_tagg = tagg;}


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
