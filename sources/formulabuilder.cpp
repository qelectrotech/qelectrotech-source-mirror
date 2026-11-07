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
#include "formulabuilder.h"

#include <QObject>

#include <cmath>

namespace FormulaBuilder
{
	namespace
	{
		const int max_depth = 40;

		/// 'text' with its quotes doubled
		QString quoted(QString value)
		{
			value.replace('\'', QStringLiteral("''"));
			return '\'' + value + '\'';
		}

		/// @p sql as text, an absent value being the empty text
		QString asText(const QString &sql)
		{
			return QStringLiteral("COALESCE(CAST((%1) AS TEXT),'')").arg(sql);
		}

		/// @p sql as a number, a comma being read as a decimal point
		QString asNumber(const QString &sql)
		{
			return QStringLiteral("CAST(REPLACE(TRIM(CAST((%1) AS TEXT)),',','.') AS REAL)").arg(sql);
		}

		bool isCondition(const QString &op)
		{
			return QStringList{"eq", "ne", "gt", "ge", "lt", "le", "between", "empty",
							   "notempty", "contains", "starts", "ends", "and", "or", "not"}.contains(op);
		}

		/**
			Whether @p node gives a number. SQLite does arithmetic in REAL even
			on text columns, so a calculation has to be compared as a number
			(Quantity = '2' is equal to 1 * 2), while two plain texts stay texts
			(002 is not 2).
		*/
		bool givesNumber(const QVariantMap &node, int depth = 0)
		{
			if (depth > max_depth) return false;
			const QString op = node.value("op").toString();
			if (QStringList{"number", "add", "subtract", "multiply", "divide", "round"}.contains(op)
				|| isCondition(op))
				return true;
			const QVariantList args = node.value("args").toList();
			const auto numberOrNull = [&](int index) {
				const QVariantMap child = args.value(index).toMap();
				return child.value("op") == "null" || givesNumber(child, depth + 1);
			};
			if (op == "if" && args.size() == 3) return numberOrNull(1) && numberOrNull(2);
			if (op == "fallback" && args.size() == 2) return numberOrNull(0) && numberOrNull(1);
			return false;
		}
	}

