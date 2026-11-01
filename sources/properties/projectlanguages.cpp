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
#include "projectlanguages.h"

#include <QDomDocument>
#include <QRegularExpression>
#include <QSet>
#include <QStringDecoder>
#include <algorithm>

/**
	@brief ProjectLanguages::isEmpty
	@return true if there is no language and no phrase: nothing is saved
*/
bool ProjectLanguages::isEmpty() const
{
	return m_languages.isEmpty() && m_phrases.isEmpty();
}

bool ProjectLanguages::operator==(const ProjectLanguages &other) const
{
	return m_source == other.m_source && m_active == other.m_active
			&& m_languages == other.m_languages && m_phrases == other.m_phrases;
}

/**
	@brief ProjectLanguages::validCode
	@return true for a language code such as "de" or "pt-BR"
*/
bool ProjectLanguages::validCode(const QString &code)
{
	static const QRegularExpression re(QStringLiteral("^[A-Za-z]{2,8}(-[A-Za-z0-9]{1,8})*$"));
	return re.match(code).hasMatch();
}

void ProjectLanguages::setSourceLanguage(const QString &code)
{
	if (validCode(code)) m_source = code;
}

/**
	@brief ProjectLanguages::activeLanguage
	@return the language the folios are shown in: the source language unless
	another one is chosen and still exists
*/
QString ProjectLanguages::activeLanguage() const
{
	return m_languages.contains(m_active) ? m_active : m_source;
}

void ProjectLanguages::setActiveLanguage(const QString &code)
{
	m_active = m_languages.contains(code) ? code : QString();
}

/**
	@brief ProjectLanguages::languages
	@return the source language first, then the added ones
*/
QStringList ProjectLanguages::languages() const
{
	return QStringList{m_source} + m_languages;
}

bool ProjectLanguages::addLanguage(const QString &code)
{
	if (!validCode(code) || code == m_source || m_languages.contains(code)) return false;
	m_languages << code;
	return true;
}

void ProjectLanguages::removeLanguage(const QString &code)
{
	if (!m_languages.removeOne(code)) return;
	for (auto &texts : m_phrases) texts.remove(code);
	if (m_active == code) m_active.clear();
}

QStringList ProjectLanguages::phrases() const
{
	return m_phrases.keys();
}

void ProjectLanguages::addPhrase(const QString &source)
{
	if (isTranslatable(source)) m_phrases[source];
}

QString ProjectLanguages::translation(const QString &source, const QString &language) const
{
	return m_phrases.value(source).value(language);
}

void ProjectLanguages::setTranslation(const QString &source, const QString &language, const QString &text)
{
	if (!m_languages.contains(language) || !m_phrases.contains(source)) return;
	if (text.isEmpty()) m_phrases[source].remove(language);
	else m_phrases[source][language] = text;
}

/**
	@brief ProjectLanguages::translate
	@param source a text as drawn on the folio
	@return what to show in the active language: the source text when the
	language is the source or has no words for it
*/
QString ProjectLanguages::translate(const QString &source) const
{
	if (m_phrases.isEmpty()) return source;
	const QString text = m_phrases.value(source).value(activeLanguage());
	return text.isEmpty() ? source : text;
}

/**
	@brief ProjectLanguages::isTranslatable
	Words, not references: "Motor protection" yes; "-K1", "24 VDC", "/12.3" no.
*/
bool ProjectLanguages::isTranslatable(const QString &text)
{
	const QString s = text.trimmed();
	if (s.isEmpty() || s.size() > 2000 || !s.contains(QRegularExpression(QStringLiteral("\\p{L}"))))
		return false;
	static const QRegularExpression identifier(QStringLiteral("^-?[A-Z0-9][A-Z0-9+./:_-]*$"));
	if (identifier.match(s).hasMatch()) return false;
	QString compact = s;
	compact.remove(QRegularExpression(QStringLiteral("\\s+")));
	if (compact.contains(QRegularExpression(QStringLiteral("[0-9+./:_-]"))) && identifier.match(compact).hasMatch())
		return false;
	static const QRegularExpression value(QStringLiteral("^[0-9.,]+\\s*(V|VAC|VDC|A|Hz|kW|W|mm|m)$"),
										  QRegularExpression::CaseInsensitiveOption);
	return !s.startsWith(QLatin1Char('/')) && !value.match(s).hasMatch();
}

