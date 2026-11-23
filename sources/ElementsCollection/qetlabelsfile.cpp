/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "qetlabelsfile.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSaveFile>
#include <QSet>

#include <algorithm>
#include <utility>

/**
	@brief QetLabelsFile::labelsFilePath
	@param collection_dir : an elements collection directory
	@return the path of the qet_labels.xml file of that collection
*/
QString QetLabelsFile::labelsFilePath(const QString &collection_dir)
{
	return QDir(collection_dir).filePath(QStringLiteral("qet_labels.xml"));
}

/**
	@brief QetLabelsFile::scanFolders
	@param collection_dir : an elements collection directory
	@return every directory of the collection, each entry being the list of
		the directory names from the collection root down to the directory
		itself (["level_one", "level_two", ...]), in tree order : a parent
		always comes before its children, siblings are sorted by name.
	Only plain, visible directories are reported : symbolic links are
	skipped (a link pointing back up the tree would never end) as are
	hidden directories such as a stray .git.
*/
QList<QStringList> QetLabelsFile::scanFolders(const QString &collection_dir)
{
	QList<QStringList> out;
	if (collection_dir.isEmpty()) {
		return out;
	}
	scanRec(collection_dir, QStringList(), out);
	return out;
}

/**
	@brief QetLabelsFile::scanRec
	Recursive helper of scanFolders()
*/
void QetLabelsFile::scanRec(const QString &path, const QStringList &relative_path, QList<QStringList> &out)
{
	const QFileInfoList sub_dirs = QDir(path).entryInfoList(
				QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks,
				QDir::Name);
	for (const QFileInfo &info : sub_dirs) {
		QStringList current = relative_path;
		current << info.fileName();
		out.append(current);
		scanRec(info.absoluteFilePath(), current, out);
	}
}

/**
	@brief QetLabelsFile::prefixForPath
	Look up a prefix for @a path (path[dirLevel] outermost, path[1] the
	deepest directory; path[0], the element's own file name, is never
	matched) in the qet_labels.xml at @a filepath.

	@return the prefix that applies, or a null QString if the file
		cannot be read, is not well-formed, or does not describe this
		path at all (as opposed to describing it with no prefix
		anywhere along it, which is a non-null empty string).

	@see prefixInDocument() for the description of the lookup itself
*/
QString QetLabelsFile::prefixForPath(const QString &filepath, const QStringList &path, int dirLevel)
{
		//Asked once for every symbol of a project that is loaded, always of
		//the same few files: each is read and parsed again only when its
		//size or its time of last change is not what it was.
	struct Parsed {
		qint64 size = -1;
		QDateTime modified;
		QDomDocument document;
	};
	static QMutex mutex;
	static QHash<QString, Parsed> parsed;

	const QFileInfo info(filepath);
	QMutexLocker locker(&mutex);
	if (!info.isFile()) {
		parsed.remove(filepath);
		return QString();
	}
	Parsed &known = parsed[filepath];
	if (known.size != info.size() || known.modified != info.lastModified()) {
		known = Parsed();
		QFile file(filepath);
		if (!file.open(QFile::ReadOnly | QFile::Text)) {
			parsed.remove(filepath);
			return QString();
		}
		QDomDocument document;
		if (!document.setContent(&file)) {
			parsed.remove(filepath);
			return QString();
		}
		known.size = info.size();
		known.modified = info.lastModified();
		known.document = document;
	}
	return prefixInDocument(known.document, path, dirLevel);
}