	QList<Function> functions()
	{
		const QString values = QObject::tr("Values");
		const QString comparisons = QObject::tr("Comparisons");
		const QString conditions = QObject::tr("Text conditions");
		const QString logic = QObject::tr("Logic");
		const QString texts = QObject::tr("Text functions");
		const QString numbers = QObject::tr("Numbers");
		return {
			{"field", values, QObject::tr("A field"), QObject::tr("Choose a column of this table's source."), 0, 0, {}},
			{"text", values, QObject::tr("Fixed text"), QObject::tr("Enter ordinary text. Quotes and other punctuation are handled for you."), 0, 0, {}},
			{"number", values, QObject::tr("A number"), QObject::tr("Enter a number."), 0, 0, {}},
			{"null", values, QObject::tr("Nothing"), QObject::tr("No value at all. To check for missing or blank text, use Is empty."), 0, 0, {}},
			{"eq", comparisons, QObject::tr("Equals"), QObject::tr("Compare two values. If one of them is a number, they are compared as numbers."), 2, 2, {QObject::tr("Value"), QObject::tr("Equals")}},
			{"ne", comparisons, QObject::tr("Differs from"), QObject::tr("True when the two values differ."), 2, 2, {QObject::tr("Value"), QObject::tr("Differs from")}},
			{"gt", comparisons, QObject::tr("Greater than"), QObject::tr("Compare numbers."), 2, 2, {QObject::tr("Number"), QObject::tr("Greater than")}},
			{"ge", comparisons, QObject::tr("At least"), QObject::tr("Compare numbers. Equal counts."), 2, 2, {QObject::tr("Number"), QObject::tr("At least")}},
			{"lt", comparisons, QObject::tr("Smaller than"), QObject::tr("Compare numbers."), 2, 2, {QObject::tr("Number"), QObject::tr("Smaller than")}},
			{"le", comparisons, QObject::tr("At most"), QObject::tr("Compare numbers. Equal counts."), 2, 2, {QObject::tr("Number"), QObject::tr("At most")}},
			{"between", comparisons, QObject::tr("Between two numbers"), QObject::tr("The two limits count as inside."), 3, 3, {QObject::tr("Number"), QObject::tr("Lower limit"), QObject::tr("Upper limit")}},
			{"empty", conditions, QObject::tr("Is empty"), QObject::tr("True for nothing, empty text or text of only spaces."), 1, 1, {QObject::tr("Value")}},
			{"notempty", conditions, QObject::tr("Is not empty"), QObject::tr("True when there is something besides spaces."), 1, 1, {QObject::tr("Value")}},
			{"contains", conditions, QObject::tr("Contains"), QObject::tr("True when the text holds the other text, whatever the case."), 2, 2, {QObject::tr("Text"), QObject::tr("Contains")}},
			{"starts", conditions, QObject::tr("Starts with"), QObject::tr("True when the text begins with the other text, whatever the case."), 2, 2, {QObject::tr("Text"), QObject::tr("Starts with")}},
			{"ends", conditions, QObject::tr("Ends with"), QObject::tr("True when the text ends with the other text, whatever the case."), 2, 2, {QObject::tr("Text"), QObject::tr("Ends with")}},
			{"if", logic, QObject::tr("If... then... else"), QObject::tr("Use the second value when the condition is true, the third otherwise."), 3, 3, {QObject::tr("If"), QObject::tr("Then use"), QObject::tr("Otherwise use")}},
			{"and", logic, QObject::tr("All of"), QObject::tr("True when every condition is true."), 2, 99, {QObject::tr("Condition 1"), QObject::tr("Condition 2")}},
			{"or", logic, QObject::tr("Any of"), QObject::tr("True when at least one condition is true."), 2, 99, {QObject::tr("Condition 1"), QObject::tr("Condition 2")}},
			{"not", logic, QObject::tr("Not"), QObject::tr("True when the condition is false."), 1, 1, {QObject::tr("Condition")}},
			{"fallback", texts, QObject::tr("If empty, use another value"), QObject::tr("Use the first value unless it is empty, then use the second."), 2, 2, {QObject::tr("Preferred value"), QObject::tr("Otherwise use")}},
			{"join", texts, QObject::tr("Join values"), QObject::tr("Put the values one after the other with the separator between them."), 2, 99, {QObject::tr("Value 1"), QObject::tr("Value 2")}},
			{"replace", texts, QObject::tr("Replace text"), QObject::tr("Replace every occurrence of the text to find."), 3, 3, {QObject::tr("Text"), QObject::tr("Find"), QObject::tr("Replace with")}},
			{"upper", texts, QObject::tr("UPPER CASE"), QObject::tr("Turn the text into capitals."), 1, 1, {QObject::tr("Text")}},
			{"lower", texts, QObject::tr("lower case"), QObject::tr("Turn the text into small letters."), 1, 1, {QObject::tr("Text")}},
			{"trim", texts, QObject::tr("Remove surrounding spaces"), QObject::tr("Remove the spaces before and after the text."), 1, 1, {QObject::tr("Text")}},
			{"add", numbers, QObject::tr("Add"), QObject::tr("Add two numbers."), 2, 2, {QObject::tr("Number"), QObject::tr("Added to")}},
			{"subtract", numbers, QObject::tr("Subtract"), QObject::tr("Take the second number from the first."), 2, 2, {QObject::tr("Number"), QObject::tr("Minus")}},
			{"multiply", numbers, QObject::tr("Multiply"), QObject::tr("Multiply two numbers."), 2, 2, {QObject::tr("Number"), QObject::tr("Times")}},
			{"divide", numbers, QObject::tr("Divide"), QObject::tr("Divide by the second number. Dividing by zero gives nothing, not an error."), 2, 2, {QObject::tr("Number"), QObject::tr("Divided by")}},
			{"round", numbers, QObject::tr("Round"), QObject::tr("Round to a number of decimal places."), 2, 2, {QObject::tr("Number"), QObject::tr("Decimal places")}}
		};
	}

