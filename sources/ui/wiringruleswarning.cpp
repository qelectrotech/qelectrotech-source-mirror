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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "wiringruleswarning.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

/**
	@brief WiringRulesWarning::create
	A warning box shown wherever the wires-per-terminal rules are set
	(discussion #1158, review of #1272): the rules count wires on a terminal
	as QElectroTech models them today, and would have to be redesigned if
	wires and conductors become separate objects, so they are experimental.
	@param parent
	@return the box, a framed icon and text
*/
QWidget *WiringRulesWarning::create(QWidget *parent)
{
	auto frame = new QFrame(parent);
	frame->setFrameShape(QFrame::StyledPanel);

	auto icon = new QLabel(frame);
	const int size = frame->style()->pixelMetric(QStyle::PM_SmallIconSize);
	icon->setPixmap(frame->style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(size, size));
	icon->setAlignment(Qt::AlignTop);

	auto text = new QLabel(QApplication::translate(
			"WiringRulesWarning",
			"Fonction expérimentale. Ces règles comptent les conducteurs tels "
			"que QElectroTech les représente aujourd'hui ; elles pourraient "
			"changer, et vos réglages devoir être refaits, si les fils et les "
			"conducteurs deviennent des objets distincts dans une version future."),
			frame);
	text->setWordWrap(true);

	auto layout = new QHBoxLayout(frame);
	layout->addWidget(icon);
	layout->addWidget(text, 1);
	return frame;
}