/**
	@brief QetLabelsFile::prefixInDocument
	Look up a prefix for @a path (path[dirLevel] outermost, path[1] the
	deepest directory; path[0], the element's own file name, is never
	matched) in @a document.

	Descends through nested \<category name="..."\> elements matching
	path[dirLevel], path[dirLevel-1], ..., path[1] in turn, considering
	only *direct* children at each step -- unlike a flat token scan,
	this cannot be fooled by a same-named category living elsewhere in
	the document at the wrong nesting depth (bugtracker #671 item 5).

	At each matched level, that category's own \<prefix\> child -- even
	an empty one -- overrides whatever a shallower ancestor already
	provided, so an explicit empty \<prefix/\> cancels inheritance
	rather than silently falling back to it (the behaviour requested in
	PR #686 review). A category with no \<prefix\> child at all leaves
	the inherited value untouched, which is how a directory with no
	prefix of its own comes to inherit its parent's, as the file's own
	header comment documents.

	@return the prefix that applies, or a null QString if the document
		does not describe this path at all (as opposed to describing it
		with no prefix anywhere along it, which is a non-null empty
		string).
*/
QString QetLabelsFile::prefixInDocument(const QDomDocument &document, const QStringList &path, int dirLevel)
{
	QDomElement node = document.documentElement();
	if (node.isNull()) {
		return QString();
	}

	QString prefix;
	for (int i = dirLevel ; i >= 1 ; --i) {
		QDomElement child = node.firstChildElement(QStringLiteral("category"));
		while (!child.isNull()
			   && child.attribute(QStringLiteral("name")) != path[i]) {
			child = child.nextSiblingElement(QStringLiteral("category"));
		}
		if (child.isNull()) {
			return QString();
		}
		node = child;

		const QDomElement own = node.firstChildElement(QStringLiteral("prefix"));
		if (!own.isNull()) {
				//readElementText()'s null-vs-empty distinction that PR
				//#686 needed for the old QXmlStreamReader-based lookup
				//has a QDomElement equivalent: text() on an empty
				//element can itself come back null depending on how the
				//XML was written, so the same explicit fallback applies
				//-- an empty QString here means "found, deliberately
				//blank", not "not found".
			prefix = own.text();
			if (prefix.isNull()) {
				prefix = QString("");
			}
		}
	}
	return prefix;
}

/**
	@brief QetLabelsFile::prefixForFolder
	The prefix the qet_labels.xml at @a filepath gives the symbols of the
	collection folder @a relative_path (as returned by scanFolders()),
	trying the two layouts the prefix lookup of a user or company file
	tries, in the same order: the whole path from the top-level folder,
	then the path without that top-level folder
	(@see autonum::elementPrefixForLocation()).

	Used to tell, in the prefix dialog of the company collection, which
	folders the user collection gives a prefix too: the user file is read
	first, so its prefix is the one the symbols get.

	@return the prefix, possibly an explicit empty one, or a null QString
		when the file does not describe that folder
*/
QString QetLabelsFile::prefixForFolder(const QString &filepath, const QStringList &relative_path)
{
		//prefixForPath() wants the deepest folder at index 1 and the
		//outermost last, index 0 being the symbol file name it never reads
	auto lookupPath = [](const QStringList &folders) {
		QStringList path {QString()};
		for (auto it = folders.crbegin() ; it != folders.crend() ; ++it) {
			path << *it;
		}
		return path;
	};

	QList<QStringList> layouts {relative_path};
	if (relative_path.size() > 1) {
		layouts << relative_path.mid(1);
	}
	for (const QStringList &folders : std::as_const(layouts)) {
		const QStringList path = lookupPath(folders);
		const QString prefix = prefixForPath(filepath, path, path.size() - 1);
		if (!prefix.isNull()) {
			return prefix;
		}
	}
	return QString();
}

/**
	@brief QetLabelsFile::parse
	Read @a device into @a document and, when it cannot be read as XML,
	tell what is wrong with it in @a reason.
	The Qt version split is the same one used by edzpart.cpp and
	cli_export.cpp : QDomDocument::ParseResult only exists since Qt 6.5.
	@return true when the document is well formed.
*/
bool QetLabelsFile::parse(QIODevice &device, QDomDocument &document, QString *reason)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	const auto result = document.setContent(&device);
	if (result) {
		return true;
	}
	if (reason != nullptr) {
		*reason = tr("syntax error at line %1, column %2:\n%3")
				.arg(result.errorLine)
				.arg(result.errorColumn)
				.arg(result.errorMessage);
	}
	return false;
