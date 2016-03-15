/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef ELEMENTS_COLLECTION_CACHE_H
#define ELEMENTS_COLLECTION_CACHE_H

#include <QSqlDatabase>
#include "elementslocation.h"

class ElementsCollection;
class ElementsCategory;
class ElementDefinition;
/**
	This class implements a SQLite cache for data related to elements
	collections, mainly names and pixmaps. This avoids the cost of parsing XML
	definitions of elements and building full CustomElement objects when
	(re)loading the elements panel.
*/
class ElementsCollectionCache : public QObject
{
	public:
	// constructor, destructor
	ElementsCollectionCache(const QString &database_path, QObject * = 0);
	virtual ~ElementsCollectionCache();
	
	// methods
	public:
	void setLocale(const QString &);
	QString locale() const;
	bool setPixmapStorageFormat(const QString &);
	QString pixmapStorageFormat() const;
	void beginCollection(ElementsCollection *);
	void endCollection(ElementsCollection *);
	bool fetchElement(ElementDefinition *);
	bool fetchElement(ElementsLocation &location);
	QString name() const;
	QPixmap pixmap() const;
	bool fetchData(const ElementsLocation &);
	bool fetchNameFromCache(const QString &path, const QUuid &uuid);
	bool fetchPixmapFromCache(const QString &path, const QUuid &uuid);
	bool cacheName(const QString &path, const QUuid &uuid = QUuid::createUuid());
	bool cachePixmap(const QString &path, const QUuid &uuid = QUuid::createUuid());
	
	// attributes
	private:
	QSqlDatabase cache_db_;         ///< Object providing access to the SQLite database this cache relies on
	QSqlQuery *select_name_;        ///< Prepared statement to fetch names from the cache
	QSqlQuery *select_pixmap_;      ///< Prepared statement to fetch pixmaps from the cache
	QSqlQuery *insert_name_;        ///< Prepared statement to insert names into the cache
	QSqlQuery *insert_pixmap_;      ///< Prepared statement to insert pixmaps into the cache
	QString locale_;                ///< Locale to be used when dealing with names
	QString pixmap_storage_format_; ///< Storage format for cached pixmaps
	QString current_name_;          ///< Last name fetched
	QPixmap current_pixmap_;        ///< Last pixmap fetched
};
#endif
