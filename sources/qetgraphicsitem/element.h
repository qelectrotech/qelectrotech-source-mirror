﻿/*
	Copyright 2006-2021 The QElectroTech Team
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
#ifndef ELEMENT_H
#define ELEMENT_H

#include "../ElementsCollection/elementslocation.h"
#include "../NameList/nameslist.h"
#include "../autoNum/assignvariables.h"
#include "../diagramcontext.h"
#include "../qet.h"
#include "qetgraphicsitem.h"
#include "../properties/elementdata.h"

#include <QHash>
#include <QPicture>
#include <algorithm>

class QETProject;
class Terminal;
class Conductor;
class DynamicElementTextItem;
class ElementTextItemGroup;

/**
	This is the base class for electrical elements.
*/
class Element : public QetGraphicsItem
{
	friend class DiagramEventAddElement;

	Q_OBJECT
	public:
			/**
				@brief The kind enum
				Used to know the kind of this element
				(master, slave, report ect...)
			*/
		enum kind {
			Simple = 1,
			NextReport = 2,
			PreviousReport = 4,
			AllReport = 6,
			Master = 8,
			Slave = 16,
			Terminale = 32,
			Thumbnail = 64};

		Element(const ElementsLocation &location,
			QGraphicsItem * = nullptr,
			int *state = nullptr,
			Element::kind link_type = Element::Simple);
		~Element() override;
	private:
		Element(const Element &);

		// attributes
	public:
			/**
				Enable the use of qgraphicsitem_cast
				to safely cast a QGraphicsItem into an Element.
				@return the QGraphicsItem type
			*/
		enum { Type = UserType + 1000 };
		int type() const override { return Type; }

	signals:
		void linkedElementChanged(); //This signal is emitted when the linked elements with this element change
		void elementInfoChange(
				DiagramContext old_info,
				DiagramContext new_info);
		void textAdded(DynamicElementTextItem *deti);
		void textRemoved(DynamicElementTextItem *deti);
		void textsGroupAdded(ElementTextItemGroup *group);
		void textsGroupAboutToBeRemoved(ElementTextItemGroup *group);
		void textAddedToGroup(
				DynamicElementTextItem *text,
				ElementTextItemGroup *group);
		void textRemovedFromGroup(
				DynamicElementTextItem *text,
				ElementTextItemGroup *group);


	public:
		QList<Terminal *> terminals() const;
		QList<Conductor *> conductors() const;
		QList<QPair<Terminal *,Terminal *>> AlignedFreeTerminals() const;

			//METHODS related to information
		DiagramContext elementInformations()const
		{return m_data.m_informations;}
		virtual void setElementInformations(DiagramContext dc);

		ElementData elementData() const;
		void setElementData (ElementData data);

		/**
		 * @brief kindInformations
		 * @deprecated
		 * use elementData function instead
		 * @return
		 */
		DiagramContext kindInformations() const
		{return m_kind_informations;}
		//@kind_information_ is used to store more information
		//about the herited class like contactelement for know
		// kind of contact (simple tempo) or number of contact show by the element.

		autonum::sequentialNumbers sequenceStruct() const
		{return m_autoNum_seq;}
		autonum::sequentialNumbers& rSequenceStruct()
		{return m_autoNum_seq;}
		void setUpFormula(bool code_letter = true);
		void setPrefix(QString);
		QString getPrefix() const;
		void freezeLabel(bool freeze);
		bool isFreezeLabel() const {return m_freeze_label;}
		void freezeNewAddedElement();
		QString actualLabel();

		QString name() const override;
		ElementsLocation location() const;
		virtual void setHighlighted(bool);
		void displayHelpLine(bool b = true);
		QSize size() const;
		QPixmap pixmap();
		QPoint setHotspot(QPoint);
		QPoint hotspot() const;
		void editProperty() override;
		static bool valideXml(QDomElement &);
		virtual bool fromXml(
				QDomElement &,
				QHash<int,
				Terminal *> &);
		virtual QDomElement toXml(
				QDomDocument &,
				QHash<Terminal *,
				int> &) const;
		QUuid uuid() const;
		int orientation() const;

