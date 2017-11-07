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
#ifndef PARTDYNAMICTEXTFIELD_H
#define PARTDYNAMICTEXTFIELD_H

#include "customelementpart.h"
#include "QGraphicsTextItem"
#include "dynamicelementtextitem.h"

/**
 * @brief The PartDynamicTextField class
 * This class represents an editable dynamic text field which may be used to compose the
 * drawing of an electrical element within the element editor.
 * The field will remain editable once the element is added onto
 * a diagram
 */
class PartDynamicTextField : public QGraphicsTextItem, public CustomElementPart
{
	Q_OBJECT
	
	Q_PROPERTY(QString tagg READ tagg WRITE setTagg NOTIFY taggChanged)
	Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
	Q_PROPERTY(DynamicElementTextItem::TextFrom textFrom READ textFrom WRITE setTextFrom NOTIFY textFromChanged)
	Q_PROPERTY(QString infoName READ infoName WRITE setInfoName NOTIFY infoNameChanged)
	Q_PROPERTY(QString compositeText READ compositeText WRITE setCompositeText NOTIFY compositeTextChanged)
	Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
	Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
	Q_PROPERTY(bool frame READ frame WRITE setFrame NOTIFY frameChanged)
	
	public:
		static bool canImportFromTextField(const QDomElement &dom_element);
	
	public:
			///PROPERTY
		void setProperty(const char *name, const QVariant &value) override {QGraphicsTextItem::setProperty(name, value);}
		QVariant property(const char *name) const override {return QGraphicsTextItem::property(name);}
	
	signals:
		void taggChanged(QString tagg);
		void textChanged(QString text);
		void textFromChanged(DynamicElementTextItem::TextFrom text_from);
		void infoNameChanged(QString info);
		void compositeTextChanged(QString text);
		void colorChanged(QColor color);
		void fontSizeChanged(int size);
		void frameChanged(bool frame);
	
	public:
		PartDynamicTextField(QETElementEditor *editor, QGraphicsItem *parent = nullptr);
		
		enum {Type = UserType + 1110};
		int type() const override {return Type;}
		
		QString name() const override;
		QString xmlName() const override;
		bool isUseless() const override {return false;}
		QRectF sceneGeometricRect() const override {return sceneBoundingRect();}
		void startUserTransformation(const QRectF &initial_selection_rect) override;
		void handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect) override;
		
		const QDomElement toXml(QDomDocument &dom_doc) const override;
		void fromXml(const QDomElement &dom_elmt) override;
		void fromTextFieldXml(const QDomElement &dom_element);
		
		DynamicElementTextItem::TextFrom textFrom() const;
		void setTextFrom (DynamicElementTextItem::TextFrom text_from);
		QString tagg() const;
		void setTagg(const QString &tagg);
		QString text() const;
		void setText(const QString &text);
		void setInfoName(const QString &info_name);
		QString infoName() const;
		void setCompositeText(const QString &text);
		QString compositeText() const;
		void setColor(QColor color);
        QColor color() const;
		void setFontSize(int s);
        int fontSize()const;
		void setFrame(bool frame);
		bool frame() const;
		
	protected:
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
		
	private:
		QPointF m_origine_pos,
				m_saved_point;
		QString m_tagg,
				m_text,
				m_info_name,
				m_composite_text;
		
		DynamicElementTextItem::TextFrom m_text_from = DynamicElementTextItem::UserText;
		QUuid m_uuid;
		
		bool m_frame = false;
};

#endif // PARTDYNAMICTEXTFIELD_H
