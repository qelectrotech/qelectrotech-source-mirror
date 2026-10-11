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
#ifndef CABLETYPELIST_H
#define CABLETYPELIST_H

#include <QByteArray>
#include <QChar>
#include <QList>
#include <QMap>
#include <QSize>
#include <QString>
#include <QStringList>

/**
	@brief One line of the cable type file.

	Same shape as MaterialRecord : the keys are column names, either one
	QElectroTech knows (designation, manufacturer...) or one the user
	invented, which is never interpreted but never thrown away either.
*/
struct CableTypeRecord
{
	QMap<QString, QString> values;
		//Cells written after the last column of the header, kept as they
		//are, exactly like MaterialRecord::extra.
	QStringList extra;

	QString value(const QString &column) const {return values.value(column);}
	void setValue(const QString &column, const QString &value) {values.insert(column, value);}
	bool operator==(const CableTypeRecord &other) const {return values == other.values;}
};

/**
	@brief The whole content of a cable type file.

	The separator and the column names are read from the file rather than
	imposed, so a catalogue written in a spreadsheet keeps working.
*/
struct CableTypeListData
{
	QStringList columns;
	QList<CableTypeRecord> records;
	QChar separator = QLatin1Char(';');

	bool isEmpty() const {return columns.isEmpty() && records.isEmpty();}
};

/**
	@brief The CableTypeList class reads and writes the cable type file (a
	CSV file listing the cable kinds a project may use : their name,
	how many cores they have and the colour of each of them).

	It is the cable counterpart of MaterialList and follows it step by
	step : same settings handling, same two header lines (the labels the
	user reads, then the canonical names QElectroTech matches against),
	same lenient reading of a hand written file.

	This is a pure data class : it knows nothing about widgets, so it
	stays usable from the tests and from the command line.
*/
class CableTypeList
{
	public:
		//Settings
		static QString settingsKey();
		static QString configuredPath();
		static void setConfiguredPath(const QString &path);
		static QString defaultPath();
		static QString defaultFileName();

			//Geometry of the selection window, remembered from one opening
			//to the next, the same way MaterialList remembers its own.
		static QByteArray savedHeaderState();
		static void saveHeaderState(const QByteArray &state);
		static QSize savedDialogSize();
		static void saveDialogSize(const QSize &size);

		//Columns
		static QStringList defaultColumns();
		static QString translatedColumn(const QString &column);
		static QStringList translatedHeader(const QStringList &columns);

		//Cable semantics
		static QString designation(const CableTypeRecord &record);
		static int coreCount(const CableTypeRecord &record);
		static QStringList coreColors(const CableTypeRecord &record);

		//File io
		static bool load(const QString &path, CableTypeListData *data, QString *error = nullptr);
		static bool writeFile(const QString &path, const CableTypeListData &data, QString *error = nullptr);
		static bool appendRecord(const QString &path, const CableTypeRecord &record, QString *error = nullptr);
		/**
			@brief Rewrite one line of a cable type file, the way a
			spreadsheet saves a row he just edited.
			@param path the file to write
			@param before the line as it stands in the file right now
			@param after the line as the user left it
			@param error receives a message when the file can't be
			written, or when that line is no longer in it
			@return true on success
		*/
		static bool updateRecord(const QString &path,
								 const CableTypeRecord &before,
								 const CableTypeRecord &after,
								 QString *error = nullptr);
		static bool createFile(const QString &path, QString *error = nullptr);
		static bool isEmptyFile(const QString &path);

	private:
		static QMap<QString, QString> headerAliasMap();
		static QString canonicalColumn(const QString &header_cell,
									   const QStringList &already_used,
									   const QMap<QString, QString> &alias_map);
		static bool isMachineHeaderLine(const QStringList &label_line,
										const QStringList &candidate,
										const QMap<QString, QString> &alias_map);
};

#endif // CABLETYPELIST_H
