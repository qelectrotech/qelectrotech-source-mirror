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
#ifndef DOCKFOLD_H
#define DOCKFOLD_H

#include <QObject>
#include <QPointer>

class QDockWidget;
class QMainWindow;
class QToolButton;

/**
	A button at the edge of the drawing, beside the docks of one side, as
	in LibreOffice's sidebar: a click folds the docks of that side away,
	and the drawing gets their room; a second click shows them again, at
	the width and heights they had. Docks opened another way (the
	Windows menu) end the fold. The folded docks are remembered over a
	restart.
*/
class DockFold : public QObject
{
	Q_OBJECT

	public:
		DockFold(QMainWindow *window, Qt::DockWidgetArea area);

			/// The setting (Settings > General, on by default)
		static bool enabled();
		static void setEnabled(bool on);

		bool isFolded() const { return !m_folded.isEmpty(); }
		QToolButton *button() const { return m_button; }

	public slots:
		void toggle();

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;

	private:
		QList<QDockWidget *> openDocks() const;
		QDockWidget *dockNamed(const QString &name) const;
		void fold();
		void unfold();
		void refresh();
		void place();
		QString settingKey() const;
		void remember();

		QMainWindow *m_window;
		Qt::DockWidgetArea m_area;
		QToolButton *m_button;
			/// The folded docks, by name: all of them, those that were
			/// in front of their tab group, the width of the side and the
			/// height of each one in front
		QStringList m_folded;
		QStringList m_in_front;
		int m_width = 0;
		QList<int> m_heights;
		bool m_busy = false;
};

#endif // DOCKFOLD_H