#else
	QString message;
	int line = 0;
	int column = 0;
	if (document.setContent(&device, &message, &line, &column)) {
		return true;
	}
	if (reason != nullptr) {
		*reason = tr("syntax error at line %1, column %2:\n%3")
				.arg(line)
				.arg(column)
				.arg(message);
	}
	return false;
#endif
}

/**
	@brief QetLabelsFile::load
	Read the qet_labels.xml of the collection @a collection_dir.
	Nothing is written : 
	- when the file does not exist yet, an empty \<labels\> document is
	  kept in memory and will only be created on the first save()
	- when the folders of a \<labels\> file leave out their top-level
	  folder, or the file has another root element, it is read the way
	  the prefix lookup reads it, if every folder keeps its prefix that
	  way : wasConverted() then tells save() to keep a copy of the file
	- when the file exists but cannot be read, is not well formed or does
	  not have \<labels\> as its root element, nothing is written : an
	  empty document is used instead, and a copy of the unusable file is
	  only made by save(), right before it is replaced. isBroken() and
	  brokenReason() tell what is wrong with it, so the caller can offer
	  repairing the file rather than silently discarding its content.
	@return false only when no collection directory was given. It is
		save() that reports a file it could not copy aside.
*/
bool QetLabelsFile::load(const QString &collection_dir)
{
	m_file_path.clear();
	m_backup_path.clear();
	m_error.clear();
	m_broken_reason.clear();
	m_snapshot.clear();
	m_file_exists = false;
	m_force_save = false;
	m_broken = false;
	m_converted = false;
	m_document = QDomDocument();

	if (collection_dir.isEmpty()) {
		m_error = tr("No collection folder was given.");
		return false;
	}
	m_file_path = labelsFilePath(collection_dir);

	QFile file(m_file_path);
	if (!file.exists()) {
		createEmptyDocument();
		m_snapshot = serialize(m_document);
		return true;
	}
	m_file_exists = true;

	QDomDocument document;
	bool well_formed = false;
	if (file.open(QFile::ReadOnly | QFile::Text)) {
		well_formed = parse(file, document, &m_broken_reason);
		const QString root_tag = document.documentElement().tagName();
		if (well_formed && root_tag != QLatin1String("labels")) {
				//A file written by hand often has its top-level folder,
				//<category name="devices">, as outer element (#1802).
				//The prefix lookup reads it all the same : when the
				//same file inside <labels> gives every folder the same
				//prefix, use that, and keep a copy when it is saved.
			QDomDocument converted;
			if (convertForeignRoot(document, scanFolders(collection_dir),
								   converted)) {
				document = converted;
				m_converted = true;
			} else {
				well_formed = false;
				m_broken_reason = tr("the root element <%1> is not <labels>.")
						.arg(root_tag);
			}
		} else if (well_formed) {
				//The shipped collections write each folder without its
				//top-level folder. The prefix lookup reads a user file
				//written that way too : copy those folders under their
				//top-level folder, where the dialog reads them.
			QDomDocument converted;
			if (placeShortPaths(document, scanFolders(collection_dir),
								converted)) {
				document = converted;
				m_converted = true;
			}
		}
		file.close();
	} else {
		m_broken_reason = tr("the file could not be opened: %1")
				.arg(file.errorString());
	}

	if (well_formed) {
		m_document = document;
	} else {
			//The file is unreadable or malformed : leave it alone and work
			//on an empty document. m_force_save makes sure the file is
			//replaced as soon as the caller validates, even though the
			//empty document we start from serializes just fine - and it is
			//also what tells save() to copy the file aside first.
		m_broken = true;
		createEmptyDocument();
		m_force_save = true;
	}
	m_snapshot = serialize(m_document);
	return true;
}

/**
	@brief QetLabelsFile::prefixForFolderInDocument
	Same as prefixForFolder(), on @a document instead of a file.
*/
QString QetLabelsFile::prefixForFolderInDocument(
		const QDomDocument &document, const QStringList &relative_path)
{
	QList<QStringList> layouts {relative_path};
	if (relative_path.size() > 1) {
		layouts << relative_path.mid(1);
	}
	for (const QStringList &folders : std::as_const(layouts)) {
		QStringList path {QString()};
		for (auto it = folders.crbegin() ; it != folders.crend() ; ++it) {
			path << *it;
		}
		const QString prefix =
				prefixInDocument(document, path, path.size() - 1);
		if (!prefix.isNull()) {
			return prefix;
		}
	}
	return QString();
}

