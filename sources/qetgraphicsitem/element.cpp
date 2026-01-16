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
#include "element.h"

#include "../PropertiesEditor/propertieseditordialog.h"
#include "../autoNum/numerotationcontextcommands.h"
#include "../diagram.h"
#include "../diagramcommands.h"
#include "../diagramcontext.h"
#include "../diagramposition.h"
#include "../elementprovider.h"
#include "../factory/elementpicturefactory.h"
#include "../properties/terminaldata.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/terminal.h"
#include "../ui/elementpropertieswidget.h"
#include "../undocommand/changeelementinformationcommand.h"
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"
#include "iostream"
#include "../qetxml.h"
#include "../qetversion.h"
#include "qgraphicsitemutility.h"

#include <QDomElement>
#include <utility>

class ElementXmlRetroCompatibility
{
	friend class Element;

	static void loadSequential(
			const QDomElement &dom_element,
			const QString& seq,
			QStringList* list)
	{
		int i = 0;
		while (!dom_element.attribute(seq +
						  QString::number(i+1)).isEmpty())
		{
				list->append(dom_element.attribute(
							 seq +
							 QString::number(i+1)));
				i++;
		}
	}

	static void loadSequential(
			const QDomElement &dom_element, Element *element)
	{
		autonum::sequentialNumbers sn;

		loadSequential(dom_element, QStringLiteral("sequ_"),  &sn.unit);
		loadSequential(dom_element, QStringLiteral("sequf_"), &sn.unit_folio);
		loadSequential(dom_element, QStringLiteral("seqt_"),  &sn.ten);
		loadSequential(dom_element, QStringLiteral("seqtf_"), &sn.ten_folio);
		loadSequential(dom_element, QStringLiteral("seqh_"),  &sn.hundred);
		loadSequential(dom_element, QStringLiteral("seqhf_"), &sn.hundred_folio);

		element->rSequenceStruct() = sn;
	}
};

/**
	@brief Element::Element
	@param location : location of this element
	@param parent : parent graphics item
	@param state : state of the instantiation
	@param link_type
*/
Element::Element(
		const ElementsLocation &location,
		QGraphicsItem *parent,
		int *state,
		kind link_type) :
	QetGraphicsItem(parent),
	m_link_type (link_type),
	m_location (location)
{
	if(! (location.isElement() && location.exist()))
	{
		if (state)
		{
			*state = 1;
			return;
		}
	}
	int elmt_state;
	buildFromXml(location.xml(), &elmt_state);
	if (state) {
		*state = elmt_state;
	}
	if (elmt_state) {
		return;
	}
	if (state) {
		*state = 0;
	}

	setPrefix(autonum::elementPrefixForLocation(location));
	m_uuid = QUuid::createUuid();
	setZValue(10);
	setFlags(QGraphicsItem::ItemIsMovable
		 | QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);

	connect(this, &Element::rotationChanged, [this]()
{
		for(QGraphicsItem *qgi : childItems())
		{
			if (Terminal *t = qgraphicsitem_cast<Terminal *>(qgi))
				t->updateConductor();
		}
	});
}

/**
	@brief Element::~Element
*/
Element::~Element()
{
	qDeleteAll (m_dynamic_text_list);
	qDeleteAll (m_terminals);
}

/**
	@brief Element::terminals
	@return the list of terminals of this element.
*/
QList<Terminal *> Element::terminals() const
{
	return m_terminals;
}

/**
	@brief Element::conductors
	@return The list of conductors docked to this element
	the list is sorted according to the position of the terminal where the conductor is docked
	from top to bottom, and left to right.
*/
QList<Conductor *> Element::conductors() const
{
	QList<Conductor *> conductors;

	for (Terminal *t : m_terminals) {
		conductors << t -> conductors();
	}

	return(conductors);
}

void Element::editProperty()
{
	if (diagram() && !diagram()->isReadOnly())
	{
		ElementPropertiesWidget *epw = new ElementPropertiesWidget(this);
		PropertiesEditorDialog dialog(epw, QApplication::activeWindow());
		connect(epw,
			&ElementPropertiesWidget::findEditClicked,
			&dialog,
			&QDialog::reject);
		//Must be windowModal, else when user do a drag and drop
		//with the "text" tab of ElementPropertiesWidget,
		//the ui freeze, until user press escape key
		dialog.setWindowModality(Qt::WindowModal);
		dialog.exec();
	}
}

/**
	@param hl true pour mettre l'element en evidence, false sinon
*/
void Element::setHighlighted(bool hl)
{
	m_must_highlight = hl;
	update();
}

/**
	@brief Element::displayHelpLine
	Display the help line of each terminal if b is true
	@param b
*/
void Element::displayHelpLine(bool b)
{
	foreach (Terminal *t, terminals())
		t->drawHelpLine(b);
}

/**
	@brief Element::paint
	@param painter
	@param options
*/
void Element::paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *options,
		QWidget *)
{
	if (m_must_highlight) {
		drawHighlight(painter, options);
	}

		//Set default pen and brush to QPainter to avoid a strange bug when
		//the Qt theme is a "dark" theme.
		//Some parts of an element are gray or white instead of black.
		//This bug seems append only when the QPainter uses drawPicture method.
		//See bug 175. https://qelectrotech.org/bugtracker/view.php?id=175
	painter->save();
	QPen pen;
	QBrush brush;
	painter->setPen(pen);
	painter->setBrush(brush);
	if (options && options->levelOfDetailFromTransform(painter->worldTransform()) < 0.5)
	{
		painter->drawPicture(0, 0, m_low_zoom_picture);
	} else {
		painter->drawPicture(0, 0, m_picture);
	}

	painter->restore(); //Restore the QPainter after use drawPicture

		//Draw the selection rectangle
	if ( isSelected() || m_mouse_over ) {
		QGIUtility::drawBoundingRectSelection(this, painter);
	}
}

