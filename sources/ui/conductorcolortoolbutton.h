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
#ifndef CONDUCTORCOLORTOOLBUTTON_H
#define CONDUCTORCOLORTOOLBUTTON_H

#include <QColor>
#include <QList>
#include <QToolButton>

class QETDiagramEditor;
class DiagramView;

/**
	@brief The ConductorColorToolButton class
	One-click conductor colour, from the "Schéma" toolbar.

	Picking a colour does two things: it recolours every conductor
	currently selected (one undo command for the lot), and it becomes the
	colour of the next conductor drawn this session, through the existing
	LastUsedStyle mechanism. Either half is useful on its own -- with
	nothing selected it only sets the pen for what comes next.

	This deliberately stores nothing in the project and nothing in
	QSettings. It is the same session-scoped "what did I just use" idea
	LastUsedStyle already implements; named presets that persist per
	project are a separate, larger feature (upstream issue #461) that
	needs a maintainer decision first.
*/
class ConductorColorToolButton : public QToolButton
{
		Q_OBJECT

	public:
		explicit ConductorColorToolButton(QETDiagramEditor *editor,
						  QWidget *parent = nullptr);

	public slots:
		void updateEnabledState();

	private:
		DiagramView *currentView() const;
		void rebuildMenu();
		void applyColor(const QColor &color);
		void chooseOtherColor();
		void rememberRecent(const QColor &color);
		void setSwatch(const QColor &color);
		static QIcon swatchIcon(const QColor &color);

		QETDiagramEditor *m_editor = nullptr;
		QList<QColor> m_recent;
		QColor m_current;
};

#endif // CONDUCTORCOLORTOOLBUTTON_H
