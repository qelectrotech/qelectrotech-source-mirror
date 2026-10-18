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
#ifndef TERMINALSTRIPITEM_H
#define TERMINALSTRIPITEM_H

#include <QGraphicsObject>
#include <QList>
#include <QRectF>
#include <QUuid>

#include "terminalstripdrawer.h"
#include "../../qetgraphicsitem/qetgraphicsitem.h"

class Diagram;
class TerminalStrip;

class TerminalStripItem : public QetGraphicsItem
{
		friend class TerminalStripItemXml;

		Q_OBJECT

	public:
		TerminalStripItem(QPointer<TerminalStrip> strip, QGraphicsItem *parent = nullptr);
		TerminalStripItem(QGraphicsItem *parent = nullptr);

		void setTerminalStrip(TerminalStrip *strip);
		QPointer<TerminalStrip> terminalStrip() const;

		enum {Type = UserType + 1011};
		int type() const override {return Type;}

		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
		QRectF boundingRect() const override;
		QString name() const override;

		void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
        void refreshPending();
        void setLayout(QSharedPointer<TerminalStripLayoutPattern> layout);
		void setDrawStyle(TerminalStripLayoutPattern::DrawStyle style);
		TerminalStripLayoutPattern::DrawStyle drawStyle() const;

		/**
		 * @brief The PdfRef struct
		 * One xref of the plan which leads to an element: used to make
		 * it a clickable link in an exported PDF.
		 */
		struct PdfRef
		{
				///The folio of the element the xref points at
			Diagram *diagram = nullptr;
				///Where the text of the xref stands on the folio of this item
			QRectF here;
				///Where the element stands on its folio
			QRectF there;
		};
		QList<PdfRef> pdfRefs() const;

	private:
		void setDefaultLayout();
		QString xrefToolTip(const TerminalStripDrawer::hoverTerminal &terminal) const;
		void connectStrip(TerminalStrip *strip);
		void stripChanged();

	private:
		QPointer<TerminalStrip> m_strip;
		QList<QMetaObject::Connection> m_strip_connections;
        TerminalStripDrawer::TerminalStripDrawer m_drawer;
        QUuid m_pending_strip_uuid;
};

#endif // TERMINALSTRIPITEM_H
