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
#ifndef ELEMENTAUTONUMSCHEMECOMMAND_H
#define ELEMENTAUTONUMSCHEMECOMMAND_H

#include "assignvariables.h"
#include "numerotationcontext.h"
#include "renumberelementscommand.h"

#include <QCoreApplication>
#include <QDomDocument>
#include <QDomElement>
#include <QList>
#include <QMap>
#include <QSet>
#include <QUndoCommand>
#include <QUuid>
#include <QVector>

#include <optional>

class Element;
class QETProject;

/**
	@brief The ElementAutoNumSchemeCommand class
	Undoable creation, edition (rename and/or new definition) and removal
	of one element numbering scheme of a project, together with what it
	does to the elements following the scheme.

	Elements follow a scheme by its uuid (QETInformation::ELMT_FORMULA_ID),
	so a rename touches no element. A new definition with a different
	formula rewrites the formula of every element following the scheme:
	when its sequential parts are the same (only the fixed text or the
	folio parts changed), each element keeps its numbers; otherwise the
	elements are numbered again, in folio and position order, as
	QETProject::renumberElementsBySchemeTitle() does.

	A scheme whose formula would change cannot be edited while an element
	with a frozen label follows it: that element would keep a label and a
	formula the scheme no longer defines (see editBlockedBy(); edit()
	refuses). A rename, or a change of the counter, touches no formula and
	is allowed. renumber() leaves a frozen element exactly as it is,
	label, formula and numbers, and does not give its label to another
	element: the numbering goes past it (see reservedLabels()).

	Counter changes made by placing elements are not edits of the scheme:
	they stay with SetAutoNumContextCommand.

	Build one with create(), edit() or remove(); each returns nullptr when
	the change is refused or changes nothing. Show changedElementCount()
	to the user before pushing it when it is not zero.
*/
class ElementAutoNumSchemeCommand : public QUndoCommand
{
	Q_DECLARE_TR_FUNCTIONS(ElementAutoNumSchemeCommand)

	public:
		struct Scheme
		{
			QString title;
			QUuid id;
			NumerotationContext context;
		};

		static ElementAutoNumSchemeCommand *create(
				QETProject *project,
				const QString &title,
				const NumerotationContext &context,
				const QUuid &id = QUuid(),
				bool make_current = true);
		static ElementAutoNumSchemeCommand *edit(
				QETProject *project,
				const QString &old_title,
				const QString &new_title,
				const NumerotationContext &context,
				bool make_current = true);
		static ElementAutoNumSchemeCommand *remove(
				QETProject *project,
				const QString &title);

		static QString nameProblem(const QETProject *project,
								   const QString &name,
								   const QString &ignored_title = QString());
		static bool sameSequentialParts(const QString &formula_a,
										const QString &formula_b);
		static NumerotationContext resetForRenumber(const NumerotationContext &context);
		static QVector<RenumberElementsCommand::ElementChange> renumberChanges(
				const QString &hash_key,
				const NumerotationContext &context,
				QVector<Element *> elements,
				const QString &formula,
				NumerotationContext *final_context,
				bool from_start = true,
				const QUuid &scheme_id = QUuid(),
				const QSet<QString> &reserved = QSet<QString>());
		static QSet<QString> reservedLabels(const QETProject *project,
											const QString &title,
											const QVector<Element *> &changing);
		static RenumberElementsCommand *renumber(QETProject *project,
												 const QString &title,
												 QVector<Element *> *frozen,
												 const QString &text);
		static QVector<Element *> frozenFollowers(const QETProject *project,
												  const QString &title);
		static QVector<Element *> editBlockedBy(const QETProject *project,
												const QString &title,
												const NumerotationContext &context);

