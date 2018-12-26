/*
	Copyright 2006-2018 The QElectroTech Team
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
#include "diagram.h"
#include "conductor.h"
#include "diagramcommands.h"
#include <QtDebug>
#include <utility>
#include "elementprovider.h"
#include "diagramposition.h"
#include "terminal.h"
#include "PropertiesEditor/propertieseditordialog.h"
#include "elementpropertieswidget.h"
#include "numerotationcontextcommands.h"
#include "diagramcontext.h"
#include "changeelementinformationcommand.h"
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"
#include "elementpicturefactory.h"
#include "iostream"

class ElementXmlRetroCompatibility
{
	friend class Element;

	static void loadSequential(const QDomElement &dom_element, const QString& seq, QStringList* list)
	{
		int i = 0;
		while (!dom_element.attribute(seq + QString::number(i+1)).isEmpty())
		{
				list->append(dom_element.attribute(seq + QString::number(i+1)));
				i++;
		}
	}

	static void loadSequential(const QDomElement &dom_element, Element *element)
	{
		autonum::sequentialNumbers sn;

		loadSequential(dom_element,"sequ_",&sn.unit);
		loadSequential(dom_element,"sequf_",&sn.unit_folio);
		loadSequential(dom_element,"seqt_",&sn.ten);
		loadSequential(dom_element,"seqtf_",&sn.ten_folio);
		loadSequential(dom_element,"seqh_",&sn.hundred);
		loadSequential(dom_element,"seqhf_",&sn.hundred_folio);

		element->rSequenceStruct() = sn;
	}
};

/**
 * @brief Element::Element
 * @param location, location of this element
 * @param parent, parent graphics item
 * @param state, state of the instanciation
 */
Element::Element(const ElementsLocation &location, QGraphicsItem *parent, int *state, kind link_type) :
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
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
	
	connect(this, &Element::rotationChanged, [this]() {
		for(QGraphicsItem *qgi : childItems())
		{
			if (Terminal *t = qgraphicsitem_cast<Terminal *>(qgi))
				t->updateConductor();
		}
	});
}

/**
 * @brief Element::~Element
 */
Element::~Element()
{
	qDeleteAll (m_dynamic_text_list);
	qDeleteAll (m_terminals);
}

/**
 * @brief Element::terminals
 * @return the list of terminals of this element.
 */
QList<Terminal *> Element::terminals() const {
	return m_terminals;
}

/**
 * @brief Element::conductors
 * @return The list of conductors docked to this element
 * the list is sorted according to the position of the terminal where the conductor is docked
 * from top to bottom, and left to right.
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
		connect(epw, &ElementPropertiesWidget::findEditClicked, &dialog, &QDialog::reject);
			//Must be windowModal, else when user do a drag and drop
			//with the "text" tab of ElementPropertiesWidget, the ui freeze, until user press escape key
		dialog.setWindowModality(Qt::WindowModal);
		dialog.exec();
	}
}

/**
	@param hl true pour mettre l'element en evidence, false sinon
*/
void Element::setHighlighted(bool hl) {
	m_must_highlight = hl;
	update();
}

/**
 * @brief Element::displayHelpLine
 * Display the help line of each terminal if b is true
 * @param b
 */
void Element::displayHelpLine(bool b)
{
	foreach (Terminal *t, terminals())
		t->drawHelpLine(b);
}

/**
 * @brief Element::paint
 * @param painter
 * @param options
 * @param widget
 */
void Element::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *)
{
	if (m_must_highlight) {
		drawHighlight(painter, options);
	}
	
	if (options && options -> levelOfDetail < 1.0) {
		painter->drawPicture(0, 0, m_low_zoom_picture);
	} else {
		painter->drawPicture(0, 0, m_picture);
	}
	
		//Draw the selection rectangle
	if ( isSelected() || m_mouse_over ) {
		drawSelection(painter, options);
	}
}

/**
	@return Le rectangle delimitant le contour de l'element
*/
QRectF Element::boundingRect() const {
	return(QRectF(QPointF(-hotspot_coord.x(), -hotspot_coord.y()), dimensions));
}

