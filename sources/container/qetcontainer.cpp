/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "qetcontainer.h"
#include "qetcontainerdb.h"

#include <QCryptographicHash>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <QUuid>

const char QetContainer::MimeType[] = "application/x-qet-project+zip";
const char QetContainer::OldMimeType[] = "application/x-qelectrotech-project+zip";

namespace {
const QString PartTag = QStringLiteral("qetz-part");
const QString FileAttribute = QStringLiteral("file");
const QString ImageFileAttribute = QStringLiteral("qetz-file");

void setError(QString *error, const QString &text)
{
	if (error) *error = text;
}

	//An element on its own, written without adding indentation: the
	//whitespace text already in the document (QETProject keeps the symbol
	//definitions as they were read, spacing included) must come back as it
	//was, or the next save writes it differently. Parts are one line.
QByteArray serialize(const QDomNode &node)
{
	QDomDocument document;
	document.appendChild(document.createProcessingInstruction(
							 QStringLiteral("xml"),
							 QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));
	document.appendChild(document.importNode(node, true));
	return document.toString(-1).toUtf8();
}

	//A file name from a name chosen by the user: no folders, nothing a zip
	//tool or a file system would read another way.
QString safeSegment(const QString &name, const QString &fallback)
{
	QString out;
	for (const QChar c : name) {
		out += (c.isLetterOrNumber() || c == QLatin1Char('-') || c == QLatin1Char('_')
				|| c == QLatin1Char('.') || c == QLatin1Char(' '))
				? c : QLatin1Char('_');
	}
	out = out.trimmed();
	while (out.startsWith(QLatin1Char('.'))) out.remove(0, 1);
	return out.isEmpty() ? fallback : out;
}

	//@p name, or name~2, name~3... if already taken
QString unique(const QString &name, QSet<QString> *taken)
{
	QString candidate = name;
	for (int n = 2 ; taken->contains(candidate) ; ++n) {
		const int dot = name.lastIndexOf(QLatin1Char('.'));
		candidate = dot > name.lastIndexOf(QLatin1Char('/'))
				? name.left(dot) + QStringLiteral("~%1").arg(n) + name.mid(dot)
				: name + QStringLiteral("~%1").arg(n);
	}
	taken->insert(candidate);
	return candidate;
}

QDomElement placeholder(QDomDocument &document, const QString &file)
{
	QDomElement part = document.createElement(PartTag);
	part.setAttribute(FileAttribute, file);
	return part;
}

QList<QDomElement> childElements(const QDomElement &parent, const QString &tag)
{
	QList<QDomElement> list;
	for (QDomElement e = parent.firstChildElement(tag) ; !e.isNull() ; e = e.nextSiblingElement(tag))
		list << e;
	return list;
}

	//An element and the text nodes on either side of it out of its parent,
	//one text node left where they were, so nothing is read twice
void removeElement(QDomElement e)
{
	QDomNode parent = e.parentNode();
	QDomNode before = e.previousSibling(), after = e.nextSibling();
	parent.removeChild(e);
	if (before.isText() && after.isText()) {
		before.setNodeValue(before.nodeValue() + after.nodeValue());
		parent.removeChild(after);
	}
}


	//The collection's symbol definitions, each to elements/<categories>/<name>
void splitCollection(QDomDocument &document, const QDomElement &category, const QString &path,
					 QList<QetZip::Entry> *parts, QSet<QString> *taken)
{
	for (QDomElement child = category.firstChildElement() ; !child.isNull() ;
		 child = child.nextSiblingElement())
	{
		const QString name = safeSegment(child.attribute(QStringLiteral("name")),
										 QStringLiteral("unnamed"));
		if (child.tagName() == QLatin1String("category")) {
			splitCollection(document, child, path + name + QLatin1Char('/'), parts, taken);
		} else if (child.tagName() == QLatin1String("element")) {
			QDomElement definition = child.firstChildElement(QStringLiteral("definition"));
			if (definition.isNull()) continue;
			const QString file = unique(path + name, taken);
			parts->append({file, serialize(definition), true});
			child.replaceChild(placeholder(document, file), definition);
		}
	}
}
}

/**
	@brief QetContainer::parse
	Parse @p xml as QETProject::openFile() reads a .qet: a text node made
	only of spaces is kept (a title-block value of " ", #973).
*/
bool QetContainer::parse(const QByteArray &xml, QDomDocument *document, QString *error)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	const auto result = document->setContent(xml, QDomDocument::ParseOption::PreserveSpacingOnlyNodes);
	if (!result) {
		setError(error, QStringLiteral("line %1: %2").arg(result.errorLine).arg(result.errorMessage));
		return false;
	}
	return true;