		/// Why assign() leaves an element as it is
		enum class Skipped {
			None,
			FollowsAlready,   ///< already follows this scheme
			Frozen,           ///< its label is frozen
			OtherFormula,     ///< follows another scheme or has a formula of its own
			Linked            ///< slave or report: takes its label from its master
		};
		static Skipped skipReason(const QETProject *project,
								  const QString &title,
								  const Element *element);
		/// What assign() would do with each of the given elements
		struct AssignPlan
		{
			QVector<Element *> todo;          ///< given the scheme
			QVector<Element *> frozen;        ///< a frozen label: left alone unless overwrite
			QVector<Element *> otherFormula;  ///< follow or hold another formula: left alone unless overwrite
			int followsAlready = 0;           ///< follow this scheme already
			int linked = 0;                   ///< slave or report: take their label from their master
		};
		static AssignPlan assignPlan(const QETProject *project,
									 const QString &title,
									 const QVector<Element *> &elements);
		static RenumberElementsCommand *assign(
				QETProject *project,
				const QString &title,
				const QVector<Element *> &elements,
				bool overwrite,
				int *skipped = nullptr,
				QUndoCommand *parent = nullptr);

		/// What a numbered element's frozen state becomes
		enum class FreezeRule {
			Unfrozen,          ///< free: the user asked for this numbering
			NewElementPolicy   ///< as an element just placed: frozen if the folio or project freezes new ones
		};
		static RenumberElementsCommand *numberElements(
				QETProject *project,
				const QString &title,
				const QVector<Element *> &elements,
				const QString &text,
				FreezeRule freeze = FreezeRule::Unfrozen,
				QUndoCommand *parent = nullptr);

		static QString schemeForFormula(const QETProject *project, const QString &formula);
		static QString followedScheme(const QETProject *project, const DiagramContext &info);
		static void writeCopiedSchemes(QDomDocument &document,
									   QDomElement &root,
									   const QETProject *project,
									   const QVector<Element *> &copied);
		static QList<Scheme> copiedSchemes(const QDomElement &root);
		static QList<Scheme> missingForPaste(const QETProject *project,
											 const QList<Scheme> &copied,
											 const QList<Element *> &pasted);
		/// How the elements of a scheme are numbered by hand (see numberSupport())
		struct NumberSupport
		{
			bool supported = false;
			int partIndex = -1;     ///< the one number part of the definition
			QString type;           ///< "unit", "ten" or "hundred"
			int increase = 1;
		};
		struct GapRange
		{
			int from = 0;
			int to = 0;
		};
		/// A counter which would meet numbers already in use
		struct CounterConflict
		{
			int counter = 0;   ///< the next number it would hand out
			int highest = 0;   ///< the highest number in use
			int count = 0;     ///< how many elements have a number of at least counter
		};
		static std::optional<CounterConflict> counterConflict(const QETProject *project,
															  const QString &title,
															  const NumerotationContext &proposed);
		static QSet<QString> labelsHeldBesides(const QETProject *project, const Element *element);
		static NumberSupport numberSupport(const NumerotationContext &context);
		static std::optional<int> numberOf(const NumberSupport &support, const Element *element);
		static std::optional<int> numberOf(const NumberSupport &support,
										   const autonum::sequentialNumbers &seq);
		static QList<GapRange> gapRanges(const QETProject *project, const QString &title);
		static QList<int> freeNumbers(const QETProject *project, const QString &title,
									  const Element *element, int limit = 500);
		static QString labelForNumber(const QETProject *project, const QString &title,
									  Element *element, int number);
		static RenumberElementsCommand *assignNumber(QETProject *project,
													 Element *element,
													 int number,
													 QString *problem = nullptr,
													 QUndoCommand *parent = nullptr);

		static QMap<QString, QVector<Element *>> pastedSchemes(
				const QETProject *project,
				const QList<Element *> &elements);
		static void linkPasted(const QETProject *project,
							   const QList<Element *> &elements);
		static int numberPasted(QETProject *project,
								const QMap<QString, QVector<Element *>> &schemes,
								QUndoCommand *parent);

			/// Elements whose label and formula change
		int changedElementCount() const {return m_changes.size();}
		const QVector<RenumberElementsCommand::ElementChange> &changes() const {return m_changes;}
		std::optional<Scheme> before() const {return m_before;}
		std::optional<Scheme> after() const {return m_after;}

		void undo() override;
		void redo() override;

	private:
		explicit ElementAutoNumSchemeCommand(QETProject *project);

		QETProject *m_project = nullptr;
		std::optional<Scheme> m_before;
		std::optional<Scheme> m_after;
		QString m_current_before;
		QString m_current_after;
		QVector<RenumberElementsCommand::ElementChange> m_changes;
};

#endif // ELEMENTAUTONUMSCHEMECOMMAND_H
