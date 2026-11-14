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

			/// The place of a folio or device: =plant+location
		QString placeOf(const QString &plant, const QString &location)
		{
			QString place;
			if (!plant.isEmpty()) place += QLatin1Char('=') + plant;
			if (!location.isEmpty()) place += QLatin1Char('+') + location;
			return place;
		}

		QString placeOf(Diagram *folio)
		{
			return placeOf(folio->border_and_titleblock.plant().trimmed(),
						   folio->border_and_titleblock.locmach().trimmed());
		}
	}

	/**
		@return what a macro can ask, in the order it holds them, each once:
		its device labels (a label several symbols share is one question;
		folio reports name a signal, not a device), and the texts with a
		question mark.
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
		const QDomElement diagram = diagramOf(macro_root);
		const auto types = linkTypes(macro_root);
		for (const QDomElement &element : children(diagram.firstChildElement(QStringLiteral("elements")), "element")) {
			const QString link = types.value(pathOf(element.attribute(QStringLiteral("type"))));
			const bool report = link.contains(QLatin1String("report"));
			for (const QDomElement &info : children(element.firstChildElement(QStringLiteral("elementInformations")), "elementInformation")) {
				const QString name = info.attribute(QStringLiteral("name"));
				if (name == QLatin1String(LabelKey)) {
					if (report) {
						if (info.text().contains(QLatin1Char('?'))) add(Field::Text, info.text());
					} else {
						add(Field::Label, info.text());
					}
				} else if (name != QLatin1String(FormulaKey) && info.text().contains(QLatin1Char('?'))) {
					add(Field::Text, info.text());
				}
			}
			for (const QDomElement &dynamic : children(element.firstChildElement(QStringLiteral("dynamic_texts")), "dynamic_elmt_text")) {
				if (dynamic.attribute(QStringLiteral("text_from")) != QLatin1String("UserText")) continue;
				const QString text = dynamic.firstChildElement(QStringLiteral("text")).text();
				if (text.contains(QLatin1Char('?'))) add(Field::Text, text);
			}
		}
		for (const QDomElement &input : children(diagram.firstChildElement(QStringLiteral("inputs")), "input")) {
			const QString text = plain(input.attribute(QStringLiteral("text")));
			if (text.contains(QLatin1Char('?'))) add(Field::Text, text);
		}
		for (const QDomElement &conductor : children(diagram.firstChildElement(QStringLiteral("conductors")), "conductor")) {
			if (conductor.attribute(QStringLiteral("num")).contains(QLatin1Char('?')))
				add(Field::Text, conductor.attribute(QStringLiteral("num")));
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
		@brief propose
		Fills the value of each device label with the next one free on
		@p folio; the labels of the macro do not take each other's.
	*/
	void propose(QList<Field> &fields, Diagram *folio)
	{
		QSet<QString> taken;
		QString place;
		if (folio) {
			place = placeOf(folio);
			if (QETProject *project = folio->project()) {
				project->dataBase()->updateDB();
				QSqlQuery query = project->dataBase()->newQuery(QStringLiteral("SELECT label, plant, location FROM qet_devices"));
				while (query.next()) {
					const QString label = query.value(0).toString().trimmed();
					if (label.isEmpty()) continue;
					taken.insert(placeOf(query.value(1).toString().trimmed(),
										 query.value(2).toString().trimmed())
								 + QLatin1Char('-') + label);
				}
			}
		}
		for (Field &field : fields) {
			if (field.kind != Field::Label) continue;
			field.value = nextFree(field.saved, place, taken);
			taken.insert(place + QLatin1Char('-') + field.value);
		}
	}

	/**
		@brief apply
		Writes the values of @p fields into the diagram of a macro, before
		it is placed. A label changed by hand no longer follows a formula.
	*/
	void apply(QDomElement &diagram, const QList<Field> &fields)
	{
		QHash<QString, QString> labels, texts;
		for (const Field &field : fields) {
			if (field.value == field.saved) continue;
			(field.kind == Field::Label ? labels : texts).insert(field.saved, field.value);
		}
		if (labels.isEmpty() && texts.isEmpty()) return;
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
				} else if (texts.contains(saved)) {
					setText(info, texts.value(saved));
				}
			}
			if (relabelled && !formula.isNull()) infos.removeChild(formula);
			for (QDomElement &dynamic : children(element.firstChildElement(QStringLiteral("dynamic_texts")), "dynamic_elmt_text")) {
				if (dynamic.attribute(QStringLiteral("text_from")) != QLatin1String("UserText")) continue;
				QDomElement text = dynamic.firstChildElement(QStringLiteral("text"));
				if (texts.contains(text.text().trimmed())) setText(text, texts.value(text.text().trimmed()));
			}
		}
		for (QDomElement &input : children(diagram.firstChildElement(QStringLiteral("inputs")), "input")) {
			const QString saved = plain(input.attribute(QStringLiteral("text"))).trimmed();
			if (texts.contains(saved)) input.setAttribute(QStringLiteral("text"), escaped(texts.value(saved)));
		}
		for (QDomElement &conductor : children(diagram.firstChildElement(QStringLiteral("conductors")), "conductor")) {
			const QString saved = conductor.attribute(QStringLiteral("num")).trimmed();
			if (texts.contains(saved)) conductor.setAttribute(QStringLiteral("num"), texts.value(saved));
		}
	}
}