#else
	QString message;
	int line = 0;
	if (!document->setContent(xml, &message, &line)) {
		setError(error, QStringLiteral("line %1: %2").arg(line).arg(message));
		return false;
	}
	return true;
#endif
}

/**
	@brief QetContainer::isZip
	A .qetz is recognised by what it holds, not its name: a crash backup
	of a .qetz project is plain XML, and a renamed file is still a zip.
*/
bool QetContainer::isZip(const QByteArray &content)
{
	return content.startsWith("PK\x03\x04");
}

/**
	@brief QetContainer::readDocument
	The project document held in @p content: unzipped and joined for a
	.qetz, parsed for a .qet. @p format, if any: the .qetz's format, 0
	for a .qet.
*/
bool QetContainer::readDocument(const QByteArray &content, QDomDocument *document,
								QString *error, Pictures *pictures, int *format)
{
	if (format) *format = 0;
	if (!isZip(content))
		return parse(content, document, error);
	QList<QetZip::Entry> entries;
	return QetZip::fromBytes(content, &entries, error)
			&& join(entries, document, error, nullptr, pictures, format);
}

/**
	@brief QetContainer::recover
	What can be read of the .qetz @p content, a damaged one, into @p project:
	a folio file, a picture or an entry that cannot be read is left out,
	symbols and wires that do not match the database lose what it held, and
	a database that cannot be read loses it all (wires, whose ends it holds,
	go too). Each thing left out is said in @p lost.
	@return false, with @p error, when nothing can be read: not a zip, no
	project.xml, or a later format
*/
bool QetContainer::recover(const QByteArray &content, QDomDocument *project, QStringList *lost, QString *error)
{
	QStringList none;
	if (!lost) lost = &none;
	QList<QetZip::Entry> entries;
	return QetZip::fromBytes(content, &entries, error, lost)
			&& join(entries, project, error, nullptr, nullptr, nullptr, lost);
}

QString QetContainer::pictureFile(const QDomElement &image)
{
	return image.attribute(ImageFileAttribute);
}

void QetContainer::inlinePicture(QDomElement image, const QByteArray &bytes)
{
	image.removeAttribute(ImageFileAttribute);
	image.insertBefore(image.ownerDocument().createTextNode(QString::fromLatin1(bytes.toBase64())),
					   image.firstChild());
}

/**
	@brief QetContainer::ownUuids
	@return true if every folio, symbol and wire of @p project has a uuid
	no other has: a wire then finds its ends without the symbols' terminal
	lists, which split() leaves out
*/
bool QetContainer::ownUuids(const QDomElement &project)
{
	QSet<QString> seen;
	auto own = [&seen](const QDomElement &item) {
		const QString uuid = item.attribute(QStringLiteral("uuid"));
		if (QUuid(uuid).isNull() || seen.contains(uuid)) return false;
		seen.insert(uuid);
		return true;
	};
	for (const QDomElement &diagram : childElements(project, QStringLiteral("diagram"))) {
		if (!own(diagram)) return false;
		for (const QDomElement &list : childElements(diagram, QStringLiteral("elements")))
			for (const QDomElement &element : childElements(list, QStringLiteral("element")))
				if (!own(element)) return false;
		for (const QDomElement &list : childElements(diagram, QStringLiteral("conductors")))
			for (const QDomElement &wire : childElements(list, QStringLiteral("conductor")))
				if (!own(wire)) return false;
	}
	return true;
}