/**
 * @brief Element::setSize
 * Define the size of the element.
 * The size must be a multiple of 10.
 * If not, the dimensions indicated will be arrrondies to higher tens.
 * @param wid
 * @param hei 
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
QSize Element::size() const {
	return(dimensions);
}

/**
	Definit le hotspot de l'element par rapport au coin superieur gauche de son rectangle delimitant.
	Necessite que la taille ait deja ete definie
	@param hs Coordonnees du hotspot
*/
QPoint Element::setHotspot(QPoint hs) {
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
QPoint Element::hotspot() const {
	return(hotspot_coord);
}

/**
 * @brief Element::pixmap
 * @return the pixmap of this element
 */
QPixmap Element::pixmap() {
	return ElementPictureFactory::instance()->pixmap(m_location);
}

/*** Methodes protegees ***/

/**
	Dessine un petit repere (axes x et y) relatif a l'element
	@param painter Le QPainter a utiliser pour dessiner les axes
	@param options Les options de style a prendre en compte
*/
void Element::drawAxes(QPainter *painter, const QStyleOptionGraphicsItem *options) {
	Q_UNUSED(options);
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
void Element::drawSelection(QPainter *painter, const QStyleOptionGraphicsItem *options) {
	Q_UNUSED(options);
	painter -> save();
	// Annulation des renderhints
	painter -> setRenderHint(QPainter::Antialiasing,          false);
	painter -> setRenderHint(QPainter::TextAntialiasing,      false);
	painter -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	// Dessin du cadre de selection en gris
	QPen t;
	t.setColor(Qt::gray);
	t.setStyle(Qt::DashDotLine);
	t.setCosmetic(true);
	painter -> setPen(t);
	// Le dessin se fait a partir du rectangle delimitant
	painter -> drawRoundRect(boundingRect().adjusted(1, 1, -1, -1), 10, 10);
	painter -> restore();
}

/**
	Dessine le cadre de selection de l'element de maniere systematiquement non antialiasee.
	@param painter Le QPainter a utiliser pour dessiner les bornes.
	@param options Les options de style a prendre en compte
 */
void Element::drawHighlight(QPainter *painter, const QStyleOptionGraphicsItem *options) {
	Q_UNUSED(options);
	painter -> save();
	
	qreal gradient_radius = qMin(boundingRect().width(), boundingRect().height()) / 2.0;
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
	painter -> drawRoundRect(boundingRect().adjusted(1, 1, -1, -1), 10, 10);
	painter -> restore();
}

/**
 * @brief Element::buildFromXml
 * Build this element from an xml description
 * @param xml_def_elmt
 * @param state
 * Optional pointer which define the status of build
 * 0 - evreything all right
 * 4 - xml isn't a "definition"
 * 5 - attribute of the definition isn't present or valid
 * 6 - the definition is empty
 * 7 - parsing of a xml node who describe a graphical part failed.
 * 8 - No part of the drawing could be loaded
 * @return 
 */
bool Element::buildFromXml(const QDomElement &xml_def_elmt, int *state)
{
	m_state = QET::GIBuildingFromXml;

	if (xml_def_elmt.tagName() != "definition" || xml_def_elmt.attribute("type") != "element")
	{
		if (state) *state = 4;
		m_state = QET::GIOK;
		return(false);
	}

		//Check if the curent version can read the xml description
	if (xml_def_elmt.hasAttribute("version"))
	{
		bool conv_ok;
		qreal element_version = xml_def_elmt.attribute("version").toDouble(&conv_ok);
		if (conv_ok && QET::version.toDouble() < element_version)
		{
			std::cerr << qPrintable(
				QObject::tr("Avertissement : l'élément "
				" a été enregistré avec une version"
				" ultérieure de QElectroTech.")
			) << std::endl;
		}
	}

		//This attribute must be present and valid
	int w, h, hot_x, hot_y;
	if (
		!QET::attributeIsAnInteger(xml_def_elmt, QString("width"), &w) ||\
		!QET::attributeIsAnInteger(xml_def_elmt, QString("height"), &h) ||\
		!QET::attributeIsAnInteger(xml_def_elmt, QString("hotspot_x"), &hot_x) ||\
		!QET::attributeIsAnInteger(xml_def_elmt, QString("hotspot_y"), &hot_y)
	) {
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
		//Extract the names
	m_names.fromXml(xml_def_elmt);
	setToolTip(name());

		//load kind informations
	m_kind_informations.fromXml(xml_def_elmt.firstChildElement("kindInformations"), "kindInformation");
		//load element information
	m_element_informations.fromXml(xml_def_elmt.firstChildElement("elementInformations"), "elementInformation");

		//scroll of the Children of the Definition: Parts of the Drawing
	int parsed_elements_count = 0;
	for (QDomNode node = xml_def_elmt.firstChild() ; !node.isNull() ; node = node.nextSibling())
	{
		QDomElement elmts = node.toElement();
		if (elmts.isNull())
			continue;
		
		if (elmts.tagName() == "description")
		{
				//Minor workaround to find if there is a "input" tagg as label.
				//If not, we set the tagg "label" to the first "input.
			QList <QDomElement> input_field;
			bool have_label = false;
			for (QDomElement input_node = node.firstChildElement("input") ; !input_node.isNull() ; input_node = input_node.nextSiblingElement("input"))
			{
				if (!input_node.isNull())
				{
					input_field << input_node;
					if (input_node.attribute("tagg", "none") == "label")
						have_label = true;
				}
			}
			if(!have_label && !input_field.isEmpty())
				input_field.first().setAttribute("tagg", "label");
			
				//Parse the definition
			for (QDomNode n = node.firstChild() ; !n.isNull() ; n = n.nextSibling())
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
	epf->getPictures(m_location, const_cast<QPicture&>(m_picture), const_cast<QPicture&>(m_low_zoom_picture));
	
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
 * @brief Element::parseElement
 * Parse the element of the xml description of this element
 * @param dom
 * @return 
 */
bool Element::parseElement(const QDomElement &dom)
{
	if      (dom.tagName() == "terminal")     return(parseTerminal(dom));
	else if (dom.tagName() == "input")        return(parseInput(dom));
	else if (dom.tagName() == "dynamic_text") return(parseDynamicText(dom));
	else return(true);
}

/**
 * @brief Element::parseInput
 * Parse the input (old text field)
 * the parsed input are converted to dynamic text field, this function
 * is only here to keep compatibility with old text.
 * @param dom_element
 * @return 
 */
bool Element::parseInput(const QDomElement &dom_element)
{
	qreal pos_x, pos_y;
	int size;
	if (
		!QET::attributeIsAReal(dom_element, "x", &pos_x) ||\
		!QET::attributeIsAReal(dom_element, "y", &pos_y) ||\
		!QET::attributeIsAnInteger(dom_element, "size", &size)
	) return(false);
	else
	{
		DynamicElementTextItem *deti = new DynamicElementTextItem(this);
		deti->setText(dom_element.attribute("text", "_"));
		deti->setFontSize(dom_element.attribute("size", QString::number(9)).toInt());
		deti->setRotation(dom_element.attribute("rotation", QString::number(0)).toDouble());

		if(dom_element.attribute("tagg", "none") != "none")
		{
			deti->setTextFrom(DynamicElementTextItem::ElementInfo);
			deti->setInfoName(dom_element.attribute("tagg"));
		}
		
			//the origin transformation point of PartDynamicTextField is the top left corner, no matter the font size
			//The origin transformation point of ElementTextItem is the middle of left edge, and so by definition, change with the size of the font
			//We need to use a QMatrix to find the pos of this text from the saved pos of text item 
		QMatrix matrix;
			//First make the rotation
		matrix.rotate(dom_element.attribute("rotation", "0").toDouble());
		QPointF pos = matrix.map(QPointF(0, -deti->boundingRect().height()/2));
		matrix.reset();
			//Second translate to the pos
		QPointF p(dom_element.attribute("x", QString::number(0)).toDouble(),
				  dom_element.attribute("y", QString::number(0)).toDouble());
		matrix.translate(p.x(), p.y());
		deti->setPos(matrix.map(pos));
		m_converted_text_from_xml_description.insert(deti, p);
		return true;
	}
	
	return false;
}

/**
 * @brief Element::parseDynamicText
 * Create the dynamic text field describ in @dom_element
 * @param dom_element
 * @return 
 */
DynamicElementTextItem *Element::parseDynamicText(const QDomElement &dom_element)
{
	DynamicElementTextItem *deti = new DynamicElementTextItem(this);
		//Because the xml description of a .elmt file is the same as how a dynamic text field is save to xml in a .qet file
		//wa call fromXml, we just change the tagg name (.elmt = dynamic_text, .qet = dynamic_elmt_text)
		//and the uuid (because the uuid, is the uuid of the descritpion and not the uuid of instantiated dynamic text field)
	
	QDomElement dom(dom_element.cloneNode(true).toElement());
	dom.setTagName(DynamicElementTextItem::xmlTaggName());
	deti->fromXml(dom);
	deti->m_uuid = QUuid::createUuid();
	this->addDynamicTextItem(deti);
	return deti;
}

Terminal *Element::parseTerminal(const QDomElement &dom_element)
{
	qreal terminalx, terminaly;
	Qet::Orientation terminalo;
	if (!QET::attributeIsAReal(dom_element, QString("x"), &terminalx)) {
		return(nullptr);
	}
	if (!QET::attributeIsAReal(dom_element, QString("y"), &terminaly)) {
		return(nullptr);
	}
	if (!dom_element.hasAttribute("orientation")) {
		return(nullptr);
	}
	if (dom_element.attribute("orientation") == "n") {
		terminalo = Qet::North;
	}
	else if (dom_element.attribute("orientation") == "s") {
		terminalo = Qet::South;
	}
	else if (dom_element.attribute("orientation") == "e") {
		terminalo = Qet::East;
	}
	else if (dom_element.attribute("orientation") == "w") {
		terminalo = Qet::West;
	}
	else {
		return(nullptr);
	}
	
	Terminal *new_terminal = new Terminal(terminalx, terminaly, terminalo, this);
	m_terminals << new_terminal;
	
		//Sort from top to bottom and left to rigth
	std::sort(m_terminals.begin(), m_terminals.end(), [](Terminal *a, Terminal *b)
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
bool Element::valideXml(QDomElement &e) {
	// verifie le nom du tag
	if (e.tagName() != "element") return(false);
	
	// verifie la presence des attributs minimaux
	if (!e.hasAttribute("type")) return(false);
	if (!e.hasAttribute("x"))    return(false);
	if (!e.hasAttribute("y"))    return(false);
	
	bool conv_ok;
	// parse l'abscisse
	e.attribute("x").toDouble(&conv_ok);
	if (!conv_ok) return(false);
	
	// parse l'ordonnee
	e.attribute("y").toDouble(&conv_ok);
	if (!conv_ok) return(false);
	return(true);
}

/**
 * @brief Element::fromXml
 * Import the parameters of this element from a xml document.
 * When call this function ensure this element is already in a scene, because
 * the dynamic text item and element text item group (in the xml file) are created in this function
 * and need a diagram for create their Xref, when this element is linked to another.
 * If not the Xref can be not displayed, until the next call of update Xref of the group or text item.
 * @param e : the dom element where the parameter is stored
 * @param table_id_adr : Reference to the mapping table between IDs of the XML file
 * and the addresses in memory. If the import succeeds, it must be add the right couples (id, address).
 * @param handle_inputs_rotation : apply the rotation of this element to his child text
 * @return 
 */
bool Element::fromXml(QDomElement &e, QHash<int, Terminal *> &table_id_adr, bool handle_inputs_rotation)
{
	m_state = QET::GILoadingFromXml;
	/*
		les bornes vont maintenant etre recensees pour associer leurs id a leur adresse reelle
		ce recensement servira lors de la mise en place des fils
	*/
	QList<QDomElement> liste_terminals;
	foreach(QDomElement qde, QET::findInDomElement(e, "terminals", "terminal")) {
		if (Terminal::valideXml(qde)) liste_terminals << qde;
	}
	
	QHash<int, Terminal *> priv_id_adr;
	int terminals_non_trouvees = 0;
	foreach(QGraphicsItem *qgi, childItems()) {
		if (Terminal *p = qgraphicsitem_cast<Terminal *>(qgi)) {
			bool terminal_trouvee = false;
			foreach(QDomElement qde, liste_terminals) {
				if (p -> fromXml(qde)) {
					priv_id_adr.insert(qde.attribute("id").toInt(), p);
					terminal_trouvee = true;
					// We used to break here, because we did not expect
					// several terminals to share the same position.
					// Of course, it finally happened.
				}
			}
			if (!terminal_trouvee) ++ terminals_non_trouvees;
		}
	}
	
	if (terminals_non_trouvees > 0)
	{
		m_state = QET::GIOK;
		return(false);
	} 
	else
	{
		// verifie que les associations id / adr n'entrent pas en conflit avec table_id_adr
		foreach(int id_trouve, priv_id_adr.keys())
		{
			if (table_id_adr.contains(id_trouve))
			{
				// cet element possede un id qui est deja reference (= conflit)
				m_state = QET::GIOK;
				return(false);
			}
		}
		// copie des associations id / adr
		foreach(int id_trouve, priv_id_adr.keys()) {
			table_id_adr.insert(id_trouve, priv_id_adr.value(id_trouve));
		}
	}

	//load uuid of connected elements
	QList <QDomElement> uuid_list = QET::findInDomElement(e, "links_uuids", "link_uuid");
	foreach (QDomElement qdo, uuid_list) tmp_uuids_link << qdo.attribute("uuid");
	
	//uuid of this element
	m_uuid= QUuid(e.attribute("uuid", QUuid::createUuid().toString()));

		//load prefix
	m_prefix = e.attribute("prefix");

	QString fl = e.attribute("freezeLabel", "false");
	m_freeze_label = fl == "false"? false : true;

		//Load Sequential Values
	if (e.hasAttribute("sequ_1") || e.hasAttribute("sequf_1") || e.hasAttribute("seqt_1") || e.hasAttribute("seqtf_1") || e.hasAttribute("seqh_1") || e.hasAttribute("sequf_1"))
		ElementXmlRetroCompatibility::loadSequential(e, this);
	else
		m_autoNum_seq.fromXml(e.firstChildElement("sequentialNumbers"));

		//Position and selection.
		//We directly call setPos from QGraphicsObject, because QetGraphicsItem will snap to grid
	QGraphicsObject::setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setZValue(e.attribute("z", QString::number(this->zValue())).toDouble());
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	
	// orientation
	bool conv_ok;
	int read_ori = e.attribute("orientation").toInt(&conv_ok);
	if (!conv_ok || read_ori < 0 || read_ori > 3) read_ori = 0;
	if (handle_inputs_rotation) {
		setRotation(rotation() + (90*read_ori));
	} else {
		setRotation(90*read_ori);
	}
	
		//Befor load the dynamic text field,
		//we remove the dynamic text field created from the description of this element, to avoid doublons.
	for(DynamicElementTextItem *deti : m_dynamic_text_list)
		delete deti;
	m_dynamic_text_list.clear();
    
		//************************//
		//***Dynamic texts item***//
		//************************//
    for (const QDomElement& qde : QET::findInDomElement(e, "dynamic_texts", DynamicElementTextItem::xmlTaggName()))
    {
        DynamicElementTextItem *deti = new DynamicElementTextItem(this);
        addDynamicTextItem(deti);
        deti->fromXml(qde);
    }
	

		//************************//
		//***Element texts item***//
		//************************//
	QList<QDomElement> inputs = QET::findInDomElement(e, "inputs", "input");
	
		//First case, we check for the text item converted to dynamic text item
	const QList <DynamicElementTextItem *> conv_deti_list = m_converted_text_from_xml_description.keys();
	QList <DynamicElementTextItem *> successfully_converted; 
	const QList <QDomElement> dom_inputs = inputs;
	
	for (DynamicElementTextItem *deti : conv_deti_list)
	{
		for(const QDomElement& dom_input : dom_inputs)
		{
				//we use the same method used in ElementTextItem::fromXml to compar and know if the input dom element is for one of the text stored.
				//The comparaison is made from the text position : if the position of the text is the same as the position stored in 'input' dom element
				//that mean this is the good text
			if (qFuzzyCompare(qreal(dom_input.attribute("x").toDouble()), m_converted_text_from_xml_description.value(deti).x()) &&
				qFuzzyCompare(qreal(dom_input.attribute("y").toDouble()), m_converted_text_from_xml_description.value(deti).y()))
			{
					//Once again this 'if', is only for retrocompatibility with old old old project
					//when element text with tagg "label" is not null, but the element information "label" is. 
				if((deti->textFrom() == DynamicElementTextItem::ElementInfo) && (deti->infoName() == "label"))
					m_element_informations.addValue("label", dom_input.attribute("text"));
				
				deti->setText(dom_input.attribute("text"));
				
				qreal rotation = deti->rotation();
				QPointF xml_pos = m_converted_text_from_xml_description.value(deti);
				
				if (dom_input.attribute("userrotation").toDouble())
					rotation = dom_input.attribute("userrotation").toDouble();
				
				if (dom_input.hasAttribute("userx"))
					xml_pos.setX(dom_input.attribute("userx").toDouble());
				if(dom_input.hasAttribute("usery"))
					xml_pos.setY(dom_input.attribute("usery", "0").toDouble());
				
					//the origin transformation point of PartDynamicTextField is the top left corner, no matter the font size
					//The origin transformation point of PartTextField is the middle of left edge, and so by definition, change with the size of the font
					//We need to use a QMatrix to find the pos of this text from the saved pos of text item
				
				deti->setPos(xml_pos);
				deti->setRotation(rotation);
				
				QMatrix matrix;
					//First make the rotation
				matrix.rotate(rotation);
				QPointF pos = matrix.map(QPointF(0, -deti->boundingRect().height()/2));
				matrix.reset();
					//Second translate to the pos
				matrix.translate(xml_pos.x(), xml_pos.y());
				deti->setPos(matrix.map(pos));
				
					//dom_input and deti matched we remove the dom_input from @inputs list,
					//to avoid unnecessary checking made below
					//we also move deti from the m_converted_text_from_xml_description to m_dynamic_text_list
				inputs.removeAll(dom_input);
				m_dynamic_text_list.append(deti);
				m_converted_text_from_xml_description.remove(deti);
				successfully_converted << deti;
			}
		}
	}
	
		//###Firts case : if this is the first time the user open the project since text item are converted to dynamic text,
		//in the previous opening of the project, every texts field present in the element description was created.
		//At save time, the values of each of them was save in the 'input' dom element.
		//The loop upper is made for the first case, to import the values in 'input' to the new converted dynamic texts field.
		//###Second case : this is not the first time the user open the project since text item are converted to dynamic text.
		//That mean, in a previous opening of the project, the text item was already converted and save as a dynamic text field.
		//So there isn't 'input' dom element in the project, and every dynamic text item present in m_converted_text_from_xml_description
		//need to be deleted (because already exist in m_dynamic_text_list, from a previous save)
	for (DynamicElementTextItem *deti : m_converted_text_from_xml_description.keys())
		delete deti;
	m_converted_text_from_xml_description.clear();
	
	for (QDomElement qde : QET::findInDomElement(e, "texts_groups", ElementTextItemGroup::xmlTaggName()))
	{
		ElementTextItemGroup *group = addTextGroup("loaded_from_xml_group");
		group->fromXml(qde);
	}
	
		//load informations
	DiagramContext dc;
	dc.fromXml(e.firstChildElement("elementInformations"), "elementInformation");
		/**
		 * Since the commit 4791, the value used as "label" and "formula" is stored in differents keys (instead of the same key, "label" in previous version),
		 * so, if "label" contain "%" (Use variable value), and "formula" does not exist,
		 * this mean the label was made before commit 4791 (0.51 dev). So we swap the value stored in "label" to "formula" as expected.
		 * @TODO remove this code at version 0.7 or more (probably useless).
		 */
	if (dc["label"].toString().contains("%") && dc["formula"].toString().isNull())
	{
		dc.addValue("formula", dc["label"]);
	}
		//retrocompatibility with older version
	if(dc.value("label").toString().isEmpty() &&
	   !m_element_informations.value("label").toString().isEmpty())
		dc.addValue("label", m_element_informations.value("label"));
	
		//We must to block the update of the alignment when load the information
		//otherwise the pos of the text will not be the same as it was at save time.
	for(DynamicElementTextItem *deti : m_dynamic_text_list)
		deti->m_block_alignment = true;
	setElementInformations(dc);
	for(DynamicElementTextItem *deti : m_dynamic_text_list)
		deti->m_block_alignment = false;
	
	
	/**
	  During the devel of the version 0.7, the "old text" was replaced by the dynamic element text item.
	  When open a project made befor the 0.7, we must to reproduce the same visual when the label are not empty and visible,
      and comment are not empty and visible and/or location are not empty and visible.
	  we create a text group with inside the needed texts, label and comment and/or location.
	  */
		//#1 There must be old text converted to dynamic text
	if(!successfully_converted.isEmpty())
	{
			//#2 the element information must have label not empty and visible
			//and a least comment or location not empty and visible
		QString label = m_element_informations.value("label").toString();
		QString comment = m_element_informations.value("comment").toString();
		QString location = m_element_informations.value("location").toString();
		bool la = m_element_informations.keyMustShow("label");
		bool c = m_element_informations.keyMustShow("comment");
		bool lo = m_element_informations.keyMustShow("location");
		
		if((m_link_type != Master) ||
		   ((m_link_type == Master) &&
			(diagram()->project()->defaultXRefProperties(m_kind_informations["type"].toString()).snapTo() == XRefProperties::Label))
		   )
		{
			if(!label.isEmpty() && la &&
			   ((!comment.isEmpty() && c) || (!location.isEmpty() && lo)))
			{
					//#2 in the converted list one text must have text from = element info and info name = label
				for(DynamicElementTextItem *deti : successfully_converted)
				{
					if(deti->textFrom() == DynamicElementTextItem::ElementInfo && deti->infoName() == "label")
					{
						qreal rotation = deti->rotation();
						
							//Create the comment item
						DynamicElementTextItem *comment_text = nullptr;
						if (m_link_type !=PreviousReport || m_link_type !=NextReport)
						{
							m_state = QET::GIOK;
							return(true);
						}
						if(!comment.isEmpty() && c)
						{
							comment_text = new DynamicElementTextItem(this);
							comment_text->setTextFrom(DynamicElementTextItem::ElementInfo);
							comment_text->setInfoName("comment");
							comment_text->setFontSize(6);
							comment_text->setFrame(true);
							if(comment_text->toPlainText().count() > 17)
								comment_text->setTextWidth(80);
							comment_text->setPos(deti->x(), deti->y()+10); //+10 is arbitrary, comment_text must be below deti
							addDynamicTextItem(comment_text);
						}
							//create the location item
						DynamicElementTextItem *location_text = nullptr;
						if (m_link_type !=PreviousReport || m_link_type !=NextReport)
						{
							m_state = QET::GIOK;
							return(true);
						}
						if(!location.isEmpty() && lo)
						{
							location_text = new DynamicElementTextItem(this);
							location_text->setTextFrom(DynamicElementTextItem::ElementInfo);
							location_text->setInfoName("location");
							location_text->setFontSize(6);
							if(location_text->toPlainText().count() > 17)
								location_text->setTextWidth(80);
							location_text->setPos(deti->x(), deti->y()+20); //+20 is arbitrary, location_text must be below deti and comment
							addDynamicTextItem(location_text);
						}
						
						QPointF pos = deti->pos();
						if (m_link_type !=PreviousReport || m_link_type !=NextReport)
						{
							m_state = QET::GIOK;
							return(true);
						}
							//Create the group
						ElementTextItemGroup *group = addTextGroup(tr("Label + commentaire"));
						addTextToGroup(deti, group);
						if(comment_text)
							addTextToGroup(comment_text, group);
						if(location_text)
							addTextToGroup(location_text, group);
						group->setAlignment(Qt::AlignVCenter);
						group->setVerticalAdjustment(-4);
						group->setRotation(rotation);
							//Change the position of the group, so that the text "label" stay in the same
							//position in scene coordinate
						group->setPos(pos - deti->pos());
						
						break;
					}
				}
			}
		}
		else
		{
				//This element is supposed to be a master and Xref property snap to bottom
			if((!comment.isEmpty() && c) || (!location.isEmpty() && lo))
			{
					//Create the comment item
				DynamicElementTextItem *comment_text = nullptr;
				if(!comment.isEmpty() && c)
				{
					comment_text = new DynamicElementTextItem(this);
					comment_text->setTextFrom(DynamicElementTextItem::ElementInfo);
					comment_text->setInfoName("comment");
					comment_text->setFontSize(6);
					comment_text->setFrame(true);
					comment_text->setTextWidth(80);
					addDynamicTextItem(comment_text);
				}
					//create the location item
				DynamicElementTextItem *location_text = nullptr;
				if(!location.isEmpty() && lo)
				{
					location_text = new DynamicElementTextItem(this);
					location_text->setTextFrom(DynamicElementTextItem::ElementInfo);
					location_text->setInfoName("location");
					location_text->setFontSize(6);
					location_text->setTextWidth(80);
					if(comment_text)
						location_text->setPos(comment_text->x(), comment_text->y()+10); //+10 is arbitrary, location_text must be below the comment
					addDynamicTextItem(location_text);
				}
				
					//Create the group
				ElementTextItemGroup *group = addTextGroup(tr("Label + commentaire"));
				if(comment_text)
					addTextToGroup(comment_text, group);
				if(location_text)
					addTextToGroup(location_text, group);
				group->setAlignment(Qt::AlignVCenter);
				group->setVerticalAdjustment(-4);
				group->setHoldToBottomPage(true);
			}
		}
	}
    m_state = QET::GIOK;
	return(true);
}

/**
	Permet d'exporter l'element en XML
	@param document Document XML a utiliser
	@param table_adr_id Table de correspondance entre les adresses des bornes
	et leur id dans la representation XML ; cette table completee par cette
	methode
	@return L'element XML representant cet element electrique
*/
QDomElement Element::toXml(QDomDocument &document, QHash<Terminal *, int> &table_adr_id) const
{
	QDomElement element = document.createElement("element");
	
		// type
	element.setAttribute("type", m_location.path());

		// uuid
	element.setAttribute("uuid", uuid().toString());

		// prefix
	element.setAttribute("prefix", m_prefix);

		//frozen label
	element.setAttribute("freezeLabel", m_freeze_label? "true" : "false");

		// sequential num
	QDomElement seq = m_autoNum_seq.toXml(document);
	if (seq.hasChildNodes())
		element.appendChild(seq);
	
	// position, selection et orientation
	element.setAttribute("x", QString::number(pos().x()));
	element.setAttribute("y", QString::number(pos().y()));
	element.setAttribute("z", QString::number(this->zValue()));
	element.setAttribute("orientation", QString::number(orientation()));
	
	/* recupere le premier id a utiliser pour les bornes de cet element */
	int id_terminal = 0;
	if (!table_adr_id.isEmpty()) {
		// trouve le plus grand id
		int max_id_t = -1;
		foreach (int id_t, table_adr_id.values()) {
			if (id_t > max_id_t) max_id_t = id_t;
		}
		id_terminal = max_id_t + 1;
	}
	
	// enregistrement des bornes de l'appareil
	QDomElement xml_terminals = document.createElement("terminals");
	// pour chaque enfant de l'element
	foreach(Terminal *t, terminals()) {
		// alors on enregistre la borne
		QDomElement terminal = t -> toXml(document);
		terminal.setAttribute("id", id_terminal);
		table_adr_id.insert(t, id_terminal ++);
		xml_terminals.appendChild(terminal);
	}
	element.appendChild(xml_terminals);
	
	// enregistrement des champ de texte de l'appareil
	QDomElement inputs = document.createElement("inputs");
	element.appendChild(inputs);

	//if this element is linked to other elements,
	//save the uuid of each other elements
	if (! isFree()) {
		QDomElement links_uuids = document.createElement("links_uuids");
		foreach (Element *elmt, connected_elements) {
			QDomElement link_uuid = document.createElement("link_uuid");
			link_uuid.setAttribute("uuid", elmt->uuid().toString());
			links_uuids.appendChild(link_uuid);
		}
		element.appendChild(links_uuids);
	}

        //save information of this element
	if (! m_element_informations.keys().isEmpty()) {
		QDomElement infos = document.createElement("elementInformations");
		m_element_informations.toXml(infos, "elementInformation");
		element.appendChild(infos);
	}
    	
        //Dynamic texts
    QDomElement dyn_text = document.createElement("dynamic_texts");
    for (DynamicElementTextItem *deti : m_dynamic_text_list)
		dyn_text.appendChild(deti->toXml(document));
	
	QDomElement texts_group = document.createElement("texts_groups");
	
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
		
			//Restor the alignement
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
 * @brief Element::addDynamiqueTextItem
 * Add @deti as a dynamic text item of this element, @deti is reparented to this
 * If @deti is null, a new DynamicElementTextItem is created and added to this element.
 * @param deti
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
 * @brief Element::removeDynamicTextItem
 * Remove @deti, no matter if is a child of this element or a child of a group of this element.
 * Set he parent item of @deti to 0, @deti is not deleted.
 * @param deti
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
 * @brief Element::dynamicTextItems
 * @return all dynamic text items of this element directly child of this element.
 * Texts in text-groups belonging to this element are not returned by this function.
 * @see ElementTextItemGroup::texts
 */
QList<DynamicElementTextItem *> Element::dynamicTextItems() const {
	return m_dynamic_text_list;
}

/**
 * @brief Element::addTextGroup
 * Create and add an element text item group to this element.
 * If this element already have a group with the same name,
 * then @name will renamed to name1 or name2 etc....
 * @param name : the name of the group
 * @return the created group.
 */
ElementTextItemGroup *Element::addTextGroup(const QString &name)
{
	if(m_texts_group.isEmpty())
	{
		ElementTextItemGroup *group = new ElementTextItemGroup(name, this);
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
 * @brief Element::addTextGroup
 * @param group add group @group to the group of this element.
 * the group must not be owned by an element.
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
 * @brief Element::removeTextGroup
 * Remove the text group @group from this element, and set the parent of group to 0.
 * group is not deleted.
 * All texts owned by the group will be reparented to this element
 * @param name
 */
void Element::removeTextGroup(ElementTextItemGroup *group)
{
	if(!m_texts_group.contains(group))
		return;
	
	const QList <QGraphicsItem *> items_list = group->childItems();
	
	for(QGraphicsItem *qgi : items_list)
	{
		if(qgi->type() == DynamicElementTextItem::Type)
		{
			DynamicElementTextItem *deti = static_cast<DynamicElementTextItem *>(qgi);
			removeTextFromGroup(deti, group);
		}
	}
	
	
	emit textsGroupAboutToBeRemoved(group);
	m_texts_group.removeOne(group);
	group->setParentItem(nullptr);
}

/**
 * @brief Element::textGroup
 * @param name
 * @return the text group named @name or nullptr if this element
 * haven't got a group with this name
 */
ElementTextItemGroup *Element::textGroup(const QString &name) const
{
	for (ElementTextItemGroup *group : m_texts_group)
		if(group->name() == name)
			return group;
	
	return nullptr;
}

/**
 * @brief Element::textGroups
 * @return All texts groups of this element
 */
QList<ElementTextItemGroup *> Element::textGroups() const
{
	return m_texts_group;
}

/**
 * @brief Element::addTextToGroup
 * Add the text @text to the group @group;
 * If @group isn't owned by this element return false.
 * The text must be a text of this element.
 * @return : true if the text was succesfully added to the group.
 */
bool Element::addTextToGroup(DynamicElementTextItem *text, ElementTextItemGroup *group)
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
 * @brief Element::removeTextFromGroup
 * Remove the text @text from the group @group, en reparent @text to this element
 * @return true if text was succesfully removed
 */
bool Element::removeTextFromGroup(DynamicElementTextItem *text, ElementTextItemGroup *group)
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
 * @brief Element::AlignedFreeTerminals
 * @return a list of terminal (owned by this element) aligned to other terminal (from other element)
 * The first Terminal of QPair is a Terminal owned by this element,
 * this terminal haven't got any conductor docked.
 * The second Terminal of QPair is a Terminal owned by an other element,
 * which is aligned with the first Terminal. The second Terminal can have or not docked conductors.
 */
QList <QPair <Terminal *, Terminal *> > Element::AlignedFreeTerminals() const
{
	QList <QPair <Terminal *, Terminal *> > list;

	foreach (Terminal *terminal, terminals())
	{
		if (terminal->conductors().isEmpty())
		{
			Terminal *other_terminal = terminal -> alignedWithTerminal();
			if (other_terminal)
				list << qMakePair(terminal, other_terminal);
		}
	}

	return list;
}

/**
 * @brief Element::initLink
 * Initialise the link between this element and other elements.
 * This method can be call once because init the link according to
 * uuid store in a private list, after link, the list is clear, so
 * call another time do nothing.
 *
 * @param prj, ownership project of this element and other element to be linked
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
 * @brief Element::setElementInformations
 * Set new information for this element.
 * If new information is different of current infotmation emit @elementInfoChange
 * @param dc
 */
void Element::setElementInformations(DiagramContext dc)
{
	if (m_element_informations == dc) return;
	DiagramContext old_info = m_element_informations;
	m_element_informations = dc;
	emit elementInfoChange(old_info, m_element_informations);
}

/**
 * @brief comparPos
 * Compare position of the two elements. Compare 3 points:
 * 1 folio - 2 row - 3 line
 * returns a response when a comparison is found.
 * @return true if elmt1 is at lower position than elmt 2, else false
 */
bool comparPos(const Element *elmt1, const Element *elmt2) {
	//Compare folio first
	if (elmt1->diagram()->folioIndex() != elmt2->diagram()->folioIndex())
		return elmt1->diagram()->folioIndex() < elmt2->diagram()->folioIndex();
	//Compare the row(in letter pos) in second
	QString a = elmt1->diagram()->convertPosition(elmt1->scenePos()).letter();
	QString b = elmt2->diagram()->convertPosition(elmt2->scenePos()).letter();
	if (a != b)
		return a<b;
	//In last compare the line, if line is egal, return sorted by row in real pos
	if (elmt1->pos().x() == elmt2->pos().x())
		return elmt1->y() <= elmt2->pos().y();
	return elmt1->pos().x() <= elmt2->pos().x();
}

/**
 * @brief Element::mouseMoveEvent
 * @param event
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
 * @brief Element::mouseReleaseEvent
 * @param event
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
 * When mouse over element
 * change m_mouse_over to true   (used in paint() function )
 * Also highlight linked elements
 * @param e QGraphicsSceneHoverEvent
*/
void Element::hoverEnterEvent(QGraphicsSceneHoverEvent *e) {
	Q_UNUSED(e);

	foreach (Element *elmt, linkedElements())
		elmt -> setHighlighted(true);

	m_mouse_over = true;
	setToolTip( name() );
	update();
}

/**
 * When mouse over element leave the position
 * change m_mouse_over to false(used in paint() function )
 * Also un-highlight linked elements
 * @param e QGraphicsSceneHoverEvent
*/
void Element::hoverLeaveEvent(QGraphicsSceneHoverEvent *e) {
	Q_UNUSED(e);

	foreach (Element *elmt, linkedElements())
		elmt -> setHighlighted(false);

	m_mouse_over = false;
	update();
}

/**
 * @brief Element::setUpFormula
 * Set up the formula used to create the label of this element
 * @param : if true set tagged text to code letter (ex K for coil) with condition :
 * formula is empty, text tagged "label" is emptty or "_";
 */
void Element::setUpFormula(bool code_letter)
{
	Q_UNUSED(code_letter)
	
	if (linkType() == Element::Slave || linkType() & Element::AllReport)
		return;

	if (diagram())
	{
		QString formula = diagram()->project()->elementAutoNumCurrentFormula();

		m_element_informations.addValue("formula", formula);
		
		QString element_currentAutoNum = diagram()->project()->elementCurrentAutoNum();
		NumerotationContext nc = diagram()->project()->elementAutoNum(element_currentAutoNum);
		NumerotationContextCommands ncc (nc);
		
		m_autoNum_seq.clear();
		autonum::setSequential(formula, m_autoNum_seq, nc, diagram(), element_currentAutoNum);
		diagram()->project()->addElementAutoNum(element_currentAutoNum, ncc.next());

		if(!m_freeze_label && !formula.isEmpty())
		{
			DiagramContext dc = m_element_informations;
			QString label = autonum::AssignVariables::formulaToLabel(formula, m_autoNum_seq, diagram(), this);
			m_element_informations.addValue("label", label);
			emit elementInfoChange(dc, m_element_informations);
		}
	}
}

/**
 * @brief Element::getPrefix
 * get Element Prefix
 */
QString Element::getPrefix() const{
	return m_prefix;
}

/**
 * @brief Element::setPrefix
 * set Element Prefix
 */
void Element::setPrefix(QString prefix) {
	m_prefix = std::move(prefix);
}

/**
 * @brief Element::freezeLabel
 * Freeze this element label
 */
void Element::freezeLabel(bool freeze)
{
	m_freeze_label = freeze;
}

/**
 * @brief Element::freezeNewAddedElement
 * Freeze this label if needed
 */
void Element::freezeNewAddedElement() {
	if (this->diagram()->freezeNewElements() || this->diagram()->project()->isFreezeNewElements()) {
		freezeLabel(true);
	}
	else return;
}

/**
 * @brief Element::name
 * @return the human name of this element
 */
QString Element::name() const {
	return m_names.name(m_location.baseName());
}

ElementsLocation Element::location() const {
	return m_location;
}
