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
#include "materiallist.h"

#include "../qetinformation.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

#include <utility>

namespace {

/**
	@brief Decode a csv file content.
	The material file is written in utf-8 with a BOM, the same way the bill
	of material export does, so spreadsheets open it with the right
	encoding. A file written by hand in the local encoding is still
	decodable: fromUtf8() is only used when the bytes are valid utf-8.
	@param raw
	@return
*/
QString decode(const QByteArray &raw)
{
	if (raw.startsWith("\xEF\xBB\xBF")) {
		return QString::fromUtf8(raw.constData() + 3, raw.size() - 3);
	}

	const QByteArray utf8 = QString::fromUtf8(raw).toUtf8();
	if (utf8 == raw) {
		return QString::fromUtf8(raw);
	}
	return QString::fromLocal8Bit(raw);
}

/**
	@brief Tell whether a file exists and holds something.
	An existing but empty file is the same as no file at all: it can be
	filled with the template without losing anything.
	@param path
	@return
*/
bool hasContent(const QString &path)
{
	const QFileInfo info(path);
	return info.exists() && info.isFile() && info.size() > 0;
}

} // namespace

/**
	@brief MaterialList::settingsKey
	@return the QSettings key holding the path of the material file
*/
QString MaterialList::settingsKey()
{
	return QStringLiteral("elements-collections/material-list-path");
}

/**
	@brief MaterialList::configuredPath
	@return the path of the material file as chosen in the preferences,
	or a null QString when the user never set one.
*/
QString MaterialList::configuredPath()
{
	QSettings settings;
	const QString path = settings.value(settingsKey(), QStringLiteral("default")).toString();
	if (path.isEmpty() || path == QLatin1String("default")) {
		return QString();
	}
	return path;
}

/**
	@brief MaterialList::setConfiguredPath
	Store the path of the material file in the preferences.
	@param path
*/
void MaterialList::setConfiguredPath(const QString &path)
{
	QSettings settings;
	settings.setValue(settingsKey(), path.isEmpty() ? QStringLiteral("default") : path);
}

/**
	@brief MaterialList::defaultFileName
	@return the file name used when the material file is created without an
	explicit location.
*/
QString MaterialList::defaultFileName()
{
	return QStringLiteral("qet_material_list.csv");
}

/**
	@brief MaterialList::defaultPath
	@return where the material file is created when no path is configured
	yet : the standard documents folder.
*/
QString MaterialList::defaultPath()
{
	QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	while (dir.endsWith(QLatin1Char('/'))) {
		dir.chop(1);
	}
	if (dir.isEmpty()) {
		dir = QDir::currentPath();
	}
	return dir + QLatin1Char('/') + defaultFileName();
}

/**
	@brief MaterialList::savedHeaderState
	@return the saved column layout of the selection window, empty when the
	window was never opened or the layout could not be read back.
*/
QByteArray MaterialList::savedHeaderState()
{
	QSettings settings;
	return settings.value(QStringLiteral("elements-collections/material-list-header")).toByteArray();
}

/**
	@brief MaterialList::saveHeaderState
	Remember the column layout of the selection window.
	@param state the header state as saved by QHeaderView::saveState()
*/
void MaterialList::saveHeaderState(const QByteArray &state)
{
	QSettings settings;
	settings.setValue(QStringLiteral("elements-collections/material-list-header"), state);
}

/**
	@brief MaterialList::savedDialogSize
	@return the size the selection window had the last time it was closed,
	an invalid size when it was never opened.
*/
QSize MaterialList::savedDialogSize()
{
	QSettings settings;
	const QSize size(settings.value(QStringLiteral("elements-collections/material-list-width")).toInt(),
					 settings.value(QStringLiteral("elements-collections/material-list-height")).toInt());
		//A size which was never written reads back as 0x0, which Qt still
		//counts as valid : it must not be handed to resize().
	return size.isEmpty() ? QSize() : size;
}

/**
	@brief MaterialList::saveDialogSize
	Remember the size of the selection window.
	@param size
*/
void MaterialList::saveDialogSize(const QSize &size)
{
	if (size.isEmpty()) {
		return;
	}
	QSettings settings;
	settings.setValue(QStringLiteral("elements-collections/material-list-width"), size.width());
	settings.setValue(QStringLiteral("elements-collections/material-list-height"), size.height());
}

/**
	@brief MaterialList::defaultColumns
	@return the columns of a brand new material file, in writing order.
	It is every information of the main article (everything but label and
	formula, which QElectroTech computes itself) plus the one column that
	describes an auxiliary block.
*/
QStringList MaterialList::defaultColumns()
{
	QStringList columns = columnsForBlock(0);
	columns << QStringLiteral("auxiliary");
	return columns;
}

