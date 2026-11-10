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
#ifndef RENUMBERCONDUCTORSCOMMAND_H
#define RENUMBERCONDUCTORSCOMMAND_H

#include <QHash>
#include <QPointer>
#include <QUndoCommand>
#include <QVector>

#include <conductorproperties.h>
#include "assignvariables.h"
#include "numerotationcontext.h"

class Conductor;
class Diagram;
class QETProject;

/**
	@brief The RenumberConductorsCommand class
	Gives the wires of the chosen folios new numbers with a conductor
	numbering, as one undoable step: one number per potential, in folio
	order and, on a folio, from its top left. A potential with a frozen
	wire keeps its number, as does one that reaches a folio not chosen;
	their numbers are not given again. The numbering's counters start over:
	a unit once, a folio counter on each folio, a plant counter in each
	plant.
*/
class RenumberConductorsCommand final : public QUndoCommand
{
	public:
		struct Change
		{
			QPointer<Conductor> wire;
			ConductorProperties old_properties;
			ConductorProperties new_properties;
			autonum::sequentialNumbers old_sequence;
			autonum::sequentialNumbers new_sequence;
		};
			/// A potential left as it is, by one of its wires, and why
		struct Kept
		{
			QPointer<Conductor> wire;
			QString text;
			QString reason;
		};
		struct Plan
		{
			QVector<Change> changes;
			QVector<Kept> kept;
			int potentials = 0;
			NumerotationContext old_context;
			NumerotationContext new_context;
			QHash<QString, QStringList> old_plant_counters;
			QHash<QString, QStringList> new_plant_counters;
		};

		static Plan plan(QETProject *project, const QString &scheme,
						 const QList<Diagram *> &folios);

		RenumberConductorsCommand(QETProject *project, const QString &scheme,
								  Plan plan, const QString &text,
								  QUndoCommand *parent = nullptr);

		const Plan &planned() const {return m_plan;}
		void undo() override;
		void redo() override;

	private:
		void apply(bool use_new);

		QPointer<QETProject> m_project;
		QString m_scheme;
		Plan m_plan;
};

#endif // RENUMBERCONDUCTORSCOMMAND_H
