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
#include "itemdescription.h"

#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/diagramimageitem.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/elementtextitemgroup.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "qetgraphicsitem/terminal.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"

#include <QCoreApplication>

namespace {
		/// At most a few words of @a text, on one line
	QString shortText(const QString &text)
	{
		QString one_line = text.simplified();
		if (one_line.size() > 40)
			one_line = one_line.left(39) + QChar(0x2026);
		return one_line;
	}

	QString elementKind(const Element *element)
	{
		switch (element->elementData().m_type) {
			case ElementData::Master:              return QCoreApplication::translate("ItemDescription", "Master symbol");
			case ElementData::Slave:               return QCoreApplication::translate("ItemDescription", "Slave symbol");
			case ElementData::NextReport:
			case ElementData::PreviousReport:      return QCoreApplication::translate("ItemDescription", "Folio report");
			case ElementData::Terminal:            return QCoreApplication::translate("ItemDescription", "Terminal");
			case ElementData::Thumbnail:           return QCoreApplication::translate("ItemDescription", "Thumbnail");
			case ElementData::ConductorDefinition: return QCoreApplication::translate("ItemDescription", "Conductor definition");
			default:                               return QCoreApplication::translate("ItemDescription", "Symbol");
		}
	}

		/// "-Q1 · circuit breaker", or the name alone without a label
	QString elementName(const Element *element)
	{
		const QString label = element->actualLabel();
		const QString name = element->name();
		if (label.isEmpty())
			return name;
		return name.isEmpty() ? label : QStringLiteral("%1 · %2").arg(label, name);
	}

	QString shapeKind(const QetShapeItem *shape)
	{
		switch (shape->shapeType()) {
			case QetShapeItem::Line:      return QCoreApplication::translate("ItemDescription", "Line (shape)");
			case QetShapeItem::Rectangle: return QCoreApplication::translate("ItemDescription", "Rectangle (shape)");
			case QetShapeItem::Ellipse:   return QCoreApplication::translate("ItemDescription", "Ellipse (shape)");
			case QetShapeItem::Polygon:   return QCoreApplication::translate("ItemDescription", "Polygon (shape)");
			case QetShapeItem::Path:      return QCoreApplication::translate("ItemDescription", "Path (shape)");
			default:                      return QCoreApplication::translate("ItemDescription", "Shape");
		}
	}
}

/**
	@brief ItemDescription::of
	@return what @a item is, in a few words, or nothing for what is no
	item of its own (the folio, a handle)
*/
QString ItemDescription::of(const QGraphicsItem *item)
{
	if (!item)
		return QString();

	switch (item->type())
	{
		case Element::Type: {
			auto element = static_cast<const Element *>(item);
			return QStringLiteral("%1: %2").arg(elementKind(element), elementName(element));
		}
		case DynamicElementTextItem::Type: {
			auto text = static_cast<const DynamicElementTextItem *>(item);
			const QString content = shortText(text->toPlainText());
			Element *element = text->parentElement();
			if (!element)
				return QCoreApplication::translate("ItemDescription", "Symbol text: %1").arg(content);
			return QCoreApplication::translate("ItemDescription", "Symbol text of %1: %2").arg(elementName(element), content);
		}
		case IndependentTextItem::Type:
			return QCoreApplication::translate("ItemDescription", "Free text: %1").arg(
					shortText(static_cast<const IndependentTextItem *>(item)->toPlainText()));
		case ConductorTextItem::Type:
			return QCoreApplication::translate("ItemDescription", "Conductor text: %1").arg(
					shortText(static_cast<const ConductorTextItem *>(item)->toPlainText()));
		case Conductor::Type: {
			auto conductor = static_cast<const Conductor *>(item);
			const QString text = shortText(conductor->properties().text);
			return text.isEmpty() ? QCoreApplication::translate("ItemDescription", "Conductor") : QCoreApplication::translate("ItemDescription", "Conductor %1").arg(text);
		}
		case Terminal::Type:
			return of(item->parentItem());
		case QetShapeItem::Type:
			return shapeKind(static_cast<const QetShapeItem *>(item));
		case DiagramImageItem::Type:
			return QCoreApplication::translate("ItemDescription", "Picture");
		case QetGraphicsTableItem::Type:
			return QCoreApplication::translate("ItemDescription", "Table");
		default:
			break;
	}

	if (auto group = dynamic_cast<const ElementTextItemGroup *>(item)) {
		Element *element = group->parentElement();
		return element ? QCoreApplication::translate("ItemDescription", "Text group %1 of %2").arg(group->name(), elementName(element))
			       : QCoreApplication::translate("ItemDescription", "Text group %1").arg(group->name());
	}
		//A part of something, a handle: say what it belongs to
	return item->parentItem() ? of(item->parentItem()) : QString();
}

/**
	@brief ItemDescription::ofSelection
	@return what @a items are: the item itself when there is one, or
	how many there are
*/
QString ItemDescription::ofSelection(const QList<QGraphicsItem *> &items)
{
	if (items.isEmpty())
		return QString();
	if (items.size() == 1)
		return of(items.first());
	return QCoreApplication::translate("ItemDescription", "%n items selected", nullptr, int(items.size()));
}
