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
#ifndef COUNTERWARNING_H
#define COUNTERWARNING_H

#include "../numerotationcontext.h"

#include <QCoreApplication>

class QETProject;
class QWidget;

/**
	@brief The CounterWarning class
	Asks before the counter of an element numbering is set to a number which
	elements of it have already: the next elements skip the numbers in use,
	so the next number is not the one asked for. The user may go on or give
	up.
*/
class CounterWarning
{
	Q_DECLARE_TR_FUNCTIONS(CounterWarning)

	public:
		static bool confirm(QWidget *parent,
							const QETProject *project,
							const QString &title,
							const NumerotationContext &proposed);
};

#endif // COUNTERWARNING_H
