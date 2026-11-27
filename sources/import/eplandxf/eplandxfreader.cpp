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
#include "eplandxfreader.h"

#include <QRegularExpression>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {
	// EPLAN attribute tags. A connection point block carries its designation
	// under one of the name tags; the function tags give its description.
	const QStringList PIN_NAME_TAGS {QStringLiteral("Connection_Name"),
					 QStringLiteral("17_20036"),
					 QStringLiteral("17_20183")};
	const QStringList PIN_FUNC_TAGS {QStringLiteral("Connection_Description"),
					 QStringLiteral("17_20039")};
	// numbered connection points by property number (name / description)
	const QStringList PIN_NUMBERED_TAGS {QStringLiteral("17_20028["), QStringLiteral("17_20029[")};
	const QStringList DEVICE_TAGS {QStringLiteral("DT_Name"),
				       QStringLiteral("17_20010")};
	const QStringList SKIP_TAGS {QStringLiteral("DT_Name"), QStringLiteral("17_20010"),
				     QStringLiteral("Overview_Xref"),
				     QStringLiteral("Cross_Reference"),
				     // legend item number of a panel layout
				     QStringLiteral("Legend_Item"), QStringLiteral("17_20450")};
	const QStringList FIELD_TAGS {QStringLiteral("PLC_Address"),
				      QStringLiteral("Symbolic_Address"),
				      QStringLiteral("Function_Text")};

	bool startsWithAny(const QString &s, const QStringList &list)
	{
		for (const QString &p : list) {
			if (s.startsWith(p)) return true;
		}
		return false;
	}

	/// segments for an arc so that no chord is more than 0.15 mm off the curve
	int arcSteps(double sweep_rad, double radius)
	{
		const double s = 0.15;
		if (radius <= s) return 4;
		const double step = 2 * qAcos(1 - s / radius);
		return qMax(4, int(std::ceil(qAbs(sweep_rad) / step)));
	}

	/// Points of a polyline segment with a DXF bulge (tan of a quarter of
	/// the included angle).
	QVector<QPointF> bulgeArc(const QPointF &a, const QPointF &b, double bulge)
	{
		const double chord = QLineF(a, b).length();
		if (qFuzzyIsNull(bulge) || chord < 1e-9) return {b};
		const double theta = 4 * qAtan(bulge);
		const double radius = chord / (2 * qSin(qAbs(theta) / 2));
		// centre: from the chord midpoint along the left normal for a
		// counter-clockwise arc (positive bulge)
		const QPointF mid = (a + b) / 2;
		const double h = radius * qCos(qAbs(theta) / 2);
		QPointF n(-(b.y() - a.y()) / chord, (b.x() - a.x()) / chord);
		if (bulge < 0) n = -n;
		const QPointF centre = mid + n * (qAbs(theta) > M_PI ? -h : h);
		const double start = qAtan2(a.y() - centre.y(), a.x() - centre.x());
		const int steps = arcSteps(theta, radius);
		QVector<QPointF> out;
		for (int i = 1; i <= steps; ++i) {
			const double t = start + theta * i / steps;
			out << QPointF(centre.x() + radius * qCos(t), centre.y() + radius * qSin(t));
		}
		out.last() = b;
		return out;
	}
}

QPointF EplanDxfReader::Transform::map(const QPointF &p) const
{
	const double x = (p.x() - base.x()) * sx;
	const double y = (p.y() - base.y()) * sy;
	const double r = qDegreesToRadians(rot);
	return origin + QPointF(x * qCos(r) - y * qSin(r), x * qSin(r) + y * qCos(r));
}

QPointF EplanDxfReader::apply(const QVector<Transform> &stack, const QPointF &p)
{
	QPointF q = p;
	for (int i = stack.size() - 1; i >= 0; --i) q = stack.at(i).map(q);
	return q;
}

