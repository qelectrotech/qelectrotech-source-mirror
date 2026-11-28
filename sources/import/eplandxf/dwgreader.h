/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DWGREADER_H
#define DWGREADER_H

#include "eplandxfreader.h"

#include <QByteArray>
#include <QHash>
#include <QLineF>
#include <QString>
#include <QVector>

/**
	@brief Reads the drawing of a symbol from an AutoCAD DWG file (R2004 to
	R2018, AC1018 to AC1032), as manufacturers and part libraries hand them
	out for "other E-CAD programs" (e.g. WSCAD Universe "Download DWG").

	Written after the public "Open Design Specification for .dwg files" of
	the Open Design Alliance: the obfuscated file header, the page and
	section maps, the LZ77-like compressed data pages, the object map
	(AcDb:Handles) and the bit-coded objects (AcDb:AcDbObjects).

	The block inserted in model space is resolved with its nested blocks;
	lines, polylines (with arcs), circles, arcs, solids and texts are
	returned in drawing coordinates (mm, y up), like EplanDxfReader does.
	Connection points are found as drawn by WSCAD: a short stub (up to 5 mm)
	starts on the device frame and points outwards; the terminal sits at its
	outer end and is named after the text right beside it (e.g. "X3.1").
*/
class DwgReader
{
	public:
		bool read(const QByteArray &data);
		QString errorString() const { return m_error; }
		QString version() const { return m_version; }
		QVector<QLineF> lines() const { return m_lines; }
		QVector<QPair<QPointF, double>> circles() const { return m_circles; }
		QVector<EplanDxfText> texts() const { return m_texts; }
		QVector<EplanDxfPin> pins() const { return m_pins; }
		/// name of the block inserted in model space, e.g. the symbol name
		QString blockName() const { return m_block; }

	private:
		struct Object
		{
			int type = 0;
			quint64 handle = 0, owner = 0;
			int mode = -1;               ///< entity mode: 0 owner in handle stream, 1 paper, 2 model space
			bool broken = false, invisible = false;
			QString text, tag;
			QPointF p, q, align;         ///< p: start/centre/insertion, q: end
			double r = 0, a1 = 0, a2 = 0, h = 0, rot = 0, sx = 1, sy = 1, ratio = 1;
			int halign = 0, valign = 0;
			bool closed = false, has_align = false;
			QVector<QPointF> pts;
			QVector<double> bulges;
			QPointF base;                ///< block header: base point
			QVector<quint64> refs;       ///< handles after the common ones (insert: block header)
		};
		struct Transform
		{
			QPointF origin, base;
			double sx = 1, sy = 1, rot = 0;
		};

		bool sections(const QByteArray &data);
		bool readObjects();
		void readObject(qint64 address, quint64 expected);
		void collect(quint64 block, QVector<Transform> stack, int depth);
		void findPins();
		static QPointF apply(const QVector<Transform> &stack, const QPointF &p);
		void addSegments(const Object &o, const QVector<Transform> &stack);

		QString m_version;
		QHash<QString, QByteArray> m_sections;
		QHash<quint64, Object> m_objects;
		QHash<quint64, QVector<quint64>> m_contents;   ///< block header -> its entities
		QVector<QLineF> m_lines;
		QVector<QPair<QPointF, double>> m_circles;
		QVector<EplanDxfText> m_texts;
		QVector<EplanDxfPin> m_pins;
		QVector<QVector<QPointF>> m_outlines;         ///< closed polylines (frame candidates)
		QVector<QLineF> m_stubs;                       ///< single strokes (terminal stub candidates)
		QString m_block;
		QString m_error;
};

#endif // DWGREADER_H
