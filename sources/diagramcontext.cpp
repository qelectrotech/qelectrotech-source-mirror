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
QList<QString> DiagramContext::keys() const {
	return(content_.keys());
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
	@param key a key string
	@return true if that key is acceptable, false otherwise
*/
bool DiagramContext::keyIsAcceptable(const QString &key) const {
	static QRegExp re("^[a-z0-9-]+$");
	return(re.exactMatch(key));
}
