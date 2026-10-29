// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef ELEMENTPLACEMENT_H
#define ELEMENTPLACEMENT_H

#include <QString>

/**
	@brief The ElementPlacement struct
	Where a placed symbol comes from and sits, as Element::toXml() writes
	it: the type (its definition's location), x, y, z, the orientation
	(0 to 3 quarter turns) and the two mirrors. Kept apart from
	projectdatabase.h so that Element can name it without the SQL headers.
*/
struct ElementPlacement
{
	QString type;
	qreal x = 0;
	qreal y = 0;
	qreal rotation = 0;
	bool horizontal_mirror = false;
	bool vertical_mirror = false;
	qreal z = 10;
	bool operator==(const ElementPlacement &o) const {
		return type == o.type && x == o.x && y == o.y && rotation == o.rotation
			&& horizontal_mirror == o.horizontal_mirror
			&& vertical_mirror == o.vertical_mirror && z == o.z; }
};

#endif // ELEMENTPLACEMENT_H
