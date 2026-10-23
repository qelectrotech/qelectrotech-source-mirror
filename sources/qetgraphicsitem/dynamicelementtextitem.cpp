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
#include "dynamicelementtextitem.h"
#include <QScopedPointer>
#include <QTextDocument>
#include "../QetGraphicsItemModeler/textresizehandles.h"
#include "../shownkinds.h"
#include "../qetproject.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qetapp.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetinformation.h"
#include "../utils/qetutils.h"
#include "../QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "crossrefitem.h"
#include "element.h"
#include "elementtextitemgroup.h"
#include <QTimer>
#include <QDomDocument>
#include <QDomElement>
#include <QtCore/qnumeric.h>
#include <QGraphicsSceneMouseEvent>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMimeData>
#include <QTextCursor>
#include <QTextDocumentFragment>

namespace {
	/**
		A composite text which uses the terminal number follows the label
		as closely as one which uses %{label}: both are drawn from the
		label of the element, and from the formula that label is made of.
	*/
	bool usesLabelVariables(const QString &composite_text)
	{
		return composite_text.contains(QLatin1String("%{label}"))
			|| composite_text.contains(
				QStringLiteral("%{") + QETInformation::ELMT_TERMINAL_NUMBER + QLatin1String("}"));
	}
}

void DynamicElementTextItem::keyPressEvent(QKeyEvent *event)
{
	if (!event->matches(QKeySequence::Paste) || m_text_from != UserText ||
	    !(textInteractionFlags() & Qt::TextEditable)) {
		DiagramTextItem::keyPressEvent(event);
		return;
	}
	if (diagram() && diagram()->isReadOnly()) {
		event->accept();
		return;
	}
	const QMimeData *mime = QApplication::clipboard()->mimeData();
	if (mime && (mime->hasText() || mime->hasHtml())) {
		const QString text = mime->hasText() ? mime->text() :
		    QTextDocumentFragment::fromHtml(mime->html()).toPlainText();
		prepareAlignment();
		QTextCursor cursor = textCursor();
		cursor.beginEditBlock();
		// An empty character format inherits the field's default font,
		// rather than the clipboard's font or the preceding character's.
		cursor.insertText(text, QTextCharFormat());
		cursor.setCharFormat(QTextCharFormat());
		cursor.endEditBlock();
		setTextCursor(cursor);
		finishAlignment();
	}
	event->accept();
}

/**
	@brief DynamicElementTextItem::DynamicElementTextItem
	Constructor
	@param parent_element
*/
DynamicElementTextItem::DynamicElementTextItem(Element *parent_element) :
	m_parent_element(parent_element),
	m_uuid(QUuid::createUuid())
{
	ShownKinds::tag(this, ShownKinds::SymbolTexts);
	setFont(QETApp::dynamicTextsItemFont());
	setText(tr("Text"));
	setParentItem(parent_element);
	QSettings settings;
	setRotation(settings.value("dynamic_text_rotation", 0).toInt());
	setKeepVisualRotation(true);
	setTextWidth(settings.value("dynamic_text_width", -1).toInt());
	connect(this, &DynamicElementTextItem::textEdited, [this](const QString &old_str, const QString &new_str)
	{
		if(this->m_parent_element && this->m_parent_element->diagram())
		{
			QUndoCommand *undo = new QPropertyUndoCommand(this, "text", old_str, new_str);
			undo->setText(tr("Edit an element text"));
			this->m_parent_element->diagram()->undoStack().push(undo);
		}
	});
	
		//Option when text is displayed in multiple line
	QTextOption option = document()->defaultTextOption();
	option.setAlignment(alignment() & Qt::AlignHorizontal_Mask);
	option.setWrapMode(QTextOption::WordWrap);
	document()->setDefaultTextOption(option);

		//Lines of a multi-line text follow the horizontal alignment
	connect(this, &DiagramTextItem::alignmentChanged, [this](Qt::Alignment alignment)
	{
		QTextOption option = document()->defaultTextOption();
		option.setAlignment(alignment & Qt::AlignHorizontal_Mask);
		document()->setDefaultTextOption(option);
		fitAutoTextWidth();
	});
	connect(document(), &QTextDocument::contentsChanged, this, &DynamicElementTextItem::fitAutoTextWidth);
		//The new font can make the lines wider or narrower than the width
		//fitted for the old one: fit again, keeping the text's anchor
	connect(this, &DiagramTextItem::fontChanged, this, [this]()
	{
		prepareAlignment();
		fitAutoTextWidth();
		finishAlignment();
	});
}

DynamicElementTextItem::~DynamicElementTextItem()
{
	removeResizeHandles();
}

/**
	@brief DynamicElementTextItem::textFromMetaEnum
	@return The QMetaEnum of the enum TextFrom
*/
QMetaEnum DynamicElementTextItem::textFromMetaEnum()
{
	DynamicElementTextItem deti;
	return deti.metaObject()->enumerator(deti.metaObject()->indexOfEnumerator("TextFrom"));
}

DynamicElementTextItem::DynamicElementTextItem()
{}