/**
	@return Le rectangle delimitant le contour de l'element
*/
QRectF Element::boundingRect() const
{
	return(QRectF(QPointF(-hotspot_coord.x(), -hotspot_coord.y()),
			  dimensions));
}

/**
	@brief Element::setSize
	Define the size of the element.
	The size must be a multiple of 10.
	If not, the dimensions indicated will be arrrondies to higher tens.
	@param wid
	@param hei
*/
void Element::setSize(int wid, int hei)
{
	prepareGeometryChange();

	while (wid % 10) ++ wid;
	while (hei % 10) ++ hei;
	dimensions = QSize(wid, hei);
}

/**
	@return la taille de l'element sur le schema
*/
QSize Element::size() const
{
	return(dimensions);
}

/**
	Definit le hotspot de l'element par rapport au coin superieur gauche de son rectangle delimitant.
	Necessite que la taille ait deja ete definie
	@param hs Coordonnees du hotspot
*/
QPoint Element::setHotspot(QPoint hs)
{
	// la taille doit avoir ete definie
	prepareGeometryChange();
	if (dimensions.isNull()) hotspot_coord = QPoint(0, 0);
	else {
		// les coordonnees indiquees ne doivent pas depasser les dimensions de l'element
		int hsx = qMin(hs.x(), dimensions.width());
		int hsy = qMin(hs.y(), dimensions.height());
		hotspot_coord = QPoint(hsx, hsy);
	}
	return(hotspot_coord);
}

/**
	@return Le hotspot courant de l'element
*/
QPoint Element::hotspot() const
{
	return(hotspot_coord);
}

/**
	@brief Element::pixmap
	@return the pixmap of this element
*/
QPixmap Element::pixmap()
{
	return ElementPictureFactory::instance()->pixmap(m_location);
}

/*** Methodes protegees ***/

/**
	Dessine un petit repere (axes x et y) relatif a l'element
	@param painter Le QPainter a utiliser pour dessiner les axes
	@param options Les options de style a prendre en compte
*/
void Element::drawAxes(
		QPainter *painter,
		const QStyleOptionGraphicsItem *options)
{
	Q_UNUSED(options)
	painter -> setPen(Qt::blue);
	painter -> drawLine(0, 0, 10, 0);
	painter -> drawLine(7,-3, 10, 0);
	painter -> drawLine(7, 3, 10, 0);
	painter -> setPen(Qt::red);
	painter -> drawLine(0,  0, 0, 10);
	painter -> drawLine(0, 10,-3,  7);
	painter -> drawLine(0, 10, 3,  7);
}

/*** Methodes privees ***/

/**
	Dessine le cadre de selection de l'element de maniere systematiquement non antialiasee.
	@param painter Le QPainter a utiliser pour dessiner les bornes.
	@param options Les options de style a prendre en compte
*/
void Element::drawHighlight(
		QPainter *painter,
		const QStyleOptionGraphicsItem *options)
{
	Q_UNUSED(options)
	painter -> save();

	qreal gradient_radius = qMin(boundingRect().width(),
					 boundingRect().height()) / 2.0;
	QRadialGradient gradient(
		boundingRect().center(),
		gradient_radius,
		boundingRect().center()
	);
	gradient.setColorAt(0.0, QColor::fromRgb(69, 137, 255, 255));
	gradient.setColorAt(1.0, QColor::fromRgb(69, 137, 255, 0));
	QBrush brush(gradient);

	painter -> setPen(Qt::NoPen);
	painter -> setBrush(brush);
	// Le dessin se fait a partir du rectangle delimitant
	painter -> drawRoundedRect(boundingRect().adjusted(1, 1, -1, -1),
				   10,
				   10);
	painter -> restore();
}