			//METHODS related to texts
		void addDynamicTextItem(DynamicElementTextItem *deti = nullptr);
		void removeDynamicTextItem(DynamicElementTextItem *deti);
		QList<DynamicElementTextItem *> dynamicTextItems() const;
		ElementTextItemGroup *addTextGroup(const QString &name);
		void addTextGroup(ElementTextItemGroup *group);
		void removeTextGroup(ElementTextItemGroup *group);
		ElementTextItemGroup *textGroup(const QString &name) const;
		QList<ElementTextItemGroup *> textGroups() const;
		bool addTextToGroup(
				DynamicElementTextItem *text,
				ElementTextItemGroup *group);
		bool removeTextFromGroup(
				DynamicElementTextItem *text,
				ElementTextItemGroup *group);

			//METHODS related to linked element
		bool isFree() const;
		virtual void linkToElement(Element *) {}
		virtual void unlinkAllElements() {}
		virtual void unlinkElement(Element *) {}
		virtual void initLink(QETProject *);
		QList<Element *> linkedElements ();

		/**
		 * @brief linkType
		 * use elementData function instead
		 * @return
		 */
		virtual kind linkType() const {return m_link_type;} // return the linkable type
		/**
		 * @brief linkTypeToString
		 * use elementData function instead
		 * @return
		 */
		QString linkTypeToString() const;

		void newUuid() {m_uuid = QUuid::createUuid();} 	//create new uuid for this element

	protected:
		void drawAxes(QPainter *, const QStyleOptionGraphicsItem *);
		void setSize(int, int);

	private:
		void drawSelection(
				QPainter *,
				const QStyleOptionGraphicsItem *);
		void drawHighlight(
				QPainter *,
				const QStyleOptionGraphicsItem *);
		bool buildFromXml(const QDomElement &, int * = nullptr);
		bool parseElement(const QDomElement &dom);
		bool parseInput(const QDomElement &dom_element);
		DynamicElementTextItem *parseDynamicText(
				const QDomElement &dom_element);
		Terminal *parseTerminal(const QDomElement &dom_element);

		//Reimplemented from QGraphicsItem
	public:
		void paint(
				QPainter *,
				const QStyleOptionGraphicsItem *,
				QWidget *) override;
		QRectF boundingRect() const override;
	protected:
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(
				QGraphicsSceneMouseEvent *event) override;
		void hoverEnterEvent(QGraphicsSceneHoverEvent *) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override;

	protected:
			//ATTRIBUTES related to linked element
		QList <Element *> connected_elements;
		QList <QUuid>     tmp_uuids_link;
		QUuid             m_uuid;
		kind              m_link_type = Element::Simple;

			//ATTRIBUTES related to informations
		DiagramContext m_kind_informations;
		autonum::sequentialNumbers m_autoNum_seq;
		bool m_freeze_label = false;
		QString m_F_str;

		ElementsLocation m_location;
		QList <Terminal *> m_terminals;
		const QPicture m_picture;
		const QPicture m_low_zoom_picture;
		ElementData m_data;

	private:
		bool m_must_highlight = false;
		QSize   dimensions;
		QPoint  hotspot_coord;
		bool m_mouse_over = false;
		QString m_prefix;
		QList <DynamicElementTextItem *> m_dynamic_text_list;
		QList <ElementTextItemGroup *> m_texts_group;

};

bool comparPos(const Element * elmt1, const Element * elmt2);

inline bool Element::isFree() const
{
	return (connected_elements.isEmpty());
}

/**
	Indicate the current orientation of this element
	O = 0°
	1 = 90°
	2 = 180°
	3 = 270°
	@return the current orientation of this element
*/
inline int Element::orientation() const
{
	return(QET::correctAngle(rotation())/90);
}

/**
	@brief Element::uuid
	@return the uuid of this element
*/
inline QUuid Element::uuid() const
{return m_uuid;}

/**
	@brief Element::linkedElements
	@return the list of linked elements, the list is sorted by position
*/
inline QList <Element *> Element::linkedElements()
{
	 std::sort(
				 connected_elements.begin(),
				 connected_elements.end(),
				 comparPos);
	return connected_elements;
}

#endif
