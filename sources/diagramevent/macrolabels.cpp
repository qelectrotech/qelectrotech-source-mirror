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
#include "macrolabels.h"

#include "../bordertitleblock.h"
#include "../dataBase/projectdatabase.h"
#include "../diagram.h"
#include "../qetproject.h"

#include <QRegularExpression>
#include <QSet>
#include <QSqlQuery>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>

namespace MacroLabels
{
	namespace
	{
		const char *const LabelKey = "label";
		const char *const FormulaKey = "formula";

		QList<QDomElement> children(const QDomElement &parent, const char *tag)
		{
			QList<QDomElement> list;
			for (QDomElement e = parent.firstChildElement(QLatin1String(tag)) ; !e.isNull()
				 ; e = e.nextSiblingElement(QLatin1String(tag)))
				list << e;
			return list;
		}

		QDomElement diagramOf(const QDomElement &macro_root)
		{
			return macro_root.firstChildElement(QStringLiteral("diagram_content"))
					.firstChildElement(QStringLiteral("diagram"));
		}

			/// The link type of each element of the macro, by its path
		QHash<QString, QString> linkTypes(const QDomElement &macro_root)
		{
			QHash<QString, QString> types;
			for (const QDomElement &e : children(macro_root.firstChildElement(QStringLiteral("collection")), "element"))
				types.insert(e.attribute(QStringLiteral("path")),
							 e.firstChildElement(QStringLiteral("definition")).attribute(QStringLiteral("link_type")));
			return types;
		}

		QString pathOf(const QString &type)
		{
			const int separator = type.indexOf(QLatin1String("://"));
			return separator < 0 ? type : type.mid(separator + 3);
		}

			/// The text of a folio's free text, stored as HTML
		QString plain(const QString &text)
		{
			if (!text.contains(QLatin1Char('<')) && !text.contains(QLatin1Char('&'))) return text;
			return QTextDocumentFragment::fromHtml(text).toPlainText();
		}

		QString escaped(const QString &text)
		{
			return QString(text).replace(QLatin1Char('&'), QStringLiteral("&amp;"))
					.replace(QLatin1Char('<'), QStringLiteral("&lt;"))
					.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
		}

		void setText(QDomElement &element, const QString &text)
		{
			while (element.hasChildNodes()) element.removeChild(element.firstChild());
			element.appendChild(element.ownerDocument().createTextNode(text));
		}

			/// The texts of a wire a macro can ask for
		const char *const ConductorTexts[] = {
			"num", "function", "tension_protocol",
			"conductor_color", "conductor_section", "bus"};

			/// A question mark followed by a name, ?motor_kw, at the start
			/// of a word: not the ? of K?A, Ok?Yes or a link's ?id=5
		const QRegularExpression &namedValue()
		{
			static const QRegularExpression re(QStringLiteral(
				"(?<![^\\s(\\[{\"'=:/])\\?([\\p{L}_][\\p{L}\\p{N}_]*+)(?!=)"));
			return re;
		}

			/// Whether @p text still holds a question mark once its
			/// named values are out
		bool hasBareMark(const QString &text)
		{
			return QString(text).remove(namedValue())
					.contains(QLatin1Char('?'));
		}

			/// @p html with each ?name of @p values replaced, in the format
			/// of the name's first letter: the rest of the text keeps its own
		QString htmlWithValues(const QString &html,
							   const QHash<QString, QString> &values)
		{
			QTextDocument document;
			document.setHtml(html);
			bool changed = false;
			QTextCursor found(&document);
			while (!(found = document.find(namedValue(), found)).isNull()) {
				if (!values.contains(found.selectedText())) continue;
				QTextCursor first(&document);
				first.setPosition(found.selectionStart() + 1);
				found.insertText(values.value(found.selectedText()),
								 first.charFormat());
				changed = true;
			}
			return changed ? document.toHtml() : html;
		}

			/// The place of a folio or device: =plant+location
		QString placeOf(const QString &plant, const QString &location)
		{
			QString place;
			if (!plant.isEmpty()) place += QLatin1Char('=') + plant;
			if (!location.isEmpty()) place += QLatin1Char('+') + location;
			return place;
		}
	}

	/**
		@return whether @p text still asks for an answer: it holds a ?name
		or a question mark of its own. The test fields() makes of each text
		of a macro, for a project to find what was left unanswered.
	*/
	bool holdsPrompt(const QString &text)
	{
		if (!text.contains(QLatin1Char('?'))) return false;
		const QString shown = plain(text);
		return !namedValues(shown).isEmpty() || hasBareMark(shown);
	}

	/**
		@return the named values @p text asks for, each once and written
		as the macro holds them: "?kw kW at ?kw" gives ?kw
	*/
	QStringList namedValues(const QString &text)
	{
		QStringList names;
		auto it = namedValue().globalMatch(text);
		while (it.hasNext()) {
			const QString name = it.next().captured(0);
			if (!names.contains(name)) names << name;
		}
		return names;
	}