/**
	@brief Element::buildFromXml
	Build this element from an xml description
	@param xml_def_elmt
	@param state
	Optional pointer which define the status of build
	0 - evreything all right
	4 - xml isn't a "definition"
	5 - attribute of the definition isn't present or valid
	6 - the definition is empty
	7 - parsing of a xml node who describe a graphical part failed.
	8 - No part of the drawing could be loaded
	@return
*/
bool Element::buildFromXml(const QDomElement &xml_def_elmt, int *state)
{
	m_state = QET::GIBuildingFromXml;

	if (xml_def_elmt.tagName() != QLatin1String("definition")
			|| xml_def_elmt.attribute(QStringLiteral("type")) != QLatin1String("element"))
	{
		if (state) *state = 4;
		m_state = QET::GIOK;
		return(false);
	}

		//Check if the current version can read the xml description
	const auto elmt_version = QetVersion::fromXmlAttribute(xml_def_elmt);
	if (!elmt_version.isNull()
		&& QetVersion::currentVersion() < elmt_version)
	{
		std::cerr << qPrintable(
						 QObject::tr("Avertissement : l'élément "
									 " a été enregistré avec une version"
									 " ultérieure de QElectroTech.")
						 ) << std::endl;
	}

		//This attribute must be present and valid
	int w = 0, h = 0, hot_x = 0, hot_y = 0;
	if (!QET::attributeIsAnInteger(xml_def_elmt, QStringLiteral("width"), &w)         ||
		!QET::attributeIsAnInteger(xml_def_elmt, QStringLiteral("height"), &h)        ||
		!QET::attributeIsAnInteger(xml_def_elmt, QStringLiteral("hotspot_x"), &hot_x) ||
		!QET::attributeIsAnInteger(xml_def_elmt, QStringLiteral("hotspot_y"), &hot_y))
	{
		if (state) *state = 5;
		m_state = QET::GIOK;
		return(false);
	}

	setSize(w, h);
	setHotspot(QPoint(hot_x, hot_y));

		//the definition must have childs
	if (xml_def_elmt.firstChild().isNull())
	{
		if (state) *state = 6;
		m_state = QET::GIOK;
		return(false);
	}

	m_data.fromXml(xml_def_elmt);
	setToolTip(name());

		//load kind informations
	m_kind_informations.fromXml(
				xml_def_elmt.firstChildElement(QStringLiteral("kindInformations")),
				QStringLiteral("kindInformation"));
		//load element information
	m_data.m_informations.fromXml(
				xml_def_elmt.firstChildElement(QStringLiteral("elementInformations")),
				QStringLiteral("elementInformation"));

		//scroll of the Children of the Definition: Parts of the Drawing
	int parsed_elements_count = 0;
	for (QDomNode node = xml_def_elmt.firstChild() ;
		 !node.isNull() ;
		 node = node.nextSibling())
	{
		QDomElement elmts = node.toElement();
		if (elmts.isNull())
			continue;

		if (elmts.tagName() == QLatin1String("description"))
		{
				//Minor workaround to find if there is a "input" tagg as label.
				//If not, we set the tagg "label" to the first "input.
			QList <QDomElement> input_field;
			bool have_label = false;
			for (QDomElement input_node = node.firstChildElement(QStringLiteral("input")) ;
				 !input_node.isNull() ;
				 input_node = input_node.nextSiblingElement(QStringLiteral("input")))
			{
				if (!input_node.isNull())
				{
					input_field << input_node;
					if (input_node.attribute(QStringLiteral("tagg"), QStringLiteral("none"))
							== QLatin1String("label"))
						have_label = true;
				}
			}
			if(!have_label && !input_field.isEmpty())
				input_field.first().setAttribute(QStringLiteral("tagg"), QStringLiteral("label"));

				//Parse the definition
			for (QDomNode n = node.firstChild() ;
				 !n.isNull() ;
				 n = n.nextSibling())
			{
				QDomElement qde = n.toElement();
				if (qde.isNull())
					continue;

				if (parseElement(qde)) {
					++ parsed_elements_count;
				}
				else
				{
					if (state)
						*state = 7;
					m_state = QET::GIOK;
					return(false);
				}
			}
		}
	}

	ElementPictureFactory *epf = ElementPictureFactory::instance();
	epf->getPictures(m_location,
			 const_cast<QPicture&>(m_picture),
			 const_cast<QPicture&>(m_low_zoom_picture));

	if(!m_picture.isNull())
		++ parsed_elements_count;

		//They must be at least one parsed graphics part
	if (!parsed_elements_count)
	{
		if (state)
			*state = 8;
		m_state = QET::GIOK;
		return(false);
	}
	else
	{
		if (state)
			*state = 0;
		m_state = QET::GIOK;
		return(true);
	}
}

/**
	@brief Element::parseElement
	Parse the element of the xml description of this element
	@param dom
	@return
*/
bool Element::parseElement(const QDomElement &dom)
{
	if      (dom.tagName() == QLatin1String("terminal"))     return(parseTerminal(dom));
	else if (dom.tagName() == QLatin1String("input"))        return(parseInput(dom));
	else if (dom.tagName() == QLatin1String("dynamic_text")) return(parseDynamicText(dom));
	else return(true);
}

/**
	@brief Element::parseInput
	Parse the input (old text field)
	the parsed input are converted to dynamic text field, this function
	is only here to keep compatibility with old text.
	@param dom_element
	@return
*/
bool Element::parseInput(const QDomElement &dom_element)
{
	qreal pos_x, pos_y;
	int size;
	if (!QET::attributeIsAReal(dom_element, QStringLiteral("x"), &pos_x) ||
		!QET::attributeIsAReal(dom_element, QStringLiteral("y"), &pos_y) ||
		!QET::attributeIsAnInteger(dom_element, QStringLiteral("size"), &size)) {
		return(false);
	} else {
		DynamicElementTextItem *deti = new DynamicElementTextItem(this);
		deti->setText(dom_element.attribute(QStringLiteral("text"), QStringLiteral("_")));
		QFont font = deti->font();
		font.setPointSize(dom_element.attribute(QStringLiteral("size"),
							QString::number(9)).toInt());
		deti->setFont(font);
		deti->setRotation(dom_element.attribute(QStringLiteral("rotation"),
							QString::number(0)).toDouble());

		if(dom_element.attribute(QStringLiteral("tagg"), QStringLiteral("none")) != QLatin1String("none"))
		{
			deti->setTextFrom(DynamicElementTextItem::ElementInfo);
			deti->setInfoName(dom_element.attribute(QStringLiteral("tagg")));
		}

			//the origin transformation point of PartDynamicTextField is the top left corner, no matter the font size
			//The origin transformation point of ElementTextItem is the middle of left edge, and so by definition, change with the size of the font
			//We need to use a QTransform to find the pos of this text from the saved pos of text item
		QTransform transform;
			//First make the rotation
		transform.rotate(dom_element.attribute(QStringLiteral("rotation"),
											   QStringLiteral("0")).toDouble());
		QPointF pos = transform.map(
						  QPointF(0,
								  -deti->boundingRect().height()/2));
		transform.reset();
			//Second translate to the pos
		QPointF p(dom_element.attribute(QStringLiteral("x"),
										QString::number(0)).toDouble(),
				  dom_element.attribute(QStringLiteral("y"),
										QString::number(0)).toDouble());
		transform.translate(p.x(), p.y());
		deti->setPos(transform.map(pos));
		m_dynamic_text_list.append(deti);
		return true;
	}
}

