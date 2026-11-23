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
#include "definitionmarkstyle.h"

#include <QDomDocument>
#include <QFontMetricsF>
#include <QPainter>
#include <QSettings>
#include <QTransform>
#include <QtMath>

namespace {
	const QString style_tag = QStringLiteral("definition_mark_style");
	const QString shape_tag = QStringLiteral("shape");
	const QString text_tag  = QStringLiteral("text");

	QString colorName(const QColor &color)
	{
		return color.isValid() ? color.name(QColor::HexArgb) : QString();
	}

	QColor colorFrom(const QString &name)
	{
		const QColor color(name);
		return color.isValid() ? color : QColor();
	}

	QString hAlignmentName(Qt::Alignment alignment)
	{
		if (alignment & Qt::AlignLeft)
			return QStringLiteral("left");
		if (alignment & Qt::AlignRight)
			return QStringLiteral("right");
		return QStringLiteral("center");
	}

	QString vAlignmentName(Qt::Alignment alignment)
	{
		if (alignment & Qt::AlignTop)
			return QStringLiteral("top");
		if (alignment & Qt::AlignBottom)
			return QStringLiteral("bottom");
		return QStringLiteral("middle");
	}

	Qt::Alignment alignmentFrom(const QString &horizontal, const QString &vertical)
	{
		Qt::Alignment alignment;
		alignment |= horizontal == QLatin1String("left") ? Qt::AlignLeft
			: horizontal == QLatin1String("right") ? Qt::AlignRight
			: Qt::AlignHCenter;
		alignment |= vertical == QLatin1String("top") ? Qt::AlignTop
			: vertical == QLatin1String("bottom") ? Qt::AlignBottom
			: Qt::AlignVCenter;
		return alignment;
	}
}

bool DefinitionMarkStyle::ShapeItem::operator==(const ShapeItem &other) const
{
	return m_kind == other.m_kind
		&& m_pos == other.m_pos
		&& m_rotation == other.m_rotation
		&& m_width == other.m_width
		&& m_height == other.m_height
		&& m_pen_width == other.m_pen_width
		&& m_color == other.m_color
		&& m_filled == other.m_filled;
}

bool DefinitionMarkStyle::ShapeItem::operator!=(const ShapeItem &other) const
{
	return !(*this == other);
}

bool DefinitionMarkStyle::TextItem::operator==(const TextItem &other) const
{
	return m_formula == other.m_formula
		&& m_pos == other.m_pos
		&& m_rotation == other.m_rotation
		&& m_size == other.m_size
		&& m_color == other.m_color
		&& m_upright == other.m_upright
		&& m_alignment == other.m_alignment;
}

bool DefinitionMarkStyle::TextItem::operator!=(const TextItem &other) const
{
	return !(*this == other);
}

/**
	@return the style the mark has always been drawn with: one slanted
	stroke, the colour beside its upper end and the section beside its
	lower end. A style that is not enabled draws exactly this way; the
	items below are what an editor starts from when it is turned on.
*/
DefinitionMarkStyle DefinitionMarkStyle::standardStyle()
{
	DefinitionMarkStyle style;

	ShapeItem stroke;
	stroke.m_kind = ShapeKind::Line;
	stroke.m_rotation = -45;
	stroke.m_width = 8 * qSqrt(2.0);
	style.m_shapes << stroke;

	TextItem color;
	color.m_formula = QStringLiteral("%wc");
	color.m_pos = QPointF(14, -7);
	style.m_texts << color;

	TextItem section;
	section.m_formula = QStringLiteral("%ws");
	section.m_pos = QPointF(14, 7);
	style.m_texts << section;

	return style;
}

/**
	@return whether the mark is drawn the way it always was, without
	the shapes and texts of a customized style
*/
bool DefinitionMarkStyle::isStandard() const
{
	return !m_enabled;
}

