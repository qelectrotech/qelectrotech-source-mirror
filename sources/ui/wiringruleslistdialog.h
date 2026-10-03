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
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef WIRINGRULESLISTDIALOG_H
#define WIRINGRULESLISTDIALOG_H

#include "../wiringrules.h"

#include <QDialog>

class QETProject;

/**
	@brief The WiringRulesListDialog class
	Lists the terminals of a project that already have more wires than its
	wires-per-terminal limit allows (discussion #1158): the limit only
	refuses new wires, so a drawing made before it was set can still have
	them. Read from the project database (terminal_wires_view), so no folio
	is walked. Read only: the list says where each terminal is (folio and
	cell), the user decides what to do about it.
*/
class WiringRulesListDialog : public QDialog
{
		Q_OBJECT

	public:
		WiringRulesListDialog(QETProject *project,
							  const WiringRules::Settings &rules,
							  QWidget *parent = nullptr);
};

#endif // WIRINGRULESLISTDIALOG_H
