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
#ifndef RESETSYMBOLTEXTSCOMMAND_H
#define RESETSYMBOLTEXTSCOMMAND_H

#include <QDomElement>
#include <QList>
#include <QPointer>
#include <QUndoCommand>

class DynamicElementTextItem;
class Element;

/**
	@brief The ResetSymbolTextsCommand class
	Lay the texts of placed symbols out again as their symbol does: the
	position, font, alignment, width and rotation of each text the symbol
	defines. A placed symbol gets a copy of its symbol's texts when it is
	placed and keeps it; editing the symbol afterwards moves nothing on
	the folio. The texts are matched to the symbol's by where they take
	their text from (an information, a composite text, a typed text), in
	the symbol's order. Their contents and colours stay; texts in a group
	and texts added on the folio are left alone.
*/
class ResetSymbolTextsCommand : public QUndoCommand
{
	public:
		ResetSymbolTextsCommand(const QList<Element *> &elements, QUndoCommand *parent = nullptr);
		void undo() override;
		void redo() override;
			/// How many texts this command lays out again
		int count() const { return m_changes.size(); }

	private:
		struct Change {
			QPointer<DynamicElementTextItem> text;
			QDomElement before;
			QDomElement after;
		};
		QDomDocument m_document;
		QList<Change> m_changes;
};

#endif // RESETSYMBOLTEXTSCOMMAND_H
