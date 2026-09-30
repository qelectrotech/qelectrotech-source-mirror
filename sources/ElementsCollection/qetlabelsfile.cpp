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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

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
	QFile file(filepath);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		return QString();
	}

	QDomDocument document;
	if (!document.setContent(&file)) {
		return QString();
	}

	return prefixInDocument(document, path, dirLevel);
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
		*reason = tr("erreur de syntaxe à la ligne %1, colonne %2 :\n%3")
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
		*reason = tr("erreur de syntaxe à la ligne %1, colonne %2 :\n%3")
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
	m_document = QDomDocument();

	if (collection_dir.isEmpty()) {
		m_error = tr("Aucun répertoire de collection n'a été donné.");
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
		if (well_formed
			&& document.documentElement().tagName() != QLatin1String("labels")) {
			well_formed = false;
			m_broken_reason = tr("l'élément racine <%1> n'est pas <labels>.")
					.arg(document.documentElement().tagName());
		}
		file.close();
	} else {
		m_broken_reason = tr("le fichier n'a pas pu être ouvert : %1")
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
				"Fichier de préfixes (étiquettes) de la collection utilisateur.\n"
				"Un préfixe est attribué à chaque dossier : les éléments d'un dossier\n"
				"reprennent le préfixe de ce dossier, sauf s'ils portent eux-mêmes une\n"
				"étiquette. Un dossier sans préfixe reprend celui de son dossier parent.\n"
				"Ce fichier est créé et modifié par QElectroTech (Programme de réglages :\n"
				"Configurer les préfixes…), mais reste modifiable à la main.");
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
		m_error = tr("Aucun fichier de préfixes à enregistrer.");
		return false;
	}

	const QString content = serialize(m_document);
	if (m_file_exists && !m_force_save && content == m_snapshot) {
		return true;
	}

	const QDir directory(QFileInfo(m_file_path).absolutePath());
	if (!directory.exists() && !QDir().mkpath(directory.absolutePath())) {
		m_error = tr("Le répertoire %1 n'a pas pu être créé.").arg(directory.absolutePath());
		return false;
	}

	if (m_force_save && m_file_exists) {
			//The file on disk could not be read : copy it aside before it
			//is replaced. Doing it here, and not when it is loaded, means
			//no copy is left behind when the user chooses to repair the
			//file instead of rebuilding it - or simply changes their mind
			//and cancels.
		m_backup_path = backupBrokenFile();
		if (m_backup_path.isEmpty()) {
			m_error = tr("Le fichier %1 n'a pas pu être copié à côté avant d'être remplacé :\nrien n'a été modifié.")
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
	m_snapshot = content;
	return true;
}