/**
	@brief MaterialList::columnsForBlock
	Columns of the material file that can be transferred to a block of the
	element informations.
	@param block 0 for the main article, 1 to 4 for an auxiliary article
	@return
*/
QStringList MaterialList::columnsForBlock(int block)
{
	static const QStringList main_columns = {
		QStringLiteral("description"),
		QStringLiteral("designation"),
		QStringLiteral("manufacturer"),
		QStringLiteral("manufacturer_reference"),
		QStringLiteral("machine_manufacturer_reference"),
		QStringLiteral("supplier"),
		QStringLiteral("quantity"),
		QStringLiteral("unity"),
		QStringLiteral("function"),
		QStringLiteral("comment"),
		QStringLiteral("notes"),
		QStringLiteral("model"),
		QStringLiteral("category"),
		QStringLiteral("voltage_rating"),
		QStringLiteral("current_rating"),
		QStringLiteral("plant"),
		QStringLiteral("location"),
		QStringLiteral("width"),
		QStringLiteral("height"),
		QStringLiteral("depth")
	};

	static const QStringList auxiliary_columns = {
		QStringLiteral("description"),
		QStringLiteral("designation"),
		QStringLiteral("manufacturer"),
		QStringLiteral("manufacturer_reference"),
		QStringLiteral("machine_manufacturer_reference"),
		QStringLiteral("supplier"),
		QStringLiteral("quantity"),
		QStringLiteral("unity"),
		QStringLiteral("auxiliary")
	};

	if (block <= 0) {
		return main_columns;
	}
	if (block > 4) {
		return QStringList();
	}
	return auxiliary_columns;
}

/**
	@brief MaterialList::elementInfoKey
	Tell which element information a column of the material file feeds
	when it is applied to a block.
	@param column a column of the material file
	@param block 0 for the main article, 1 to 4 for an auxiliary article
	@return the element information key, or a null QString when that column
	does not belong to that block
*/
QString MaterialList::elementInfoKey(const QString &column, int block)
{
	if (!columnsForBlock(block).contains(column)) {
		return QString();
	}

	if (block <= 0) {
		return column;
	}

	//Every auxiliary column carries the same name as its main counterpart,
	//only the suffix tells the block apart. The block description column
	//is the exception: it is named auxiliary in the file and auxiliary1 to
	//auxiliary4 in the element informations.
	if (column == QLatin1String("auxiliary")) {
		return QStringLiteral("auxiliary%1").arg(block);
	}
	return column + QStringLiteral("_auxiliary%1").arg(block);
}

/**
	@brief MaterialList::isArticleBound
	Tell whether a column describes the article itself, rather than what
	the element does or where it stands.

	An empty cell of such a column clears the field it feeds when the
	entry is applied: an element cannot carry the manufacturer of two
	parts at once, so the value of the article picked before has to go.
	Every other column (function, comment, notes, plant, location, and
	quantity and unity, which say how many pieces this element takes and
	in which unit) is left alone by an empty cell, and so is a column the
	file does not hold at all: a file which mentions nothing about a
	field is no reason to empty it.
	@param column a column of the material file
	@return true when an empty cell of that column clears the field
*/
bool MaterialList::isArticleBound(const QString &column)
{
	static const QStringList article_columns = {
		QStringLiteral("description"),
		QStringLiteral("designation"),
		QStringLiteral("manufacturer"),
		QStringLiteral("manufacturer_reference"),
		QStringLiteral("machine_manufacturer_reference"),
		QStringLiteral("supplier"),
		QStringLiteral("model"),
		QStringLiteral("category"),
		QStringLiteral("voltage_rating"),
		QStringLiteral("current_rating"),
		QStringLiteral("width"),
		QStringLiteral("height"),
		QStringLiteral("depth"),
		QStringLiteral("auxiliary")
	};

	return article_columns.contains(column);
}

/**
	@brief MaterialList::translatedColumn
	@param column a column of the material file
	@return that column named the way it is shown to the user, in the
	current language. A column QElectroTech doesn't know is returned as it
	is, since it can only have been named by the user.
*/
QString MaterialList::translatedColumn(const QString &column)
{
	if (column == QLatin1String("auxiliary")) {
		return QCoreApplication::translate("MaterialList", "Auxiliary block");
	}

	const QString translated = QETInformation::translatedInfoKey(column);
	return translated.isEmpty() ? column : translated;
}

