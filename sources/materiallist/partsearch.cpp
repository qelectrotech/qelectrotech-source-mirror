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
#include "partsearch.h"

#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace
{
		/// The searched fields, with what a match in each adds to the score
	struct Field
	{
		const char *column;
		int bonus;
	};
	const Field Fields[] = {
		{"manufacturer_reference", 10},
		{"machine_manufacturer_reference", 10},
		{"designation", 6},
		{"description", 4},
		{"manufacturer", 3},
		{"device_type", 3},
		{"category", 3},
		{"supplier", 2},
	};

	const int ExactScore = 100;
	const int PrefixScore = 70;
	const int InsideScore = 40;
	const int FuzzyScore = 20;
	const int GlobScore = 60;

		/// A searched field: whole and in words
	struct Prepared
	{
		QString whole;
		QStringList tokens;
		int bonus;
	};

	QStringList tokensOf(const QString &folded)
	{
		static const QRegularExpression separators(QStringLiteral("[^\\p{L}\\p{N}]+"));
		QStringList tokens = folded.split(separators, Qt::SkipEmptyParts);
		return tokens;
	}

	bool hasDigit(const QString &word)
	{
		for (const QChar c : word)
			if (c.isDigit()) return true;
		return false;
	}

		/// What @p word scores in @p field; 0 when it does not match
	int wordScore(const QString &word, const QRegularExpression &glob, const Prepared &field, bool fuzzy)
	{
		int best = 0;
		if (glob.isValid() && !glob.pattern().isEmpty()) {
			if (glob.match(field.whole).hasMatch()) return GlobScore + field.bonus;
			for (const QString &token : field.tokens)
				if (glob.match(token).hasMatch()) return GlobScore + field.bonus;
			return 0;
		}
		if (field.whole == word) return ExactScore + field.bonus + 5;
		for (const QString &token : field.tokens) {
			if (token == word) best = qMax(best, ExactScore);
			else if (token.startsWith(word)) best = qMax(best, PrefixScore);
		}
		if (best == 0 && field.whole.startsWith(word)) best = PrefixScore;
		if (best == 0 && field.whole.contains(word)) best = InsideScore;
		if (best == 0 && fuzzy) {
			const int allowed = word.size() >= 10 ? 2 : 1;
			for (const QString &token : field.tokens) {
				if (qAbs(token.size() - word.size()) > allowed) continue;
				const int d = PartSearch::distance(word, token);
				if (d <= allowed) best = qMax(best, FuzzyScore - d * 5 + 5);
			}
		}
			// a word still being typed: it is close to the start of a longer
			// word ("contc" is on its way to "contactor")
		if (best == 0 && fuzzy) {
			const int allowed = word.size() >= 10 ? 2 : 1;
			for (const QString &token : field.tokens) {
				if (token.size() <= word.size()) continue;
				int d = PartSearch::distance(word, token.left(word.size()));
				if (allowed > 0) d = qMin(d, PartSearch::distance(word, token.left(word.size() + 1)));
				if (d <= allowed) best = qMax(best, FuzzyScore - d * 5);
			}
		}
		return best ? best + field.bonus : 0;
	}

	QRegularExpression globOf(const QString &word)
	{
		if (!word.contains(QLatin1Char('*'))) return QRegularExpression();
		QStringList parts = word.split(QLatin1Char('*'));
		for (QString &part : parts) part = QRegularExpression::escape(part);
		return QRegularExpression(QLatin1Char('^') + parts.join(QStringLiteral(".*")) + QLatin1Char('$'));
	}
}

QStringList PartSearch::words(const QString &text)
{
	return text.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

QString PartSearch::fold(const QString &text)
{
	const QString decomposed = text.toLower().normalized(QString::NormalizationForm_D);
	QString folded;
	folded.reserve(decomposed.size());
	for (const QChar c : decomposed)
		if (c.category() != QChar::Mark_NonSpacing) folded += c;
	return folded;
}

/**
	Optimal string alignment: the distance with the swap of two
	neighbour letters counted once
*/
int PartSearch::distance(const QString &a, const QString &b)
{
	const int n = a.size(), m = b.size();
	QList<QList<int>> d(n + 1, QList<int>(m + 1, 0));
	for (int i = 0 ; i <= n ; ++i) d[i][0] = i;
	for (int j = 0 ; j <= m ; ++j) d[0][j] = j;
	for (int i = 1 ; i <= n ; ++i) {
		for (int j = 1 ; j <= m ; ++j) {
			const int cost = a.at(i - 1) == b.at(j - 1) ? 0 : 1;
			d[i][j] = std::min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost});
			if (i > 1 && j > 1 && a.at(i - 1) == b.at(j - 2) && a.at(i - 2) == b.at(j - 1))
				d[i][j] = std::min(d[i][j], d[i - 2][j - 2] + 1);
		}
	}
	return d[n][m];
}

int PartSearch::score(const MaterialRecord &record, const QStringList &words)
{
	QList<Prepared> fields;
	for (const Field &field : Fields) {
		const QString text = fold(record.value(QString::fromLatin1(field.column)).trimmed());
		if (text.isEmpty()) continue;
		fields << Prepared{text, tokensOf(text), field.bonus};
	}
	int total = 0;
	for (const QString &raw : words) {
		const QString word = fold(raw);
		if (word.isEmpty()) continue;
		const QRegularExpression glob = globOf(word);
		const bool fuzzy = glob.pattern().isEmpty() && word.size() >= 4 && !hasDigit(word);
		int best = 0;
		for (const Prepared &field : std::as_const(fields))
			best = qMax(best, wordScore(word, glob, field, fuzzy));
		if (best == 0) return -1;
		total += best;
	}
	return total;
}

QString PartSearch::partNumber(const MaterialRecord &record)
{
	for (const QString &column : {QStringLiteral("manufacturer_reference"),
								  QStringLiteral("machine_manufacturer_reference"),
								  QStringLiteral("designation")}) {
		const QString value = record.value(column).trimmed();
		if (!value.isEmpty()) return value;
	}
	return QString();
}

QList<int> PartSearch::rank(const QList<MaterialRecord> &records, const QString &text, int limit,
							const QStringList &first)
{
	const QStringList wanted = words(text);
	QSet<QString> firsts;
	for (const QString &number : first) firsts << number.trimmed().toLower();
	const auto is_first = [&firsts](const MaterialRecord &record) {
		return !firsts.isEmpty() && firsts.contains(partNumber(record).trimmed().toLower());
	};
	QList<QPair<int, int>> found;   // score, index
	for (int i = 0 ; i < records.size() ; ++i) {
		const int s = wanted.isEmpty() ? 0 : score(records.at(i), wanted);
		if (s >= 0) found << qMakePair(s, i);
	}
	if (!wanted.isEmpty() || !firsts.isEmpty()) {
		const bool by_number = !wanted.isEmpty();
		std::stable_sort(found.begin(), found.end(), [&](const QPair<int, int> &x, const QPair<int, int> &y) {
			if (x.first != y.first) return x.first > y.first;
			const bool fx = is_first(records.at(x.second)), fy = is_first(records.at(y.second));
			if (fx != fy) return fx;
			if (!by_number) return false;
			return QString::compare(partNumber(records.at(x.second)), partNumber(records.at(y.second)),
									Qt::CaseInsensitive) < 0;
		});
	}
	QList<int> indexes;
	for (const auto &item : std::as_const(found)) {
		if (limit >= 0 && indexes.size() >= limit) break;
		indexes << item.second;
	}
	return indexes;
}
