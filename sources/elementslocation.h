/*
	Copyright 2006-2013 The QElectroTech Team
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
#ifndef ELEMENTS_LOCATION_H
#define ELEMENTS_LOCATION_H
#include <QtCore>
#include <QString>
class QETProject;
/**
	Cette classe represente la localisation, l'emplacement d'un element ou
	d'une categorie, voire d'une collection... dans une collection. Elle
	encapsule un chemin virtuel.
*/
class ElementsLocation {
	// constructors, destructor et operateur d'affectation
	public:
	ElementsLocation();
	explicit ElementsLocation(const QString &, QETProject * = 0);
	ElementsLocation(const ElementsLocation &);
	virtual ~ElementsLocation();
	ElementsLocation &operator=(const ElementsLocation &);
	bool operator==(const ElementsLocation &) const;
	bool operator!=(const ElementsLocation &) const;
	
	// methods
	public:
	QString baseName() const;
	QString path() const;
	void setPath(const QString &);
	bool addToPath(const QString &);
	ElementsLocation parent() const;
	QETProject *project() const;
	void setProject(QETProject *);
	bool isNull() const;
	QString toString() const;
	void fromString(const QString &);
	static ElementsLocation locationFromString(const QString &);
	
	// attributes
	private:
	QString path_;
	QETProject *project_;
	
	public:
	static int MetaTypeId; ///< Id of the corresponding Qt meta type
};
Q_DECLARE_METATYPE(ElementsLocation)
uint qHash(const ElementsLocation &);
#endif