/**
	Read an ASCII DXF. @return false with errorString() set when the data is
	not a DXF this reader understands.
*/
bool EplanDxfReader::read(const QByteArray &data)
{
	m_error.clear();
	// AutoCAD 2007 and later write UTF-8, older files the ANSI code page.
	const bool utf8 = data.contains("AC1021") || data.contains("AC1024")
			  || data.contains("AC1027") || data.contains("AC1032");
	const QString text = utf8 ? QString::fromUtf8(data) : QString::fromLatin1(data);
	const QStringList lines = text.split(QRegularExpression(QStringLiteral("\r?\n")));
	QVector<QPair<int, QString>> codes;
	codes.reserve(lines.size() / 2);
	for (int i = 0; i + 1 < lines.size(); i += 2) {
		bool ok = false;
		const int code = lines.at(i).trimmed().toInt(&ok);
		if (!ok) {
			m_error = QStringLiteral("Not an ASCII DXF file (line %1)").arg(i + 1);
			return false;
		}
		codes.append({code, lines.at(i + 1).trimmed()});
	}
	parse(codes);
	if (m_entities.isEmpty()) {
		m_error = QStringLiteral("The DXF contains no drawing");
		return false;
	}
	QVector<QLineF> unused;
	for (const Entity &e : std::as_const(m_entities)) collect(e, {}, 0, unused);

	// EPLAN writes the macro's own description under it: the macro file
	// (".ema"), representation type, variant, date and macro name, one text
	// per line at the same x. None of it belongs in a symbol.
	for (const EplanDxfText &ema : QVector<EplanDxfText>(m_texts)) {
		if (!ema.tag.isEmpty() || !ema.text.trimmed().endsWith(QLatin1String(".ema"), Qt::CaseInsensitive))
			continue;
		m_texts.erase(std::remove_if(m_texts.begin(), m_texts.end(), [&](const EplanDxfText &t) {
			return t.tag.isEmpty() && qAbs(t.insert.x() - ema.insert.x()) < 0.5
			       && t.insert.y() <= ema.insert.y() + 0.5 && t.insert.y() >= ema.insert.y() - 20;
		}), m_texts.end());
	}
	return true;
}

