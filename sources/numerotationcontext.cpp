/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "qet.h"

/**
 * Constructor
 */
NumerotationContext::NumerotationContext(){
}

/**
 * Constructor from xml
 */
NumerotationContext::NumerotationContext(QDomElement &e) {
	fromXml(e);
}

/**
 * @brief NumerotationContext::clear, clear the content
 */
void NumerotationContext::clear () {
	content_.clear();
}

/**
 * @brief NumerotationContext::addValue, add a new value on the contexte
 * @param type the type of value
 * @param value the value itself
 * @param increase the increase number of value
 * @return true if value is append
 */
bool NumerotationContext::addValue(const QString &type, const QVariant &value, const int increase, const int initialvalue) {
	if (!keyIsAcceptable(type) && !value.canConvert(QVariant::String)) return false;
	if (keyIsNumber(type) && !value.canConvert(QVariant::Int)) return false;

	QString valuestr = value.toString();
	valuestr.remove("|");
	content_ << type + "|" + valuestr + "|" + QString::number(increase) + "|" + QString::number(initialvalue);
	return true;
}

/**
 * @brief NumerotationContext::operator []
 * @return the string at position @i
 */
QString NumerotationContext::operator [] (const int &i) const {
	return (content_.at(i));
}

/**
 * @brief NumerotationContext::operator << , append other
 */
void NumerotationContext::operator << (const NumerotationContext &other) {
	for (int i=0; i<other.size(); ++i) content_.append(other[i]);
}

/**
 * @brief NumerotationContext::size
 * @return size of context
 */
int NumerotationContext::size() const {
	return (content_.size());
}

/**
 * @brief NumerotationContext::isEmpty
 * @return true if numerotation contet is empty
 */
bool NumerotationContext::isEmpty() const {
	if (content_.size() > 0) return false;
	return true;
}
/**
 * @brief NumerotationContext::itemAt
 * @return the content at position @i 1:type 2:value 3:increase
 */
QStringList NumerotationContext::itemAt(const int i) const {
	return (content_.at(i).split("|"));
}

/**
 * @brief validRegExpNum
 * @return all type use to numerotation
 */
QString NumerotationContext::validRegExpNum () const {
	return ("unit|unitfolio|ten|hundred|string|idfolio|folio|elementline|elementcolumn|elementprefix");
}

/**
 * @brief NumerotationContext::validRegExpNumber
 * @return all type represents a number
 */
QString NumerotationContext::validRegExpNumber() const {
	return ("unit|unitfolio|ten|hundred");
}

/**
 * @brief NumerotationContext::keyIsAcceptable
 * @return true if @type is acceptable
 */
bool NumerotationContext::keyIsAcceptable(const QString &type) const {
	return (type.contains(QRegExp(validRegExpNum())));
}

/**
 * @brief NumerotationContext::keyIsNumber
 * @return true if @type represent a number
 */
bool NumerotationContext::keyIsNumber(const QString &type) const {
	return (type.contains(QRegExp(validRegExpNumber())));
}

/**
 * @brief NumerotationContext::toXml
 * Save the numerotation context in a QDomElement under the element name @str
 */
QDomElement NumerotationContext::toXml(QDomDocument &d, QString str) {
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
		num_auto.appendChild(part);
	}
	return num_auto;
}

/**
 * @brief NumerotationContext::fromXml
 * load numerotation context from @e
 */
void NumerotationContext::fromXml(QDomElement &e) {
	clear();
	foreach(QDomElement qde, QET::findInDomElement(e, "part")) addValue(qde.attribute("type"), qde.attribute("value"), qde.attribute("increase").toInt(), qde.attribute("initialvalue").toInt());
}

/**
 * @brief NumerotationContext::replaceValue
 * This class replaces the current NC field value with content
 * @param index of NC Item
 * @param QString content to replace current value
 */
void NumerotationContext::replaceValue(int index, QString content) {
	QString sep = "|";
	QString type = content_[index].split("|").at(0);
	QString value = content;
	QString increase = content_[index].split("|").at(2);
	QString initvalue = content_[index].split("|").at(3);
	content_[index].replace(content_[index], type + "|" + value + "|" + increase + "|" + initvalue);
}