/**
	@brief ProjectLanguages::placeholders
	@return the %{variable} style fields of @p text, sorted: a translation
	has to keep the same ones
*/
QStringList ProjectLanguages::placeholders(const QString &text)
{
	static const QRegularExpression re(QStringLiteral("%\\{[^}]+\\}|%[A-Za-z_][A-Za-z0-9_]*|\\{[A-Za-z_][A-Za-z0-9_]*\\}"));
	QStringList result;
	auto matches = re.globalMatch(text);
	while (matches.hasNext()) result << matches.next().captured();
	std::sort(result.begin(), result.end());
	return result;
}

/**
	@brief ProjectLanguages::toXml
	<languages source="en" active="de"><language code="de"/>
	<phrase source="Motor"><text language="de">Motor</text></phrase></languages>
*/
QDomElement ProjectLanguages::toXml(QDomDocument &document) const
{
	QDomElement root = document.createElement(QStringLiteral("languages"));
	root.setAttribute(QStringLiteral("source"), m_source);
	if (!m_active.isEmpty() && m_languages.contains(m_active))
		root.setAttribute(QStringLiteral("active"), m_active);
	for (const QString &code : m_languages) {
		QDomElement language = document.createElement(QStringLiteral("language"));
		language.setAttribute(QStringLiteral("code"), code);
		root.appendChild(language);
	}
	for (auto it = m_phrases.cbegin() ; it != m_phrases.cend() ; ++it) {
		QDomElement phrase = document.createElement(QStringLiteral("phrase"));
		phrase.setAttribute(QStringLiteral("source"), it.key());
		for (const QString &code : m_languages) {
			const QString text = it.value().value(code);
			if (text.isEmpty()) continue;
			QDomElement t = document.createElement(QStringLiteral("text"));
			t.setAttribute(QStringLiteral("language"), code);
			t.setAttribute(QStringLiteral("value"), text);
			phrase.appendChild(t);
		}
		root.appendChild(phrase);
	}
	return root;
}

/**
	@brief ProjectLanguages::fromXml
	@param element a <languages> element; anything else gives no languages
*/
ProjectLanguages ProjectLanguages::fromXml(const QDomElement &element)
{
	ProjectLanguages result;
	if (element.tagName() != QLatin1String("languages")) return result;
	result.setSourceLanguage(element.attribute(QStringLiteral("source")));
	for (QDomElement e = element.firstChildElement(QStringLiteral("language")) ; !e.isNull()
		 ; e = e.nextSiblingElement(QStringLiteral("language")))
		result.addLanguage(e.attribute(QStringLiteral("code")));
	result.setActiveLanguage(element.attribute(QStringLiteral("active")));
	for (QDomElement p = element.firstChildElement(QStringLiteral("phrase")) ; !p.isNull()
		 ; p = p.nextSiblingElement(QStringLiteral("phrase"))) {
		const QString source = p.attribute(QStringLiteral("source"));
		if (!isTranslatable(source)) continue;
		result.addPhrase(source);
		for (QDomElement t = p.firstChildElement(QStringLiteral("text")) ; !t.isNull()
			 ; t = t.nextSiblingElement(QStringLiteral("text")))
			result.setTranslation(source, t.attribute(QStringLiteral("language")),
								  t.attribute(QStringLiteral("value")));
	}
	return result;
}

static QString csvCell(QString value)
{
	value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
	return QLatin1Char('"') + value + QLatin1Char('"');
}

/**
	@brief ProjectLanguages::exportCsv
	@return a UTF-8 CSV with a BOM (for a spreadsheet): Source, then one
	column per added language
*/
QByteArray ProjectLanguages::exportCsv() const
{
	QString out;
	const auto line = [&out](const QStringList &cells) {
		QStringList quoted;
		for (const QString &cell : cells) quoted << csvCell(cell);
		out += quoted.join(QLatin1Char(',')) + QStringLiteral("\r\n");
	};
	line(QStringList{QStringLiteral("Source")} + m_languages);
	for (auto it = m_phrases.cbegin() ; it != m_phrases.cend() ; ++it) {
		QStringList row{it.key()};
		for (const QString &code : m_languages) row << it.value().value(code);
		line(row);
	}
	return QByteArray("\xEF\xBB\xBF") + out.toUtf8();
}

