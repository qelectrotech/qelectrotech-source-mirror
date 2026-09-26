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
#ifndef DIAGRAMCONTEXTTOOLBAR_H
#define DIAGRAMCONTEXTTOOLBAR_H

#include <QFrame>
#include <QList>

class QAction;
class QGraphicsOpacityEffect;
class QHBoxLayout;

/**
	@brief A small row of commands shown beside the cursor after a click
	selects something on a folio, as SolidWorks does.

	It lives on the view's viewport, so it never takes the focus or opens a
	window. It fades as the mouse moves away and hides once the mouse is far
	enough, on a click elsewhere, a wheel turn or a key press. The commands
	are the shortcut bar's for the selection (ShortcutBarSettings).
*/
class DiagramContextToolbar : public QFrame
{
	Q_OBJECT

	public:
		explicit DiagramContextToolbar(QWidget *viewport);

		void showAt(const QPoint &viewport_pos, const QList<QAction *> &actions);
		static bool isEnabled();

			/// At most this many commands, so it stays small
		static const int max_actions = 8;

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;

	private:
		void followMouse(const QPoint &viewport_pos);

		QHBoxLayout *m_layout = nullptr;
		QGraphicsOpacityEffect *m_opacity = nullptr;
};

#endif // DIAGRAMCONTEXTTOOLBAR_H
