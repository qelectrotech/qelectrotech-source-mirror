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
#include "../autoNum/assignvariables.h"
#include "../bomexport.h"
#include "../NameList/nameslist.h"
#include "../qetapp.h"

#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSet>

#include <algorithm>

namespace {

	bool readXml(const QString &file_path, QDomDocument &document,
				 QString *error)
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
			*error = QStringLiteral("line %1: %2")
					 .arg(result.errorLine).arg(result.errorMessage);
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

		/// The names in one cell
	const QString separator = QStringLiteral(", ");

		/// The end of the run of ASCII digits from @p i in @p s
	int digitsEnd(const QString &s, int i)
	{
		while (i < s.size() && s.at(i) >= QLatin1Char('0')
			   && s.at(i) <= QLatin1Char('9'))
			++i;
		return i;
	}

	/**
		Compares @p a and @p b in natural order: numbers by value (2
		before 10), letters regardless of case. Not QCollator, which
		ignores its numeric mode in the C locale, and orders the same
		on every machine.
		@return < 0, 0 or > 0
	*/
	int naturalCompare(const QString &a, const QString &b)
	{
		int i = 0, j = 0;
		while (i < a.size() && j < b.size()) {
			const int a_end = digitsEnd(a, i), b_end = digitsEnd(b, j);
			if (a_end > i && b_end > j) {
					//Leading zeros left out, the longer number is the
					//bigger
				while (i < a_end - 1 && a.at(i) == QLatin1Char('0')) ++i;
				while (j < b_end - 1 && b.at(j) == QLatin1Char('0')) ++j;
				if (a_end - i != b_end - j)
					return (a_end - i) - (b_end - j);
				const int order = QStringView(a).mid(i, a_end - i)
						.compare(QStringView(b).mid(j, b_end - j));
				if (order != 0) return order;
				i = a_end;
				j = b_end;
				continue;
			}
			const QChar ca = a.at(i).toCaseFolded();
			const QChar cb = b.at(j).toCaseFolded();
			if (ca != cb) return ca < cb ? -1 : 1;
			++i;
			++j;
		}
		const int rest = int(a.size() - i) - int(b.size() - j);
		return rest != 0 ? rest : a.compare(b);
	}
}

