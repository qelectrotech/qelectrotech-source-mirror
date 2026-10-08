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
#include "counterwarning.h"

#include "../elementautonumschemecommand.h"
#include "../../qetmessagebox.h"

/**
	@brief CounterWarning::confirm
	@param title : the element numbering
	@param proposed : its definition, with the counter to give it
	@return true if the counter meets no number in use, or the user goes
	on; false if the user gives up (and, with nobody to ask, always)
*/
bool CounterWarning::confirm(QWidget *parent,
							 const QETProject *project,
							 const QString &title,
							 const NumerotationContext &proposed)
{
	const auto conflict = ElementAutoNumSchemeCommand::counterConflict(project, title, proposed);
	if (!conflict) {
		return true;
	}
	const auto answer = QET::QetMessageBox::question(
				parent,
				tr("Numbering counter"),
				tr("The next number would be %1, but %n elements of the “%2” numbering already "
				   "have an equal or higher number (up to %3).", "", conflict->count)
				.arg(conflict->counter).arg(title).arg(conflict->highest)
				+ tr("\n"
					 "New elements will skip numbers that are already taken: they will not "
					 "necessarily receive numbers starting from %1.\n"
					 "\n"
					 "Continue?").arg(conflict->counter),
				QMessageBox::Yes | QMessageBox::Cancel,
				QMessageBox::Cancel);
	return answer == QMessageBox::Yes;
}
