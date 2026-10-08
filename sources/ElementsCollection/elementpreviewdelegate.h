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
#ifndef ELEMENTPREVIEWDELEGATE_H
#define ELEMENTPREVIEWDELEGATE_H

#include <QHash>
#include <QIcon>
#include <QStyledItemDelegate>

/**
	@brief The ElementPreviewDelegate class
	Draws the items of an element collection tree with icons that read on
	the current palette. Element previews are black line art drawn for a
	white sheet; on a dark palette this delegate hands the view the same
	picture with its lightness inverted (QET::Palette::forPalette), so the
	ink is light on the dark row. Colored icons, such as folders, are left
	alone, and nothing changes on a light palette. Adapted icons are kept
	per source icon, so a repaint costs a hash lookup.
*/
class ElementPreviewDelegate : public QStyledItemDelegate
{
	Q_OBJECT

	public:
		using QStyledItemDelegate::QStyledItemDelegate;

	protected:
		void initStyleOption(QStyleOptionViewItem *option,
		                     const QModelIndex &index) const override;

	private:
		mutable QHash<qint64, QIcon> m_dark_icons;
};

#endif