/**
	@brief DynamicElementTextItem::toXml
	Export this text to xml
	@param dom_doc
	@return
*/
QDomElement DynamicElementTextItem::toXml(QDomDocument &dom_doc) const
{
	QDomElement root_element = dom_doc.createElement(xmlTagName());
	
	root_element.setAttribute("x", QString::number(pos().x()));
	root_element.setAttribute("y", QString::number(pos().y()));
	root_element.setAttribute("rotation", QString::number(QET::correctAngle(rotation())));
	root_element.setAttribute("uuid", m_uuid.toString());
	root_element.setAttribute("frame", m_frame? "true" : "false");
	root_element.setAttribute("text_width", QString::number(m_text_width));
	root_element.setAttribute("font", QETUtils::fontToString(font()));
	root_element.setAttribute("keep_visual_rotation", m_keep_visual_rotation ? "true" : "false");
	root_element.setAttribute("rotation_point_center", m_rotation_point_center ? "true" : "false");

	QMetaEnum me = textFromMetaEnum();
	root_element.setAttribute("text_from", me.valueToKey(m_text_from));
	
	me = QMetaEnum::fromType<Qt::Alignment>();
	if(this->alignment() &Qt::AlignRight)
		root_element.setAttribute("Halignment", me.valueToKey(Qt::AlignRight));
	else if(this->alignment() &Qt::AlignLeft)
		root_element.setAttribute("Halignment", me.valueToKey(Qt::AlignLeft));
	else if(this->alignment() &Qt::AlignHCenter)
		root_element.setAttribute("Halignment", me.valueToKey(Qt::AlignHCenter));
	
	if(this->alignment() &Qt::AlignBottom)
		root_element.setAttribute("Valignment", me.valueToKey(Qt::AlignBottom));
	else if(this->alignment() & Qt::AlignTop)
		root_element.setAttribute("Valignment", me.valueToKey(Qt::AlignTop));
	else if(this->alignment() &Qt::AlignVCenter)
		root_element.setAttribute("Valignment", me.valueToKey(Qt::AlignVCenter));
	
	
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
	@brief DynamicElementTextItem::fromXml
	Import this text from xml
	@param dom_elmt
*/
void DynamicElementTextItem::fromXml(const QDomElement &dom_elmt)
{
	if (dom_elmt.tagName() != xmlTagName()) {
		qDebug() << "DynamicElementTextItem::fromXml : Wrong tagg name";
		return;
	}
	
	QGraphicsTextItem::setRotation(dom_elmt.attribute("rotation", QString::number(0)).toDouble());
	setKeepVisualRotation(dom_elmt.attribute("keep_visual_rotation", "true") == "true"? true : false);
	setRotationPointCenter(dom_elmt.attribute("rotation_point_center", "false") == "true"? true : false);

	if (dom_elmt.hasAttribute("font"))
	{
		QFont font;
		QETUtils::fontFromString(font, dom_elmt.attribute("font"));
		setFont(font);
	}
	else	//Retrocompatibility during the 0.7 dev because the font property was added lately. TODO remove this part in futur
	{
		QFont font_(dom_elmt.attribute("font_family", font().family()),
					dom_elmt.attribute("font_size", QString::number(9)).toInt());
		font_.setStyleName(dom_elmt.attribute("dynamicitemstyle", font().styleName()));
		setFont(font_);
	}

	m_uuid = QUuid(dom_elmt.attribute("uuid", QUuid::createUuid().toString()));
	setFrame(dom_elmt.attribute("frame", "false") == "true"? true : false);
	setTextWidth(dom_elmt.attribute("text_width", QString::number(-1)).toDouble());
	
		//Text from
	QMetaEnum me = textFromMetaEnum();
	setTextFrom(DynamicElementTextItem::TextFrom(me.keyToValue(dom_elmt.attribute("text_from").toStdString().data())));
	
	me = QMetaEnum::fromType<Qt::Alignment>();
	if(dom_elmt.hasAttribute("Halignment"))
		setAlignment(Qt::Alignment(me.keyToValue(dom_elmt.attribute("Halignment").toStdString().data())));
	if(dom_elmt.hasAttribute(("Valignment")))
		setAlignment(Qt::Alignment(me.keyToValue(dom_elmt.attribute("Valignment").toStdString().data())) | this->alignment());

		//Text
	QDomElement dom_text = dom_elmt.firstChildElement("text");
	if (!dom_text.isNull()) {
		setText(dom_text.text());
		setPlainText(dom_text.text());
	}
	else {
		setPlainText("");
	}

		//Info name
	QDomElement dom_info_name = dom_elmt.firstChildElement("info_name");
	if(!dom_info_name.isNull())
		setInfoName(dom_info_name.text());
	
		//Composite text
	QDomElement dom_comp_text = dom_elmt.firstChildElement("composite_text");
	if(!dom_comp_text.isNull())
		setCompositeText(dom_comp_text.text());

		//Color
	QDomElement dom_color = dom_elmt.firstChildElement("color");
	if(!dom_color.isNull())
		setColor(QColor(dom_color.text()));
	
		//Force the update of the displayed text
	setTextFrom(m_text_from);
	
		//QString::toDouble() accepts "nan"/"inf"/"-inf" as a successful
		//conversion, so a corrupted or hand-edited file can hand this a
		//non-finite position -- fall back to 0 the same way a missing
		//attribute already does, rather than letting it reach the scene
		//(see the matching comment in Element::valideXml(), the fromXml()
		//gate a plain element's position goes through; this text item has
		//no such gate to reject the whole item at, so it clamps instead).
	double x = dom_elmt.attribute("x", QString::number(0)).toDouble();
	double y = dom_elmt.attribute("y", QString::number(0)).toDouble();
	QGraphicsTextItem::setPos(qIsFinite(x) ? x : 0, qIsFinite(y) ? y : 0);
}

/**
	@brief DynamicElementTextItem::ParentElement
	@return a pointer to the parent element. Note the pointer can be null.
	Note that the text can return a parent element,
	even if the text belong to a group of this same element.
*/
Element *DynamicElementTextItem::parentElement() const
{
	return m_parent_element;
}

/**
	@brief DynamicElementTextItem::parentGroup
	@return The group where this text belong, if this item
	is note in a group, return nullptr.
*/
ElementTextItemGroup *DynamicElementTextItem::parentGroup() const
{
	if(parentItem())
	{
		if(ElementTextItemGroup *grp = dynamic_cast<ElementTextItemGroup *>(parentItem()))
		   return grp;
	}
	
	return nullptr;
}

/**
	@brief DynamicElementTextItem::masterElement
	@return the master element for slave elements, or the parent element for others.
	For PLC slaves, returns the actual master from linkedElements() instead of
	m_master_element (which points to self because elementUseForInfo() returns
	self for PLC slaves to resolve %{plc_*} variables).
*/
Element *DynamicElementTextItem::masterElement() const
{
	Element *elmt = parentElement();
	if (!elmt)
		return m_master_element.data();

	if (elmt->linkType() == Element::Slave &&
		elmt->elementData().m_slave_type == ElementData::PLCSlave &&
		!elmt->linkedElements().isEmpty())
	{
		return elmt->linkedElements().first();
	}

	return m_master_element.data();
}

/**
	@brief DynamicElementTextItem::elementUseForInfo
	@return a pointer to the element we must use for the variable information.
	If this text is owned by a simple element, the simple element is returned, this is the same element returned by the function parentElement().
	If this text is owned by a master element, the master element is returned, this is the same element returned by the function parentElement().
	If this text is owned by a report element, the report element is returned, this is the same element returned by the function parentElement().
	If this text is owned by a terminal element, the terminal element is returned, this is the same element returned by the function parentElement().
	If this text is owned by a slave element, we return the master element set as master of the parent slave element,
	if the parent slave is not linked to a master, this function return a nullptr.
	If this text have no parent element, return nullptr
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
		case Element::Thumbnail:
			return elmt;
		case Element::NextReport:
			return elmt;
		case Element::PreviousReport:
			return elmt;
		case Element::Master:
			return elmt;
		case Element::Slave:
		{
				//PLC slave stores its own PLC variables in elementInformations
				//(populated from master on link). Return self so %{plc_*} vars resolve.
			if (elmt->elementData().m_slave_type == ElementData::PLCSlave)
				return elmt;
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
	@brief DynamicElementTextItem::refreshLabelConnection
	Refresh the connection of this text when the source of text is label,
	or composite text, with a variable %{label}
*/
void DynamicElementTextItem::refreshLabelConnection()
{
	refreshLabelShown();
	if ((m_text_from == ElementInfo &&
		 (m_info_name == QLatin1String("label")
		  || m_info_name == QETInformation::ELMT_TERMINAL_NUMBER)) ||
		(m_text_from == CompositeText && usesLabelVariables(m_composite_text)))
	{
		if((m_parent_element.data()->linkType() & Element::AllReport) ||
			m_parent_element.data()->linkType() == Element::ConductorDefinition)
		{
			updateReportFormulaConnection();
			updateReportText();
		}
		else
		{
			setupFormulaConnection();
			updateLabel();
		}
	}
}

/**
	@brief DynamicElementTextItem::refreshLabelShown
	Whether this field is drawn at all: a field bound to the label of
	an element whose "Show name" box is unticked (@sa Element::labelShown)
	does not stay on the sheet as an empty frame to select -- it is
	hidden, and comes back the moment the box is ticked again. Both ways
	run through the same signals which refresh the text, so the field
	can never get stuck on one side. Every other field is drawn as
	always, and a composite text keeps drawing what is not the label.
*/
void DynamicElementTextItem::refreshLabelShown()
{
	const bool shown = !(m_text_from == ElementInfo
						 && m_info_name == QETInformation::ELMT_LABEL)
			|| !m_parent_element
			|| m_parent_element.data()->labelShown();

	if (!shown && isSelected()) {
		setSelected(false);
	}
	setVisible(shown);
	refreshResizeHandlesVisibility();
}

/**
	@brief DynamicElementTextItem::textFrom
	@return what the final text is created from.
*/
DynamicElementTextItem::TextFrom DynamicElementTextItem::textFrom() const
{
	return m_text_from;
}

/**
	@brief DynamicElementTextItem::setTextFrom
	Set the final text is created from.
	@param text_from
*/
void DynamicElementTextItem::setTextFrom(DynamicElementTextItem::TextFrom text_from)
{
	if(m_text_from == text_from)
		return;
	
	setNoEditable(text_from == UserText? false : true);
	clearFormulaConnection();
	
	TextFrom old_text_from = m_text_from;
	m_text_from = text_from;
	
	if(m_text_from == UserText)
	{
		setPlainText(m_text);
		disconnect(m_parent_element.data(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
	}
	else if (m_text_from == ElementInfo && elementUseForInfo())
	{
		if(m_info_name == QLatin1String("label")
		   || m_info_name == QETInformation::ELMT_TERMINAL_NUMBER)
		{
			setupFormulaConnection();
			updateLabel();
		}
		else
			setPlainText(elementUseForInfo()->infoForDisplay(m_info_name));
		
		if(old_text_from == UserText)
			connect(elementUseForInfo(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
	}
	else if (m_text_from == CompositeText && elementUseForInfo())
	{
		if(usesLabelVariables(m_composite_text))
		{
			setupFormulaConnection();
			updateLabel();
		}
		else
		{
				//The label and the terminal number are drawn the way the
				//settings say, every other information as it is stored
			DiagramContext dc = elementUseForInfo()->elementInformations();
			dc.addValue(QETInformation::ELMT_LABEL,            elementUseForInfo()->labelForDisplay());
			dc.addValue(QETInformation::ELMT_TERMINAL_NUMBER, elementUseForInfo()->terminalNumberForDisplay());
			setPlainText(autonum::AssignVariables::replaceVariable(m_composite_text, dc));
		}
		
		if(old_text_from == UserText)
			connect(elementUseForInfo(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
	}
		
	if(m_parent_element.data()->linkType() == Element::Master ||
	   m_parent_element.data()->linkType() == Element::Slave)
		updateXref();

		//A field which becomes, or stops being, the field for the
		//terminal number changes what the label of the symbol leaves
		//to it (@sa Element::labelForDisplay)
	if (m_info_name == QETInformation::ELMT_TERMINAL_NUMBER
		&& (m_text_from == ElementInfo || old_text_from == ElementInfo)
		&& parentElement()) {
		parentElement()->refreshLabelFields();
	}

	emit textFromChanged(m_text_from);
}

/**
	@brief DynamicElementTextItem::text
	@return the text of this text
*/
QString DynamicElementTextItem::text() const
{
	return m_text;
}

/**
	@brief DynamicElementTextItem::setText
	Set the text of this text
	@param text
*/
void DynamicElementTextItem::setText(const QString &text)
{
	m_text = text;
	if(m_text_from == DynamicElementTextItem::UserText)
		setPlainText(m_text);
	emit textChanged(m_text);
}

/**
	@brief DynamicElementTextItem::setInfoName
	Set the information name of the parent element.
	@param info_name
*/
void DynamicElementTextItem::setInfoName(const QString &info_name)
{
	QString old_info_name = m_info_name;
	m_info_name = info_name;

	
	if(old_info_name == QLatin1String("label")
	   || old_info_name == QETInformation::ELMT_TERMINAL_NUMBER)
	{
		clearFormulaConnection();
		updateXref();
	}

	if (m_parent_element && ((m_parent_element.data()->linkType() & Element::AllReport) ||
		m_parent_element.data()->linkType() == Element::ConductorDefinition)) //special treatment for report
	{
		if(old_info_name != info_name)
		{
			if(old_info_name == "label" || old_info_name == "cross_ref") {
				removeConnectionForReportFormula(m_report_formula);
			}
			if(info_name == "label" || info_name == "cross_ref")
			{
				setupFormulaConnection();
				setConnectionForReportFormula(m_report_formula);
				if (m_parent_element.data()->state() != QET::GIBuildingFromXml && m_parent_element.data()->state() != QET::GILoadingFromXml) {
					updateReportText();
				}
			}
			else
				conductorPropertiesChanged();
		}
	}
	else if ((m_info_name == QLatin1String("label")
			  || m_info_name == QETInformation::ELMT_TERMINAL_NUMBER)
			 && elementUseForInfo())
	{	
		setupFormulaConnection();
		updateLabel();
		updateXref();
	}
	else if(elementUseForInfo() && m_text_from == DynamicElementTextItem::ElementInfo) {
		setPlainText(elementUseForInfo()->infoForDisplay(info_name));
	}

		//A field which becomes, or stops being, the field for the
		//terminal number changes what the label of the symbol leaves
		//to it (@sa Element::labelForDisplay)
	if (parentElement()
		&& (info_name == QETInformation::ELMT_TERMINAL_NUMBER
			|| old_info_name == QETInformation::ELMT_TERMINAL_NUMBER)) {
		parentElement()->refreshLabelFields();
	}

		//A field which becomes, or stops being, the field for the
		//label of its element is shown or hidden with the "Show name"
		//box (@sa refreshLabelShown)
	refreshLabelShown();

	emit infoNameChanged(info_name);
}

/**
	@brief DynamicElementTextItem::infoName
	@return the info name of this text
*/
QString DynamicElementTextItem::infoName() const
{
	return m_info_name;
}

/**
	@brief DynamicElementTextItem::setCompositeText
	Set the composite text of this text item to text
	@param text
*/
void DynamicElementTextItem::setCompositeText(const QString &text)
{
	QString old_composite_text = m_composite_text;
	m_composite_text = text;
	
	if(old_composite_text.contains("%{label}"))
	{
		clearFormulaConnection();
		updateXref();
	}

	if (m_parent_element && ((m_parent_element.data()->linkType() & Element::AllReport) ||
		m_parent_element.data()->linkType() == Element::ConductorDefinition)) //special treatment for report
	{
			/*
			 * May be in some case the old and new composite text both have the var %{label},
			 * and so we don't have to remove connection and after set connection,
			 * but for that we must do several checks and because I'm lazy,
			 * in every case I remove connection and set it after ;)
			 */
		if(old_composite_text.contains("%{label}"))
			removeConnectionForReportFormula(m_report_formula);
		if(m_composite_text.contains("%{label}") || m_composite_text.contains("%{cross_ref}")) {
			setupFormulaConnection();
			setConnectionForReportFormula(m_report_formula);
		}
		
		updateReportText();
	}
	else if (m_composite_text.contains("%{label}") && elementUseForInfo())
	{
		setupFormulaConnection();
		updateLabel();
		updateXref();
	}
	else if (m_text_from == DynamicElementTextItem::CompositeText)
	{
		DiagramContext dc;
		if(elementUseForInfo())
			dc = elementUseForInfo()->elementInformations();
		setPlainText(autonum::AssignVariables::replaceVariable(m_composite_text, dc));
	}
	
	emit compositeTextChanged(m_composite_text);
}

/**
	@brief DynamicElementTextItem::compositeText
	@return
*/
QString DynamicElementTextItem::compositeText() const
{
	return m_composite_text;
}

void DynamicElementTextItem::setFrame(const bool frame)
{
	m_frame = frame;
	update();
	emit frameChanged(m_frame);
}

bool DynamicElementTextItem::frame() const
{
	return m_frame;
}

QUuid DynamicElementTextItem::uuid() const
{
	return m_uuid;
}

/**
	@brief DynamicElementTextItem::mousePressEvent
	@param event
*/
void DynamicElementTextItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
		//The text become selected, we set the real color, otherwise the editor will display the color of text as blue,
		//but this is not the color set by the user.
	if(m_user_color.isValid())
	{
		setDefaultTextColor(m_user_color);
		m_user_color = QColor(); //m_user_color is now invalid
		if(m_slave_Xref_item)
			m_slave_Xref_item->setDefaultTextColor(color());
	}

	// Shift or no parent initiates movement of dynamic text, otherwise movement of parent element
	if((event->modifiers() & Qt::ShiftModifier) || !m_parent_element)
	{
		m_move_parent = false;
		DiagramTextItem::mousePressEvent(event);
	} else {
		m_move_parent = true;
		parentElement()->mousePressEvent(event);
	}
}

/**
	@brief DynamicElementTextItem::mouseMoveEvent
	@param event
*/
void DynamicElementTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{	
	if((event->buttons() & Qt::LeftButton) && (flags() & ItemIsMovable))
	{
		if(m_move_parent)
		{
			parentElement()->mouseMoveEvent(event);
		} else {
			if(diagram() && m_first_move)
				diagram()->elementTextsMover().beginMovement(diagram(), this);
			
			if(m_first_move)
			{
				m_initial_position = pos();
				if(parentElement())
					parentElement()->setHighlighted(true);
			}
			
			QPointF current_parent_pos;
			QPointF button_down_parent_pos;
			current_parent_pos = mapToParent(mapFromScene(event->scenePos()));
			button_down_parent_pos = mapToParent(mapFromScene(event->buttonDownScenePos(Qt::LeftButton)));
			
			int diffx = qRound(current_parent_pos.x() - button_down_parent_pos.x());
			int diffy = qRound(current_parent_pos.y() - button_down_parent_pos.y());
			QPointF new_pos = m_initial_position + QPointF(diffx, diffy);
				//Snap to the grid, Ctrl to place freely -- the same line
				//ElementTextItemGroup::mouseMoveEvent() and
				//ElementTextsMover::continueMovement() already use, and
				//DiagramTextItem::mouseMoveEvent() for independent texts.
				//Without it this was the only text move in the editor that
				//ignored the grid.
			event->modifiers() == Qt::ControlModifier ? setPos(new_pos) : setPos(Diagram::snapToTextGrid(new_pos));

			if(diagram())
				diagram()->elementTextsMover().continueMovement(event);
		}
	} else {
		event->ignore();
	}
	
	if(m_first_move)
		m_first_move = false;
}

/**
	@brief DynamicElementTextItem::mouseReleaseEvent
	@param event
*/
void DynamicElementTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if(m_move_parent)
	{
		parentElement()->mouseReleaseEvent(event);
	} else {
		if (m_parent_element)
			m_parent_element->setHighlighted(false);
	
		if(m_parent_element && m_parent_element->diagram())
			m_parent_element.data()->diagram()->elementTextsMover().endMovement();
		
		if(!(event->modifiers() & Qt::ControlModifier))
			QGraphicsTextItem::mouseReleaseEvent(event);
	}
}

/**
	@brief DynamicElementTextItem::mouseDoubleClickEvent
	Reimplemented functions, for add extra feature when this text is owned by a slave.
	In this case if the parent slave element is linked to a master, and this text display the label of the master
	(both if the 'text from' is 'element info' or 'composite text') the QGraphicsView go to master and select it.
	@param event
*/
void DynamicElementTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	DiagramTextItem::mouseDoubleClickEvent(event);
	zoomToLinkedElement();
}

/**
	@brief DynamicElementTextItem::hoverEnterEvent
	If the parent element of this text is a folio report or a slave element, the element is linked
	and the text display the variable "label" we set the text blue for signal the user that the text act like
	a link when we double click on.
	@param event
*/
void DynamicElementTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	DiagramTextItem::hoverEnterEvent(event);
	
		//If the text is selected we set the real color, otherwise the editor will display the color of text as blue,
		//but this is not the color set by the user.
	if(isSelected())
		return;
	
	if (linkedElementShown())
	{
		m_user_color = color();
		setDefaultTextColor(Qt::blue);
		setCursor(Qt::PointingHandCursor);
	}
}

/**
	@brief DynamicElementTextItem::hoverLeaveEvent
	@param event
*/
void DynamicElementTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	DiagramTextItem::hoverLeaveEvent(event);
	
	if(m_user_color.isValid())
	{
		setDefaultTextColor(m_user_color);
		m_user_color = QColor(); //m_user_color is now invalid
	}
		//Out of the test above: a click (mousePressEvent()) already put
		//the colour back, and the hand would stay after it
	unsetCursor();
}

void DynamicElementTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (m_jump_highlight)
		painter->fillRect(boundingRect(), QColor(69, 137, 255, 90));

	DiagramTextItem::paint(painter, option, widget);

	if (m_frame)
	{
		painter->save();
		painter->setFont(QETApp::dynamicTextsItemFont(font().pointSize()));
		
			//Adjust the thickness according to the font size, 
		qreal w=0.3;
		if(font().pointSize() >= 5)
		{
			w = font().pointSizeF()*0.1;
			if(w > 2.5)
				w = 2.5;
		}
		
		QPen pen;
		pen.setColor(color());
		pen.setWidthF(w);
		painter->setPen(pen);
		painter->setRenderHint(QPainter::Antialiasing);

			//Adjust the rounding of the rectangle according to the size of the font
		qreal ro = font().pointSizeF()/3;
		painter->drawRoundedRect(frameRect(), ro, ro);
		
		painter->restore();
	}
}

QVariant DynamicElementTextItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
		//The first time this text is added to a scene, we make several checking and connection
		//according to the link type of the parent element
	if(change == QGraphicsItem::ItemSceneHasChanged && m_first_scene_change)
	{
		if(m_parent_element.isNull())
			return QGraphicsObject::itemChange(change, value);
		
			//If the parent is slave, we keep aware about the changement of master.
		if(m_parent_element.data()->linkType() == Element::Slave)
		{
			connect(m_parent_element.data(), &Element::linkedElementChanged, this, &DynamicElementTextItem::masterChanged);
			if(m_parent_element.data()->diagram())
				connect(m_parent_element.data()->diagram()->project(), &QETProject::XRefPropertiesChanged, this, &DynamicElementTextItem::updateXref);
				//The parent is already linked, wa call master changed for init the connection
			if(!m_parent_element.data()->linkedElements().isEmpty())
				masterChanged();
		}
		else if((m_parent_element.data()->linkType() & Element::AllReport) ||
			m_parent_element.data()->linkType() == Element::ConductorDefinition)
		{
			//Get the report formula, and add connection to keep up to date the formula.
			if (m_parent_element.data()->diagram() && m_parent_element.data()->diagram()->project())
			{
				m_report_formula = m_parent_element.data()->diagram()->project()->defaultReportProperties();
				m_report_formula_con = connect(m_parent_element.data()->diagram()->project(), &QETProject::reportPropertiesChanged, this, &DynamicElementTextItem::reportFormulaChanged);
			}

			//Add connection to keep up to date the status of the element linked to the parent folio report of this text.
			connect(m_parent_element.data(), &Element::linkedElementChanged, this, &DynamicElementTextItem::reportChanged);
			//The parent is already linked, we call reportChanged for init the connection
			if(!m_parent_element.data()->linkedElements().isEmpty())
				reportChanged();

			if(m_parent_element.data()->terminals().size())
			{
				//Add connection to keep up date the conductors added or removed to the parent folio report element
				connect(m_parent_element.data()->terminals().first(), &Terminal::conductorWasAdded, this, &DynamicElementTextItem::conductorWasAdded);
				connect(m_parent_element.data()->terminals().first(), &Terminal::conductorWasRemoved, this, &DynamicElementTextItem::conductorWasRemoved);
			}
			//Get a conductor in the potential
			setPotentialConductor();
		}
		else if(m_parent_element.data()->linkType() == Element::Master)
		{
			connect(m_parent_element.data(), &Element::linkedElementChanged, this, &DynamicElementTextItem::updateXref);
			if(m_parent_element.data()->diagram())
				connect(m_parent_element.data()->diagram()->project(), &QETProject::XRefPropertiesChanged, this, &DynamicElementTextItem::updateXref);
			//Also call updateXref for a master without any linked slave:
			//when the contact comb must show every contact group the master
			//defines, the cross ref is expected the moment the element lands
			//on the diagram, not only after the first link or the first
			//settings change.
			updateXref();
		}
		
		m_first_scene_change = false;
		return QGraphicsObject::itemChange(change, value);
	}
	else if (change == QGraphicsItem::ItemParentHasChanged)
	{
		updateXref();
		updateXref();
	}
	else if (change == QGraphicsItem::ItemSelectedHasChanged)
	{
		refreshResizeHandlesVisibility();
	}
	else if (change == QGraphicsItem::ItemSceneHasChanged && !scene())
	{
		removeResizeHandles();
	}

	return QGraphicsObject::itemChange(change, value);
}

