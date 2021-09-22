/*
	Copyright 2006-2021 The QElectroTech Team
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
#include "diagramcontext.h"
#include "qet.h"
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

/**
	@brief DiagramContext::add
	Add all value of other to this.
	If a key already exist, the value is replaced.
	If a key doesn't exist, she will be added.
	All other keys of this context, which are not present in other, stay unchanged.
	@param other
*/
void DiagramContext::add(DiagramContext other)
{
	for (QString key : other.keys()) {
		addValue(key, other.value(key));
	}
}

/**
	@brief DiagramContext::remove
	@param key
*/
void DiagramContext::remove(const QString &key) {
	m_content.remove(key);
}

/**
	@return a list containing all the keys in the context object.
*/
QList<QString> DiagramContext::keys(DiagramContext::KeyOrder order) const
{
	if (order == None) {
		return m_content.keys();
	}
	else
	{
		QList<QString> keys_list = m_content.keys();
		if (order == Alphabetical) {
			std::sort(keys_list.begin(), keys_list.end());
		} else {
			std::sort(keys_list.begin(), keys_list.end(), DiagramContext::stringLongerThan);
		}
		return(keys_list);
	}
}

/**
	@param key string key
	@return true if that key is known to the diagram context, false otherwise
*/
bool DiagramContext::contains(const QString &key) const
{
	return(m_content.contains(key));
}

/**
	@param key
*/
const QVariant DiagramContext::operator[](const QString &key) const
{
	return(m_content[key]);
}

/**
	@param key key to insert in the context - the key may only contain lowercase
	letters and dashes.
	If embedded key is set, key must be find it else value is not added.
	@see DiagramContext::keyIsAcceptable()
	@param value value to insert in the context
	@param show if value is used to be show on the diagram or somewhere else,
	we can specify if he is show(true) or not(false)
	@return true if the insertion succeeds, false otherwise
*/
bool DiagramContext::addValue(const QString &key, const QVariant &value, bool show) {
	if (isKeyAcceptable(key)) {
		m_content.insert(key, value);
		m_content_show.insert(key, show);
		return(true);
		}
	return(false);
}

QVariant DiagramContext::value(const QString &key) const
{
	return m_content.value(key);
}

/**
	Clear the content of this diagram context.
*/
void DiagramContext::clear()
{
	m_content.clear();
	m_content_show.clear();
}

/**
	@return the number of key/value pairs stored in this object.
*/
int DiagramContext::count()
{
	return(m_content.count());
}

/**
	@brief DiagramContext::keyMustShow
	@return the value pairs with key, if key no found, return false
*/
bool DiagramContext::keyMustShow(const QString &key) const
{
	if (m_content_show.contains(key))
		return m_content_show[key];
	return false;
}

bool DiagramContext::operator==(const DiagramContext &dc) const
{
	return(m_content     == dc.m_content &&
		   m_content_show == dc.m_content_show);
}

bool DiagramContext::operator!=(const DiagramContext &dc) const
{
	return(!(*this == dc));
}

/**
	Export this context properties under the \a e XML element, using tags
	named \a tag_name (defaults to "property").
*/
void DiagramContext::toXml(QDomElement &e, const QString &tag_name) const
{
	foreach (QString key, keys()) {
		QDomElement property = e.ownerDocument().createElement(tag_name);
		property.setAttribute("name", key);
		property.setAttribute("show",m_content_show[key]);
		QDomText value = e.ownerDocument().createTextNode(m_content[key].toString());
		property.appendChild(value);
		e.appendChild(property);
	}
}

/**
	Read this context properties from the \a e XML element, looking for tags named
	\a tag_name (defaults to "property").
*/
void DiagramContext::fromXml(const QDomElement &e, const QString &tag_name) {
	foreach (QDomElement property, QET::findInDomElement(e, tag_name)) {
		if (!property.hasAttribute("name")) continue;
		addValue(property.attribute("name"), QVariant(property.text()));
		m_content_show.insert(property.attribute("name"), property.attribute("show", "1").toInt());
	}
}

/**
	@brief DiagramContext::fromXml
	Read this context properties from the dom_element,
	looking for tags named tag_name
	@param dom_element : dom element to parse
	@param tag_name : tag name to find, by default "property"
*/
void DiagramContext::fromXml(const pugi::xml_node &dom_element, const QString &tag_name)
{
	for(auto node = dom_element.child(tag_name.toStdString().c_str()) ; node ; node = node.next_sibling(tag_name.toStdString().c_str()))
	{
		addValue(node.attribute("name").as_string(), QVariant(node.text().as_string()));
		m_content_show.insert(node.attribute("name").as_string(), node.attribute("show").empty()? 1 : node.attribute("show").as_int());
	}
}

/**
	Export this context properties to \a settings by creating an array named \a
	array_name.
*/
void DiagramContext::toSettings(QSettings &settings, const QString &array_name) const
{
	settings.beginWriteArray(array_name);
	int i = 0;
	foreach (QString key, m_content.keys()) {
		settings.setArrayIndex(i);
		settings.setValue("name", key);
		settings.setValue("value", m_content[key].toString());
		++ i;
	}
	settings.endArray();
}

/**
	Read this context properties from \a settings by running through the array
	named \a array_name.
*/
void DiagramContext::fromSettings(QSettings &settings, const QString &array_name) {
	int size = settings.beginReadArray(array_name);
	for (int i = 0 ; i < size; ++ i) {
		settings.setArrayIndex(i);
		QString key = settings.value("name").toString();
		if (key.isEmpty()) continue;
		addValue(key, settings.value("value").toString());
	}
	settings.endArray();
}

/**
	@return the regular expression used to check whether a given key is acceptable.
	@see keyIsAcceptable()
*/
QString DiagramContext::validKeyRegExp()
{
	return("^[a-z0-9-_]+$");
}

/**
	@return True if \a a is longer than \a b, false otherwise.
*/
bool DiagramContext::stringLongerThan(const QString &a, const QString &b) {
	return (a.length() > b.length());
}

/**
	@param key a key string
	@return true if that key is acceptable, false otherwise
*/
bool DiagramContext::isKeyAcceptable(const QString &key)
{
	static const QRegularExpression re(DiagramContext::validKeyRegExp());

	QRegularExpressionMatch match = re.match(key);
	return match.hasMatch();
}

QDebug operator <<(QDebug debug, const DiagramContext &context)
{
	debug << "DiagramContext";
	for (auto key : context.keys()) {
		debug.nospace() << key << " : " << context.value(key) << "\n";
	}
	debug << " end DiagramContext";

	return debug;
}
