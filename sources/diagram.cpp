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
#include <math.h>
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/customelement.h"
#include "factory/elementfactory.h"
#include "diagram.h"
#include "diagramcommands.h"
#include "diagramcontent.h"
#include "diagramposition.h"
#include "exportdialog.h"
#include "qetgraphicsitem/ghostelement.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/diagramimageitem.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "terminal.h"
#include "diagrameventinterface.h"
#include "qetapp.h"
#include "elementcollectionhandler.h"
#include "element.h"
#include "diagramview.h"
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"
#include "undocommand/addelementtextcommand.h"

const int   Diagram::xGrid  = 10;
const int   Diagram::yGrid  = 10;
const qreal Diagram::margin = 5.0;

// static variable to keep track of present background color of the diagram.
QColor		Diagram::background_color = Qt::white;

/**
 * @brief Diagram::Diagram
 * Constructor
 * @param project : The project of this diagram and also parent QObject
 */
Diagram::Diagram(QETProject *project) :
	QGraphicsScene           (project),
	m_project                 (nullptr),
	diagram_qet_version_     (-1),
	draw_grid_               (true),
	use_border_              (true),
	draw_terminals_          (true),
	draw_colored_conductors_ (true),
	m_event_interface (nullptr),
	m_freeze_new_elements   (false),
	m_freeze_new_conductors_ (false)
{
	setItemIndexMethod(QGraphicsScene::NoIndex);
		//Set to no index, because they can be the source of the crash with conductor and shape ghost.
		//https://forum.qt.io/topic/71316/qgraphicsscenefinditembsptreevisitor-visit-crashes-due-to-an-obsolete-paintevent-after-qgraphicsscene-removeitem
		//https://stackoverflow.com/questions/38458830/crash-after-qgraphicssceneremoveitem-with-custom-item-class
		//http://www.qtcentre.org/archive/index.php/t-33730.html
		//http://tech-artists.org/t/qt-properly-removing-qgraphicitems/3063
	
	setProject(project);
	qgi_manager_ = new QGIManager(this);
	setBackgroundBrush(Qt::white);
	conductor_setter_ = new QGraphicsLineItem(nullptr);
	conductor_setter_ -> setZValue(1000000);

	QPen pen(Qt::NoBrush, 1.5, Qt::DashLine);
	pen.setColor(Qt::black);
	conductor_setter_ -> setPen(pen);

	connect(&border_and_titleblock, SIGNAL(needTitleBlockTemplate(const QString &)), this, SLOT(setTitleBlockTemplate(const QString &)));
	connect(&border_and_titleblock, SIGNAL(diagramTitleChanged(const QString &)),    this, SLOT(titleChanged(const QString &)));
	connect(&border_and_titleblock, SIGNAL(titleBlockFolioChanged(const QString &)),    this, SLOT(titleChanged(const QString &)));
	connect(&border_and_titleblock, SIGNAL(borderChanged(QRectF,QRectF)), this, SLOT(adjustSceneRect()));
	connect(&border_and_titleblock, SIGNAL(titleBlockFolioChanged(const QString &)), this, SLOT(updateLabels()));
	connect(this, SIGNAL (diagramActivated()), this, SLOT(loadElmtFolioSeq()));
	connect(this, SIGNAL (diagramActivated()), this, SLOT(loadCndFolioSeq()));
	adjustSceneRect();
}

/**
 * @brief Diagram::~Diagram
 * Destructor
 */
Diagram::~Diagram()
{
        //First clear every selection to close an hypothetical editor
    clearSelection();
        // clear undo stack to prevent errors, because contains pointers to this diagram and is elements.
	undoStack().clear();
        //delete of QGIManager, every elements he knows are removed
	delete qgi_manager_;
        // remove of conductor setter
	delete conductor_setter_;

	if (m_event_interface)
        delete m_event_interface;
	
        // list removable items
	QList<QGraphicsItem *> deletable_items;
	for(QGraphicsItem *qgi : items())
    {
		if (qgi -> parentItem()) continue;
		if (qgraphicsitem_cast<Conductor *>(qgi)) continue;
		deletable_items << qgi;
	}

	qDeleteAll (deletable_items);
}

/**
	Dessine l'arriere-plan du schema, cad la grille.
	@param p Le QPainter a utiliser pour dessiner
	@param r Le rectangle de la zone a dessiner
*/
void Diagram::drawBackground(QPainter *p, const QRectF &r) {
	p -> save();
	
	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	// dessine un fond blanc
	p -> setPen(Qt::NoPen);
	//set brush color to present background color.
	p -> setBrush(Diagram::background_color);
	p -> drawRect(r);
	
	if (draw_grid_) {
			//Draw the point of the grid
			// if background color is black, then grid spots shall be white, else they shall be black in color.
		QPen pen;
		Diagram::background_color == Qt::black? pen.setColor(Qt::white) : pen.setColor(Qt::black);
		pen.setCosmetic(true);
		p->setPen(pen);

		p -> setBrush(Qt::NoBrush);

			//If user allow zoom out beyond of folio, we draw grid outside of border.
		QSettings settings;
		QRectF rect = settings.value("diagrameditor/zoom-out-beyond-of-folio", false).toBool() ?
						  r :
						  border_and_titleblock.insideBorderRect().intersected(r);

		qreal limite_x = rect.x() + rect.width();
		qreal limite_y = rect.y() + rect.height();
		
		int g_x = (int)ceil(rect.x());
		while (g_x % xGrid) ++ g_x;
		int g_y = (int)ceil(rect.y());
		while (g_y % yGrid) ++ g_y;
		
		QPolygon points;
		for (int gx = g_x ; gx < limite_x ; gx += xGrid) {
			for (int gy = g_y ; gy < limite_y ; gy += yGrid) {
				points << QPoint(gx, gy);
			}
		}
		p -> drawPoints(points);
	}
	
	if (use_border_) border_and_titleblock.draw(p);
	p -> restore();
}

/**
 * @brief Diagram::mouseDoubleClickEvent
 * This event is managed by diagram event interface if any.
 * @param event :
 */
void Diagram::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_event_interface)
		if(m_event_interface->mouseDoubleClickEvent(event))
		{
			if(!m_event_interface->isRunning())
			{
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}

	QGraphicsScene::mouseDoubleClickEvent(event);
}

/**
 * @brief Diagram::mousePressEvent
 * This event is managed by diagram event interface if any.
 * @param event
 */
void Diagram::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_event_interface)
		if(m_event_interface->mousePressEvent(event))
		{
			if(!m_event_interface->isRunning())
			{
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}

	QGraphicsScene::mousePressEvent(event);
}

/**
 * @brief Diagram::mouseMoveEvent
 * This event is managed by diagram event interface if any.
 * @param event
 */
void Diagram::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_event_interface)
		if(m_event_interface->mouseMoveEvent(event))
		{
			if(!m_event_interface->isRunning())
			{
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}

	QGraphicsScene::mouseMoveEvent(event);
}

/**
 * @brief Diagram::mouseReleaseEvent
 * This event is managed by diagram event interface if any.
 * @param event
 */
void Diagram::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_event_interface)
		if(m_event_interface->mouseReleaseEvent(event))
		{
			if(!m_event_interface->isRunning())
			{
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}

	QGraphicsScene::mouseReleaseEvent(event);
}

/**
 * @brief Diagram::wheelEvent
 * This event is managed by diagram event interface if any.
 * @param event
 */
