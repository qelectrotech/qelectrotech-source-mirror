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
#ifndef PART_TEXT_H
#define PART_TEXT_H
#include <QtWidgets>
#include "customelementpart.h"
#include "qetapp.h"
class TextEditor;
class ElementPrimitiveDecorator;
/**
	This class represents an static text primitive which may be used to compose
	the drawing of an electrical element within the element editor.
*/
class PartText : public QGraphicsTextItem, public CustomElementPart {
	Q_OBJECT
	
	// constructors, destructor
	public:
    PartText(QETElementEditor *, QGraphicsItem * = nullptr);
	~PartText() override;
	
	private:
	PartText(const PartText &);
	
	// methods
	public:
	enum { Type = UserType + 1107 };
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		PartText.
		@return the QGraphicsItem type
	*/
	int type() const override { return Type; }
	QString name() const override { return(QObject::tr("texte", "element part name")); }
	QString xmlName() const override { return(QString("text")); }
	void fromXml(const QDomElement &) override;
	const QDomElement toXml(QDomDocument &) const override;
	void setRotation(qreal angle) {(QGraphicsObject::setRotation(QET::correctAngle(angle)));}
	bool isUseless() const override;
	QRectF sceneGeometricRect() const override;
	void startUserTransformation(const QRectF &) override;
	void handleUserTransformation(const QRectF &, const QRectF &) override;

	///PROPERTY
	void setProperty(const char *name, const QVariant &value) override {QGraphicsTextItem::setProperty(name, value);}
	QVariant property(const char *name) const override {return QGraphicsTextItem::property(name);}
	// Size value
	Q_PROPERTY(qreal size READ size WRITE setSize)
		qreal size () const {return font().pointSize();}
		void setSize (qreal s) {setFont(QETApp::dynamicTextsItemFont(s));}
	// Real size value
	Q_PROPERTY(qreal real_size READ realSize WRITE setRealSize)
		qreal realSize() const {return real_font_size_;}
		void setRealSize(qreal rs) {real_font_size_ = rs;}
	// Color value (true = black , false = white)
	Q_PROPERTY(bool color READ isBlack WRITE setBlack)
		bool isBlack() const {return defaultTextColor() == Qt::black;}
		void setBlack(bool b) {setDefaultTextColor(b ? Qt::black : Qt::white);}
	// displayed string
	Q_PROPERTY(QString text READ toPlainText WRITE setPlainText)
	
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
        QGraphicsItem *decorator_;
        QPointF m_origine_pos;
};
#endif
