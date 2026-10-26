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
#ifndef QETZREADER_H
#define QETZREADER_H

#include "qetcontainer.h"

#include <QByteArray>
#include <QDomDocument>
#include <QHash>
#include <QString>
#include <QStringList>

/**
	@brief The QetzReader class
	Reads the bytes of a project file, a .qet or a .qetz, into its project
	document, and keeps what a .qetz holds beside that document for as long
	as the project is open: its entries (a save copies the parts of the
	folios not built from them), the part each folio was read from, its
	pictures and its format.
*/
class QetzReader
{
	public:
		bool read(const QByteArray &content, QDomDocument *document, QString *error = nullptr);

			/// The format the .qetz says it is; 0 for a .qet
		int format() const {return m_format;}
			/// The entries of the .qetz read, by name
		const QHash<QString, QByteArray> &files() const {return m_files;}
		QByteArray file(const QString &name) const {return m_files.value(name);}
			/// The part each folio was read from, in order
		const QStringList &folioParts() const {return m_folio_parts;}
			/// The picture in @p file, or an empty array when there is none
		QByteArray picture(const QString &file) const {return m_pictures.value(file);}

	private:
		QHash<QString, QByteArray> m_files;
		QStringList m_folio_parts;
		QetContainer::Pictures m_pictures;
		int m_format = 0;
};

#endif // QETZREADER_H
