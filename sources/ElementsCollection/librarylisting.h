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
#ifndef LIBRARYLISTING_H
#define LIBRARYLISTING_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

/**
	@brief The LibraryListing class
	A list of the symbols (.elmt) and macros (.qetmak) of a collection
	folder and its sub-folders, one row each, written as CSV: to see what a
	library holds, and which symbols lack terminal names or a class letter.
*/
class LibraryListing
{
	public:
		struct Row
		{
				/// in the language of the settings, else English
			QString name;
				/// relative to the listed folder, "/" between
			QString folder;
			QString file;
			bool macro = false;
				/// link_type of a symbol: simple, master, slave...
			QString type;
				/// the one placing gives it; empty for none
			QString class_letter;
			int terminals = 0;
				/// the named ones, in file order
			QStringList terminal_names;
			int unnamed_terminals = 0;
				/// of the symbols of a macro, in file order
			QStringList labels;
				/// why the file could not be read
			QString error;
		};

		QList<Row> scan(
				const QString &folder,
				const std::function<bool (int, int)> &progress = nullptr);
		static QByteArray toCsv(const QList<Row> &rows);
		static bool write(const QList<Row> &rows, const QString &file_name,
						  QString *error = nullptr);

	private:
		Row symbol(const QString &file_path);
		Row macro(const QString &file_path);
		QString classLetter(const QString &file_path);

			/// class letter per folder already looked up
		QHash<QString, QString> m_letters;
};

#endif // LIBRARYLISTING_H
