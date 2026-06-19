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
#ifndef EDZIMPORTER_H
#define EDZIMPORTER_H

#include <QString>

/**
	@brief Orchestrates importing an EPLAN .edz package as a QET element.

	Ties together EdzArchive (extract) -> EdzPart (parse) -> EdzElementBuilder
	(build) and writes the resulting .elmt into a destination collection folder.
	UI-decoupled so it stays unit-testable headless; the collection widget calls
	importToDirectory() and then refreshes the panel.
*/
class EdzImporter
{
	public:
		bool importToDirectory(const QString &edz_path,
				       const QString &dest_dir);

		QString writtenPath() const { return m_written; }
		QString errorString() const { return m_error; }

	private:
		QString m_written;
		QString m_error;
};

#endif // EDZIMPORTER_H
