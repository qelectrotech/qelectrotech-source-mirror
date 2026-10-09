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
#ifndef TEXTLINES_H
#define TEXTLINES_H

#include <QStringList>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>

namespace TextLines
{
	/**
		@return the lines of document as they are laid out: a paragraph
		wrapped to the width of the text gives several lines. For the
		exports that write a text line by line (DXF).
		A paragraph that is not wrapped gives exactly its line of
		toPlainText().split('\n'), as these exports wrote before: trailing
		spaces are kept, and non-breaking spaces become plain spaces. In a
		wrapped paragraph, the lines lose their trailing spaces (the one a
		line was broken at, and a line made only of spaces becomes empty).
	*/
	inline QStringList layoutLines(const QTextDocument *document)
	{
		document->size();	//Lay the document out

		QStringList lines;
		for (QTextBlock block = document->begin() ; block.isValid() ; block = block.next())
		{
			const QString text = block.text();
			const QTextLayout *layout = block.layout();
			const int count = layout ? layout->lineCount() : 0;
			if (count == 0) {
				lines << QString(text).replace(QChar::Nbsp, QLatin1Char(' '));
				continue;
			}

				//Wrapped: a line ends without a line break typed by the user
			bool wrapped = false;
			for (int i = 0 ; i < count - 1 ; ++i) {
				const QTextLine line = layout->lineAt(i);
				const int end = line.textStart() + line.textLength();
				if (end == 0 || text.at(end - 1) != QChar::LineSeparator)
					wrapped = true;
			}

			for (int i = 0 ; i < count ; ++i)
			{
				const QTextLine line = layout->lineAt(i);
				QString part = text.mid(line.textStart(), line.textLength());
				if (part.endsWith(QChar::LineSeparator))
					part.chop(1);	//A line break typed by the user (Shift+Enter)
				if (wrapped) {
					while (!part.isEmpty() && part.back().isSpace())
						part.chop(1);
				}
				lines << part.replace(QChar::Nbsp, QLatin1Char(' '));
			}
		}
		return lines;
	}
}

#endif // TEXTLINES_H
