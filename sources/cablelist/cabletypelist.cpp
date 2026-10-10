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
#include "cabletypelist.h"

#include "../materiallist/materiallist.h"
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
	@brief The columns of a cable type file -- the whole catalogue, and
	the only ones the program reads anything from.

	Three of them: the name of the type, how many cores it has and the
	colour of each of them. In that order they are written into a brand
	new file, and a label line is matched against them so that a
	catalogue read back never depends on the wording of its first line.

	The material file the pattern was copied from holds more columns
	(section, sheath colour, manufacturer and so on); they were never
	read here and are not listed. A file which has them anyway loses
	nothing: a header cell outside this list keeps its own name and its
	own data, it carries no meaning, and the machine header line is
	recognized by its shape rather than by this list, so an extra
	column there never turns that line into a record of cables.
*/
const QStringList &canonicalColumns()
{
	static const QStringList columns = {
		QStringLiteral("designation"),
		QStringLiteral("cores"),
		QStringLiteral("core_colors")
	};
	return columns;
}

/**
	@brief Tell whether a cell of the machine header can be a column key.

	A key is one plain lowercase word -- core_colors and
	manufacturer_reference are typical -- written the same whichever
	language the file is read in, which is exactly why it can sit under
	the label line. A key the program doesn't know is still a key : its
	column is kept and shown the way it is written, it carries no
	meaning. What the shape really protects is the other side: a cable
	("H07V-K 3G1,5") never looks like this, so a line of cables is
	never mistaken for a header.
	@param cell
	@return true when the cell can be part of the machine header line
*/
bool isMachineKey(const QString &cell)
{
	if (cell.isEmpty()) {
		return false;
	}
	for (const QChar c : cell)
	{
		const ushort u = c.unicode();
		const bool key = (u >= 'a' && u <= 'z')
						  || (u >= '0' && u <= '9')
						  || u == '_';
		if (!key) {
			return false;
		}
	}
		//A key starts with a letter, so that a row of numbers can never
		//pass for one.
	return cell.at(0).unicode() >= 'a' && cell.at(0).unicode() <= 'z';
}

/**
	@brief Decode a csv file content.
	The cable type file is written in utf-8 with a BOM, the same way the
	material file is, so spreadsheets open it with the right encoding. A
	file written by hand in the local encoding is still decodable:
	fromUtf8() is only used when the bytes are valid utf-8.
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
	@brief CableTypeList::settingsKey
	@return the QSettings key holding the path of the cable type file
*/
QString CableTypeList::settingsKey()
{
	return QStringLiteral("cable-management/cable-type-list-path");
}

/**
	@brief CableTypeList::configuredPath
	@return the path of the cable type file as chosen in the preferences,
	or a null QString when the user never set one.
*/
QString CableTypeList::configuredPath()
{
	QSettings settings;
	const QString path = settings.value(settingsKey(), QStringLiteral("default")).toString();
	if (path.isEmpty() || path == QLatin1String("default")) {
		return QString();
	}
	return path;
}

/**
	@brief CableTypeList::setConfiguredPath
	Store the path of the cable type file in the preferences.
	@param path
*/
void CableTypeList::setConfiguredPath(const QString &path)
{
	QSettings settings;
	settings.setValue(settingsKey(), path.isEmpty() ? QStringLiteral("default") : path);
}

/**
	@brief CableTypeList::defaultFileName
	@return the file name used when the cable type file is created without
	an explicit location.
*/
QString CableTypeList::defaultFileName()
{
	return QStringLiteral("qet_cable_types.csv");
}

/**
	@brief CableTypeList::defaultPath
	@return where the cable type file is created when no path is
	configured yet : the standard documents folder.
*/
QString CableTypeList::defaultPath()
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
	@brief CableTypeList::savedHeaderState
	@return the saved column layout of the selection window, empty when the
	window was never opened or the layout could not be read back.
*/
QByteArray CableTypeList::savedHeaderState()
{
	QSettings settings;
	return settings.value(QStringLiteral("cable-management/cable-type-list-header")).toByteArray();
}

/**
	@brief CableTypeList::saveHeaderState
	Remember the column layout of the selection window.
	@param state the header state as saved by QHeaderView::saveState()
*/
void CableTypeList::saveHeaderState(const QByteArray &state)
{
	QSettings settings;
	settings.setValue(QStringLiteral("cable-management/cable-type-list-header"), state);
}

