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
#include "../qetxml.h"
#include "../ui/structureboxpropertieswidget.h"

#include <QDomDocument>
#include <QPainter>

StructureBoxItem::StructureBoxItem(QPointF p1, QPointF p2, QGraphicsItem *parent) :
	QetShapeItem(p1, p2, QetShapeItem::Rectangle, parent)
{
}

void StructureBoxItem::setPlant(const QString &plant)
{
	if (m_plant == plant) return;
	m_plant = plant;
	update();
}

void StructureBoxItem::setLocation(const QString &location)
{
	if (m_location == location) return;
	m_location = location;
	update();
}

void StructureBoxItem::setPrefix(const QString &prefix)
{
	if (m_prefix == prefix) return;
	m_prefix = prefix;
	update();
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

bool StructureBoxItem::fromXml(const QDomElement &e)
{
	if (e.tagName() != xmlTagName())
		return false;

	setPen(QETXML::penFromXml(e.firstChildElement("pen")));
	setBrush(QETXML::brushFromXml(e.firstChildElement("brush")));

	setRect(QRectF(
		QPointF(e.attribute("x1").toDouble(), e.attribute("y1").toDouble()),
		QPointF(e.attribute("x2").toDouble(), e.attribute("y2").toDouble())));

	m_plant = e.attribute("plant");
	m_location = e.attribute("location");
	m_prefix = e.attribute("prefix");

	setZValue(e.attribute("z", QString::number(this->zValue())).toDouble());

	return true;
}

QDomElement StructureBoxItem::toXml(QDomDocument &document) const
{
	QDomElement result = document.createElement(xmlTagName());

	result.appendChild(QETXML::penToXml(document, pen()));
	result.appendChild(QETXML::brushToXml(document, brush()));

	const QPointF p1 = mapToScene(line().p1());
	const QPointF p2 = mapToScene(line().p2());
	result.setAttribute("x1", QString::number(p1.x()));
	result.setAttribute("y1", QString::number(p1.y()));
	result.setAttribute("x2", QString::number(p2.x()));
	result.setAttribute("y2", QString::number(p2.y()));

	result.setAttribute("plant", m_plant);
	result.setAttribute("location", m_location);
	result.setAttribute("prefix", m_prefix);
	result.setAttribute("z", QString::number(this->zValue()));

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
	return tr("un cadre de repérage");
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
