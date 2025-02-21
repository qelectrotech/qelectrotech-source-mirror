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
#include "elementscollectioncache.h"

#include "factory/elementfactory.h"
#include "qet.h"
#include "qetgraphicsitem/element.h"

#include <QImageWriter>
#include <QSqlError>
#include <QSqlQuery>

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

	if (!cache_db_.open())
		qDebug() << "Unable to open the SQLite database " << database_path << " as " << connection_name << ": " << cache_db_.lastError();
	else
	{
		QSqlQuery *temp_query = new QSqlQuery(cache_db_);
		temp_query->exec("PRAGMA temp_store = MEMORY");
		temp_query->exec("PRAGMA journal_mode = MEMORY");
		temp_query->exec("PRAGMA page_size = 4096");
		temp_query->exec("PRAGMA cache_size = 16384");
		temp_query->exec("PRAGMA locking_mode = EXCLUSIVE");
		temp_query->exec("PRAGMA synchronous = OFF");

#if TODO_LIST
#pragma message("@TODO the tables could already exist, handle that case.")
#endif
			//@TODO the tables could already exist, handle that case.
		temp_query->exec("CREATE TABLE names"
					   "("
					   "path VARCHAR(512) NOT NULL,"
					   "locale VARCHAR(2) NOT NULL,"
					   "uuid VARCHAR(512) NOT NULL,"
					   "name VARCHAR(128),"
					   "PRIMARY KEY(path, locale)"
					   ");");

		temp_query->exec("CREATE TABLE pixmaps"
					   "("
					   "path VARCHAR(512) NOT NULL UNIQUE,"
					   "uuid VARCHAR(512) NOT NULL,"
					   "pixmap BLOB, PRIMARY KEY(path),"
					   "FOREIGN KEY(path) REFERENCES names (path) ON DELETE CASCADE);");
		delete temp_query;

			// prepare queries
		select_name_   = new QSqlQuery(cache_db_);
		select_pixmap_ = new QSqlQuery(cache_db_);
		insert_name_   = new QSqlQuery(cache_db_);
		insert_pixmap_ = new QSqlQuery(cache_db_);
		select_name_   -> prepare("SELECT name FROM names WHERE path = :path AND locale = :locale AND uuid = :uuid");
		select_pixmap_ -> prepare("SELECT pixmap FROM pixmaps WHERE path = :path AND uuid = :uuid");
		insert_name_   -> prepare("REPLACE INTO names (path, locale, uuid, name) VALUES (:path, :locale, :uuid, :name)");
		insert_pixmap_ -> prepare("REPLACE INTO pixmaps (path, uuid, pixmap) VALUES (:path, :uuid, :pixmap)");
	}
}

