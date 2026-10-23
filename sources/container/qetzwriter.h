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
#ifndef QETZWRITER_H
#define QETZWRITER_H

#include <QByteArray>
#include <QDomDocument>
#include <QHash>
#include <QString>

/**
	@brief The QetzWriter namespace
	Writes a project document as a .qetz file.
*/
namespace QetzWriter
{
	bool write(const QDomDocument &project, const QString &path, QString *error = nullptr,
			   const QHash<QString, QByteArray> &kept_from = QHash<QString, QByteArray>());
}

#endif // QETZWRITER_H
