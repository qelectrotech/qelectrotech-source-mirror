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
#ifndef ESEVENTMEASURE_H
#define ESEVENTMEASURE_H

#include <QCoreApplication>
#include <QList>
#include <QPointer>
#include "eseventinterface.h"

class ElementScene;
class ElementMeasureItem;
class QAction;
class QGraphicsSceneMouseEvent;
class QKeyEvent;

/**
	@brief The ESEventMeasure class
	Manages CAD tape measure tool in the Element Editor.
	Allows point-to-point distance measurement with live distance (mm), ΔX, ΔY, and angle.
*/
class ESEventMeasure : public ESEventInterface
{
	Q_DECLARE_TR_FUNCTIONS(ESEventMeasure)

	public:
		ESEventMeasure(ElementScene *scene, QAction *action = nullptr);
		~ESEventMeasure() override;

		bool mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		bool mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		bool keyPressEvent(QKeyEvent *event) override;
		bool isFinish() const override;

	private:
		void updateStatusMessage(const QPointF &p1, const QPointF &p2);
		void clearAllMeasurements();

		QPointer<QAction> m_action;
		ElementMeasureItem *m_current_measure = nullptr;
		QList<ElementMeasureItem *> m_pinned_measures;
		bool m_in_first_click = true;
};

#endif // ESEVENTMEASURE_H
