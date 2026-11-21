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
#ifndef PROJECTCHECK_H
#define PROJECTCHECK_H

#include <QCoreApplication>
#include <QGraphicsObject>
#include <QList>
#include <QPointer>
#include <QString>
#include <QUuid>
#include <QVector>

class QETProject;

/**
	@brief The ProjectCheck class
	The checks of a project, run one after the other for the Check panel
	(#1878 B1). Each check is the one QElectroTech already has; this only
	gives their findings one shape: a finding is a line of text and the
	places it is about, each of which can be shown on its folio.

	Duplicate labels and device types read the built folios: the caller
	builds the lazy ones first. The others read the project database only.
	A check reports; it changes nothing in the project.
*/
class ProjectCheck
{
	Q_DECLARE_TR_FUNCTIONS(ProjectCheck)

	public:
		enum class Kind {
			DuplicateLabels,   ///< DuplicateLabels::find()
			PotentialTypes,    ///< projectDataBase::potentialTypeMismatches()
			OpenWireEnds,      ///< projectDataBase::openWireEnds()
			UnwiredSymbols,    ///< symbols none of whose terminals has a wire
			DeviceTypes,       ///< DeviceTypeCheck::check()
			WiresPerTerminal,  ///< terminals with more wires than WiringRules::limit()
			TerminalNames      ///< symbols with two terminals of one name (TerminalNameCheck)
		};

			/// Where a finding is. One of item, element or wires says what
			/// to show; a place with none of them is a line of detail only.
		struct Place {
			QString text;                    ///< what is there: a label, a part
			QString folio;                   ///< "3 — Power", empty if not known
			QString cell;                    ///< "B4", empty if not known
			QPointer<QGraphicsObject> item;  ///< a built item
			QUuid element;                   ///< a symbol, when item is not given
			QUuid diagram;                   ///< the symbol's folio
			QList<QUuid> wires;              ///< wires, found when shown, on
											 ///< the folio diagram if given
			bool canShow() const {return item || !element.isNull() || !wires.isEmpty();}
		};

		struct Finding {
			QString text;
			QList<Place> places;
		};

		struct Result {
			Kind kind = Kind::DuplicateLabels;
				/// Why the check found nothing to check (switched off for
				/// the project, no limit set); empty when it ran
			QString note;
			QVector<Finding> findings;
		};

			/// Every check, in the order the panel lists them
		static QList<Kind> kinds();
		static QString title(Kind kind);
			/// Where the check's own window is, with its options
		static QString toolTip(Kind kind);
		static Result run(QETProject *project, Kind kind);

	private:
		static Result duplicateLabels(QETProject *project);
		static Result potentialTypes(QETProject *project);
		static Result openWireEnds(QETProject *project);
		static Result unwiredSymbols(QETProject *project);
		static Result deviceTypes(QETProject *project);
		static Result wiresPerTerminal(QETProject *project);
		static Result terminalNames(QETProject *project);
};

#endif // PROJECTCHECK_H
