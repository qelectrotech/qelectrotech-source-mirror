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
#ifndef MATERIALLIST_H
#define MATERIALLIST_H

#include <QByteArray>
#include <QChar>
#include <QList>
#include <QMap>
#include <QSize>
#include <QString>
#include <QStringList>

/**
	@brief One line of the material file.

	Keys are column names : either a canonical element information key
	(description, manufacturer...) or the raw name of a column added by
	the user, which QElectroTech does not know about and therefore never
	Transfers to an element, but keeps intact when a row is appended.
*/
struct MaterialRecord
{
	QMap<QString, QString> values;

	QString value(const QString &column) const {return values.value(column);}
	void setValue(const QString &column, const QString &value) {values.insert(column, value);}
	bool operator==(const MaterialRecord &other) const {return values == other.values;}
};

/**
	@brief The whole content of a material file.

	Both the column names and the separator are read from the file rather
	than imposed, so a file edited in a spreadsheet keeps working : the
	separator is detected (;, tab or ,), unknown columns are preserved, and
	only the header cells that can be matched to a known element
	information are canonicalized.
*/
struct MaterialListData
{
	QStringList columns;
	QList<MaterialRecord> records;
	QChar separator = QLatin1Char(';');

	bool isEmpty() const {return columns.isEmpty() && records.isEmpty();}
};

/**
	@brief The MaterialList class reads and writes the material file (a CSV
	file listing purchasable articles) and maps its columns onto element
	informations.

	This is a pure data class : it does not know anything about widgets,
	so it stays usable from the tests and from the command line.
*/
class MaterialList
{
	public:
		//Settings
		static QString settingsKey();
		static QString configuredPath();
		static void setConfiguredPath(const QString &path);
		static QString defaultPath();
		static QString defaultFileName();

			//Geometry of the selection window, remembered from one opening
			//to the next : column widths are part of how the user reads his
			//own catalogue and must not be lost when the window closes.
		static QByteArray savedHeaderState();
		static void saveHeaderState(const QByteArray &state);
		static QSize savedDialogSize();
		static void saveDialogSize(const QSize &size);

		//Columns
		static QStringList defaultColumns();
		static QStringList columnsForBlock(int block);
		static QString elementInfoKey(const QString &column, int block);
		static QString translatedColumn(const QString &column);
		static QStringList translatedHeader(const QStringList &columns);

		//File io
		static bool load(const QString &path, MaterialListData *data, QString *error = nullptr);
		static bool writeFile(const QString &path, const MaterialListData &data, QString *error = nullptr);
		static bool appendRecord(const QString &path, const MaterialRecord &record, QString *error = nullptr);
		static bool createFile(const QString &path, QString *error = nullptr);
		static bool isEmptyFile(const QString &path);

		//Csv helpers, public only to keep them testable
		static QList<QStringList> parseCsv(const QString &content, QChar separator);
		static QByteArray serializeCsv(const QList<QStringList> &header_lines,
									   const QList<QStringList> &rows,
									   QChar separator);
		static QChar detectSeparator(const QString &first_line);

	private:
		static QString canonicalColumn(const QString &header_cell,
									   const QStringList &already_used,
									   const QMap<QString, QString> &alias_map);
		static QMap<QString, QString> headerAliasMap();
		static bool isMachineHeaderLine(const QStringList &label_line,
										const QStringList &candidate,
										const QMap<QString, QString> &alias_map);
};

#endif // MATERIALLIST_H