/**
	@brief QetLabelsFile::convertForeignRoot
	Write in @a converted the file @a document, whose root element is not
	\<labels\>, the way the prefix dialog reads a file: a \<labels\> root
	whose children are the top-level folders of the collection.

	The prefix lookup never reads the root element itself, neither its
	name nor its own \<prefix\>, and it tries each child of the root
	both as a top-level folder and as a folder inside any top-level
	folder. So every child of the root is placed under each top-level
	folder that has a folder of that name, or used as that top-level
	folder when it is named after one. The others are also kept under
	\<labels\> : the lookup reads them for the other collections too.

	@param folders : the folders of the collection, from scanFolders()
	@return true when every folder of @a folders gets the same prefix
		from @a converted as from @a document. A folder the document
		does not describe and one it describes with no prefix count as
		the same : the dialog describes every folder when it saves.
*/
bool QetLabelsFile::convertForeignRoot(const QDomDocument &document,
									   const QList<QStringList> &folders,
									   QDomDocument &converted)
{
	const QDomElement root = document.documentElement();
	converted = QDomDocument();
	QDomElement labels;
	for (QDomNode node = document.firstChild() ; !node.isNull() ;
		 node = node.nextSibling()) {
		if (node == root) {
			labels = converted.createElement(QStringLiteral("labels"));
			converted.appendChild(labels);
		} else {
			converted.appendChild(converted.importNode(node, true));
		}
	}

	QSet<QString> existing;
	for (const QStringList &folder : folders) {
		existing.insert(folder.join(QLatin1Char('/')));
	}

	for (const QStringList &folder : folders) {
		if (folder.size() != 1) {
			continue;
		}
		const QString top = folder.first();
		QDomElement top_category = directChildCategory(root, top);
		top_category = top_category.isNull()
				? converted.createElement(QStringLiteral("category"))
				: converted.importNode(top_category, true).toElement();
		top_category.setAttribute(QStringLiteral("name"), top);

		for (QDomElement child = root.firstChildElement(QStringLiteral("category")) ;
			 !child.isNull() ;
			 child = child.nextSiblingElement(QStringLiteral("category"))) {
			const QString name = child.attribute(QStringLiteral("name"));
			if (name == top
				|| !existing.contains(top + QLatin1Char('/') + name)
				|| !directChildCategory(top_category, name).isNull()) {
				continue;
			}
			top_category.appendChild(converted.importNode(child, true));
		}
		if (top_category.hasChildNodes()) {
			labels.appendChild(top_category);
		}
	}
		//The lookup also reads the other children for the symbols of the
		//other collections, which folders does not list
	QStringList tops;
	for (const QStringList &folder : folders) {
		if (folder.size() == 1) {
			tops << folder.first();
		}
	}
	for (QDomElement child = root.firstChildElement(QStringLiteral("category")) ;
		 !child.isNull() ;
		 child = child.nextSiblingElement(QStringLiteral("category"))) {
		if (!tops.contains(child.attribute(QStringLiteral("name")))) {
			labels.appendChild(converted.importNode(child, true));
		}
	}

	return keepsEveryPrefix(document, converted, folders, false);
}