/**
	@brief DefinitionMarkStyle::toXml
	Write the style as a <definition_mark_style> element, with a
	<shape> child per shape and a <text> child per text.
	@param e
*/
void DefinitionMarkStyle::toXml(QDomElement &e) const
{
	e.setAttribute(QStringLiteral("enabled"), m_enabled ? QStringLiteral("1") : QStringLiteral("0"));
	if (m_vertical_turn != 0)
		e.setAttribute(QStringLiteral("vertical_turn"), QString::number(m_vertical_turn));
	QDomDocument document = e.ownerDocument();

	for (const ShapeItem &shape : m_shapes)
	{
		QDomElement shape_e = document.createElement(shape_tag);
		switch (shape.m_kind) {
			case ShapeKind::Circle:    shape_e.setAttribute(QStringLiteral("kind"), QStringLiteral("circle")); break;
			case ShapeKind::Rectangle: shape_e.setAttribute(QStringLiteral("kind"), QStringLiteral("rectangle")); break;
			default:                   shape_e.setAttribute(QStringLiteral("kind"), QStringLiteral("line")); break;
		}
		shape_e.setAttribute(QStringLiteral("x"), QString::number(shape.m_pos.x()));
		shape_e.setAttribute(QStringLiteral("y"), QString::number(shape.m_pos.y()));
		shape_e.setAttribute(QStringLiteral("rotation"), QString::number(shape.m_rotation));
		shape_e.setAttribute(QStringLiteral("width"), QString::number(shape.m_width));
		shape_e.setAttribute(QStringLiteral("height"), QString::number(shape.m_height));
		shape_e.setAttribute(QStringLiteral("pen_width"), QString::number(shape.m_pen_width));
		if (shape.m_color.isValid())
			shape_e.setAttribute(QStringLiteral("color"), colorName(shape.m_color));
		if (shape.m_filled)
			shape_e.setAttribute(QStringLiteral("filled"), QStringLiteral("1"));
		e.appendChild(shape_e);
	}

	for (const TextItem &text : m_texts)
	{
		QDomElement text_e = document.createElement(text_tag);
		text_e.setAttribute(QStringLiteral("formula"), text.m_formula);
		text_e.setAttribute(QStringLiteral("x"), QString::number(text.m_pos.x()));
		text_e.setAttribute(QStringLiteral("y"), QString::number(text.m_pos.y()));
		text_e.setAttribute(QStringLiteral("rotation"), QString::number(text.m_rotation));
		text_e.setAttribute(QStringLiteral("size"), QString::number(text.m_size));
		if (text.m_color.isValid())
			text_e.setAttribute(QStringLiteral("color"), colorName(text.m_color));
		if (text.m_upright)
			text_e.setAttribute(QStringLiteral("upright"), QStringLiteral("1"));
		if (text.m_alignment != Qt::AlignCenter) {
			text_e.setAttribute(QStringLiteral("halign"), hAlignmentName(text.m_alignment));
			text_e.setAttribute(QStringLiteral("valign"), vAlignmentName(text.m_alignment));
		}
		e.appendChild(text_e);
	}
}

/**
	@brief DefinitionMarkStyle::fromXml
	Read back a style written by toXml(). An element that says nothing
	leaves the standard style.
	@param e
*/
void DefinitionMarkStyle::fromXml(const QDomElement &e)
{
	*this = DefinitionMarkStyle();
	if (e.tagName() != style_tag) {
		return;
	}
	m_enabled = e.attribute(QStringLiteral("enabled")).toInt();
	m_vertical_turn = e.attribute(QStringLiteral("vertical_turn")).toDouble();

	for (QDomElement child = e.firstChildElement();
	     !child.isNull();
	     child = child.nextSiblingElement())
	{
		if (child.tagName() == shape_tag)
		{
			ShapeItem shape;
			const QString kind = child.attribute(QStringLiteral("kind"));
			if (kind == QLatin1String("circle"))
				shape.m_kind = ShapeKind::Circle;
			else if (kind == QLatin1String("rectangle"))
				shape.m_kind = ShapeKind::Rectangle;
			shape.m_pos = QPointF(child.attribute(QStringLiteral("x")).toDouble(),
					      child.attribute(QStringLiteral("y")).toDouble());
			shape.m_rotation = child.attribute(QStringLiteral("rotation")).toDouble();
			shape.m_width = child.attribute(QStringLiteral("width"), QStringLiteral("8")).toDouble();
			shape.m_height = child.attribute(QStringLiteral("height"), QStringLiteral("8")).toDouble();
			shape.m_pen_width = child.attribute(QStringLiteral("pen_width")).toDouble();
			shape.m_color = colorFrom(child.attribute(QStringLiteral("color")));
			shape.m_filled = child.attribute(QStringLiteral("filled")).toInt();
			m_shapes << shape;
		}
		else if (child.tagName() == text_tag)
		{
			TextItem text;
			text.m_formula = child.attribute(QStringLiteral("formula"));
			text.m_pos = QPointF(child.attribute(QStringLiteral("x")).toDouble(),
					     child.attribute(QStringLiteral("y")).toDouble());
			text.m_rotation = child.attribute(QStringLiteral("rotation")).toDouble();
			text.m_size = child.attribute(QStringLiteral("size")).toDouble();
			text.m_color = colorFrom(child.attribute(QStringLiteral("color")));
			text.m_upright = child.attribute(QStringLiteral("upright")).toInt();
			text.m_alignment = alignmentFrom(child.attribute(QStringLiteral("halign")),
							 child.attribute(QStringLiteral("valign")));
			m_texts << text;
		}
	}
}

