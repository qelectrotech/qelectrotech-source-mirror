/*
	Copyright 2006-2026 The QElectroTech Team
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
#ifndef INDEPENDENT_TEXT_ITEM_H
#define INDEPENDENT_TEXT_ITEM_H

#include "diagramtextitem.h"

#include <QUuid>

/**
	This class represents an independent text field on a particular diagram.
	It may be moved, edited, and rotated.
*/
class IndependentTextItem : public DiagramTextItem
{
	Q_OBJECT

	Q_PROPERTY(qreal textWidth READ textWidth WRITE setTextWidth NOTIFY textWidthChanged)
		
		// constructors, destructor
	signals:
		void uuidChanged();
		void textWidthChanged(qreal width);

	public:
		IndependentTextItem();
		IndependentTextItem(const QString &);
		~IndependentTextItem() override;
	
		// attributes
	public:
		enum { Type = UserType + 1005 };
		int type() const override { return Type; }
		
		void fromXml(const QDomElement &) override;
		QDomElement toXml(QDomDocument &) const override;
		QUuid uuid() const {return m_uuid;}
		void setUuid(const QUuid &uuid) {m_uuid = uuid; emit uuidChanged();}
		void newUuid() {setUuid(QUuid::createUuid());}	//create new uuid for this item

			//Hide QGraphicsTextItem::textWidth()/setTextWidth(), which are not
			//virtual: called through a QGraphicsTextItem or DiagramTextItem
			//pointer they would change the document only, and the width would
			//be neither saved nor shown in the properties. Use these, or the
			//"textWidth" property.
		qreal textWidth() const {return m_text_width;}
		void setTextWidth(qreal width);
		
	protected:
		void focusOutEvent(QFocusEvent *event) override;
		bool textResizeHandlesWanted() const override;
		bool hasUserTextWidth() const override {return m_text_width > 0;}

	private:
		void wrapAtWords();

		QUuid m_uuid = QUuid::createUuid();
		qreal m_text_width = -1;
};
#endif
