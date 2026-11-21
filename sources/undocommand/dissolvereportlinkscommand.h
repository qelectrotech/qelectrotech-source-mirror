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
#ifndef DISSOLVEREPORTLINKSCOMMAND_H
#define DISSOLVEREPORTLINKSCOMMAND_H

#include <QList>
#include <QPair>
#include <QPointer>
#include <QUndoCommand>

class Conductor;
class Element;

/**
	@brief The DissolveReportLinksCommand class
	Take away the folio report links of the potentials of some wires, the
	first thing a name written over those potentials does
	(Conductor::setPropertyToPotential): the wires a folio report joins
	are one wire, but they need not carry the same name. Undo puts the
	links back as they were; the chaining makes them again from the names
	either way (QETProject::reportKeySourceChanged).
	Made as a child of the undo step of the edit, and the first of its
	children, so that the links go before the names are written and come
	back after the names are taken away again.
*/
class DissolveReportLinksCommand : public QUndoCommand
{
	public:
		DissolveReportLinksCommand(const QList<Conductor *> &conductors,
								   QUndoCommand *parent = nullptr);
		void redo() override;
		void undo() override;

	private:
		QList<QPair<QPointer<Element>, QPointer<Element>>> m_links;
};

#endif // DISSOLVEREPORTLINKSCOMMAND_H
