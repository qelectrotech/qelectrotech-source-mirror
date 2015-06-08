/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef LINKELEMENTCOMMAND_H
#define LINKELEMENTCOMMAND_H

#include <QUndoCommand>

class Element;

/**
 * @brief The LinkElementCommand class
 * This undo class manage link between elements.
 * In the same instance of this class, we can link and unlink elements from an edited element
 * This undo class support the merge.
 */
class LinkElementCommand : public QUndoCommand
{
	public:
		LinkElementCommand(Element *element_, QUndoCommand *parent = 0);

		virtual int id() const {return 2;}
		virtual bool mergeWith(const QUndoCommand *other);

		static bool isLinkable (Element *element_a, Element *element_b, bool already_linked = false);

		void addLink (QList<Element *> element_list);
		void addLink (Element *element_);
		void setLink (QList<Element *> element_list);
		void setLink (Element *element_);
		void unlink  (QList<Element *> element_list);
		void unlinkAll ();

		void undo();
		void redo();

	private:
		void setUpNewLink (const QList<Element *> &element_list, bool already_link);
		void makeLink (const QList <Element *> &element_list);

	private:
		Element *m_element;
		QList<Element *> m_linked_before; //<Linked elements before this command, or when we call "undo"
		QList<Element *> m_linked_after;  //<Linked elements after this command, or when we recall "redo"
};

#endif // LINKELEMENTCOMMAND_H