/**
	@brief CableTypeList::savedDialogSize
	@return the size the selection window had the last time it was closed,
	an invalid size when it was never opened.
*/
QSize CableTypeList::savedDialogSize()
{
	QSettings settings;
	const QSize size(settings.value(QStringLiteral("cable-management/cable-type-list-width")).toInt(),
					 settings.value(QStringLiteral("cable-management/cable-type-list-height")).toInt());
		//A size which was never written reads back as 0x0, which Qt still
		//counts as valid : it must not be handed to resize().
	return size.isEmpty() ? QSize() : size;
}

/**
	@brief CableTypeList::saveDialogSize
	Remember the size of the selection window.
	@param size
*/
void CableTypeList::saveDialogSize(const QSize &size)
{
	if (size.isEmpty()) {
		return;
	}
	QSettings settings;
	settings.setValue(QStringLiteral("cable-management/cable-type-list-width"), size.width());
	settings.setValue(QStringLiteral("cable-management/cable-type-list-height"), size.height());
}

/**
	@brief CableTypeList::defaultColumns
	@return the columns of a brand new cable type file, in writing order.
*/
QStringList CableTypeList::defaultColumns()
{
	return canonicalColumns();
}

/**
	@brief CableTypeList::translatedColumn
	@param column a column of the cable type file
	@return that column named the way it is shown to the user, in the
	current language. A column QElectroTech doesn't know is returned as it
	is, since it can only have been named by the user.
*/
QString CableTypeList::translatedColumn(const QString &column)
{
	//The columns which are specific to a cable keep their own wording,
	//the ones shared with the material file reuse the element
	//information wording, so both catalogues read the same. The type
	//column is the exception: it is called Désignation here, the way
	//the cable form calls it, and not "Numéro d'article", which is
	//what an element means by it.
	static const QMap<QString, QString> own = {
		{QStringLiteral("designation"),
		 QCoreApplication::translate("CableTypeList", "Désignation")},
		{QStringLiteral("cores"),
		 QCoreApplication::translate("CableTypeList", "Nombre d'âmes")},
		{QStringLiteral("core_colors"),
		 QCoreApplication::translate("CableTypeList", "Couleurs des âmes")},
		{QStringLiteral("section"),
		 QCoreApplication::translate("CableTypeList", "Section du câble")},
		{QStringLiteral("color"),
		 QCoreApplication::translate("CableTypeList", "Couleur du fourreau")}
	};

	const auto it = own.constFind(column);
	if (it != own.constEnd()) {
		return it.value();
	}

	const QString translated = QETInformation::translatedInfoKey(column);
	return translated.isEmpty() ? column : translated;
}

/**
	@brief CableTypeList::translatedHeader
	The label line of a cable type file : the columns the way the user
	reads them, in the current language.
	@param columns
	@return
*/
QStringList CableTypeList::translatedHeader(const QStringList &columns)
{
	QStringList header;
	header.reserve(columns.size());
	for (const QString &column : columns) {
		header.append(translatedColumn(column));
	}
	return header;
}

/**
	@brief CableTypeList::designation
	@param record
	@return the type name of that cable, the way it shows in the
	selection list and on the drawing
*/
QString CableTypeList::designation(const CableTypeRecord &record)
{
	return record.value(QStringLiteral("designation"));
}

/**
	@brief CableTypeList::coreCount
	Tell how many cores a cable type holds.
	@param record
	@return the number of cores, or 0 when the file doesn't say. 0 means
	"unknown", which the selection window treats as usable : a file which
	says nothing is no reason to refuse a type.
*/
int CableTypeList::coreCount(const CableTypeRecord &record)
{
	bool ok = false;
	const int cores = record.value(QStringLiteral("cores")).trimmed().toInt(&ok);
	return (ok && cores > 0) ? cores : 0;
}

/**
	@brief CableTypeList::coreColors
	The colours of the cores, in the order they are written in the file,
	which is the order they are given to the cores of a drawn cable.
	@param record
	@return

	The cell holds the colours separated by |, ; or ,. Writing always
	uses | because it can't appear in a colour name and never needs
	quoting, reading accepts the three so a hand written file works too.
*/
QStringList CableTypeList::coreColors(const CableTypeRecord &record)
{
	QStringList colors;
	const QString cell = record.value(QStringLiteral("core_colors"));
	for (const QString &raw : cell.split(QLatin1Char('|'), Qt::SkipEmptyParts))
	{
		for (const QString &part : raw.split(QLatin1Char(';'), Qt::SkipEmptyParts))
		{
			for (const QString &color : part.split(QLatin1Char(','), Qt::SkipEmptyParts))
			{
				const QString trimmed = color.trimmed();
				if (!trimmed.isEmpty()) {
					colors.append(trimmed);
				}
			}
		}
	}
	return colors;
}

