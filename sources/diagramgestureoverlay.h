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
#ifndef DIAGRAMGESTUREOVERLAY_H
#define DIAGRAMGESTUREOVERLAY_H

#include <QList>
#include <QWidget>

class QAction;

/**
	@brief The ring shown while right-dragging on a folio: up to eight
	commands around the point where the button went down, the one in the
	mouse's direction highlighted. Releasing runs it, releasing near the
	centre cancels -- SolidWorks' mouse gestures.

	Commands are placed clockwise from the top. The ring only draws; the
	view tracks the mouse and asks sectorAt() on release.
*/
class DiagramGestureOverlay : public QWidget
{
	Q_OBJECT

	public:
		explicit DiagramGestureOverlay(QWidget *viewport);

		void showAt(const QPoint &center, const QList<QAction *> &actions);
		void setPointer(const QPoint &viewport_pos);
		int sectorAt(const QPoint &viewport_pos) const;
		QAction *actionAt(const QPoint &viewport_pos) const;

		static bool isEnabled();

		static const int sectors = 8;

	protected:
		void paintEvent(QPaintEvent *event) override;

	private:
		QPoint m_center;
		QList<QAction *> m_actions;
		int m_active = -1;
};

#endif // DIAGRAMGESTUREOVERLAY_H