bool DynamicElementTextItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
	if (watched == m_left_resize_handle || watched == m_right_resize_handle)
	{
		auto *handle = static_cast<QetGraphicsHandlerItem *>(watched);
		if (event->type() == QEvent::GraphicsSceneMousePress) {
			handlerMousePressEvent(handle, static_cast<QGraphicsSceneMouseEvent *>(event));
			return true;
		}
		else if (event->type() == QEvent::GraphicsSceneMouseMove) {
			handlerMouseMoveEvent(handle, static_cast<QGraphicsSceneMouseEvent *>(event));
			return true;
		}
		else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
			handlerMouseReleaseEvent(handle, static_cast<QGraphicsSceneMouseEvent *>(event));
			return true;
		}
		return false;
	}

	if(watched != m_slave_Xref_item)
		return false;

	if(event->type() == QEvent::GraphicsSceneHoverEnter) {
		m_slave_Xref_item->setDefaultTextColor(Qt::blue);
		return true;
	}
	else if(event->type() == QEvent::GraphicsSceneHoverLeave) {
		m_slave_Xref_item->setDefaultTextColor(color());
		return true;
	}
	else if(event->type() == QEvent::GraphicsSceneMouseDoubleClick) {
		zoomToLinkedElement();
		return true;
	}

	return false;
}

