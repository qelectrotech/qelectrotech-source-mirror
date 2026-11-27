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
#ifndef EPLANDXFREADER_H
#define EPLANDXFREADER_H

#include <QHash>
#include <QLineF>
#include <QPair>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

/**
	@brief A placed text of an EPLAN DXF: free TEXT or a visible ATTRIB,
	in drawing coordinates (mm, y up).
*/
struct EplanDxfText
{
	QString text;
	QString tag;          ///< attribute tag, empty for a free TEXT
	QPointF anchor;       ///< insertion point, or alignment point when aligned
	QPointF insert;       ///< insertion point, also when aligned
	double height = 2.5;
	double rotation = 0;  ///< degrees, counter-clockwise
	int halign = 0;       ///< 0 left, 1 centre, 2 right
	int valign = 0;       ///< 0 baseline, 1 bottom, 2 middle, 3 top
};

/**
	@brief What an EPLAN macro says about a connection point of a PLC: its
	function definition (category 300, groups and kinds A1382/A1383) and
	PLC properties (P20405, P20407, P20400). Empty for DXF data.
*/
struct EplanPlcPoint
{
	enum Role {
		None,
		Signal,   ///< the DI, DQ, AI or AQ terminal of a channel
		Part,     ///< another terminal of a PLC connection, e.g. "U-" of an analog input
		Supply    ///< a supply or reference terminal (L+, M, 24V, 0V)
	};
	Role role = None;
	bool input = true;     ///< Signal: input (DI, AI) or output (DQ, AQ)
	bool analog = false;   ///< Signal: AI or AQ
	QString channel;       ///< Signal: channel name, e.g. "DI1", "AQ0"; empty when the macro numbers none
	int number = -1;       ///< Signal: channel number, -1 when unknown
	QString key;           ///< P20407 as written; the terminals of one channel share it
	QString data_type;     ///< P20405, e.g. "BOOL", "WORD"
	QString address;       ///< P20400, e.g. "DIx.0"
	QString function_text; ///< P20011
};

/**
	@brief A connection point found from EPLAN attributes.
*/
struct EplanDxfPin
{
	QPointF pos;
	QString name;         ///< terminal designation, e.g. "X10:1" or "L(+)"
	QString function;     ///< connection description, e.g. "DI a.0"
	EplanPlcPoint plc;    ///< PLC channel data of an EPLAN macro
};

/**
	@brief An empty EPLAN text field the schematic fills in per placement
	(PLC address, symbolic address, function text).
*/
struct EplanDxfField
{
	QString tag;
	QPointF anchor;
	int halign = 0;
	int valign = 0;
	bool of_pin = false;  ///< belongs to a connection point block (a channel)
};

/**
	@brief Minimal reader for the ASCII DXF files EPLAN exports
	(EPLAN Data Portal DXF packages, manufacturer downloads).

	Reads LINE, LWPOLYLINE/POLYLINE (with bulges), ARC, CIRCLE, TEXT and
	INSERT with ATTRIBs; block references are resolved recursively.
	Connection points and fields are recognised by the EPLAN attribute tags
	they carry; EPLAN black-box frames ("Symbol14" blocks) are reported as
	boxes, one per part of a multi-part device.
*/
class EplanDxfReader
{
	public:
		bool read(const QByteArray &data);
		QString errorString() const { return m_error; }

		QVector<QLineF> lines() const { return m_lines; }
		QVector<QPair<QPointF, double>> circles() const { return m_circles; }
		QVector<EplanDxfText> texts() const { return m_texts; }
		QVector<EplanDxfPin> pins() const { return m_pins; }
		QVector<EplanDxfField> fields() const { return m_fields; }
		QVector<QRectF> boxes() const { return m_boxes; }
		/// lines and circles placed directly in the drawing, not in a block
		QVector<QLineF> modelLines() const { return m_model_lines; }
		QVector<QPair<QPointF, double>> modelCircles() const { return m_model_circles; }
		QString deviceTag() const { return m_device_tag; }

	private:
		struct Entity
		{
			QString type, layer, block, tag, text;
			QPointF p1, p2, align;
			double r = 0, a1 = 0, a2 = 360, h = 2.5, rot = 0;
			double sx = 1, sy = 1;
			int halign = 0, valign = 0, flags = 0;
			bool has_align = false;
			QVector<QPointF> pts;
			QVector<double> bulges;
			bool closed = false;
			QVector<Entity> attribs;
		};
		struct Block
		{
			QPointF base;
			QVector<Entity> entities;
		};
		struct Transform
		{
			QPointF origin;  ///< insertion point
			QPointF base;    ///< block base point
			double sx = 1, sy = 1, rot = 0;
			QPointF map(const QPointF &p) const;
		};

		void parse(const QVector<QPair<int, QString>> &codes);
		void collect(const Entity &e, const QVector<Transform> &stack, int depth,
			     QVector<QLineF> &block_lines);
		void collectAttribs(const Entity &insert, const QVector<Transform> &stack,
				    const QVector<QLineF> &block_lines);
		static QPointF apply(const QVector<Transform> &stack, const QPointF &p);

		QHash<QString, Block> m_blocks;
		QVector<Entity> m_entities;
		QVector<QLineF> m_lines;
		QVector<QPair<QPointF, double>> m_circles;
		QVector<EplanDxfText> m_texts;
		QVector<EplanDxfPin> m_pins;
		QVector<EplanDxfField> m_fields;
		QVector<QRectF> m_boxes;
		QVector<QLineF> m_model_lines;
		QVector<QPair<QPointF, double>> m_model_circles;
		QString m_device_tag;
		QString m_error;
};

#endif // EPLANDXFREADER_H
