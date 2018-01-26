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
#ifndef DYNAMICELEMENTTEXTITEM_H
#define DYNAMICELEMENTTEXTITEM_H

#include "diagramtextitem.h"
#include "element.h"
#include <QUuid>
#include <QPointer>

class Element;
class Conductor;
class ElementTextItemGroup;
class CrossRefItem;

/**
 * @brief The DynamicElementTextItem class
 * This class provide a simple text field of element who can be added or removed directly from the diagram editor.
 * This text is created to compensate a big lack of the ElementTextItem : ElementTextItem can't be added or removed directly in the diagram editor
 * 
 */
class DynamicElementTextItem : public DiagramTextItem
{
	friend class DynamicTextItemDelegate;
	friend class CompositeTextEditDialog;
	friend class CustomElement;
	
    Q_OBJECT
    
	Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
	Q_PROPERTY(TextFrom textFrom READ textFrom WRITE setTextFrom NOTIFY textFromChanged)
	Q_PROPERTY(QString infoName READ infoName WRITE setInfoName NOTIFY infoNameChanged)
	Q_PROPERTY(QString compositeText READ compositeText WRITE setCompositeText NOTIFY compositeTextChanged)
	Q_PROPERTY(bool frame READ frame WRITE setFrame NOTIFY frameChanged)
	Q_PROPERTY(qreal textWidth READ textWidth WRITE setTextWidth NOTIFY textWidthChanged)
	
	public:
		Q_ENUMS(TextFrom)
		enum TextFrom {
			UserText,
			ElementInfo,
			CompositeText
		};
		enum {Type = UserType + 1010};
		int type() const override {return Type;}
		
	signals:
		void textChanged(QString text);
		void textFromChanged(DynamicElementTextItem::TextFrom text_from);
		void infoNameChanged(QString info);
		void compositeTextChanged(QString text);
		void frameChanged(bool frame);
		void plainTextChanged();
		void textWidthChanged(qreal width);
	
	public:
		DynamicElementTextItem(Element *parent_element);
		~DynamicElementTextItem() override;
		static QMetaEnum textFromMetaEnum();
	private:
		DynamicElementTextItem ();
		DynamicElementTextItem(const DynamicElementTextItem &);
		
	public:
		QDomElement toXml(QDomDocument &dom_doc) const override;
		void fromXml(const QDomElement &dom_elmt) override;
		
		Element *parentElement() const;
		ElementTextItemGroup *parentGroup() const;
		Element *elementUseForInfo() const;
		void refreshLabelConnection();
		
		DynamicElementTextItem::TextFrom textFrom() const;
		void setTextFrom (DynamicElementTextItem::TextFrom text_from);
		QString text() const;
		void setText(const QString &text);
        static QString xmlTaggName() {return QString("dynamic_elmt_text");}
		void setInfoName(const QString &info_name);
		QString infoName() const;
		void setCompositeText(const QString &text);
		QString compositeText() const;
		void setFrame(const bool frame);
		bool frame() const;
		QUuid uuid() const;
		void updateXref();
		void setPlainText(const QString &text);
		void setTextWidth(qreal width);
		
	protected:
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
		void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
		bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
		
	private:
		void elementInfoChanged();
		void masterChanged();
		void reportChanged();
		void reportFormulaChanged();
		void setConnectionForReportFormula(const QString &formula);
		void removeConnectionForReportFormula(const QString &formula);
		void setupFormulaConnection();
		void clearFormulaConnection();
		void updateReportFormulaConnection();
		void updateReportText();
		void updateLabel();
		void conductorWasAdded(Conductor *conductor);
		void conductorWasRemoved(Conductor *conductor);
		void setPotentialConductor();
		void conductorPropertiesChanged();
		QString reportReplacedCompositeText() const;
		void zoomToLinkedElement();
		
	private:
		QPointer <Element> m_parent_element,
						   m_master_element,
						   m_other_report;
		QPointer <Conductor> m_watched_conductor;
		QString m_text,
				m_info_name,
				m_composite_text,
				m_report_formula,
				m_F_str;
		DynamicElementTextItem::TextFrom m_text_from = UserText;
		QUuid m_uuid;
		QMetaObject::Connection m_report_formula_con;
		QList<QMetaObject::Connection> m_formula_connection,
									   m_update_slave_Xref_connection;
		QColor m_user_color;
		bool m_frame = false,
			 m_first_scene_change = true;
		CrossRefItem *m_Xref_item = nullptr;
		QGraphicsTextItem *m_slave_Xref_item = nullptr;
		qreal m_text_width = -1;
};

#endif // DYNAMICELEMENTTEXTITEM_H