/**
	@brief DynamicElementTextItem::refreshResizeHandlesVisibility
	Show the resize handles when this text is selected directly, OR when its
	parent element is -- which is what an ordinary click without Shift
	selects (DynamicElementTextItem::mousePressEvent() forwards a plain
	click to the parent, so dragging a symbol by its label moves the whole
	symbol; a pre-existing, unrelated behaviour, left untouched here).
	Without this, the handles were reachable only via Shift+click or a
	right-click's context menu, neither of which a user reaches for to
	resize a text field (qelectrotech#591, reported by @arummler).

	Called from itemChange() -- both this item's own ItemSelectedHasChanged,
	below, and Element::itemChange() on the parent's, which calls this on
	every one of its texts. Not from paint(): see the comment there for why
	that crashed.
*/
void DynamicElementTextItem::refreshResizeHandlesVisibility()
{
		//A hidden text has no handles, not even while its element is
		//selected: refreshLabelShown() puts them back when the text
		//is shown again
	const bool handles_wanted = isVisible()
			&& (isSelected() || (m_parent_element && m_parent_element->isSelected()));
	if (handles_wanted && !m_left_resize_handle)
		addResizeHandles();
	else if (!handles_wanted && m_left_resize_handle)
		removeResizeHandles();
}

/**
	@brief DynamicElementTextItem::addResizeHandles
	Create and show the two width-resize handles (left/right edge of
	frameRect()), reusing QetGraphicsHandlerItem the same way QetShapeItem
	does for its own resize handles.
*/
void DynamicElementTextItem::addResizeHandles()
{
	if (m_left_resize_handle || !scene())
		return;

	qreal size = QETUtils::graphicsHandlerSize(this);
	m_left_resize_handle = new QetGraphicsHandlerItem(size);
	m_right_resize_handle = new QetGraphicsHandlerItem(size);

	for (QetGraphicsHandlerItem *handle : {m_left_resize_handle, m_right_resize_handle})
	{
			//Children of this text, not free scene items: Qt then carries
			//them along when the parent element moves, rotates or is
			//zoomed, and repaints their old and new area in the same
			//update as this text. Moving free items from paint() instead
			//left green fragments behind (qelectrotech#1002).
		handle->setParentItem(this);
		handle->setColor(Qt::darkGreen);
		handle->installSceneEventFilter(this);
	}
	m_resize_handles_con = connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
								   this, &DynamicElementTextItem::updateResizeHandlesPos);

	updateResizeHandlesPos();
}

/**
	@brief DynamicElementTextItem::removeResizeHandles
*/
void DynamicElementTextItem::removeResizeHandles()
{
	disconnect(m_resize_handles_con);
	delete m_left_resize_handle;
	delete m_right_resize_handle;
	m_left_resize_handle = nullptr;
	m_right_resize_handle = nullptr;
}

/**
	@brief DynamicElementTextItem::updateResizeHandlesPos
	Keep the two resize handles at the vertical middle of boundingRect()'s
	left and right edges, in scene coordinates -- called on every paint() so
	it stays correct across every kind of change that can move this item or
	change its size (position, rotation, font, text, textWidth...) without
	needing a dedicated hook for each one.

	Deliberately boundingRect(), not frameRect(): frameRect() is a tight box
	around the text's own natural (idealWidth()) size, re-centred inside
	boundingRect() -- it does not grow with textWidth(). Once a text has
	been widened, that leaves a growing gap between the tight frame and the
	dashed selection outline QGraphicsView draws at boundingRect(), which is
	the box a user actually sees and expects a resize handle to sit on
	(qelectrotech#591, reported by @arummler: "the drag elements should be
	on the border of the box"). boundingRect() reflects the full
	textWidth() (it is QGraphicsTextItem's own, driven by the document's
	laid-out size), so the handles now track the box that is visibly
	resized rather than the text glyphs inside it.
*/
void DynamicElementTextItem::updateResizeHandlesPos()
{
	if (!m_left_resize_handle || !m_right_resize_handle)
		return;

	QRectF br = boundingRect();
	m_left_resize_handle->setPos(br.left(), br.center().y());
	m_right_resize_handle->setPos(br.right(), br.center().y());
}

/**
	@brief DynamicElementTextItem::handlerMousePressEvent
	@param handle
	@param event
*/
void DynamicElementTextItem::handlerMousePressEvent(QetGraphicsHandlerItem *handle, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(handle)

		//The actual property value, kept as-is (possibly -1, meaning "auto")
		//so a later undo restores the exact original state rather than a
		//synthesized fixed width.
	m_resize_original_width = textWidth();
		//A concrete baseline for the live drag's delta math, which can't
		//start from -1.
	m_resize_baseline_width = (m_resize_original_width < 0) ? frameRect().width() : m_resize_original_width;
		//The text moves while it is dragged, to keep its alignment point
		//in place: read the mouse in the text's coordinates as they were
		//at the press, otherwise the delta grows with every step
	m_resize_scene_to_local = sceneTransform().inverted();
	m_resize_start_local_x = m_resize_scene_to_local.map(event->scenePos()).x();

		//The point the text is aligned on stays where it is: remember it
		//in the parent's coordinates, which take the text's rotation
		//into account
	m_resize_start_pos = pos();
	m_resize_fixed_parent = mapToParent(resizeAnchor());
}

