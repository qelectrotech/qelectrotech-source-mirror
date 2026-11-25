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
#include "structureboxitem.h"

#include "../PropertiesEditor/propertieseditordialog.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../autoNum/assignvariables.h"
#include "../diagram.h"
#include "element.h"
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"
#include "../qetproject.h"
#include "../qetxml.h"
#include "../ui/structureboxpropertieswidget.h"

#include <QApplication>
#include <QDomDocument>
#include <QSettings>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QLineF>

#include <algorithm>

StructureBoxItem::StructureBoxItem(QPointF p1, QPointF p2, QGraphicsItem *parent) :
	QetShapeItem(p1, p2, QetShapeItem::Rectangle, parent)
{
		//A location boundary is drawn dash-dot (IEC 60617, IEC 61082).
		//Only the default: the pen is saved with the box, so a box
		//drawn another way keeps its own line.
	QPen dash_dot = pen();
	dash_dot.setStyle(Qt::DashDotLine);
	setPen(dash_dot);
		//What the symbols inside take from it changed
	connect(this, &StructureBoxItem::identityChanged, this, [this]() { refreshSymbolTexts(diagram()); });
	connect(this, &QetShapeItem::geometryChanged, this, [this]() { refreshSymbolTexts(diagram()); });
	m_label = new StructureBoxLabelItem(this);
	connect(m_label, &QGraphicsObject::xChanged,
			this, &StructureBoxItem::labelPosChanged);
	connect(m_label, &QGraphicsObject::yChanged,
			this, &StructureBoxItem::labelPosChanged);
	connect(this, &QetShapeItem::geometryChanged,
			this, &StructureBoxItem::placeLabel);
		//The name is drawn in the colour of the box's line
	connect(this, &QetShapeItem::penChanged, this, &StructureBoxItem::refreshLabel);
	refreshLabel();
}

/**
	@brief StructureBoxItem::itemChange
	A box put on a folio (drawn, pasted, loaded), taken off it (deleted,
	undone) or moved changes the place of the symbols it holds or held:
	their texts are worked out again. Loaded, a box comes after the symbols
	of its folio, which first worked out their texts without it.
*/
QVariant StructureBoxItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	const QVariant result = QetShapeItem::itemChange(change, value);
	if (change == ItemSceneHasChanged) {
		if (m_folio && m_folio != diagram()) {
			refreshSymbolTexts(m_folio);
		}
		m_folio = diagram();
		refreshSymbolTexts(m_folio);
	} else if (change == ItemPositionHasChanged) {
		refreshSymbolTexts(diagram());
	}
	return result;
}

/**
	@brief StructureBoxItem::refreshSymbolTexts
	Work out again every text of the symbols of @p diagram that shows a
	reference designation or a place (refreshTextsOf()).
*/
void StructureBoxItem::refreshSymbolTexts(Diagram *diagram, bool label_fields)
{
	if (!diagram) {
		return;
	}
	for (Element *element : diagram->elements()) {
		refreshTextsOf(element, label_fields);
	}
}

/**
	@brief StructureBoxItem::refreshTextsOf
	Work out again the texts of @p element that show a reference
	designation or a place: %{structure_id} and %{structure_id_full} read
	the boxes around the symbol (enclosing()) and its folio, and so do
	%{plant} and %{location} when the project inherits them
	(QETProject::plantAndLocationInherited()). A symbol does not watch them.
	So does its label field when the project shows it as a reference
	designation (QETProject::designationDisplay()), or @p label_fields
	asks for it (the setting has just changed), and the label fields of
	the elements linked to it: a contact showing its coil's.
*/
void StructureBoxItem::refreshTextsOf(Element *element, bool label_fields)
{
	if (!element || !element->diagram()) {
		return;
	}
	const QETProject *project = element->diagram()->project();
	const bool designation = label_fields || (project
			&& project->designationDisplay() != QETProject::DesignationDisplay::Label);
	const auto isLabelField = [](const DynamicElementTextItem *text) {
		return text->textFrom() == DynamicElementTextItem::ElementInfo
				&& text->infoName() == QLatin1String("label");
	};
	const auto refresh = [designation, isLabelField](DynamicElementTextItem *text) {
		if (designation && isLabelField(text)) {
			text->refreshConductorText();
			return;
		}
		if (text->textFrom() != DynamicElementTextItem::CompositeText) {
			return;
		}
		const QString composite = text->compositeText();
		if (composite.contains(QLatin1String("%{structure_id"))
				|| composite.contains(QLatin1String("%{plant}"))
				|| composite.contains(QLatin1String("%{location}"))) {
			text->refreshConductorText();
		}
	};
	for (DynamicElementTextItem *text : element->dynamicTextItems())
		refresh(text);
	for (ElementTextItemGroup *group : element->textGroups())
		for (DynamicElementTextItem *text : group->texts())
			refresh(text);

	if (!designation) {
		return;
	}
	for (Element *linked : element->linkedElements()) {
		if (!linked || linked == element) {
			continue;
		}
		const auto refreshLabel = [isLabelField](DynamicElementTextItem *text) {
			if (isLabelField(text)) {
				text->refreshConductorText();
			}
		};
		for (DynamicElementTextItem *text : linked->dynamicTextItems())
			refreshLabel(text);
		for (ElementTextItemGroup *group : linked->textGroups())
			for (DynamicElementTextItem *text : group->texts())
				refreshLabel(text);
	}
}