/**
	@brief QetLabelsFile::placeShortPaths
	Write in @a converted the \<labels\> file @a document with a copy of
	every child of its root that is named after a folder inside a
	top-level folder, rather than after a top-level folder, under each
	top-level folder that has a folder of that name and does not
	describe it yet : the place the dialog reads it from. The prefix
	lookup reads such a child both ways (@see prefixForFolder()).

	The child itself stays where it is : the lookup also reads it for
	the symbols of the other collections, which @a folders does not
	list. orphanPaths() does not report it.

	@param folders : the folders of the collection, from scanFolders()
	@return true when at least one copy was made and every folder of
		@a folders gets exactly the same prefix from @a converted as from
		@a document, an explicit empty one included.
*/
bool QetLabelsFile::placeShortPaths(const QDomDocument &document,
									const QList<QStringList> &folders,
									QDomDocument &converted)
{
	converted = document.cloneNode(true).toDocument();
	QDomElement root = converted.documentElement();

	QStringList tops;
	QSet<QString> existing;
	for (const QStringList &folder : folders) {
		existing.insert(folder.join(QLatin1Char('/')));
		if (folder.size() == 1) {
			tops << folder.first();
		}
	}

	bool copied = false;
	for (QDomElement child = root.firstChildElement(QStringLiteral("category")) ;
		 !child.isNull() ;
		 child = child.nextSiblingElement(QStringLiteral("category"))) {
		const QString name = child.attribute(QStringLiteral("name"));
			//A child named after a top-level folder is also read as a
			//folder of that name inside the other top-level folders
		for (const QString &top : std::as_const(tops)) {
			if (top == name
				|| !existing.contains(top + QLatin1Char('/') + name)) {
				continue;
			}
			QDomElement top_category = directChildCategory(root, top);
			if (top_category.isNull()) {
				top_category = converted.createElement(QStringLiteral("category"));
				top_category.setAttribute(QStringLiteral("name"), top);
				root.appendChild(top_category);
			} else if (!directChildCategory(top_category, name).isNull()) {
				continue;
			}
			top_category.appendChild(child.cloneNode(true));
			copied = true;
		}
	}

	return copied && keepsEveryPrefix(document, converted, folders, true);
}

/**
	@brief QetLabelsFile::keepsEveryPrefix
	@return true when every folder of @a folders gets the same prefix from
		@a after as from @a before. Unless @a strict, a folder not
		described and one described with no prefix count as the same.
		The dialog adds every folder before it saves (@see
		ensureStructure()), and a folder added with no prefix inherits
		its parent's instead of being read in the other layout : a
		folder that has a prefix in @a before must keep it then too.
*/
bool QetLabelsFile::keepsEveryPrefix(const QDomDocument &before,
									 const QDomDocument &after,
									 const QList<QStringList> &folders,
									 bool strict)
{
	QetLabelsFile saved;
	saved.m_document = after.cloneNode(true).toDocument();
	saved.ensureStructure(folders);

	for (const QStringList &folder : folders) {
		const QString old_prefix = prefixForFolderInDocument(before, folder);
		const QString new_prefix = prefixForFolderInDocument(after, folder);
		if (strict ? (old_prefix.isNull() != new_prefix.isNull()
					  || old_prefix != new_prefix)
				   : old_prefix.isEmpty() != new_prefix.isEmpty()
					 || (!old_prefix.isEmpty() && old_prefix != new_prefix)) {
			return false;
		}
		if (!old_prefix.isEmpty()
			&& prefixForFolderInDocument(saved.m_document, folder)
			   != old_prefix) {
			return false;
		}
	}
	return true;
}

/**
	@brief QetLabelsFile::createEmptyDocument
	Reset the in memory document to an empty \<labels\> document, preceded
	by the comment explaining how the prefixes are meant to be written.
*/
void QetLabelsFile::createEmptyDocument()
{
	m_document = QDomDocument();
	m_document.appendChild(m_document.createProcessingInstruction(
							   QStringLiteral("xml"),
							   QStringLiteral("version=\"1.0\" encoding=\"utf-8\"")));

	const QString comment = tr(
				"Prefix (label) file of the user collection.\n"
				"Each folder is given a prefix: the symbols of a folder take that\n"
				"folder's prefix, unless they carry a label of their own. A folder\n"
				"without a prefix takes its parent folder's.\n"
				"QElectroTech creates and changes this file (Settings:\n"
				"Configure prefixes…), but it can still be edited by hand.");
	m_document.appendChild(m_document.createComment(comment));

	m_document.appendChild(m_document.createElement(QStringLiteral("labels")));
}