/**
	@brief Element::parseDynamicText
	Create the dynamic text field described in dom_element
	@param dom_element
	@return
*/
DynamicElementTextItem *Element::parseDynamicText(
		const QDomElement &dom_element)
{
	DynamicElementTextItem *deti = new DynamicElementTextItem(this);
		//Because the xml description of a .elmt file is the same as how a dynamic text field is saved to xml in a .qet file
		//we call fromXml, we just change the tagg name (.elmt = dynamic_text, .qet = dynamic_elmt_text)
		//and the uuid (because the uuid, is the uuid of the description and not the uuid of instantiated dynamic text field)

	QDomElement dom(dom_element.cloneNode(true).toElement());
	dom.setTagName(DynamicElementTextItem::xmlTagName());
	deti->fromXml(dom);
	deti->m_uuid = QUuid::createUuid();
	this->addDynamicTextItem(deti);
	return deti;
}

/*!
	\brief Element::parseTerminal
	Parse partTerminal from xml structure
	\param dom_element
	\return
*/
Terminal *Element::parseTerminal(const QDomElement &dom_element)
{
	TerminalData* data = new TerminalData();
	if (!data->fromXml(dom_element)) {
		delete data;
		return nullptr;
	}

	Terminal *new_terminal = new Terminal(data, this);
	m_terminals << new_terminal;

		//Sort from top to bottom and left to right
	std::sort(m_terminals.begin(),
		  m_terminals.end(),
		  [](Terminal *a,
		  Terminal *b)
	{
		if(a->dockConductor().y() == b->dockConductor().y())
			return (a->dockConductor().x() < b->dockConductor().x());
		else
			return (a->dockConductor().y() < b->dockConductor().y());
	});

	return(new_terminal);
}

/**
	Permet de savoir si un element XML (QDomElement) represente bien un element
	@param e Le QDomElement a valide
	@return true si l'element XML est un Element, false sinon
*/
bool Element::valideXml(QDomElement &e)
{
	if (e.tagName() != QLatin1String("element") ||
		!e.hasAttribute(QStringLiteral("type")) ||
		!e.hasAttribute(QStringLiteral("x"))    ||
		!e.hasAttribute(QStringLiteral("y"))) {
		return(false);
	}

	bool conv_ok;
	e.attribute(QStringLiteral("x")).toDouble(&conv_ok);
	if (!conv_ok) return(false);

	e.attribute(QStringLiteral("y")).toDouble(&conv_ok);
	if (!conv_ok) return(false);

	return(true);
}

/**
	@brief Element::fromXml
	Import the parameters of this element from a xml document.
	When call this function ensure this element is already in a scene, because
	the dynamic text item and element text item group (in the xml file) are created in this function
	and need a diagram for create their Xref, when this element is linked to another.
	If not the Xref can be not displayed, until the next call of update Xref of the group or text item.
	@param e : the dom element where the parameter is stored
	@param table_id_adr : Reference to the mapping table between IDs of the XML file
	and the addresses in memory. If the import succeeds, it must be add the right couples (id, address).
	@return
*/
bool Element::fromXml(QDomElement &e,
					  QHash<int,Terminal *> &table_id_adr)
{
	m_state = QET::GILoadingFromXml;
	/*
		les bornes vont maintenant etre recensees pour associer leurs id a leur adresse reelle
		ce recensement servira lors de la mise en place des fils
	*/
	QList<QDomElement> liste_terminals;
	for (auto qde :
			QET::findInDomElement(e, QStringLiteral("terminals"), QStringLiteral("terminal"))) {
		if (Terminal::valideXml(qde)) liste_terminals << qde;
	}

	QHash<int, Terminal *> priv_id_adr;

	for (auto *qgi : childItems())
	{
		if (auto terminal_ = qgraphicsitem_cast<Terminal *>(qgi))
		{
			for(auto qde : liste_terminals)
			{
				if (terminal_ -> fromXml(qde))
				{
					priv_id_adr.insert(qde.attribute(QStringLiteral("id")).toInt(),
									   terminal_);
				}
			}
		}
	}


		//Check that associated id/address doesn't conflict with table_id_adr
	for(auto found_id : priv_id_adr.keys())
	{
		if (table_id_adr.contains(found_id))
		{
				//This element got an id who is already referenced (= conflict)
			m_state = QET::GIOK;
			return(false);
		}
	}

		//Copie the association id/address
	for(auto found_id : priv_id_adr.keys()) {
		table_id_adr.insert(found_id,
							priv_id_adr.value(found_id));
	}

	//load uuid of connected elements
	QList <QDomElement> uuid_list = QET::findInDomElement(e,
														  QStringLiteral("links_uuids"),
														  QStringLiteral("link_uuid"));
	foreach (QDomElement qdo, uuid_list) {
		tmp_uuids_link << QUuid(qdo.attribute(QStringLiteral("uuid")));
	}

	//uuid of this element
	m_uuid = QUuid(e.attribute(QStringLiteral("uuid"), QUuid::createUuid().toString()));

		//load prefix
	m_prefix = e.attribute(QStringLiteral("prefix"));

	QString fl = e.attribute(QStringLiteral("freezeLabel"), QStringLiteral("false"));
	m_freeze_label = fl == QLatin1String("false") ? false : true;

		//Load Sequential Values
	if (e.hasAttribute(QStringLiteral("sequ_1"))
			|| e.hasAttribute(QStringLiteral("sequf_1"))
			|| e.hasAttribute(QStringLiteral("seqt_1"))
			|| e.hasAttribute(QStringLiteral("seqtf_1"))
			|| e.hasAttribute(QStringLiteral("seqh_1"))
			|| e.hasAttribute(QStringLiteral("sequf_1")))
		ElementXmlRetroCompatibility::loadSequential(e, this);
	else
		m_autoNum_seq.fromXml(e.firstChildElement(QStringLiteral("sequentialNumbers")));

		//Position and selection.
		//We directly call setPos from QGraphicsObject, because QetGraphicsItem will snap to grid
	QGraphicsObject::setPos(e.attribute(QStringLiteral("x")).toDouble(),
							e.attribute(QStringLiteral("y")).toDouble());
	setZValue(e.attribute(QStringLiteral("z"), QString::number(this->zValue())).toDouble());
	setFlags(QGraphicsItem::ItemIsMovable
		 | QGraphicsItem::ItemIsSelectable);

	// orientation
	bool conv_ok;
	int read_ori = e.attribute(QStringLiteral("orientation")).toInt(&conv_ok);
	if (!(conv_ok) || (read_ori < 0) || (read_ori > 3)) {
		read_ori = 0;
	}
	setRotation(90*read_ori);

		//Before loading the dynamic text field,
		//we remove the dynamic text field created from the description of this element, to avoid doubles.
	for(DynamicElementTextItem *deti : m_dynamic_text_list)
		delete deti;
	m_dynamic_text_list.clear();

		//************************//
		//***Dynamic texts item***//
		//************************//
	for (const QDomElement& qde : QET::findInDomElement(
			 e,
			 QStringLiteral("dynamic_texts"),
			 DynamicElementTextItem::xmlTagName()))
	{
		DynamicElementTextItem *deti = new DynamicElementTextItem(this);
		addDynamicTextItem(deti);
		deti->fromXml(qde);
	}

	for (QDomElement qde : QET::findInDomElement(
			 e,
			 QStringLiteral("texts_groups"),
			 ElementTextItemGroup::xmlTaggName()))
	{
		ElementTextItemGroup *group =
				addTextGroup(QStringLiteral("loaded_from_xml_group"));
		group->fromXml(qde);
	}

		//load informations
	DiagramContext dc;
	dc.fromXml(e.firstChildElement(QStringLiteral("elementInformations")),
			   QStringLiteral("elementInformation"));

		//Load override properties (For now, only used when the element is a terminal)
	if (m_data.m_type == ElementData::Terminal)
	{

		auto elmt_type_list = QETXML::subChild(e, QStringLiteral("properties"), QStringLiteral("element_type"));
		if (elmt_type_list.size())
		{
			auto elmt_type = elmt_type_list.first();
			m_data.setTerminalType(
						m_data.terminalTypeFromString(
							elmt_type.attribute(QStringLiteral("terminal_type"))));

			m_data.setTerminalFunction(
						m_data.terminalFunctionFromString(
							elmt_type.attribute(QStringLiteral("terminal_function"))));

			m_data.setTerminalLED(
						QETXML::boolFromString(
							elmt_type.attribute(QStringLiteral("terminal_led")), false));
		}
	}

	//We must block the update of the alignment when loading the information
	//otherwise the pos of the text will not be the same as it was at save time.
	for(DynamicElementTextItem *deti : m_dynamic_text_list)
		deti->m_block_alignment = true;
	setElementInformations(dc);
	for(DynamicElementTextItem *deti : m_dynamic_text_list)
		deti->m_block_alignment = false;

	m_state = QET::GIOK;
	return(true);
}

