/*
	Copyright 2006-2016 The QElectroTech Team
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

#include "qet.h"
#include "qetgraphicsitem.h"
#include "diagramcontext.h"

class ElementTextItem;
class QETProject;
class Terminal;
class Conductor;

/**
	This is the base class for electrical elements.
*/
class Element : public QetGraphicsItem {	
	Q_OBJECT
	
		// constructors, destructor
	public:
		Element(QGraphicsItem * = 0);
		virtual ~Element();
	private:
		Element(const Element &);
	
		// attributes
	public:
			/**
			 * Enable the use of qgraphicsitem_cast to safely cast
			 * a QGraphicsItem into an Element.
			 * @return the QGraphicsItem type
			 */
		enum { Type = UserType + 1000 };
		virtual int type() const { return Type; }

			/**
			 * @brief The kind enum
			 * Used to know the kind of this element (master, slave, report ect...)
			 */
		enum kind {Simple = 1,
				   NextReport = 2,
				   PreviousReport = 4,
				   AllReport = 6,
				   Master = 8,
				   Slave = 16,
				   Terminale = 32};
	
	private:
		QSize   dimensions;
		QPoint  hotspot_coord;
		QPixmap preview;

		// methods
	public:
		/// @return the list of terminals for this element
	virtual QList<Terminal *> terminals() const = 0;
		/// @return the list of conductors attached to this element
	virtual QList<Conductor *> conductors() const = 0;
		/// @return the list of text items attached to this element
	virtual QList<ElementTextItem *> texts() const = 0;
		/// @return the text field tagged with @tagg or NULL if text field isn't found
	virtual ElementTextItem* taggedText(const QString &tagg) const = 0;
		/// @return the list of lines items in this element
	virtual QList<QLineF *> lines() const = 0;
		/// @return the list of rectangles items in this element
	virtual QList<QRectF *> rectangles() const = 0;
		/// @return the list of bounding rectangles for circles items in this element
	virtual QList<QRectF *> circles() const = 0;
		/// @return the list of polygons in this element
	virtual QList<QVector<QPointF> *> polygons() const = 0;	
		/// @return the list of arcs in this element
	virtual QList<QVector<qreal> *> arcs() const = 0;
		/// @return the current number of terminals of this element
	virtual int terminalsCount() const = 0;
		/// @return the minimum number of terminals for this element
	virtual int minTerminalsCount() const = 0;
		/// @return the maximum number of terminals for this element
	virtual int maxTerminalsCount() const = 0;

		QList <QPair <Terminal *, Terminal *> > AlignedFreeTerminals () const;

	/**
	 *related method and attributes,
	 *about none graphic thing
	 *like the linked element or information about this element
	 */
		//METHODS related to linked element
	public:
				bool isFree            () const;
		virtual void linkToElement     (Element *) {}
		virtual void unlinkAllElements () {}
		virtual void unlinkElement     (Element *) {}
		virtual void initLink          (QETProject *);
		QList<Element *> linkedElements ();
		virtual kind linkType() const {return link_type_;} // @return the linkable type
		void newUuid() {uuid_ = QUuid::createUuid();} 	//create new uuid for this element

		//ATTRIBUTES related to linked element
	protected:
		QList <Element *> connected_elements;
		QList <QUuid>     tmp_uuids_link;
		QUuid             uuid_;
		kind              link_type_;

	signals:
		void linkedElementChanged(); //This signal is emited when the linked elements with this element change
		void elementInfoChange(DiagramContext old_info, DiagramContext new_info);
		void updateLabel(); //This signal is emited to update element's label

		//METHODS related to information
	public:
		DiagramContext  elementInformations    ()const              {return m_element_informations;}
		DiagramContext& rElementInformations   ()                   {return m_element_informations;}
		virtual void    setElementInformations (DiagramContext dc);
		DiagramContext  kindInformations       () const             {return kind_informations_;}	//@kind_information_ is used to store more information
																									//about the herited class like contactelement for know
																									// kind of contact (simple tempo) or number of contact show by the element.
		QString assignVariables (QString, Element *);

	//ATTRIBUTES
	protected:
		DiagramContext m_element_informations, kind_informations_;

	/**
		Draw this element
	*/
	public:
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *) = 0;
		/// @return This element type ID
		virtual QString typeId() const = 0;
		/// @return the human name for this element
		virtual QString name() const = 0;
	
		virtual bool isHighlighted() const;
		virtual void setHighlighted(bool);
		void displayHelpLine(bool b = true);
		void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
		QRectF boundingRect() const;
		QSize setSize(int, int);
		QSize size() const;
		QPixmap pixmap();
	
	// methods related to the hotspot
	QPoint setHotspot(QPoint);
	QPoint hotspot() const;
	
	// selection-related methods
	void select();
	void deselect();
	
	virtual void rotateBy(const qreal &);
	virtual void editProperty();
	
	// methods related to XML import/export
	static bool valideXml(QDomElement &);
	virtual bool fromXml(QDomElement &, QHash<int, Terminal *> &, bool = false);
	virtual QDomElement toXml(QDomDocument &, QHash<Terminal *, int> &) const;
	QUuid uuid() const;
	
	// orientation-related methods
	int orientation() const;
	
	protected:
		void drawAxes(QPainter *, const QStyleOptionGraphicsItem *);
	
	private:
		bool must_highlight_;
		void drawSelection(QPainter *, const QStyleOptionGraphicsItem *);
		void drawHighlight(QPainter *, const QStyleOptionGraphicsItem *);
		void updatePixmap();

	protected:
		virtual void mouseMoveEvent    ( QGraphicsSceneMouseEvent *event );
		virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent *event );
		virtual void hoverEnterEvent   ( QGraphicsSceneHoverEvent * );
		virtual void hoverLeaveEvent   ( QGraphicsSceneHoverEvent * );

	private:
		bool m_mouse_over;

};

bool comparPos(const Element * elmt1, const Element * elmt2);

inline bool Element::isFree() const {
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
inline int Element::orientation() const {
	return(QET::correctAngle(rotation())/90);
}

/**
 * @brief Element::uuid
 * @return the uuid of this element
 */
inline QUuid Element::uuid() const {
	return uuid_;
}

/**
 * @brief Element::linkedElements
 * @return the list of linked elements, the list is sorted by position
 */
inline QList <Element *> Element::linkedElements() {
	 qSort(connected_elements.begin(), connected_elements.end(), comparPos);
	return connected_elements;
}

#endif
