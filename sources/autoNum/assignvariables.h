/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef ASSIGNVARIABLES_H
#define ASSIGNVARIABLES_H

#include <QString>
#include <QPointF>
#include <QStringList>

class Diagram;
class Element;

namespace autonum
{
	struct sequenceStruct {
			QStringList unit;
			QStringList unit_folio;
			QStringList ten;
			QStringList ten_folio;
			QStringList hundred;
			QStringList hundred_folio;
	};

	/**
	 * @brief The AssignVariables class
	 * This class assign variable of a formula string.
	 * Return the final string used to be displayed from a formula string.
	 */
	class AssignVariables
	{
		public:
			static QString formulaToLabel (QString formula, sequenceStruct &seqStruct, Diagram *diagram, Element *elmt = nullptr);

		private:
			AssignVariables(QString formula, sequenceStruct seqStruct , Diagram *diagram, Element *elmt = nullptr);
			void assignTitleBlockVar();
			void assignProjectVar();
			void assignSequence();

			Diagram *m_diagram  = nullptr;
			QString m_arg_formula;
			QString m_assigned_label;
			sequenceStruct m_seq_struct;
			Element *m_element = nullptr;
	};



}

#endif // ASSIGNVARIABLES_H
