/*
	Copyright 2006 The QElectroTech Team
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
#ifndef EDZSEVENZIP_H
#define EDZSEVENZIP_H

#include <QString>

/**
	Extract every entry of the 7-Zip archive at @a archivePath into @a destDir,
	using the bundled (public-domain) LZMA SDK — no external 7-Zip needed.
	@return true on success; on failure @a error explains why.
*/
bool sevenZipExtract(const QString &archivePath, const QString &destDir,
		     QString &error);

#endif // EDZSEVENZIP_H
