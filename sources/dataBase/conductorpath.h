// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef CONDUCTORPATH_H
#define CONDUCTORPATH_H

#include <QList>
#include <QString>
#include <QtGlobal>

/**
	@brief The ConductorSegmentRow struct
	One segment of a wire the user reshaped: its direction and its length, as
	the <segment> element of the saved wire holds them. Kept apart from
	projectdatabase.h so that Conductor can name it without the SQL headers.
*/
struct ConductorSegmentRow
{
	bool horizontal = false;
	qreal length = 0;
	bool operator==(const ConductorSegmentRow &o) const {
		return horizontal == o.horizontal && length == o.length; }
};

	/// A segment length as a saved wire holds it: the file keeps six
	/// significant digits, so that is the value a row carries
inline qreal savedSegmentLength(qreal length) {
	return QString("%1").arg(length).toDouble(); }

	/// The segments of a reshaped wire, in order; none for a wire that was
	/// not reshaped
using ConductorPath = QList<ConductorSegmentRow>;

#endif // CONDUCTORPATH_H