	/// @return the operation @p id, an empty one (no id) if there is none
	Function function(const QString &id)
	{
		const auto all = functions();
		for (const Function &f : all)
			if (f.id == id) return f;
		return {};
	}

	QVariantMap field(const QString &expression)
	{
		return {{"op", "field"}, {"value", expression}};
	}

	QVariantMap text(const QString &value)
	{
		return {{"op", "text"}, {"value", value}};
	}

	QVariantMap number(double value)
	{
		return {{"op", "number"}, {"value", value}};
	}

	QVariantMap operation(const QString &op, const QVariantList &arguments, const QVariantMap &parameters)
	{
		QVariantMap node = parameters;
		node["op"] = op;
		node["args"] = arguments;
		return node;
	}

	/**
		@brief FormulaBuilder::defaultNode
		A node for @p op that compiles as it stands, its inputs filled with
		the first of @p fields: what the builder starts from when the user
		picks an operation.
	*/
	QVariantMap defaultNode(const QString &op, const QList<Field> &fields)
	{
		const QVariantMap first = fields.isEmpty() ? text(QString()) : field(fields.first().expression);
		QVariantMap node = operation(op);
		QVariantList args;
		for (int i = 0 ; i < function(op).minimum ; ++i)
			args << first;
		if (op == "field") node["value"] = fields.isEmpty() ? QString() : fields.first().expression;
		if (op == "text") node["value"] = QString();
		if (op == "number") node["value"] = 1;
		if (op == "join") node["separator"] = QStringLiteral(" ");
		if (op == "if") args = {operation("empty", {first}), text(QString()), first};
		if (op == "and" || op == "or") args = {operation("notempty", {first}), operation("notempty", {first})};
		if (op == "not") args = {operation("notempty", {first})};
		if (QStringList{"eq", "ne", "contains", "starts", "ends"}.contains(op)) args = {first, text(QString())};
		if (QStringList{"gt", "ge", "lt", "le", "add", "subtract", "multiply", "divide"}.contains(op)) args = {first, number(1)};
		if (op == "between") args = {first, number(0), number(100)};
		if (op == "replace") args = {first, text(QString()), text(QString())};
		if (op == "round") args = {first, number(2)};
		node["args"] = args;
		return node;
	}