	/**
		@return @p text with each ?name of @p values replaced by its value;
		a name @p values does not hold stays as it is
	*/
	QString withValues(const QString &text,
					   const QHash<QString, QString> &values)
	{
		if (values.isEmpty() || !text.contains(QLatin1Char('?'))) return text;
		QString result;
		qsizetype from = 0;
		auto it = namedValue().globalMatch(text);
		while (it.hasNext()) {
			const auto match = it.next();
			if (!values.contains(match.captured(0))) continue;
			result += text.mid(from, match.capturedStart(0) - from)
					+ values.value(match.captured(0));
			from = match.capturedEnd(0);
		}
		return result + text.mid(from);
	}

	/**
		@return what a macro can ask, in the order it holds them, each once:
		its device labels (a label several symbols share is one question;
		folio reports name a signal, not a device), each ?name its texts
		hold, and the texts with a question mark of their own.
	*/
	QList<Field> fields(const QDomElement &macro_root)
	{
		QList<Field> list;
		QSet<QPair<int, QString>> known;
		const auto add = [&](Field::Kind kind, const QString &value) {
			const QString saved = value.trimmed();
			if (saved.isEmpty() || known.contains({kind, saved})) return;
			known.insert({kind, saved});
			Field field;
			field.kind = kind;
			field.saved = saved;
			field.value = saved;
			list << field;
		};
		const auto addText = [&](const QString &text) {
			if (!text.contains(QLatin1Char('?'))) return;
			for (const QString &name : namedValues(text))
				add(Field::Value, name);
			if (hasBareMark(text)) add(Field::Text, text);
		};
		const QDomElement diagram = diagramOf(macro_root);
		const auto types = linkTypes(macro_root);
		for (const QDomElement &element : children(diagram.firstChildElement(QStringLiteral("elements")), "element")) {
			const QString link = types.value(pathOf(element.attribute(QStringLiteral("type"))));
			const bool report = link.contains(QLatin1String("report"));
			for (const QDomElement &info : children(element.firstChildElement(QStringLiteral("elementInformations")), "elementInformation")) {
				const QString name = info.attribute(QStringLiteral("name"));
				if (name == QLatin1String(LabelKey)) {
					if (report) {
						addText(info.text());
					} else {
						add(Field::Label, info.text());
					}
				} else if (name != QLatin1String(FormulaKey)) {
					addText(info.text());
				}
			}
			for (const QDomElement &dynamic : children(element.firstChildElement(QStringLiteral("dynamic_texts")), "dynamic_elmt_text")) {
				if (dynamic.attribute(QStringLiteral("text_from")) != QLatin1String("UserText")) continue;
				addText(dynamic.firstChildElement(QStringLiteral("text"))
						.text());
			}
		}
		for (const QDomElement &input : children(diagram.firstChildElement(QStringLiteral("inputs")), "input")) {
			addText(plain(input.attribute(QStringLiteral("text"))));
		}
		for (const QDomElement &conductor : children(diagram.firstChildElement(QStringLiteral("conductors")), "conductor")) {
			for (const char *attribute : ConductorTexts)
				addText(conductor.attribute(QLatin1String(attribute)));
		}
		return list;
	}

	/**
		@return the label to give for @p label in @p place (=plant+location)
		so that its designation is free in @p taken: itself when it is;
		else its last number counted up; a question mark is a number to find.
	*/
	QString nextFree(const QString &label, const QString &place, const QSet<QString> &taken)
	{
		const auto isFree = [&](const QString &candidate) {
			return !taken.contains(place + QLatin1Char('-') + candidate);
		};
		const int mark = label.indexOf(QLatin1Char('?'));
		if (mark >= 0) {
			for (int n = 1 ; ; ++n) {
				QString candidate = label;
				candidate.replace(mark, 1, QString::number(n));
				if (isFree(candidate)) return candidate;
			}
		}
		if (isFree(label)) return label;
		static const QRegularExpression last_number(QStringLiteral("(\\d+)(\\D*)$"));
		const auto match = last_number.match(label);
		if (!match.hasMatch()) {
			for (int n = 1 ; ; ++n)
				if (isFree(label + QString::number(n))) return label + QString::number(n);
		}
		const QString head = label.left(match.capturedStart(1));
		const QString tail = match.captured(2);
		for (qlonglong n = match.captured(1).toLongLong() + 1 ; ; ++n) {
			const QString candidate = head + QString::number(n) + tail;
			if (isFree(candidate)) return candidate;
		}
	}

	/**
		@return the place of @p folio: =plant+location
	*/
	QString placeOf(Diagram *folio)
	{
		return placeOf(folio->border_and_titleblock.plant().trimmed(),
					   folio->border_and_titleblock.locmach().trimmed());
	}