/**
	@brief CableTypeList::headerAliasMap
	@return every accepted header cell of a cable type file, lowercased,
	mapped to the canonical column it means.

	Beside the key itself, a header may use the translated name of the
	column ("Nombre d'âmes" for cores), or the name and translation of an
	element information, so a catalogue sharing columns with the material
	file is understood in both.
*/
QMap<QString, QString> CableTypeList::headerAliasMap()
{
	QMap<QString, QString> map;

	for (const QString &key : canonicalColumns())
	{
		map.insert(key.toLower(), key);
		const QString translated = translatedColumn(key);
		if (!translated.isEmpty()) {
			map.insert(translated.toLower(), key);
		}
			//The element information spells the same column another way
			//("Numéro d'article" for designation); both are accepted, so
			//that a file written with either wording reads the same.
		const QString info_key = QETInformation::translatedInfoKey(key);
		if (!info_key.isEmpty() && info_key != translated) {
			map.insert(info_key.toLower(), key);
		}
	}

		//Every element information is an acceptable column name, so that
		//columns copied over from the material file keep their meaning.
	const QStringList keys = QETInformation::elementInfoKeys();
	for (const QString &key : keys)
	{
		if (map.contains(key.toLower())) {
			continue;
		}
		map.insert(key.toLower(), key);
		const QString translated = QETInformation::translatedInfoKey(key);
		if (!translated.isEmpty()) {
			map.insert(translated.toLower(), key);
		}
	}

	return map;
}

