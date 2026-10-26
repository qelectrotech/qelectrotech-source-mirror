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
#include <QAction>
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
		~DiagramBgColorToolButton() override;

		/// View > Dark mode: the black sheet with colored lines
		QAction *darkModeAction() const { return m_dark_mode; }

	public slots:
		void updateEnabledState();
		void syncFromDiagram();

	private:
		void rebuildMenu();
		void applyColor(const QColor &color, bool colored_ink = false);
		void applySystemColor();
		void setDarkMode(bool on);
		void chooseOtherColor();
		void rememberRecent(const QColor &color);
		void persistRecent();
		void setSwatch(const QColor &color);
		static QIcon swatchIcon(const QColor &color, bool colored_ink = false);
		void showSheetState();
		void spreadSheetState();
		static QList<DiagramBgColorToolButton *> &allButtons();

		QETDiagramEditor *m_editor = nullptr;
		QList<QColor> m_recent;
		QColor m_current;
		bool m_is_system_color = false;
		bool m_colored_ink = false;
		QAction *m_dark_mode = nullptr;
			/// The sheet before View > Dark mode was switched on, in
			/// whichever editor window: switched off in another, it
			/// comes back too
		static inline bool s_before_dark_system = true;
		static inline QColor s_before_dark_color;
};

#endif // DIAGRAMBGCOLORTOOLBUTTON_H
