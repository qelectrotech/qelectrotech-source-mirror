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
#include "diagrambgcolorbutton.h"

#include "../diagram.h"
#include "../diagramview.h"
#include "../palettegraphicsview.h"
#include "../qetdiagrameditor.h"
#include "../projectview.h"
#include "../qetproject.h"
#include "../utils/qetsettings.h"

#include <QSignalBlocker>
#include <QApplication>
#include <QColorDialog>
#include <QMenu>
#include <QPainter>
#include <QPixmap>

namespace {
	struct NamedColor { const char *context_name; QColor color; };

	QList<NamedColor> standardColors()
	{
		return {
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "White"),      QColor(0xFF, 0xFF, 0xFF)},
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Off-white"),  QColor(0xFD, 0xFB, 0xF5)},
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Light gray"),   QColor(0xE0, 0xE0, 0xE0)},
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Gray"),         QColor(0x80, 0x80, 0x80)},
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Dark gray"),   QColor(0x40, 0x40, 0x40)},
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Black"),         QColor(0x00, 0x00, 0x00)},
		};
	}

	const int MAX_RECENT = 6;
}

/**
	@brief DiagramBgColorToolButton::DiagramBgColorToolButton
	@param editor : the diagram editor this button acts on
	@param parent
*/
DiagramBgColorToolButton::DiagramBgColorToolButton(QETDiagramEditor *editor, QWidget *parent) :
	QToolButton(parent),
	m_editor(editor)
{
	setPopupMode(QToolButton::InstantPopup);
	setToolTip(tr("Sheet background color"));
	setStatusTip(tr("Choose the sheet background color",
		"status bar tip"));

		//main() already restored the stored choice onto the global
		//Diagram::background_color and PaletteGraphicsView flag; mirror it
		//here so the button opens on what the editor actually draws, rather
		//than always claiming "system colour" as it used to.
	if (PaletteGraphicsView::customBackgroundColor()) {
		m_is_system_color = false;
		m_colored_ink = PaletteGraphicsView::coloredInk();
		m_current = Diagram::background_color;
	} else {
		m_is_system_color = true;
		m_current = QApplication::palette().color(QPalette::Base);
	}
	for (const QString &name : QetSettings::sheetBackgroundRecentColors())
	{
		const QColor c(name);
		if (c.isValid()) {
			m_recent.append(c);
		}
	}
	while (m_recent.size() > MAX_RECENT) {
		m_recent.removeLast();
	}

	setMenu(new QMenu(tr("Sheet background color"), this));
	rebuildMenu();
	setSwatch(m_current);

	m_dark_mode = new QAction(tr("Dark mode"), this);
	m_dark_mode->setCheckable(true);
	m_dark_mode->setChecked(m_colored_ink);
	m_dark_mode->setStatusTip(tr("A black sheet: symbols and texts in yellow, conductors in red, "
				     "dashed ones in light blue and dash-dotted ones in yellow, the rest "
				     "in white. Printing and exporting stay black on white."));
	connect(m_dark_mode, &QAction::triggered, this, &DiagramBgColorToolButton::setDarkMode);
	allButtons() << this;
}

DiagramBgColorToolButton::~DiagramBgColorToolButton()
{
	allButtons().removeAll(this);
}

/**
	@brief DiagramBgColorToolButton::allButtons
	@return the buttons of every editor window: the sheet color and the
	dark mode are the same for all of them
*/
QList<DiagramBgColorToolButton *> &DiagramBgColorToolButton::allButtons()
{
	static QList<DiagramBgColorToolButton *> buttons;
	return buttons;
}

/**
	@brief DiagramBgColorToolButton::showSheetState
	Show the sheet the views draw now (Diagram::background_color and the
	PaletteGraphicsView flags): the swatch, and the dark mode's check
	mark, without triggering it.
*/
void DiagramBgColorToolButton::showSheetState()
{
	m_is_system_color = !PaletteGraphicsView::customBackgroundColor();
	m_colored_ink = PaletteGraphicsView::coloredInk();
	const QSignalBlocker blocker(m_dark_mode);
	m_dark_mode->setChecked(m_colored_ink);
	setSwatch(m_is_system_color ? QApplication::palette().color(QPalette::Base)
				    : Diagram::background_color);
}

/**
	@brief DiagramBgColorToolButton::spreadSheetState
	After a change here, every editor window shows the new sheet: its
	button and its menu entry, its recent colors, and its folios, built
	or not (a folio not built yet takes the color when it is drawn).
*/
void DiagramBgColorToolButton::spreadSheetState()
{
	for (DiagramBgColorToolButton *button : std::as_const(allButtons()))
	{
		if (button != this && button->m_recent != m_recent) {
			button->m_recent = m_recent;
			button->rebuildMenu();
		}
		button->showSheetState();
		if (QETDiagramEditor *editor = button->m_editor)
			for (ProjectView *pv : editor->openedProjects())
				for (Diagram *d : pv->project()->folios())
					d->update();
	}
}

/**
	@brief DiagramBgColorToolButton::setDarkMode
	Switch to the black sheet with colored lines, or back to the sheet
	that was there before (the system color after a restart).
	@param on
*/
void DiagramBgColorToolButton::setDarkMode(bool on)
{
	if (on) {
		s_before_dark_system = m_is_system_color;
		s_before_dark_color = m_current;
		applyColor(Qt::black, true);
	} else if (s_before_dark_system || !s_before_dark_color.isValid()) {
		applySystemColor();
	} else {
		applyColor(s_before_dark_color);
	}
}

