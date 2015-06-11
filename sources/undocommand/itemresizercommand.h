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
#ifndef ITEMRESIZERCOMMAND_H
#define ITEMRESIZERCOMMAND_H

#include <QUndoCommand>
#include <QPropertyAnimation>

class QetGraphicsItem;
class Diagram;

/**
 * @brief The ItemResizerCommand class
 * This Undo command manage the scale of QetGraphicsItem.
 */
class ItemResizerCommand : public QUndoCommand
{
	public:
		ItemResizerCommand (QetGraphicsItem *qgi, qreal &old_, qreal &new_,const QString  &text, QUndoCommand *parent = 0);
		virtual ~ItemResizerCommand();

	public:
		virtual int id() const {return 3;}
		virtual bool mergeWith(const QUndoCommand *other);
		virtual void undo();
		virtual void redo();

	private:
		QetGraphicsItem *m_qgi;
		qreal m_old_size, m_new_size;
		Diagram *m_diagram;
		QString m_text;
		QPropertyAnimation m_animation;
		bool m_first_redo;
};

#endif // ITEMRESIZERCOMMAND_H
