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
#include "dynamicelementtextitem.h"
#include "qet.h"
#include "element.h"
#include "qetapp.h"
#include "diagram.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

#include <QDomDocument>
#include <QDomElement>
#include <QGraphicsSceneMouseEvent>

/**
 * @brief DynamicElementTextItem::DynamicElementTextItem
 * Constructor
 * @param parent_element
 */
DynamicElementTextItem::DynamicElementTextItem(Element *parent_element) :
	m_parent_element(parent_element),
	m_uuid(QUuid::createUuid())
{
	setFont(QETApp::diagramTextsFont(9));
	setText(tr("Texte"));
    setParentItem(parent_element);
	
	connect(this, &DynamicElementTextItem::textEdited, [this](const QString &old_str, const QString &new_str)
	{
		if(this->m_parent_element && this->m_parent_element->diagram())
		{
			QUndoCommand *undo = new QPropertyUndoCommand(this, "text", old_str, new_str);
			undo->setText(tr("Éditer un texte d'élément"));
			this->m_parent_element->diagram()->undoStack().push(undo);
		}
		
	});
}

DynamicElementTextItem::~DynamicElementTextItem()
{}

/**
 * @brief DynamicElementTextItem::toXml
 * Export this text to xml
 * @param dom_doc
 * @return 
 */
QDomElement DynamicElementTextItem::toXml(QDomDocument &dom_doc) const
{
	QDomElement root_element = dom_doc.createElement(xmlTaggName());
	
	root_element.setAttribute("x", QString::number(pos().x()));
	root_element.setAttribute("y", QString::number(pos().y()));
	root_element.setAttribute("rotation", QString::number(QET::correctAngle(rotation())));
	root_element.setAttribute("font_size", font().pointSize());
	root_element.setAttribute("uuid", m_uuid.toString());
	
	QMetaEnum me = metaObject()->enumerator(metaObject()->indexOfEnumerator("TextFrom"));
	root_element.setAttribute("text_from", me.valueToKey(m_text_from));
	
    QDomElement dom_text = dom_doc.createElement("text");
    dom_text.appendChild(dom_doc.createTextNode(toPlainText()));
    root_element.appendChild(dom_text);
    
		//tagg
	if (!m_tagg.isEmpty())
	{
		QDomElement dom_tagg = dom_doc.createElement("tagg");
		dom_tagg.appendChild(dom_doc.createTextNode(m_tagg));
		root_element.appendChild(dom_tagg);
	}
	
		//Color
	if(color() != QColor(Qt::black))
	{
		QDomElement dom_color = dom_doc.createElement("color");
		dom_color.appendChild(dom_doc.createTextNode(color().name()));
		root_element.appendChild(dom_color);
	}
    
    return root_element;
}

/**
 * @brief DynamicElementTextItem::fromXml
 * Import this text from xml
 * @param dom_elmt
 */
void DynamicElementTextItem::fromXml(const QDomElement &dom_elmt)
{
	if (dom_elmt.tagName() != xmlTaggName()) {
		qDebug() << "DynamicElementTextItem::fromXml : Wrong tagg name";
		return;
	}
	
	QGraphicsTextItem::setPos(dom_elmt.attribute("x", QString::number(0)).toDouble(),
							  dom_elmt.attribute("y", QString::number(0)).toDouble());
	QGraphicsTextItem::setRotation(dom_elmt.attribute("rotation", QString::number(0)).toDouble());
	setFont(QETApp::diagramTextsFont(dom_elmt.attribute("font_size", QString::number(9)).toInt()));
	m_uuid = QUuid(dom_elmt.attribute("uuid", QUuid::createUuid().toString()));
	
	QMetaEnum me = metaObject()->enumerator(metaObject()->indexOfEnumerator("TextFrom"));
	m_text_from = DynamicElementTextItem::TextFrom(me.keyToValue(dom_elmt.attribute("text_from").toStdString().data()));
	setNoEditable(m_text_from == ElementInfo? true : false);

		//Text
    QDomElement dom_text = dom_elmt.firstChildElement("text");
	if (!dom_text.isNull())
        setPlainText(dom_text.text());
    
		//tagg
    QDomElement dom_tagg = dom_elmt.firstChildElement("tagg");
	if (!dom_tagg.isNull())
		m_tagg = dom_tagg.text();

		//Color
	QDomElement dom_color = dom_elmt.firstChildElement("color");
	if(!dom_color.isNull())
		setColor(QColor(dom_color.text()));
}

