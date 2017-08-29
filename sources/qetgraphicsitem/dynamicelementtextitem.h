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
#include <QUuid>
#include <QPointer>

class Element;

/**
 * @brief The DynamicElementTextItem class
 * This class provide a simple text field of element who can be added or removed directly from the diagram editor.
 * This text is created to compensate a big lack of the ElementTextItem : ElementTextItem can't be added or removed directly in the diagram editor
 * 
 */
class DynamicElementTextItem : public DiagramTextItem
{
    Q_OBJECT
    
	Q_PROPERTY(QString tagg READ tagg WRITE setTagg NOTIFY taggChanged)
	Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
	Q_PROPERTY(TextFrom textFrom READ textFrom WRITE setTextFrom NOTIFY textFromChanged)
	Q_PROPERTY(QString infoName READ infoName WRITE setInfoName NOTIFY infoNameChanged)
	Q_PROPERTY(QString compositeText READ compositeText WRITE setCompositeText NOTIFY compositeTextChanged)
	
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
		void taggChanged(QString tagg);
		void textChanged(QString text);
		void textFromChanged(DynamicElementTextItem::TextFrom text_from);
		void infoNameChanged(QString info);
		void compositeTextChanged(QString text);
	
	public:
		DynamicElementTextItem(Element *parent_element);
		~DynamicElementTextItem() override;
	private:
		DynamicElementTextItem(const DynamicElementTextItem &);
		
	public:
		QDomElement toXml(QDomDocument &dom_doc) const override;
		void fromXml(const QDomElement &dom_elmt) override;
		
		Element *parentElement() const;
		Element *elementUseForInfo() const;
		
		DynamicElementTextItem::TextFrom textFrom() const;
		void setTextFrom (DynamicElementTextItem::TextFrom text_from);
		QString tagg() const;
		void setTagg(const QString &tagg);
		QString text() const;
		void setText(const QString &text);
        static QString xmlTaggName() {return QString("dynamic_elmt_text");}
		void setInfoName(const QString &info_name);
		QString infoName() const;
		void setCompositeText(const QString &text);
		QString compositeText() const;
		
	protected:
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
		
	private:
		void elementInfoChanged();
		void masterChanged();
		
	private:
		QPointer <Element> m_parent_element,
						   m_master_element;
		QString m_tagg,
				m_text,
				m_info_name,
				m_composite_text;
		DynamicElementTextItem::TextFrom m_text_from = UserText;
		QUuid m_uuid;		
};

#endif // DYNAMICELEMENTTEXTITEM_H
