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
#include "resetsymboltextscommand.h"

#include "../qetgraphicsitem/dynamicelementtextitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetxml.h"

#include <QObject>

namespace {

	/// What a text takes its text from, to match it with the symbol's
QString source(const QDomElement &text)
{
	const QString from = text.attribute(QStringLiteral("text_from"));
	if (from == QLatin1String("ElementInfo"))
		return from + QLatin1Char(':') + text.firstChildElement(QStringLiteral("info_name")).text();
	if (from == QLatin1String("CompositeText"))
		return from + QLatin1Char(':') + text.firstChildElement(QStringLiteral("composite_text")).text();
	return from;
}

} // namespace

ResetSymbolTextsCommand::ResetSymbolTextsCommand(const QList<Element *> &elements, QUndoCommand *parent) :
	QUndoCommand(QObject::tr("lay out the symbol texts as in the symbol"), parent)
{
	for (Element *element : elements)
	{
		const QDomElement definition = element->location().xml();
		if (definition.isNull())
			continue;
		QList<QDomElement> templates;
		for (QDomElement description = definition.firstChildElement(QStringLiteral("description")) ;
			 !description.isNull() ;
			 description = description.nextSiblingElement(QStringLiteral("description")))
			for (QDomElement t = description.firstChildElement(QStringLiteral("dynamic_text")) ;
				 !t.isNull() ; t = t.nextSiblingElement(QStringLiteral("dynamic_text")))
				templates << t;

		QList<DynamicElementTextItem *> texts = element->dynamicTextItems();
		for (const QDomElement &t : std::as_const(templates))
		{
			const QString wanted = source(t);
			for (int i = 0 ; i < texts.size() ; ++i)
			{
				DynamicElementTextItem *text = texts.at(i);
				const QDomElement before = text->toXml(m_document);
				if (source(before) != wanted)
					continue;
				texts.removeAt(i);

					//The symbol's layout, with this text's own id,
					//contents and colour
				QDomElement after = t.cloneNode(true).toElement();
				after.setTagName(DynamicElementTextItem::xmlTagName());
				after.setAttribute(QStringLiteral("uuid"), before.attribute(QStringLiteral("uuid")));
				for (const QString child : {QStringLiteral("text"), QStringLiteral("color")}) {
					QDomElement old_child = after.firstChildElement(child);
					if (!old_child.isNull())
						after.removeChild(old_child);
					const QDomElement own = before.firstChildElement(child);
					if (!own.isNull())
						after.appendChild(own.cloneNode(true));
				}
				if (after.attribute(QStringLiteral("x")) != before.attribute(QStringLiteral("x"))
					|| after.attribute(QStringLiteral("y")) != before.attribute(QStringLiteral("y"))
					|| after.attribute(QStringLiteral("font")) != before.attribute(QStringLiteral("font"))
					|| after.attribute(QStringLiteral("Halignment")) != before.attribute(QStringLiteral("Halignment"))
					|| after.attribute(QStringLiteral("Valignment")) != before.attribute(QStringLiteral("Valignment"))
					|| after.attribute(QStringLiteral("text_width")) != before.attribute(QStringLiteral("text_width"))
					|| after.attribute(QStringLiteral("rotation"), QStringLiteral("0")) != before.attribute(QStringLiteral("rotation"), QStringLiteral("0")))
					m_changes << Change{text, before, after};
				break;
			}
		}
	}
}

void ResetSymbolTextsCommand::undo()
{
	for (const Change &change : std::as_const(m_changes))
		if (change.text)
			change.text->fromXml(change.before);
}

void ResetSymbolTextsCommand::redo()
{
	for (const Change &change : std::as_const(m_changes))
		if (change.text)
			change.text->fromXml(change.after);
}