	/**
		@return the designations (=plant+location-label) the devices of
		@p folio's project hold. A label typed with its sign ("-K1") is
		the same designation as "K1"; the ones typed so are also put in
		@p typed_with_sign, without it, when given.
	*/
	QSet<QString> taken(Diagram *folio, QSet<QString> *typed_with_sign)
	{
		QSet<QString> taken;
		if (!folio || !folio->project()) return taken;
		QETProject *project = folio->project();
		project->dataBase()->updateDB();
		QSqlQuery query = project->dataBase()->newQuery(QStringLiteral("SELECT label, plant, location FROM qet_devices"));
		while (query.next()) {
			QString label = query.value(0).toString().trimmed();
			if (label.startsWith(QLatin1Char('-'))) {
				label.remove(0, 1);
				if (typed_with_sign) typed_with_sign->insert(label);
			}
			if (label.isEmpty()) continue;
			taken.insert(placeOf(query.value(1).toString().trimmed(),
								 query.value(2).toString().trimmed())
						 + QLatin1Char('-') + label);
		}
		return taken;
	}

	/**
		@brief propose
		Fills the value of each device label with the next one free on
		@p folio; the labels of the macro do not take each other's.
	*/
	void propose(QList<Field> &fields, Diagram *folio)
	{
		QSet<QString> taken_designations;
		QString place;
		if (folio) {
			place = placeOf(folio);
			taken_designations = taken(folio);
		}
		for (Field &field : fields) {
			if (field.kind != Field::Label) continue;
			field.value = nextFree(field.saved, place, taken_designations);
			taken_designations.insert(place + QLatin1Char('-') + field.value);
		}
	}

	/**
		@brief apply
		Writes the values of @p fields into the diagram of a macro, before
		it is placed. A label changed by hand no longer follows a formula.
	*/
	void apply(QDomElement &diagram, const QList<Field> &fields)
	{
		QHash<QString, QString> labels, texts, values;
		QSet<QString> device_labels;
		for (const Field &field : fields) {
			if (field.kind == Field::Label) device_labels.insert(field.saved);
			if (field.value == field.saved) continue;
			(field.kind == Field::Label ? labels
				: field.kind == Field::Text ? texts : values)
					.insert(field.saved, field.value);
		}
		if (labels.isEmpty() && texts.isEmpty() && values.isEmpty()) return;
			//A whole text answered is written as typed; else its ?names
		const auto answered = [&](const QString &text) {
			const QString saved = text.trimmed();
			return texts.contains(saved) ? texts.value(saved)
										 : withValues(text, values);
		};
		const auto answer = [&](QDomElement &element) {
			const QString text = answered(element.text());
			if (text != element.text()) setText(element, text);
		};
		for (QDomElement &element : children(diagram.firstChildElement(QStringLiteral("elements")), "element")) {
			QDomElement infos = element.firstChildElement(QStringLiteral("elementInformations"));
			QDomElement formula;
			bool relabelled = false;
			for (QDomElement &info : children(infos, "elementInformation")) {
				const QString name = info.attribute(QStringLiteral("name"));
				const QString saved = info.text().trimmed();
				if (name == QLatin1String(FormulaKey)) {
					formula = info;
				} else if (name == QLatin1String(LabelKey) && labels.contains(saved)) {
					setText(info, labels.value(saved));
					relabelled = true;
				} else if (name != QLatin1String(LabelKey)
						   || !device_labels.contains(saved)
						   || texts.contains(saved)) {
						//Not a device label, which is numbered, never read
						//for ?names; or a report's text sharing its label
					answer(info);
				}
			}
			if (relabelled && !formula.isNull()) infos.removeChild(formula);
			for (QDomElement &dynamic : children(element.firstChildElement(QStringLiteral("dynamic_texts")), "dynamic_elmt_text")) {
				if (dynamic.attribute(QStringLiteral("text_from")) != QLatin1String("UserText")) continue;
				QDomElement text = dynamic.firstChildElement(QStringLiteral("text"));
				answer(text);
			}
		}
		for (QDomElement &input : children(diagram.firstChildElement(QStringLiteral("inputs")), "input")) {
				//Held as HTML: a whole text answered is written plain,
				//a ?name in the format it had
			const QString html = input.attribute(QStringLiteral("text"));
			const QString saved = plain(html).trimmed();
			const QString text = texts.contains(saved)
					? escaped(texts.value(saved))
					: htmlWithValues(html, values);
			if (text != html) input.setAttribute(QStringLiteral("text"), text);
		}
		for (QDomElement &conductor : children(diagram.firstChildElement(QStringLiteral("conductors")), "conductor")) {
			for (const char *attribute : ConductorTexts) {
				const QString saved =
						conductor.attribute(QLatin1String(attribute));
				const QString text = answered(saved);
				if (text == saved) continue;
				conductor.setAttribute(QLatin1String(attribute), text);
					//An answered number no longer follows a formula
				if (qstrcmp(attribute, "num") == 0)
					conductor.removeAttribute(QStringLiteral("formula"));
			}
		}
	}
}