	namespace
	{
		QString compileNode(const QVariantMap &n, const QList<Field> &fields, QString *error, int depth)
		{
			const auto fail = [error](const QString &why) {
				if (error) *error = why;
				return QString();
			};
			if (depth > max_depth)
				return fail(QObject::tr("The formula is nested too deeply (at most %1 levels).").arg(max_depth));
			const QString op = n.value("op").toString();
			const Function f = function(op);
			if (f.id.isEmpty())
				return fail(QObject::tr("Choose an operation."));
			const QVariantList arguments = n.value("args").toList();
			if (arguments.size() < f.minimum || arguments.size() > f.maximum)
				return fail(QObject::tr("%1 needs %2 to %3 inputs.").arg(f.label).arg(f.minimum).arg(f.maximum));

			if (op == "field") {
				const QString expression = n.value("value").toString();
				for (const Field &candidate : fields)
					if (!expression.isEmpty() && candidate.expression == expression)
						return '(' + expression + ')';
				return fail(QObject::tr("This field is not available in this table. Choose one from the list."));
			}
			if (op == "text") return quoted(n.value("value").toString());
			if (op == "number") {
				bool ok = false;
				const double value = n.value("value").toDouble(&ok);
				if (!ok || !std::isfinite(value)) return fail(QObject::tr("Enter a number."));
				return QString::number(value, 'g', 16);
			}
			if (op == "null") return QStringLiteral("NULL");

			QStringList a;
			for (const QVariant &child : arguments) {
				const QString sql = compileNode(child.toMap(), fields, error, depth + 1);
				if (sql.isEmpty()) return QString();   // the child said why
				a << sql;
			}
			const auto t = [&](int i) { return asText(a.value(i)); };
			const auto m = [&](int i) { return asNumber(a.value(i)); };

			if (op == "eq" || op == "ne") {
				const bool numeric = givesNumber(arguments[0].toMap()) || givesNumber(arguments[1].toMap());
				return '(' + (numeric ? m(0) : t(0)) + (op == "eq" ? " = " : " <> ") + (numeric ? m(1) : t(1)) + ')';
			}
			if (QStringList{"gt", "ge", "lt", "le"}.contains(op)) {
				const QMap<QString, QString> signs{{"gt", ">"}, {"ge", ">="}, {"lt", "<"}, {"le", "<="}};
				return '(' + m(0) + ' ' + signs.value(op) + ' ' + m(1) + ')';
			}
			if (op == "between") return '(' + m(0) + " BETWEEN " + m(1) + " AND " + m(2) + ')';
			if (op == "empty" || op == "notempty")
				return "(TRIM(" + t(0) + ") " + (op == "empty" ? "=" : "<>") + " '')";
			if (op == "contains") return "(INSTR(LOWER(" + t(0) + "),LOWER(" + t(1) + ")) > 0)";
			if (op == "starts")
				return "(SUBSTR(LOWER(" + t(0) + "),1,LENGTH(" + t(1) + ")) = LOWER(" + t(1) + "))";
			if (op == "ends")
				return "(SUBSTR(LOWER(" + t(0) + "),LENGTH(" + t(0) + ")-LENGTH(" + t(1) + ")+1) = LOWER(" + t(1) + "))";
			if (op == "if")
				return "(CASE WHEN COALESCE((" + a[0] + "),0) THEN (" + a[1] + ") ELSE (" + a[2] + ") END)";
			if (op == "and" || op == "or") {
				QStringList terms;
				for (const QString &sql : std::as_const(a))
					terms << "COALESCE((" + sql + "),0)";
				return '(' + terms.join(op == "and" ? " AND " : " OR ") + ')';
			}
			if (op == "not") return "(NOT COALESCE((" + a[0] + "),0))";
			if (op == "fallback")
				return "(CASE WHEN TRIM(" + t(0) + ") = '' THEN (" + a[1] + ") ELSE (" + a[0] + ") END)";
			if (op == "join") {
				QStringList parts;
				for (int i = 0 ; i < a.size() ; ++i)
					parts << t(i);
				return '(' + parts.join(" || " + quoted(n.value("separator", " ").toString()) + " || ") + ')';
			}
			if (op == "replace") return "REPLACE(" + t(0) + "," + t(1) + "," + t(2) + ")";
			if (op == "upper" || op == "lower" || op == "trim") return op.toUpper() + '(' + t(0) + ')';
			if (QStringList{"add", "subtract", "multiply", "divide"}.contains(op)) {
				const QMap<QString, QString> signs{{"add", "+"}, {"subtract", "-"}, {"multiply", "*"}, {"divide", "/"}};
				return '(' + m(0) + ' ' + signs.value(op) + ' '
						+ (op == "divide" ? "NULLIF(" + m(1) + ",0)" : m(1)) + ')';
			}
			if (op == "round") return "ROUND(" + m(0) + ",CAST((" + a[1] + ") AS INTEGER))";
			return fail(QObject::tr("This operation is not supported."));
		}
	}

	/**
		@brief FormulaBuilder::compile
		@param node the expression
		@param fields the columns it may read
		@param error receives why it cannot be compiled, if @p error is given
		@return the SQLite expression, or an empty string if @p node is not valid
	*/
	QString compile(const QVariantMap &node, const QList<Field> &fields, QString *error)
	{
		return compileNode(node, fields, error, 0);
	}
}
