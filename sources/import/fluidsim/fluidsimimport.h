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
#ifndef FLUIDSIMIMPORT_H
#define FLUIDSIMIMPORT_H

#include <QString>
#include <QStringList>

class QWidget;

/**
	@brief The FluidSimImport class
	File > Import FluidSIM circuit: turns FluidSIM 6 circuits (.circ) into a
	QElectroTech project, one folio per circuit, with the parts drawn anew
	(own graphics after ISO 1219-1 / IEC 60617, never the FluidSIM
	symbols), the wires, the coil-valve and relay-contact links, the
	visible values and the GRAFCET charts. The work is done by the Python 3
	program tools/fluidsim_import/fluidsim_import.py (installed as
	fluidsim_import/fluidsim_import.py), which writes the project through
	this QElectroTech.
*/
class FluidSimImport
{
	public:
			/// asks for the circuits and the target, runs the import;
			/// @return the written project, or empty when cancelled or failed
		static QString run(QWidget *parent);
			/// the import program: QET_FLUIDSIM_IMPORT, next to an installed
			/// QElectroTech or in the source tree
		static QString findScript(const QString &application_dir);
			/// the Python 3 interpreters to try, in this order
		static QStringList pythons();
			/// arguments for the Python that runs @a script
		static QStringList arguments(const QString &script, const QStringList &circuits,
					     const QString &target, const QString &qet_binary);
			/// the project the program reports as written ("done: …")
		static QString resultFromOutput(const QString &output);
};

#endif // FLUIDSIMIMPORT_H