/**
	@brief QetLabelsFile::backupBrokenFile
	Keep a copy of the unreadable file before it gets overwritten by the
	next save().
	@return the path of the created backup, or an empty QString if the
		copy failed.
*/
QString QetLabelsFile::backupBrokenFile()
{
	QString candidate = m_file_path + QStringLiteral(".bak");
	int counter = 1;
	while (QFile::exists(candidate) && counter < 1000) {
		candidate = m_file_path + QStringLiteral(".bak.") + QString::number(counter++);
	}
	if (!QFile::copy(m_file_path, candidate)) {
		return QString();
	}
	return candidate;
}

/**
	@brief QetLabelsFile::prefix
	@param relative_path : a directory of the collection, from the
		collection root down to the directory itself
	@return the prefix stored for that very directory, an empty QString
		when it has no prefix of its own -- which means it inherits the
		prefix of its parent directory, and is shown with an empty field
		by the editor. The inherited value is never reported here: a
		field the user leaves empty must keep inheriting.
*/
QString QetLabelsFile::prefix(const QStringList &relative_path) const
{
	QDomElement node = m_document.documentElement();
	for (const QString &name : relative_path) {
		node = directChildCategory(node, name);
		if (node.isNull()) {
			return QString();
		}
	}

	const QDomElement own = node.firstChildElement(QStringLiteral("prefix"));
	if (own.isNull()) {
		return QString();
	}
	const QString value = own.text();
	return value.isNull() ? QString() : value;
}

/**
	@brief QetLabelsFile::hasPrefix
	@return true when @a relative_path owns a \<prefix\> child of its own,
		even an empty one. prefix() alone cannot tell that apart from a
		category without any \<prefix\> : both give it no value, while an
		empty \<prefix/\> cancels the inheritance where a category
		without one inherits.
*/
bool QetLabelsFile::hasPrefix(const QStringList &relative_path) const
{
	QDomElement node = m_document.documentElement();
	for (const QString &name : relative_path) {
		node = directChildCategory(node, name);
		if (node.isNull()) {
			return false;
		}
	}

	return !node.firstChildElement(QStringLiteral("prefix")).isNull();
}

/**
	@brief QetLabelsFile::directChildCategory
	@return the \<category\> child of @a parent named @a name, or a null
		element when there is none. Only direct children are considered,
		the same way the prefix lookup descends the document.
*/
QDomElement QetLabelsFile::directChildCategory(const QDomElement &parent, const QString &name)
{
	QDomElement child = parent.firstChildElement(QStringLiteral("category"));
	while (!child.isNull()
		   && child.attribute(QStringLiteral("name")) != name) {
		child = child.nextSiblingElement(QStringLiteral("category"));
	}
	return child;
}

/**
	@brief QetLabelsFile::categoryForPath
	@param relative_path : a directory of the collection, from the
		collection root down to the directory itself
	@param create : when true, the missing \<category\> elements are
		created on the way down
	@return the \<category\> element describing @a relative_path, or a
		null element when it does not exist and @a create is false.
*/
QDomElement QetLabelsFile::categoryForPath(const QStringList &relative_path, bool create)
{
	QDomElement node = m_document.documentElement();
	for (const QString &name : relative_path) {
		if (node.isNull()) {
			break;
		}
		QDomElement child = directChildCategory(node, name);
		if (child.isNull()) {
			if (!create) {
				return QDomElement();
			}
			child = m_document.createElement(QStringLiteral("category"));
			child.setAttribute(QStringLiteral("name"), name);
				//A prefix always comes after the child categories, as
				//documented in the header of the shipped labels files :
				//insert the new category before the prefix, then push the
				//prefix back to the end when the file did not follow that
				//convention in the first place. A node nothing is inserted
				//into is never repositioned, so a hand formatted file stays
				//byte for byte the same as long as it is not modified.
			QDomElement prefix = node.firstChildElement(QStringLiteral("prefix"));
			if (prefix.isNull()) {
				node.appendChild(child);
			} else {
				node.insertBefore(child, prefix);
				if (!prefix.nextSibling().isNull()) {
					node.appendChild(prefix);
				}
			}
		}
		node = child;
	}
	return node;
}

