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

const char QetContainer::MimeType[] = "application/x-qelectrotech-project+zip";

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
	.qetz, parsed for a .qet.
*/
bool QetContainer::readDocument(const QByteArray &content, QDomDocument *document, QString *error)
{
	if (!isZip(content))
		return parse(content, document, error);
	QList<QetZip::Entry> entries;
	return QetZip::fromBytes(content, &entries, error)
			&& join(entries, document, error);
}

/**
	@brief QetContainer::split
	@param project : the project document, as QETProject::toXml() makes it
	@param writer : the program writing it, kept in the manifest
	@return the entries of the .qetz, mimetype first; empty with @p error
	set if @p project is not a project
*/
QList<QetZip::Entry> QetContainer::split(const QDomDocument &project, const QString &writer,
										 QString *error)
{
	QDomDocument document = project.cloneNode(true).toDocument();
	QDomElement root = document.documentElement();
	if (root.tagName() != QLatin1String("project")) {
		setError(error, QStringLiteral("not a project document"));
		return {};
	}

		//The engineering data out of the document, into project.sqlite
	const QByteArray database = QetContainerDb::extract(root, error);
	if (database.isEmpty())
		return {};

	QList<QetZip::Entry> parts;
	QSet<QString> taken;

		//Folios, each with its pictures taken out
	int folio = 0;
	QMap<QString, QByteArray> pictures;   //name -> png, once each
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
*/
bool QetContainer::join(const QList<QetZip::Entry> &entries, QDomDocument *project, QString *error)
{
	QHash<QString, QByteArray> files;
	for (const QetZip::Entry &entry : entries)
		files.insert(entry.name, entry.data);

	if (files.value(QStringLiteral("mimetype")) != QByteArray(MimeType)) {
		setError(error, QStringLiteral("not a QElectroTech project (.qetz)"));
		return false;
	}
	QDomDocument manifest;
	if (!parse(files.value(QStringLiteral("manifest.xml")), &manifest, error))
		return false;
	bool ok = false;
	const int min_reader = manifest.documentElement().attribute(QStringLiteral("min-reader")).toInt(&ok);
	if (!ok || min_reader > Format) {
		setError(error, QStringLiteral("made by a later QElectroTech (format %1, this one reads up to %2)")
				 .arg(manifest.documentElement().attribute(QStringLiteral("format")))
				 .arg(Format));
		return false;
	}

	if (!parse(files.value(QStringLiteral("project.xml")), project, error))
		return false;

		//Parts, a snapshot first: the list is live and shrinks as they go
	const QDomNodeList list = project->elementsByTagName(PartTag);
	QList<QDomElement> placeholders;
	for (int i = 0 ; i < list.size() ; ++i) placeholders << list.at(i).toElement();
	for (QDomElement part : placeholders)
	{
		const QString file = part.attribute(FileAttribute);
		if (!files.contains(file)) {
			setError(error, QStringLiteral("part \"%1\" is missing").arg(file));
			return false;
		}
		QDomDocument document;
		QString part_error;
		if (!parse(files.value(file), &document, &part_error)) {
			setError(error, QStringLiteral("%1: %2").arg(file, part_error));
			return false;
		}
		part.parentNode().replaceChild(project->importNode(document.documentElement(), true), part);
	}

		//Pictures back to base64, as the image's first child
	const QDomNodeList images = project->elementsByTagName(QStringLiteral("image"));
	for (int i = 0 ; i < images.size() ; ++i)
	{
		QDomElement image = images.at(i).toElement();
		if (!image.hasAttribute(ImageFileAttribute)) continue;
		const QString file = image.attribute(ImageFileAttribute);
		if (!files.contains(file)) {
			setError(error, QStringLiteral("picture \"%1\" is missing").arg(file));
			return false;
		}
		image.removeAttribute(ImageFileAttribute);
		image.insertBefore(project->createTextNode(QString::fromLatin1(files.value(file).toBase64())),
						   image.firstChild());
	}

		//The engineering data back from project.sqlite (format 2 on)
	if (files.contains(QStringLiteral("project.sqlite"))
		&& !QetContainerDb::restore(project->documentElement(),
									files.value(QStringLiteral("project.sqlite")), error))
		return false;
	return true;
}