/**
	Destructor
*/
ElementsCollectionCache::~ElementsCollectionCache()
{
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
QString ElementsCollectionCache::locale() const
{
	return(locale_);
}

/**
	Define the storage format for the pixmaps within the SQLite database. See
	Qt's QPixmap documentation for more information.
	@param format The new pixmap storage format.
	@return True if the format change was accepted, false otherwise.
*/
bool ElementsCollectionCache::setPixmapStorageFormat(const QString &format) {
	if (QImageWriter::supportedImageFormats().contains(format.toLatin1())) {
		pixmap_storage_format_= format;
		return(true);
	}
	return(false);
}

/**
	@return the pixmap storage format. Default is "PNG"
	@see setPixmapStorageFormat()
*/
QString ElementsCollectionCache::pixmapStorageFormat() const
{
	return(pixmap_storage_format_);
}

/**
	@brief ElementsCollectionCache::fetchElement
	Retrieve the data for a given element, using the cache if available,
	filling it otherwise. Data are then available through pixmap() and name() methods.
	@param location The definition of an element.
	@see pixmap()
	@see name()
	@return True if the retrieval succeeded, false otherwise.
*/
bool ElementsCollectionCache::fetchElement(ElementsLocation &location)
{
		// can we use the cache with this element?
	bool use_cache = cache_db_.isOpen() && !location.isProject();

		// attempt to fetch the element name from the cache database
	if (!use_cache) {
		return(fetchData(location));
	}
	else
	{
		auto uuid = location.uuid();
		QString element_path = location.toString();
		bool got_name   = fetchNameFromCache(element_path, uuid);
		bool got_pixmap = fetchPixmapFromCache(element_path, uuid);

		if (got_name && got_pixmap) {
			return(true);
		}

		if (fetchData(location))
		{
			cacheName(element_path, uuid);
			cachePixmap(element_path, uuid);
		}
		return(true);
	}
}

/**
	@return The last name fetched through fetchElement().
*/
QString ElementsCollectionCache::name() const
{
	return(current_name_);
}

/**
	@return The last pixmap fetched through fetchElement().
*/
QPixmap ElementsCollectionCache::pixmap() const
{
	return(current_pixmap_);
}

/**
	Retrieve the data by building the full CustomElement object matching the
	given location, without using the cache. Data are then available through
	pixmap() and name() methods.
	@param location Location of a given Element.
	@return True if the retrieval succeeded, false otherwise.
*/
bool ElementsCollectionCache::fetchData(const ElementsLocation &location) {
	int state;
	Element *custom_elmt = ElementFactory::Instance() -> createElement(location, nullptr, &state);
	if (state) {
		qDebug() << "ElementsCollectionCache::fetchData() : Le chargement du composant"
			 << qPrintable(location.toString())
			 << "a echoue avec le code d'erreur"
			 << state;
	} else {
		current_name_   = custom_elmt -> name();
		current_pixmap_ = custom_elmt -> pixmap();
	}
	delete custom_elmt;
	return(!state);
}

/**
	@brief ElementsCollectionCache::fetchNameFromCache
	Retrieve the name for an element, given its path and uuid
	The value is then available through the name() method.
	@param path : Element path (as obtained using ElementsLocation::toString())
	@param uuid : Element uuid
	@return True if the retrieval succeeded, false otherwise.
*/
bool ElementsCollectionCache::fetchNameFromCache(const QString &path,
						 const QUuid &uuid)
{
	select_name_ -> bindValue(":path", path);
	select_name_ -> bindValue(":locale", locale_);
	select_name_ -> bindValue(":uuid", uuid.toString());
	if (select_name_ -> exec())
	{
		if (select_name_ -> first())
		{
			current_name_ = select_name_ -> value(0).toString();
			select_name_ -> finish();
			return(true);
		}
	}
	else
		qDebug() << "select_name_->exec() failed";

	return(false);
}

/**
	@brief ElementsCollectionCache::fetchPixmapFromCache
	Retrieve the pixmap for an element, given its path and uuid.
	It is then available through the pixmap() method.
	@param path : Element path (as obtained using ElementsLocation::toString())
	@param uuid : Element uuid
	@return True if the retrieval succeeded, false otherwise.
*/
bool ElementsCollectionCache::fetchPixmapFromCache(const QString &path,
						   const QUuid &uuid)
{
	select_pixmap_ -> bindValue(":path", path);
	select_pixmap_ -> bindValue(":uuid", uuid.toString());
	if (select_pixmap_ -> exec())
	{
		if (select_pixmap_ -> first())
		{
			QByteArray ba = select_pixmap_ -> value(0).toByteArray();
			// avoid returning always the same pixmap (i.e. same cacheKey())
			current_pixmap_.detach();
			current_pixmap_.loadFromData(ba, qPrintable(pixmap_storage_format_));
			select_pixmap_ -> finish();
		}
		return(true);
	}
	else
		qDebug() << "select_pixmap_->exec() failed";

	return(false);
}

/**
	@brief ElementsCollectionCache::cacheName
	Cache the current (i.e. last retrieved) name The cache entry will use the locale set via setLocale().
	@param path : Element path (as obtained using ElementsLocation::toString())
	@param uuid :Element uuid
	@return True if the caching succeeded, false otherwise.
	@see name()
*/
bool ElementsCollectionCache::cacheName(const QString &path,
					const QUuid &uuid)
{
	insert_name_ -> bindValue(":path",   path);
	insert_name_ -> bindValue(":locale", locale_);
	insert_name_ -> bindValue(":uuid",  uuid.toString());
	insert_name_ -> bindValue(":name",   current_name_);
	if (!insert_name_ -> exec())
	{
		qDebug() << cache_db_.lastError();
		return(false);
	}
	return(true);
}

/**
	@brief ElementsCollectionCache::cachePixmap
	Cache the current (i.e. last retrieved) pixmap
	@param path : Element path (as obtained using ElementsLocation::toString())
	@param uuid : Element uuid
	@return True if the caching succeeded, false otherwise.
	@see pixmap()
*/
bool ElementsCollectionCache::cachePixmap(const QString &path,
					  const QUuid &uuid)
{
	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.open(QIODevice::WriteOnly);
	current_pixmap_.save(&buffer, qPrintable(pixmap_storage_format_));
	insert_pixmap_ -> bindValue(":path", path);
	insert_pixmap_ -> bindValue(":uuid", uuid.toString());
	insert_pixmap_ -> bindValue(":pixmap", QVariant(ba));
	if (!insert_pixmap_->exec())
	{
		qDebug() << cache_db_.lastError();
		return(false);
	}
	return(true);
}
