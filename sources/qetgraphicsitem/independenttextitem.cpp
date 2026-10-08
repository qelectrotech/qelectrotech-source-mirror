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
#include "independenttextitem.h"
#include "../shownkinds.h"

#include "../diagram.h"
#include "../diagramcommands.h"
#include "../lastusedstyle.h"
#include "../qet.h"
#include "../qetapp.h"
#include "../utils/qetutils.h"

#include <QDomElement>
#include <QScopedPointer>
#include <QSettings>
#include <QtCore/qnumeric.h>

/**
	Constructeur
	@param parent_diagram Le schema auquel est rattache le champ de texte
*/
IndependentTextItem::IndependentTextItem() :
	DiagramTextItem(nullptr)
{
	ShownKinds::tag(this, ShownKinds::FreeTexts);
	wrapAtWords();
		//Start from the font last applied to a text item this session,
		//falling back to the app-wide Preferences default otherwise.
	setFont(LastUsedStyle::hasTextFont() ? LastUsedStyle::textFont()
					      : QETApp::indiTextsItemFont());
	QSettings settings;
	setRotation(settings.value("diagrameditor/independent_text_rotation", 0).toInt());
}

/**
	@brief IndependentTextItem::IndependentTextItem
	Constructeur
	@param text Le texte affiche par le champ de texte
*/
IndependentTextItem::IndependentTextItem(const QString &text) :
	DiagramTextItem(text, nullptr)
{
	ShownKinds::tag(this, ShownKinds::FreeTexts);
	wrapAtWords();
}

/**
	@brief IndependentTextItem::wrapAtWords
	A text with a width wraps between words only: a word longer than the
	width goes past it rather than being cut, as for the texts of symbols.
*/
void IndependentTextItem::wrapAtWords()
{
	QTextOption option = document()->defaultTextOption();
	option.setWrapMode(QTextOption::WordWrap);
	document()->setDefaultTextOption(option);
}

/// Destructeur
IndependentTextItem::~IndependentTextItem()
{
}

/**
	Permet de lire le texte a mettre dans le champ a partir d'un element XML.
	Cette methode se base sur la position du champ pour assigner ou non la
	valeur a ce champ.
	@param e L'element XML representant le champ de texte
*/
void IndependentTextItem::fromXml(const QDomElement &e) {
	const QUuid uuid(e.attribute(QStringLiteral("uuid")));
	if (!uuid.isNull() && uuid != m_uuid) setUuid(uuid);
	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setHtml(e.attribute("text"));
	setRotation(e.attribute("rotation").toDouble());
	if (e.hasAttribute("font"))
	{
		QFont font;
		QETUtils::fontFromString(font, e.attribute("font"));
		setFont(font);
	}
		//Optional: absent for a text with the automatic width, the only
		//kind older versions know (they show such a text unwrapped).
		//Read after the text, setHtml() sets a width of its own.
	setTextWidth(e.attribute(QStringLiteral("text_width"), QStringLiteral("-1")).toDouble());
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant ce champ de texte
*/
QDomElement IndependentTextItem::toXml(QDomDocument &document) const
{
	QDomElement result = document.createElement("input");
	result.setAttribute("uuid", m_uuid.toString());
	result.setAttribute("x", QString("%1").arg(pos().x()));
	result.setAttribute("y", QString("%1").arg(pos().y()));
	result.setAttribute("text", toHtml());
	result.setAttribute("rotation", QString::number(QET::correctAngle(rotation())));
	result.setAttribute("font", QETUtils::fontToString(font()));
		//Only when set, so a text with the automatic width is saved as before
	if (m_text_width > 0)
		result.setAttribute("text_width", QString::number(m_text_width));
	
	return(result);
}

/**
	@brief IndependentTextItem::setTextWidth
	Set the width of this text (-1 = automatic width): the text wraps to
	it, its top-left corner stays in place.
	@param width
*/
void IndependentTextItem::setTextWidth(qreal width)
{
	if (!qIsFinite(width) || width <= 0)
		width = -1;
	if (qFuzzyCompare(width, m_text_width))
		return;

	qreal document_width = width;
		//The automatic width of a text with centred or right-aligned
		//lines, as setHtml() gives it
	if (width < 0 && m_non_left_alignment)
	{
		QScopedPointer<QTextDocument> natural(document()->clone());
		natural->setTextWidth(-1);
		document_width = natural->idealWidth() + 40.0;
	}

	document()->setTextWidth(document_width);
	m_text_width = width;
	emit textWidthChanged(width);
}

/**
	@brief IndependentTextItem::textResizeHandlesWanted
	@return true when this text is selected and not being typed in: the
	corner handles then change its width.
*/
bool IndependentTextItem::textResizeHandlesWanted() const
{
	return isSelected() && !isEditing();
}

void IndependentTextItem::focusOutEvent(QFocusEvent *event)
{
	DiagramTextItem::focusOutEvent(event);
	if (diagram() && (m_previous_html_text != this->toHtml())) {
		diagram()->undoStack().push(new ChangeDiagramTextCommand(this, m_previous_html_text, this->toHtml()));
	}
}