/**
	@brief Element::toXml
	Allows to export the element in XML
	\~French Permet d'exporter l'element en XML
	\~ @param document : XML document to use
	\~French Document XML a utiliser
	\~ @param table_adr_id :
	Correspondence table between the addresses of the terminals
	and their id in the XML representation;
	this table completed by this method
	\~French Table de correspondance entre les adresses des bornes
	et leur id dans la representation XML ;
	cette table completee par cette methode
	\~ @return The XML element representing this electrical element
	\~French L'element XML representant cet element electrique
*/
QDomElement Element::toXml(
		QDomDocument &document,
		QHash<Terminal *,
		int> &table_adr_id) const
{
	QDomElement element = document.createElement(QStringLiteral("element"));

		// type
	element.setAttribute(QStringLiteral("type"), m_location.path());

		// uuid
	element.setAttribute(QStringLiteral("uuid"), uuid().toString());

		// prefix
	element.setAttribute(QStringLiteral("prefix"), m_prefix);

		//frozen label
	element.setAttribute(QStringLiteral("freezeLabel"), m_freeze_label? QStringLiteral("true") : QStringLiteral("false"));

		// sequential num
	QDomElement seq = m_autoNum_seq.toXml(document);
	if (seq.hasChildNodes())
		element.appendChild(seq);

	// position, selection et orientation
	element.setAttribute(QStringLiteral("x"), QString::number(pos().x()));
	element.setAttribute(QStringLiteral("y"), QString::number(pos().y()));
	element.setAttribute(QStringLiteral("z"), QString::number(this->zValue()));
	element.setAttribute(QStringLiteral("orientation"), QString::number(orientation()));

	/* get the first id to use for the bounds of this element
	 * recupere le premier id a utiliser pour les bornes de cet element */
	int id_terminal = 0;
	if (!table_adr_id.isEmpty()) {
		// trouve le plus grand id
		int max_id_t = -1;
		foreach (int id_t, table_adr_id.values()) {
			if (id_t > max_id_t) max_id_t = id_t;
		}
		id_terminal = max_id_t + 1;
	}

	// registration of device terminals
	// enregistrement des bornes de l'appareil
	QDomElement xml_terminals = document.createElement(QStringLiteral("terminals"));
	// for each child of the element
	// pour chaque enfant de l'element
	foreach(Terminal *t, terminals()) {
		// alors on enregistre la borne
		QDomElement terminal = t -> toXml(document);
		terminal.setAttribute(QStringLiteral("id"), id_terminal); // for backward compatibility
		table_adr_id.insert(t, id_terminal ++);
		xml_terminals.appendChild(terminal);
	}
	element.appendChild(xml_terminals);

	// enregistrement des champ de texte de l'appareil
	QDomElement inputs = document.createElement(QStringLiteral("inputs"));
	element.appendChild(inputs);

	//if this element is linked to other elements,
	//save the uuid of each other elements
	if (! isFree()) {
		QDomElement links_uuids = document.createElement(QStringLiteral("links_uuids"));
		foreach (Element *elmt, connected_elements) {
			QDomElement link_uuid =
					document.createElement(QStringLiteral("link_uuid"));
			link_uuid.setAttribute(QStringLiteral("uuid"), elmt->uuid().toString());
			links_uuids.appendChild(link_uuid);
		}
		element.appendChild(links_uuids);
	}

	//save information of this element
	if (! m_data.m_informations.keys().isEmpty()) {
		QDomElement infos =
				document.createElement(QStringLiteral("elementInformations"));
		m_data.m_informations.toXml(infos, QStringLiteral("elementInformation"));
		element.appendChild(infos);
	}

		//Save override properties (For now, only used when the element is a terminal)
	if (m_data.m_type == ElementData::Terminal)
	{
		QDomElement properties = document.createElement(QStringLiteral("properties"));
		QDomElement element_type = document.createElement(QStringLiteral("element_type"));

		element_type.setAttribute(QStringLiteral("terminal_type"),
								m_data.terminalTypeToString(m_data.terminalType()));
		element_type.setAttribute(QStringLiteral("terminal_function"),
								m_data.terminalFunctionToString(m_data.terminalFunction()));
		element_type.setAttribute(QStringLiteral("terminal_led"),
								QETXML::boolToString(m_data.terminalLed()));

		properties.appendChild(element_type);
		element.appendChild(properties);
	}

	//Dynamic texts
	QDomElement dyn_text = document.createElement(QStringLiteral("dynamic_texts"));
	for (DynamicElementTextItem *deti : m_dynamic_text_list)
		dyn_text.appendChild(deti->toXml(document));

	QDomElement texts_group = document.createElement(QStringLiteral("texts_groups"));

		//Dynamic texts owned by groups
	for(ElementTextItemGroup *group : m_texts_group)
	{
		group->blockAlignmentUpdate(true);
			//temporarily remove the texts from group to get the pos relative to element and not group.
			//Set the alignment to top, because top is not used by groupand so,
			//each time a text is removed from the group, the alignement is not updated
		Qt::Alignment al = group->alignment();
		group->setAlignment(Qt::AlignTop);

			//Remove the texts from group
		QList<DynamicElementTextItem *> deti_list = group->texts();
		for(DynamicElementTextItem *deti : deti_list)
			group->removeFromGroup(deti);

			//Save the texts to xml
		for (DynamicElementTextItem *deti : deti_list)
			dyn_text.appendChild(deti->toXml(document));

			//Re add texts to group
		for(DynamicElementTextItem *deti : deti_list)
			group->addToGroup(deti);

			//Restorr the alignment
		group->setAlignment(al);

			//Save the group to xml
		texts_group.appendChild(group->toXml(document));
		group->blockAlignmentUpdate(false);
	}

		//Append the dynamic texts to element
	element.appendChild(dyn_text);
		//Append the texts group to element
	element.appendChild(texts_group);

	return(element);
}

