/*
	Copyright 2006-2012 Xavier Guerrin
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
#include <QRegExp>

/**
	@return a list containing all the keys in the context object.
*/
QList<QString> DiagramContext::keys(DiagramContext::KeyOrder order) const {
	if (order == None) {
		return content_.keys();
	} else {
		QList<QString> keys_list = content_.keys();
		if (order == Alphabetical) {
			qSort(keys_list);
		} else {
			qSort(keys_list.begin(), keys_list.end(), DiagramContext::stringLongerThan);
		}
		return(keys_list);
	}
}

/**
	@param key string key
	@return true if that key is known to the diagram context, false otherwise
*/
bool DiagramContext::contains(const QString &key) const {
	return(content_.contains(key));
}

/**
	@param key
*/
const QVariant DiagramContext::operator[](const QString &key) const {
	return(content_[key]);
}

/**
	@param key key to insert in the context - the key may only contain lowercase
	letters and dashes
	@see DiagramContext::keyIsAcceptable()
	@param value value to insert in the context
	@return true if the insertion succeeds, false otherwise
*/
bool DiagramContext::addValue(const QString &key, const QVariant &value) {
	if (keyIsAcceptable(key)) {
		content_.insert(key, value);
		return(true);
	}
	return(false);
}

bool DiagramContext::operator==(const DiagramContext &dc) const {
	return(content_ == dc.content_);
}

bool DiagramContext::operator!=(const DiagramContext &dc) const {
	return(!(*this == dc));
}

/**
	@return the regular expression used to check whether a given key is acceptable.
	@see keyIsAcceptable()
*/
QString DiagramContext::validKeyRegExp() {
	return("^[a-z0-9-]+$");
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
bool DiagramContext::keyIsAcceptable(const QString &key) const {
	static QRegExp re(DiagramContext::validKeyRegExp());
	return(re.exactMatch(key));
}