/**
	@brief DynamicElementTextItem::minimumTextWidth
	@return the narrowest width the text can be dragged to: that of its
	widest word. Narrower, a long word runs out of the text's box while
	the short lines are still aligned inside it, and a centred or right
	aligned text falls apart (a lone "1" in the middle under its words).
*/
qreal DynamicElementTextItem::minimumTextWidth() const
{
	QScopedPointer<QTextDocument> narrowest(document()->clone());
	narrowest->setTextWidth(0);
	return qMax(narrowest->idealWidth(), qreal(10));
}

/**
	@brief DynamicElementTextItem::resizeAnchor
	@return the point of the text, in its own coordinates, that a drag on
	a width handle keeps in place: the point it is aligned on, one of the
	nine of its alignment (left, centre or right; top, middle or bottom).
	A text aligned top right keeps its top right corner whichever handle
	is dragged, and a text that wraps onto more lines while it narrows
	grows away from that point instead of moving into its neighbours.
*/
QPointF DynamicElementTextItem::resizeAnchor() const
{
	const QRectF br = boundingRect();
	const Qt::Alignment a = alignment();
	const qreal x = (a & Qt::AlignRight) ? br.right()
				  : (a & Qt::AlignHCenter) ? br.center().x()
				  : br.left();
	const qreal y = (a & Qt::AlignBottom) ? br.bottom()
				  : (a & Qt::AlignVCenter) ? br.center().y()
				  : br.top();
	return QPointF(x, y);
}

/**
	@brief DynamicElementTextItem::handlerMouseMoveEvent
	Live-resize the text while dragging, exactly like the element editor's
	resize handles live-update geometry during a drag (undo is only pushed
	on release). The drag delta is resolved in this item's own local
	coordinates (not scene coordinates) so a rotated text box still resizes
	along its own baseline.
	@param handle
	@param event
*/
void DynamicElementTextItem::handlerMouseMoveEvent(QetGraphicsHandlerItem *handle, QGraphicsSceneMouseEvent *event)
{
	qreal local_x = m_resize_scene_to_local.map(event->scenePos()).x();
	qreal delta = local_x - m_resize_start_local_x;
	if (handle == m_left_resize_handle)
		delta = -delta;

	qreal new_width = qMax(m_resize_baseline_width + delta, minimumTextWidth());
	setTextWidth(new_width);

		//Keep the point the text is aligned on in place, whatever the new
		//width and number of lines
	setPos(pos() + m_resize_fixed_parent - mapToParent(resizeAnchor()));
	updateResizeHandlesPos();
}

/**
	@brief DynamicElementTextItem::handlerMouseReleaseEvent
	Make the drag undoable with a TextResizeCommand, as the corner handles
	of a free text do: the width and, after a drag on the left handle, the
	position that kept the right edge in place. Both are already applied,
	live from the drag.
	@param handle
	@param event
*/
void DynamicElementTextItem::handlerMouseReleaseEvent(QetGraphicsHandlerItem *handle, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(handle)
	Q_UNUSED(event)

	qreal new_width = textWidth();
	if ((!qFuzzyCompare(m_resize_original_width, new_width) || pos() != m_resize_start_pos)
			&& m_parent_element && m_parent_element->diagram())
	{
			//Width and position together: undoing a drag on the left
			//handle puts the text back where it was
		auto *undo = new TextResizeCommand(this, m_resize_original_width, new_width,
										   m_resize_start_pos, pos());
		undo->setText(tr("Resize an element text"));
		m_parent_element->diagram()->undoStack().push(undo);
	}
}

void DynamicElementTextItem::elementInfoChanged()
{
		//The "Show name" box of the selection properties was possibly
		//ticked or unticked: the field of a hidden name leaves the
		//sheet and comes back with the box (@sa refreshLabelShown)
	refreshLabelShown();

		//A folio report's label shows where its partner is
		//(updateReportText()); the label in its information is only where
		//the partner was when last saved, and changing any other
		//information wrote that stale position back (F116)
	if (m_text_from == ElementInfo && m_info_name == QLatin1String("label")
			&& m_parent_element && (m_parent_element.data()->linkType() & Element::AllReport)
			&& m_other_report) {
		setupFormulaConnection();
		updateReportText();
		emit plainTextChanged();
		return;
	}

	DiagramContext dc;
	Element *element = elementUseForInfo();
	if(element) {
		dc = element->elementInformations();
			//The label and the terminal number are drawn the way the
			//project says (@sa QETProject::terminalLabelSplit): a
			//composite text which uses them must see the same values the
			//fields bound to them show
		dc.addValue(QETInformation::ELMT_LABEL,           element->labelForDisplay());
		dc.addValue(QETInformation::ELMT_TERMINAL_NUMBER, element->terminalNumberForDisplay());
	}
	
	QString final_text;


	if (m_text_from == ElementInfo)
	{
			//If the info is the label, then we must make some connection
			//if the label is created from a formula
		if(m_info_name == QLatin1String("label")
		   || m_info_name == QETInformation::ELMT_TERMINAL_NUMBER)
		{
			setupFormulaConnection();

			if (element) {
				final_text = element->infoForDisplay(m_info_name);
			}
		}
		else {
			final_text = dc.value(m_info_name).toString();
		}
	}
	else if (m_text_from == CompositeText)
	{
			//If the composite have the label variable, we must make some
			//connection if the label is created from a formula
		if (usesLabelVariables(m_composite_text))
			setupFormulaConnection();
		
			//A report's %{label} is where its partner is, not its own
			//label: the same text updateReportText() builds
		if (m_parent_element && (m_parent_element.data()->linkType() & Element::AllReport))
			final_text = reportReplacedCompositeText();
		else
			final_text = autonum::AssignVariables::replaceVariable(m_composite_text, dc);
	}
	else if (m_text_from  == UserText)
		final_text = m_text;
	
	setPlainText(final_text);
	emit plainTextChanged();
}

