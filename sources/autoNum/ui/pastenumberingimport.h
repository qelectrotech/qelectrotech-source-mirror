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
#ifndef PASTENUMBERINGIMPORT_H
#define PASTENUMBERINGIMPORT_H

#include "../elementautonumschemecommand.h"
#include "../../diagramcontent.h"

#include <QCoreApplication>
#include <QList>
#include <QUndoCommand>

class Diagram;
class QWidget;

/**
	@brief The PasteNumberingImport class
	Pastes elements copied from another project, which may follow element
	numberings this project does not have.

	Such an element would be left with a formula which names no numbering
	here, and with a label nothing explains. So the user is asked, once
	for all the numberings missing, whether to import them. Imported, they
	number the pasted elements from their first number (a number of the other
	project means nothing here); not imported, the elements are pasted
	without a numbering.

	The import and the paste are one undo step.

	A paste which brings more than itself (the cable lines of the copy,
	for instance, which are no part of a DiagramContent) hands those
	commands over through @p extra: they are pushed inside the very
	same macro, so one Ctrl+Z still takes the whole of it back.
*/
class PasteNumberingImport
{
	Q_DECLARE_TR_FUNCTIONS(PasteNumberingImport)

	public:
		static void push(QWidget *parent,
						 Diagram *diagram,
						 const DiagramContent &content,
						 const QList<ElementAutoNumSchemeCommand::Scheme> &copied,
						 const QList<QUndoCommand *> &extra = QList<QUndoCommand *>());

		static QList<ElementAutoNumSchemeCommand::Scheme> copiedBy(const QDomDocument &clipboard);
};

#endif // PASTENUMBERINGIMPORT_H
