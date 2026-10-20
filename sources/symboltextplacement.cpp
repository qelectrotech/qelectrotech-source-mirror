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
#include "symboltextplacement.h"

#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/elementtextitemgroup.h"

#include <QApplication>
#include <QCoreApplication>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QUndoCommand>

namespace {

	/// How the lines of the block line up, for a block placed at @a where
Qt::Alignment linesFor(Qt::Alignment where)
{
	if (where & Qt::AlignLeft)  return Qt::AlignRight;
	if (where & Qt::AlignRight) return Qt::AlignLeft;
	return Qt::AlignHCenter;
}

	/// One line of the block: a text, or a group of texts
struct Line
{
	QObject *object;		///< for its properties
	QGraphicsItem *item;	///< for where it stands
	QPointF old_pos;
	QVariant old_alignment;
		/// For a group, where its texts lay: a new alignment lays them out
		/// anew, and undoing it to AlignJustify lays out nothing
	QList<QPair<DynamicElementTextItem *, QPointF>> old_texts;
};

	/// Move @a item so that the top left of its box in the folio is at @a to
void moveTo(QGraphicsItem *item, const QPointF &to)
{
	const QPointF by = to - item->sceneBoundingRect().topLeft();
	QGraphicsItem *parent = item->parentItem();
	const QPointF step = parent ? parent->mapFromScene(by) - parent->mapFromScene(QPointF(0, 0))
								: by;
	item->setPos(item->pos() + step);
}

bool placeOne(Element *element, Qt::Alignment where, QUndoCommand *parent)
{
	const Qt::Alignment lines_alignment = linesFor(where);

	QList<Line> lines;
	for (DynamicElementTextItem *text : element->dynamicTextItems())
		if (!text->parentGroup() && text->isVisible())
			lines.append({text, text, text->pos(), text->property("alignment"), {}});
	for (ElementTextItemGroup *group : element->textGroups())
		if (group->isVisible())
		{
			Line line {group, group, group->pos(), group->property("alignment"), {}};
			for (DynamicElementTextItem *text : group->texts())
				line.old_texts.append({text, text->pos()});
			lines.append(line);
		}
	if (lines.isEmpty())
		return false;

		//The lines first take the alignment of the block: a group lays its
		//texts out anew, and later changes of a text keep that side.
	for (const Line &line : lines)
	{
		Qt::Alignment a = line.old_alignment.value<Qt::Alignment>();
		if (qobject_cast<ElementTextItemGroup *>(line.object))
			a = lines_alignment == Qt::AlignHCenter ? Qt::AlignVCenter : lines_alignment;
		else
			a = (a & Qt::AlignVertical_Mask) | lines_alignment;
		line.object->setProperty("alignment", QVariant::fromValue(a));
	}

	QRectF block;
	for (const Line &line : lines)
		block |= line.item->sceneBoundingRect();

	const QRectF body = element->mapRectToScene(element->drawingRect());
	const qreal gap = SymbolTextPlacement::gap;
	QRectF to = block;
	if (where & Qt::AlignLeft)        to.moveRight(body.left() - gap);
	else if (where & Qt::AlignRight)  to.moveLeft(body.right() + gap);
	else                              to.moveLeft(body.center().x() - block.width() / 2);
	if (where & Qt::AlignTop)         to.moveBottom(body.top() - gap);
	else if (where & Qt::AlignBottom) to.moveTop(body.bottom() + gap);
	else                              to.moveTop(body.center().y() - block.height() / 2);

	for (const Line &line : lines)
	{
		const QRectF box = line.item->sceneBoundingRect();
		const qreal y = to.top() + box.top() - block.top();
		const qreal x = lines_alignment == Qt::AlignRight ? to.right() - box.width()
					  : lines_alignment == Qt::AlignLeft  ? to.left()
					  : to.center().x() - box.width() / 2;
		moveTo(line.item, QPointF(x, y));

		const QVariant alignment = line.object->property("alignment");
		if (alignment != line.old_alignment)
			new QPropertyUndoCommand(line.object, "alignment", line.old_alignment, alignment, parent);
		if (line.item->pos() != line.old_pos)
			new QPropertyUndoCommand(line.object, "pos", line.old_pos, line.item->pos(), parent);
			//The group's texts too: undoing its alignment to AlignJustify
			//would leave them where the new alignment put them
		for (const auto &text : line.old_texts)
			if (text.first->pos() != text.second)
				new QPropertyUndoCommand(text.first, "pos", text.second, text.first->pos(), parent);
	}
	return true;
}

} // namespace

QUndoCommand *SymbolTextPlacement::place(const QList<Element *> &elements, Qt::Alignment where)
{
	auto *undo = new QUndoCommand(
				QCoreApplication::translate("SymbolTextPlacement", "Place symbol texts"));
	bool any = false;
	for (Element *element : elements)
		any |= placeOne(element, where, undo);
	if (!any || !undo->childCount()) {
		delete undo;
		return nullptr;
	}
	return undo;
}

QIcon SymbolTextPlacement::icon(Qt::Alignment where)
{
	const Qt::Alignment lines = linesFor(where);
	const QPalette palette = QApplication::palette();
	QIcon icon;
	for (int size : {16, 22, 24, 32, 48})
	{
		QPixmap pixmap(size, size);
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.scale(size / 24.0, size / 24.0);

			//The symbol
		painter.setPen(Qt::NoPen);
		painter.setBrush(palette.color(QPalette::Mid));
		painter.drawRect(QRectF(8, 8, 8, 8));

			//Two lines of text, a long one over a short one
		const qreal right = (where & Qt::AlignLeft) ? 6.8 : (where & Qt::AlignRight) ? 24 : 19;
		const qreal left = (where & Qt::AlignLeft) ? 0 : (where & Qt::AlignRight) ? 17.2 : 5;
		const qreal top = (where & Qt::AlignTop) ? 0 : (where & Qt::AlignBottom) ? 16.6 : 8.3;
		painter.setBrush(palette.color(QPalette::WindowText));
		for (int i = 0; i < 2; ++i)
		{
			const qreal width = i == 0 ? right - left : (right - left) * 0.6;
			const qreal x = lines == Qt::AlignRight ? right - width
						  : lines == Qt::AlignLeft  ? left
						  : (left + right - width) / 2;
			painter.drawRect(QRectF(x, top + i * 4.2, width, 3.2));
		}
		painter.end();
		icon.addPixmap(pixmap);
	}
	return icon;
}
