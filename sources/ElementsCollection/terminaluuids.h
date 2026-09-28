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
#ifndef TERMINALUUIDS_H
#define TERMINALUUIDS_H

#include <QUuid>

class QDomElement;

namespace TerminalUuids
{
	QUuid derived(qreal x, qreal y, int orientation, int occurrence = 0);
	int fillMissing(const QDomElement &collection_root);
	int fillMissingInDefinition(const QDomElement &definition);
	void keep(const QDomElement &old_element, QDomElement &new_element);
	void keepInDirectory(const QDomElement &old_directory,
						 QDomElement &new_directory);
}

#endif // TERMINALUUIDS_H
