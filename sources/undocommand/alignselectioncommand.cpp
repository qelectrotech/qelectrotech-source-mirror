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
#include "alignselectioncommand.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../alignment.h"
#include "../diagram.h"
#include "../diagramcontent.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/independenttextitem.h"

#include <QSettings>

/**
	@brief AlignSelectionCommand::AlignSelectionCommand
	Works out the movement of every selected item for @a mode. Nothing
	moves until the command is pushed.
	Elements redraw their conductors themselves when their position
	changes (see Element's constructor), so a plain "pos" property undo
	per item is enough.
	@param diagram : diagram whose selection is aligned
	@param mode : what to align the selection to
	@param parent : parent undo command
*/
AlignSelectionCommand::AlignSelectionCommand(Diagram *diagram, Mode mode, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_diagram(diagram)
{
	Q_UNUSED(mode)

	DiagramContent dc(diagram);
	m_locked_count = dc.removeNonMovableItems();

	QSettings settings;
	const int x_grid = settings.value(QStringLiteral("diagrameditor/Xgrid"), Diagram::xGrid).toInt();
	const int y_grid = settings.value(QStringLiteral("diagrameditor/Ygrid"), Diagram::yGrid).toInt();
	const qreal text_divisor = settings.value(TextGrid::settings_key, 1).toReal();

	auto move = [this](QGraphicsObject *item, const QPointF &offset)
	{
		if (!offset.isNull())
			new QPropertyUndoCommand(item, "pos", item->pos(), item->pos() + offset, this);
	};

		//Each kind goes where dragging it would have left it: symbols and
		//pictures on the folio grid, free texts on the text grid.
		//Shapes are left out: they are made of several points and no single
		//one of them is the obvious one to snap.
	for (Element *element : std::as_const(dc.m_elements))
		move(element, Alignment::gridOffset(element->pos(), x_grid, y_grid));
	for (DiagramImageItem *image : std::as_const(dc.m_images))
		move(image, Alignment::gridOffset(image->pos(), x_grid, y_grid));
	for (IndependentTextItem *text : std::as_const(dc.m_text_fields))
		move(text, Alignment::gridOffset(text->pos(), x_grid, y_grid, text_divisor));

	setText(QObject::tr("Aligner %n objet(s) sur la grille", "", childCount()));
}

/**
	@brief AlignSelectionCommand::undo
*/
void AlignSelectionCommand::undo()
{
	if (m_diagram)
		m_diagram->showMe();
	QUndoCommand::undo();
}

/**
	@brief AlignSelectionCommand::redo
*/
void AlignSelectionCommand::redo()
{
	if (m_diagram)
		m_diagram->showMe();
	QUndoCommand::redo();
}

/**
	@brief AlignSelectionCommand::isValid
	@return true if this command moves at least one item.
*/
bool AlignSelectionCommand::isValid() const
{
	return childCount() > 0;
}

/**
	@return the number of items this command moves.
*/
int AlignSelectionCommand::movedCount() const
{
	return childCount();
}

/**
	@return the number of selected items left in place because their
	position is locked.
*/
int AlignSelectionCommand::lockedCount() const
{
	return m_locked_count;
}