/**
	@brief Element::addDynamiqueTextItem
	Add deti as a dynamic text item of this element,
	deti is reparented to this
	If deti is null, a new DynamicElementTextItem is created
	and added to this element.
	@param deti
*/
void Element::addDynamicTextItem(DynamicElementTextItem *deti)
{
	if (deti && !m_dynamic_text_list.contains(deti))
	{
		m_dynamic_text_list.append(deti);
		deti->setParentItem(this);
		emit textAdded(deti);
	}
	else
	{
		DynamicElementTextItem *text = new DynamicElementTextItem(this);
		m_dynamic_text_list.append(text);
		emit textAdded(text);
	}
}

/**
	@brief Element::removeDynamicTextItem
	Remove deti, no matter if is a child of this element
	or a child of a group of this element.
	Set he parent item of deti to 0, deti is not deleted.
	@param deti
*/
void Element::removeDynamicTextItem(DynamicElementTextItem *deti)
{
	if (m_dynamic_text_list.contains(deti))
	{
		m_dynamic_text_list.removeOne(deti);
		deti->setParentItem(nullptr);
		emit textRemoved(deti);
		return;
	}

	for(ElementTextItemGroup *group : m_texts_group)
	{
		if(group->texts().contains(deti))
		{
			removeTextFromGroup(deti, group);
			m_dynamic_text_list.removeOne(deti);
			deti->setParentItem(nullptr);
			emit textRemoved(deti);
			return;
		}
	}
}

/**
	@brief Element::dynamicTextItems
	@return all dynamic text items of this element directly child of this element.
	Texts in text-groups belonging to this element are not returned by this function.
	@see ElementTextItemGroup::texts
*/
QList<DynamicElementTextItem *> Element::dynamicTextItems() const
{
	return m_dynamic_text_list;
}

/**
	@brief Element::addTextGroup
	Create and add an element text item group to this element.
	If this element already have a group with the same name,
	then name will renamed to name1 or name2 etc....
	@param name : the name of the group
	@return the created group.
*/
ElementTextItemGroup *Element::addTextGroup(const QString &name)
{
	if(m_texts_group.isEmpty())
	{
		ElementTextItemGroup *group = new ElementTextItemGroup(name,
									   this);
		m_texts_group << group;
		emit textsGroupAdded(group);
		return group;
	}

		//Set a new name if name already exist
	QString rename = name;
	int i=1;
	while (textGroup(rename))
	{
		rename = name+QString::number(i);
		i++;
	}

		//Create the group
	ElementTextItemGroup *group = new ElementTextItemGroup(rename, this);
	m_texts_group << group;
	emit textsGroupAdded(group);
	return group;
}

