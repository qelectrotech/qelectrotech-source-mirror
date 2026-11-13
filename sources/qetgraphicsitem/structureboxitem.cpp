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
#include "../autoNum/assignvariables.h"
#include "../diagram.h"
#include "element.h"
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"
#include "../qetxml.h"
#include "../ui/structureboxpropertieswidget.h"

#include <QDomDocument>
#include <QPainter>

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
void StructureBoxItem::refreshSymbolTexts(Diagram *diagram)
{
	if (!diagram) {
		return;
	}
	for (Element *element : diagram->elements()) {
		refreshTextsOf(element);
	}
}

/**
	@brief StructureBoxItem::refreshTextsOf
	Work out again the texts of @p element that show a reference
	designation or a place: %{structure_id} and %{structure_id_full} read
	the boxes around the symbol (enclosing()) and its folio, and so do
	%{plant} and %{location} when the project inherits them
	(QETProject::plantAndLocationInherited()). A symbol does not watch them.
*/
void StructureBoxItem::refreshTextsOf(Element *element)
{
	if (!element || !element->diagram()) {
		return;
	}
	const auto refresh = [](DynamicElementTextItem *text) {
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
}

void StructureBoxItem::setPlant(const QString &plant)
{
	if (m_plant == plant) return;
	m_plant = plant;
	update();
	emit identityChanged();
}

void StructureBoxItem::setLocation(const QString &location)
{
	if (m_location == location) return;
	m_location = location;
	update();
	emit identityChanged();
}

void StructureBoxItem::setPrefix(const QString &prefix)
{
	if (m_prefix == prefix) return;
	m_prefix = prefix;
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
	return autonum::AssignVariables::buildStructureId(m_plant, m_location, m_prefix);
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
	QList<QPair<qreal, const StructureBoxItem *>> found;
	Diagram *diagram = element ? element->diagram() : nullptr;
	if (!diagram)
		return {};
	const QPointF pos = element->sceneBoundingRect().center();
		//By bounding rect, then the box's own rectangle: a box is not
		//filled, so a hit test on its shape would only find its outline
		//and miss every symbol drawn inside it.
	const auto items = diagram->items(pos, Qt::IntersectsItemBoundingRect);
	for (QGraphicsItem *item : items)
	{
		if (item->type() != StructureBoxItem::Type)
			continue;
		auto box = static_cast<const StructureBoxItem *>(item);
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
	Place place;
	Diagram *diagram = element ? element->diagram() : nullptr;
	if (diagram) {
		place.plant = diagram->border_and_titleblock.plant();
		place.location = diagram->border_and_titleblock.locmach();
	}
	const auto boxes = enclosing(element);
	for (auto it = boxes.crbegin() ; it != boxes.crend() ; ++it) {
		if (!(*it)->plant().isEmpty())    place.plant = (*it)->plant();
		if (!(*it)->location().isEmpty()) place.location = (*it)->location();
	}
	return place;
}

/**
	@brief StructureBoxItem::placeOf
	@param element an element, possibly null
	@return the element's own plant and location, an empty one taken from
	referencePlace()
*/
StructureBoxItem::Place StructureBoxItem::placeOf(const Element *element)
{
	if (!element)
		return {};
	const DiagramContext info = element->elementInformations();
	Place place{info.value(QStringLiteral("plant")).toString(),
				info.value(QStringLiteral("location")).toString()};
	if (place.plant.isEmpty() || place.location.isEmpty()) {
		const Place reference = referencePlace(element);
		if (place.plant.isEmpty())    place.plant = reference.plant;
		if (place.location.isEmpty()) place.location = reference.location;
	}
	return place;
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

void StructureBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	QetShapeItem::paint(painter, option, widget);

	const QString id = structureId();
	if (id.isEmpty())
		return;

	painter->save();
	QFont font = painter->font();
	font.setPointSizeF(8);
	painter->setFont(font);
	painter->setPen(pen().color());
	const QRectF text_rect = rect().normalized().adjusted(2, 2, -2, -2);
	painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignTop, id);
	painter->restore();
}