/**
	@brief StructureBoxItem::refreshLabel
	Its label says the box's designation, in the box's colour and size.
	A box with no designation has no label to click.
*/
void StructureBoxItem::refreshLabel()
{
	QFont font = m_label->font();
	font.setPointSizeF(m_label_size);
	m_label->setFont(font);
	m_label->setDefaultTextColor(pen().color());
	const QString id = structureId();
	m_label->setPlainText(id);
	m_label->setVisible(!id.isEmpty());
	placeLabel();
}

/**
	@brief StructureBoxItem::placeLabel
	A label nobody has moved stands above the top left corner, outside the
	box, as a device's designation stands outside its frame; it stays there
	however the box is resized.
*/
void StructureBoxItem::placeLabel()
{
	if (!m_label_moved)
		m_label->setPos(cornerLabelPos());
}

/**
	@return where a label nobody has moved stands: above the top left
	corner, outside the box
*/
QPointF StructureBoxItem::cornerLabelPos() const
{
	return rect().normalized().topLeft()
			- QPointF(0, m_label->boundingRect().height());
}

/**
	@brief StructureBoxItem::labelPosChanged
	A label put back on the corner, by a drag or by an undo, follows the
	corner again and is saved as never moved.
*/
void StructureBoxItem::labelPosChanged()
{
	m_label_moved = QLineF(m_label->pos(), cornerLabelPos()).length() > 0.01;
}

/**
	@return where the label stands, from the box's top left corner
*/
QPointF StructureBoxItem::labelOffset() const
{
	return m_label->pos() - rect().normalized().topLeft();
}

void StructureBoxItem::setLabelOffset(const QPointF &offset)
{
	m_label->setPos(rect().normalized().topLeft() + offset);
}

void StructureBoxItem::setLabelSize(qreal size)
{
	if (qFuzzyCompare(m_label_size, size) || !(size > 0) || !qIsFinite(size))
		return;
	m_label_size = size;
	refreshLabel();
	emit labelSizeChanged();
}

void StructureBoxItem::setPlant(const QString &plant)
{
	if (m_plant == plant) return;
	m_plant = plant;
	refreshLabel();
	update();
	emit identityChanged();
}

void StructureBoxItem::setLocation(const QString &location)
{
	if (m_location == location) return;
	m_location = location;
	refreshLabel();
	update();
	emit identityChanged();
}

void StructureBoxItem::setPrefix(const QString &prefix)
{
	if (m_prefix == prefix) return;
	m_prefix = prefix;
	refreshLabel();
	update();
	emit identityChanged();
}

void StructureBoxItem::setHigherPlant(const QString &higher_plant)
{
	if (m_higher_plant == higher_plant) return;
	m_higher_plant = higher_plant;
	refreshLabel();
	update();
	emit identityChanged();
}

void StructureBoxItem::setHigherLocation(const QString &higher_location)
{
	if (m_higher_location == higher_location) return;
	m_higher_location = higher_location;
	refreshLabel();
	update();
	emit identityChanged();
}

/**
	@brief StructureBoxItem::structureId
	@return this box's own full IEC 81346 designation ("=plant+location-prefix"),
	always unabbreviated since the box itself is a reference point, not
	something abbreviated relative to one.
*/
QString StructureBoxItem::structureId() const
{
		//With its own == and ++, when it has them: ==A1=P1++S1+L1-A1
	return autonum::AssignVariables::buildStructureId(
				autonum::AssignVariables::withHigherLevel(QLatin1Char('='), m_higher_plant, m_plant),
				autonum::AssignVariables::withHigherLevel(QLatin1Char('+'), m_higher_location, m_location),
				m_prefix);
}

