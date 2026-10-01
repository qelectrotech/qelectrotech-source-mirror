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
};

#endif // SCRIPTHEADER_H