/**
	@brief Element::addTextGroup
	@param group add group to the group of this element.
	the group must not be owned by an element.
*/
void Element::addTextGroup(ElementTextItemGroup *group)
{
	if(group->parentElement())
		return;

	m_texts_group << group;
	group->setParentItem(this);
	emit textsGroupAdded(group);
}

/**
	@brief Element::removeTextGroup
	Remove the text group group from this element,
	and set the parent of group to 0.
	group is not deleted.
	All texts owned by the group will be reparented to this element
	@param group
*/
void Element::removeTextGroup(ElementTextItemGroup *group)
{
	if(!m_texts_group.contains(group))
		return;

	const QList<QGraphicsItem *> items_list = group->childItems();

	for(QGraphicsItem *qgi : items_list)
	{
		if(qgi->type() == DynamicElementTextItem::Type)
		{
			DynamicElementTextItem *deti =
				static_cast<DynamicElementTextItem *>(qgi);
			removeTextFromGroup(deti, group);
		}
	}


	emit textsGroupAboutToBeRemoved(group);
	m_texts_group.removeOne(group);
	group->setParentItem(nullptr);
}

/**
	@brief Element::textGroup
	@param name
	@return the text group named name or nullptr if this element
	haven't got a group with this name
*/
ElementTextItemGroup *Element::textGroup(const QString &name) const
{
	for (ElementTextItemGroup *group : m_texts_group)
		if(group->name() == name)
			return group;

	return nullptr;
}

/**
	@brief Element::textGroups
	@return All texts groups of this element
*/
QList<ElementTextItemGroup *> Element::textGroups() const
{
	return m_texts_group;
}

/**
	@brief Element::addTextToGroup
	Add the text text to the group group;
	If group isn't owned by this element return false.
	The text must be a text of this element.
	@return : true if the text was successfully added to the group.
*/
bool Element::addTextToGroup(DynamicElementTextItem *text,
				 ElementTextItemGroup *group)
{
	if(!m_dynamic_text_list.contains(text))
		return false;
	if(!m_texts_group.contains(group))
		return false;

	m_dynamic_text_list.removeOne(text);
	emit textRemoved(text);

	group->addToGroup(text);
	emit textAddedToGroup(text, group);

	return true;
}

/**
	@brief Element::removeTextFromGroup
	Remove the text text from the group group,
	en reparent text to this element
	@return true if text was successfully removed
*/
bool Element::removeTextFromGroup(DynamicElementTextItem *text,
				  ElementTextItemGroup *group)
{
	if(!m_texts_group.contains(group))
		return false;

	if(group->texts().contains(text))
	{
		group->removeFromGroup(text);
		emit textRemovedFromGroup(text, group);
		addDynamicTextItem(text);
		return true;
	}

	return false;
}

/**
	@brief Element::AlignedFreeTerminals
	@return a list of terminal (owned by this element) aligned to other terminal (from other element)
	The first Terminal of QPair is a Terminal owned by this element,
	this terminal haven't got any conductor docked.
	The second Terminal of QPair is a Terminal owned by an other element,
	which is aligned with the first Terminal. The second Terminal can have or not docked conductors.
*/
QList <QPair <Terminal *, Terminal *> > Element::AlignedFreeTerminals() const
{
	QList <QPair <Terminal *, Terminal *> > list;

	foreach (Terminal *terminal, terminals())
	{
		if (terminal->conductors().isEmpty())
		{
			Terminal *other_terminal =
					terminal -> alignedWithTerminal();
			if (other_terminal)
				list << qMakePair(terminal, other_terminal);
		}
	}

	return list;
}

/**
	@brief Element::initLink
	Initialise the link between this element and other elements.
	This method can be call once because init the link according to
	uuid store in a private list, after link, the list is clear, so
	call another time do nothing.

	@param prj :
	ownership project of this element and other element to be linked
*/
void Element::initLink(QETProject *prj)
{
		// if nothing to link return now
	if (tmp_uuids_link.isEmpty()) return;

	ElementProvider ep(prj);
	foreach (Element *elmt, ep.fromUuids(tmp_uuids_link)) {
		elmt->linkToElement(this);
	}
	tmp_uuids_link.clear();
}

/**
 * @brief Element::linkTypeToString
 * \deprecated use instead ElementData::typeToString
 * \todo remove this function
 * @return
 */
QString Element::linkTypeToString() const
{
	switch (m_link_type)
	{
		case Simple:
			return QStringLiteral("Simple");
		case NextReport :
			return QStringLiteral("NextReport");
		case PreviousReport:
			return QStringLiteral("PreviousReport");
		case Master:
			return QStringLiteral("Master");
		case Slave:
			return QStringLiteral("Slave");
		case Terminale:
			return QStringLiteral("Terminale");
		case Thumbnail:
			return QStringLiteral("Thumbnail");
		default:
			return QStringLiteral("Unknown");
	}
}

/**
	@brief Element::setElementInformations
	Set new information for this element.
	If new information is different of current infotmation emit elementInfoChange
	@param dc
*/
void Element::setElementInformations(DiagramContext dc)
{
	if (m_data.m_informations == dc) {
		return;
	}

	const auto old_info = m_data.m_informations;
	m_data.m_informations = dc;

	const auto actual_label{actualLabel()};
	if (!actual_label.isEmpty()) {
		m_data.m_informations.addValue(QStringLiteral("label"), actual_label); //Update the label if there is a formula
	}
	emit elementInfoChange(old_info, m_data.m_informations);
}

/**
 * @brief Element::elementData
 * @return the element data used by this element
 */
ElementData Element::elementData() const
{
	return m_data;
}

/**
 * @brief Element::setElementData
 * Set new data for this element.
 * If m_information of \p data is changed, emit elementInfoChange
 * @param data
 */
