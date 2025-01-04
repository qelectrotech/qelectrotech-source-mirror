/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef TERMINALSTRIPLAYOUTEDITOR_H
#define TERMINALSTRIPLAYOUTEDITOR_H

#include <QGraphicsItem>
#include <QWidget>

#include "../GraphicsItem/demoterminalstrip.h"
#include "../GraphicsItem/terminalstripdrawer.h"

class TerminalStripLayoutPattern;

namespace Ui {
	class TerminalStripLayoutEditor;
}

class PreviewStripItem : public QGraphicsItem
{
	friend class TerminalStripLayoutEditor;

	public:
		PreviewStripItem (QSharedPointer<TerminalStripLayoutPattern> layout) :
			m_drawer {QSharedPointer<TerminalStripDrawer::DemoTerminalStrip>{new TerminalStripDrawer::DemoTerminalStrip},
					 layout}
		{}

		QRectF boundingRect() const override {
			return m_drawer.boundingRect();
		}

	protected:
		void paint(QPainter *painter,
				   const QStyleOptionGraphicsItem *option,
				   QWidget *widget = nullptr) override
		{
			Q_UNUSED (option); Q_UNUSED (widget);
			m_drawer.paint(painter);
		}

	private:
		TerminalStripDrawer::TerminalStripDrawer m_drawer;
	};

/**
 * @brief The TerminalStripLayoutEditor class
 * Widget used to edit the layout of a terminal strip item
 */
class TerminalStripLayoutEditor : public QWidget
{
		Q_OBJECT

	public:
		explicit TerminalStripLayoutEditor(QSharedPointer<TerminalStripLayoutPattern> layout,
										   QWidget *parent = nullptr);
		~TerminalStripLayoutEditor();

	protected:
		void resizeEvent(QResizeEvent *event) override;
		void showEvent(QShowEvent *event) override;

	private slots:
		void valueEdited();

		void on_m_display_preview_help_clicked(bool checked);

	private:
		void updateUi();
		void updatePreview();

	private:
		Ui::TerminalStripLayoutEditor *ui;
		QSharedPointer<TerminalStripLayoutPattern> m_layout;
		bool m_ui_updating { false } ;
		PreviewStripItem m_preview_strip_item {m_layout};
};

#endif // TERMINALSTRIPLAYOUTEDITOR_H