void Diagram::wheelEvent(QGraphicsSceneWheelEvent *event)
{
	if (m_event_interface)
		if(m_event_interface->wheelEvent(event))
		{
			if(!m_event_interface->isRunning())
			{
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}

	QGraphicsScene::wheelEvent(event);
}

/**
 * @brief Diagram::keyPressEvent
 * This event is managed by diagram event interface if any.
 * Else move selected elements
 * @param e
 */
void Diagram::keyPressEvent(QKeyEvent *event)
{
	if (m_event_interface)
		if(m_event_interface->keyPressEvent(event))
		{
			if(!m_event_interface->isRunning())
			{
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	
	if (!isReadOnly())
	{
		QPointF movement;
		qreal top_position = 0;
		qreal left_position = 0;
		DiagramContent dc(this);
		if (!dc.items(DiagramContent::All).isEmpty())
		{
				//Move item with the keyborb arrow
			if(event->modifiers() == Qt::NoModifier)
			{
				switch(event->key())
				{
					case Qt::Key_Left:
						for (Element *item : dc.m_elements)
						{
							left_position = item->sceneBoundingRect().x();
							if(left_position <= 5)
								return;
						}
						movement = QPointF(-xGrid, 0.0);
						break;
					case Qt::Key_Right:
						movement = QPointF(+xGrid, 0.0);
						break;
					case Qt::Key_Up:
						for(Element *item : dc.m_elements)
						{
							top_position = item->sceneBoundingRect().y();
							if(top_position <= 5)
								return;
						}
						movement = QPointF(0.0, -yGrid);
						break;
					case Qt::Key_Down:
						movement = QPointF(0.0, +yGrid);
						break;
				}
				
				if (!movement.isNull() && !focusItem())
				{
					m_elements_mover.beginMovement(this);
					m_elements_mover.continueMovement(movement);
					event->accept();
					return;
				}
			}
			else if(event->modifiers() == Qt::ControlModifier)
			{
				//Adjust the alignment of a texts group
				if(selectedItems().size() == 1 && selectedItems().first()->type() == QGraphicsItemGroup::Type)
				{
					if(ElementTextItemGroup *etig = dynamic_cast<ElementTextItemGroup *>(selectedItems().first()))
					{
						if(event->key() == Qt::Key_Left &&  etig->alignment() != Qt::AlignLeft)
							undoStack().push(new AlignmentTextsGroupCommand(etig, Qt::AlignLeft));
						
						else if (event->key() == Qt::Key_Up && etig->alignment() != Qt::AlignVCenter)
							undoStack().push(new AlignmentTextsGroupCommand(etig, Qt::AlignVCenter));
						
						else if (event->key() == Qt::Key_Right && etig->alignment() != Qt::AlignRight)
							undoStack().push(new AlignmentTextsGroupCommand(etig, Qt::AlignRight));
					}
				}
			}
		}
		
		event->ignore();
		QGraphicsScene::keyPressEvent(event);
	}
}

/**
 * @brief Diagram::keyReleaseEvent
 * This event is managed by diagram event interface if any.
 * Else move selected element
 * @param e
 */
void Diagram::keyReleaseEvent(QKeyEvent *e)
{
	if (m_event_interface)
		if(m_event_interface->keyReleaseEvent(e))
		{
			if(!m_event_interface->isRunning())
			{
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	
	bool transmit_event = true;
	if (!isReadOnly()) {
		// detecte le relachement d'une touche de direction ( = deplacement d'elements)
		if (
			(e -> key() == Qt::Key_Left || e -> key() == Qt::Key_Right  ||
			 e -> key() == Qt::Key_Up   || e -> key() == Qt::Key_Down)  &&
			!e -> isAutoRepeat()
		) {
			m_elements_mover.endMovement();
			e -> accept();
			transmit_event = false;
		}
	}
	if (transmit_event) {
		QGraphicsScene::keyReleaseEvent(e);
	}
}

/**
 * @brief Diagram::setEventInterface
 * Set event_interface has current interface.
 * Diagram become the ownership of event_interface
 * If there is a previous interface, they will be delete before
 * and call init() to the new interface.
 * @param event_interface
 */
void Diagram::setEventInterface(DiagramEventInterface *event_interface)
{
	if (m_event_interface)
	{
		delete m_event_interface;
		event_interface -> init();
	}
	m_event_interface = event_interface;
}

/**
 * @brief Diagram::clearEventInterface
 * Clear the current event interface.
 */
void Diagram::clearEventInterface()
{
	if(m_event_interface)
	{
		delete m_event_interface;
		m_event_interface = nullptr;
	}
}

/**
 * @brief Diagram::conductorsAutonumName
 * @return the name of autonum to use.
 */
QString Diagram::conductorsAutonumName() const {
	return m_conductors_autonum_name;
}

/**
 * @brief Diagram::setConductorsAutonumName
 * @param name, name of autonum to use.
 */
void Diagram::setConductorsAutonumName(const QString &name) {
	m_conductors_autonum_name= name;
}

/**
	Exporte le schema vers une image
	@return Une QImage representant le schema
*/
bool Diagram::toPaintDevice(QPaintDevice &pix, int width, int height, Qt::AspectRatioMode aspectRatioMode) {
	// determine la zone source =  contenu du schema + marges
	QRectF source_area;
	if (!use_border_) {
		source_area = itemsBoundingRect();
		source_area.translate(-margin, -margin);
		source_area.setWidth (source_area.width () + 2.0 * margin);
		source_area.setHeight(source_area.height() + 2.0 * margin);
	} else {
		source_area = QRectF(
			0.0,
			0.0,
			border_and_titleblock.borderAndTitleBlockRect().width()  + 2.0 * margin,
			border_and_titleblock.borderAndTitleBlockRect().height() + 2.0 * margin
		);
	}
	
	// si les dimensions ne sont pas precisees, l'image est exportee a l'echelle 1:1
	QSize image_size = (width == -1 && height == -1) ? source_area.size().toSize() : QSize(width, height);
	
	// prepare le rendu
	QPainter p;
	if (!p.begin(&pix)) return(false);
	
	// rendu antialiase
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::TextAntialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);
	
	// deselectionne tous les elements
	QList<QGraphicsItem *> selected_elmts = selectedItems();
	foreach (QGraphicsItem *qgi, selected_elmts) qgi -> setSelected(false);
	
	// effectue le rendu lui-meme
	render(&p, QRect(QPoint(0, 0), image_size), source_area, aspectRatioMode);
	p.end();
	
	// restaure les elements selectionnes
	foreach (QGraphicsItem *qgi, selected_elmts) qgi -> setSelected(true);
	
	return(true);
}

/**
	Permet de connaitre les dimensions qu'aura l'image generee par la methode toImage()
	@return La taille de l'image generee par toImage()
*/
QSize Diagram::imageSize() const {
	// determine la zone source =  contenu du schema + marges
	qreal image_width, image_height;
	if (!use_border_) {
		QRectF items_rect = itemsBoundingRect();
		image_width  = items_rect.width();
		image_height = items_rect.height();
	} else {
		image_width  = border_and_titleblock.borderAndTitleBlockRect().width();
		image_height = border_and_titleblock.borderAndTitleBlockRect().height();
	}
	
	image_width  += 2.0 * margin;
	image_height += 2.0 * margin;
	
	// renvoie la taille de la zone source
	return(QSizeF(image_width, image_height).toSize());
}

/**
	@return true si le schema est considere comme vide, false sinon.
	Un schema vide ne contient ni element, ni conducteur, ni champ de texte
*/
bool Diagram::isEmpty() const {
	return(!items().count());
}

/**
 * @brief Diagram::potential
 * @return all potential in the diagram
 *each potential are in the QList and each conductors of one potential are in the QSet
 */
QList < QSet <Conductor *> > Diagram::potentials() {
	QList < QSet <Conductor *> > potential_List;
	if (content().conductors().size() == 0) return (potential_List); //return an empty potential
	QList <Conductor *> conductors_list = content().conductors();

	do {
		QSet <Conductor *> one_potential = conductors_list.first() -> relatedPotentialConductors();
		one_potential << conductors_list.takeFirst();
		foreach (Conductor *c, one_potential) conductors_list.removeOne(c);
		potential_List << one_potential;
	} while (!conductors_list.empty());

	return (potential_List);
}

/**
	Exporte tout ou partie du schema 
	@param whole_content Booleen (a vrai par defaut) indiquant si le XML genere doit
	representer l'integralite du schema ou seulement le contenu selectionne
	@return Un Document XML (QDomDocument)
*/
QDomDocument Diagram::toXml(bool whole_content) {
	// document
	QDomDocument document;
	
	// racine de l'arbre XML
	QDomElement racine = document.createElement("diagram");
	
	// add the application version number
	racine.setAttribute("version", QET::version);
	
	// proprietes du schema
	if (whole_content) {
		border_and_titleblock.titleBlockToXml(racine);
		border_and_titleblock.borderToXml(racine);
		
		// Default conductor properties
		QDomElement default_conductor = document.createElement("defaultconductor");
		defaultConductorProperties.toXml(default_conductor);
		racine.appendChild(default_conductor);

		// Conductor autonum
		if (!m_conductors_autonum_name.isEmpty()) {
			racine.setAttribute("conductorAutonum", m_conductors_autonum_name);
		}

		//Default New Element
		racine.setAttribute("freezeNewElement", m_freeze_new_elements ? "true" : "false");

		//Default New Conductor
		racine.setAttribute("freezeNewConductor", m_freeze_new_conductors_ ? "true" : "false");

		//Element Folio Sequential Variables
		if (!m_elmt_unitfolio_max.isEmpty() || !m_elmt_tenfolio_max.isEmpty() || !m_elmt_hundredfolio_max.isEmpty()) {
			QDomElement elmtfoliosequential = document.createElement("elementautonumfoliosequentials");
			if (!m_elmt_unitfolio_max.isEmpty()) {
				QDomElement elmtfolioseq = document.createElement("elementunitfolioseq");
				folioSequentialsToXml(&m_elmt_unitfolio_max, &elmtfolioseq, "sequf_", "unitfolioseq", &document);
				elmtfoliosequential.appendChild(elmtfolioseq);
			}
			if (!m_elmt_tenfolio_max.isEmpty()) {
				QDomElement elmtfolioseq = document.createElement("elementtenfolioseq");
				folioSequentialsToXml(&m_elmt_tenfolio_max, &elmtfolioseq, "seqtf_", "tenfolioseq", &document);
				elmtfoliosequential.appendChild(elmtfolioseq);
			}
			if (!m_elmt_hundredfolio_max.isEmpty()) {
				QDomElement elmtfolioseq = document.createElement("elementhundredfolioseq");
				folioSequentialsToXml(&m_elmt_hundredfolio_max, &elmtfolioseq, "seqhf_", "hundredfolioseq", &document);
				elmtfoliosequential.appendChild(elmtfolioseq);
			}
			racine.appendChild(elmtfoliosequential);
		}
		//Conductor Folio Sequential Variables
		if (!m_cnd_unitfolio_max.isEmpty() || !m_cnd_tenfolio_max.isEmpty() || !m_cnd_hundredfolio_max.isEmpty()) {
			QDomElement cndfoliosequential = document.createElement("conductorautonumfoliosequentials");
			QHash<QString, QStringList>::iterator i;
			if (!m_cnd_unitfolio_max.isEmpty()) {
				QDomElement cndfolioseq = document.createElement("conductorunitfolioseq");
				folioSequentialsToXml(&m_cnd_unitfolio_max, &cndfolioseq, "sequf_", "unitfolioseq", &document);
				cndfoliosequential.appendChild(cndfolioseq);
			}
			if (!m_cnd_tenfolio_max.isEmpty()) {
				QDomElement cndfolioseq = document.createElement("conductortenfolioseq");
				folioSequentialsToXml(&m_cnd_tenfolio_max, &cndfolioseq, "seqtf_", "tenfolioseq", &document);
				cndfoliosequential.appendChild(cndfolioseq);
			}
			if (!m_cnd_hundredfolio_max.isEmpty()) {
				QDomElement cndfolioseq = document.createElement("conductorhundredfolioseq");
				folioSequentialsToXml(&m_cnd_hundredfolio_max, &cndfolioseq, "seqhf_", "hundredfolioseq", &document);
				cndfoliosequential.appendChild(cndfolioseq);
			}
			racine.appendChild(cndfoliosequential);
		}
	}
	else {
			//this method with whole_content to false,
			//is often use to copy and paste the current selection
			//so we add the id of the project where copy occur.
		racine.setAttribute("projectId", QETApp::projectId(m_project));
	}
	document.appendChild(racine);
	
	// si le schema ne contient pas d'element (et donc pas de conducteurs), on retourne de suite le document XML
	if (items().isEmpty()) return(document);
	
	// creation de trois listes : une qui contient les elements, une qui contient les conducteurs, une qui contient les champs de texte
	QList<Element *> list_elements;
	QList<Conductor *> list_conductors;
	QList<DiagramTextItem *> list_texts;
	QList<DiagramImageItem *> list_images;
	QList<QetShapeItem *> list_shapes;
	
	QList<QGraphicsItem *> list_items = items();
	;
	// Determine les elements a "XMLiser"
	foreach(QGraphicsItem *qgi, list_items) {
		if (Element *elmt = qgraphicsitem_cast<Element *>(qgi)) {
			if (whole_content) list_elements << elmt;
			else if (elmt -> isSelected()) list_elements << elmt;
		} else if (Conductor *f = qgraphicsitem_cast<Conductor *>(qgi)) {
			if (whole_content) list_conductors << f;
			// lorsqu'on n'exporte pas tout le diagram, il faut retirer les conducteurs non selectionnes
			// et pour l'instant, les conducteurs non selectionnes sont les conducteurs dont un des elements n'est pas selectionne
			else if (f -> terminal1 -> parentItem() -> isSelected() && f -> terminal2 -> parentItem() -> isSelected()) {
				list_conductors << f;
			}
		} else if (IndependentTextItem *iti = qgraphicsitem_cast<IndependentTextItem *>(qgi)) {
			if (whole_content) list_texts << iti;
			else if (iti -> isSelected()) list_texts << iti;
		} else if (DiagramImageItem *dii = qgraphicsitem_cast<DiagramImageItem *>(qgi)) {
			if (whole_content) list_images << dii;
			else if (dii -> isSelected()) list_images << dii;
		} else if (QetShapeItem *dsi = qgraphicsitem_cast<QetShapeItem *>(qgi)) {
			if (whole_content) list_shapes << dsi;
			else if (dsi -> isSelected()) list_shapes << dsi;
		}
	}
	
	// table de correspondance entre les adresses des bornes et leurs ids
	QHash<Terminal *, int> table_adr_id;
	
	// enregistrement des elements
	if (!list_elements.isEmpty()) {
		QDomElement elements = document.createElement("elements");
		foreach(Element *elmt, list_elements) {
			elements.appendChild(elmt -> toXml(document, table_adr_id));
		}
		racine.appendChild(elements);
	}
	
	// enregistrement des conducteurs
	if (!list_conductors.isEmpty()) {
		QDomElement conductors = document.createElement("conductors");
		foreach(Conductor *cond, list_conductors) {
			conductors.appendChild(cond -> toXml(document, table_adr_id));
		}
		racine.appendChild(conductors);
	}
	
	// enregistrement des champs de texte
	if (!list_texts.isEmpty()) {
		QDomElement inputs = document.createElement("inputs");
		foreach(DiagramTextItem *dti, list_texts) {
			inputs.appendChild(dti -> toXml(document));
		}
		racine.appendChild(inputs);
	}

	// save of images
	if (!list_images.isEmpty()) {
		QDomElement images = document.createElement("images");
		foreach (DiagramImageItem *dii, list_images) {
			images.appendChild(dii -> toXml(document));
		}
		racine.appendChild(images);
	}

	// save of basic shapes
	if (!list_shapes.isEmpty()) {
		QDomElement shapes = document.createElement("shapes");
		foreach (QetShapeItem *dii, list_shapes) {
			shapes.appendChild(dii -> toXml(document));
		}
		racine.appendChild(shapes);
	}
	// on retourne le document XML ainsi genere
	return(document);
}

/**
+ * @brief Diagram::folioSequentialsToXml
+ * Add folio sequential to QDomElement
+ * @param domElement to add attributes
+ * @param hash to retrieve content with content
+ * @param sequential type
+ */
void Diagram::folioSequentialsToXml(QHash<QString, QStringList> *hash, QDomElement *domElement, QString seq_type, QString type, QDomDocument *doc) {
	QHash<QString, QStringList>::iterator i;
	for (i = hash->begin(); i != hash->end(); i++) {
		QDomElement folioseq = doc->createElement(type);
		folioseq.setAttribute("title", i.key());
		for (int j = 0; j < i.value().size(); j++) {
			folioseq.setAttribute(seq_type + QString::number(j+1), i.value().at(j));
		}
		domElement->appendChild(folioseq);
	}
}

/**
	Importe le schema decrit dans un document XML. Si une position est
	precisee, les elements importes sont positionnes de maniere a ce que le
	coin superieur gauche du plus petit rectangle pouvant les entourant tous
	(le bounding rect) soit a cette position.
	@param document Le document XML a analyser
	@param position La position du schema importe
	@param consider_informations Si vrai, les informations complementaires
	(auteur, titre, ...) seront prises en compte
	@param content_ptr si ce pointeur vers un DiagramContent est different de 0,
	il sera rempli avec le contenu ajoute au schema par le fromXml
	@return true si l'import a reussi, false sinon
*/
bool Diagram::fromXml(QDomDocument &document, QPointF position, bool consider_informations, DiagramContent *content_ptr) {
	QDomElement root = document.documentElement();
	return(fromXml(root, position, consider_informations, content_ptr));
}

/**
	Importe le schema decrit dans un element XML. Cette methode delegue son travail a Diagram::fromXml
	Si l'import reussit, cette methode initialise egalement le document XML
	interne permettant de bien gerer l'enregistrement de ce schema dans le
	projet auquel il appartient.
	@see Diagram::fromXml
	@param document Le document XML a analyser
	@param position La position du schema importe
	@param consider_informations Si vrai, les informations complementaires
	(auteur, titre, ...) seront prises en compte
	@param content_ptr si ce pointeur vers un DiagramContent est different de 0,
	il sera rempli avec le contenu ajoute au schema par le fromXml
	@return true si l'import a reussi, false sinon
	
*/
bool Diagram::initFromXml(QDomElement &document, QPointF position, bool consider_informations, DiagramContent *content_ptr) {
	// import le contenu et les proprietes du schema depuis l'element XML fourni en parametre
	bool from_xml = fromXml(document, position, consider_informations, content_ptr);
	
	return(from_xml);
}

/**
	Importe le schema decrit dans un element XML. Si une position est
	precisee, les elements importes sont positionnes de maniere a ce que le
	coin superieur gauche du plus petit rectangle pouvant les entourant tous
	(le bounding rect) soit a cette position.
	@param document Le document XML a analyser
	@param position La position du schema importe
	@param consider_informations Si vrai, les informations complementaires
	(auteur, titre, ...) seront prises en compte
	@param content_ptr si ce pointeur vers un DiagramContent est different de 0,
	il sera rempli avec le contenu ajoute au schema par le fromXml
	@return true si l'import a reussi, false sinon
*/
bool Diagram::fromXml(QDomElement &document, QPointF position, bool consider_informations, DiagramContent *content_ptr) {
	QDomElement root = document;
	// The first element must be a diagram
	if (root.tagName() != "diagram") return(false);
	
	// Read attributes of this diagram
	if (consider_informations) {
		// Version of diagram
		bool conv_ok;
		qreal version_value = root.attribute("version").toDouble(&conv_ok);
		if (conv_ok) {
			diagram_qet_version_ = version_value;
		}
		
		// Load border and titleblock
		border_and_titleblock.titleBlockFromXml(root);
		border_and_titleblock.borderFromXml(root);
		
		// Find the element "defaultconductor".
		// If found, load default conductor properties.
		QDomElement default_conductor_elmt = root.firstChildElement("defaultconductor");
		if (!default_conductor_elmt.isNull()) {
			defaultConductorProperties.fromXml(default_conductor_elmt);
		}

		// Load the autonum
		m_conductors_autonum_name = root.attribute("conductorAutonum");

		// Load Freeze New Element
		m_freeze_new_elements = root.attribute("freezeNewElement").toInt();

		// Load Freeze New Conductor
		m_freeze_new_conductors_ = root.attribute("freezeNewConductor").toInt();

		//Load Element Folio Sequential
		folioSequentialsFromXml(root, &m_elmt_unitfolio_max, "elementunitfolioseq","sequf_","unitfolioseq", "elementautonumfoliosequentials");
		folioSequentialsFromXml(root, &m_elmt_tenfolio_max, "elementtenfolioseq","seqtf_", "tenfolioseq", "elementautonumfoliosequentials");
		folioSequentialsFromXml(root, &m_elmt_hundredfolio_max, "elementhundredfolioseq","seqhf_", "hundredfolioseq", "elementautonumfoliosequentials");

		//Load Conductor Folio Sequential
		folioSequentialsFromXml(root, &m_cnd_unitfolio_max, "conductorunitfolioseq","sequf_","unitfolioseq", "conductorautonumfoliosequentials");
		folioSequentialsFromXml(root, &m_cnd_tenfolio_max, "conductortenfolioseq","seqtf_","tenfolioseq", "conductorautonumfoliosequentials");
		folioSequentialsFromXml(root, &m_cnd_hundredfolio_max, "conductorhundredfolioseq","seqhf_","hundredfolioseq", "conductorautonumfoliosequentials");
	}

	// if child haven't got a child, loading is finish (diagram is empty)
	if (root.firstChild().isNull()) {
		return(true);
	}
	
	// Backward compatibility: prior to version 0.3, we need to compensate, at
	// diagram-opening time, the rotation of the element for each of its
	// textfields having the "FollowParentRotation" option disabled.
	// After 0.3, elements textfields get userx, usery and userrotation attributes
	// that explicitly specify their position and orientation.
	qreal project_qet_version = declaredQElectroTechVersion(true);
	bool handle_inputs_rotation = (
		project_qet_version != -1 && project_qet_version < 0.3 &&
		m_project -> state() == QETProject::ProjectParsingRunning
	);

		//If paste from another project
	if (root.hasAttribute("projectId")) {
		QETProject *other_project = QETApp::project(root.attribute("projectId", "-1").toInt());

			//We try to paste from another project, then befor paste elements,
			//we must to import the definition of the pasted elements (owned by other project)
			//in the embedded collection of this project
		if (other_project && other_project != m_project) {
			ElementCollectionHandler ech;
			foreach (QDomElement element_xml, QET::findInDomElement(root, "elements", "element")) {
				if (!Element::valideXml(element_xml)) continue;

				QString type_id = element_xml.attribute("type");

				if (type_id.startsWith("embed://")) {
					ElementsLocation location(type_id, other_project);
					ech.importFromProject(m_project, location);
				}
			}
		}
	}
		//Load all elements from the XML
	QList<Element *> added_elements;
	QHash<int, Terminal *> table_adr_id;
	foreach (QDomElement element_xml, QET::findInDomElement(root, "elements", "element"))
	{
		if (!Element::valideXml(element_xml)) continue;
		
		// cree un element dont le type correspond a l'id type
		QString type_id = element_xml.attribute("type");
		ElementsLocation element_location;
		if (type_id.startsWith("embed://")) {
			element_location = ElementsLocation(type_id, m_project);
		}
		else {
			element_location = ElementsLocation(type_id);
		}
		
		int state = 0;
		Element *nvel_elmt = ElementFactory::Instance() -> createElement(element_location, nullptr, &state);
		if (state)
		{
			QString debug_message = QString("Diagram::fromXml() : Le chargement de la description de l'element %1 a echoue avec le code d'erreur %2").arg(element_location.path()).arg(state);
			qDebug() << qPrintable(debug_message);
			delete nvel_elmt;
			
			qDebug() << "Diagram::fromXml() : Utilisation d'un GhostElement en lieu et place de cet element.";
			nvel_elmt = new GhostElement(element_location);
		}
		
		addItem(nvel_elmt);
			//Loading fail, remove item from the diagram
		if (!nvel_elmt->fromXml(element_xml, table_adr_id, handle_inputs_rotation))
		{
			removeItem(nvel_elmt);
			delete nvel_elmt;
			qDebug() << "Diagram::fromXml() : Le chargement des parametres d'un element a echoue";
		} else {
			added_elements << nvel_elmt;
		}
	}
	
	// Load text
	QList<IndependentTextItem *> added_texts;
	foreach (QDomElement text_xml, QET::findInDomElement(root, "inputs", "input")) {
		IndependentTextItem *iti = new IndependentTextItem();
		iti -> fromXml(text_xml);
		addItem(iti);
		added_texts << iti;
	}

	// Load image
	QList<DiagramImageItem *> added_images;
	foreach (QDomElement image_xml, QET::findInDomElement(root, "images", "image")) {
		DiagramImageItem *dii = new DiagramImageItem ();
		dii -> fromXml(image_xml);
		addItem(dii);
		added_images << dii;
	}

	// Load shape
	QList<QetShapeItem *> added_shapes;
	foreach (QDomElement shape_xml, QET::findInDomElement(root, "shapes", "shape")) {
		QetShapeItem *dii = new QetShapeItem (QPointF(0,0));
		dii -> fromXml(shape_xml);
		addItem(dii);
		added_shapes << dii;
	}

		// Load conductor
	QList<Conductor *> added_conductors;
	foreach (QDomElement f, QET::findInDomElement(root, "conductors", "conductor"))
	{
		if (!Conductor::valideXml(f)) continue;

			//Check if terminal that conductor must be linked is know
		int id_p1 = f.attribute("terminal1").toInt();
		int id_p2 = f.attribute("terminal2").toInt();
		if (table_adr_id.contains(id_p1) && table_adr_id.contains(id_p2))
		{
			Terminal *p1 = table_adr_id.value(id_p1);
			Terminal *p2 = table_adr_id.value(id_p2);
			if (p1 != p2)
			{
				Conductor *c = new Conductor(p1, p2);
				if (c->isValid())
				{
					addItem(c);
					c -> fromXml(f);
					added_conductors << c;
				}
				else
					delete c;
			}
		}
		else qDebug() << "Diagram::fromXml() : Le chargement du conducteur" << id_p1 << id_p2 << "a echoue";
	}

	//Translate items if a new position was given in parameter
	if (position != QPointF()) {

		QList<QGraphicsItem *> added_items;
		foreach (Element          *added_element, added_elements  ) added_items << added_element;
		foreach (Conductor        *added_cond,    added_conductors) added_items << added_cond;
		foreach (QetShapeItem     *added_shape,   added_shapes    ) added_items << added_shape;
		foreach (DiagramTextItem  *added_text,    added_texts     ) added_items << added_text;
		foreach (DiagramImageItem *added_image,   added_images    ) added_items << added_image;

		//Get the top left corner of the rectangle that contain all added items
		QRectF items_rect;
		foreach (QGraphicsItem *item, added_items) {
			items_rect = items_rect.united(item -> mapToScene(item -> boundingRect()).boundingRect());
		}

		QPointF point_ = items_rect.topLeft();
		QPointF pos_ = Diagram::snapToGrid(QPointF (position.x() - point_.x(),
													position.y() - point_.y()));

		//Translate all added items
		foreach (QGraphicsItem *qgi, added_items)
			qgi -> setPos( qgi -> pos() += pos_);
	}
	
	// remplissage des listes facultatives
	if (content_ptr) {
		content_ptr -> m_elements         = added_elements;
		content_ptr -> m_conductors_to_move = added_conductors;
		content_ptr -> m_text_fields       = added_texts.toSet();
		content_ptr -> m_images			= added_images.toSet();
		content_ptr -> m_shapes			= added_shapes.toSet();
	}
	adjustSceneRect();
	return(true);
}

/**
 * @brief Diagram::folioSequentialsFromXml
 * Load folio sequential from QDomElement
 * @param root containing all folio sequentials
 * @param hash to be loaded with content
 * @param folioSeq type
 * @param seq type
 * @param type of sequential
 */
void Diagram::folioSequentialsFromXml(const QDomElement &root, QHash<QString, QStringList>* hash, QString folioSeq, QString seq, QString type, QString autonumFolioSeqType) {
	foreach (QDomElement folioSeqAutoNum, QET::findInDomElement(root, autonumFolioSeqType, folioSeq)) {
		for(QDomElement folioseq = folioSeqAutoNum.firstChildElement(type); !folioseq.isNull(); folioseq = folioseq.nextSiblingElement(type)) {
			QString title = folioseq.attribute("title");
			QStringList list;
			int i = 1;
			while (folioseq.hasAttribute(seq + QString::number(i))) {
				list << folioseq.attribute(seq + QString::number(i));
				i++;
			}
			hash->insert(title,list);
		}
	}
}

/**
 * @brief Diagram::refreshContents
 * refresh all content of diagram.
 * - refresh conductor text.
 * - linking the elements waiting to be linked
 * - Refresh the connection of the dynamic element text item (use for text with source of text label)
 */
void Diagram::refreshContents()
{
	for (Element *elmt : elements())
	{
		elmt->initLink(project());
		for (DynamicElementTextItem *deti : elmt->dynamicTextItems())
			deti->refreshLabelConnection();
	}

	for (Conductor *conductor : conductors())
		conductor->refreshText();
}

/**
 * @brief Diagram::addItem
 * Réimplemented from QGraphicsScene::addItem(QGraphicsItem *item)
 * Do some specific operation if item need it (for exemple an element)
 * @param item
 */
void Diagram::addItem(QGraphicsItem *item)
{
	if (!item || isReadOnly() || item->scene() == this) return;
	QGraphicsScene::addItem(item);

	switch (item->type())
	{
		case Conductor::Type:
		{
			Conductor *conductor = static_cast<Conductor *>(item);
			conductor->terminal1->addConductor(conductor);
			conductor->terminal2->addConductor(conductor);
			conductor->calculateTextItemPosition();
		}
			break;

		case IndependentTextItem::Type:
		{
			const IndependentTextItem *text = static_cast<const IndependentTextItem *>(item);
			connect(text, &IndependentTextItem::diagramTextChanged, this, &Diagram::diagramTextChanged);
		}
	}
}

/**
 * @brief Diagram::removeItem
 * Réimplemented from QGraphicsScene::removeItem(QGraphicsItem *item)
 * Do some specific operation if item need it (for exemple an element)
 * @param item
 */
void Diagram::removeItem(QGraphicsItem *item)
{
	if (!item || isReadOnly()) return;

	switch (item->type())
	{
		case Element::Type:
		{
			Element *elmt = static_cast<Element*>(item);
			elmt->unlinkAllElements();
		}
			break;
		case Conductor::Type:
		{
			Conductor *conductor = static_cast<Conductor *>(item);
			conductor->terminal1->removeConductor(conductor);
			conductor->terminal2->removeConductor(conductor);
		}
			break;

		case IndependentTextItem::Type:
		{
			const IndependentTextItem *text = static_cast<const IndependentTextItem *>(item);
			disconnect(text, &IndependentTextItem::diagramTextChanged, this, &Diagram::diagramTextChanged);
		}
	}

	QGraphicsScene::removeItem(item);
}

void Diagram::titleChanged(const QString &title) {
	emit(diagramTitleChanged(this, title));
}

/**
	Gere le fait qu'un texte du schema ait ete modifie
	@param text_item Texte modifie
	@param old_text Ancien texte
	@param new_text Nouveau texte
*/
void Diagram::diagramTextChanged(DiagramTextItem *text_item, const QString &old_text, const QString &new_text) {
	if (!text_item) return;
	undoStack().push(new ChangeDiagramTextCommand(text_item, old_text, new_text));
}

/**
	This slot may be used to inform the diagram object that the given title
	block template has changed. The diagram will thus flush its title
	block-dedicated rendering cache.
	@param template_name Name of the title block template that has changed
*/
void Diagram::titleBlockTemplateChanged(const QString &template_name) {
	if (border_and_titleblock.titleBlockTemplateName() != template_name) return;
	
	border_and_titleblock.titleBlockTemplateChanged(template_name);
	update();
}

/**
	This slot has to be be used to inform this class that the given title block
	template is about to be removed and is no longer accessible. This class
	will either use the provided  optional TitleBlockTemplate or the default
	title block provided by QETApp::defaultTitleBlockTemplate()
	@param template_name Name of the title block template that has changed
	@param new_template (Optional) Name of the title block template to use instead
*/
void Diagram::titleBlockTemplateRemoved(const QString &template_name, const QString &new_template)
{
	if (border_and_titleblock.titleBlockTemplateName() != template_name) return;	
	const TitleBlockTemplate *final_template = m_project->embeddedTitleBlockTemplatesCollection()->getTemplate(new_template);
	border_and_titleblock.titleBlockTemplateRemoved(template_name, final_template);
	update();
}

/**
	Set the template to use to render the title block of this diagram.
	@param template_name Name of the title block template.
*/
void Diagram::setTitleBlockTemplate(const QString &template_name)
{
	if (!m_project) return;
	
	QString current_name = border_and_titleblock.titleBlockTemplateName();
	const TitleBlockTemplate *titleblock_template = m_project->embeddedTitleBlockTemplatesCollection()->getTemplate(template_name);
	border_and_titleblock.titleBlockTemplateRemoved(current_name, titleblock_template);
	
	if (template_name != current_name)
		emit(usedTitleBlockTemplateChanged(template_name));
}

/**
	Selectionne tous les objets du schema
*/
void Diagram::selectAll() {
	if (items().isEmpty()) return;
	
	blockSignals(true);
	foreach(QGraphicsItem *qgi, items()) qgi -> setSelected(true);
	blockSignals(false);
	emit(selectionChanged());
}

/**
	Deslectionne tous les objets selectionnes
*/
void Diagram::deselectAll() {
	if (items().isEmpty()) return;
	
	clearSelection();
}

/**
	Inverse l'etat de selection de tous les objets du schema
*/
void Diagram::invertSelection() {
	if (items().isEmpty()) return;
	
	blockSignals(true);
	foreach (QGraphicsItem *item, items()) item -> setSelected(!item -> isSelected());
	blockSignals(false);
	emit(selectionChanged());
}

/**
 * @brief Diagram::updateLabels
 * Update elements and conductors that reference folio field
 * in their labels.
 */
void Diagram::updateLabels()
{
	for (Conductor *cnd : content().conductors())
	{
		cnd->refreshText();
	}
}

/**
 * @brief Diagram::insertFolioSeqHash
 * This class inserts a stringlist containing all
 * sequential variables related to an autonum in a QHash
 * @param Hash to be accessed
 * @param autonum title
 * @param sequential to be treated
 * @param type to be treated
 * @param Numerotation Context to be manipulated
 */
void Diagram::insertFolioSeqHash(QHash<QString, QStringList> *hash, QString title, QString type, NumerotationContext *nc) {
	QStringList max;
	for (int i = 0; i < nc->size(); i++) {
		if (nc->itemAt(i).at(0) == type) {
			nc->replaceValue(i, QString::number(nc->itemAt(i).at(3).toInt()));
			max.append(QString::number(nc->itemAt(i).at(3).toInt() - nc->itemAt(i).at(2).toInt()));
		}
	}
	hash->insert(title,max);
}

/**
 * @brief Diagram::loadFolioSeqHash
 * This class loads all folio sequential variables
 * related to the current autonum
 * @param Hash to be accessed
 * @param autonum title
 * @param sequential to be treated
 * @param type to be treated
 * @param Numerotation Context to be manipulated
 */
void Diagram::loadFolioSeqHash(QHash<QString, QStringList> *hash, QString title, QString type, NumerotationContext *nc) {
		int j = 0;
		for (int i = 0; i < nc->size(); i++) {
			if (nc->itemAt(i).at(0) == type) {
				QString new_value;
				new_value = QString::number(hash->value(title).at(j).toInt() + nc->itemAt(i).at(2).toInt());
				nc->replaceValue(i,new_value);
				j++;
			}
		}
}

/**
 * @brief Diagram::loadElmtFolioSeq
 * This class loads all folio sequential variables related
 * to the current autonum
 */
void Diagram::loadElmtFolioSeq() {
	QString title = project()->elementCurrentAutoNum();
	NumerotationContext nc = project()->elementAutoNum(title);

	//Unit Folio
	if (m_elmt_unitfolio_max.isEmpty() || !m_elmt_unitfolio_max.contains(title)) {
		//Insert Initial Value
		if (project()->elementAutoNumCurrentFormula().contains("%sequf_")) {
			insertFolioSeqHash(&m_elmt_unitfolio_max,title,"unitfolio",&nc);
			project()->addElementAutoNum(title,nc);
		}
	}
	else if (m_elmt_unitfolio_max.contains(title)) {
		//Load Folio Current Value
		if (project()->elementAutoNumCurrentFormula().contains("%sequf_")) {
			loadFolioSeqHash(&m_elmt_unitfolio_max,title,"unitfolio",&nc);
			project()->addElementAutoNum(title,nc);
		}
	}

	//Ten Folio
	if (m_elmt_tenfolio_max.isEmpty() || !m_elmt_tenfolio_max.contains(title)) {
		//Insert Initial Value
		if (project()->elementAutoNumCurrentFormula().contains("%seqtf_")) {
			insertFolioSeqHash(&m_elmt_tenfolio_max,title,"tenfolio",&nc);
			project()->addElementAutoNum(title,nc);
		}
	}
	else if (m_elmt_tenfolio_max.contains(title)) {
		//Load Folio Current Value
		if (project()->elementAutoNumCurrentFormula().contains("%seqtf_")) {
			loadFolioSeqHash(&m_elmt_tenfolio_max,title,"tenfolio",&nc);
			project()->addElementAutoNum(title,nc);
		}
	}

	//Hundred Folio
	if (m_elmt_hundredfolio_max.isEmpty() || !m_elmt_hundredfolio_max.contains(title)) {
		//Insert Initial Value
		if (project()->elementAutoNumCurrentFormula().contains("%seqhf_")) {
			insertFolioSeqHash(&m_elmt_hundredfolio_max,title,"hundredfolio",&nc);
			project()->addElementAutoNum(title,nc);
		}
	}
	else if (m_elmt_hundredfolio_max.contains(title)) {
		//Load Folio Current Value
		if (project()->elementAutoNumCurrentFormula().contains("%seqhf_")) {
			loadFolioSeqHash(&m_elmt_hundredfolio_max,title,"hundredfolio",&nc);
			project()->addElementAutoNum(title,nc);
		}
	}
}

/**
 * @brief Diagram::loadCndFolioSeq
 * This class loads all conductor folio sequential variables related
 * to the current autonum
 */
void Diagram::loadCndFolioSeq() {
	//Conductor
	QString title = project()->conductorCurrentAutoNum();
	NumerotationContext nc = project()->conductorAutoNum(title);
	QString formula = autonum::numerotationContextToFormula(nc);

	//Unit Folio
	if (m_cnd_unitfolio_max.isEmpty() || !m_cnd_unitfolio_max.contains(title)) {
		//Insert Initial Value
		if (formula.contains("%sequf_")) {
			insertFolioSeqHash(&m_cnd_unitfolio_max,title,"unitfolio",&nc);
			project()->addConductorAutoNum(title,nc);
		}
	}
	else if (m_cnd_unitfolio_max.contains(title)) {
		//Load Folio Current Value
		if (formula.contains("%sequf_")) {
			loadFolioSeqHash(&m_cnd_unitfolio_max,title,"unitfolio",&nc);
			project()->addConductorAutoNum(title,nc);
		}
	}

	//Ten Folio
	if (m_cnd_tenfolio_max.isEmpty() || !m_cnd_tenfolio_max.contains(title)) {
		//Insert Initial Value
		if (formula.contains("%seqtf_")) {
			insertFolioSeqHash(&m_cnd_tenfolio_max,title,"tenfolio",&nc);
			project()->addConductorAutoNum(title,nc);
		}
	}
	else if (m_cnd_tenfolio_max.contains(title)) {
		//Load Folio Current Value
		if (formula.contains("%seqtf_")) {
			loadFolioSeqHash(&m_cnd_tenfolio_max,title,"tenfolio",&nc);
			project()->addConductorAutoNum(title,nc);
		}
	}

	//Hundred Folio
	if (m_cnd_hundredfolio_max.isEmpty() || !m_cnd_hundredfolio_max.contains(title)) {
		//Insert Initial Value
		if (formula.contains("%seqhf_")) {
			insertFolioSeqHash(&m_cnd_hundredfolio_max,title,"hundredfolio",&nc);
			project()->addConductorAutoNum(title,nc);
		}
	}
	else if (m_cnd_hundredfolio_max.contains(title)) {
		//Load Folio Current Value
		if (formula.contains("%seqhf_")) {
			loadFolioSeqHash(&m_cnd_hundredfolio_max,title,"hundredfolio",&nc);
			project()->addConductorAutoNum(title,nc);
		}
	}
}

/**
	@return le titre du cartouche
*/
QString Diagram::title() const {
	return(border_and_titleblock.title());
}

/**
	@return la liste des elements de ce schema
*/
QList<CustomElement *> Diagram::customElements() const {
	QList<CustomElement *> elements_list;
	foreach(QGraphicsItem *qgi, items()) {
		if (CustomElement *elmt = qgraphicsitem_cast<CustomElement *>(qgi)) {
			elements_list << elmt;
		}
	}
	return(elements_list);
}

QList <Element *> Diagram::elements() const {
	QList<Element *> element_list;
	foreach (QGraphicsItem *qgi, items()) {
		if (Element *elmt = qgraphicsitem_cast<Element *>(qgi))
			element_list <<elmt;
	}
	return (element_list);
}

/**
 * @brief Diagram::conductors
 * Return the list containing all conductors
 */
QList <Conductor *> Diagram::conductors() const {
	QList<Conductor *> cnd_list;
	foreach (QGraphicsItem *qgi, items()) {
		if (Conductor *cnd = qgraphicsitem_cast<Conductor *>(qgi))
			cnd_list <<cnd;
	}
	return (cnd_list);
}

ElementsMover &Diagram::elementsMover() {
	return m_elements_mover;
}

ElementTextsMover &Diagram::elementTextsMover() {
	return m_element_texts_mover;
}

/**
	Permet de savoir si un element est utilise sur un schema
	@param location Emplacement d'un element
	@return true si l'element location est utilise sur ce schema, false sinon
*/
bool Diagram::usesElement(const ElementsLocation &location) {
	foreach(CustomElement *element, customElements()) {
		if (element -> location() == location) {
			return(true);
		}
	}
	return(false);
}

/**
	@param a title block template name
	@return true if the provided template is used by this diagram, false
	otherwise.
*/
bool Diagram::usesTitleBlockTemplate(const QString &name) {
	return(name == border_and_titleblock.titleBlockTemplateName());
}

/**
 * @brief Diagram::freezeElements
 * Freeze every existent element label.
 */
void Diagram::freezeElements(bool freeze) {
	foreach (Element *elmt, elements()) {
		elmt->freezeLabel(freeze);
	}
}

/**
 * @brief Diagram::unfreezeElements
 * Unfreeze every existent element label.
 */
void Diagram::unfreezeElements() {
	foreach (Element *elmt, elements()) {
		elmt->freezeLabel(false);
	}
}

/**
 * @brief Diagram::freezeNewElements
 * Set new element label to be frozen.
 */
void Diagram::setFreezeNewElements(bool b) {
	m_freeze_new_elements = b;
}

/**
 * @brief Diagram::freezeNewElements
 * @return current freeze new element status .
 */
bool Diagram::freezeNewElements() {
	return m_freeze_new_elements;
}

/**
 * @brief Diagram::freezeConductors
 * Freeze every existent conductor label.
 */
void Diagram::freezeConductors(bool freeze) {
	foreach (Conductor *cnd, conductors()) {
		cnd->setFreezeLabel(freeze);
	}
}

/**
 * @brief Diagram::setfreezeNewConductors
 * Set new conductor label to be frozen.
 */
void Diagram::setFreezeNewConductors(bool b) {
	m_freeze_new_conductors_ = b;
}

/**
 * @brief Diagram::freezeNewConductors
 * @return current freeze new conductor status .
 */
bool Diagram::freezeNewConductors() {
	return m_freeze_new_conductors_;
}

/**
 * @brief Diagram::adjustSceneRect
 * Recalcul and adjust the size of the scene
 */
void Diagram::adjustSceneRect()
{
	QRectF old_rect = sceneRect();
	setSceneRect(border_and_titleblock.borderAndTitleBlockRect().united(itemsBoundingRect()));
	update(old_rect.united(sceneRect()));
}

/**
	Cette methode permet d'appliquer de nouvelles options de rendu tout en
	accedant aux proprietes de rendu en cours.
	@param new_properties Nouvelles options de rendu a appliquer
	@return les options de rendu avant l'application de new_properties
*/
ExportProperties Diagram::applyProperties(const ExportProperties &new_properties) {
	// exporte les options de rendu en cours
	ExportProperties old_properties;
	old_properties.draw_grid               = displayGrid();
	old_properties.draw_border             = border_and_titleblock.borderIsDisplayed();
	old_properties.draw_titleblock         = border_and_titleblock.titleBlockIsDisplayed();
	old_properties.draw_terminals          = drawTerminals();
	old_properties.draw_colored_conductors = drawColoredConductors();
	old_properties.exported_area           = useBorder() ? QET::BorderArea : QET::ElementsArea;
	
	// applique les nouvelles options de rendu
	setUseBorder                  (new_properties.exported_area == QET::BorderArea);
	setDrawTerminals              (new_properties.draw_terminals);
	setDrawColoredConductors      (new_properties.draw_colored_conductors);
	setDisplayGrid                (new_properties.draw_grid);
	border_and_titleblock.displayBorder(new_properties.draw_border);
	border_and_titleblock.displayTitleBlock (new_properties.draw_titleblock);
	
	// retourne les anciennes options de rendu
	return(old_properties);
}

/**
	@param pos Position cartesienne (ex : 10.3, 45.2) a transformer en position
	dans la grille (ex : B2)
	@return la position dans la grille correspondant a pos
*/
DiagramPosition Diagram::convertPosition(const QPointF &pos) {
	// delegue le calcul au BorderTitleBlock
	DiagramPosition diagram_position = border_and_titleblock.convertPosition(pos);
	
	// embarque la position cartesienne
	diagram_position.setPosition(pos);
	
	return(diagram_position);
}

/**
 * @brief Diagram::snapToGrid
 * Return a nearest snap point of p
 * @param p point to find the nearest snaped point
 * @return
 */
QPointF Diagram::snapToGrid(const QPointF &p)
{
		//Return a point rounded to the nearest pixel
	if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
	{
		int p_x = qRound(p.x());
		int p_y = qRound(p.y());
		return (QPointF(p_x, p_y));
	}

		//Return a point snapped to the grid
	int p_x = qRound(p.x() / Diagram::xGrid) * Diagram::xGrid;
	int p_y = qRound(p.y() / Diagram::yGrid) * Diagram::yGrid;
	return (QPointF(p_x, p_y));
}



/**
	Definit s'il faut afficher ou non les bornes
	@param dt true pour afficher les bornes, false sinon
*/
void Diagram::setDrawTerminals(bool dt) {
	foreach(QGraphicsItem *qgi, items()) {
		if (Terminal *t = qgraphicsitem_cast<Terminal *>(qgi)) {
			t -> setVisible(dt);
		}
	}
}

/**
	Definit s'il faut respecter ou non les couleurs des conducteurs.
	Si non, les conducteurs sont tous dessines en noir.
	@param dcc true pour respecter les couleurs, false sinon
*/
void Diagram::setDrawColoredConductors(bool dcc) {
	draw_colored_conductors_ = dcc;
}

/**
	@return la liste des conducteurs selectionnes sur le schema
*/
QSet<Conductor *> Diagram::selectedConductors() const {
	QSet<Conductor *> conductors_set;
	foreach(QGraphicsItem *qgi, selectedItems()) {
		if (Conductor *c = qgraphicsitem_cast<Conductor *>(qgi)) {
			conductors_set << c;
		}
	}
	return(conductors_set);
}

/// @return true si le presse-papier semble contenir un schema
bool Diagram::clipboardMayContainDiagram() {
	QString clipboard_text = QApplication::clipboard() -> text().trimmed();
	bool may_be_diagram = clipboard_text.startsWith("<diagram") && clipboard_text.endsWith("</diagram>");
	return(may_be_diagram);
}

/**
	@return le projet auquel ce schema appartient ou 0 s'il s'agit d'un schema
	independant.
*/
QETProject *Diagram::project() const {
	return(m_project);
}

/**
 * @brief Diagram::setProject
 * Set parent project of this diagram, project also become the parent QObject of this diagram
 * @param project new project
 */
void Diagram::setProject(QETProject *project)
{
	if (m_project == project)
		return;

	m_project = project;
	setParent (project);
}

/**
	@return the folio number of this diagram within its parent project, or -1
	if it is has no parent project
*/
int Diagram::folioIndex() const {
	if (!m_project) return(-1);
	return(m_project -> folioIndex(this));
}

/**
	@param fallback_to_project When a diagram does not have a declared version,
	this method will use the one declared by its parent project only if
	fallback_to_project is true.
	@return the declared QElectroTech version of this diagram
*/
qreal Diagram::declaredQElectroTechVersion(bool fallback_to_project) const {
	if (diagram_qet_version_ != -1) {
		return diagram_qet_version_;
	}
	if (fallback_to_project && m_project) {
		return(m_project -> declaredQElectroTechVersion());
	}
	return(-1);
}

/**
 * @brief Diagram::isReadOnly
 * @return  true if this diagram is read only.
 * This method is same has call Diagram::project() -> isReadOnly()
 */
bool Diagram::isReadOnly() const
{
	return m_project -> isReadOnly();
}

/**
	@return Le contenu du schema. Les conducteurs sont tous places dans
	conductorsToMove.
*/
DiagramContent Diagram::content() const {
	DiagramContent dc;
	foreach(QGraphicsItem *qgi, items()) {
		if (Element *e = qgraphicsitem_cast<Element *>(qgi)) {
			dc.m_elements << e;
		} else if (IndependentTextItem *iti = qgraphicsitem_cast<IndependentTextItem *>(qgi)) {
			dc.m_text_fields << iti;
		} else if (Conductor *c = qgraphicsitem_cast<Conductor *>(qgi)) {
			dc.m_conductors_to_move << c;
		}
	}
	return(dc);
}

/**
 * @brief Diagram::canRotateSelection
 * @return True if a least one of selected items can be rotated
 */
bool Diagram::canRotateSelection() const
{
	for (QGraphicsItem *qgi : selectedItems())
	{
		if (qgi->type() == IndependentTextItem::Type ||
			qgi->type() == ConductorTextItem::Type ||
			qgi->type() == DiagramImageItem::Type ||
			qgi->type() == Element::Type ||
			qgi->type() == DynamicElementTextItem::Type)
			return true;
		
		if(qgi->type() == QGraphicsItemGroup::Type)
			if(dynamic_cast<ElementTextItemGroup *>(qgi))
				return true;
	}
	
	return false;
}