/**
	@brief DefinitionMarkStyle::toSettings
	Save the style in the application's settings, as the default new
	projects draw their marks with.
	@param settings
	@param prefix
*/
void DefinitionMarkStyle::toSettings(QSettings &settings, const QString &prefix) const
{
	QDomDocument document;
	QDomElement element = document.createElement(style_tag);
	toXml(element);
	document.appendChild(element);
	settings.setValue(prefix + QStringLiteral("definitionmarkstyle"), document.toString(-1));
}

/**
	@brief DefinitionMarkStyle::fromSettings
	Read back a style written by toSettings(). Settings that say nothing
	leave the standard style.
	@param settings
	@param prefix
*/
void DefinitionMarkStyle::fromSettings(const QSettings &settings, const QString &prefix)
{
	QDomDocument document;
	document.setContent(settings.value(prefix + QStringLiteral("definitionmarkstyle")).toString());
	fromXml(document.documentElement());
}

/**
	@return the rectangle holding every shape and text of the style,
	around the point where the mark stands. @p turn is the turn the
	drawing takes where the conductor runs up and down, in degrees.
*/
QRectF DefinitionMarkStyle::boundingRect(const RenderContext &context,
					 const QStringList &texts,
					 qreal turn) const
{
	QRectF rect(-1, -1, 2, 2);
	for (const ShapeItem &shape : m_shapes)
		rect = rect.united(itemRect(shape, context, turn));
	for (int i = 0 ; i < m_texts.size() ; ++i)
		rect = rect.united(itemRect(m_texts.at(i), context,
					    i < texts.size() ? texts.at(i) : QString(),
					    turn));
	return rect;
}

/**
	Draw the style's shapes and texts, around the point where the mark
	stands. @p texts holds each text item's formula already resolved,
	@p turn is the turn the drawing takes where the conductor runs up
	and down, in degrees: everything turns with it, the texts that do
	not stay upright among them.
*/
void DefinitionMarkStyle::paint(QPainter *painter,
				const RenderContext &context,
				const QStringList &texts,
				qreal turn) const
{
	for (const ShapeItem &shape : m_shapes)
		paintItem(painter, shape, context, turn);
	for (int i = 0 ; i < m_texts.size() ; ++i)
		paintItem(painter, m_texts.at(i), context,
			  i < texts.size() ? texts.at(i) : QString(),
			  turn);
}

/**
	@return where a shape is drawn
*/
QRectF DefinitionMarkStyle::itemRect(const ShapeItem &shape,
				     const RenderContext &context,
				     qreal turn)
{
	const QPointF at = QTransform().rotate(turn).map(shape.m_pos);
	QTransform turned;
	turned.translate(at.x(), at.y());
	turned.rotate(shape.m_rotation + turn);
	const qreal pen = shape.m_pen_width > 0 ? shape.m_pen_width : context.conductor_width;
	if (shape.m_kind == ShapeKind::Line) {
		const QLineF line(-shape.m_width / 2, 0, shape.m_width / 2, 0);
		QPolygonF polygon;
		polygon << turned.map(line.p1()) << turned.map(line.p2());
		return polygon.boundingRect().adjusted(-pen / 2 - 1, -pen / 2 - 1,
						       pen / 2 + 1, pen / 2 + 1);
	}
	return turned.mapRect(QRectF(-shape.m_width / 2, -shape.m_height / 2,
				     shape.m_width, shape.m_height))
		.adjusted(-1, -1, 1, 1);
}

