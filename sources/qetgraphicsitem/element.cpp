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
#include "element.h"
#include "diagram.h"
#include "conductor.h"
#include "elementtextitem.h"
#include "diagramcommands.h"
#include <QtDebug>
#include "elementprovider.h"
#include "diagramposition.h"
#include "terminal.h"
#include "PropertiesEditor/propertieseditordialog.h"
#include "elementpropertieswidget.h"
#include "numerotationcontextcommands.h"
#include "diagramcontext.h"

/**
	Constructeur pour un element sans scene ni parent
*/
Element::Element(QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	must_highlight_(false),
	m_mouse_over(false)
{
	link_type_ = Simple;
	uuid_ = QUuid::createUuid();
	setZValue(10);
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
}

/**
	Destructeur
*/
Element::~Element() {
}

void Element::editProperty()
{
	if (diagram() && !diagram()->isReadOnly())
	{
		ElementPropertiesWidget *epw = new ElementPropertiesWidget(this);
		PropertiesEditorDialog dialog(epw, QApplication::activeWindow());
		connect(epw, &ElementPropertiesWidget::findEditClicked, &dialog, &QDialog::reject);
		dialog.exec();
	}
}



/**
	@return true si l'element est mis en evidence
*/
bool Element::isHighlighted() const {
	return(must_highlight_);
}

/**
	@param hl true pour mettre l'element en evidence, false sinon
*/
void Element::setHighlighted(bool hl) {
	must_highlight_ = hl;
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

	if (must_highlight_) drawHighlight(painter, options);
	
		//Draw the element himself
	paint(painter, options);
	
		//Draw the selection rectangle
	if ( isSelected() || m_mouse_over ) drawSelection(painter, options);
}

/**
	@return Le rectangle delimitant le contour de l'element
*/
QRectF Element::boundingRect() const {
	return(QRectF(QPointF(-hotspot_coord.x(), -hotspot_coord.y()), dimensions));
}

/**
	Definit la taille de l'element sur le schema. Les tailles doivent etre
	des multiples de 10 ; si ce n'est pas le cas, les dimensions indiquees
	seront arrrondies aux dizaines superieures.
	@param wid Largeur de l'element
	@param hei Hauteur de l'element
	@return La taille finale de l'element
*/
QSize Element::setSize(int wid, int hei) {
	prepareGeometryChange();
	// chaque dimension indiquee est arrondie a la dizaine superieure
	while (wid % 10) ++ wid;
	while (hei % 10) ++ hei;
	// les dimensions finales sont conservees et retournees
	return(dimensions = QSize(wid, hei));
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
	Selectionne l'element
*/
void Element::select() {
	setSelected(true);
}

/**
	Deselectionne l'element
*/
void Element::deselect() {
	setSelected(false);
}

/**
	@return La pixmap de l'element
*/
QPixmap Element::pixmap() {
	if (preview.isNull()) updatePixmap(); // on genere la pixmap si ce n'est deja fait
	return(preview);
}

/**
 * @brief Element::rotateBy
 * this methode is redefined for handle child item
 * @param angle
 */
void Element::rotateBy(const qreal &angle) {
	qreal applied_angle = QET::correctAngle(angle);
	applyRotation(applied_angle + rotation());

	//update the path of conductor
	foreach(QGraphicsItem *qgi, childItems()) {
		if (Terminal *p = qgraphicsitem_cast<Terminal *>(qgi)) {
			p -> updateConductor();
		}
	}

	// repositionne les textes de l'element qui ne comportent pas l'option "FollowParentRotations"
	foreach(ElementTextItem *eti, texts()) {
		if (!eti -> followParentRotations())  {
			// on souhaite pivoter le champ de texte par rapport a son centre
			QPointF eti_center = eti -> boundingRect().center();
			// pour ce faire, on repere la position de son centre par rapport a son parent
			QPointF parent_eti_center_before = eti -> mapToParent(eti_center);
			// on applique ensuite une simple rotation contraire, qui sera donc appliquee sur le milieu du cote gauche du champ de texte
			eti -> rotateBy(-applied_angle);
			// on regarde ensuite la nouvelle position du centre du champ de texte par rapport a son parent
			QPointF parent_eti_center_after = eti -> mapToParent(eti_center);
			// on determine la translation a appliquer
			QPointF eti_translation = parent_eti_center_before - parent_eti_center_after;
			// on applique cette translation
			eti -> setPos(eti -> pos() + eti_translation);
		}
	}
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
	Fonction initialisant et dessinant la pixmap de l'element.
*/
void Element::updatePixmap() {
	// Pixmap transparente faisant la taille de base de l'element
	preview = QPixmap(dimensions);
	preview.fill(QColor(255, 255, 255, 0));
	// QPainter sur la pixmap, avec antialiasing
	QPainter p(&preview);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);
	// Translation de l'origine du repere de la pixmap
	p.translate(hotspot_coord);
	// L'element se dessine sur la pixmap
	paint(&p, 0);
}

/**
	This class is used to retrieve label and function information from element
	and add it to Diagram Context. Used to make older versions work correctly
	@param Element Text item to check information
*/
void Element::etiToElementLabels(ElementTextItem *eti) {
	if (eti->tagg() == "label" && eti->toPlainText()!= "_") {
		DiagramContext &dc = this->rElementInformations();
		dc.addValue("label", eti->toPlainText());
		this->setElementInformations(dc);
		this->setTaggedText("label", eti->toPlainText());
	}
	else if(eti->tagg() == "function" && eti->toPlainText() != "_") {
		DiagramContext &dc = this->rElementInformations();
		dc.addValue("function", eti->toPlainText());
		this->setElementInformations(dc);
	}
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
	Methode d'import XML. Cette methode est appelee lors de l'import de contenu
	XML (coller, import, ouverture de fichier...) afin que l'element puisse
	gerer lui-meme l'importation de ses bornes. Ici, comme cette classe est
	caracterisee par un nombre fixe de bornes, l'implementation exige de
	retrouver exactement ses bornes dans le fichier XML.
	@param e L'element XML a analyser.
	@param table_id_adr Reference vers la table de correspondance entre les IDs
	du fichier XML et les adresses en memoire. Si l'import reussit, il faut y
	ajouter les bons couples (id, adresse).
	@return true si l'import a reussi, false sinon
	
*/
bool Element::fromXml(QDomElement &e, QHash<int, Terminal *> &table_id_adr, bool handle_inputs_rotation) {
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
	
	if (terminals_non_trouvees > 0) {
		return(false);
	} else {
		// verifie que les associations id / adr n'entrent pas en conflit avec table_id_adr
		foreach(int id_trouve, priv_id_adr.keys()) {
			if (table_id_adr.contains(id_trouve)) {
				// cet element possede un id qui est deja reference (= conflit)
				return(false);
			}
		}
		// copie des associations id / adr
		foreach(int id_trouve, priv_id_adr.keys()) {
			table_id_adr.insert(id_trouve, priv_id_adr.value(id_trouve));
		}
	}
	
	// importe les valeurs des champs de texte
	QList<QDomElement> inputs = QET::findInDomElement(e, "inputs", "input");
	foreach(QGraphicsItem *qgi, childItems()) {
		if (ElementTextItem *eti = qgraphicsitem_cast<ElementTextItem *>(qgi)) {
			foreach(QDomElement input, inputs) {
				eti -> fromXml(input);
				etiToElementLabels(eti);
			}
		}
	}

	//load uuid of connected elements
	QList <QDomElement> uuid_list = QET::findInDomElement(e, "links_uuids", "link_uuid");
	foreach (QDomElement qdo, uuid_list) tmp_uuids_link << qdo.attribute("uuid");
	
	//uuid of this element
	uuid_= QUuid(e.attribute("uuid", QUuid::createUuid().toString()));

	//load prefix
	m_prefix = e.attribute("prefix");

	//Load Sequential Values
	loadSequential(&e,"sequ_",&m_autoNum_seq.unit);
	loadSequential(&e,"sequf_",&m_autoNum_seq.unit_folio);
	loadSequential(&e,"seqt_",&m_autoNum_seq.ten);
	loadSequential(&e,"seqtf_",&m_autoNum_seq.ten_folio);
	loadSequential(&e,"seqh_",&m_autoNum_seq.hundred);
	loadSequential(&e,"seqhf_",&m_autoNum_seq.hundred_folio);

	//load informations
	m_element_informations.fromXml(e.firstChildElement("elementInformations"), "elementInformation");

	//Position and selection.
	//We directly call setPos from QGraphicsObject, because QetGraphicsItem will snap to grid
	QGraphicsObject::setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	
	// orientation
	bool conv_ok;
	int read_ori = e.attribute("orientation").toInt(&conv_ok);
	if (!conv_ok || read_ori < 0 || read_ori > 3) read_ori = 0;
	if (handle_inputs_rotation) {
		rotateBy(90*read_ori);
	} else {
		applyRotation(90*read_ori);
	}
	return(true);
}

/**
	Load Sequentials to display on element label
	@param element QDomElement to set Attributes
	@param Qstring seq to be retrieved
	@param QStringList list to be inserted values
*/
void Element::loadSequential(QDomElement* e, QString seq, QStringList* list) {
	//Load Sequential Values
	int i = 0;
	while (!e->attribute(seq + QString::number(i+1)).isEmpty()) {
			list->append(e->attribute(seq + QString::number(i+1)));
			i++;
	}
}

/**
	Permet d'exporter l'element en XML
	@param document Document XML a utiliser
	@param table_adr_id Table de correspondance entre les adresses des bornes
	et leur id dans la representation XML ; cette table completee par cette
	methode
	@return L'element XML representant cet element electrique
*/
QDomElement Element::toXml(QDomDocument &document, QHash<Terminal *, int> &table_adr_id) const {
	QDomElement element = document.createElement("element");
	
	// type
	element.setAttribute("type", typeId());
	// uuid
	element.setAttribute("uuid", uuid().toString());
	// prefix
	element.setAttribute("prefix", m_prefix);

	// Save Element sequential values to Xml
	// Save Unit Sequential Values
	for (int i = 0; i < m_autoNum_seq.unit.size(); i++) {
			element.setAttribute("sequ_" + QString::number(i+1),m_autoNum_seq.unit.at(i));
	}

	// Save UnitFolio Sequential Values
	for (int i = 0; i < m_autoNum_seq.unit_folio.size(); i++) {
			element.setAttribute("sequf_" + QString::number(i+1),m_autoNum_seq.unit_folio.at(i));
	}

	// Save Ten Sequential Values
	for (int i = 0; i < m_autoNum_seq.ten.size(); i++) {
			element.setAttribute("seqt_" + QString::number(i+1),m_autoNum_seq.ten.at(i));
	}

	// Save TenFolio Sequential Values
	for (int i = 0; i < m_autoNum_seq.ten_folio.size(); i++) {
			element.setAttribute("seqtf_" + QString::number(i+1),m_autoNum_seq.ten_folio.at(i));
	}

	// Save Hundred Sequential Values
	for (int i = 0; i < m_autoNum_seq.hundred.size(); i++) {
			element.setAttribute("seqh_" + QString::number(i+1),m_autoNum_seq.hundred.at(i));
	}

	// Save Hundred Sequential Values
	for (int i = 0; i < m_autoNum_seq.hundred_folio.size(); i++) {
			element.setAttribute("seqhf_" + QString::number(i+1),m_autoNum_seq.hundred_folio.at(i));
	}
	
	// position, selection et orientation
	element.setAttribute("x", QString("%1").arg(pos().x()));
	element.setAttribute("y", QString("%1").arg(pos().y()));
	element.setAttribute("orientation", QString("%1").arg(orientation()));
	
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
	foreach(ElementTextItem *eti, texts()) {
		inputs.appendChild(eti -> toXml(document));
	}
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

	return(element);
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
 * @brief Element::SetUpSequential
 * Setup the sequential value of this element
 */
void Element::SetUpSequential()
{
	if (diagram())
	{
		QString element_currentAutoNum = diagram()->project()->elementCurrentAutoNum();
		NumerotationContext nc = diagram()->project()->elementAutoNum(element_currentAutoNum);
		NumerotationContextCommands ncc (nc);

		autonum::setSequential(elementInformations()["label"].toString(), m_autoNum_seq, nc, diagram(), element_currentAutoNum);
		diagram()->project()->addElementAutoNum(element_currentAutoNum, ncc.next());
	}
}

/**
 * @brief ElementTextItem::setTaggedText
 * Set text @newstr to the text tagged with @tagg.
 * If tagg is found return the text item, else return NULL.
 * @param tagg required tagg
 * @param newstr new label
 * @param noeditable set editable or not (by default, set editable)
 */
ElementTextItem* Element::setTaggedText(const QString &tagg, const QString &newstr, const bool noeditable) {
	ElementTextItem *eti = taggedText(tagg);
	if (eti) {
		eti -> setPlainText(newstr);
		eti -> setNoEditable(noeditable);
	}
	return eti;
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
	m_prefix = prefix;
}

/**
 * @brief Element::freezeLabel
 * Freeze this element label
 */
void Element::freezeLabel()
{
	DiagramContext &dc = this->rElementInformations();
	QString freezelabel = dc["label"].toString();
	QString label = autonum::AssignVariables::formulaToLabel(freezelabel, m_autoNum_seq, diagram(),this );

	if (!(label == freezelabel))
	{
		dc.addValue("frozenlabel", freezelabel);
		dc.addValue("label",label);
		this->setTaggedText("label", label);
		this->setElementInformations(dc);
	}
}

/**
 * @brief Element::unfreezeLabel
 * Unfreeze this element label
 */
void Element::unfreezeLabel()
{
	DiagramContext &dc = this->rElementInformations();
	QString frozenlabel = dc["frozenlabel"].toString();
	if (frozenlabel == "") return;
	dc.addValue("frozenlabel", "");
	dc.addValue("label",frozenlabel);
	frozenlabel = autonum::AssignVariables::formulaToLabel(frozenlabel, m_autoNum_seq, diagram(),this );
	this->setTaggedText("label", frozenlabel);
	this->setElementInformations(dc);
}

/**
 * @brief Element::freezeNewAddedElement
 * Freeze this label if needed
 */
void Element::freezeNewAddedElement() {
	if (this->diagram()->freezeNewElements() || this->diagram()->project()->freezeNewElements()) {
		freezeLabel();
	}
	else return;
}