/**
	@brief MaterialList::translatedHeader
	The label line of a material file : the columns the way the user reads
	them, in the current language.
	@param columns
	@return
*/
QStringList MaterialList::translatedHeader(const QStringList &columns)
{
	QStringList header;
	header.reserve(columns.size());
	for (const QString &column : columns) {
		header.append(translatedColumn(column));
	}
	return header;
}

/**
	@brief MaterialList::headerAliasMap
	@return every accepted header cell of a material file, lowercased,
	mapped to the canonical column it means.
	Beside the key itself, a header may use the translated name of the
	information ("Artikelbeschreibung" for description), so a file written
	by hand in the user's language is understood as well as the canonical
	one.
*/
QMap<QString, QString> MaterialList::headerAliasMap()
{
	QMap<QString, QString> map;
	const QStringList keys = QETInformation::elementInfoKeys();

	for (const QString &key : keys)
	{
		map.insert(key.toLower(), key);
		const QString translated = QETInformation::translatedInfoKey(key);
		if (!translated.isEmpty()) {
			map.insert(translated.toLower(), key);
		}
	}

	map.insert(QStringLiteral("auxiliary"), QStringLiteral("auxiliary"));
	const QString auxiliary_label = QCoreApplication::translate("MaterialList", "Auxiliary block").toLower();
	map.insert(auxiliary_label, QStringLiteral("auxiliary"));

	return map;
}

/**
	@brief MaterialList::canonicalColumn
	Turn one header cell into a canonical column name.
	@param header_cell
	@param already_used the names chosen for the previous cells, to keep
	the header free of duplicates
	@param alias_map lowercased alias -> canonical name
	@return
*/
QString MaterialList::canonicalColumn(const QString &header_cell,
									  const QStringList &already_used,
									  const QMap<QString, QString> &alias_map)
{
	const QString raw = header_cell.trimmed();

	if (raw.isEmpty()) {
		//A nameless column keeps its data and its place, it just can't be
		//matched to an element information.
		QString name = QStringLiteral("column_%1").arg(already_used.size() + 1);
		int i = already_used.size() + 1;
		while (already_used.contains(name)) {
			name = QStringLiteral("column_%1").arg(++i);
		}
		return name;
	}

	const auto it = alias_map.constFind(raw.toLower());
	if (it != alias_map.constEnd() && !already_used.contains(it.value())) {
		return it.value();
	}

	//Either an unknown name, or a second cell meaning something which is
	//already used : the cell is kept as it is written, its data is never
	//thrown away.
	QString name = raw;
	int i = 2;
	while (already_used.contains(name)) {
		name = raw + QStringLiteral("_%1").arg(i++);
	}
	return name;
}

/**
	@brief MaterialList::isMachineHeaderLine
	Tell whether the second line of a material file is the machine header
	QElectroTech writes under the label line : the canonical column names,
	kept so that a file written in one language is still understood in
	another one.

	The check is deliberately strict about what it recognises and lenient
	about the rest : every cell is either a known column name, or the very
	same text as the label above it (which is how an unknown column shows
	up in both lines). A data row holding free text never passes.
	@param label_line the first line of the file, the one the user reads
	@param candidate the line which may be the machine header
	@param alias_map lowercased alias -> canonical name
	@return true when candidate must not be read as a record
*/
bool MaterialList::isMachineHeaderLine(const QStringList &label_line,
									   const QStringList &candidate,
									   const QMap<QString, QString> &alias_map)
{
	if (candidate.isEmpty() || candidate.size() != label_line.size()) {
		return false;
	}

	bool has_known_column = false;
	for (int i = 0; i < candidate.size(); ++i)
	{
		const QString cell = candidate.at(i).trimmed();
		if (cell.isEmpty()) {
			continue;
		}
		if (alias_map.contains(cell.toLower()))
		{
			has_known_column = true;
			continue;
		}
		if (cell == label_line.at(i).trimmed()) {
			continue;
		}
		return false;
	}

	return has_known_column;
}

/**
	@brief MaterialList::detectSeparator
	Tell which separator a csv text uses. Spreadsheets of the german
	region write a semicolon, the english ones a comma, some tools a tab.
	@param first_line the header line
	@return
*/
QChar MaterialList::detectSeparator(const QString &first_line)
{
	const int semicolons = first_line.count(QLatin1Char(';'));
	const int tabs = first_line.count(QLatin1Char('\t'));
	const int commas = first_line.count(QLatin1Char(','));

	if (semicolons == 0 && tabs == 0 && commas == 0) {
			//A header holding no separator character at all is a file of a
			//single column : falling back on the semicolon is deliberate,
			//it is the one QElectroTech writes itself, and such a file
			//needs no detection anyway. Leave this alone.
		return QLatin1Char(';');
	}
	if (tabs >= semicolons && tabs >= commas && tabs > 0) {
		return QLatin1Char('\t');
	}
	if (commas > semicolons) {
		return QLatin1Char(',');
	}
	return QLatin1Char(';');
}

