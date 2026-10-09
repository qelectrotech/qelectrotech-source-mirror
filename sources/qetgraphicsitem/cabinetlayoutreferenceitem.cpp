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

#include "cabinetlayoutreferenceitem.h"

#include "../diagram.h"
#include "../elementprovider.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <qsvgrenderer.h>

CabinetLayoutReferenceItem::CabinetLayoutReferenceItem(
		const QUuid &source_element_uuid,
		bool is_side_view,
		QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_source_element_uuid(source_element_uuid),
	m_is_side_view(is_side_view)
{
	setFlags(QGraphicsItem::ItemIsMovable
			 | QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
	setZValue(10);
	m_font = QFont(QStringLiteral("Sans Serif"), 7);
}

CabinetLayoutReferenceItem::~CabinetLayoutReferenceItem()
{
}

/**
	@brief CabinetLayoutReferenceItem::setPos
	Bypasses QetGraphicsItem::setPos()'s automatic snap-to-grid --
	this item has its own, purpose-built snapping
	(snapToNearbyReferenceEdges(), computed by the caller before this
	is ever invoked), which the base class's coarse grid-snap would
	otherwise silently override on every single call.
	@param p
*/
void CabinetLayoutReferenceItem::setPos(const QPointF &p)
{
	QGraphicsItem::setPos(p);
}

/**
	@brief CabinetLayoutReferenceItem::boxRect
	The box itself, centered on the local origin -- unlike
	boundingRect(), which additionally reserves space above the box
	for the label when a layout picture is active (@see paint()).
*/
QRectF CabinetLayoutReferenceItem::boxRect() const
{
	return QRectF(-m_box_width / 2, -m_box_height / 2, m_box_width, m_box_height);
}

QRectF CabinetLayoutReferenceItem::boundingRect() const
{
	QRectF rect = boxRect();
	if (m_has_layout_picture) {
		const qreal label_height = QFontMetricsF(m_font).height() + 4;
		rect.setTop(rect.top() - label_height);
	}
	return rect;
}

/**
	@brief fitFontSizeToBox
	Computes and sets a font point size that fits within the
	box's current dimensions. Called exactly once, only at the
	moment a brand-new box is interactively created (@see
	DiagramEventAddCabinetLayoutReference's constructor) --
	never from recomputeBoxSize() itself, since that also runs
	for boxes loaded from file (whose font was already read
	from XML and must not be recalculated) and for live updates
	to an already-placed box's source device.
*/
void CabinetLayoutReferenceItem::fitFontSizeToBox()
{
	const int size = qBound(4, int(qMin(m_box_width, m_box_height) / 8), 7);
	m_font.setPointSize(size);
}

/**
	@brief CabinetLayoutReferenceItem::recomputeBoxSize
	Recomputes m_box_width/m_box_height from the cached real-world
	m_real_width_mm/m_real_height_mm and this folio's current
	Diagram::cabinetLayoutScale(). Deliberately reads the *current*
	scale each time rather than caching a pre-scaled pixel size, so an
	orphaned reference (whose mm values can't be refreshed until the
	next reload) still tracks a later change to the folio's own scale
	setting correctly.
*/
void CabinetLayoutReferenceItem::recomputeBoxSize()
{
	prepareGeometryChange();

	qreal scale = diagram() ? diagram()->cabinetLayoutScale() : 0.0;
	if (scale <= 0.0)
		scale = 2.0; //fall back to the library's informal default

	m_box_width  = (m_real_width_mm  > 0.0) ? m_real_width_mm  * scale : 20.0;
	m_box_height = (m_real_height_mm > 0.0) ? m_real_height_mm * scale : 20.0;

	update();
}

void CabinetLayoutReferenceItem::paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *options,
		QWidget *)
{
	Q_UNUSED(options)

	painter->save();
	drawBox(painter);
	drawLabel(painter);

	if (isSelected() || isHovered()) {
		QPen sel_pen(Qt::blue);
		sel_pen.setStyle(Qt::DashLine);
		sel_pen.setCosmetic(true);
		painter->setPen(sel_pen);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(boundingRect().adjusted(1, 1, -1, -1));
	}

	painter->restore();
}

/**
	@brief CabinetLayoutReferenceItem::applyElementData
	Pulls label/width/height/depth directly off an already-found,
	live Element -- no database round-trip -- and updates this item's
	cache and on-screen size accordingly. Shared by linkToSource()'s
	success path and onSourceInfoChanged().
*/
void CabinetLayoutReferenceItem::applyElementData(Element *element)
{
	if (m_displayed_info_key == QLatin1String("label")) {
		m_label = element->actualLabel();
	} else {
		m_label = element->elementInformations()
				.value(m_displayed_info_key).toString();
	}
	if (m_label.isEmpty())
		m_label = QObject::tr("(sans label)");

	const DiagramContext info = element->elementInformations();
	bool w_ok = false, h_ok = false;
	const qreal width_mm  = info.value(QStringLiteral("width")).toString().toDouble(&w_ok);
	const qreal height_mm = info.value(QStringLiteral("height")).toString().toDouble(&h_ok);
	const qreal depth_mm  = info.value(QStringLiteral("depth")).toString().toDouble();

	m_real_width_mm  = w_ok ? (m_is_side_view ? depth_mm : width_mm) : 0.0;
	m_real_height_mm = h_ok ? height_mm : 0.0;

	recomputeBoxSize();
	applyLayoutPicture(element);
}

/**
	@brief CabinetLayoutReferenceItem::applyLayoutPicture
	Fills m_brush with the source element's own embedded layout
	reference (@see ElementData::m_layout_reference), if one exists
	with actually embedded data -- an unconverted "reference" (source
	== "reference" but data still empty, i.e. the symbol author picked
	a collection element but never checked "Convertir en SVG") is
	treated the same as having none at all, matching the deliberate
	design that generation only ever happens as an explicit editor
	action, never implicitly here at draw time.
	Falls back to the existing solid-color fill (unchanged from
	before this feature existed) when there's nothing to show.
*/
void CabinetLayoutReferenceItem::applyLayoutPicture(Element *element)
{
	prepareGeometryChange();

	const ElementData::ReferenceData &ref = element->elementData().m_layout_reference;

	m_layout_svg_renderer.reset();

	if (ref.data.isEmpty()) {
		m_has_layout_picture = false;
		update();
		return;
	}

	if (!m_has_layout_picture)
		m_text_rotation = 0;

	m_has_layout_picture = true;

	if (ref.format == QLatin1String("svg")) {
		m_layout_svg_renderer.reset(new QSvgRenderer(ref.data));
		m_box_brush = QBrush();
	} else {
		QPixmap pixmap;
		pixmap.loadFromData(ref.data);
		m_box_brush = QBrush(pixmap.scaled(boundingRect().size().toSize(),
										Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}

	update();
}

void CabinetLayoutReferenceItem::drawBox(QPainter *painter) const
{
	QPen pen = m_box_pen;
	if (m_is_orphaned) {
		pen.setColor(Qt::red);
		pen.setStyle(Qt::DashLine);
	}
	pen.setCosmetic(true);
	painter->setPen(pen);

	const QRectF box_rect = boxRect();

	if (m_layout_svg_renderer) {
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(box_rect);

		QSizeF svg_size = m_layout_svg_renderer->viewBoxF().size();
		if (svg_size.isEmpty())
			svg_size = m_layout_svg_renderer->defaultSize();

		if (!svg_size.isEmpty()) {
			svg_size.scale(box_rect.size(), Qt::KeepAspectRatio);
			QRectF target_rect(
				box_rect.center().x() - svg_size.width() / 2.0,
				box_rect.center().y() - svg_size.height() / 2.0,
				svg_size.width(), svg_size.height());
			m_layout_svg_renderer->render(painter, target_rect);
		}
		else
		{
			m_layout_svg_renderer->render(painter, box_rect);
		}
	} else {
		painter->setBrush(m_box_brush);
		painter->setBrushOrigin(box_rect.topLeft());
		painter->drawRect(box_rect);
	}
}

void CabinetLayoutReferenceItem::drawLabel(QPainter *painter) const
{
	QPen pen = m_text_pen;
	if (m_is_orphaned)
		pen.setColor(Qt::red);
	pen.setStyle(Qt::SolidLine);
	pen.setCosmetic(true);
	painter->setPen(pen);

	painter->setFont(m_font);
	const QString display_label = m_is_orphaned
			? QStringLiteral("⚠ ") + m_label
			: m_label;

	const QRectF box_rect = boxRect();

	painter->save();
	if (m_has_layout_picture) {
		const qreal label_height = QFontMetricsF(m_font).height() + 4;
		painter->translate(box_rect.center().x(), box_rect.top() - label_height / 2.0);
	}
	painter->rotate(m_text_rotation);
	const qreal longest_side = qMax(m_box_width, m_box_height);
	QRectF text_rect(-longest_side / 2, -longest_side / 2, longest_side, longest_side);
	painter->drawText(text_rect, Qt::AlignCenter, display_label);
	painter->restore();
}

void CabinetLayoutReferenceItem::setDisplayedInfoKey(const QString &key)
{
	m_displayed_info_key = key;
	if (m_source_element)
		applyElementData(m_source_element.data());
	else
		update();
}

/**
	@brief CabinetLayoutReferenceItem::linkToSource
	See header.
*/
void CabinetLayoutReferenceItem::linkToSource(QETProject *project)
{
	if (!project)
		return;

	ElementProvider provider(project);
	const QList<Element *> found = provider.fromUuids({m_source_element_uuid});
	if (found.isEmpty()) {
		m_is_orphaned = true;
		update();
		return;
	}

	m_source_element = found.first();
	m_is_orphaned = false;

	connect(m_source_element.data(), &Element::elementInfoChange,
			this, &CabinetLayoutReferenceItem::onSourceInfoChanged,
			Qt::UniqueConnection);
	connect(m_source_element.data(), &QObject::destroyed,
			this, &CabinetLayoutReferenceItem::onSourceDestroyed,
			Qt::UniqueConnection);

	applyElementData(m_source_element.data());
}

/**
	@brief CabinetLayoutReferenceItem::onSourceInfoChanged
	See header.
*/
void CabinetLayoutReferenceItem::onSourceInfoChanged()
{
	if (!m_source_element)
		return;

	applyElementData(m_source_element.data());
}

/**
	@brief CabinetLayoutReferenceItem::onSourceDestroyed
	See header.
*/
void CabinetLayoutReferenceItem::onSourceDestroyed()
{
	m_is_orphaned = true;
	m_source_element = nullptr;
	update();
}

/**
	@brief CabinetLayoutReferenceItem::jumpToSource
	See header.
*/
void CabinetLayoutReferenceItem::jumpToSource() const
{
	if (!m_source_element)
		return;

	Element *source = m_source_element.data();
	if (!source->diagram())
		return;

	source->diagram()->showMe();
	source->diagram()->clearSelection();
	source->setSelected(true);

	if (!source->diagram()->views().isEmpty())
		source->diagram()->views().first()->centerOn(source);
}

void CabinetLayoutReferenceItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event)
	jumpToSource();
}

/**
	@brief CabinetLayoutReferenceItem::existsReferenceFor
	See header.
*/
bool CabinetLayoutReferenceItem::existsReferenceFor(
		QETProject *project,
		const QUuid &source_element_uuid,
		bool is_side_view)
{
	if (!project)
		return false;

	for (Diagram *dia : project->diagrams()) {
		for (QGraphicsItem *item : dia->items()) {
			if (item->type() != CabinetLayoutReferenceItem::Type)
				continue;

			auto *reference = static_cast<CabinetLayoutReferenceItem *>(item);
			if (reference->sourceElementUuid() == source_element_uuid
				&& reference->isSideView() == is_side_view) {
				return true;
			}
		}
	}
	return false;
}

/**
	@brief CabinetLayoutReferenceItem::toXml
	Stores only what can't be recomputed from a live Element (the
	source's UUID, this item's own position/rotation), plus a cache of
	the source's last known label and real-world width/height in mm.
	The cache exists purely for the orphaned case (@see linkToSource()
	and the class comment on why linking is a one-shot, not a live
	database lookup): once successfully linked, it's kept accurate via
	Element::elementInfoChange, so a save always persists the current
	state.
*/
QDomElement CabinetLayoutReferenceItem::toXml(QDomDocument &document) const
{
	QDomElement element = document.createElement(QStringLiteral("cabinetLayoutReference"));
	element.setAttribute(QStringLiteral("source_uuid"), m_source_element_uuid.toString());
	element.setAttribute(QStringLiteral("side_view"),
						  m_is_side_view ? QStringLiteral("true") : QStringLiteral("false"));
	element.setAttribute(QStringLiteral("x"), QString::number(pos().x()));
	element.setAttribute(QStringLiteral("y"), QString::number(pos().y()));
	element.setAttribute(QStringLiteral("z"), QString::number(zValue()));
	element.setAttribute(QStringLiteral("rotation"), QString::number(rotation()));
	element.setAttribute(QStringLiteral("last_known_label"), m_label);
	element.setAttribute(QStringLiteral("last_known_width_mm"), QString::number(m_real_width_mm));
	element.setAttribute(QStringLiteral("last_known_height_mm"), QString::number(m_real_height_mm));
	element.setAttribute(QStringLiteral("pen_color"), m_box_pen.color().name());
	element.setAttribute(QStringLiteral("pen_width"), QString::number(m_box_pen.widthF()));
	element.setAttribute(QStringLiteral("brush_color"), m_box_brush.color().name(QColor::HexArgb));
	element.setAttribute(QStringLiteral("text_rotation"), QString::number(m_text_rotation));
	element.setAttribute(QStringLiteral("font"), m_font.toString());
	element.setAttribute(QStringLiteral("displayed_info_key"), m_displayed_info_key);
	return element;
}

/**
	@brief CabinetLayoutReferenceItem::fromXml
	Parses this item's own saved state directly -- no database or
	Element lookup here at all, hence no dependency on load order
	across folios. Always shows the last known cached label/size
	immediately, as orphaned, until linkToSource() (called once from
	Diagram::refreshContents(), after every folio has finished
	loading) either confirms the source is still there or leaves it
	orphaned.
*/
bool CabinetLayoutReferenceItem::fromXml(const QDomElement &dom_element)
{
	if (dom_element.tagName() != QLatin1String("cabinetLayoutReference"))
		return false;

	m_source_element_uuid = QUuid(dom_element.attribute(QStringLiteral("source_uuid")));
	m_is_side_view = dom_element.attribute(QStringLiteral("side_view")) == QLatin1String("true");

	QGraphicsObject::setPos(dom_element.attribute(QStringLiteral("x")).toDouble(),
							 dom_element.attribute(QStringLiteral("y")).toDouble());
	setZValue(dom_element.attribute(QStringLiteral("z"), QString::number(zValue())).toDouble());
	setRotation(dom_element.attribute(QStringLiteral("rotation"), QStringLiteral("0")).toDouble());

	m_label = dom_element.attribute(QStringLiteral("last_known_label"));
	if (m_label.isEmpty())
		m_label = QObject::tr("(sans label)");
	m_real_width_mm  = dom_element.attribute(QStringLiteral("last_known_width_mm")).toDouble();
	m_real_height_mm = dom_element.attribute(QStringLiteral("last_known_height_mm")).toDouble();

	m_box_pen.setColor(QColor(dom_element.attribute(QStringLiteral("pen_color"), QStringLiteral("#000000"))));
	m_box_pen.setWidthF(dom_element.attribute(QStringLiteral("pen_width"), QStringLiteral("1")).toDouble());
	m_box_brush.setColor(QColor(dom_element.attribute(QStringLiteral("brush_color"), QStringLiteral("#41c8c8c8"))));
	m_box_brush.setStyle(Qt::SolidPattern);
	m_text_rotation = dom_element.attribute(QStringLiteral("text_rotation"), QStringLiteral("270")).toDouble();
	QFont f; f.fromString(dom_element.attribute(QStringLiteral("font"), m_font.toString()));
	m_font = f;
	m_displayed_info_key = dom_element.attribute(QStringLiteral("displayed_info_key"), QStringLiteral("label"));

	m_is_orphaned = true; //until linkToSource() says otherwise
	recomputeBoxSize();

	return true;
}