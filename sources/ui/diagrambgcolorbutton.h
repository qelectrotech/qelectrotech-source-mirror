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
#ifndef DIAGRAMBGCOLORTOOLBUTTON_H
#define DIAGRAMBGCOLORTOOLBUTTON_H

#include <QColor>
#include <QList>
#include <QToolButton>

class QETDiagramEditor;

/**
	@brief The DiagramBgColorToolButton class
	Color picker for the diagram sheet background, placed in the
	"Affichage" toolbar.  Mirrors the ConductorColorToolButton UX:
	preset colors in the dropdown, "Autre couleur..." at the bottom,
	and a swatch icon on the button itself.

	Picking "Couleur système" clears any custom colour and lets the
	dark-mode inversion handle the background as before.
*/
class DiagramBgColorToolButton : public QToolButton
{
		Q_OBJECT

	public:
		explicit DiagramBgColorToolButton(QETDiagramEditor *editor,
						  QWidget *parent = nullptr);

	public slots:
		void updateEnabledState();
		void syncFromDiagram();

	private:
		void rebuildMenu();
		void applyColor(const QColor &color);
		void applySystemColor();
		void chooseOtherColor();
		void rememberRecent(const QColor &color);
		void setSwatch(const QColor &color);
		static QIcon swatchIcon(const QColor &color);

		QETDiagramEditor *m_editor = nullptr;
		QList<QColor> m_recent;
		QColor m_current;
		bool m_is_system_color = false;
};

#endif // DIAGRAMBGCOLORTOOLBUTTON_H
