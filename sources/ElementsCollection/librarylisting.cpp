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
#include "librarylisting.h"
#include "qetlabelsfile.h"
#include "../NameList/nameslist.h"

#include <QCollator>
#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringBuilder>

#include <algorithm>

namespace {

	/**
		@brief csvField
		@return @p text as a field of RFC 4180 CSV: quoted when it holds a
		comma, a quote or a line break
	*/
	QString csvField(const QString &text)
	{
		if (!text.contains(QLatin1Char(',')) && !text.contains(QLatin1Char('"'))
				&& !text.contains(QLatin1Char('\n')) && !text.contains(QLatin1Char('\r'))) {
			return text;
		}
		QString quoted = text;
		quoted.replace(QLatin1Char('"'), QStringLiteral("\"\""));
		return QLatin1Char('"') % quoted % QLatin1Char('"');
	}

	bool readXml(const QString &file_path, QDomDocument &document, QString *error)
	{
		QFile file(file_path);
		if (!file.open(QIODevice::ReadOnly)) {
			*error = file.errorString();
			return false;
		}
			//QDomDocument::ParseResult only exists since Qt 6.5
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
		const auto result = document.setContent(&file);
		if (!result) {
			*error = QStringLiteral("line %1: %2").arg(result.errorLine).arg(result.errorMessage);
			return false;
		}
#else
		QString message;
		int line = 0;
		if (!document.setContent(&file, &message, &line)) {
			*error = QStringLiteral("line %1: %2").arg(line).arg(message);
			return false;
		}
#endif
		return true;
	}
}

/**
	@brief LibraryListing::scan
	@param folder : a collection folder, or any folder of symbols
	@return a row per symbol and per macro in @p folder and its
	sub-folders, folder by folder, in natural order (2 before 10)
*/
QList<LibraryListing::Row> LibraryListing::scan(const QString &folder)
{
	const QDir root(folder);
	QStringList files;
	QDirIterator it(folder, {QStringLiteral("*.elmt"), QStringLiteral("*.qetmak")},
					QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		files << root.relativeFilePath(it.next());
	}
	QCollator collator;
	collator.setNumericMode(true);
	collator.setCaseSensitivity(Qt::CaseInsensitive);
		//A folder's own files, then its sub-folders
	std::sort(files.begin(), files.end(), [&collator](const QString &a, const QString &b) {
		const QFileInfo ia(a), ib(b);
		const int folder = collator.compare(ia.path(), ib.path());
		return folder != 0 ? folder < 0 : collator.compare(ia.fileName(), ib.fileName()) < 0;
	});

	QList<Row> rows;
	for (const QString &relative : std::as_const(files)) {
		const QString path = root.filePath(relative);
		Row row = relative.endsWith(QLatin1String(".qetmak")) ? macro(path) : symbol(path);
		const QFileInfo info(relative);
		row.folder = info.path() == QLatin1String(".") ? QString() : info.path();
		row.file = info.fileName();
		rows << row;
	}
	return rows;
}

/**
	@brief LibraryListing::symbol
	@return the row of the .elmt at @p file_path
*/
LibraryListing::Row LibraryListing::symbol(const QString &file_path)
{
	Row row;
	QDomDocument document;
	if (!readXml(file_path, document, &row.error)) {
		return row;
	}
	const QDomElement definition = document.documentElement();
	NamesList names;
	names.fromXml(definition);
	row.name = names.name(QFileInfo(file_path).completeBaseName());
	row.type = definition.attribute(QStringLiteral("link_type"), QStringLiteral("simple"));

	const QDomNodeList terminals = definition.elementsByTagName(QStringLiteral("terminal"));
	row.terminals = terminals.count();
	for (int i = 0 ; i < terminals.count() ; ++i) {
		const QString name = terminals.at(i).toElement().attribute(QStringLiteral("name")).trimmed();
		if (name.isEmpty()) {
			++row.unnamed_terminals;
		} else {
			row.terminal_names << name;
		}
	}
	row.class_letter = classLetter(QFileInfo(file_path).absolutePath());
	return row;
}