/**
	@brief StructureBoxItem::enclosing
	@param element an element, possibly null
	@return the boxes of the element's folio whose rectangle holds the centre
	of the symbol, smallest (most specific) first; empty if the element is
	null, has no folio or lies in no box.
*/
QList<const StructureBoxItem *> StructureBoxItem::enclosing(const Element *element)
{
	Diagram *diagram = element ? element->diagram() : nullptr;
	if (!diagram)
		return {};
		//By bounding rect, then the box's own rectangle: a box is not
		//filled, so a hit test on its shape would only find its outline
		//and miss every symbol drawn inside it.
	return enclosing(element, boxesAmong(diagram->items(
			element->sceneBoundingRect().center(), Qt::IntersectsItemBoundingRect)));
}

/**
	@brief StructureBoxItem::enclosing
	@param element an element, possibly null
	@param folio_boxes the boxes which may hold it: boxesOf() its folio
	@return those of @p folio_boxes whose rectangle holds the centre of the
	symbol, smallest (most specific) first
*/
QList<const StructureBoxItem *> StructureBoxItem::enclosing(
		const Element *element,
		const QList<const StructureBoxItem *> &folio_boxes)
{
	QList<QPair<qreal, const StructureBoxItem *>> found;
	if (!element || !element->diagram())
		return {};
	const QPointF pos = element->sceneBoundingRect().center();
	for (const StructureBoxItem *box : folio_boxes)
	{
		const QRectF r = box->mapRectToScene(box->rect().normalized());
		if (!r.contains(pos))
			continue;
		found.append({r.width() * r.height(), box});
	}
	std::stable_sort(found.begin(), found.end(),
					 [](const auto &a, const auto &b) { return a.first < b.first; });
	QList<const StructureBoxItem *> boxes;
	for (const auto &entry : found) boxes << entry.second;
	return boxes;
}

/**
	@brief StructureBoxItem::referencePlace
	@param element an element, possibly null
	@return the plant and location the boxes around @p element give it,
	field by field from the outermost box in, over its folio's
*/
StructureBoxItem::Place StructureBoxItem::referencePlace(const Element *element)
{
	return placeIn(element, enclosing(element));
}

/**
	@brief StructureBoxItem::referencePlace
	The same, with the boxes looked for among @p folio_boxes
*/
StructureBoxItem::Place StructureBoxItem::referencePlace(
		const Element *element,
		const QList<const StructureBoxItem *> &folio_boxes)
{
	return placeIn(element, enclosing(element, folio_boxes));
}

/**
	@brief StructureBoxItem::placeIn
	@return the place @p boxes, those around @p element smallest first,
	give it over its folio's; the product is the prefixes of @p boxes,
	the largest first
*/
StructureBoxItem::Place StructureBoxItem::placeIn(
		const Element *element,
		const QList<const StructureBoxItem *> &boxes)
{
	Place place;
	Diagram *diagram = element ? element->diagram() : nullptr;
	if (diagram) {
		place.plant = diagram->border_and_titleblock.plant();
		place.location = diagram->border_and_titleblock.locmach();
		place.higher_plant = diagram->border_and_titleblock.higherPlant();
		place.higher_location = diagram->border_and_titleblock.higherLocation();
	}
	for (auto it = boxes.crbegin() ; it != boxes.crend() ; ++it) {
		if (!(*it)->plant().isEmpty())    place.plant = (*it)->plant();
		if (!(*it)->location().isEmpty()) place.location = (*it)->location();
		if (!(*it)->higherPlant().trimmed().isEmpty())    place.higher_plant = (*it)->higherPlant();
		if (!(*it)->higherLocation().trimmed().isEmpty()) place.higher_location = (*it)->higherLocation();
		place.product = autonum::AssignVariables::addProductLevel(place.product, (*it)->prefix());
	}
	return place;
}