/**
	@brief DynamicElementTextItem::masterChanged
	This function is only use when the parent element is a slave.
	Call when the master element linked to the parent slave element of this text change
*/
void DynamicElementTextItem::masterChanged()
{
		//First we remove the old connection
	if(!m_master_element.isNull() && (m_text_from == ElementInfo || m_text_from == CompositeText))
	{
		disconnect(m_master_element.data(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
		m_master_element.clear();
		updateXref();
	}
	
	if(elementUseForInfo())
	{
		m_master_element = elementUseForInfo();
		if(m_text_from == ElementInfo || m_text_from == CompositeText)
			connect(m_master_element.data(), &Element::elementInfoChange, this, &DynamicElementTextItem::elementInfoChanged);
		
		updateXref();
	}
		
		//Because master changed we update this text
	elementInfoChanged();
}

/**
	@brief DynamicElementTextItem::reportChanged
	This function is only use when parent element of this text is a folio report
	The linked report of the parent element was changed
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
	
	if((textFrom() == ElementInfo && (m_info_name == "label" || m_info_name == "cross_ref")) ||
	   (textFrom() == CompositeText && (m_composite_text.contains("%{label}") || m_composite_text.contains("%{cross_ref}"))))
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
	@brief DynamicElementTextItem::reportFormulaChanged
	The report formula use in the project was changed
*/
void DynamicElementTextItem::reportFormulaChanged()
{
	m_report_formula = parentElement()->diagram()->project()->defaultReportProperties();
	
	if(m_text_from == ElementInfo && (m_info_name == "label" || m_info_name == "cross_ref"))
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
		//we must replace %F by the real text, to check if the real text contains the variable %id
	if (other_diagram && string.contains("%F"))
	{
		m_F_str = other_diagram->border_and_titleblock.folio();
		string.replace("%F", other_diagram->border_and_titleblock.folio());
		connect(&other_diagram->border_and_titleblock, &BorderTitleBlock::titleBlockFolioChanged, this, &DynamicElementTextItem::updateReportFormulaConnection);
	}
	
	if (other_diagram && (string.contains("%f") || string.contains("%id")))
	{
		connect(other_diagram->project(), &QETProject::projectDiagramsOrderChanged, this, &DynamicElementTextItem::updateReportText);
		connect(other_diagram->project(), &QETProject::diagramRemoved, this, &DynamicElementTextItem::updateReportText);
	}
	if (other_diagram && string.contains("%T"))
		connect(&other_diagram->border_and_titleblock, &BorderTitleBlock::informationChanged, this, &DynamicElementTextItem::updateReportText);
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
		//we must replace %F with the real text, to check if the real text contains the variable %id
	if (other_diagram && string.contains("%F"))
	{
		string.replace("%F", m_F_str);
		disconnect(&other_diagram->border_and_titleblock, &BorderTitleBlock::titleBlockFolioChanged, this, &DynamicElementTextItem::updateReportFormulaConnection);
	}
	
	if (other_diagram && (string.contains("%f") || string.contains("%id")))
		disconnect(other_diagram->project(), &QETProject::projectDiagramsOrderChanged, this, &DynamicElementTextItem::updateReportText);
	if (other_diagram && string.contains("%T"))
		disconnect(&other_diagram->border_and_titleblock, &BorderTitleBlock::informationChanged, this, &DynamicElementTextItem::updateReportText);
	if (string.contains("%l"))
		disconnect(other_element, &Element::yChanged, this, &DynamicElementTextItem::updateReportText);
	if (string.contains("%c"))
		disconnect(other_element, &Element::xChanged, this, &DynamicElementTextItem::updateReportText);
	
}

/**
	@brief DynamicElementTextItem::setupFormulaConnection
	Setup the required connection for the formula of the label.
*/
void DynamicElementTextItem::setupFormulaConnection()
{
	if ((m_text_from == ElementInfo &&
		 (m_info_name == QLatin1String("label")
		  || m_info_name == QETInformation::ELMT_TERMINAL_NUMBER)) ||
		(m_text_from == CompositeText && usesLabelVariables(m_composite_text)))
	{
		clearFormulaConnection();
		
		Element *element = elementUseForInfo();
		if (!element)
			return;
		
		Diagram *diagram = element->diagram();
		QString formula = element->elementInformations().value("formula").toString();

			//Label is frozen, so we don't update it.
		if (element->isFreezeLabel())
			return;
		
		if (diagram && formula.contains("%F"))
		{
			m_F_str = diagram->border_and_titleblock.folio();
			formula.replace("%F", m_F_str);
			m_formula_connection << connect(&diagram->border_and_titleblock, &BorderTitleBlock::titleBlockFolioChanged, this, &DynamicElementTextItem::updateLabel);
		}
		
		if (diagram && (formula.contains("%f") || formula.contains("%id")))
		{
			m_formula_connection << connect(diagram->project(), &QETProject::projectDiagramsOrderChanged, this, &DynamicElementTextItem::updateLabel);
			m_formula_connection << connect(diagram->project(), &QETProject::diagramRemoved, this, &DynamicElementTextItem::updateLabel);
		}
		if (formula.contains("%l"))
			m_formula_connection << connect(element, &Element::yChanged, this, &DynamicElementTextItem::updateLabel);
		if (formula.contains("%c"))
			m_formula_connection << connect(element, &Element::xChanged, this, &DynamicElementTextItem::updateLabel);
			
	}
}

void DynamicElementTextItem::clearFormulaConnection()
{
	for (const QMetaObject::Connection& con : m_formula_connection)
		disconnect(con);
	m_formula_connection.clear();
}

void DynamicElementTextItem::updateReportFormulaConnection()
{
	if(!(m_parent_element.data()->linkType() & Element::AllReport) &&
		m_parent_element.data()->linkType() != Element::ConductorDefinition)
		return;
	
	removeConnectionForReportFormula(m_report_formula);
	setConnectionForReportFormula(m_report_formula);
	updateReportText();
}

/**
	@brief DynamicElementTextItem::updateReportText
	This function is only use when this text is owned by a report, and this text have for info the Label.
*/
void DynamicElementTextItem::updateReportText()
{
	if(!(m_parent_element.data()->linkType() & Element::AllReport) &&
		m_parent_element.data()->linkType() != Element::ConductorDefinition)
		return;
	
	bool should_update = false;
	if (m_text_from == ElementInfo)
	{
		if (m_info_name == "cross_ref") {
			should_update = true;
		} else if (m_info_name == "label") {
			should_update = true;
		}
	}

	if (should_update)
	{
		if(m_other_report)
		{
			Element *elmt = m_other_report.data();
			QString xref_str = m_report_formula;
			xref_str = autonum::AssignVariables::formulaToLabel(xref_str, elmt->rSequenceStruct(), elmt->diagram(), elmt);
			
			if (m_info_name == "label") {
				QString lbl = m_parent_element ? m_parent_element.data()->actualLabel() : "";
				if (!lbl.isEmpty() && !xref_str.isEmpty()) {
					if (m_parent_element && m_parent_element.data()->linkType() == Element::PreviousReport) {
						setPlainText(xref_str + " / " + lbl);
					} else {
						setPlainText(lbl + " / " + xref_str);
					}
				} else if (!lbl.isEmpty()) {
					setPlainText(lbl);
				} else {
					setPlainText(xref_str);
				}
			} else {
				setPlainText(xref_str);
			}
		}
		else
		{
			if (m_info_name == "label") {
				QString lbl = m_parent_element ? m_parent_element.data()->actualLabel() : "";
				setPlainText(lbl);
			} else {
				setPlainText("");
			}
		}
	}
	else if (m_text_from == ElementInfo && m_info_name == "label")
	{
		// if we are label and NOT supposed to update with report text, update as normal label
		QString lbl = elementUseForInfo() ? elementUseForInfo()->actualLabel() : "";
		if (lbl.isEmpty() && elementUseForInfo()) {
			lbl = elementUseForInfo()->elementInformations().value("label").toString();
		}
		setPlainText(lbl);
	}
	else if (m_text_from == CompositeText) {
		setPlainText(reportReplacedCompositeText());
	}
}

/**
	@brief DynamicElementTextItem::updateLabel
	Update the displayed text, when this dynamic text is based on the label of the parent element.
	This function is notably use when the label itself is based from a formula.
	If this dynamic text isn't based on label, this function do nothing.
*/
void DynamicElementTextItem::updateLabel()
{
	if ((m_text_from == ElementInfo &&
		 (m_info_name == QLatin1String("label")
		  || m_info_name == "cross_ref"
		  || m_info_name == QETInformation::ELMT_TERMINAL_NUMBER)) ||
		(m_text_from == CompositeText &&
		 (usesLabelVariables(m_composite_text) || m_composite_text.contains("%{cross_ref}"))))
	{
		DiagramContext dc;
		Element *element = elementUseForInfo();
		if(element) {
			dc = element->elementInformations();
		}
		

		if(m_text_from == ElementInfo && element) {
			QString new_label = element->infoForDisplay(m_info_name);
			if (toPlainText() != new_label) {
				setPlainText(new_label);
			}
		}
		else if (m_text_from == CompositeText) {
			// Use the label as it must be drawn, so %{label} and
			// %{terminal_number} reflect the current resolved label (e.g.
			// after a folio/page-number change).
			// A contact not linked to a coil has no element to read from
			// (bugtracker #345): %{label} then shows empty, as it did
			// before actualLabel() was used here.
			if (element) {
				dc.addValue(QETInformation::ELMT_LABEL,           element->labelForDisplay());
				dc.addValue(QETInformation::ELMT_TERMINAL_NUMBER, element->terminalNumberForDisplay());
			}
			setPlainText(autonum::AssignVariables::replaceVariable(m_composite_text, dc));
		}
	}
}

/**
	@brief DynamicElementTextItem::conductorWasAdded
	Function only use when parent element is a folio report
	@param conductor
*/
void DynamicElementTextItem::conductorWasAdded(Conductor *conductor)
{
	Q_UNUSED(conductor)
	QTimer::singleShot(100, this, [this]() {
		setPotentialConductor();
		conductorPropertiesChanged();
	});
}

/**
	@brief DynamicElementTextItem::conductorWasRemoved
	Function only use when parent element is a folio report
	@param conductor
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
	@brief DynamicElementTextItem::setPotentialConductor
	This function is only used when the parent element of this text is a report element
	Get a conductor in the potential of the parent report
*/
void DynamicElementTextItem::setPotentialConductor()
{
	if(parentElement() && ((parentElement()->linkType() & Element::AllReport) || (parentElement()->linkType() == Element::ConductorDefinition) ||
		parentElement()->linkType() == Element::ConductorDefinition))
	{
		if(parentElement()->terminals().isEmpty())
			return;
		
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
		else if(m_watched_conductor.isNull() && m_other_report
				//A report symbol can have no terminal (some older collections')
				&& !m_other_report.data()->terminals().isEmpty())
		{
			if (!m_other_report.data()->terminals().first()->conductors().isEmpty())
			{
				m_watched_conductor = m_other_report.data()->terminals().first()->conductors().first();
				connect(m_watched_conductor.data(), &Conductor::propertiesChange, this, &DynamicElementTextItem::conductorPropertiesChanged);
			}
		}
		conductorPropertiesChanged();
	}
	else //This text haven't got a parent element, then ther isn't a conductor in the potential
	{
		if(!m_watched_conductor.isNull())
			disconnect(m_watched_conductor.data(), &Conductor::propertiesChange, this, &DynamicElementTextItem::conductorPropertiesChanged);
		
		m_watched_conductor.clear();
	}
}

/**
	@brief DynamicElementTextItem::conductorPropertiesChanged
	This function is only used when the parent element of this text is a report element
*/
void DynamicElementTextItem::conductorPropertiesChanged()
{
	if(m_parent_element && ((m_parent_element.data()->linkType() & Element::AllReport) || (m_parent_element.data()->linkType() == Element::ConductorDefinition) ||
		m_parent_element.data()->linkType() == Element::ConductorDefinition))
	{
		if(m_text_from == ElementInfo)
		{
			if(m_info_name == QETInformation::COND_FUNCTION)
				setPlainText(m_watched_conductor? m_watched_conductor.data()->properties().m_function : "");
			else if (m_info_name == QETInformation::COND_TENSION_PROTOCOL)
				setPlainText(m_watched_conductor? m_watched_conductor.data()->properties().m_tension_protocol : "");
			else if (m_info_name == QETInformation::COND_COLOR)
				setPlainText(m_watched_conductor? m_watched_conductor.data()->properties().m_wire_color : "");
			else if (m_info_name == QETInformation::COND_SECTION)
				setPlainText(m_watched_conductor? m_watched_conductor.data()->properties().m_wire_section : "");
		}
		else if (m_text_from == CompositeText) {
			setPlainText(reportReplacedCompositeText());
		}
	}
}

/**
	@brief DynamicElementTextItem::reportReplacedCompositeText
	This function is only used when the parent element of this text is a report element
	@return the composite text with the variable replaced by the real value.
	If the parent element of this text is not a folio report, return a default QString.
*/
QString DynamicElementTextItem::reportReplacedCompositeText() const
{
	QString string;

	if((m_parent_element.data()->linkType() & Element::AllReport) || (m_parent_element.data()->linkType() == Element::ConductorDefinition) ||
		m_parent_element.data()->linkType() == Element::ConductorDefinition)
	{
		string = m_composite_text;
		
		if (string.contains("%{label}") && m_other_report)
		{
			Element *elmt = m_other_report.data();
			QString label = m_report_formula;
			label = autonum::AssignVariables::formulaToLabel(label, elmt->rSequenceStruct(), elmt->diagram(), elmt);
			string.replace("%{label}", label);
		}
			// if element is not linked, replace an empty string
		else
		{
			string.replace("%{label}", "");
		}

		if (m_watched_conductor)
		{
			if(string.contains("%{function}"))
				string.replace("%{function}", m_watched_conductor.data()->properties().m_function);
			if(string.contains("%{tension_protocol}"))
				string.replace("%{tension_protocol}", m_watched_conductor.data()->properties().m_tension_protocol);
			if(string.contains("%{conductor_color}"))
				string.replace("%{conductor_color}", m_watched_conductor.data()->properties().m_wire_color);
			if(string.contains("%{conductor_section}"))
				string.replace("%{conductor_section}", m_watched_conductor.data()->properties().m_wire_section);
		}
			// if no conductor is connected, replace an empty String
		else
		{
			if(string.contains("%{function}"))
				string.replace("%{function}", "");
			if(string.contains("%{tension_protocol}"))
				string.replace("%{tension_protocol}", "");
			if(string.contains("%{conductor_color}"))
				string.replace("%{conductor_color}", "");
			if(string.contains("%{conductor_section}"))
				string.replace("%{conductor_section}", "");
		}

			//What is left is the report's own information, as in the
			//composite text of any other element: a report can then say
			//which device and terminal it leads to (%{comment}), the way
			//EPLAN prints "-XS27:N 2 UG Raum - 1.1"
		string = autonum::AssignVariables::replaceVariable(
					string, m_parent_element.data()->elementInformations());
	}
	
	return string;
}

/**
	@brief DynamicElementTextItem::linkedElementShown
	@return the element this text leads to: the master of a linked slave
	or the other end of a linked folio report, when the text shows its
	label; nullptr otherwise. Such a text turns blue under the mouse, shows
	a pointing hand and jumps there on a double click.
*/
Element *DynamicElementTextItem::linkedElementShown() const
{
	if(!parentElement())
		return nullptr;
	if (!((m_text_from == ElementInfo && m_info_name == "label") ||
		  (m_text_from == CompositeText && m_composite_text.contains("%{label}"))))
		return nullptr;
	if(parentElement()->linkType() == Element::Slave && m_master_element)
		return m_master_element.data();
	if((parentElement()->linkType() & Element::AllReport) && m_other_report)
		return m_other_report.data();
	return nullptr;
}

/**
	@brief DynamicElementTextItem::zoomToLinkedElement
	If the parent element is a folio report or a slave element,
	and is linked, zoom to the linked element
*/
void DynamicElementTextItem::zoomToLinkedElement()
{
	Element *zoomed_element = linkedElementShown();
	if(zoomed_element)
	{
			//Unselect and ungrab mouse to prevent unwanted
			//move when linked element is in the same scene of this.
		setSelected(false);
		ungrabMouse();

		Element::jumpTo(zoomed_element);
	}
}

void DynamicElementTextItem::setJumpHighlight(bool on)
{
	if (on == m_jump_highlight)
		return;
	m_jump_highlight = on;
	update();
}

/**
 * @brief DynamicElementTextItem::parentElementRotationChanged
 * Called when the parent element is rotated
 */
void DynamicElementTextItem::parentElementRotationChanged()
{
	if (m_parent_element && m_keep_visual_rotation)
	{
			//We temporally disconnect for not change m_visual_rotation value.
			//We don't use block signal, because rotationChanged signal is used in other place.
		disconnect(this, &DynamicElementTextItem::rotationChanged, this, &DynamicElementTextItem::thisRotationChanged);
		setTransformOriginPoint(m_rotation_point_center ? boundingRect().center() : QPointF(0, 0));
		this->setRotation(QET::correctAngle(m_visual_rotation_ref - m_parent_element->rotation(), true));
		connect(this, &DynamicElementTextItem::rotationChanged, this, &DynamicElementTextItem::thisRotationChanged);
	}
}

/**
 * @brief DynamicElementTextItem::thisRotationChanged
 * This function is called when user change the rotation of the text
 * and "keep visual rotation" is to true
 * to keep in memory the visual rotation wanted by the user.
 */
void DynamicElementTextItem::thisRotationChanged() {
	m_visual_rotation_ref = this->rotation() + m_parent_element->rotation();
}

/**
	@brief DynamicElementTextItem::updateXref
	Create or delete the Xref according to the current properties of the project
*/
void DynamicElementTextItem::updateXref()
{
	if(diagram())
	{
		if(m_parent_element.data()->linkType() == Element::Master)
		{
			XRefProperties xrp = diagram()->project()->defaultXRefProperties(m_parent_element.data()->kindInformations()["type"].toString());
			
			if(m_text_from == DynamicElementTextItem::ElementInfo &&
			   m_info_name == "label" &&
			   xrp.snapTo() == XRefProperties::Label &&
			   (!m_parent_element.data()->linkedElements().isEmpty()
			    || CrossRefItem::showAllConfiguredSlaves(
				    m_parent_element.data(), xrp)))
			{
				//For add a Xref, this text must not be in a group
				if(!parentGroup())
				{
					if(!m_Xref_item)
						m_Xref_item = new CrossRefItem(m_parent_element.data(), this);
					return;
				}
			}
		}
		else if (m_parent_element.data()->linkType() == Element::Slave &&
				!m_parent_element.data()->linkedElements().isEmpty())
		{
			Element *master_elmt = m_parent_element.data()->linkedElements().first();
			if(master_elmt && !parentGroup())
			{
				XRefProperties xrp = diagram()->project()->defaultXRefProperties(master_elmt->kindInformations()["type"].toString());

				//Champ de texte: store xref in element informations
				if(xrp.getXrefPos() == Qt::AlignHCenter)
				{
					if(m_text_from == DynamicElementTextItem::ElementInfo && m_info_name == "xref")
					{
						QString xref_label = xrp.slaveLabel();
						xref_label = autonum::AssignVariables::formulaToLabel(xref_label, master_elmt->rSequenceStruct(), master_elmt->diagram(), master_elmt);

						DiagramContext dc = m_parent_element->elementInformations();
						if(dc.value("xref").toString() != xref_label)
						{
							dc.addValue("xref", xref_label);
							m_parent_element->setElementInformations(dc);
						}

						//Set up connections for future updates
						if(m_update_slave_Xref_connection.isEmpty())
						{
							m_update_slave_Xref_connection << connect(master_elmt, &Element::xChanged,                       this, &DynamicElementTextItem::updateXref);
							m_update_slave_Xref_connection << connect(master_elmt, &Element::yChanged,                       this, &DynamicElementTextItem::updateXref);
							m_update_slave_Xref_connection << connect(master_elmt, &Element::elementInfoChange,              this, &DynamicElementTextItem::updateXref);
							m_update_slave_Xref_connection << connect(diagram(), &Diagram::diagramInformationChanged,                    this, &DynamicElementTextItem::updateXref);
							m_update_slave_Xref_connection << connect(diagram()->project(),    &QETProject::projectDiagramsOrderChanged, this, &DynamicElementTextItem::updateXref);
							m_update_slave_Xref_connection << connect(diagram()->project(),    &QETProject::diagramRemoved,              this, &DynamicElementTextItem::updateXref);
							m_update_slave_Xref_connection << connect(diagram()->project(),    &QETProject::XRefPropertiesChanged,       this, &DynamicElementTextItem::updateXref);
						}
						return;
					}
					//For label/composite text: fall through to cleanup (delete m_slave_Xref_item)
				}
				else if(
				   (m_text_from == DynamicElementTextItem::ElementInfo && m_info_name == "label") ||
				   (m_text_from == DynamicElementTextItem::CompositeText && m_composite_text.contains("%{label}"))
			   )
				{
					QString xref_label = xrp.slaveLabel();
					xref_label = autonum::AssignVariables::formulaToLabel(xref_label, master_elmt->rSequenceStruct(), master_elmt->diagram(), master_elmt);
					
					if(!m_slave_Xref_item)
					{
						m_slave_Xref_item = new QGraphicsTextItem(xref_label, this);
						ShownKinds::tag(m_slave_Xref_item, ShownKinds::CrossReferences);
						m_slave_Xref_item->setFont(QETApp::diagramTextsFont(5));
							// Match the parent text's user-configurable color instead of
							// hardcoding black, which renders invisible under dark themes
							// or stylesheets that do not affect this sub-item the same way
							// they affect the parent (bugtracker #247).
						m_slave_Xref_item->setDefaultTextColor(color());
						m_slave_Xref_item->installSceneEventFilter(this);
						
						m_update_slave_Xref_connection << connect(master_elmt, &Element::xChanged,                       this, &DynamicElementTextItem::updateXref);
						m_update_slave_Xref_connection << connect(master_elmt, &Element::yChanged,                       this, &DynamicElementTextItem::updateXref);
						m_update_slave_Xref_connection << connect(master_elmt, &Element::elementInfoChange,              this, &DynamicElementTextItem::updateXref);
						m_update_slave_Xref_connection << connect(diagram(), &Diagram::diagramInformationChanged,                    this, &DynamicElementTextItem::updateXref);
						m_update_slave_Xref_connection << connect(diagram()->project(),    &QETProject::projectDiagramsOrderChanged, this, &DynamicElementTextItem::updateXref);
						m_update_slave_Xref_connection << connect(diagram()->project(),    &QETProject::diagramRemoved,              this, &DynamicElementTextItem::updateXref);
						m_update_slave_Xref_connection << connect(diagram()->project(),    &QETProject::XRefPropertiesChanged,       this, &DynamicElementTextItem::updateXref);
					}
					else
						m_slave_Xref_item->setPlainText(xref_label);
					setXref_item(xrp.getXrefPos(), xrp.slaveOffset());
					return;
				}
			}
		}
	}

		//There is no reason to have a xref, we delete it if exist
	if(m_Xref_item)
	{
		delete m_Xref_item;
		m_Xref_item = nullptr;
	}
	
	m_update_slave_Xref_connection.clear();

	if(m_slave_Xref_item)
	{
		delete m_slave_Xref_item;
		m_slave_Xref_item = nullptr;

		//If position changed to Champ de texte, store xref in element info
		if(m_parent_element->linkType() == Element::Slave &&
		   !m_parent_element->linkedElements().isEmpty())
		{
			Element *master_elmt = m_parent_element->linkedElements().first();
			if(master_elmt && diagram())
			{
				XRefProperties xrp = diagram()->project()->defaultXRefProperties(master_elmt->kindInformations()["type"].toString());
				if(xrp.getXrefPos() == Qt::AlignHCenter)
				{
					QString xref_label = xrp.slaveLabel();
					xref_label = autonum::AssignVariables::formulaToLabel(xref_label, master_elmt->rSequenceStruct(), master_elmt->diagram(), master_elmt);

					DiagramContext dc = m_parent_element->elementInformations();
					if(dc.value("xref").toString() != xref_label)
					{
						dc.addValue("xref", xref_label);
						m_parent_element->setElementInformations(dc);
					}
				}
			}
		}
	}

	//Remove stale "xref" from elementInformations when no longer needed
	if(m_parent_element->linkType() == Element::Slave &&
	   m_text_from == ElementInfo && m_info_name == "xref" &&
	   !parentGroup())
	{
		DiagramContext dc = m_parent_element->elementInformations();
		if(!dc.value("xref").toString().isEmpty())
		{
			dc.remove("xref");
			m_parent_element->setElementInformations(dc);
		}
	}
}

void DynamicElementTextItem::setPlainText(const QString &text)
{
	if (toPlainText() == text)
		return;


	bool update_alignment = true;
	bool parsing_only = false;
	if (diagram() && (diagram()->project()->state() == QETProject::ProjectParsingRunning))
	{
		update_alignment = false;
		parsing_only = true;
	}
	if (m_parent_element.data()->state() == QET::GIBuildingFromXml ||
		m_parent_element.data()->state() == QET::GILoadingFromXml)
	{
		update_alignment = false;
		parsing_only = false;
	}

		//A text emptied with its alignment kept (the project not being
		//opened) is filled again the same way, even if the project is
		//being opened by then: lazy folios build the folios left late with
		//the project in that state, and the text would stay where emptying
		//moved it (F119)
	if (text.isEmpty())
		m_emptied_aligned = update_alignment;
	else if (!toPlainText().isEmpty())
		m_emptied_aligned = false;
	else if (parsing_only && m_emptied_aligned)
	{
		update_alignment = true;
		m_emptied_aligned = false;
	}
	else
		m_emptied_aligned = false;

	if (update_alignment) {
		prepareAlignment();
	}
	
	DiagramTextItem::setPlainText(text);
	
		//User define a text width
	if (m_text_width > 0)
	{
		if (document()->size().width() > m_text_width)
		{
			document()->setTextWidth(m_text_width);
			if (document()->size().width() > m_text_width)
			{
				document()->setTextWidth(document()->idealWidth());
			}
		}
	}
	
	if (update_alignment) {
		finishAlignment();
	}
	
	if (m_Xref_item) {
		m_Xref_item->autoPos();
	}
	else if (m_slave_Xref_item)
	{
		Element *master_elmt = m_parent_element.data()->linkedElements().isEmpty()
			? nullptr : m_parent_element.data()->linkedElements().first();
		if (master_elmt) {
			XRefProperties xrp = diagram()->project()->defaultXRefProperties(master_elmt->kindInformations()["type"].toString());
			setXref_item(xrp.getXrefPos(), xrp.slaveOffset());
		}
	}
}

void DynamicElementTextItem::setTextWidth(qreal width)
{
	this->document()->setTextWidth(width);
	m_text_width = width;
	fitAutoTextWidth();
	emit textWidthChanged(width);
}

/**
	@brief DynamicElementTextItem::fitAutoTextWidth
	With no width set by the user, a QTextDocument has no width to centre
	or right-align its lines in, so every line of a multi-line text stays
	on the left whatever the alignment. Give the document the width of its
	longest line instead. This does not change the size of the text, only
	where the shorter lines sit.
*/
void DynamicElementTextItem::fitAutoTextWidth()
{
	if (m_text_width > 0)
		return;

	document()->setTextWidth(-1);
	if (alignment() & (Qt::AlignHCenter | Qt::AlignRight))
		document()->setTextWidth(document()->idealWidth());
}

void DynamicElementTextItem::setXref_item(Qt::AlignmentFlag m_exHrefPos, int slave_offset)
{
	QRectF r = boundingRect();
	QPointF pos;
	//QPointF pos(r.center().x() - m_slave_Xref_item->boundingRect().width()/2,r.top());
	if (m_exHrefPos == Qt::AlignBottom)
	{
		pos = QPointF(r.center().x() - m_slave_Xref_item->boundingRect().width()/2,r.bottom() + slave_offset);
	}
	else if (m_exHrefPos == Qt::AlignTop)
	{
		pos = QPointF(r.center().x() - m_slave_Xref_item->boundingRect().width()/2,r.top() - m_slave_Xref_item->boundingRect().height() - slave_offset);
	}
	else if (m_exHrefPos == Qt::AlignLeft)  //
	{
		pos = QPointF(r.left() -  m_slave_Xref_item->boundingRect().width() - slave_offset,r.center().y() - m_slave_Xref_item->boundingRect().height()/2);
	}
	else if (m_exHrefPos == Qt::AlignRight)  //
	{
		pos = QPointF(r.right() + slave_offset ,r.center().y() - m_slave_Xref_item->boundingRect().height()/2);
	}
	else if (m_exHrefPos == Qt::AlignBaseline)  //
	{
		if(this->alignment() &Qt::AlignBottom)
		{
			pos = QPointF(r.center().x() - m_slave_Xref_item->boundingRect().width()/2,r.bottom() + slave_offset);
		}
		else if(this->alignment() &Qt::AlignTop)
		{
			pos = QPointF(r.center().x() - m_slave_Xref_item->boundingRect().width()/2,r.top() - m_slave_Xref_item->boundingRect().height() - slave_offset);
		}
		else if(this->alignment() &Qt::AlignLeft)
		{
			pos = QPointF(r.left() -  m_slave_Xref_item->boundingRect().width() - slave_offset,r.center().y() - m_slave_Xref_item->boundingRect().height()/2);
		}
		else if(this->alignment() &Qt::AlignRight)
		{
			pos = QPointF(r.right() + slave_offset ,r.center().y() - m_slave_Xref_item->boundingRect().height()/2);
		}
	}
	m_slave_Xref_item->setPos(pos);

	return;
}

void DynamicElementTextItem::setKeepVisualRotation(bool set)
{
	m_keep_visual_rotation = set;
	emit keepVisualRotationChanged(set);
	if (set) {
		m_visual_rotation_ref = this->rotation() + m_parent_element->rotation();
		setTransformOriginPoint(m_rotation_point_center ? boundingRect().center() : QPointF(0,0));
		connect(m_parent_element, &Element::rotationChanged, this, &DynamicElementTextItem::parentElementRotationChanged);
		connect(this, &DynamicElementTextItem::rotationChanged, this, &DynamicElementTextItem::thisRotationChanged);
	}
	else {
		disconnect(m_parent_element, &Element::rotationChanged, this, &DynamicElementTextItem::parentElementRotationChanged);
		disconnect(this, &DynamicElementTextItem::rotationChanged, this, &DynamicElementTextItem::thisRotationChanged);
	}
}

bool DynamicElementTextItem::keepVisualRotation() const {
	return m_keep_visual_rotation;
}

void DynamicElementTextItem::setRotationPointCenter(bool center)
{
	if (m_rotation_point_center == center) return;
	m_rotation_point_center = center;
	if (m_keep_visual_rotation)
		setTransformOriginPoint(center ? boundingRect().center() : QPointF(0,0));
	emit rotationPointCenterChanged(center);
}

bool DynamicElementTextItem::rotationPointCenter() const {
	return m_rotation_point_center;
}
