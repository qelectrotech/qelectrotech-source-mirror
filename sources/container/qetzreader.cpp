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
#include "qetzreader.h"

#include "qetzip.h"

/**
	@brief QetzReader::read
	Read @p content, the bytes of a .qet or a .qetz, into @p document.
	For a .qetz, its entries, folio parts, pictures and format are kept.
	@return false, with @p error set, if it cannot be read
*/
bool QetzReader::read(const QByteArray &content, QDomDocument *document, QString *error)
{
	m_files.clear();
	m_folio_parts.clear();
	m_format = 0;
	bool read = false;
	if (QetContainer::isZip(content)) {
		QList<QetZip::Entry> entries;
		read = QetZip::fromBytes(content, &entries, error)
				&& QetContainer::join(entries, document, error, &m_folio_parts,
									  &m_pictures, &m_format);
		if (read)
			for (const QetZip::Entry &entry : std::as_const(entries))
				m_files.insert(entry.name, entry.data);
	} else {
		read = QetContainer::readDocument(content, document, error);
	}
	if (!read) {
		m_pictures.clear();
		m_folio_parts.clear();
	}
	return read;
}
