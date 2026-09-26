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
#include "conductorcolortoolbutton.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../diagramcontent.h"
#include "../diagramview.h"
#include "../lastusedstyle.h"
#include "../qetdiagrameditor.h"
#include "../projectview.h"
#include "../qetgraphicsitem/conductor.h"
#include "../conductorproperties.h"

#include <QColorDialog>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSet>

namespace {
	/**
		The colours an electrician reaches for, in the order the trade
		names them: the three phases, neutral, earth, then the ones used
		for control and extra-low-voltage circuits. Not a standard in
		itself -- IEC 60445 only fixes blue for neutral and green/yellow
		for protective earth -- but it covers the wiring a schematic
		actually shows, which is what makes the toolbar worth a click.
	*/
	struct NamedColor { const char *context_name; QColor color; };

	QList<NamedColor> standardColors()
	{
		return {
			{QT_TRANSLATE_NOOP("ConductorColorToolButton", "Noir"),        QColor(0x00, 0x00, 0x00)},
			{QT_TRANSLATE_NOOP("ConductorColorToolButton", "Marron"),      QColor(0x7B, 0x3F, 0x00)},
			{QT_TRANSLATE_NOOP("ConductorColorToolButton", "Gris"),        QColor(0x80, 0x80, 0x80)},
			{QT_TRANSLATE_NOOP("ConductorColorToolButton", "Bleu"),        QColor(0x00, 0x00, 0xFF)},
			{QT_TRANSLATE_NOOP("ConductorColorToolButton", "Vert"),        QColor(0x00, 0x80, 0x00)},
			{QT_TRANSLATE_NOOP("ConductorColorToolButton", "Rouge"),       QColor(0xFF, 0x00, 0x00)},
			{QT_TRANSLATE_NOOP("ConductorColorToolButton", "Orange"),      QColor(0xFF, 0x80, 0x00)},
			{QT_TRANSLATE_NOOP("ConductorColorToolButton", "Violet"),      QColor(0x80, 0x00, 0x80)},
			{QT_TRANSLATE_NOOP("ConductorColorToolButton", "Blanc"),       QColor(0xFF, 0xFF, 0xFF)},
		};
	}

	const int MAX_RECENT = 6;
}

/**
	@brief ConductorColorToolButton::ConductorColorToolButton
	@param editor : the diagram editor this button acts on
	@param parent
*/
ConductorColorToolButton::ConductorColorToolButton(QETDiagramEditor *editor, QWidget *parent) :
	QToolButton(parent),
	m_editor(editor)
{
	setPopupMode(QToolButton::InstantPopup);
	setToolTip(tr("Couleur de conducteur"));
	setStatusTip(tr("Applique une couleur aux conducteurs sélectionnés, et l'utilise pour les prochains conducteurs tracés",
			"status bar tip"));

	m_current = LastUsedStyle::hasConductorColor() ? LastUsedStyle::conductorColor()
						      : QColor(Qt::black);
	setMenu(new QMenu(this));
	rebuildMenu();
	setSwatch(m_current);
}

/**
	@brief ConductorColorToolButton::currentView
	@return the folio being edited, or nullptr when no project is open.
	Goes through the project view because QETDiagramEditor keeps its own
	currentDiagramView() private.
*/
DiagramView *ConductorColorToolButton::currentView() const
{
	ProjectView *pv = m_editor ? m_editor->currentProjectView() : nullptr;
	return pv ? pv->currentDiagram() : nullptr;
}

/**
	@brief ConductorColorToolButton::updateEnabledState
	Greyed out when there is no folio to act on, or when the project is
	read-only -- the same rule the other conductor actions follow.
*/
void ConductorColorToolButton::updateEnabledState()
{
	DiagramView *dv = currentView();
	setEnabled(dv && dv->diagram() && !dv->diagram()->isReadOnly());
}

/**
	@brief ConductorColorToolButton::rebuildMenu
	The standard colours never change; the recent ones do, so the menu is
	rebuilt rather than kept in sync entry by entry.
*/
void ConductorColorToolButton::rebuildMenu()
{
	QMenu *m = menu();
	m->clear();

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
	connect(other, &QAction::triggered, this, &ConductorColorToolButton::chooseOtherColor);
}

/**
	@brief ConductorColorToolButton::applyColor
	Recolour the selected conductors, and remember the colour for the next
	one drawn.
	@param color
*/
void ConductorColorToolButton::applyColor(const QColor &color)
{
	if (!color.isValid()) {
		return;
	}

		//Always: this is the half that works with nothing selected.
	LastUsedStyle::setConductorColor(color);
	rememberRecent(color);
	setSwatch(color);

	DiagramView *dv = currentView();
	if (!dv) {
		return;
	}
	Diagram *diagram = dv->diagram();
	if (!diagram || diagram->isReadOnly()) {
		return;
	}

	DiagramContent dc(diagram);
	const auto selected_conductors = dc.conductors(DiagramContent::AnyConductor);
	if (selected_conductors.isEmpty()) {
		return;
	}

		//A wire drawn across a junction is several Conductor objects
		//sharing one electrical potential; selecting one segment must
		//still recolour the whole potential, the way the F2 dialog's
		//"apply to all" already does.
	QSet<Conductor *> conductors;
	for (Conductor *conductor : selected_conductors)
	{
		conductors << conductor;
		conductors += conductor->relatedPotentialConductors();
	}

	QUndoCommand *undo = new QUndoCommand(tr("Modifier la couleur de %n conducteur(s)",
						 "undo caption", conductors.count()));
	int changed = 0;
	for (Conductor *conductor : conductors)
	{
		ConductorProperties before = conductor->properties();
		if (before.color == color) {
			continue;
		}
		ConductorProperties after = before;
		after.color = color;

		QVariant old_value, new_value;
		old_value.setValue(before);
		new_value.setValue(after);
		new QPropertyUndoCommand(conductor, "properties", old_value, new_value, undo);
		++changed;
	}

		//Every selected conductor was already this colour: pushing an
		//empty command would put a no-op step in the undo stack.
	if (changed) {
		diagram->undoStack().push(undo);
	} else {
		delete undo;
	}
}

/**
	@brief ConductorColorToolButton::chooseOtherColor
*/
void ConductorColorToolButton::chooseOtherColor()
{
	const QColor c = QColorDialog::getColor(m_current, this,
					        tr("Choisir une couleur de conducteur"));
	if (c.isValid()) {
		applyColor(c);
	}
}

/**
	@brief ConductorColorToolButton::rememberRecent
	Most recent first, no duplicates, capped.
	@param color
*/
void ConductorColorToolButton::rememberRecent(const QColor &color)
{
		//A colour that is already one row up in the standard list would
		//only appear twice, under a hex name it does not need.
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
	@brief ConductorColorToolButton::setSwatch
	@param color
*/
void ConductorColorToolButton::setSwatch(const QColor &color)
{
	m_current = color;
	setIcon(swatchIcon(color));
}

/**
	@brief ConductorColorToolButton::swatchIcon
	@param color
	@return a plain square of that colour, outlined so that white and very
	light colours are still visible against the toolbar.
*/
QIcon ConductorColorToolButton::swatchIcon(const QColor &color)
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
