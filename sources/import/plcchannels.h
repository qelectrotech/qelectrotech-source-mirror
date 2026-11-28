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
#ifndef PLCCHANNELS_H
#define PLCCHANNELS_H

#include "eplandxf/eplandxfreader.h"

#include <QDomDocument>
#include <QStringList>
#include <QVector>

/**
	@brief The channel table of an imported PLC module.

	An element built from the manufacturer's EPLAN data writes the
	connection description next to each terminal ("DQ0" beside terminal 1,
	"M" beside terminal 9). From those the module gets its channel table
	(plcMasterData/plcIOs): per channel its kind, its name as address and
	its terminals -- the signal terminal and the reference terminal right
	after it. With the table the element is a PLC module (link_type
	master, type plc), and a generic PLC channel can be put on any of its
	channels (Selection properties > PLC).

	An EPLAN macro names its channels itself (function definition and PLC
	properties of each connection point, EplanPlcPoint): where it does,
	that table is used instead of the rules.
*/
namespace PlcChannels
{
	struct Channel {
		QString type;		///< ElementData::plcIOTypeToString(), e.g. "sortie_digitale"
		QString name;		///< "DQ0"
		QStringList terminals;	///< signal terminal first
	};

	/// the channels of an element; empty without channel texts
	QVector<Channel> fromElement(const QDomDocument &element);

	/**
		The channels an EPLAN macro defines for @p pins: per channel its
		signal terminals, then the other terminals of the same channel
		(P20407) and its supply terminals, four at most.
		@param terminal_names the names the element gives the pins
		(EplanElementBuilder::terminalNames())
	*/
	QVector<Channel> fromMacro(const QVector<EplanDxfPin> &pins, const QStringList &terminal_names);

	/// makes @p element a PLC module with @p channels; false if it
	/// already is exactly that
	bool setChannels(QDomDocument &element, const QVector<Channel> &channels);

	/**
		For every folder a written element lies in: the channel table of
		the element with the most channels (at least two) goes into every
		element of that folder that has all channel terminals (overview,
		device picture, black box), except mounting layouts and slaves.
		@param table the channels where the part data names them (EPLAN
		function definitions); empty: read from the texts of the elements
		Elements in @p from_macro already hold the table of their macro
		and are left as they are; their tables, joined, come before the
		rules. Only elements the import wrote and nobody edited since
		(ElementFingerprint) are changed.
		@return one line per element changed
	*/
	QStringList applyToFolders(const QStringList &written_paths,
				   const QStringList &from_macro = QStringList(),
				   const QVector<Channel> &table = {});
}

#endif // PLCCHANNELS_H