/**
 * @brief DynamicElementTextItem::ParentElement
 * @return a pointer to the parent element. Note the pointer can be null.
 */
Element *DynamicElementTextItem::ParentElement() const {
	return m_parent_element;
}

/**
 * @brief DynamicElementTextItem::textFrom
 * @return what the final text is created from.
 */
DynamicElementTextItem::TextFrom DynamicElementTextItem::textFrom() const {
	return m_text_from;
}

/**
 * @brief DynamicElementTextItem::setTextFrom
 * Set the final text is created from.
 * @param text_from
 */
void DynamicElementTextItem::setTextFrom(DynamicElementTextItem::TextFrom text_from)
{
	m_text_from = text_from;
	setNoEditable(m_text_from == ElementInfo? true : false);
	emit TextFromChanged(m_text_from);
}

/**
 * @brief DynamicElementTextItem::tagg
 * @return the tagg of this text
 */
QString DynamicElementTextItem::tagg() const {
	return m_tagg;
}

/**
 * @brief DynamicElementTextItem::setTagg
 * set the taggof this text
 * @param tagg
 */
void DynamicElementTextItem::setTagg(const QString &tagg)
{
	m_tagg = tagg;
	emit taggChanged(m_tagg);
}

/**
 * @brief DynamicElementTextItem::text
 * @return the text of this text
 */
QString DynamicElementTextItem::text() const {
	return m_text;
}

/**
 * @brief DynamicElementTextItem::setText
 * Set the text of this text
 * @param formula
 */
void DynamicElementTextItem::setText(const QString &text)
{
	m_text = text;
	setPlainText(m_text);
	emit textChanged(m_text);
}

/**
 * @brief DynamicElementTextItem::mouseMoveEvent
 * @param event
 */
void DynamicElementTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if(event->buttons() & Qt::LeftButton)
	{
		QPointF old_pos = pos(); //The old pos
		QPointF movement = event->pos() - event->buttonDownPos(Qt::LeftButton); //The movement since the button down pos
		QPointF new_pos = pos() + mapMovementToParent(movement); //The new pos with this event
		event->modifiers() == Qt::ControlModifier ? setPos(new_pos) : setPos(Diagram::snapToGrid(new_pos));
		
		if(m_parent_element && m_parent_element->diagram())
		{
			Diagram *diagram = m_parent_element->diagram();
			
			if(m_first_move)
			{
				if(diagram->beginMoveElementTexts(this) == 1)
					m_parent_element->setHighlighted(true);
			}
			
				//Because setPos() can be snaped to grid or not, we calcule the real movement
			QPointF effective_movement = pos() - old_pos;
			QPointF scene_effective_movement = mapMovementToScene(mapMovementFromParent(effective_movement));
			diagram->continueMoveElementTexts(scene_effective_movement);
		}
	}
	else
		event->ignore();
		
	if(m_first_move)
		m_first_move = false;
}

/**
 * @brief DynamicElementTextItem::mouseReleaseEvent
 * @param event
 */
void DynamicElementTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_parent_element)
		m_parent_element->setHighlighted(false);
	
	if(m_parent_element && m_parent_element->diagram())
		m_parent_element->diagram()->endMoveElementTexts();
	
	if(!(event->modifiers() & Qt::ControlModifier))
		QGraphicsTextItem::mouseReleaseEvent(event);
}

