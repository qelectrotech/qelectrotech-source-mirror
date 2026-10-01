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
#ifndef SCRIPTHEADER_H
#define SCRIPTHEADER_H

#include <QRegularExpression>
#include <QString>
#include <QStringList>

/**
	@brief The ScriptHeader struct
	How a stored script's button looks, read from a comment block at the top
	of the .js file itself, so one file is all there is to write by hand,
	to share, or for an assistant to create:

	@code
	// ==QETScript==
	// @name     Add revision note
	// @icon     note.svg           (a file next to the script, or builtin:<theme icon>)
	// @tooltip  Puts a "Rev A" note on the folio on screen
	// @shortcut Ctrl+Alt+R
	// @context  canvas             (canvas, selection or conductor)
	// @api      1
	// ==/QETScript==
	@endcode

	Only @name is required. An unknown key is an error rather than ignored:
	a misspelt "@shortcut" that silently did nothing would be harder to find
	than a script refused with the line that is wrong.

	Header-only, with no QElectroTech dependency, so it is tested on its own.
*/
struct ScriptHeader
{
	QString id;        ///< file name without .js; the action id is diagrameditor.script.<id>
	QString name;
	QString icon;
	QString tooltip;
	QString shortcut;  ///< as QKeySequence::fromString() reads it
	QString context = QStringLiteral("canvas");
	int api = 1;
	QString error;     ///< empty if the header is usable

	bool isValid() const { return error.isEmpty(); }

	static QStringList contexts()
	{
		return {QStringLiteral("canvas"), QStringLiteral("selection"),
			QStringLiteral("conductor")};
	}

	/**
		@brief parse
		@param text : the whole script
		@param id : the script's file name without its extension
	*/
	static ScriptHeader parse(const QString &text, const QString &id)
	{
		ScriptHeader h;
		h.id = id;
		static const QRegularExpression block(
			QStringLiteral("//\\s*==QETScript==\\s*\\n(.*?)//\\s*==/QETScript=="),
			QRegularExpression::DotMatchesEverythingOption);
		const QRegularExpressionMatch m = block.match(text);
		if (!m.hasMatch()) {
			h.error = QStringLiteral("no // ==QETScript== header");
			return h;
		}
		static const QRegularExpression line_re(
			QStringLiteral("^\\s*//\\s*@(\\w+)\\s+(.*?)\\s*$"));
		const QStringList lines = m.captured(1).split(QLatin1Char('\n'));
		for (const QString &line : lines) {
			const QRegularExpressionMatch lm = line_re.match(line);
			if (!lm.hasMatch()) {
				continue;
			}
			const QString key = lm.captured(1);
			const QString value = lm.captured(2);
			if (key == QLatin1String("name")) h.name = value;
			else if (key == QLatin1String("icon")) h.icon = value;
			else if (key == QLatin1String("tooltip")) h.tooltip = value;
			else if (key == QLatin1String("shortcut")) h.shortcut = value;
			else if (key == QLatin1String("context")) h.context = value;
			else if (key == QLatin1String("api")) h.api = value.toInt();
			else {
				h.error = QStringLiteral("unknown header key @%1").arg(key);
				return h;
			}
		}
		if (h.name.isEmpty()) {
			h.error = QStringLiteral("@name is required");
		} else if (!contexts().contains(h.context)) {
			h.error = QStringLiteral("@context must be one of: %1")
					.arg(contexts().join(QStringLiteral(", ")));
		} else if (h.api != 1) {
			h.error = QStringLiteral("@api %1 is not supported by this "
						 "version (1 is)").arg(h.api);
		}
		return h;
	}

	/**
		@brief compose
		A whole script file: the header parse() reads, then @a body. Empty
		fields are left out, so a file saved by the script manager and a
		hand-written one look alike.
	*/
	static QString compose(const QString &name, const QString &icon,
			       const QString &tooltip, const QString &shortcut,
			       const QString &context, const QString &body)
	{
		QString text = QStringLiteral("// ==QETScript==\n");
		auto line = [&text](const QString &key, const QString &value) {
			const QString v = value.simplified();
			if (!v.isEmpty())
				text += QStringLiteral("// @%1 %2\n").arg(key.leftJustified(8), v);
		};
		line(QStringLiteral("name"), name);
		line(QStringLiteral("icon"), icon);
		line(QStringLiteral("tooltip"), tooltip);
		line(QStringLiteral("shortcut"), shortcut);
		if (context != QLatin1String("canvas")) line(QStringLiteral("context"), context);
		line(QStringLiteral("api"), QStringLiteral("1"));
		text += QStringLiteral("// ==/QETScript==\n") + body;
		if (!text.endsWith(QLatin1Char('\n'))) text += QLatin1Char('\n');
		return text;
	}

	/**
		@brief bodyOf
		Everything after the header, or the whole text if there is none.
	*/
	static QString bodyOf(const QString &text)
	{
		const int end = text.indexOf(QStringLiteral("// ==/QETScript=="));
		if (end < 0) return text;
		const int next = text.indexOf(QLatin1Char('\n'), end);
		return next < 0 ? QString() : text.mid(next + 1);
	}

	/**
		@brief idFor
		A file name for a new script called @a name: lower-case letters,
		digits and dashes, accents dropped, not one of @a taken.
	*/
	static QString idFor(const QString &name, const QStringList &taken)
	{
		QString base = name.normalized(QString::NormalizationForm_KD).toLower();
		static const QRegularExpression not_kept(QStringLiteral("[^a-z0-9\\s_-]"));
		base.remove(not_kept);
		base = base.simplified().replace(QLatin1Char(' '), QLatin1Char('-')).left(48);
		if (base.isEmpty()) base = QStringLiteral("script");
		QString id = base;
		for (int i = 2; taken.contains(id); ++i)
			id = base + QLatin1Char('-') + QString::number(i);
		return id;
	}
};

#endif // SCRIPTHEADER_H
