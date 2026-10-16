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
#ifndef PREVIOUSVERSIONS_H
#define PREVIOUSVERSIONS_H

#include <QString>
#include <QStringList>

/**
	@brief The PreviousVersions namespace
	The versions of a project file kept before it is saved over, as KiCad
	and AutoCAD keep theirs: a copy of the file as it was goes in a folder
	beside it, "<name>-backups", named by the time that version was saved
	("industrial 2026-10-16 08-30-12.qetz"), and only the newest are kept.
	The file is copied, never moved: a save that fails leaves it in place.
*/
namespace PreviousVersions
{
		/// How many versions to keep (Settings > General > Projects); 0 keeps none
	int count();
	void setCount(int count);
		/// The folder the versions of @p file_path go in
	QString folder(const QString &file_path);
		/// The versions kept of @p file_path, newest first
	QStringList kept(const QString &file_path);
		/// Copy @p file_path, about to be saved over, among its versions and
		/// keep only the @p count newest. Nothing is copied when the file does
		/// not exist, when @p count is 0, or when the newest version kept
		/// holds the same bytes. @return false, with @p error, when the copy
		/// could not be made: the save goes on all the same.
	bool keep(const QString &file_path, int count, QString *error = nullptr);
}

#endif // PREVIOUSVERSIONS_H