/**
	@brief QetLabelsFile::ensureStructure
	Create, in memory, one \<category\> element for every directory of the
	collection, nesting them the way the directories are nested.
	Existing elements -- categories, prefixes and comments alike -- are
	left where and how they are: only missing categories are added, and
	those get no \<prefix\> child, which is exactly "inherit the parent's
	prefix".
	@param folders : as returned by scanFolders()
*/
void QetLabelsFile::ensureStructure(const QList<QStringList> &folders)
{
	for (const QStringList &folder : folders) {
		categoryForPath(folder, true);
	}
}

/**
	@brief QetLabelsFile::setPrefix
	Store @a prefix as the prefix of the directory @a relative_path,
	creating the missing categories on the way.
	An empty @a prefix means "that directory has no prefix of its own" :
	any \<prefix\> element it had is removed so the lookup falls back to
	the prefix of the parent directory.
*/
void QetLabelsFile::setPrefix(const QStringList &relative_path, const QString &prefix)
{
	if (relative_path.isEmpty()) {
		return;
	}
	QDomElement node = categoryForPath(relative_path, true);
	if (node.isNull()) {
		return;
	}

	QDomElement element = node.firstChildElement(QStringLiteral("prefix"));
	if (prefix.isEmpty()) {
		if (!element.isNull()) {
			node.removeChild(element);
		}
		return;
	}

	if (element.isNull()) {
		element = m_document.createElement(QStringLiteral("prefix"));
		node.appendChild(element);
	}
	while (!element.firstChild().isNull()) {
		element.removeChild(element.firstChild());
	}
	element.appendChild(m_document.createTextNode(prefix));
		//keep the convention : the prefix stays after the child categories
	if (!element.nextSibling().isNull()) {
		node.appendChild(element);
	}
}

/**
	@brief QetLabelsFile::collectOrphans
	Recursive helper shared by orphanPaths() and removeOrphans().
	Appends to @a orphans the top most \<category\> elements whose path is
	not in @a folders_key -- their children go away with them.
*/
void QetLabelsFile::collectOrphans(const QDomElement &parent, const QString &relative_path, const QSet<QString> &folders_key, QList<QDomElement> &orphans)
{
	for (QDomElement child = parent.firstChildElement(QStringLiteral("category"));
		 !child.isNull();
		 child = child.nextSiblingElement(QStringLiteral("category"))) {
		const QString path = relative_path.isEmpty()
				? child.attribute(QStringLiteral("name"))
				: relative_path + QLatin1Char('/') + child.attribute(QStringLiteral("name"));
		if (folders_key.contains(path)) {
			collectOrphans(child, path, folders_key, orphans);
		} else {
			orphans.append(child);
		}
	}
}

/**
	@brief QetLabelsFile::orphanPaths
	@param folders : as returned by scanFolders()
	@return the paths of the \<category\> elements of the document that do
		not describe any existing directory of the collection, e.g. a
		directory the user deleted or renamed, or entries written by hand
		in another layout. The editor asks the user what to do with them.
*/
QStringList QetLabelsFile::orphanPaths(const QList<QStringList> &folders) const
{
	QSet<QString> folders_key;
	for (const QStringList &folder : folders) {
		folders_key.insert(folder.join(QLatin1Char('/')));
	}

	QList<QDomElement> orphans;
	const QDomElement root = m_document.documentElement();
	if (!root.isNull()) {
		collectOrphans(root, QString(), folders_key, orphans);
	}
		//A child of the root named after a folder inside a top-level
		//folder is read by the prefix lookup, for the symbols of every
		//collection (@see placeShortPaths()) : it is not an orphan
	QSet<QString> inner_names;
	for (const QStringList &folder : folders) {
		if (folder.size() == 2) {
			inner_names.insert(folder.last());
		}
	}
	orphans.erase(std::remove_if(orphans.begin(), orphans.end(),
								 [&](const QDomElement &element) {
		return element.parentNode() == root
				&& inner_names.contains(element.attribute(QStringLiteral("name")));
	}), orphans.end());

	QStringList out;
	for (const QDomElement &element : orphans) {
		QStringList names;
		QDomElement node = element;
		while (!node.isNull() && node.tagName() != QLatin1String("labels")) {
			names.prepend(node.attribute(QStringLiteral("name")));
			node = node.parentNode().toElement();
		}
		out.append(names.join(QLatin1Char('/')));
	}
	return out;
}