/**
	@brief QetContainer::dropComputed
	Take out of the folio @p diagram what QElectroTech computes when it
	builds the folio (stage D3c): a wire's description of
	its ends (element1_label, element1_name, element1_linked, terminalname1,
	and the same for end 2), a symbol text's content where it shows the
	symbol's information or a composition of it, and -- with
	@p terminal_lists, when every symbol has a uuid of its own -- a symbol's
	list of its terminals. Checked on 342 projects: opened and saved, each
	comes back as it was, but the terminal lists of a file whose symbols
	share a uuid (F100), where wires find their ends by them.
*/
void QetContainer::dropComputed(QDomElement diagram, bool terminal_lists)
{
	for (QDomElement list : childElements(diagram, QStringLiteral("conductors")))
		for (QDomElement wire : childElements(list, QStringLiteral("conductor")))
			for (const char *end : {"1", "2"})
				for (const char *name : {"element%1_label", "element%1_name", "element%1_linked",
										 "terminalname%1"})
					wire.removeAttribute(QString::fromLatin1(name).arg(QLatin1String(end)));
	for (QDomElement list : childElements(diagram, QStringLiteral("elements")))
	{
		for (QDomElement element : childElements(list, QStringLiteral("element")))
		{
			if (terminal_lists)
				for (QDomElement terminals : childElements(element, QStringLiteral("terminals")))
					removeElement(terminals);
			for (QDomElement texts : childElements(element, QStringLiteral("dynamic_texts")))
			{
				for (QDomElement text : childElements(texts, QStringLiteral("dynamic_elmt_text")))
				{
					const QString from = text.attribute(QStringLiteral("text_from"));
					if (from != QLatin1String("ElementInfo") && from != QLatin1String("CompositeText"))
						continue;
					for (QDomElement content : childElements(text, QStringLiteral("text")))
						while (content.hasChildNodes())
							content.removeChild(content.firstChild());
				}
			}
		}
	}
}

