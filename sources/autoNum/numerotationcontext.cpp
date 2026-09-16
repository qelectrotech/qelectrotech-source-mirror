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
#include "numerotationcontext.h"

#include "../qet.h"

#include <QRegularExpression>
#include <utility>

/**
	Constructor
*/
NumerotationContext::NumerotationContext()
{
}

/**
	Constructor from xml
*/
NumerotationContext::NumerotationContext(QDomElement &e) {
	fromXml(e);
}

/**
	@brief NumerotationContext::clear, clear the content
*/
void NumerotationContext::clear ()
{
	content_.clear();
}

/**
	@brief NumerotationContext::addValue
	add a new value on the contexte
	@param type the type of value
	@param value the value itself
	@param increase the increase number of value
	@param initialvalue
	@param modulus wrap-and-carry modulus (0 means "not a wrapping part")
	@param format zero-padding mask, spreadsheet style: "00" pads to two
	digits, "000" to three. Empty keeps the part type's natural width, so
	an absent format reproduces exactly the behaviour of every context
	written before this field existed.
	@return true if value is append
*/
bool NumerotationContext::addValue(const QString &type,
				   const QVariant &value,
				   const int increase,
				   const int initialvalue,
				   const int modulus,
				   const QString &format) {
	if (!keyIsAcceptable(type) && !value.canConvert<QString>())
		return false;
	if (keyIsNumber(type) && !value.canConvert<int>())
		return false;

	QString valuestr = value.toString();
	valuestr.remove("|");
	content_ << type
		    + "|"
		    + valuestr
		    + "|"
		    + QString::number(increase)
		    + "|"
		    + QString::number(initialvalue)
		    + "|"
		    + QString::number(modulus)
		    + "|"
		    + QString(format).remove("|");
	return true;
}

/**
	@brief NumerotationContext::operator []
	@param i
	@return the string at position i
*/
QString NumerotationContext::operator [] (const int &i) const
{
	return (content_.at(i));
}

/**
	@brief NumerotationContext::operator << , append other
*/
void NumerotationContext::operator << (const NumerotationContext &other) {
	for (int i=0; i<other.size(); ++i) content_.append(other[i]);
}

/**
	@brief NumerotationContext::size
	@return size of context
*/
int NumerotationContext::size() const
{
	return (content_.size());
}

/**
	@brief NumerotationContext::isEmpty
	@return true if numerotation content is empty
*/
bool NumerotationContext::isEmpty() const
{
	if (content_.size() > 0) return false;
	return true;
}
/**
	@brief NumerotationContext::itemAt
	@param i
	@return the content at position i 1:type 2:value 3:increase
*/
QStringList NumerotationContext::itemAt(const int i) const
{
	return (content_.at(i).split("|"));
}

/**
	@brief validRegExpNum
	@return all type use to numerotation
*/
QString NumerotationContext::validRegExpNum () const
{
	return ("unit|unitfolio|ten|tenfolio|hundred|hundredfolio|wrap|alpha|string|idfolio|folio|plant|locmach|elementline|elementcolumn|elementprefix");
}

/**
	@brief NumerotationContext::validRegExpNumber
	@return all type represents a number
*/
QString NumerotationContext::validRegExpNumber() const
{
	return ("unit|unitfolio|ten|tenfolio|hundred|hundredfolio|wrap");
}

/**
	@brief NumerotationContext::keyIsAcceptable
	@return true if type is acceptable
*/
bool NumerotationContext::keyIsAcceptable(const QString &type) const
{
	return (type.contains(QRegularExpression(validRegExpNum())));
}

/**
	@brief NumerotationContext::keyIsNumber
	@return true if type represents a number
*/
bool NumerotationContext::keyIsNumber(const QString &type) const
{
	return (type.contains(QRegularExpression(validRegExpNumber())));
}