/**
	@brief QetLabelsFile::removeOrphans
	Remove from the document every category orphanPaths() reports.
	@param folders : as returned by scanFolders()
	@return the number of removed categories (children included)
*/
int QetLabelsFile::removeOrphans(const QList<QStringList> &folders)
{
	QStringList orphans = orphanPaths(folders);
	if (orphans.isEmpty()) {
		return 0;
	}

	int removed = 0;
	for (const QString &path : orphans) {
		QDomElement node = m_document.documentElement();
		const QStringList names = path.split(QLatin1Char('/'));
		for (const QString &name : names) {
			node = directChildCategory(node, name);
			if (node.isNull()) {
				break;
			}
		}
		if (node.isNull()) {
			continue;
		}
		node.parentNode().removeChild(node);
		++removed;
	}
	return removed;
}

/**
	@brief QetLabelsFile::serialize
	@return @a document as the text written to the disk : a canonical xml
		declaration followed by a two space per level indentation, the
		same as the shipped labels files.
*/
QString QetLabelsFile::serialize(const QDomDocument &document)
{
	QString content = document.toString(2);

		//QDomDocument may or may not emit the xml declaration itself,
		//depending on how the document was built, so drop it to always
		//write the very same header. "<?xml " (with the trailing space)
		//tells the real declaration from any <?xml-stylesheet ...?>.
	const int declaration_end = content.indexOf(QLatin1String("?>"));
	if (content.startsWith(QLatin1String("<?xml ")) && declaration_end > 0) {
		content.remove(0, declaration_end + 2);
	}
	while (content.startsWith(QLatin1Char('\n'))) {
		content.remove(0, 1);
	}

	return QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n") + content;
}

/**
	@brief QetLabelsFile::save
	Write the document back to its qet_labels.xml.
	The file is only rewritten when the document actually differs from
	what was loaded, so a file the user formatted by hand is left alone
	when nothing was changed. When the file does not exist yet it is of
	course always created.
	@return true when there was nothing to write or the file was
		successfully written; otherwise false and errorString() tells why.
*/
bool QetLabelsFile::save()
{
	m_error.clear();

	if (m_file_path.isEmpty()) {
		m_error = tr("No prefix file to save.");
		return false;
	}

	const QString content = serialize(m_document);
	if (m_file_exists && !m_force_save && content == m_snapshot) {
		return true;
	}

	const QDir directory(QFileInfo(m_file_path).absolutePath());
	if (!directory.exists() && !QDir().mkpath(directory.absolutePath())) {
		m_error = tr("The folder %1 could not be created.").arg(directory.absolutePath());
		return false;
	}

	if ((m_force_save || m_converted) && m_file_exists) {
			//The file on disk could not be read, or had no <labels>
			//around it : copy it aside before it is replaced. Doing it
			//here, and not when it is loaded, means no copy is left
			//behind when the user chooses to repair the file instead of
			//rebuilding it - or simply changes their mind and cancels.
		m_backup_path = backupBrokenFile();
		if (m_backup_path.isEmpty()) {
			m_error = tr("The file %1 could not be copied beside itself before being replaced:\nnothing has been changed.")
					.arg(m_file_path);
			return false;
		}
	}

	QSaveFile file(m_file_path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		m_error = file.errorString();
		return false;
	}
	if (file.write(content.toUtf8()) < 0) {
		m_error = file.errorString();
		file.cancelWriting();
		return false;
	}
	if (!file.commit()) {
		m_error = file.errorString();
		return false;
	}

	m_file_exists = true;
	m_force_save = false;
	m_converted = false;
	m_snapshot = content;
	return true;
}