// RFC 4180, a semicolon accepted as the delimiter (spreadsheets in many locales)
static bool parseCsv(QString input, QList<QStringList> &rows, QString *error)
{
	if (input.startsWith(QChar(0xfeff))) input.remove(0, 1);
	const int first_end = input.indexOf(QRegularExpression(QStringLiteral("[\\r\\n]")));
	const QString first = input.left(first_end < 0 ? input.size() : first_end);
	const QChar delimiter = first.count(QLatin1Char(';')) > first.count(QLatin1Char(',')) ? QLatin1Char(';') : QLatin1Char(',');
	QStringList row;
	QString value;
	bool quoted = false, after_quote = false;
	const auto fail = [&](const QString &message) {
		if (error) *error = message;
		rows.clear();
		return false;
	};
	for (int i = 0 ; i < input.size() ; ++i) {
		const QChar c = input[i];
		if (quoted) {
			if (c == QLatin1Char('"')) {
				if (i + 1 < input.size() && input[i + 1] == QLatin1Char('"')) {value += c; ++i;}
				else {quoted = false; after_quote = true;}
			}
			else value += c;
			continue;
		}
		if (c == QLatin1Char('"')) {
			if (after_quote || !value.isEmpty()) return fail(QObject::tr("Unexpected quote in the CSV file."));
			quoted = true;
			continue;
		}
		if (c == delimiter) {row << value; value.clear(); after_quote = false; continue;}
		if (c == QLatin1Char('\r') || c == QLatin1Char('\n')) {
			if (c == QLatin1Char('\r') && i + 1 < input.size() && input[i + 1] == QLatin1Char('\n')) ++i;
			row << value;
			if (row.size() > 1 || !row.first().isEmpty()) rows << row;
			row.clear(); value.clear(); after_quote = false;
			continue;
		}
		if (after_quote) return fail(QObject::tr("Unexpected text after a quoted cell in the CSV file."));
		value += c;
	}
	if (quoted) return fail(QObject::tr("Unterminated quoted cell in the CSV file."));
	if (!value.isEmpty() || !row.isEmpty() || after_quote) {row << value; rows << row;}
	return true;
}

/**
	@brief ProjectLanguages::importCsv
	Fill in translations from a CSV as exportCsv() writes it. Rows whose
	source is not a known phrase, and translations that lose or add a
	%{field}, are skipped with a warning; empty cells change nothing.
*/
ProjectLanguages::CsvResult ProjectLanguages::importCsv(const QByteArray &bytes)
{
	CsvResult result;
	QStringDecoder decoder(QStringDecoder::Utf8);
	const QString decoded = decoder.decode(bytes);
	if (decoder.hasError()) {
		result.error = QObject::tr("The CSV file must be UTF-8.");
		return result;
	}
	QList<QStringList> rows;
	if (!parseCsv(decoded, rows, &result.error)) return result;
	if (rows.isEmpty()) {
		result.error = QObject::tr("The CSV file is empty.");
		return result;
	}
	const QStringList header = rows.takeFirst();
	if (header.value(0) != QLatin1String("Source")) {
		result.error = QObject::tr("The first column of the CSV file must be \"Source\".");
		return result;
	}
	QList<QPair<int, QString>> columns;
	for (int i = 1 ; i < header.size() ; ++i) {
		const QString code = header.at(i).trimmed();
		if (!validCode(code)) {
			result.warnings << QObject::tr("Ignored column \"%1\": not a language code.").arg(header.at(i));
			continue;
		}
		columns << qMakePair(i, code);
	}
	if (columns.isEmpty()) {
		result.error = QObject::tr("The CSV file has no language column.");
		return result;
	}
	ProjectLanguages next = *this;
	for (const auto &column : std::as_const(columns)) next.addLanguage(column.second);
	for (int n = 0 ; n < rows.size() ; ++n) {
		const QStringList &row = rows.at(n);
		if (row.size() != header.size()) {
			result.warnings << QObject::tr("Row %1 has the wrong number of cells and was skipped.").arg(n + 2);
			continue;
		}
		const QString source = row.first();
		if (!next.m_phrases.contains(source)) {
			result.warnings << QObject::tr("Row %1: \"%2\" is not a text of this project and was skipped.").arg(n + 2).arg(source);
			continue;
		}
		for (const auto &column : std::as_const(columns)) {
			const QString text = row.at(column.first);
			if (text.isEmpty() || text == next.translation(source, column.second)) continue;
			if (placeholders(text) != placeholders(source)) {
				result.warnings << QObject::tr("Row %1 (%2): the %{...} fields differ from the source; skipped.").arg(n + 2).arg(column.second);
				continue;
			}
			next.setTranslation(source, column.second, text);
			++result.imported;
		}
	}
	*this = next;
	result.ok = true;
	return result;
}