/**
	@brief DiagramBgColorToolButton::rebuildMenu
*/
void DiagramBgColorToolButton::rebuildMenu()
{
	QMenu *m = menu();
	m->clear();

	QAction *sys = m->addAction(tr("System color"));
	connect(sys, &QAction::triggered, this, &DiagramBgColorToolButton::applySystemColor);
	m->addSeparator();

	for (const auto &nc : standardColors())
	{
		const QColor c = nc.color;
		QAction *a = m->addAction(swatchIcon(c),
					  tr(nc.context_name));
		connect(a, &QAction::triggered, this, [this, c]() { applyColor(c); });
	}

	if (!m_recent.isEmpty())
	{
		m->addSeparator();
		QAction *title = m->addAction(tr("Recently used"));
		title->setEnabled(false);
		for (const QColor &c : std::as_const(m_recent))
		{
			QAction *a = m->addAction(swatchIcon(c), c.name());
			connect(a, &QAction::triggered, this, [this, c]() { applyColor(c); });
		}
	}

	m->addSeparator();
	QAction *other = m->addAction(tr("Other color…"));
	connect(other, &QAction::triggered, this, &DiagramBgColorToolButton::chooseOtherColor);
}

/**
	@brief DiagramBgColorToolButton::applyColor
	Set a custom background color on all open diagrams. On a dark color
	the folios are shown in light lines (see QET::Palette::sheetInk()).
	@param color
	@param colored_ink : the dark mode's colored lines instead of one ink
	for all
*/
void DiagramBgColorToolButton::applyColor(const QColor &color, bool colored_ink)
{
	if (!color.isValid()) {
		return;
	}

	m_is_system_color = false;
	m_colored_ink = colored_ink;
	m_dark_mode->setChecked(colored_ink);
	rememberRecent(color);
	setSwatch(color);

	PaletteGraphicsView::setCustomBackgroundColor(true);
	PaletteGraphicsView::setColoredInk(colored_ink);
	Diagram::background_color = color;
	QetSettings::setSheetBackground(QetSettings::SheetBackground{color, true, colored_ink});
	spreadSheetState();
}

/**
	@brief DiagramBgColorToolButton::applySystemColor
	Restore the system-default background and re-enable dark-mode
	inversion.
*/
void DiagramBgColorToolButton::applySystemColor()
{
	m_is_system_color = true;
	m_colored_ink = false;
	m_dark_mode->setChecked(false);
	m_current = QApplication::palette().color(QPalette::Base);
	setSwatch(m_current);

	PaletteGraphicsView::setCustomBackgroundColor(false);
	PaletteGraphicsView::setColoredInk(false);
	Diagram::background_color = Qt::white;
	QetSettings::setSheetBackground(
				QetSettings::SheetBackground{QColor(Qt::white), false});
	spreadSheetState();
}

/**
	@brief DiagramBgColorToolButton::chooseOtherColor
*/
void DiagramBgColorToolButton::chooseOtherColor()
{
	const QColor c = QColorDialog::getColor(m_current, this,
						tr("Choose a background color"),
						QColorDialog::DontUseNativeDialog);
	if (c.isValid()) {
		applyColor(c);
	}
}

/**
	@brief DiagramBgColorToolButton::rememberRecent
	Most recent first, no duplicates, capped.
	@param color
*/
void DiagramBgColorToolButton::rememberRecent(const QColor &color)
{
	for (const auto &nc : standardColors()) {
		if (nc.color == color) {
			return;
		}
	}

	m_recent.removeAll(color);
	m_recent.prepend(color);
	while (m_recent.size() > MAX_RECENT) {
		m_recent.removeLast();
	}
	rebuildMenu();
	persistRecent();
}

/**
	@brief DiagramBgColorToolButton::persistRecent
	Write the "recently used" colours back to the settings, so they are
	still there on the next start instead of starting out empty.
*/
void DiagramBgColorToolButton::persistRecent()
{
	QStringList names;
	for (const QColor &c : std::as_const(m_recent)) {
		names << c.name();
	}
	QetSettings::setSheetBackgroundRecentColors(names);
}

/**
	@brief DiagramBgColorToolButton::setSwatch
	@param color
*/
void DiagramBgColorToolButton::setSwatch(const QColor &color)
{
	m_current = color;
	setIcon(swatchIcon(color, m_colored_ink));
}

/**
	@brief DiagramBgColorToolButton::syncFromDiagram
	Sync the button swatch to whatever Diagram::background_color is
	currently set to.  Called when the active folio changes.
*/
void DiagramBgColorToolButton::syncFromDiagram()
{
	if (m_is_system_color) {
		m_current = QApplication::palette().color(QPalette::Base);
	} else {
		m_current = Diagram::background_color;
	}
	setSwatch(m_current);
}

/**
	@brief DiagramBgColorToolButton::updateEnabledState
*/
void DiagramBgColorToolButton::updateEnabledState()
{
	setEnabled(m_editor && m_editor->currentProjectView());
}

/**
	@brief DiagramBgColorToolButton::swatchIcon
	@param color
	@param colored_ink : with a yellow and a red stroke across, for the
	dark mode
	@return a plain square of that colour, outlined so that white and very
	light colours are still visible against the toolbar.
*/
QIcon DiagramBgColorToolButton::swatchIcon(const QColor &color, bool colored_ink)
{
	QPixmap pix(16, 16);
	pix.fill(Qt::transparent);
	QPainter p(&pix);
	p.setRenderHint(QPainter::Antialiasing, false);
	p.setBrush(color);
	p.setPen(QPen(QColor(0x40, 0x40, 0x40), 1));
	p.drawRect(0, 0, 15, 15);
	if (colored_ink) {
		p.fillRect(3, 4, 10, 2, QColor(255, 255, 0));
		p.fillRect(3, 10, 10, 2, QColor(255, 0, 0));
	}
	p.end();
	return QIcon(pix);
}
