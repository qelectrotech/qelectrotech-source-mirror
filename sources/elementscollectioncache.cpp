/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "elementscollectioncache.h"
#include "elementscollection.h"
#include "elementscategory.h"
#include "elementdefinition.h"
#include "factory/elementfactory.h"
#include "element.h"
#include <QImageWriter>

/**
	Construct a cache for elements collections.
	@param database_path Path of the SQLite database to open.
	@param parent Parent QObject
*/
ElementsCollectionCache::ElementsCollectionCache(const QString &database_path, QObject *parent) :
	QObject(parent),
	locale_("en"),
	pixmap_storage_format_("PNG")
{
	// initialize the cache SQLite database
	static int cache_instances = 0;
	QString connection_name = QString("ElementsCollectionCache-%1").arg(cache_instances++);
	cache_db_ = QSqlDatabase::addDatabase("QSQLITE", connection_name);
	cache_db_.setDatabaseName(database_path);
	if (!cache_db_.open()) {
		qDebug() << "Unable to open the SQLite database " << database_path << " as " << connection_name << ": " << cache_db_.lastError();
	} else {
		cache_db_.exec("PRAGMA temp_store = MEMORY");
		cache_db_.exec("PRAGMA journal_mode = MEMORY");
		cache_db_.exec("PRAGMA page_size = 4096");
		cache_db_.exec("PRAGMA cache_size = 16384");
		cache_db_.exec("PRAGMA locking_mode = EXCLUSIVE");
		cache_db_.exec("PRAGMA synchronous = OFF");
		/// @todo the tables could already exist, handle that case.
		cache_db_.exec("CREATE TABLE names (path VARCHAR(512) NOT NULL, locale VARCHAR(2) NOT NULL, mtime DATETIME NOT NULL, name VARCHAR(128), PRIMARY KEY(path, locale));");
		cache_db_.exec("CREATE TABLE pixmaps (path VARCHAR(512) NOT NULL UNIQUE, mtime DATETIME NOT NULL, pixmap BLOB, PRIMARY KEY(path), FOREIGN KEY(path) REFERENCES names (path) ON DELETE CASCADE);");
		
		// prepare queries
		select_name_   = new QSqlQuery(cache_db_);
		select_pixmap_ = new QSqlQuery(cache_db_);
		insert_name_   = new QSqlQuery(cache_db_);
		insert_pixmap_ = new QSqlQuery(cache_db_);
		select_name_   -> prepare("SELECT name FROM names WHERE path = :path AND locale = :locale AND mtime = :file_mtime");
		select_pixmap_ -> prepare("SELECT pixmap FROM pixmaps WHERE path = :path AND mtime = :file_mtime");
		insert_name_   -> prepare("REPLACE INTO names (path, locale, mtime, name) VALUES (:path, :locale, :mtime, :name)");
		insert_pixmap_ -> prepare("REPLACE INTO pixmaps (path, mtime, pixmap) VALUES (:path, :mtime, :pixmap)");
	}
}

/**
	Destructor
*/
ElementsCollectionCache::~ElementsCollectionCache() {
	delete select_name_;
	delete select_pixmap_;
	delete insert_name_;
	delete insert_pixmap_;
	cache_db_.close();
}

/**
	Define the locale to be used when dealing with names.
	@param locale New locale to be used.
*/
void ElementsCollectionCache::setLocale(const QString &locale) {
	locale_ = locale;
}

/**
	@return The locale to be used when dealing with names.
*/
QString ElementsCollectionCache::locale() const {
	return(locale_);
}

/**
	Define the storage format for the pixmaps within the SQLite database. See
	Qt's QPixmap documentation for more information.
	@param format The new pixmap storage format.
	@return True if the format change was accepted, false otherwise.
*/
bool ElementsCollectionCache::setPixmapStorageFormat(const QString &format) {
	if (QImageWriter::supportedImageFormats().contains(format.toAscii())) { 
		pixmap_storage_format_= format;
		return(true);
	}
	return(false);
}

/**
	@return the pixmap storage format. Default is "PNG"
	@see setPixmapStorageFormat()
*/
QString ElementsCollectionCache::pixmapStorageFormat() const {
	return(pixmap_storage_format_);
}

/**
	Indicate the cache a new collection is about to be browsed. This is mainly
	used to delimit database transactions.
	@param collection The elements collection about to be browsed.
*/
void ElementsCollectionCache::beginCollection(ElementsCollection *collection) {
	bool use_cache = cache_db_.isOpen() && collection -> isCacheable();
	if (use_cache) {
		bool transaction_started = cache_db_.transaction();
		qDebug() << (transaction_started ? "transaction began for " : "transaction not started for ") << collection -> protocol();
	}
}

/**
	Indicate the cache the currently browsed collection end has been reached. This
	is mainly used to delimit database transactions.
	@param collection The elements collection being browsed.
*/
void ElementsCollectionCache::endCollection(ElementsCollection *collection) {
	bool use_cache = cache_db_.isOpen() && collection -> isCacheable();
	if (use_cache) {
		bool transaction_commited = cache_db_.commit();
		if (transaction_commited) {
			qDebug() << "transaction commited for " << collection -> protocol();
		} else {
			qDebug() << "transaction not commited for " << collection -> protocol() << ":" << cache_db_.lastError();
		}
	}
}