/**
	@brief CableTypeList::canonicalColumn
	Turn one header cell into a canonical column name.
	@param header_cell
	@param already_used the names chosen for the previous cells, to keep
	the header free of duplicates
	@param alias_map lowercased alias -> canonical name
	@return
*/
QString CableTypeList::canonicalColumn(const QString &header_cell,
									   const QStringList &already_used,
									   const QMap<QString, QString> &alias_map)
{
	const QString raw = header_cell.trimmed();

	if (raw.isEmpty()) {
		//A nameless column keeps its data and its place, it just can't be
		//matched to anything.
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
	@brief CableTypeList::isMachineHeaderLine
	Tell whether the second line of a cable type file is the machine
	header QElectroTech writes under the label line : the canonical
	column names, kept so that a file written in one language is still
	understood in another one.
	@param label_line the first line of the file, the one the user reads
	@param candidate the line which may be the machine header
	@param alias_map lowercased alias -> canonical name
	@return true when candidate must not be read as a record
*/
bool CableTypeList::isMachineHeaderLine(const QStringList &label_line,
										const QStringList &candidate,
										const QMap<QString, QString> &alias_map)
{
	if (candidate.isEmpty() || candidate.size() != label_line.size()) {
		return false;
	}

		//At least one cell the program really knows : a line made only
		//of key shaped words nobody here gave a meaning to is far more
		//likely to be a cable than a header.
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
			//An unknown key is still a key : the column comes back as
			//it was written, its data is never thrown away.
		if (isMachineKey(cell)) {
			continue;
		}
		return false;
	}

	return has_known_column;
}

/**
	@brief CableTypeList::load
	Read a cable type file.
	@param path
	@param data receives the columns and the records
	@param error receives a message when the file can't be read
	@return true on success
*/
bool CableTypeList::load(const QString &path, CableTypeListData *data, QString *error)
{
	if (error) {
		error->clear();
	}
	if (!data) {
		if (error) {
			*error = QCoreApplication::translate("CableTypeList", "Aucun récepteur pour le fichier des types de câbles.");
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
	data->separator = MaterialList::detectSeparator(eol < 0 ? content : content.left(eol));

	QList<QStringList> rows = MaterialList::parseCsv(content, data->separator);
	if (rows.isEmpty()) {
		return true;
	}

	const QStringList header = rows.takeFirst();
	const QMap<QString, QString> alias_map = headerAliasMap();

		//The label line is followed by the machine header QElectroTech
		//writes itself : the canonical names, so that the file is read
		//the same way whatever the language it was written in.
	QStringList machine_line;
	if (!rows.isEmpty() && isMachineHeaderLine(header, rows.first(), alias_map)) {
		machine_line = rows.takeFirst();
	}

	QStringList used;
	for (int i = 0; i < header.size(); ++i)
	{
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
		CableTypeRecord record;
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
			//written back the way they were read.
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
	@brief CableTypeList::writeFile
	Write a whole cable type file atomically : a crash or a full disk
	never leaves a half written catalogue behind.
	@param path
	@param data
	@param error receives a message when the file can't be written
	@return true on success
*/
bool CableTypeList::writeFile(const QString &path, const CableTypeListData &data, QString *error)
{
	if (error) {
		error->clear();
	}

	QList<QStringList> rows;
	rows.reserve(data.records.size());
	for (const CableTypeRecord &record : data.records)
	{
		QStringList row;
		row.reserve(data.columns.size());
		for (const QString &column : data.columns) {
			row.append(record.value(column));
		}
		row.append(record.extra);
		rows.append(row);
	}

	QList<QStringList> header_lines;
	header_lines.append(translatedHeader(data.columns));
	header_lines.append(data.columns);
	const QByteArray content = MaterialList::serializeCsv(header_lines, rows, data.separator);

		//The folder may not be there yet: a fresh profile has no
		//documents folder, and a path typed into the settings may name a
		//folder which was never created. QSaveFile cannot write into a
		//folder which does not exist, so without this the whole write
		//fails and the message says nothing about the real reason.
	const QString folder = QFileInfo(path).absolutePath();
	if (!folder.isEmpty() && !QDir(folder).exists() && !QDir().mkpath(folder))
	{
		if (error) {
			*error = QCoreApplication::translate(
				"CableTypeList",
				"Cannot create the folder %1 for the cable types file.").arg(folder);
		}
		return false;
	}

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
	@brief CableTypeList::appendRecord
	Add one cable type at the end of a cable type file.
	The file is read again first: changes the user just saved from his
	spreadsheet are part of what gets rewritten, never dropped.
	@param path
	@param record
	@param error receives a message when the file can't be written
	@return true on success
*/
bool CableTypeList::appendRecord(const QString &path, const CableTypeRecord &record, QString *error)
{
	if (error) {
		error->clear();
	}

	CableTypeListData data;
	if (hasContent(path))
	{
		if (!load(path, &data, error)) {
			return false;
		}
		if (data.columns.isEmpty())
		{
			if (error) {
				*error = QCoreApplication::translate("CableTypeList", "Le fichier ne contient pas d'en-tête : colonnes manquantes.");
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
	@brief CableTypeList::updateRecord
	Rewrite one line of a cable type file, the way a spreadsheet saves a
	row he just edited: the line is found as it stands at this moment,
	replaced by what the user left, and the whole file is written again
	-- so the changes he saved from his spreadsheet in the meantime are
	part of it rather than being thrown away.
	@param path the file to write
	@param before the line as it stands in the file right now
	@param after the line as the user left it
	@param error receives a message when the file can't be written, or
	when the line he started from is no longer in it
	@return true on success
*/
bool CableTypeList::updateRecord(const QString &path,
								 const CableTypeRecord &before,
								 const CableTypeRecord &after,
								 QString *error)
{
	if (error) {
		error->clear();
	}

	CableTypeListData data;
	if (!load(path, &data, error)) {
		return false;
	}
	if (data.columns.isEmpty())
	{
		if (error) {
			*error = QCoreApplication::translate("CableTypeList", "Le fichier ne contient pas d'en-tête : colonnes manquantes.");
		}
		return false;
	}

	int row = -1;
	for (int i = 0; i < data.records.size(); ++i)
	{
		if (data.records.at(i) == before) {
			row = i;
			break;
		}
	}
	if (row < 0)
	{
			//The line he started from is gone: rewriting whatever stands
			//there now would overwrite somebody else's work.
		if (error) {
			*error = QCoreApplication::translate("CableTypeList",
												 "Cette entrée n'existe plus dans le fichier : elle a peut-être été modifiée entre-temps.");
		}
		return false;
	}

	data.records[row] = after;
	return writeFile(path, data, error);
}

/**
	@brief CableTypeList::createFile
	Create an empty cable type file holding only the header lines.
	@param path
	@param error receives a message when the file can't be written
	@return true on success
*/
bool CableTypeList::createFile(const QString &path, QString *error)
{
	if (error) {
		error->clear();
	}

	if (hasContent(path))
	{
		if (error) {
			*error = QCoreApplication::translate("CableTypeList", "Le fichier existe déjà et n'est pas vide.");
		}
		return false;
	}

	CableTypeListData data;
	data.columns = defaultColumns();
	data.separator = QLatin1Char(';');
	return writeFile(path, data, error);
}

/**
	@brief CableTypeList::isEmptyFile
	@param path
	@return true when no file, or an empty one, exists at path
*/
bool CableTypeList::isEmptyFile(const QString &path)
{
	return !hasContent(path);
}
