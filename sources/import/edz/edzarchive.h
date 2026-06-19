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
#ifndef EDZARCHIVE_H
#define EDZARCHIVE_H

#include <QString>
#include <QScopedPointer>

class QTemporaryDir;

/**
	@brief Extracts an EPLAN Data Portal package (.edz) to a temporary folder.

	A .edz is a 7-Zip archive holding a part definition (part.xml), an EPLAN
	macro, a product image and metadata. EdzArchive unpacks it so the rest of the
	import pipeline can read the (portable) part.xml.

	The extraction backend sits behind a single private method so it can be
	swapped without touching callers: this M0 spike shells out to a 7-Zip CLI via
	QProcess; a later milestone replaces that with a bundled decompressor so no
	external tool is required at runtime.

	The extracted tree lives in a QTemporaryDir owned by this object and is
	removed when the EdzArchive is destroyed.
*/
class EdzArchive
{
	public:
		EdzArchive();
		~EdzArchive();

		bool extract(const QString &edz_path);

		QString extractedDir() const;
		QString partXmlPath() const;
		QString errorString() const;

	private:
		bool extractWithSevenZipCli(const QString &edz_path,
					    const QString &dest);
		static QString findSevenZip();

		QScopedPointer<QTemporaryDir> m_dir;
		QString m_error;
};

#endif // EDZARCHIVE_H
