// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef FORMULABUILDER_H
#define FORMULABUILDER_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>

/**
	@brief The FormulaBuilder namespace
	A small, read-only expression language for the columns of a table:
	"if the article is empty use the description", "quantity times two",
	"upper case of the label". An expression is a tree of nodes, each a map
	with an "op", the "args" it works on and, for a few, a "value"
	(a field, a text, a number), a "separator" or a "format".
	compile() turns the tree into one SQLite expression. A field must be
	one the caller lists, and a text or a number is written as a quoted
	literal, so the generated SQL never holds anything the user typed as
	SQL syntax.
*/
namespace FormulaBuilder
{
	/// A column the formula may read: what the user sees, and its SQL name
	struct Field
	{
		QString label;
		QString expression;
	};

	/// What the builder shows for one operation
	struct Function
	{
		QString id;
		QString category;
		QString label;
		QString help;
		int minimum = 0;   ///< fewest inputs
		int maximum = 0;   ///< most inputs
		QStringList arguments;   ///< what the first inputs are for
	};

	QList<Function> functions();
	Function function(const QString &id);

	QVariantMap field(const QString &expression);
	QVariantMap text(const QString &value);
	QVariantMap number(double value);
	QVariantMap operation(const QString &op, const QVariantList &arguments = {},
						  const QVariantMap &parameters = {});
	QVariantMap defaultNode(const QString &op, const QList<Field> &fields);

	QString compile(const QVariantMap &node, const QList<Field> &fields, QString *error = nullptr);
}

#endif // FORMULABUILDER_H
