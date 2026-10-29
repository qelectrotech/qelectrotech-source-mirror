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
#ifndef DIAGRAMEVENTADDDIMENSION_H
#define DIAGRAMEVENTADDDIMENSION_H

#include "../qetgraphicsitem/dimensionitem.h"
#include "diagrameventinterface.h"

class QGraphicsLineItem;

/**
	@brief The DiagramEventAddDimension class
	Interactive drawing tool / state machine for placing linear and chain dimensions.
*/
class DiagramEventAddDimension : public DiagramEventInterface
{
	Q_OBJECT

public:
	enum ToolState {
		WaitingForP1,
		WaitingForP2,
		WaitingForOffset,
		WaitingForChainPoint
	};

	DiagramEventAddDimension(Diagram *diagram, DimensionItem::DimensionMode mode = DimensionItem::Linear);
	~DiagramEventAddDimension() override;

	void init() override;
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

	static QCursor dimensionCursor();

private:
	void updateStatusHint();
	void commitCurrentDimension();
	void finishChain();
	void abortTool();

	DimensionItem::DimensionMode m_mode = DimensionItem::Linear;
	ToolState m_state = WaitingForP1;
	DimensionItem *m_dimension_item = nullptr;

	QPointF m_p1;
	QPointF m_p2;
	qreal m_fixed_offset = 20.0;
	bool m_offset_determined = false;
	QUuid m_active_chain_id;
};

#endif // DIAGRAMEVENTADDDIMENSION_H