void Element::setElementData(ElementData data)
{
	auto old_info = m_data.m_informations;
	m_data = data;

	if (old_info != m_data.m_informations) {
		m_data.m_informations.addValue(QStringLiteral("label"), actualLabel()); //Update the label if there is a formula
		if (diagram()) {
			diagram()->project()->dataBase()->elementInfoChanged(this);
		}
		emit elementInfoChange(old_info, m_data.m_informations);
	}
}

/**
	@brief comparPos
	Compare position of the two elements. Compare 3 points:
	1 folio - 2 row - 3 line
	returns a response when a comparison is found.
	@return true if elmt1 is at lower position than elmt 2, else false
*/
bool comparPos(const Element *elmt1, const Element *elmt2)
{
	//Compare folio first
	if (elmt1->diagram()->folioIndex() != elmt2->diagram()->folioIndex())
		return elmt1->diagram()->folioIndex()
				<
				elmt2->diagram()->folioIndex();
	//Compare the row(in letter pos) in second
	QString a = elmt1
			->diagram()
			->convertPosition(elmt1->scenePos()).letter();
	QString b = elmt2
			->diagram()
			->convertPosition(elmt2->scenePos()).letter();
	if (a != b)
		return a<b;
	//In last compare the line, if line is egal, return sorted by row in real pos
	if (elmt1->pos().x() == elmt2->pos().x())
		return elmt1->y() <= elmt2->pos().y();
	return elmt1->pos().x() <= elmt2->pos().x();
}

/**
	@brief Element::mouseMoveEvent
	@param event
*/
void Element::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QetGraphicsItem::mouseMoveEvent(event);
	foreach (Terminal *t, terminals())
	{
		t -> drawHelpLine(true);
	}
}

/**
	@brief Element::mouseReleaseEvent
	@param event
*/
void Element::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	QetGraphicsItem::mouseReleaseEvent(event);
	foreach (Terminal *t, terminals())
	{
		t -> drawHelpLine(false);
	}
}

/**
	When mouse over element
	change m_mouse_over to true   (used in paint() function )
	Also highlight linked elements
	@param e QGraphicsSceneHoverEvent
*/
void Element::hoverEnterEvent(QGraphicsSceneHoverEvent *e)
{
	Q_UNUSED(e)

	foreach (Element *elmt, linkedElements())
		elmt -> setHighlighted(true);

	m_mouse_over = true;
	setToolTip( name() );
	update();
}

/**
	When mouse over element leave the position
	change m_mouse_over to false(used in paint() function )
	Also un-highlight linked elements
	@param e QGraphicsSceneHoverEvent
*/
void Element::hoverLeaveEvent(QGraphicsSceneHoverEvent *e)
{
	Q_UNUSED(e)

	foreach (Element *elmt, linkedElements())
		elmt -> setHighlighted(false);

	m_mouse_over = false;
	update();
}

/**
	@brief Element::setUpFormula
	Set up the formula used to create the label of this element
	@param code_letter : Q_UNUSED(code_letter)
	if true set tagged text to code letter
	(ex K for coil) with condition :
	formula is empty, text tagged "label" is emptty or "_";
*/
void Element::setUpFormula(bool code_letter)
{
	Q_UNUSED(code_letter)

	if (linkType() == Element::Slave || linkType() & Element::AllReport)
		return;

	if (diagram())
	{
		QString formula = diagram()
				->project()
				->elementAutoNumCurrentFormula();

		m_data.m_informations.addValue(QStringLiteral("formula"), formula);

		QString element_currentAutoNum = diagram()
				->project()
				->elementCurrentAutoNum();
		NumerotationContext nc = diagram()
				->project()
				->elementAutoNum(element_currentAutoNum);
		NumerotationContextCommands ncc (nc);

		m_autoNum_seq.clear();
		autonum::setSequential(formula,
					   m_autoNum_seq,
					   nc,
					   diagram(),
					   element_currentAutoNum);
		diagram()->project()->addElementAutoNum(element_currentAutoNum,
							ncc.next());

		if(!m_freeze_label && !formula.isEmpty())
		{
			DiagramContext dc = m_data.m_informations;
			m_data.m_informations.addValue(QStringLiteral("label"), actualLabel());
			emit elementInfoChange(dc, m_data.m_informations);
		}
	}
}

/**
	@brief Element::getPrefix
	get Element Prefix
*/
QString Element::getPrefix() const
{
	return m_prefix;
}

/**
	@brief Element::setPrefix
	set Element Prefix
*/
void Element::setPrefix(QString prefix)
{
	m_prefix = std::move(prefix);
}

/**
	@brief Element::freezeLabel
	Freeze this element label
*/
void Element::freezeLabel(bool freeze)
{
	m_freeze_label = freeze;
}

/**
	@brief Element::freezeNewAddedElement
	Freeze this label if needed
*/
void Element::freezeNewAddedElement()
{
	if (this->diagram()->freezeNewElements()
			|| this->diagram()->project()->isFreezeNewElements()) {
		freezeLabel(true);
	}
	else return;
}

/**
	@brief Element::actualLabel
	Always return the current label to be displayed.
	This function is useful when label is based on formula, because label can change at any time.
	@return
*/
QString Element::actualLabel()
{
	if (m_data.m_informations.value(QStringLiteral("formula")).toString().isEmpty()) {
		return m_data.m_informations.value(QStringLiteral("label")).toString();
	} else {
	return autonum::AssignVariables::formulaToLabel(
				m_data.m_informations.value(
					QStringLiteral("formula")).toString(),
				m_autoNum_seq,
				diagram(),
				this);
	}
}

/**
	@brief Element::name
	@return the human name of this element
*/
QString Element::name() const {
	return m_data.m_names_list.name(m_location.baseName());
}

ElementsLocation Element::location() const
{
	return m_location;
}
