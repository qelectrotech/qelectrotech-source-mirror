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
#include "terminal.h"
#include "conductor.h"

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

		//If the parent is slave, we keep aware about the changement of master.
	if(parent_element->linkType() == Element::Slave)
	{
		connect(parent_element, &Element::linkedElementChanged, this, &DynamicElementTextItem::masterChanged);
			//The parent is already linked, wa call master changed for init the connection
		if(!parent_element->linkedElements().isEmpty())
			masterChanged();
	}
	if(parent_element->linkType() & Element::AllReport)
	{
			//Get the report formula, and add connection to keep up to date the formula.
		if (parent_element->diagram() && parent_element->diagram()->project())
		{
			m_report_formula = parent_element->diagram()->project()->defaultReportProperties();
			m_report_formula_con = connect(parent_element->diagram()->project(), &QETProject::reportPropertiesChanged, this, &DynamicElementTextItem::reportFormulaChanged);
		}

			//Add connection to keep up to date the status of the element linked to the parent folio report of this text.
		connect(parent_element, &Element::linkedElementChanged, this, &DynamicElementTextItem::reportChanged);
			//The parent is already linked, we call reportChanged for init the connection
		if(!parent_element->linkedElements().isEmpty())
			reportChanged();
		
			//Add connection to keep up date the conductors added or removed to the parent folio report element
		connect(parent_element->terminals().first(), &Terminal::conductorWasAdded, this, &DynamicElementTextItem::conductorWasAdded);
		connect(parent_element->terminals().first(), &Terminal::conductorWasRemoved, this, &DynamicElementTextItem::conductorWasRemoved);
			//Get a conductor in the potential
		setPotentialConductor();
	}
}

DynamicElementTextItem::~DynamicElementTextItem()
{}

/**
 * @brief DynamicElementTextItem::textFromMetaEnum
 * @return The QMetaEnum of the enum TextFrom 
 */
QMetaEnum DynamicElementTextItem::textFromMetaEnum()
{
	DynamicElementTextItem deti;
	return deti.metaObject()->enumerator(deti.metaObject()->indexOfEnumerator("TextFrom"));
}

