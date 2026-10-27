// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef CONDUCTORLAYOUT_H
#define CONDUCTORLAYOUT_H

#include <optional>
#include <QPointF>

/**
	@brief The ConductorLayout struct
	Where a wire sits and how its text was placed: the x, y, freezeLabel,
	userx, usery and rotation of a saved wire. No text position (rotation)
	means the user did not move (rotate) the text. Kept apart from
	projectdatabase.h so that Conductor can name it without the SQL headers.
*/
struct ConductorLayout
{
	qreal x = 0;
	qreal y = 0;
	bool freeze_label = false;
	std::optional<QPointF> text_pos;
	std::optional<qreal> text_rotation;
	bool operator==(const ConductorLayout &o) const {
		return x == o.x && y == o.y && freeze_label == o.freeze_label
			&& text_pos == o.text_pos && text_rotation == o.text_rotation; }
};

#endif // CONDUCTORLAYOUT_H
