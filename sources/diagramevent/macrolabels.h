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
#ifndef MACROLABELS_H
#define MACROLABELS_H

#include <QDomElement>
#include <QList>
#include <QString>

class Diagram;

/**
	@brief The MacroLabels namespace
	What is to be asked when a macro (.qetmak) is placed: the labels of its
	devices, and every text holding a question mark, the macro's way of
	saying "fill this in". A device label is proposed as the next one free
	in the place of the folio: a macro with K1 placed twice gives K1 and K2.
*/
namespace MacroLabels
{
	struct Field
	{
		enum Kind {Label, Text};
		Kind kind = Label;
		QString saved;      ///< what the macro holds
		QString value;      ///< what to put in its place
	};

	QList<Field> fields(const QDomElement &macro_root);
	QString nextFree(const QString &label, const QString &place, const QSet<QString> &taken);
	void propose(QList<Field> &fields, Diagram *folio);
	void apply(QDomElement &diagram, const QList<Field> &fields);
}

#endif // MACROLABELS_H