DynamicElementTextItem::DynamicElementTextItem()
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
	
	QMetaEnum me = textFromMetaEnum();
	root_element.setAttribute("text_from", me.valueToKey(m_text_from));
	
    QDomElement dom_text = dom_doc.createElement("text");
    dom_text.appendChild(dom_doc.createTextNode(toPlainText()));
    root_element.appendChild(dom_text);
	
		//Info name
	if(!m_info_name.isEmpty())
	{
		QDomElement dom_info_name = dom_doc.createElement("info_name");
		dom_info_name.appendChild(dom_doc.createTextNode(m_info_name));
		root_element.appendChild(dom_info_name);
	}
	
		//Composite text
	if(!m_composite_text.isEmpty())
	{
		QDomElement dom_comp_text = dom_doc.createElement("composite_text");
		dom_comp_text.appendChild(dom_doc.createTextNode(m_composite_text));
		root_element.appendChild(dom_comp_text);
	}
    
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
	
	QMetaEnum me = textFromMetaEnum();
	m_text_from = DynamicElementTextItem::TextFrom(me.keyToValue(dom_elmt.attribute("text_from").toStdString().data()));
	if(m_text_from == ElementInfo || m_text_from == CompositeText)
	{
		setNoEditable(true);
			//We don't made the connection below if the parent element is a report, because report haven't got info.
		if (!(parentElement()->linkType() & Element::AllReport) && elementUseForInfo()) 
			connect(elementUseForInfo(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
	}
	else {
		setNoEditable(false);
	}

		//Text
    QDomElement dom_text = dom_elmt.firstChildElement("text");
	if (!dom_text.isNull())
	{
		m_text = dom_text.text();
		setPlainText(m_text);
	}
	
		//Info name
	QDomElement dom_info_name = dom_elmt.firstChildElement("info_name");
	if(!dom_info_name.isNull())
		m_info_name = dom_info_name.text();
	
		//Composite text
	QDomElement dom_comp_text = dom_elmt.firstChildElement("composite_text");
	if(!dom_comp_text.isNull())
		m_composite_text = dom_comp_text.text();
    
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
Element *DynamicElementTextItem::parentElement() const {
	return m_parent_element;
}

/**
 * @brief DynamicElementTextItem::elementUseForInfo
 * @return a pointer to the element we must use for the variable information.
 * If this text is owned by a simple element, the simple element is returned, this is the same element returned by the function parentElement().
 * If this text is owned by a master element, the master element is returned, this is the same element returned by the function parentElement().
 * If this text is owned by a report element, the report element is returned, this is the same element returned by the function parentElement().
 * If this text is owned by a terminal element, the terminal element is returned, this is the same element returned by the function parentElement().
 * If this text is owned by a slave element, we return the master element set as master of the parent slave element,
 * if the parent slave is not linked to a master, this function return a nullptr.
 * If this text have no parent element, return nullptr
 */
Element *DynamicElementTextItem::elementUseForInfo() const
{
	Element *elmt = parentElement();
	if(!elmt)
		return nullptr;
	
	switch (elmt->linkType())
	{
		case Element::Simple:
			return elmt;
		case Element::NextReport:
			return elmt;
		case Element::PreviousReport:
			return elmt;
		case Element::Master:
			return elmt;
		case Element::Slave:
		{
			if(elmt->linkedElements().isEmpty())
				return nullptr;
			else
				return elmt->linkedElements().first();
		}
		case Element::Terminale:
			return elmt;
		default:
			return elmt;
	}
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
	setNoEditable(text_from == UserText? false : true);
	
	if(text_from == UserText)
	{
		setPlainText(m_text);
		disconnect(m_parent_element.data(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
	}
	else if (text_from == ElementInfo && elementUseForInfo())
	{
		setPlainText(elementUseForInfo()->elementInformations().value(m_info_name).toString());
		
		if(m_text_from == UserText)
			connect(elementUseForInfo(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
	}
	else if (text_from == CompositeText && elementUseForInfo())
	{
		setPlainText(autonum::AssignVariables::replaceVariable(m_composite_text, elementUseForInfo()->elementInformations()));
		if(m_text_from == UserText)
			connect(elementUseForInfo(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
	}
	
	if(m_text_from != text_from)
	{
		m_text_from = text_from;
		emit textFromChanged(m_text_from);
	}
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
 * @brief DynamicElementTextItem::setInfoName
 * Set the information name of the parent element.
 * @param info_name
 */
void DynamicElementTextItem::setInfoName(const QString &info_name)
{
	QString old_info_name = m_info_name;
	m_info_name = info_name;

	if (m_parent_element && (m_parent_element.data()->linkType() & Element::AllReport)) //special treatment for report
	{
		if(old_info_name != info_name)
		{
			if(old_info_name == "label") {
				removeConnectionForReportFormula(m_report_formula);
			}
			if(info_name == "label")
			{
				setConnectionForReportFormula(m_report_formula);
				updateReportText();
			}
			else
				conductorPropertiesChanged();
		}
	}
	else if(elementUseForInfo()) {
		setPlainText(elementUseForInfo()->elementInformations().value(info_name).toString());
	}
	
	emit infoNameChanged(info_name);
}

/**
 * @brief DynamicElementTextItem::infoName
 * @return the info name of this text
 */
QString DynamicElementTextItem::infoName() const {
	return m_info_name;
}

/**
 * @brief DynamicElementTextItem::setCompositeText
 * Set the composite text of this text item to @text
 * @param text
 */
void DynamicElementTextItem::setCompositeText(const QString &text)
{
	QString old_composite_text = m_composite_text;
	m_composite_text = text;

	if (m_parent_element && (m_parent_element.data()->linkType() & Element::AllReport)) //special treatment for report
	{
			/*
			 * May be in some case the old and new composite text have both the var %{label},
			 * and so we don't have to remove connection and after set conection,
			 * but for that we must to do several check and because I'm lazy,
			 * in every case I remove connection and set it after ;)
			 */
		if(old_composite_text.contains("%{label}"))
			removeConnectionForReportFormula(m_report_formula);
		if(m_composite_text.contains("%{label}"))
			setConnectionForReportFormula(m_report_formula);
		
		updateReportText();
	}
	else
	{
		DiagramContext dc;
		if(elementUseForInfo())
			dc = elementUseForInfo()->elementInformations();
		setPlainText(autonum::AssignVariables::replaceVariable(m_composite_text, dc));
	}
	
	emit compositeTextChanged(m_composite_text);
}

/**
 * @brief DynamicElementTextItem::compositeText
 * @return 
 */
QString DynamicElementTextItem::compositeText() const
{
	return m_composite_text;
}

/**
 * @brief DynamicElementTextItem::mousePressEvent
 * @param event
 */
void DynamicElementTextItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
		//The text become selected, we set the real color, otherwise the editor will display the color of text as blue,
		//but this is not the color set by the user.
	if(m_user_color.isValid())
	{
		setDefaultTextColor(m_user_color);
		m_user_color = QColor(); //m_user_color is now invalid
	}
	
	DiagramTextItem::mousePressEvent(event);
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

/**
 * @brief DynamicElementTextItem::mouseDoubleClickEvent
 * Reimplemented functions, for add extra feature when this text is owned by a slave.
 * In this case if the parent slave element is linked to a master, and this text display the label of the master
 * (both if the 'text from' is 'element info' or 'composite text') the QGraphicsView go to master and select it.
 * @param event
 */
void DynamicElementTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	DiagramTextItem::mouseDoubleClickEvent(event);
	
	if(!parentElement())
		return;
	
	Element *zoomed_element = nullptr;
	
	if(parentElement()->linkType() == Element::Slave && m_master_element)
	{
		if ((m_text_from == ElementInfo && m_info_name == "label") ||
			(m_text_from == CompositeText && m_composite_text.contains("%{label}")))
			zoomed_element = m_master_element.data();
	}
	if((parentElement()->linkType() & Element::AllReport) && m_other_report)
	{
		if((m_text_from == ElementInfo && m_info_name == "label") ||
		   (m_text_from == CompositeText && m_composite_text.contains("%{label}")))
			zoomed_element = m_other_report.data();
	}
	
	if(zoomed_element)
	{
			//Unselect and ungrab mouse to prevent unwanted
			//move when linked element is in the same scene of this.
		setSelected(false);
		ungrabMouse();
		
		if(scene() != zoomed_element->scene())
			zoomed_element->diagram()->showMe();
		zoomed_element->setSelected(true);
		
			//Zoom to the element
		for(QGraphicsView *view : zoomed_element->scene()->views())
		{
			QRectF fit = zoomed_element->sceneBoundingRect();
			fit.adjust(-200, -200, 200, 200);
			view->fitInView(fit, Qt::KeepAspectRatioByExpanding);
		}
	}
}

/**
 * @brief DynamicElementTextItem::hoverEnterEvent
 * If the parent element of this text is a folio report or a slave element, the element is linked
 * and the text display the variable "label" we set the text blue for signal the user that the text act like
 * a link when we double click on.
 * @param event
 */
void DynamicElementTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	DiagramTextItem::hoverEnterEvent(event);
	
		//If the text is selected we set the real color, otherwise the editor will display the color of text as blue,
		//but this is not the color set by the user.
	if(isSelected())
		return;
	
	Element::kind type = parentElement()->linkType();
	bool blue = false;
	
	if ((type & Element::AllReport) && m_other_report)
	{
		if( (m_text_from == ElementInfo && m_info_name == "label") ||
			(m_text_from == CompositeText && m_composite_text.contains("%{label}")) )
			blue = true;
	}
	else if (type == Element::Slave && m_master_element)
	{
		if( (m_text_from == ElementInfo && m_info_name == "label") ||
			(m_text_from == CompositeText && m_composite_text.contains("%{label}")) )
			blue = true;
	}

	if(blue)
	{
		m_user_color = color();
		setDefaultTextColor(Qt::blue);
	}
}

/**
 * @brief DynamicElementTextItem::hoverLeaveEvent
 * @param event
 */
void DynamicElementTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	DiagramTextItem::hoverLeaveEvent(event);
	
	if(m_user_color.isValid())
	{
		setDefaultTextColor(m_user_color);
		m_user_color = QColor(); //m_user_color is now invalid
	}
	
}

void DynamicElementTextItem::elementInfoChanged()
{
	DiagramContext dc;
	if(elementUseForInfo())
		dc = elementUseForInfo()->elementInformations();
	
	QString final_text;

	if (m_text_from == ElementInfo)
		final_text = dc.value(m_info_name).toString();
	else if (m_text_from == CompositeText)
		final_text = autonum::AssignVariables::replaceVariable(m_composite_text, dc);
	else if (m_text_from  == UserText)
		final_text = m_text;
	
	setPlainText(final_text);
}

/**
 * @brief DynamicElementTextItem::masterChanged
 * This function is only use when the parent element is a slave.
 * Call when the master element linked to the parent slave element of this text change
 */
void DynamicElementTextItem::masterChanged()
{
		//First we remove the old connection
	if(!m_master_element.isNull() && (m_text_from == ElementInfo || m_text_from == CompositeText))
		disconnect(m_master_element.data(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
	
	if(elementUseForInfo())
	{
		m_master_element = elementUseForInfo();
		if(m_text_from == ElementInfo || m_text_from == CompositeText)
			connect(m_master_element.data(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
	}
		
		//Because master changed we update this text
	elementInfoChanged();
}

/**
 * @brief DynamicElementTextItem::reportChanged
 * This function is only use when parent element of this text is a folio report
 * The linked report of the parent element was changed
 */
void DynamicElementTextItem::reportChanged()
{
		/*
		 * When the dynamic text are added by a drag & drop from the element panel,
		 * the connection below are made in the constructor.
		 * If the text are added at load of a .qet file, the text is not yet added to a diagram then the connection is not made.
		 * We make it now, because when the linked report changed, that mean this text is in a diagram
		 */
	if(!m_report_formula_con)
	{
			//Get the report formula, and add connection to keep up to date the formula.
		if (parentElement()->diagram() && parentElement()->diagram()->project())
		{
			m_report_formula = parentElement()->diagram()->project()->defaultReportProperties();
			m_report_formula_con = connect(parentElement()->diagram()->project(), &QETProject::reportPropertiesChanged, this, &DynamicElementTextItem::reportFormulaChanged);
		}
	}
	
	bool text_have_label = false;
	
	if((textFrom() == ElementInfo && m_info_name == "label") ||
	   (textFrom() == CompositeText && m_composite_text.contains("%{label}")))
		text_have_label = true;
	
	if(text_have_label)
		removeConnectionForReportFormula(m_report_formula);
	
	m_other_report.clear();
	if(!m_parent_element.data()->linkedElements().isEmpty())
		m_other_report = m_parent_element.data()->linkedElements().first();
		
		//Because linked report was changed, we ensure there is a conductor watched
	setPotentialConductor();
	
	if(text_have_label)
	{
		setConnectionForReportFormula(m_report_formula);
		updateReportText();
	}
}

/**
 * @brief DynamicElementTextItem::reportFormulaChanged
 * The report formula use in the project was changed
 */
void DynamicElementTextItem::reportFormulaChanged()
{
	m_report_formula = parentElement()->diagram()->project()->defaultReportProperties();
	
	if(m_text_from == ElementInfo && m_info_name == "label")
		updateReportText();
}

void DynamicElementTextItem::setConnectionForReportFormula(const QString &formula)
{
	if(m_other_report.isNull() || formula.isEmpty())
		return;
	
	Element *other_elmt = m_other_report.data();
	QString string = formula;
	Diagram *other_diagram = m_other_report.data()->diagram();
	
		//Because the variable %F is a reference to another text which can contain variables,
		//we must to replace %F by the real text, to check if the real text contain the variable %id
	if (other_diagram && string.contains("%F"))
	{
		m_F_str = other_diagram->border_and_titleblock.folio();
		string.replace("%F", other_diagram->border_and_titleblock.folio());
		connect(&other_diagram->border_and_titleblock, &BorderTitleBlock::titleBlockFolioChanged, this, &DynamicElementTextItem::updateReportFormulaConnection);
	}
	
	if (other_diagram && (string.contains("%f") || string.contains("%id")))
		connect(other_diagram->project(), &QETProject::projectDiagramsOrderChanged, this, &DynamicElementTextItem::updateReportText);
	if (string.contains("%l"))
		connect(other_elmt, &Element::yChanged, this, &DynamicElementTextItem::updateReportText);
	if (string.contains("%c"))
		connect(other_elmt, &Element::xChanged, this, &DynamicElementTextItem::updateReportText);
}

void DynamicElementTextItem::removeConnectionForReportFormula(const QString &formula)
{
	if(m_other_report.isNull() || formula.isEmpty())
		return;
	
	Element *other_element = m_other_report.data();
	QString string = formula;
	Diagram *other_diagram = m_other_report.data()->diagram();
	
		//Because the variable %F is a reference to another text which can contain variables,
		//we must to replace %F by the real text, to check if the real text contain the variable %id
	if (other_diagram && string.contains("%F"))
	{
		string.replace("%F", m_F_str);
		disconnect(&other_diagram->border_and_titleblock, &BorderTitleBlock::titleBlockFolioChanged, this, &DynamicElementTextItem::updateReportFormulaConnection);
	}
	
	if (other_diagram && (string.contains("%f") || string.contains("%id")))
		disconnect(other_diagram->project(), &QETProject::projectDiagramsOrderChanged, this, &DynamicElementTextItem::updateReportText);
	if (string.contains("%l"))
		disconnect(other_element, &Element::yChanged, this, &DynamicElementTextItem::updateReportText);
	if (string.contains("%c"))
		disconnect(other_element, &Element::xChanged, this, &DynamicElementTextItem::updateReportText);
	
}

void DynamicElementTextItem::updateReportFormulaConnection()
{
	removeConnectionForReportFormula(m_report_formula);
	setConnectionForReportFormula(m_report_formula);
	updateReportText();
}

/**
 * @brief DynamicElementTextItem::updateReportText
 * This function is only use when this text is owned by a report, and this text have for info the Label.
 */
void DynamicElementTextItem::updateReportText()
{	
	if (m_text_from == ElementInfo && m_info_name == "label" && m_other_report)
	{
		Element *elmt = m_other_report.data();
		QString label = m_report_formula;
		label = autonum::AssignVariables::formulaToLabel(label, elmt->rSequenceStruct(), elmt->diagram(), elmt);
		setPlainText(label);
	}
	else if (m_text_from == CompositeText) {
		setPlainText(reportReplacedCompositeText());
	}
}

/**
 * @brief DynamicElementTextItem::conductorWasAdded
 * Function only use when parent element is a folio report
 * @param conductor
 */
void DynamicElementTextItem::conductorWasAdded(Conductor *conductor)
{
	Q_UNUSED(conductor)
	setPotentialConductor();
}

/**
 * @brief DynamicElementTextItem::conductorWasRemoved
 * Function only use when parent element is a folio report
 * @param conductor
 */
void DynamicElementTextItem::conductorWasRemoved(Conductor *conductor)
{
	if(m_watched_conductor.data() == conductor)
	{
		disconnect(m_watched_conductor.data(), &Conductor::propertiesChange, this, &DynamicElementTextItem::conductorPropertiesChanged);
		m_watched_conductor.clear();
		setPotentialConductor();
	}
}

/**
 * @brief DynamicElementTextItem::setPotentialConductor
 * This function is only used when the parent element of this text is a report element
 * Get a conductor in the potential of the parent report
 */
void DynamicElementTextItem::setPotentialConductor()
{
	if(parentElement() && (parentElement()->linkType() & Element::AllReport))
	{
		/*
		 * #First case, if m_watched_conductor is a conductor of the parent report, everything is ok
		 * #Second case, if the conductors list of parent report element is not empty,
		 * we set one of these conductor as m_watched_conductor, even if m_watched_conductor is already set,
		 * because that mean the conductor is a conductor of the linked report, and we prefer to set a conductor
		 * owned by the parent report element of this text.
		 * #third case, if m_watched_conductor is null, we set a conductor of the linked report, if any.
		 */
		QList<Conductor *> c_list = parentElement()->terminals().first()->conductors();
		
		if(!c_list.isEmpty() && c_list.contains(m_watched_conductor.data()))
			return;
		else if(!c_list.isEmpty())
		{
			if(!m_watched_conductor.isNull())
				disconnect(m_watched_conductor.data(), &Conductor::propertiesChange, this, &DynamicElementTextItem::conductorPropertiesChanged);
			
			m_watched_conductor = c_list.first();
			connect(m_watched_conductor.data(), &Conductor::propertiesChange, this, &DynamicElementTextItem::conductorPropertiesChanged);
		}
		else if(m_watched_conductor.isNull() && m_other_report)
		{
			if (!m_other_report.data()->terminals().first()->conductors().isEmpty())
			{
				m_watched_conductor = m_other_report.data()->terminals().first()->conductors().first();
				connect(m_watched_conductor.data(), &Conductor::propertiesChange, this, &DynamicElementTextItem::conductorPropertiesChanged);
			}
		}
	}
	else //This text haven't got a parent element, then ther isn't a conductor in the potential
	{
		if(!m_watched_conductor.isNull())
			disconnect(m_watched_conductor.data(), &Conductor::propertiesChange, this, &DynamicElementTextItem::conductorPropertiesChanged);
		
		m_watched_conductor.clear();
	}
}

/**
 * @brief DynamicElementTextItem::conductorPropertiesChanged
 * This function is only used when the parent element of this text is a report element
 */
void DynamicElementTextItem::conductorPropertiesChanged()
{
	if(m_parent_element && (m_parent_element.data()->linkType() & Element::AllReport))
	{
		if(m_text_from == ElementInfo)
		{
			if(m_info_name == "function")
				setPlainText(m_watched_conductor? m_watched_conductor.data()->properties().m_function : "");
			else if (m_info_name == "tension-protocol")
				setPlainText(m_watched_conductor? m_watched_conductor.data()->properties().m_tension_protocol : "");
		}
		else if (m_text_from == CompositeText) {
			setPlainText(reportReplacedCompositeText());
		}
	}
}

/**
 * @brief DynamicElementTextItem::reportReplacedCompositeText
 * This function is only used when the parent element of this text is a report element
 * @return the composite text with the variable replaced by the real value.
 * If the parent element of this text is not a folio report, return a default QString.
 */
QString DynamicElementTextItem::reportReplacedCompositeText() const
{
	QString string;
	
	if(m_parent_element.data()->linkType() & Element::AllReport)
	{
		string = m_composite_text;
		
		if (string.contains("%{label}") && m_other_report)
		{
			Element *elmt = m_other_report.data();
			QString label = m_report_formula;
			label = autonum::AssignVariables::formulaToLabel(label, elmt->rSequenceStruct(), elmt->diagram(), elmt);
			string.replace("%{label}", label);
		}
		if (m_watched_conductor)
		{
			if(string.contains("%{function}"))
				string.replace("%{function}", m_watched_conductor.data()->properties().m_function);
			if(string.contains("%{tension-protocol}"))
				string.replace("%{tension-protocol}", m_watched_conductor.data()->properties().m_tension_protocol);
		}
	}
	
	return string;
}