/**
	@return where a text is drawn, for @p text as its words
*/
QRectF DefinitionMarkStyle::itemRect(const TextItem &item,
				     const RenderContext &context,
				     const QString &text,
				     qreal turn)
{
	QFont font = context.font;
	if (item.m_size > 0)
		font.setPointSizeF(item.m_size);
	const QFontMetricsF metrics(font);
	const QRectF rect = textRect(item, metrics, text);
	QTransform transform;
	transform.rotate(turn);
	const QPointF at = transform.map(item.m_pos);
	QTransform turned;
	turned.translate(at.x(), at.y());
	turned.rotate(item.m_rotation + (item.m_upright ? 0 : turn));
	return turned.mapRect(rect).adjusted(-1, -1, 1, 1);
}

/**
	@return the box a text is written in, its place at m_pos as its
	m_alignment says: centered, or growing in one direction from it
*/
QRectF DefinitionMarkStyle::textRect(const TextItem &item,
				     const QFontMetricsF &metrics,
				     const QString &text)
{
	const qreal width = metrics.horizontalAdvance(text);
	const qreal height = metrics.height();
	const qreal x = (item.m_alignment & Qt::AlignLeft) ? 0
		: (item.m_alignment & Qt::AlignRight) ? -width
		: -width / 2;
	const qreal y = (item.m_alignment & Qt::AlignTop) ? 0
		: (item.m_alignment & Qt::AlignBottom) ? -height
		: -height / 2;
	return QRectF(x, y, width, height);
}

/**
	Draw one shape around the point where the mark stands
*/
void DefinitionMarkStyle::paintItem(QPainter *painter,
				    const ShapeItem &shape,
				    const RenderContext &context,
				    qreal turn)
{
	const QColor color = shape.m_color.isValid() ? shape.m_color : context.conductor_color;
	const qreal pen_width = shape.m_pen_width > 0 ? shape.m_pen_width : context.conductor_width;

	painter->save();
	painter->translate(QTransform().rotate(turn).map(shape.m_pos));
	painter->rotate(shape.m_rotation + turn);
	QPen pen(color, qMax<qreal>(1, pen_width));
	pen.setCapStyle(Qt::FlatCap);
	painter->setPen(pen);
	painter->setBrush(shape.m_filled ? QBrush(color) : Qt::NoBrush);
	const QRectF rect(-shape.m_width / 2, -shape.m_height / 2,
			  shape.m_width, shape.m_height);
	switch (shape.m_kind) {
		case ShapeKind::Circle:
			painter->drawEllipse(rect);
			break;
		case ShapeKind::Rectangle:
			painter->drawRect(rect);
			break;
		default:
			painter->drawLine(QPointF(-shape.m_width / 2, 0),
					  QPointF(shape.m_width / 2, 0));
			break;
	}
	painter->restore();
}

/**
	Draw one text around the point where the mark stands, at the place
	its m_alignment gives it. A text that does not stay upright turns
	with the drawing's @p turn, in degrees.
*/
void DefinitionMarkStyle::paintItem(QPainter *painter,
				    const TextItem &item,
				    const RenderContext &context,
				    const QString &text,
				    qreal turn)
{
	QFont font = context.font;
	if (item.m_size > 0)
		font.setPointSizeF(item.m_size);
	const QFontMetricsF metrics(font);
	const QRectF rect = textRect(item, metrics, text);

	painter->save();
	painter->translate(QTransform().rotate(turn).map(item.m_pos));
	painter->rotate(item.m_rotation + (item.m_upright ? 0 : turn));
	painter->setFont(font);
	painter->setPen(item.m_color.isValid() ? item.m_color : context.text_color);
	painter->drawText(rect, Qt::AlignCenter, text);
	painter->restore();
}

bool DefinitionMarkStyle::operator==(const DefinitionMarkStyle &other) const
{
	return m_enabled == other.m_enabled
		&& m_vertical_turn == other.m_vertical_turn
		&& m_shapes == other.m_shapes
		&& m_texts == other.m_texts;
}

bool DefinitionMarkStyle::operator!=(const DefinitionMarkStyle &other) const
{
	return !(*this == other);
}