/**
	@brief MaterialList::parseCsv
	Split a csv text into rows. Quoted fields may contain the separator
	and line breaks, doubled quotes mean one quote.
	@param content
	@param separator
	@return
*/
QList<QStringList> MaterialList::parseCsv(const QString &content, QChar separator)
{
	QList<QStringList> rows;
	QStringList row;
	QString field;
	bool quoted = false;

	for (int i = 0; i < content.size(); ++i)
	{
		const QChar c = content.at(i);

		if (quoted)
		{
			if (c == QLatin1Char('"'))
			{
				if (i + 1 < content.size() && content.at(i + 1) == QLatin1Char('"')) {
					field += QLatin1Char('"');
					++i;
				} else {
					quoted = false;
				}
			} else {
				field += c;
			}
		}
		else if (c == QLatin1Char('"') && field.isEmpty())
		{
			quoted = true;
		}
		else if (c == separator)
		{
			row.append(field);
			field.clear();
		}
		else if (c == QLatin1Char('\n'))
		{
			row.append(field);
			field.clear();
			rows.append(row);
			row.clear();
		}
		else if (c != QLatin1Char('\r'))
		{
			field += c;
		}
	}

	if (!field.isEmpty() || !row.isEmpty()) {
		row.append(field);
		rows.append(row);
	}

	//A trailing line break produces a last, empty row : without this it
	//would show up as a blank line in the selection dialog.
	while (!rows.isEmpty() && rows.last().size() == 1 && rows.last().first().isEmpty()) {
		rows.removeLast();
	}

	return rows;
}

/**
	@brief MaterialList::serializeCsv
	Build the content of a csv file : utf-8 BOM, quoted fields, one line
	per record. Same dialect as the bill of material export.
	The file opens with two header lines : the columns the way the user
	reads them, then the canonical names QElectroTech matches against the
	element informations.
	@param header_lines the header lines, in writing order
	@param rows
	@param separator
	@return
*/
QByteArray MaterialList::serializeCsv(const QList<QStringList> &header_lines, const QList<QStringList> &rows, QChar separator)
{
	//Helpers duplicated from the bill of material export : every field is
	//quoted, so a semicolon inside a value is never read as a separator.
	auto escape = [](QString value) {
		value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
		return QLatin1Char('"') + value + QLatin1Char('"');
	};

	auto line = [&](const QStringList &values) {
		QStringList escaped;
		escaped.reserve(values.size());
		for (const QString &value : values) {
			escaped.append(escape(value));
		}
		return escaped.join(separator) + QLatin1Char('\n');
	};

	QByteArray out("\xEF\xBB\xBF");
	for (const QStringList &header_line : header_lines) {
		out += line(header_line).toUtf8();
	}
	for (const QStringList &row : rows) {
		out += line(row).toUtf8();
	}
	return out;
}

