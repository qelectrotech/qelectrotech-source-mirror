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
#ifndef PLACERENAME_H
#define PLACERENAME_H

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUuid>

class Diagram;
class QETProject;
class QUndoCommand;
class StructureBoxItem;

/**
	@brief The PlaceRename namespace
	Renames a plant or a location in the whole project, in one undo step.
	It changes what the project stores (folio title blocks, structure boxes,
	the plant and location typed in a symbol, the defaults for new folios,
	the plant counters of the wire numbering) and nothing it works out from
	them.
*/
namespace PlaceRename
{
		/// What to rename: a plant (@a rename_location false), or the
		/// location @a location of @a plant
	struct Request
	{
		QString plant;
		QString location;
		QString new_name;
		bool rename_location = false;
	};

		/// What to do when the new plant already has wire counters
	enum class CounterPolicy
	{
		MergeHigher,  ///< the higher number of each counter
		KeepNew,      ///< the new plant's counters
		KeepOld       ///< the old plant's counters
	};

		/// What a rename would change
	struct Plan
	{
		QList<Diagram *> folios;            ///< title block holds the old name
		QList<StructureBoxItem *> boxes;
		QList<QUuid> symbols;               ///< hold the old name themselves
		bool defaults = false;              ///< the new-folio title block
		QStringList counter_schemes;        ///< numbering schemes with counters
		int count() const;
	};

	QString higherCounter(const QString &a, const QString &b);
	QStringList mergeCounter(const QStringList &old_plant, const QStringList &new_plant,
							 CounterPolicy policy);

	bool isValid(const Request &request, QString *why = nullptr);
	Plan collect(QETProject *project, const Request &request);
	bool counterClash(QETProject *project, const Request &request);
	bool targetExists(QETProject *project, const Request &request);
	QUndoCommand *command(QETProject *project, const Request &request,
						  CounterPolicy policy = CounterPolicy::MergeHigher);
}

#endif // PLACERENAME_H