/**
	Split the group code stream into blocks and model-space entities.
*/
void EplanDxfReader::parse(const QVector<QPair<int, QString>> &codes)
{
	QString section, block_name;
	bool in_block = false;
	int open_poly = -1;    // index of a POLYLINE collecting VERTEX entities
	int open_insert = -1;  // index of an INSERT collecting ATTRIB entities
	int i = 0;
	const int n = codes.size();
	auto target = [&]() -> QVector<Entity>& {
		return in_block ? m_blocks[block_name].entities : m_entities;
	};
	auto skipToNext = [&]() { for (++i; i < n && codes.at(i).first != 0; ++i) {} };

	while (i < n) {
		const int code = codes.at(i).first;
		const QString value = codes.at(i).second;
		if (code != 0) { ++i; continue; }
		if (value == QLatin1String("SECTION")) {
			section = (i + 1 < n) ? codes.at(i + 1).second : QString();
			i += 2;
			continue;
		}
		if (value == QLatin1String("ENDSEC")) { section.clear(); ++i; continue; }
		if (section != QLatin1String("BLOCKS") && section != QLatin1String("ENTITIES")) { ++i; continue; }
		if (value == QLatin1String("BLOCK")) {
			QPointF base;
			block_name.clear();
			for (++i; i < n && codes.at(i).first != 0; ++i) {
				const int c = codes.at(i).first;
				const QString &v = codes.at(i).second;
				if (c == 2) block_name = v;
				else if (c == 10) base.setX(v.toDouble());
				else if (c == 20) base.setY(v.toDouble());
			}
			m_blocks[block_name].base = base;
			in_block = true;
			open_poly = open_insert = -1;
			continue;
		}
		if (value == QLatin1String("ENDBLK")) {
			in_block = false;
			open_poly = open_insert = -1;
			skipToNext();
			continue;
		}
		if (value == QLatin1String("SEQEND")) {
			open_poly = open_insert = -1;
			skipToNext();
			continue;
		}

		Entity e;
		e.type = value;
		const bool is_mtext = e.type == QLatin1String("MTEXT");
		const bool is_text = e.type == QLatin1String("TEXT") || e.type == QLatin1String("ATTRIB") || is_mtext;
		const bool is_lw = e.type == QLatin1String("LWPOLYLINE");
		QString subclass;  // an ATTRIB repeats code 72 (as an MTEXT flag) in AcDbAttribute
		for (++i; i < n && codes.at(i).first != 0; ++i) {
			const int c = codes.at(i).first;
			const QString &v = codes.at(i).second;
			const double d = v.toDouble();
			switch (c) {
				case 100: subclass = v; break;
				case 1: if (is_mtext) e.text += v; else e.text = v; break;
				case 3: if (is_mtext) e.text += v; break;  // leading MTEXT chunks
				case 71:
					// MTEXT attachment point 1..9: rows top/middle/bottom, columns left/centre/right
					if (is_mtext && v.toInt() >= 1 && v.toInt() <= 9) {
						const int a = v.toInt() - 1;
						e.halign = a % 3;
						e.valign = 3 - a / 3;
					}
					break;
				case 2: if (e.type == QLatin1String("INSERT")) e.block = v; else e.tag = v; break;
				case 8: e.layer = v; break;
				case 10: if (is_lw) { e.pts.append(QPointF(d, 0)); e.bulges.append(0); } else e.p1.setX(d); break;
				case 20: if (is_lw && !e.pts.isEmpty()) e.pts.last().setY(d); else e.p1.setY(d); break;
				case 11: e.p2.setX(d); e.align.setX(d); e.has_align = true; break;
				case 21: e.p2.setY(d); e.align.setY(d); break;
				case 40: if (is_text) e.h = d; else e.r = d; break;
				case 41: e.sx = d; break;
				case 42:
					if (is_lw && !e.bulges.isEmpty()) e.bulges.last() = d;
					else if (e.type == QLatin1String("VERTEX")) e.bulges = {d};
					else e.sy = d;
					break;
				case 50: if (e.type == QLatin1String("ARC")) e.a1 = d; else e.rot = d; break;
				case 51: e.a2 = d; break;
				case 70:
					e.flags = v.toInt();
					if (is_lw || e.type == QLatin1String("POLYLINE")) e.closed = e.flags & 1;
					break;
				case 72: if (subclass != QLatin1String("AcDbAttribute")) e.halign = v.toInt(); break;
				case 73: if (e.type == QLatin1String("TEXT")) e.valign = v.toInt(); break;
				case 74: if (e.type == QLatin1String("ATTRIB")) e.valign = v.toInt(); break;
				default: break;
			}
		}

		if (e.type == QLatin1String("VERTEX")) {
			if (open_poly >= 0) {
				Entity &poly = target()[open_poly];
				poly.pts.append(e.p1);
				poly.bulges.append(e.bulges.isEmpty() ? 0 : e.bulges.first());
			}
			continue;
		}
		if (e.type == QLatin1String("ATTRIB")) {
			if (open_insert >= 0) target()[open_insert].attribs.append(e);
			continue;
		}
		target().append(e);
		if (e.type == QLatin1String("POLYLINE")) open_poly = target().size() - 1;
		else if (e.type == QLatin1String("INSERT")) open_insert = target().size() - 1;
	}
}

