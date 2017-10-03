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
#include "diagramcontent.h"
#include <QGraphicsItem>
#include "element.h"
#include "independenttextitem.h"
#include "conductor.h"
#include "diagramimageitem.h"
#include "elementtextitem.h"
#include "qetshapeitem.h"
#include "dynamicelementtextitem.h"

/**
 * @brief DiagramContent::DiagramContent
 */
DiagramContent::DiagramContent() {}

/**
 * @brief DiagramContent::DiagramContent
 * Copy constructor
 * @param other
 */
DiagramContent::DiagramContent(const DiagramContent &other) :
	m_elements(other.m_elements),
	m_text_fields(other.m_text_fields),
	m_images(other.m_images),
	m_shapes(other.m_shapes),
	m_conductors_to_update(other.m_conductors_to_update),
	m_conductors_to_move(other.m_conductors_to_move),
	m_other_conductors(other.m_other_conductors),
	m_element_texts(other.m_element_texts)
{}

/**
 * @brief DiagramContent::~DiagramContent
 */
DiagramContent::~DiagramContent() {}

/**
 * @brief DiagramContent::conductors
 * @param filter
 * @return Every conductors according to the filter
 */
QList<Conductor *> DiagramContent::conductors(int filter) const
{
	QSet<Conductor *> result;
	if (filter & ConductorsToMove)   result += m_conductors_to_move;
	if (filter & ConductorsToUpdate) result += m_conductors_to_update;
	if (filter & OtherConductors)    result += m_other_conductors;
	if (filter & SelectedOnly) {
		for(Conductor *conductor : result) {
			if (!conductor->isSelected()) result.remove(conductor);
		}
	}
	return(result.toList());
}

/**
 * @brief DiagramContent::clear
 * Remove all items from the diagram content
 */
void DiagramContent::clear()
{
	m_elements.clear();
	m_text_fields.clear();
	m_images.clear();
	m_shapes.clear();
	m_conductors_to_update.clear();
	m_conductors_to_move.clear();
	m_other_conductors.clear();
	m_element_texts.clear();
}

/**
 * @brief DiagramContent::removeNonMovableItems
 * Remove all non movable item.
 * @return : return the numbers of removed item
 */
int DiagramContent::removeNonMovableItems()
{
	int count_ = 0;
	
	const QSet<Element *> elements_set = m_elements;
	for(Element *elmt : elements_set) {
		if (!elmt->isMovable()) {
			m_elements.remove(elmt);
			++count_;
		}
	}
	
	const QSet<DiagramImageItem *> images_set = m_images;
	for(DiagramImageItem *img : images_set) {
		if (!img->isMovable()) {
			m_images.remove(img);
			++count_;
		}
	}
	
	const QSet<QetShapeItem *> shapes_set = m_shapes;
	for(QetShapeItem *shape : shapes_set) {
		if (!shape->isMovable()) {
			m_shapes.remove(shape);
			++count_;
		}
	}

	return count_;
}

/**
 * @brief DiagramContent::items
 * @param filter
 * @return The items of this diagram content according to @filter
 */
QList<QGraphicsItem *> DiagramContent::items(int filter) const
{
	QList<QGraphicsItem *> items_list;
	
	for(QGraphicsItem *qgi : conductors(filter)) items_list << qgi;

	if (filter & Elements)          for(QGraphicsItem *qgi : m_elements)      items_list << qgi;
	if (filter & TextFields)        for(QGraphicsItem *qgi : m_text_fields)   items_list << qgi;
	if (filter & Images)            for(QGraphicsItem *qgi : m_images)        items_list << qgi;
	if (filter & Shapes)            for(QGraphicsItem *qgi : m_shapes)        items_list << qgi;
	if (filter & ElementTextFields) for(QGraphicsItem *qgi : m_element_texts) items_list << qgi;

	if (filter & SelectedOnly) {
		for(QGraphicsItem *qgi : items_list) {
			if (!qgi -> isSelected()) items_list.removeOne(qgi);
		}
	}
	return(items_list);
}

/**
 * @brief DiagramContent::count
 * @param filter
 * @return The number of items, according to @filter
 */
int DiagramContent::count(int filter) const
{
	int count = 0;
	if (filter & SelectedOnly) {
		if (filter & Elements)           for(Element *element :     m_elements)               { if (element   -> isSelected()) ++ count; }
		if (filter & TextFields)         for(DiagramTextItem *dti : m_text_fields)            { if (dti       -> isSelected()) ++ count; }
		if (filter & Images)             for(DiagramImageItem *dii : m_images)                { if (dii       -> isSelected()) ++ count; }
		if (filter & Shapes)             for(QetShapeItem *dsi : m_shapes)                    { if (dsi       -> isSelected()) ++ count; }
		if (filter & ConductorsToMove)   for(Conductor *conductor : m_conductors_to_move)     { if (conductor -> isSelected()) ++ count; }
		if (filter & ConductorsToUpdate) for(Conductor *conductor : m_conductors_to_update)   { if (conductor -> isSelected()) ++ count; }
		if (filter & OtherConductors)    for(Conductor *conductor : m_other_conductors)       { if (conductor -> isSelected()) ++ count; }
		if (filter & ElementTextFields)   for(DynamicElementTextItem *deti : m_element_texts) { if (deti      -> isSelected()) ++ count; }
	}
	else {
		if (filter & Elements)           count += m_elements.count();
		if (filter & TextFields)         count += m_text_fields.count();
		if (filter & Images)             count += m_images.count();
		if (filter & Shapes)             count += m_shapes.count();
		if (filter & ConductorsToMove)   count += m_conductors_to_move.count();
		if (filter & ConductorsToUpdate) count += m_conductors_to_update.count();
		if (filter & OtherConductors)    count += m_other_conductors.count();
		if (filter & ElementTextFields)  count += m_element_texts.count();
	}
	return(count);
}

/**
 * @brief DiagramContent::sentence
 * @param filter
 * @return A string that describe the items of the diagram content according to @filter.
 * Exemple : X elements, Y conductors etc....
 */
QString DiagramContent::sentence(int filter) const
{
	int elements_count   = (filter & Elements) ? m_elements.count() : 0;
	int conductors_count = conductors(filter).count();
	int textfields_count = (filter & TextFields) ? (m_text_fields.count()) : 0;
	int images_count	 = (filter & Images) ? m_images.count() : 0;
	int shapes_count	 = (filter & Shapes) ? m_shapes.count() : 0;
	int elmt_text_count  = (filter & ElementTextFields) ? m_element_texts.count() : 0;
	
	return(
		QET::ElementsAndConductorsSentence(
			elements_count,
			conductors_count,
			textfields_count,
			images_count,
			shapes_count,
			elmt_text_count		
		)
	);
}

/**
 * @brief operator << Use to debug a diagram content
 * @param d : QDebug to use for display the debug info
 * @param content : content to debug
 * @return 
 */
QDebug &operator<<(QDebug d, DiagramContent &content) {
	Q_UNUSED(content);
	d << "DiagramContent {" << "\n";
	/*
	FIXME Le double-heritage QObject / QGraphicsItem a casse cet operateur
	d << "  elements :" << c.elements << "\n";
	d << "  conductorsToUpdate :" << c.conductorsToUpdate << "\n";
	d << "  conductorsToMove :" << c.conductorsToMove << "\n";
	d << "  otherConductors :" << c.otherConductors << "\n";
	*/
	d << "}";
	return(d.space());
}