/**
	@brief NumerotationContext::toXml
	Save the numerotation context in a QDomElement under the element name str
*/
QDomElement NumerotationContext::toXml(QDomDocument &d, const QString& str) {
	QDomElement num_auto = d.createElement(str);
	for (int i=0; i<content_.size(); ++i) {
		QStringList strl = itemAt(i);
		QDomElement part = d.createElement("part");
		part.setAttribute("type", strl.at(0));
		part.setAttribute("value", strl.at(1));
		part.setAttribute("increase", strl.at(2));
		if (strl.at(0) == ("unitfolio") ||
			strl.at(0) == ("tenfolio")  ||
			strl.at(0) == ("hundredfolio")) {
			part.setAttribute("initialvalue", strl.at(3));
		}
		if (strl.at(0) == ("wrap") && strl.size() > 4) {
			part.setAttribute("modulus", strl.at(4));
		}
		if (strl.size() > 5 && !strl.at(5).isEmpty()) {
			part.setAttribute("format", strl.at(5));
		}
		num_auto.appendChild(part);
	}
	return num_auto;
}

/**
	@brief NumerotationContext::fromXml
	load numerotation context from e
*/
void NumerotationContext::fromXml(QDomElement &e) {
	clear();
	foreach(QDomElement qde, QET::findInDomElement(e, "part")) addValue(qde.attribute("type"), qde.attribute("value"), qde.attribute("increase").toInt(), qde.attribute("initialvalue").toInt(), qde.attribute("modulus").toInt(), qde.attribute("format"));
}

/**
	@brief NumerotationContext::replaceValue
	This class replaces the current NC field value with content
	@param index of NC Item
	@param content to replace current value
*/
void NumerotationContext::replaceValue(int index, QString content) {
	QStringList strl = content_[index].split("|");
	QString type = strl.at(0);
	const QString& value = std::move(content);
	QString increase = strl.at(2);
	QString initvalue = strl.at(3);
	QString modulus = strl.size() > 4 ? strl.at(4) : QStringLiteral("0");
	QString format  = strl.size() > 5 ? strl.at(5) : QString();
	content_[index] = type + "|" + value + "|" + increase + "|" + initvalue + "|" + modulus + "|" + format;
}

/**
	@brief NumerotationContext::replaceIncrease
	Change how much this part advances per step, leaving its current value,
	initial value, modulus and format untouched. Sibling to replaceValue(),
	which deliberately never touches this field.
	@param index of NC item
	@param increase new increase for that item
*/
void NumerotationContext::replaceIncrease(int index, int increase) {
	QStringList strl = content_[index].split("|");
	QString type = strl.at(0);
	QString value = strl.at(1);
	QString initvalue = strl.at(3);
	QString modulus = strl.size() > 4 ? strl.at(4) : QStringLiteral("0");
	QString format  = strl.size() > 5 ? strl.at(5) : QString();
	content_[index] = type + "|" + value + "|" + QString::number(increase) + "|" + initvalue + "|" + modulus + "|" + format;
}

/**
	@brief NumerotationContext::formatOf
	@param item : a context item as returned by itemAt()
	@return the part's zero-padding mask, or an empty string when it has
	none -- which every context written before the field existed will be.
*/
QString NumerotationContext::formatOf(const QStringList &item)
{
	return item.size() > 5 ? item.at(5) : QString();
}

/**
	@brief NumerotationContext::formatValue
	@param item : a context item as returned by itemAt()
	@return the part's value, zero-padded exactly as
	autonum::setSequentialToList() pads it when composing a real label: an
	explicit format mask wins, then "ten"/"hundred" parts get their
	implicit 2/3-digit width, "alpha" is used as-is, everything else is a
	plain number. Kept in step with that function by hand since the two
	cannot share code without exposing an assignvariables.cpp-local helper.
*/
QString NumerotationContext::formatValue(const QStringList &item)
{
	const QString &type = item.at(0);
	const QString &value = item.at(1);
	if (type == QLatin1String("alpha"))
		return value;

	const QString mask = formatOf(item);
	if (!mask.isEmpty())
		return QString("%1").arg(value.toInt(), mask.length(), 10, QChar('0'));
	if (type == QLatin1String("ten") || type == QLatin1String("tenfolio"))
		return QString("%1").arg(value.toInt(), 2, 10, QChar('0'));
	if (type == QLatin1String("hundred") || type == QLatin1String("hundredfolio"))
		return QString("%1").arg(value.toInt(), 3, 10, QChar('0'));
	return QString::number(value.toInt());
}