/**
	Resolve one entity (and, for an INSERT, its block) into drawing primitives.
	@param block_lines receives the lines in drawing coordinates, so that the
	caller can place connection points that sit inside a symbol block.
*/
void EplanDxfReader::collect(const Entity &e, const QVector<Transform> &stack,
			     int depth, QVector<QLineF> &block_lines)
{
	auto addPolyline = [&](const QVector<QPointF> &pts) {
		for (int k = 1; k < pts.size(); ++k) {
			const QLineF l(apply(stack, pts.at(k - 1)), apply(stack, pts.at(k)));
			m_lines.append(l);
			block_lines.append(l);
			if (stack.isEmpty()) m_model_lines.append(l);
		}
	};

	if (e.type == QLatin1String("LINE")) {
		addPolyline({e.p1, e.p2});
	} else if (e.type == QLatin1String("LWPOLYLINE") || e.type == QLatin1String("POLYLINE")) {
		if (e.pts.isEmpty()) return;
		QVector<QPointF> pts {e.pts.first()};
		const int count = e.pts.size();
		const int segments = e.closed ? count : count - 1;
		for (int k = 0; k < segments; ++k)
			pts += bulgeArc(e.pts.at(k), e.pts.at((k + 1) % count), e.bulges.value(k));
		addPolyline(pts);
	} else if (e.type == QLatin1String("ARC")) {
		double sweep = e.a2 - e.a1;
		if (sweep <= 0) sweep += 360;
		const int steps = arcSteps(qDegreesToRadians(sweep), e.r);
		QVector<QPointF> pts;
		for (int k = 0; k <= steps; ++k) {
			const double t = qDegreesToRadians(e.a1 + sweep * k / steps);
			pts << e.p1 + QPointF(e.r * qCos(t), e.r * qSin(t));
		}
		addPolyline(pts);
	} else if (e.type == QLatin1String("CIRCLE")) {
		// EPLAN keeps hidden helper circles (connection point marks) on this layer
		if (e.layer == QLatin1String("EPLAN_HIDDEN")) return;
		double scale = 1;
		for (const Transform &t : stack) scale *= qAbs(t.sx);
		m_circles.append({apply(stack, e.p1), e.r * scale});
		if (stack.isEmpty()) m_model_circles.append(m_circles.last());
	} else if (e.type == QLatin1String("TEXT") || e.type == QLatin1String("MTEXT")) {
		const bool mtext = e.type == QLatin1String("MTEXT");
		QString text = e.text;
		if (mtext) {
			// drop MTEXT formatting ("\fArial|b0;", "{...}"); paragraphs become spaces
			static const QRegularExpression format(QStringLiteral("\\\\[A-OQ-Za-oq-z][^;\\\\]*;"));
			text.replace(QStringLiteral("\\P"), QStringLiteral(" "));
			text.remove(format);
			text.remove(QLatin1Char('{'));
			text.remove(QLatin1Char('}'));
			text = text.simplified();
		}
		if (text.trimmed().isEmpty()) return;
		double rot = e.rot;
		for (const Transform &t : stack) rot += t.rot;
		EplanDxfText t;
		t.text = text;
		// an MTEXT is placed by its attachment point, which is its insertion point
		t.anchor = apply(stack, !mtext && (e.halign || e.valign) && e.has_align ? e.align : e.p1);
		t.insert = apply(stack, e.p1);
		t.height = e.h;
		t.rotation = rot;
		t.halign = e.halign;
		t.valign = e.valign;
		m_texts.append(t);
	} else if (e.type == QLatin1String("INSERT") && depth < 8) {
		if (!m_blocks.contains(e.block)) return;
		const Block b = m_blocks.value(e.block);
		Transform tr;
		tr.origin = e.p1;
		tr.base = b.base;
		tr.sx = e.sx;
		tr.sy = e.sy;
		tr.rot = e.rot;
		QVector<Transform> inner = stack;
		inner.append(tr);
		// An EPLAN black-box frame marks one part of a multi-part device; the
		// importer draws its own (shortened) frame for it.
		if (e.block.contains(QLatin1String("Symbol14"))) {
			double x0 = 1e18, y0 = 1e18, x1 = -1e18, y1 = -1e18;
			for (const Entity &c : b.entities) {
				QVector<QPointF> pts = c.pts;
				if (c.type == QLatin1String("LINE")) pts = {c.p1, c.p2};
				for (const QPointF &p : pts) {
					const QPointF q = apply(inner, p);
					x0 = qMin(x0, q.x()); y0 = qMin(y0, q.y());
					x1 = qMax(x1, q.x()); y1 = qMax(y1, q.y());
				}
			}
			if (x1 > x0) m_boxes.append(QRectF(QPointF(x0, y0), QPointF(x1, y1)));
			if (depth == 0) collectAttribs(e, stack, {});
			return;
		}
		QVector<QLineF> own_lines;
		for (const Entity &c : b.entities) collect(c, inner, depth + 1, own_lines);
		block_lines += own_lines;
		// only references placed in the drawing carry EPLAN's attributes
		if (depth == 0) collectAttribs(e, stack, own_lines);
	}
}

