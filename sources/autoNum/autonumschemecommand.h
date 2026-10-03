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
#ifndef AUTONUMSCHEMECOMMAND_H
#define AUTONUMSCHEMECOMMAND_H

#include "numerotationcontext.h"

#include <QCoreApplication>
#include <QList>
#include <QPointer>
#include <QUndoCommand>

#include <optional>

class Diagram;
class QETProject;

/**
	@brief The AutoNumSchemeCommand class
	Undoable creation, edition (rename and/or new definition) and removal of
	one conductor or folio numbering scheme of a project.

	A folio refers to the scheme it follows by its title, as the project file
	has always had it (the title of the conductor numbering a folio reads,
	the title of the folio numbering its title block follows). So a rename
	moves those references with it, in the same undo step, and a scheme which
	a folio still follows cannot be removed: the folio would be left reading
	a numbering which does not exist.

	A folio numbering is applied once: the number is written into the folio
	field of the title block, in place of %autonum, and the title block keeps
	the name of the numbering it came from. So a folio really depends on a
	folio numbering only while its folio field still holds %autonum, and
	only then can the numbering not be removed; every title block which
	names it has its name moved by a rename.

	Conductors keep the formula they were numbered with, and the numbers
	folios show are worked out from the scheme when they are drawn: a new
	definition changes what is numbered from then on, nothing is renumbered.

	Counter changes made by numbering something are not edits of the scheme
	and do not come here.
*/
class AutoNumSchemeCommand : public QUndoCommand
{
	Q_DECLARE_TR_FUNCTIONS(AutoNumSchemeCommand)

	public:
		enum class Kind { Conductor, Folio };

		struct Scheme
		{
			QString title;
			NumerotationContext context;
		};

		static QStringList titles(const QETProject *project, Kind kind);
		static bool contains(const QETProject *project, Kind kind, const QString &title);
		static NumerotationContext contextOf(const QETProject *project, Kind kind, const QString &title);
		static QString nameClash(const QETProject *project, Kind kind,
								 const QString &name, const QString &ignored_title = QString());
		static QString nameProblem(const QETProject *project, Kind kind,
								   const QString &name, const QString &ignored_title = QString());
		static QList<Diagram *> usersOf(const QETProject *project, Kind kind,
										const QString &title, bool references = false);

		static AutoNumSchemeCommand *create(QETProject *project, Kind kind,
											const QString &title,
											const NumerotationContext &context,
											bool make_current = false);
		static AutoNumSchemeCommand *edit(QETProject *project, Kind kind,
										  const QString &old_title,
										  const QString &new_title,
										  const NumerotationContext &context,
										  bool make_current = false);
		static AutoNumSchemeCommand *remove(QETProject *project, Kind kind,
											const QString &title);

		void undo() override;
		void redo() override;

	private:
		AutoNumSchemeCommand(QETProject *project, Kind kind);
		void rename(const QString &from, const QString &to);
		void announce(bool added, bool removed);

		QETProject *m_project = nullptr;
		Kind m_kind;
		std::optional<Scheme> m_before;
		std::optional<Scheme> m_after;
		QString m_current_before;
		QString m_current_after;
		QList<QPointer<Diagram>> m_users;   ///< the folios which follow the scheme being renamed
};

#endif // AUTONUMSCHEMECOMMAND_H
