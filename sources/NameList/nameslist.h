/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef NAMES_LIST_H
#define NAMES_LIST_H
#include "pugixml/src/pugixml.hpp"

#include <QtXml>
/**
	This class represents a list of names, used by elements and categories
	to embed the same name in several languages.
	Languages are represented by two or five letters (typically the first
	two of the system locale); examples: en for English, fr for French,
	pt_BR for Brazilian Portuguese.
	French:
	Cette classe represente une liste de noms, utilisee
	par les elements et categories pour embarquer un meme nom en plusieurs
	langues.
	Les langues sont representees par deux ou cinq lettres (typiquement : 
	les deux premieres de la locale du systeme) ; exemples : en pour l'anglais, 
	fr pour le francais, pt_BR pour le portugais brésilien. 
*/
class NamesList {
	// constructors, destructor
	public:
	NamesList();
	virtual ~NamesList();
	
	// attributes
	private:
	QMap<QString, QString> map_names;
	
	public:
	static int MetaTypeId;
	
	// methods
	public:
	// methods relatives a la gestion de la liste
	void addName(const QString &, const QString &);
	void removeName(const QString &);
	void clearNames();
	QList<QString> langs() const;
	bool isEmpty() const;
	int count() const;
	QString &operator[](const QString &);
	const QString operator[](const QString &) const;
	bool operator!=(const NamesList &) const;
	bool operator==(const NamesList &) const;
	QString name(const QString & = QString()) const;
	
	// methods relatives a XML
	void fromXml(const QDomElement &, const QHash<QString, QString> & = QHash<QString, QString>());
	void fromXml(const pugi::xml_node &xml_element, const QHash<QString, QString> &xml_options = QHash<QString, QString>());
	QDomElement toXml(QDomDocument &, const QHash<QString, QString> & = QHash<QString, QString>()) const;
	
	protected:
	QHash<QString, QString> getXmlOptions(const QHash<QString, QString> & = QHash<QString, QString>()) const;
};
Q_DECLARE_METATYPE(NamesList);
#endif
