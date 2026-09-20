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
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Blanc"),      QColor(0xFF, 0xFF, 0xFF)},
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Blanc cassé"),  QColor(0xFD, 0xFB, 0xF5)},
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Gris clair"),   QColor(0xE0, 0xE0, 0xE0)},
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Gris"),         QColor(0x80, 0x80, 0x80)},
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Gris foncé"),   QColor(0x40, 0x40, 0x40)},
			{QT_TRANSLATE_NOOP("DiagramBgColorToolButton", "Noir"),         QColor(0x00, 0x00, 0x00)},
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
	setToolTip(tr("Couleur de fond du folio"));
	setStatusTip(tr("Choisir la couleur de fond du folio",
		"status bar tip"));

	m_is_system_color = true;
	m_current = QApplication::palette().color(QPalette::Base);
	setMenu(new QMenu(this));
	rebuildMenu();
	setSwatch(m_current);
}

/**
	@brief DiagramBgColorToolButton::rebuildMenu
*/
void DiagramBgColorToolButton::rebuildMenu()
{
	QMenu *m = menu();
	m->clear();

	QAction *sys = m->addAction(tr("Couleur système"));
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
		QAction *title = m->addAction(tr("Récemment utilisées"));
		title->setEnabled(false);
		for (const QColor &c : std::as_const(m_recent))
		{
			QAction *a = m->addAction(swatchIcon(c), c.name());
			connect(a, &QAction::triggered, this, [this, c]() { applyColor(c); });
		}
	}

	m->addSeparator();
	QAction *other = m->addAction(tr("Autre couleur…"));
	connect(other, &QAction::triggered, this, &DiagramBgColorToolButton::chooseOtherColor);
}

/**
	@brief DiagramBgColorToolButton::applyColor
	Set a custom background color on all open diagrams.
	@param color
*/
void DiagramBgColorToolButton::applyColor(const QColor &color)
{
	if (!color.isValid()) {
		return;
	}

	m_is_system_color = false;
	rememberRecent(color);
	setSwatch(color);

	PaletteGraphicsView::setCustomBackgroundColor(true);
	Diagram::background_color = color;

	QETDiagramEditor *editor = m_editor;
	if (!editor) {
		return;
	}
	for (ProjectView *pv : editor->openedProjects())
		for (Diagram *d : pv->project()->diagrams())
			d->update();
}

/**
	@brief DiagramBgColorToolButton::applySystemColor
	Restore the system-default background and re-enable dark-mode
	inversion.
*/
void DiagramBgColorToolButton::applySystemColor()
{
	m_is_system_color = true;
	m_current = QApplication::palette().color(QPalette::Base);
	setSwatch(m_current);

	PaletteGraphicsView::setCustomBackgroundColor(false);
	Diagram::background_color = Qt::white;

	QETDiagramEditor *editor = m_editor;
	if (!editor) {
		return;
	}
	for (ProjectView *pv : editor->openedProjects())
		for (Diagram *d : pv->project()->diagrams())
			d->update();
}

/**
	@brief DiagramBgColorToolButton::chooseOtherColor
*/
void DiagramBgColorToolButton::chooseOtherColor()
{
	const QColor c = QColorDialog::getColor(m_current, this,
						tr("Choisir une couleur de fond"));
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
}

/**
	@brief DiagramBgColorToolButton::setSwatch
	@param color
*/
void DiagramBgColorToolButton::setSwatch(const QColor &color)
{
	m_current = color;
	setIcon(swatchIcon(color));
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
	@return a plain square of that colour, outlined so that white and very
	light colours are still visible against the toolbar.
*/
QIcon DiagramBgColorToolButton::swatchIcon(const QColor &color)
{
	QPixmap pix(16, 16);
	pix.fill(Qt::transparent);
	QPainter p(&pix);
	p.setRenderHint(QPainter::Antialiasing, false);
	p.setBrush(color);
	p.setPen(QPen(QColor(0x40, 0x40, 0x40), 1));
	p.drawRect(0, 0, 15, 15);
	p.end();
	return QIcon(pix);
}