/**
	@brief QetContainer::split
	@param project : the project document, as QETProject::toXml() makes it
	@param writer : the program writing it, kept in the manifest
	@return the entries of the .qetz, mimetype first; empty with @p error
	set if @p project is not a project
*/
QList<QetZip::Entry> QetContainer::split(const QDomDocument &project, const QString &writer,
										 QString *error, const QHash<QString, QByteArray> &kept_from)
{
	QDomDocument document = project.cloneNode(true).toDocument();
	QDomElement root = document.documentElement();
	if (root.tagName() != QLatin1String("project")) {
		setError(error, QStringLiteral("not a project document"));
		return {};
	}

		//The engineering data out of the document, into project.sqlite
		//A folio written as it was read (<diagram qetz-kept-part="..."/>,
		//stage D4) keeps its part and, through
		//@p kept_from's project.sqlite, its rows
		//A symbol changed on a folio kept as it was gets its information
		//rows written anew: its part must have the place they go back to
	for (const QDomElement &diagram : childElements(root, QStringLiteral("diagram")))
	{
		const QList<QDomElement> edited = childElements(diagram, QStringLiteral("qetz-edited-element"));
		if (edited.isEmpty()) continue;
		QDomDocument part;
		if (!parse(kept_from.value(diagram.attribute(QStringLiteral("qetz-kept-part"))), &part, error))
			return {};
		QSet<QString> placed;
		const QDomNodeList elements = part.elementsByTagName(QStringLiteral("element"));
		for (int i = 0 ; i < elements.size() ; ++i) {
			const QDomElement element = elements.at(i).toElement();
			for (const QDomElement &marker : childElements(element, QStringLiteral("qetz-db")))
				if (marker.attribute(QStringLiteral("block")) == QLatin1String("elementInformations"))
					placed.insert(element.attribute(QStringLiteral("uuid")));
		}
		for (const QDomElement &symbol : edited)
			if (!placed.contains(symbol.attribute(QStringLiteral("uuid")))) {
				setError(error, QStringLiteral("a symbol changed on a folio kept as it was has no place for its information"));
				return {};
			}
	}
	const QByteArray database = QetContainerDb::extract(
				root, error, kept_from.value(QStringLiteral("project.sqlite")));
	if (database.isEmpty())
		return {};

	QList<QetZip::Entry> parts;
	QSet<QString> taken;

		//Folios, each with its pictures taken out
	int folio = 0;
		//Nothing QElectroTech computes is kept (format 4)
	const bool own_uuids = ownUuids(root);
	for (QDomElement diagram : childElements(root, QStringLiteral("diagram")))
		dropComputed(diagram, own_uuids);

		//The parts of the folios kept as they were, and their pictures
	QMap<QString, QByteArray> pictures;   //name -> png, once each
	for (QDomElement diagram : childElements(root, QStringLiteral("diagram")))
	{
		const QString kept = diagram.attribute(QStringLiteral("qetz-kept-part"));
		if (kept.isEmpty()) continue;
		if (!kept_from.contains(kept)) {
			setError(error, QStringLiteral("the part \"%1\" of a folio kept as it was is missing").arg(kept));
			return {};
		}
		taken.insert(kept);
		const QByteArray part = kept_from.value(kept);
		static const QRegularExpression image_file(QStringLiteral("%1=\"(images/[^\"]+)\"").arg(ImageFileAttribute));
		for (const auto &match : image_file.globalMatch(QString::fromUtf8(part))) {
			const QString name = match.captured(1);
			if (!kept_from.contains(name)) {
				setError(error, QStringLiteral("the picture \"%1\" of a folio kept as it was is missing").arg(name));
				return {};
			}
			pictures.insert(name, kept_from.value(name));
		}
		parts.append(QetZip::Entry{kept, part, true});
		root.replaceChild(placeholder(document, kept), diagram);
	}

	for (QDomElement diagram : childElements(root, QStringLiteral("diagram")))
	{
		++folio;
		for (QDomElement images : childElements(diagram, QStringLiteral("images"))) {
			for (QDomElement image : childElements(images, QStringLiteral("image"))) {
				const QDomText text = image.firstChild().toText();
				if (text.isNull() || image.hasAttribute(ImageFileAttribute)) continue;
				const QByteArray base64 = text.data().toLatin1();
				const auto decoded = QByteArray::fromBase64Encoding(
										 base64, QByteArray::AbortOnBase64DecodingErrors);
					//Only what comes back exactly leaves the folio
				if (!decoded || decoded.decoded.toBase64() != base64 || decoded.decoded.isEmpty())
					continue;
				const QString name = QStringLiteral("images/%1.%2")
						.arg(QString::fromLatin1(QCryptographicHash::hash(
													 decoded.decoded, QCryptographicHash::Sha256).toHex()),
							 decoded.decoded.startsWith("\x89PNG") ? QStringLiteral("png")
																	: QStringLiteral("bin"));
				pictures.insert(name, decoded.decoded);
				image.removeChild(text);
				image.setAttribute(ImageFileAttribute, name);
			}
		}
		const QString uuid = diagram.attribute(QStringLiteral("uuid"));
		const QString file = unique(QStringLiteral("folios/%1.xml").arg(
										safeSegment(QString(uuid).remove(QLatin1Char('{')).remove(QLatin1Char('}')),
													QString::number(folio))),
									&taken);
		parts.append(QetZip::Entry{file, serialize(diagram), true});
		root.replaceChild(placeholder(document, file), diagram);
	}

		//The project's symbols
	for (QDomElement collection : childElements(root, QStringLiteral("collection")))
		splitCollection(document, collection, QStringLiteral("elements/"), &parts, &taken);

		//Title block templates
	for (QDomElement templates : childElements(root, QStringLiteral("titleblocktemplates"))) {
		for (QDomElement tbt : childElements(templates, QStringLiteral("titleblocktemplate"))) {
			const QString file = unique(QStringLiteral("titleblocks/%1.titleblock").arg(
											safeSegment(tbt.attribute(QStringLiteral("name")),
														QStringLiteral("unnamed"))),
										&taken);
			parts.append(QetZip::Entry{file, serialize(tbt), true});
			templates.replaceChild(placeholder(document, file), tbt);
		}
	}

	QDomDocument manifest;
	QDomElement qetz = manifest.createElement(QStringLiteral("qetz"));
	qetz.setAttribute(QStringLiteral("format"), Format);
	qetz.setAttribute(QStringLiteral("min-reader"), Format);
	if (!writer.isEmpty()) qetz.setAttribute(QStringLiteral("writer"), writer);
	manifest.appendChild(qetz);

	QList<QetZip::Entry> entries;
	entries.append(QetZip::Entry{QStringLiteral("mimetype"), QByteArray(MimeType), false});
	entries.append(QetZip::Entry{QStringLiteral("manifest.xml"), serialize(qetz), true});
	entries.append(QetZip::Entry{QStringLiteral("project.xml"), serialize(root), true});
	entries.append(QetZip::Entry{QStringLiteral("project.sqlite"), database, true});
	entries += parts;
	for (auto it = pictures.cbegin() ; it != pictures.cend() ; ++it)
		entries.append(QetZip::Entry{it.key(), it.value(), false});   //PNG is compressed already
	return entries;
}

