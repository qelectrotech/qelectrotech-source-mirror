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
class PartTextField : public QGraphicsTextItem, public CustomElementPart
{
	Q_OBJECT
	
	// constructors, destructor
	public:
    PartTextField(QETElementEditor *, QGraphicsItem * = nullptr);
	~PartTextField() override;
	
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
	int type() const override { return Type; }
	QString name() const override { return(QObject::tr("champ de texte", "element part name")); }
	QString xmlName() const override { return(QString("input")); }
	void fromXml(const QDomElement &) override;
	const QDomElement toXml(QDomDocument &) const override;
	bool isUseless() const override;
	QRectF sceneGeometricRect() const override;
	void startUserTransformation(const QRectF &) override;
	void handleUserTransformation(const QRectF &, const QRectF &) override;

	///PROPERTY
	void setProperty(const char *name, const QVariant &value) override {QGraphicsTextItem::setProperty(name, value);}
	QVariant property(const char *name) const override {return QGraphicsTextItem::property(name);}

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
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
        void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
        void focusInEvent(QFocusEvent *) override;
        void focusOutEvent(QFocusEvent *) override;
        void keyPressEvent(QKeyEvent *) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) override;
        QVariant itemChange(GraphicsItemChange, const QVariant &) override;
        QRectF boundingRect() const override;
	
	private:
        QPointF margin() const;
        QString previous_text;
        qreal real_font_size_;
        QPointF saved_point_;
        qreal saved_font_size_;
        QPointF m_origine_pos;
};
#endif