/**
	Retrieve the data for a given element, using the cache if available,
	filling it otherwise. Data are then available through pixmap() and name()
	methods.
	@param element The definition of an element.
	@see pixmap()
	@see name()
	@return True if the retrieval succeeded, false otherwise.
*/
bool ElementsCollectionCache::fetchElement(ElementDefinition *element) {
	// can we use the cache with this element?
	bool use_cache = cache_db_.isOpen() && element -> parentCollection() -> isCacheable();
	
	// attempt to fetch the element name from the cache database
	if (!use_cache) {
		return(fetchData(element -> location()));
	} else {
		QString element_path = element -> location().toString();
		QDateTime mtime = element -> modificationTime();
		bool got_name   = fetchNameFromCache(element_path, mtime);
		bool got_pixmap = fetchPixmapFromCache(element_path, mtime);
		if (got_name && got_pixmap) {
			return(true);
		}
		if (fetchData(element -> location())) {
			cacheName(element_path, mtime);
			cachePixmap(element_path, mtime);
		}
		return(true);
	}
}

/**
	@return The last name fetched through fetchElement().
*/
QString ElementsCollectionCache::name() const {
	return(current_name_);
}

/**
	@return The last pixmap fetched through fetchElement().
*/
QPixmap ElementsCollectionCache::pixmap() const {
	return(current_pixmap_);
}

/**
	Retrieve the data by building the full CustomElement object matching the
	given location, without using the cache. Data are then available through
	pixmap() and name() methods.
	@param Location Location of a given Element.
	@return True if the retrieval succeeded, false otherwise.
*/
bool ElementsCollectionCache::fetchData(const ElementsLocation &location) {
	int state;
	Element *custom_elmt = ElementFactory::Instance() -> createElement(location, 0, &state);
	if (state) {
		qDebug() << "ElementsCollectionCache::fetchData() : Le chargement du composant" << qPrintable(location.toString()) << "a echoue avec le code d'erreur" << state;
	} else {
		current_name_   = custom_elmt -> name();
		current_pixmap_ = custom_elmt -> pixmap();
	}
	delete custom_elmt;
	return(!state);
}

/**
	Retrieve the name for an element, given its path and last modification
	time. The value is then available through the name() method.
	@param path Element path (as obtained using ElementsLocation::toString())
	@param file_mtime Date and time of last modification of this element. Any
	older cached value will be ignored.
	@return True if the retrieval succeeded, false otherwise.
*/
bool ElementsCollectionCache::fetchNameFromCache(const QString &path, const QDateTime &file_mtime) {
	select_name_ -> bindValue(":path", path);
	select_name_ -> bindValue(":locale", locale_);
	select_name_ -> bindValue(":file_mtime", file_mtime);
	if (select_name_ -> exec()) {
		if (select_name_ -> first()) {
			current_name_ = select_name_ -> value(0).toString();
			select_name_ -> finish();
			return(true);
		}
	} else {
		qDebug() << "select_name_->exec() failed";
	}
	return(false);
}

/**
	Retrieve the pixmap for an element, given its path and last modification
	time. It is then available through the pixmap() method.
	@param path Element path (as obtained using ElementsLocation::toString())
	@param file_mtime Date and time of last modification of this element. Any
	older cached pixmap will be ignored.
	@return True if the retrieval succeeded, false otherwise.
*/
bool ElementsCollectionCache::fetchPixmapFromCache(const QString &path, const QDateTime &file_mtime) {
	select_pixmap_ -> bindValue(":path", path);
	select_pixmap_ -> bindValue(":file_mtime", file_mtime);
	if (select_pixmap_ -> exec()) {
		if (select_pixmap_ -> first()) {
			QByteArray ba = select_pixmap_ -> value(0).toByteArray();
			// avoid returning always the same pixmap (i.e. same cacheKey())
			current_pixmap_.detach();
			current_pixmap_.loadFromData(ba, qPrintable(pixmap_storage_format_));
			select_pixmap_ -> finish();
		}
		return(true);
	} else {
		qDebug() << "select_pixmap_->exec() failed";
	}
	return(false);
}

/**
	Cache the current (i.e. last retrieved) name. The cache entry will use
	the current date and time and the locale set via setLocale().
	@param path Element path (as obtained using ElementsLocation::toString())
	@param mtime Modification time associated with the cache entry -- defaults to current datetime
	@return True if the caching succeeded, false otherwise.
	@see name()
*/
bool ElementsCollectionCache::cacheName(const QString &path, const QDateTime &mtime) {
	insert_name_ -> bindValue(":path",   path);
	insert_name_ -> bindValue(":locale", locale_);
	insert_name_ -> bindValue(":mtime",  QVariant(mtime));
	insert_name_ -> bindValue(":name",   current_name_);
	if (!insert_name_ -> exec()) {
		qDebug() << cache_db_.lastError();
		return(false);
	}
	return(true);
}

/**
	Cache the current (i.e. last retrieved) pixmap. The cache entry will use
	the current date and time.
	@param path Element path (as obtained using ElementsLocation::toString())
	@param mtime Modification time associated with the cache entry -- defaults to current datetime
	@return True if the caching succeeded, false otherwise.
	@see pixmap()
*/
bool ElementsCollectionCache::cachePixmap(const QString &path, const QDateTime &mtime) {
	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.open(QIODevice::WriteOnly);
	current_pixmap_.save(&buffer, qPrintable(pixmap_storage_format_));
	insert_pixmap_ -> bindValue(":path", path);
	insert_pixmap_ -> bindValue(":mtime", QVariant(mtime));
	insert_pixmap_ -> bindValue(":pixmap", QVariant(ba));
	if (!insert_pixmap_->exec()) {
		qDebug() << cache_db_.lastError();
		return(false);
	}
	return(true);
}
