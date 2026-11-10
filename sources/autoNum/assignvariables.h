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
#ifndef ASSIGNVARIABLES_H
#define ASSIGNVARIABLES_H
#include <diagramcontext.h>
#include <diagramposition.h>
#include "numerotationcontext.h"

#include <QPointF>
#include <QString>
#include <QStringList>

class Conductor;
class Diagram;
class Element;
class ElementsLocation;
class QETProject;

namespace autonum
{
	class sequentialNumbers
	{
		public:
			sequentialNumbers();
			sequentialNumbers(const sequentialNumbers &other);
			~sequentialNumbers();

			sequentialNumbers &operator= (const sequentialNumbers &other);
			bool operator== (const sequentialNumbers &other) const;
			bool operator!= (const sequentialNumbers &other) const;

			QDomElement toXml(QDomDocument &document, const QString& tag_name = QString("sequentialNumbers")) const;
			void fromXml(const QDomElement &element);
			void clear();

			QStringList unit;
				/// Values of the cyclic (modulo) parts, referenced by %seqw_N.
			QStringList wrap;
			QStringList unit_folio;
			QStringList ten;
			QStringList ten_folio;
			QStringList hundred;
			QStringList hundred_folio;
			QStringList alpha;
	};

	/**
		@brief The FormulaContext struct
		Everything a formula's variables are read from: the folio it is on
		and, for an element's or a conductor's formula, that item.
		AssignVariables::formulaToLabel() fills one from a built folio; the
		project database fills one from the project's file.
	*/
	struct FormulaContext
	{
			/// border_and_titleblock.folio(), folioIndex(), folioTotal()...
		QString folio;
			/// the folio's title, %T
		QString folio_title;
		int folio_index = 0;
		int folio_total = 0;
		QString plant;
		QString locmach;
			/// the title block's additional fields and the project's properties
		DiagramContext title_block_fields;
		DiagramContext project_properties;
			/// an element's grid cell (%c, %l) and prefix
		bool has_element = false;
		DiagramPosition element_position;
		QString element_prefix;
			/// a conductor's %wf, %wv, %wc and %ws
		bool has_conductor = false;
		QString wire_function;
		QString wire_tension_protocol;
		QString wire_color;
		QString wire_section;
	};

	/**
		@brief The AssignVariables class
		This class assign variable of a formula string.
		Return the final string used to be displayed from a formula string.
	*/
	class AssignVariables
	{
		public:
			static QString formulaToLabel (QString formula, sequentialNumbers &seqStruct, Diagram *diagram, const Element *elmt = nullptr, const Conductor *cndr = nullptr);
			static QString formulaToLabel (QString formula, sequentialNumbers &seqStruct, const FormulaContext &context);
			static QString replaceVariable (const QString &formula, const DiagramContext &dc);
			static QString genericXref (const Element *element);

		private:
			AssignVariables(const QString& formula, const sequentialNumbers& seqStruct, const FormulaContext &context);
			void assignTitleBlockVar();
			void assignProjectVar();
			void assignSequence();

			const FormulaContext &m_context;
			QString m_arg_formula;
			QString m_assigned_label;
			sequentialNumbers m_seq_struct;
	};

	void setSequentialToList(QStringList &list, NumerotationContext &nc, const QString& type);
	void setFolioSequentialToHash(QStringList &list, QHash<QString, QStringList> &hash, const QString& autoNumName);
	void setSequential(const QString& label, autonum::sequentialNumbers &seqStruct, NumerotationContext &context, Diagram *diagram, const QString& hashKey);
	/**
		The numbers @p seq has just given on @p diagram under the name
		@p key: where that folio's own counter for the numbering is from
		now on, in the hash and in the rows the project saves it in.
		Sibling of setFolioSequentialToHash(), which only ever raises it.
	*/
	void storeFolioSequential(Diagram *diagram, const QString &key,
				  const sequentialNumbers &seq);
	QString numerotationContextToFormula(const NumerotationContext &nc);
	/**
		Put back every part of a numbering rule whose counter belongs to
		one single folio -- "Chiffre 1 - Folio" and its two longer forms
		-- to the beginning that folio is numbered from, which is the
		initial value written into the rule itself. The parts which count
		over the whole project are left exactly as they are: a folio
		starts again, the project does not.
		@param nc the rule to read
		@return the same rule, with its folio-bound counters restarted
	*/
	NumerotationContext resetFolioCounters(const NumerotationContext &nc);
	/**
		Where @p diagram numbers the folio-bound parts of @p nc under
		the name @p key: that folio's own counter -- the numbers it has
		actually handed out -- or, when it has handed out none yet, where
		the rule says a folio starts. A numbering which counts per folio
		has to read this rather than the project's counter, which holds
		wherever the last folio worked on got to.

		@param nc the rule to read, its counters as they are
		@param diagram the folio which is being numbered
		@param key the numbering's name in the folio's own counters
		@return @p nc with its folio-bound counters set for that folio
	*/
	NumerotationContext folioCounters(const NumerotationContext &nc,
					  Diagram *diagram,
					  const QString &key);
	NumerotationContext plantCounters(const NumerotationContext &nc,
					  QETProject *project,
					  const QString &plant,
					  const QString &scheme);
	NumerotationContext plantCounters(const NumerotationContext &nc,
					  const QStringList &given);
	QStringList plantCounterValues(const NumerotationContext &nc);
	sequentialNumbers sequenceOf(const NumerotationContext &nc);
	void storePlantCounters(QETProject *project, const QString &plant,
				const QString &scheme,
				const NumerotationContext &given);
	/**
		Set every part of a numbering rule which counts back to its very
		first value, so that numbering starts at the beginning again --
		which is what numbering a whole project over has to do, whatever
		the last number handed out happened to be. A part which counts
		from an initial value of its own goes back to that value, a plain
		number goes back to 1, a cyclic one to 0 and an alphabetic one to
		"a". The parts which do not count -- a fixed text, the number of
		the folio, the plant -- are left exactly as they are: they say
		what a number is made of, they are not the part which goes on.
		@param nc the rule to read
		@return the same rule, with its counters at their beginning
	*/
	NumerotationContext resetContextCounters(const NumerotationContext &nc);
	QString elementPrefixForLocation(const ElementsLocation &location);
}

Q_DECLARE_METATYPE(autonum::sequentialNumbers)

#endif // ASSIGNVARIABLES_H