/**
	@brief LibraryListing::macro
	@return the row of the .qetmak at @p file_path: its file name as name,
	and the labels of the symbols it places
*/
LibraryListing::Row LibraryListing::macro(const QString &file_path)
{
	Row row;
	row.macro = true;
	row.name = QFileInfo(file_path).completeBaseName();
	QDomDocument document;
	if (!readXml(file_path, document, &row.error)) {
		return row;
	}
	const QDomElement content = document.documentElement().firstChildElement(QStringLiteral("diagram_content"));
	const QDomNodeList elements = content.elementsByTagName(QStringLiteral("element"));
	for (int i = 0 ; i < elements.count() ; ++i) {
		const QDomNodeList infos = elements.at(i).toElement().elementsByTagName(QStringLiteral("elementInformation"));
		for (int j = 0 ; j < infos.count() ; ++j) {
			const QDomElement info = infos.at(j).toElement();
			if (info.attribute(QStringLiteral("name")) == QLatin1String("label")) {
				const QString label = info.text().trimmed();
				if (!label.isEmpty()) row.labels << label;
				break;
			}
		}
	}
	return row;
}

/**
	@brief LibraryListing::classLetter
	The class letter the symbols of the folder @p dir_path get when placed:
	the prefix of the nearest qet_labels.xml, in that folder or above it,
	that describes the folder (a folder it does not name takes its parent's
	prefix, as when placing a symbol)
	@return the letter, or an empty string for none
*/
QString LibraryListing::classLetter(const QString &dir_path)
{
	const auto cached = m_letters.constFind(dir_path);
	if (cached != m_letters.constEnd()) {
		return cached.value();
	}

	QString letter;
	QDir ancestor(dir_path);
	while (ancestor.cdUp()) {
		const QString labels = ancestor.filePath(QStringLiteral("qet_labels.xml"));
		if (!QFileInfo::exists(labels)) continue;
		const QStringList folders = ancestor.relativeFilePath(dir_path).split(QLatin1Char('/'), Qt::SkipEmptyParts);
		const QString prefix = QetLabelsFile::prefixForFolder(labels, folders);
		if (!prefix.isNull()) {
			letter = prefix;
			break;
		}
	}
	m_letters.insert(dir_path, letter);
	return letter;
}

/**
	@brief LibraryListing::toCsv
	@return @p rows as CSV with a header row; the names in a cell are
	separated by a space
*/
QString LibraryListing::toCsv(const QList<Row> &rows)
{
	QString csv = QStringLiteral("name,folder,file,kind,type,class_letter,terminals,"
								 "terminal_names,unnamed_terminals,labels,error\n");
	for (const Row &row : rows) {
		const QStringList fields {
			row.name, row.folder, row.file,
			row.macro ? QStringLiteral("macro") : QStringLiteral("symbol"),
			row.type, row.class_letter,
			row.macro ? QString() : QString::number(row.terminals),
			row.terminal_names.join(QLatin1Char(' ')),
			row.macro ? QString() : QString::number(row.unnamed_terminals),
			row.labels.join(QLatin1Char(' ')),
			row.error
		};
		QStringList cells;
		for (const QString &field : fields) cells << csvField(field);
		csv += cells.join(QLatin1Char(',')) % QLatin1Char('\n');
	}
	return csv;
}

/**
	@brief LibraryListing::write
	Writes @p rows to @p file_name as CSV in UTF-8 with a byte order mark,
	as the other list exports do, so that Excel reads it right
	@return true if written; else false and @p error says why
*/
bool LibraryListing::write(const QList<Row> &rows, const QString &file_name, QString *error)
{
	QSaveFile file(file_name);
	if (!file.open(QIODevice::WriteOnly)) {
		if (error) *error = file.errorString();
		return false;
	}
	file.write("\xEF\xBB\xBF");
	file.write(toCsv(rows).toUtf8());
	if (!file.commit()) {
		if (error) *error = file.errorString();
		return false;
	}
	return true;
}
