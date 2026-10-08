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
#include "wiringruleswarning.h"

#include <QApplication>
#include <QMessageBox>

/**
	@brief WiringRulesWarning::show
	Warn, when a wires-per-terminal rule is turned on (discussion #1158,
	review of #1272), that the rules are experimental: they count wires on
	a terminal as QElectroTech models them today, and would have to be
	redesigned if wires and conductors become separate objects. A plain
	warning, as the element editor gives for unnamed terminals: OK goes on.
	@param parent
*/
void WiringRulesWarning::show(QWidget *parent)
{
	QMessageBox::warning(
		parent,
		QApplication::translate("WiringRulesWarning", "Avertissement"),
		QApplication::translate(
			"WiringRulesWarning",
			"<b>Les règles de conducteurs par borne sont une fonction expérimentale.</b>"
			"<br><br>Elles comptent les conducteurs tels que QElectroTech les représente "
			"aujourd'hui. Elles pourraient changer, et vos réglages devoir être refaits, "
			"si les fils et les conducteurs deviennent des objets distincts dans une "
			"version future."
			"<br><br>Toutes ces règles peuvent être désactivées dans Configurer "
			"QElectroTech > Général."));
}
