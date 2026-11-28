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
#ifndef ELEMENTFINGERPRINT_H
#define ELEMENTFINGERPRINT_H

#include <QDomDocument>
#include <QString>

/**
	@brief The fingerprint an imported element carries of its own content.

	The part import writes it into every element (a hidden element
	information "import_hash": SHA-256 over the definition without that
	entry, independent of attribute order and indentation). A later import
	overwrites the file only while the fingerprint still matches, i.e.
	nobody edited the element since; an element edited by hand or by a
	script, or one without fingerprint, is kept unless the import is forced.
*/
namespace ElementFingerprint
{
	enum State {
		Missing,	///< no fingerprint: not written by the import, or by an older one
		Unedited,	///< the content is what the import wrote
		Edited		///< changed since the import
	};

	/// the fingerprint of @p element, without its own entry
	QString compute(const QDomDocument &element);
	State state(const QDomDocument &element);
	/// stores the fingerprint of @p element in it, replacing an old one
	void stamp(QDomDocument &element);
	/**
		Takes over from @p old what diagrams and the user refer to: the
		element uuid, the names, the element information (entries only
		@p element has are added) and per terminal name the terminal uuid,
		so that "Update element" keeps the wires connected.
	*/
	void keepIdentity(QDomDocument &element, const QDomDocument &old);
}

#endif // ELEMENTFINGERPRINT_H