/**
	@brief StructureBoxItem::placeOf
	@param element an element, possibly null
	@return the element's own plant and location, an empty one taken from
	referencePlace(), and the product levels of its boxes
*/
StructureBoxItem::Place StructureBoxItem::placeOf(const Element *element)
{
	if (!element)
		return {};
	const DiagramContext info = element->elementInformations();
	Place place{info.value(QStringLiteral("plant")).toString(),
				info.value(QStringLiteral("location")).toString(), QString()};
	const Place reference = referencePlace(element);
	if (place.plant.isEmpty())    place.plant = reference.plant;
	if (place.location.isEmpty()) place.location = reference.location;
		//A symbol has no product levels of its own, only its boxes'
	place.product = reference.product;
		//Its own == and ++ over its boxes' and its folio's, field by field
	const QString own_higher_plant = info.value(QStringLiteral("higher_plant")).toString();
	const QString own_higher_location = info.value(QStringLiteral("higher_location")).toString();
	place.higher_plant = own_higher_plant.trimmed().isEmpty() ? reference.higher_plant : own_higher_plant;
	place.higher_location = own_higher_location.trimmed().isEmpty() ? reference.higher_location
																	 : own_higher_location;
	return place;
}

/**
	@brief StructureBoxItem::Place::plantPart
*/
QString StructureBoxItem::Place::plantPart() const
{
	return autonum::AssignVariables::withHigherLevel(QLatin1Char('='), higher_plant, plant);
}

/**
	@brief StructureBoxItem::Place::locationPart
*/
QString StructureBoxItem::Place::locationPart() const
{
	return autonum::AssignVariables::withHigherLevel(QLatin1Char('+'), higher_location, location);
}

/**
	@brief StructureBoxItem::boxesOf
	@return the structure boxes drawn on @p diagram, possibly null
*/
QList<const StructureBoxItem *> StructureBoxItem::boxesOf(const Diagram *diagram)
{
	return diagram ? boxesAmong(diagram->items()) : QList<const StructureBoxItem *>();
}

/**
	@brief StructureBoxItem::boxesAmong
	@return the structure boxes among @p items
*/
QList<const StructureBoxItem *> StructureBoxItem::boxesAmong(const QList<QGraphicsItem *> &items)
{
	QList<const StructureBoxItem *> boxes;
	for (QGraphicsItem *item : items)
		if (item->type() == StructureBoxItem::Type)
			boxes << static_cast<const StructureBoxItem *>(item);
	return boxes;
}

bool StructureBoxItem::isStructureBoxXml(const QDomElement &e)
{
	return e.tagName() == QLatin1String("shape")
			&& e.attribute(QStringLiteral("structure_box")) == QLatin1String("true");
}

	//The geometry, pen, brush, uuid and transform are the shape's own
	//(QetShapeItem::fromXml()); the box only adds its identity.
bool StructureBoxItem::fromXml(const QDomElement &e)
{
	if (!QetShapeItem::fromXml(e))
		return false;
	m_plant = e.attribute(QStringLiteral("plant"));
	m_location = e.attribute(QStringLiteral("location"));
	m_prefix = e.attribute(QStringLiteral("prefix"));
	m_higher_plant = e.attribute(QStringLiteral("higher_plant"));
	m_higher_location = e.attribute(QStringLiteral("higher_location"));
	m_label_size = e.attribute(QStringLiteral("label_size"),
							   QStringLiteral("8")).toDouble();
	if (!(m_label_size > 0) || !qIsFinite(m_label_size))
		m_label_size = 8;
	refreshLabel();
		//Read into a box already on a folio (an undo), a name saved at the
		//corner goes back to the corner
	if (e.hasAttribute(QStringLiteral("label_x")))
		setLabelOffset(QPointF(e.attribute(QStringLiteral("label_x")).toDouble(),
							   e.attribute(QStringLiteral("label_y")).toDouble()));
	else
		m_label->setPos(cornerLabelPos());
	emit labelSizeChanged();
	update();
	emit identityChanged();
	return true;
}

QDomElement StructureBoxItem::toXml(QDomDocument &document) const
{
	QDomElement result = QetShapeItem::toXml(document);
	result.setAttribute(QStringLiteral("structure_box"), QStringLiteral("true"));
	result.setAttribute(QStringLiteral("plant"), m_plant);
	result.setAttribute(QStringLiteral("location"), m_location);
	result.setAttribute(QStringLiteral("prefix"), m_prefix);
		//Only when set, so a box without them is saved as before
	if (!m_higher_plant.isEmpty())
		result.setAttribute(QStringLiteral("higher_plant"), m_higher_plant);
	if (!m_higher_location.isEmpty())
		result.setAttribute(QStringLiteral("higher_location"), m_higher_location);
	if (m_label_moved) {
		const QPointF offset = labelOffset();
		result.setAttribute(QStringLiteral("label_x"), QString::number(offset.x(), 'f', 1));
		result.setAttribute(QStringLiteral("label_y"), QString::number(offset.y(), 'f', 1));
	}
	if (!qFuzzyCompare(m_label_size, 8.0))
		result.setAttribute(QStringLiteral("label_size"), QString::number(m_label_size));
	return result;
}