/**
	Interpret the attributes of an INSERT: connection points, the device tag,
	empty fields and visible texts.
*/
void EplanDxfReader::collectAttribs(const Entity &insert, const QVector<Transform> &stack,
				    const QVector<QLineF> &block_lines)
{
	QHash<QString, Entity> by_tag;
	for (const Entity &a : insert.attribs) by_tag.insert(a.tag, a);
	for (const QString &t : DEVICE_TAGS) {
		if (!by_tag.value(t).text.isEmpty()) m_device_tag = by_tag.value(t).text;
	}
	auto anchorOf = [&](const Entity &a) {
		return apply(stack, a.has_align && (a.halign || a.valign) ? a.align : a.p1);
	};

	// connection point block: its insertion point is the connection point
	QString pin_name;
	for (const QString &t : PIN_NAME_TAGS) {
		if (!by_tag.value(t).text.isEmpty()) { pin_name = by_tag.value(t).text; break; }
	}
	if (!pin_name.isEmpty()) {
		EplanDxfPin p;
		p.pos = apply(stack, insert.p1);
		p.name = QString(pin_name).replace(QLatin1String(" R:"), QString()).trimmed();
		for (const QString &t : PIN_FUNC_TAGS) {
			if (!by_tag.value(t).text.isEmpty()) { p.function = by_tag.value(t).text.trimmed(); break; }
		}
		m_pins.append(p);
	}

	// several connection points in one symbol block (Connection_Name1..n, or
	// by property number 17_20028[n] with description 17_20029[n]):
	// placed at the line end nearest to its label
	static const QRegularExpression numbered(
		QStringLiteral("^(?:Connection_Name(\\d+)|17_20028\\[(\\d+)\\])$"));
	for (const Entity &a : insert.attribs) {
		const auto m = numbered.match(a.tag);
		if (!m.hasMatch()) continue;
		const QString descr = m.captured(1).isEmpty()
			? by_tag.value(QStringLiteral("17_20029[%1]").arg(m.captured(2))).text
			: by_tag.value(QStringLiteral("Connection_Description") + m.captured(1)).text;
		// a PLC connection point leaves its designation to the project ("DI"
		// channel without a name): its description names the terminal then
		if (a.text.isEmpty() && descr.isEmpty()) continue;
		const QPointF label = anchorOf(a);
		QPointF best = label;
		double best_d = 1e18;
		for (const QLineF &l : block_lines) {
			for (const QPointF &q : {l.p1(), l.p2()}) {
				const double d = QLineF(q, label).length();
				if (d < best_d) { best_d = d; best = q; }
			}
		}
		EplanDxfPin p;
		p.pos = best;
		p.name = a.text.isEmpty() ? descr : a.text;
		p.function = descr.isEmpty() ? by_tag.value(QStringLiteral("Connection_Description")).text : descr;
		m_pins.append(p);
	}

	for (const Entity &a : insert.attribs) {
		const bool invisible = a.flags & 1;
		if (!invisible && a.text.isEmpty() && startsWithAny(a.tag, FIELD_TAGS)) {
			EplanDxfField f;
			f.tag = a.tag;
			f.anchor = anchorOf(a);
			f.halign = a.halign;
			f.valign = a.valign;
			f.of_pin = !pin_name.isEmpty();
			m_fields.append(f);
			continue;
		}
		if (invisible || a.text.isEmpty() || SKIP_TAGS.contains(a.tag)) continue;
		if (startsWithAny(a.tag, PIN_NAME_TAGS) || startsWithAny(a.tag, PIN_FUNC_TAGS)
		    || startsWithAny(a.tag, PIN_NUMBERED_TAGS))
			continue;
		double rot = a.rot;
		for (const Transform &t : stack) rot += t.rot;
		EplanDxfText t;
		t.text = a.text;
		t.tag = a.tag;
		t.anchor = anchorOf(a);
		t.insert = apply(stack, a.p1);
		t.height = a.h;
		t.rotation = rot;
		t.halign = a.halign;
		t.valign = a.valign;
		m_texts.append(t);
	}
}
