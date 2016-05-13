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
#ifndef DIAGRAM_CONTEXT_H
#define DIAGRAM_CONTEXT_H
#include <QDomElement>
#include <QHash>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QStringList>
/**
	This class represents a diagram context, i.e. the data (a list of key/value
	pairs) of a diagram at a given time. It is notably used by titleblock templates
	to fetch the informations they need to do their rendering.
*/
class DiagramContext {
	public:
	enum KeyOrder {
		None,
		Alphabetical,
		DecreasingLength
	};

	QList<QString> keys(KeyOrder = None) const;
	bool contains(const QString &) const;
	const QVariant operator[](const QString &) const;
	bool addValue(const QString &, const QVariant &, bool show = true);
	void clear();
	int count();
	bool keyMustShow (const QString &) const;
	
	bool operator==(const DiagramContext &) const;
	bool operator!=(const DiagramContext &) const;
	
	void toXml(QDomElement &, const QString & = "property") const;
	void fromXml(const QDomElement &, const QString & = "property");
	void toSettings(QSettings &, const QString &) const;
	void fromSettings(QSettings &, const QString &);
	
	static QString validKeyRegExp();
	
	private:
	static bool stringLongerThan(const QString &, const QString &);
	bool keyIsAcceptable(const QString &) const;
	/// Diagram context data (key/value pairs)
	QHash<QString, QVariant> content_;
	QHash<QString, bool> content_show;
};
#endif