void StructureBoxItem::editProperty()
{
	if (diagram() && diagram()->isReadOnly())
		return;

	PropertiesEditorDialog ped(new StructureBoxPropertiesWidget(this), diagram() ? diagram()->views().value(0) : nullptr);
	ped.exec();
}

QString StructureBoxItem::name() const
{
	return tr("a structure box");
}

StructureBoxLabelItem::StructureBoxLabelItem(StructureBoxItem *box) :
	DiagramTextItem(box),
	m_box(box)
{
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	setFlag(QGraphicsItem::ItemIsMovable, true);
}

/**
	@brief StructureBoxLabelItem::mousePressEvent
	Selected with its box, a drag moves the whole selection, as a drag of
	the box would; Ctrl still adds or takes the label alone. Else it
	moves the label alone.
*/
void StructureBoxLabelItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	m_dragging = false;
	m_move_box = event->button() == Qt::LeftButton
			&& !(event->modifiers() & Qt::ControlModifier)
			&& isSelected() && m_box->isSelected();
	if (m_move_box) {
		m_box->mousePressEvent(event);
		return;
	}
	DiagramTextItem::mousePressEvent(event);
	m_start = pos();
	m_grab = pos() - parentItem()->mapFromScene(event->scenePos());
}

void StructureBoxLabelItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_move_box) {
		m_box->mouseMoveEvent(event);
		return;
	}
	if (!(event->buttons() & Qt::LeftButton)) {
		event->ignore();
		return;
	}
	event->accept();
		//A locked box keeps its name where it is
	if (!m_box->isMovable())
		return;
	if (!m_dragging) {
		const QPointF dragged =
				event->scenePos() - event->buttonDownScenePos(Qt::LeftButton);
		if (dragged.manhattanLength() < QApplication::startDragDistance())
			return;
		m_dragging = true;
	}
		//On the text grid like every other text, its middle there when the
		//text grid says so; Ctrl places it freely
	const QPointF wanted = parentItem()->mapToScene(
			parentItem()->mapFromScene(event->scenePos()) + m_grab);
	setPos(parentItem()->mapFromScene(Diagram::snapToTextGrid(wanted, this)));
}

void StructureBoxLabelItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_move_box) {
		m_move_box = false;
		m_box->mouseReleaseEvent(event);
		return;
	}
		//The corner is off the text grid: dropped next to it, the name
		//goes back to it and follows it again
	if (m_dragging
		&& QLineF(pos(), m_box->cornerLabelPos()).length() < 4)
		setPos(m_box->cornerLabelPos());
	const bool moved = m_dragging && pos() != m_start;
	m_dragging = false;
	if (event->button() == Qt::LeftButton && moved) {
		if (Diagram *folio = diagram()) {
				//A parent command: two drags are two undo steps, never
				//merged as the steps of one QPropertyUndoCommand are
			auto *undo = new QUndoCommand(
					tr("Move the name of a structure box"));
			new QPropertyUndoCommand(this, "pos", m_start, pos(), undo);
			folio->undoStack().push(undo);
		}
		event->accept();
		return;
	}
	QGraphicsTextItem::mouseReleaseEvent(event);
}

void StructureBoxLabelItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	event->accept();
	m_box->editProperty();
}

QColor StructureBoxItem::newBoxColor()
{
	const QColor color(QSettings().value(QStringLiteral("diagrameditor/structure_box_color"),
										 QStringLiteral("#000000")).toString());
	return color.isValid() ? color : QColor(Qt::black);
}

void StructureBoxItem::setNewBoxColor(const QColor &color)
{
	QSettings().setValue(QStringLiteral("diagrameditor/structure_box_color"), color.name());
}

bool StructureBoxItem::askPlaceWhenDrawn()
{
	return QSettings().value(QStringLiteral("diagrameditor/structure_box_ask_place"), false).toBool();
}

void StructureBoxItem::setAskPlaceWhenDrawn(bool ask)
{
	QSettings().setValue(QStringLiteral("diagrameditor/structure_box_ask_place"), ask);
}
