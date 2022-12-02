/*
	Copyright 2006-2022 The QElectroTech Team
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
#ifndef TERMINALSTRIPITEM_H
#define TERMINALSTRIPITEM_H

#include <QGraphicsObject>

#include "terminalstripdrawer.h"
#include "../../qetgraphicsitem/qetgraphicsitem.h"

class TerminalStrip;

class TerminalStripItem : public QetGraphicsItem
{
		Q_OBJECT

	public:
		TerminalStripItem(QPointer<TerminalStrip> strip, QGraphicsItem *parent = nullptr);

		enum {Type = UserType + 1011};
		int type() const override {return Type;}

		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
		QRectF boundingRect() const override;
		QString name() const override;

        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

	private:
		QPointer<TerminalStrip> m_strip;
		TerminalStripDrawer m_drawer;

};

#endif // TERMINALSTRIPITEM_H
