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
	*/
	inline QStringList layoutLines(const QTextDocument *document)
	{
		document->size();	//Lay the document out

		QStringList lines;
		for (QTextBlock block = document->begin() ; block.isValid() ; block = block.next())
		{
			const QString text = block.text();
			const QTextLayout *layout = block.layout();
			if (!layout || layout->lineCount() == 0) {
				lines << text;
				continue;
			}
			for (int i = 0 ; i < layout->lineCount() ; ++i)
			{
				const QTextLine line = layout->lineAt(i);
				QString part = text.mid(line.textStart(), line.textLength());
					//The space the line was broken at, or a line separator
				while (!part.isEmpty() && part.back().isSpace())
					part.chop(1);
				lines << part;
			}
		}
		return lines;
	}
}

#endif // TEXTLINES_H