/**
	@brief MaterialList::load
	Read a material file.
	@param path
	@param data receives the columns and the records
	@param error receives a message when the file can't be read
	@return true on success
*/
bool MaterialList::load(const QString &path, MaterialListData *data, QString *error)
{
	if (error) {
		error->clear();
	}
	if (!data) {
		if (error) {
			*error = QCoreApplication::translate("MaterialList", "Nothing to receive the materials list.");
		}
		return false;
	}

	data->columns.clear();
	data->records.clear();
	data->separator = QLatin1Char(';');

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
	{
		if (error) {
			*error = file.errorString();
		}
		return false;
	}
	const QByteArray raw = file.readAll();
	file.close();

	const QString content = decode(raw);
	if (content.trimmed().isEmpty()) {
		return true;
	}

	const int eol = content.indexOf(QLatin1Char('\n'));
	data->separator = detectSeparator(eol < 0 ? content : content.left(eol));

	QList<QStringList> rows = parseCsv(content, data->separator);
	if (rows.isEmpty()) {
		return true;
	}

	const QStringList header = rows.takeFirst();
	const QMap<QString, QString> alias_map = headerAliasMap();

		//The label line is followed by the machine header QElectroTech
		//writes itself : the canonical names, so that the file is read the
		//same way whatever the language it was written in. That machine
		//line is the one naming the columns when it is there : the label
		//line above it may be written in another language than the one
		//running now, and would then match nothing. The label line stays
		//on disk, it is only what the user reads.
	QStringList machine_line;
	if (!rows.isEmpty() && isMachineHeaderLine(header, rows.first(), alias_map)) {
		machine_line = rows.takeFirst();
	}

	QStringList used;
	for (int i = 0; i < header.size(); ++i)
	{
			//An empty machine cell means the column is named by the label
			//above it (a column added by hand shows up in both lines).
		QString cell = header.at(i);
		if (i < machine_line.size() && !machine_line.at(i).isEmpty()) {
			cell = machine_line.at(i);
		}
		const QString column = canonicalColumn(cell, used, alias_map);
		used.append(column);
		data->columns.append(column);
	}

	for (const QStringList &row : std::as_const(rows))
	{
		MaterialRecord record;
		bool empty = true;
		for (int i = 0; i < data->columns.size(); ++i)
		{
			const QString value = i < row.size() ? row.at(i) : QString();
			if (!value.isEmpty()) {
				empty = false;
			}
			record.setValue(data->columns.at(i), value);
		}
			//Cells written past the header are not thrown away : they are
			//written back the way they were read, and they are content too,
			//so a line only they fill is kept as well.
		for (int i = data->columns.size(); i < row.size(); ++i)
		{
			if (!row.at(i).isEmpty()) {
				empty = false;
			}
			record.extra.append(row.at(i));
		}
		if (!empty) {
			data->records.append(record);
		}
	}

	return true;
}

/**
	@brief MaterialList::writeFile
	Write a whole material file atomically : a crash or a full disk never
	leaves a half written catalogue behind.
	@param path
	@param data
	@param error receives a message when the file can't be written
	@return true on success
*/
bool MaterialList::writeFile(const QString &path, const MaterialListData &data, QString *error)
{
	if (error) {
		error->clear();
	}

	QList<QStringList> rows;
	rows.reserve(data.records.size());
	for (const MaterialRecord &record : data.records)
	{
		QStringList row;
		row.reserve(data.columns.size());
		for (const QString &column : data.columns) {
			row.append(record.value(column));
		}
			//The cells the file holds past the header, if any, follow the
			//columns : a line wider than the header stays that wide.
		row.append(record.extra);
		rows.append(row);
	}

	QList<QStringList> header_lines;
	header_lines.append(translatedHeader(data.columns));
	header_lines.append(data.columns);
	const QByteArray content = serializeCsv(header_lines, rows, data.separator);

	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size())
	{
		if (error) {
			*error = file.errorString();
		}
		file.cancelWriting();
		return false;
	}
	if (!file.commit())
	{
		if (error) {
			*error = file.errorString();
		}
		return false;
	}
	return true;
}

/**
	@brief MaterialList::appendRecord
	Add one article at the end of a material file.
	The file is read again first: changes the user just saved from his
	spreadsheet are part of what gets rewritten, never dropped.
	@param path
	@param record
	@param error receives a message when the file can't be written
	@return true on success
*/
bool MaterialList::appendRecord(const QString &path, const MaterialRecord &record, QString *error)
{
	if (error) {
		error->clear();
	}

	MaterialListData data;
	if (hasContent(path))
	{
		if (!load(path, &data, error)) {
			return false;
		}
		if (data.columns.isEmpty())
		{
			if (error) {
				*error = QCoreApplication::translate("MaterialList", "The file has no header: columns are missing.");
			}
			return false;
		}
	}
	else
	{
		data.columns = defaultColumns();
		data.separator = QLatin1Char(';');
	}

	data.records.append(record);
	return writeFile(path, data, error);
}

/**
	@brief MaterialList::createFile
	Create an empty material file holding only the header line.
	@param path
	@param error receives a message when the file can't be written
	@return true on success
*/
bool MaterialList::createFile(const QString &path, QString *error)
{
	if (error) {
		error->clear();
	}

	if (hasContent(path))
	{
		if (error) {
			*error = QCoreApplication::translate("MaterialList", "The file already exists and is not empty.");
		}
		return false;
	}

	MaterialListData data;
	data.columns = defaultColumns();
	data.separator = QLatin1Char(';');
	return writeFile(path, data, error);
}

/**
	@brief MaterialList::isEmptyFile
	@param path
	@return true when no file, or an empty one, exists at path
*/
bool MaterialList::isEmptyFile(const QString &path)
{
	return !hasContent(path);
}