/**
	@brief QetContainer::join
	Rebuild in @p project the document split() made @p entries from.
	Refused, with @p error set: not a .qetz, made for a later QElectroTech
	(the manifest's min-reader), or a part missing or unreadable. Entries
	nothing names are ignored, so a later format may add some.
	With @p pictures, the pictures stay out of the document: see the
	namespace. With @p folio_files, the part each folio was read from, in
	order. @p format, if any: the format the file says it is, above Format
	for a later one this may read (its min-reader allows it).
*/
bool QetContainer::join(const QList<QetZip::Entry> &entries, QDomDocument *project, QString *error,
						QStringList *folio_files, Pictures *pictures, int *format, QStringList *lost)
{
		//Recovering a damaged project (recover()): what is missing or
		//damaged is noted in lost and left out, instead of refusing it all
	const auto noted = [error, lost](const QString &problem) {
		if (lost) {
			lost->append(problem);
			return true;
		}
		setError(error, problem);
		return false;
	};
	QHash<QString, QByteArray> files;
	for (const QetZip::Entry &entry : entries)
		files.insert(entry.name, entry.data);

	const QByteArray mimetype = files.value(QStringLiteral("mimetype"));
	if (mimetype != QByteArray(MimeType) && mimetype != QByteArray(OldMimeType)
			&& !noted(QStringLiteral("not a QElectroTech project (.qetz)")))
		return false;
	QDomDocument manifest;
	QString manifest_error;
	const bool has_manifest = parse(files.value(QStringLiteral("manifest.xml")), &manifest, &manifest_error);
	if (!has_manifest && !noted(manifest_error))
		return false;
	bool ok = false;
	const int min_reader = manifest.documentElement().attribute(QStringLiteral("min-reader")).toInt(&ok);
		//A later format is not damage: refused all the same
	if (has_manifest && (!ok || min_reader > Format)) {
		setError(error, QStringLiteral("made by a later QElectroTech (format %1, this one reads up to %2)")
				 .arg(manifest.documentElement().attribute(QStringLiteral("format")))
				 .arg(Format));
		return false;
	}
	if (format)
		*format = manifest.documentElement().attribute(QStringLiteral("format")).toInt();

	if (!parse(files.value(QStringLiteral("project.xml")), project, error))
		return false;

		//Parts, a snapshot first: the list is live and shrinks as they go
	const QDomNodeList list = project->elementsByTagName(PartTag);
	QList<QDomElement> placeholders;
	for (int i = 0 ; i < list.size() ; ++i) placeholders << list.at(i).toElement();
	for (QDomElement part : placeholders)
	{
		const QString file = part.attribute(FileAttribute);
		if (folio_files && part.parentNode() == project->documentElement()
				&& file.startsWith(QLatin1String("folios/")))
			folio_files->append(file);
		if (!files.contains(file)) {
			if (!noted(QStringLiteral("part \"%1\" is missing").arg(file))) return false;
			part.parentNode().removeChild(part);
			continue;
		}
		QDomDocument document;
		QString part_error;
		if (!parse(files.value(file), &document, &part_error)) {
			if (!noted(QStringLiteral("%1: %2").arg(file, part_error))) return false;
			part.parentNode().removeChild(part);
			continue;
		}
		part.parentNode().replaceChild(project->importNode(document.documentElement(), true), part);
	}

		//Pictures back to base64, as the image's first child, or into
		//pictures
	const QDomNodeList image_list = project->elementsByTagName(QStringLiteral("image"));
	QList<QDomElement> images;
	for (int i = 0 ; i < image_list.size() ; ++i) images << image_list.at(i).toElement();
	for (QDomElement image : images)
	{
		if (!image.hasAttribute(ImageFileAttribute)) continue;
		const QString file = image.attribute(ImageFileAttribute);
		if (!files.contains(file)) {
			if (!noted(QStringLiteral("picture \"%1\" is missing").arg(file))) return false;
			image.parentNode().removeChild(image);
			continue;
		}
		if (pictures) {
			pictures->insert(file, files.value(file));
			continue;
		}
		image.removeAttribute(ImageFileAttribute);
		image.insertBefore(project->createTextNode(QString::fromLatin1(files.value(file).toBase64())),
						   image.firstChild());
	}

		//The engineering data back from project.sqlite (format 2 on)
	if (files.contains(QStringLiteral("project.sqlite"))
		&& !QetContainerDb::restore(project->documentElement(),
									files.value(QStringLiteral("project.sqlite")), error, lost))
		return false;
	if (lost) {
		if (!files.contains(QStringLiteral("project.sqlite")))
			lost->append(QStringLiteral("project.sqlite is missing"));
		QetContainerDb::dropLeftovers(project->documentElement(), lost);
	}
	return true;
}