/**
	@brief LibraryListing::scan
	@param folder : a collection folder, or any folder of symbols
	@param progress : if set, called before each file with how many are
	done and how many there are; returning false stops the scan
	@return a row per symbol and per macro in @p folder and its
	sub-folders: a folder's own files, then its sub-folders, each in
	natural order (2 before 10); none if stopped
*/
QList<LibraryListing::Row> LibraryListing::scan(
		const QString &folder, const std::function<bool (int, int)> &progress)
{
	const QDir root(folder);
		//Folders then the file name, relative to root. Name filters
		//match regardless of case unless QDir::CaseSensitive is given.
	QList<QStringList> files;
	QDirIterator it(folder,
					{QStringLiteral("*.elmt"), QStringLiteral("*.qetmak")},
					QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		files << root.relativeFilePath(it.next()).split(QLatin1Char('/'));
	}
		//Folder name by folder name, so that a folder's sub-folders come
		//right after it; a folder's own files before its sub-folders
	std::sort(files.begin(), files.end(),
			  [](const QStringList &a, const QStringList &b) {
		const int folders = int(std::min(a.size(), b.size())) - 1;
		for (int i = 0 ; i < folders ; ++i) {
			const int order = naturalCompare(a.at(i), b.at(i));
			if (order != 0) return order < 0;
		}
		if (a.size() != b.size()) return a.size() < b.size();
		return naturalCompare(a.last(), b.last()) < 0;
	});

	QList<Row> rows;
	for (int i = 0 ; i < files.size() ; ++i) {
		if (progress && !progress(i, int(files.size()))) {
			return QList<Row>();
		}
		const QStringList &parts = files.at(i);
		const QString path = root.filePath(parts.join(QLatin1Char('/')));
		const bool is_macro = parts.last().endsWith(
					QLatin1String(".qetmak"), Qt::CaseInsensitive);
		Row row = is_macro ? macro(path) : symbol(path);
		row.folder = parts.mid(0, parts.size() - 1).join(QLatin1Char('/'));
		row.file = parts.last();
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
	row.name = QFileInfo(file_path).completeBaseName();
	QDomDocument document;
	if (!readXml(file_path, document, &row.error)) {
		return row;
	}
	const QDomElement definition = document.documentElement();
	NamesList names;
	names.fromXml(definition);
	row.name = names.name(row.name);
	row.type = definition.attribute(QStringLiteral("link_type"),
									QStringLiteral("simple"));

	const QDomNodeList terminals =
			definition.elementsByTagName(QStringLiteral("terminal"));
	row.terminals = terminals.count();
	for (int i = 0 ; i < terminals.count() ; ++i) {
		const QString name = terminals.at(i).toElement()
				.attribute(QStringLiteral("name")).trimmed();
		if (name.isEmpty()) {
			++row.unnamed_terminals;
		} else {
			row.terminal_names << name;
		}
	}
	row.class_letter = classLetter(file_path);
	return row;
}

/**
	@brief LibraryListing::macro
	@return the row of the .qetmak at @p file_path: its file name as name,
	and the labels of the symbols it places, folio reports left out
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
	const QDomElement root = document.documentElement();

		//The symbols a macro carries, by the type its elements name them by
	QSet<QString> reports;
	const QString element_tag = QStringLiteral("element");
	const QDomElement collection =
			root.firstChildElement(QStringLiteral("collection"));
	for (QDomElement e = collection.firstChildElement(element_tag)
		 ; !e.isNull() ; e = e.nextSiblingElement(element_tag)) {
		const QString link_type =
				e.firstChildElement(QStringLiteral("definition"))
				.attribute(QStringLiteral("link_type"));
		if (link_type.contains(QLatin1String("report"))) {
			reports << QStringLiteral("macro://")
					   + e.attribute(QStringLiteral("path"));
		}
	}

	const QDomElement content =
			root.firstChildElement(QStringLiteral("diagram_content"));
	const QDomNodeList elements = content.elementsByTagName(element_tag);
	for (int i = 0 ; i < elements.count() ; ++i) {
		const QDomElement element = elements.at(i).toElement();
		if (reports.contains(element.attribute(QStringLiteral("type"))))
			continue;
		const QDomNodeList infos = element.elementsByTagName(
					QStringLiteral("elementInformation"));
		for (int j = 0 ; j < infos.count() ; ++j) {
			const QDomElement info = infos.at(j).toElement();
			if (info.attribute(QStringLiteral("name"))
				== QLatin1String("label")) {
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
	The class letter the symbol at @p file_path gets when placed. In a
	collection QElectroTech knows (the common, company and user ones),
	read as placing reads it (autonum::elementPrefixForTree()). Elsewhere,
	from the nearest qet_labels.xml in a folder above the symbol's, if it
	describes the symbol's folder; one further up is not read.
	@return the letter, or an empty string for none
*/
QString LibraryListing::classLetter(const QString &file_path)
{
	const QFileInfo info(file_path);
	const QString dir_path = info.absolutePath();
	const auto cached = m_letters.constFind(dir_path);
	if (cached != m_letters.constEnd()) {
		return cached.value();
	}

	QString letter;
	bool known = false;
	const QStringList collections {QETApp::commonElementsDir(),
								   QETApp::companyElementsDir(),
								   QETApp::customElementsDir()};
	for (const QString &collection : collections) {
		const QString relative =
				QDir(collection).relativeFilePath(info.absoluteFilePath());
		if (collection.isEmpty() || relative.startsWith(QLatin1String(".."))
			|| QDir::isAbsolutePath(relative)) {
			continue;
		}
			//The file name, then its folders up to the top-level tree
		const QStringList parts = relative.split(QLatin1Char('/'));
		QStringList path;
		for (int i = int(parts.size()) - 1 ; i >= 1 ; --i)
			path << parts.at(i);
		if (path.isEmpty()) path << parts.first();
		letter = autonum::elementPrefixForTree(
					path, parts.size() > 1 ? parts.first() : QString());
		known = true;
		break;
	}

		//Not in a known collection: the nearest labels file above decides,
		//so that a stray one further up, in a home folder say, does not
	QDir ancestor(dir_path);
	while (!known) {
		const QString labels =
				ancestor.filePath(QStringLiteral("qet_labels.xml"));
		if (QFileInfo::exists(labels)) {
			const QString relative = ancestor.relativeFilePath(dir_path);
			const QStringList folders = relative == QLatin1String(".")
					? QStringList()
					: relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);
			if (!folders.isEmpty()) {
				QStringList path {info.fileName()};
				for (auto it = folders.crbegin() ; it != folders.crend() ; ++it)
					path << *it;
				letter = QetLabelsFile::prefixForPath(
							labels, path, int(path.size()) - 1);
			}
			break;
		}
		if (!ancestor.cdUp()) break;
	}
	letter = letter.isNull() ? QString() : letter;
	m_letters.insert(dir_path, letter);
	return letter;
}

/**
	@brief LibraryListing::toCsv
	@return @p rows as CSV with a header row, separated by ';' as the
	other list exports; the names in a cell are separated by ", "
*/
QByteArray LibraryListing::toCsv(const QList<Row> &rows)
{
	QByteArray csv = BomExport::csvRecord({
			QStringLiteral("name"), QStringLiteral("folder"),
			QStringLiteral("file"), QStringLiteral("kind"),
			QStringLiteral("type"), QStringLiteral("class_letter"),
			QStringLiteral("terminals"), QStringLiteral("terminal_names"),
			QStringLiteral("unnamed_terminals"), QStringLiteral("labels"),
			QStringLiteral("error")});
	for (const Row &row : rows) {
		const bool readable = row.error.isEmpty();
		csv += BomExport::csvRecord({
			row.name, row.folder, row.file,
			row.macro ? QStringLiteral("macro") : QStringLiteral("symbol"),
			row.type, row.class_letter,
			row.macro || !readable ? QString()
								   : QString::number(row.terminals),
			row.terminal_names.join(separator),
			row.macro || !readable ? QString()
								   : QString::number(row.unnamed_terminals),
			row.labels.join(separator),
			row.error});
	}
	return csv;
}

/**
	@brief LibraryListing::write
	Writes @p rows to @p file_name as CSV in UTF-8 with a byte order mark,
	as the other list exports do, so that Excel reads it right
	@return true if written; else false and @p error says why
*/
bool LibraryListing::write(const QList<Row> &rows, const QString &file_name,
						   QString *error)
{
	QSaveFile file(file_name);
	if (!file.open(QIODevice::WriteOnly)) {
		if (error) *error = file.errorString();
		return false;
	}
	const QByteArray csv = QByteArray("\xEF\xBB\xBF") + toCsv(rows);
	if (file.write(csv) != csv.size()) {
		if (error) *error = file.errorString();
		file.cancelWriting();
		return false;
	}
	if (!file.commit()) {
		if (error) *error = file.errorString();
		return false;
	}
	return true;
}
