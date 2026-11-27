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
#include "eplanelementbuilder.h"

#include <QSet>
#include <QUuid>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
	const QString STYLE = QStringLiteral(
		"line-style:normal;line-weight:normal;filling:none;color:black");

	// QET's dynamic texts keep an inner margin: offset glyph <-> field top (font 5)
	const double OFF_TOP = 4, OFF_MID = 8, OFF_BOT = 12;

	QString font(int size)
	{
		return QStringLiteral("Liberation Sans,%1,-1,5,50,0,0,0,0,0,Regular").arg(size);
	}

	QString uuidStr() { return QUuid::createUuid().toString(); }

	/// round half to even, as the coordinates of the reference converter
	double rnd(double v) { return std::nearbyint(v); }

	QString num(double v)
	{
		QString s = QString::number(v, 'f', 2);
		while (s.endsWith(QLatin1Char('0'))) s.chop(1);
		if (s.endsWith(QLatin1Char('.'))) s.chop(1);
		return s == QLatin1String("-0") ? QStringLiteral("0") : s;
	}

	QDomElement makeText(QDomDocument &doc, double x, double y, const QString &text,
			     int size = 5, double rotation = 0)
	{
		QDomElement e = doc.createElement(QStringLiteral("text"));
		e.setAttribute(QStringLiteral("x"), num(x));
		e.setAttribute(QStringLiteral("y"), num(y));
		e.setAttribute(QStringLiteral("text"), text);
		e.setAttribute(QStringLiteral("rotation"), num(rotation));
		e.setAttribute(QStringLiteral("font"), font(size));
		e.setAttribute(QStringLiteral("color"), QStringLiteral("#000000"));
		return e;
	}

	QDomElement makeUserText(QDomDocument &doc, double x, double y, double width,
				 const QString &halign, const QString &valign, int size)
	{
		QDomElement e = doc.createElement(QStringLiteral("dynamic_text"));
		e.setAttribute(QStringLiteral("Halignment"), halign);
		e.setAttribute(QStringLiteral("Valignment"), valign);
		e.setAttribute(QStringLiteral("font"), font(size));
		e.setAttribute(QStringLiteral("frame"), QStringLiteral("false"));
		e.setAttribute(QStringLiteral("keep_visual_rotation"), QStringLiteral("false"));
		e.setAttribute(QStringLiteral("rotation"), QStringLiteral("0"));
		e.setAttribute(QStringLiteral("text_from"), QStringLiteral("UserText"));
		e.setAttribute(QStringLiteral("text_width"), num(width));
		e.setAttribute(QStringLiteral("uuid"), uuidStr());
		e.setAttribute(QStringLiteral("x"), num(x));
		e.setAttribute(QStringLiteral("y"), num(y));
		e.setAttribute(QStringLiteral("z"), QStringLiteral("21"));
		QDomElement t = doc.createElement(QStringLiteral("text"));
		t.appendChild(doc.createTextNode(QStringLiteral("…")));
		e.appendChild(t);
		return e;
	}

	/// EPLAN draws many outlines two or three times over and splits straight
	/// lines into short pieces; drawn as they are, the antialiased copies look
	/// thick and blurred. Drop repeats and join touching collinear pieces of
	/// horizontal and vertical lines (coordinates in px).
	QVector<QLineF> cleanLines(const QVector<QLineF> &in)
	{
		auto r2 = [](double v) { return std::round(v * 100) / 100; };
		QMap<double, QVector<QPair<double, double>>> horiz, vert;  // y -> [x0,x1], x -> [y0,y1]
		QVector<QLineF> other;
		QSet<QString> seen;
		for (const QLineF &l : in) {
			const double x1 = r2(l.x1()), y1 = r2(l.y1()), x2 = r2(l.x2()), y2 = r2(l.y2());
			if (x1 == x2 && y1 == y2) continue;
			if (y1 == y2) horiz[y1].append({qMin(x1, x2), qMax(x1, x2)});
			else if (x1 == x2) vert[x1].append({qMin(y1, y2), qMax(y1, y2)});
			else {
				const bool swap = x1 > x2 || (x1 == x2 && y1 > y2);
				const QLineF n = swap ? QLineF(x2, y2, x1, y1) : QLineF(x1, y1, x2, y2);
				const QString key = QStringLiteral("%1,%2,%3,%4").arg(n.x1()).arg(n.y1()).arg(n.x2()).arg(n.y2());
				if (!seen.contains(key)) { seen.insert(key); other << n; }
			}
		}
		auto merge = [](QVector<QPair<double, double>> v) {
			std::sort(v.begin(), v.end());
			QVector<QPair<double, double>> out;
			for (const auto &s : v) {
				if (!out.isEmpty() && s.first <= out.last().second + 0.01)
					out.last().second = qMax(out.last().second, s.second);
				else
					out << s;
			}
			return out;
		};
		QVector<QLineF> out;
		for (auto it = horiz.cbegin(); it != horiz.cend(); ++it)
			for (const auto &s : merge(it.value())) out << QLineF(s.first, it.key(), s.second, it.key());
		for (auto it = vert.cbegin(); it != vert.cend(); ++it)
			for (const auto &s : merge(it.value())) out << QLineF(it.key(), s.first, it.key(), s.second);
		return out + other;
	}

	struct Terminal { double x, y; QString ori, name; };
	struct PinText { double x, y; QString ori; int name_len; };
	struct FieldPx { QString tag; double x, y; int ha, va; };
	struct HLine { double y, x1, x2; };
}

QStringList EplanElementBuilder::terminalNames(const QVector<EplanDxfPin> &pins)
{
	// QET wants unique terminal names; EPLAN repeats them (both sides of a
	// feed-through terminal, channels named after their function only):
	// repeated names are numbered "1.1", "1.2"
	QHash<QString, int> total, seen;
	for (const EplanDxfPin &p : pins) ++total[p.name];
	QStringList out;
	for (const EplanDxfPin &p : pins)
		out << (total.value(p.name) > 1 ? QStringLiteral("%1.%2").arg(p.name).arg(++seen[p.name]) : p.name);
	return out;
}

QDomDocument EplanElementBuilder::build(const EplanSymbolData &data,
					const QMap<QString, QString> &names,
					const QVector<QPair<QString, QString>> &info)
{
	const double S = data.scale;  // px per mm of the drawing (EPLAN: 8 mm grid -> 20 px, QET grid 10 px)
	QDomDocument doc;
	QVector<EplanDxfPin> pins = data.pins;
	{
		const QStringList unique = terminalNames(pins);
		for (int i = 0; i < pins.size(); ++i) pins[i].name = unique.at(i);
	}
	// the first terminal is the grid reference; a drawing without terminals
	// (overview, panel layout) is referenced at its top left corner
	double ox, oy;
	if (!pins.isEmpty()) {
		ox = pins.first().pos.x();
		oy = pins.first().pos.y();
	} else if (!data.lines.isEmpty()) {
		ox = data.lines.first().x1();
		oy = data.lines.first().y1();
		for (const QLineF &l : data.lines) {
			ox = qMin(ox, qMin(l.x1(), l.x2()));
			oy = qMax(oy, qMax(l.y1(), l.y2()));
		}
	} else if (!data.circles.isEmpty()) {
		// a front view of round parts only (push buttons, lamps)
		ox = data.circles.first().first.x() - data.circles.first().second;
		oy = data.circles.first().first.y() + data.circles.first().second;
		for (const auto &c : data.circles) {
			ox = qMin(ox, c.first.x() - c.second);
			oy = qMax(oy, c.first.y() + c.second);
		}
	} else {
		return doc;
	}
	auto T = [&](const QPointF &p) { return QPointF((p.x() - ox) * S, (oy - p.y()) * S); };

	QList<QDomElement> parts;
	QVector<double> xs, ys;
	QVector<HLine> hlines;
	QVector<QLineF> px_lines;
	for (const QLineF &l : data.lines) px_lines << QLineF(T(l.p1()), T(l.p2()));
	for (const QLineF &l : cleanLines(px_lines)) {
		const QPointF a = l.p1(), c = l.p2();
		xs << a.x() << c.x();
		ys << a.y() << c.y();
		if (num(a.y()) == num(c.y()))
			hlines.append({num(a.y()).toDouble(), qMin(a.x(), c.x()), qMax(a.x(), c.x())});
		QDomElement e = doc.createElement(QStringLiteral("line"));
		e.setAttribute(QStringLiteral("x1"), num(a.x()));
		e.setAttribute(QStringLiteral("y1"), num(a.y()));
		e.setAttribute(QStringLiteral("x2"), num(c.x()));
		e.setAttribute(QStringLiteral("y2"), num(c.y()));
		e.setAttribute(QStringLiteral("end1"), QStringLiteral("none"));
		e.setAttribute(QStringLiteral("end2"), QStringLiteral("none"));
		e.setAttribute(QStringLiteral("length1"), QStringLiteral("1.5"));
		e.setAttribute(QStringLiteral("length2"), QStringLiteral("1.5"));
		e.setAttribute(QStringLiteral("antialias"), QStringLiteral("false"));
		e.setAttribute(QStringLiteral("style"), STYLE);
		parts << e;
	}
	for (const auto &c : data.circles) {
		const double r = c.second;
		const QPointF a = T(c.first + QPointF(-r, r));
		if (data.lines.isEmpty()) {  // round parts only: the circles span the drawing
			xs << a.x() << a.x() + 2 * r * S;
			ys << a.y() << a.y() + 2 * r * S;
		}
		QDomElement e = doc.createElement(QStringLiteral("ellipse"));
		e.setAttribute(QStringLiteral("x"), num(a.x()));
		e.setAttribute(QStringLiteral("y"), num(a.y()));
		e.setAttribute(QStringLiteral("width"), num(2 * r * S));
		e.setAttribute(QStringLiteral("height"), num(2 * r * S));
		e.setAttribute(QStringLiteral("antialias"), QStringLiteral("true"));
		e.setAttribute(QStringLiteral("style"), STYLE);
		parts << e;
	}
	for (const QRectF &r : data.filled) {
		const QPointF a = T(QPointF(r.left(), r.bottom()));  // upper left (y up)
		xs << a.x() << a.x() + r.width() * S;
		ys << a.y() << a.y() + r.height() * S;
		QDomElement e = doc.createElement(QStringLiteral("rect"));
		e.setAttribute(QStringLiteral("x"), num(a.x()));
		e.setAttribute(QStringLiteral("y"), num(a.y()));
		e.setAttribute(QStringLiteral("width"), num(r.width() * S));
		e.setAttribute(QStringLiteral("height"), num(r.height() * S));
		e.setAttribute(QStringLiteral("rx"), QStringLiteral("0"));
		e.setAttribute(QStringLiteral("ry"), QStringLiteral("0"));
		e.setAttribute(QStringLiteral("antialias"), QStringLiteral("false"));
		e.setAttribute(QStringLiteral("style"), QString(STYLE).replace(
			QStringLiteral("filling:none"), QStringLiteral("filling:black")));
		parts << e;
	}

	QVector<QRectF> boxes;  // space taken by horizontal texts
	for (const EplanDxfText &t : data.texts) {
		// a smaller scale (5 mm grid) leaves less room: allow the next smaller font there
		const int size = qMax(S < 2.5 ? 4 : 5, int(rnd(t.height * S * 1.1)));
		const double w = 0.6 * size * t.text.size();  // rough text width in px
		QPointF a = T(t.anchor);
		double rot = std::fmod(t.rotation, 360.0);
		if (qAbs(rot) < 1e-6 || qAbs(qAbs(rot) - 360) < 1e-6) rot = 0;
		if (rot == 0) {  // QET: x/y = left end of the baseline
			const double hf = t.halign == 1 || t.halign == 4 ? 0.5 : t.halign == 2 ? 1 : 0;
			const double vf = t.valign == 2 ? 0.5 : t.valign == 3 ? 1 : 0;
			a.rx() -= w * hf;
			a.ry() += size * vf;
			const QRectF box(a.x(), a.y() - size, w, size);
			// EPLAN's font is narrower: drop texts that would overlap in QET
			const bool overlaps = std::any_of(boxes.cbegin(), boxes.cend(), [&](const QRectF &o) {
				return box.left() < o.right() && o.left() < box.right()
				       && box.top() < o.bottom() && o.top() < box.bottom();
			});
			if (overlaps) continue;
			boxes << box;
		}
		parts << makeText(doc, a.x(), a.y(), t.text, size, -rot);
	}

	// the device frame decides which way a terminal faces
	double left = 0, right = 0, top = 0, bottom = 0;
	if (!xs.isEmpty()) {
		left = *std::min_element(xs.cbegin(), xs.cend());
		right = *std::max_element(xs.cbegin(), xs.cend());
		top = *std::min_element(ys.cbegin(), ys.cend());
		bottom = *std::max_element(ys.cbegin(), ys.cend());
	}
	QVector<Terminal> terminals;
	QVector<PinText> pin_texts;
	for (const EplanDxfPin &p : pins) {
		const QPointF q = T(p.pos);
		const double x = rnd(q.x() / 10) * 10, y = rnd(q.y() / 10) * 10;
		const QVector<QPair<QString, double>> dist {
			{QStringLiteral("w"), qAbs(x - left)}, {QStringLiteral("e"), qAbs(x - right)},
			{QStringLiteral("n"), qAbs(y - top)}, {QStringLiteral("s"), qAbs(y - bottom)}};
		QString ori = dist.first().first;
		double best = dist.first().second;
		for (const auto &d : dist) {
			if (d.second < best) { best = d.second; ori = d.first; }
		}
		terminals.append({x, y, ori, p.name});
		// terminal name outside at the wire, function inside -- unless the
		// symbol itself already prints that text next to the terminal (an
		// EPLAN PLC channel box labels "DI", "VS" inside), which would only
		// repeat it
		auto shown = [&](const QString &s) {
			return std::any_of(data.texts.cbegin(), data.texts.cend(), [&](const EplanDxfText &t) {
				return t.text.trimmed() == s.trimmed()
				       && QLineF(T(t.anchor), QPointF(x, y)).length() < 40;
			});
		};
		// a PLC channel named after its function ("DI"): written once, level
		// inside the box as EPLAN shows it, not again turned at the wire
		const bool same = !p.name.isEmpty() && p.function == p.name;
		const bool show_name = !same && !p.name.isEmpty() && !shown(p.name);
		const bool show_func = !p.function.isEmpty() && !shown(p.function);
		const int nl = show_name ? p.name.size() : 0, fl = p.function.size();
		if (ori == QLatin1String("e") || ori == QLatin1String("w")) {
			const bool e = ori == QLatin1String("e");
			if (show_name) parts << makeText(doc, e ? x + 3 : x - 3 - 3.2 * nl, y - 2, p.name);
			if (show_func)
				parts << makeText(doc, e ? x - 6 - 3.2 * fl : x + 6, y + 6, p.function);
		} else {
			const bool n = ori == QLatin1String("n");
			if (show_name) parts << makeText(doc, x - 2, n ? y - 3 : y + 3 + 3.2 * nl, p.name, 5, -90);
			if (show_func)
				parts << makeText(doc, x + 2, n ? y + 8 : y - 4, p.function);
		}
		pin_texts.append({x, y, ori, nl});
		xs << x;
		ys << y;
	}

	const double top_all = *std::min_element(ys.cbegin(), ys.cend());
	const double left_all = *std::min_element(xs.cbegin(), xs.cend());
	const double width = *std::max_element(xs.cbegin(), xs.cend()) - left_all;
	const double height = *std::max_element(ys.cbegin(), ys.cend()) - top_all;
	const double pad = 20;

	QDomElement defn = doc.createElement(QStringLiteral("definition"));
	defn.setAttribute(QStringLiteral("version"), QStringLiteral("0.100.0"));
	defn.setAttribute(QStringLiteral("type"), QStringLiteral("element"));
	defn.setAttribute(QStringLiteral("link_type"), QStringLiteral("simple"));
	defn.setAttribute(QStringLiteral("width"), num(std::ceil((width + 2 * pad) / 10) * 10));
	defn.setAttribute(QStringLiteral("height"), num(std::ceil((height + 2 * pad + 20) / 10) * 10));
	defn.setAttribute(QStringLiteral("hotspot_x"), num(rnd(-left_all + pad)));
	defn.setAttribute(QStringLiteral("hotspot_y"), num(rnd(-top_all + pad + 20)));
	doc.appendChild(defn);

	QDomElement uuid_el = doc.createElement(QStringLiteral("uuid"));
	uuid_el.setAttribute(QStringLiteral("uuid"), uuidStr());
	defn.appendChild(uuid_el);

	QDomElement names_el = doc.createElement(QStringLiteral("names"));
	for (auto it = names.cbegin(); it != names.cend(); ++it) {
		QDomElement nm = doc.createElement(QStringLiteral("name"));
		nm.setAttribute(QStringLiteral("lang"), it.key());
		nm.appendChild(doc.createTextNode(it.value()));
		names_el.appendChild(nm);
	}
	defn.appendChild(names_el);

	QDomElement einfos = doc.createElement(QStringLiteral("elementInformations"));
	for (const auto &kv : info) {
		if (kv.second.isEmpty()) continue;
		QDomElement e = doc.createElement(QStringLiteral("elementInformation"));
		e.setAttribute(QStringLiteral("name"), kv.first);
		e.setAttribute(QStringLiteral("show"), QStringLiteral("1"));
		e.appendChild(doc.createTextNode(kv.second));
		einfos.appendChild(e);
	}
	defn.appendChild(einfos);

	QDomElement infos = doc.createElement(QStringLiteral("informations"));
	infos.appendChild(doc.createTextNode(data.source.isEmpty()
		? QStringLiteral("Generated from the manufacturer's EPLAN DXF data") : data.source));
	defn.appendChild(infos);

	QDomElement desc = doc.createElement(QStringLiteral("description"));
	defn.appendChild(desc);
	for (const QDomElement &e : std::as_const(parts)) desc.appendChild(e);

	// device tag above the device
	QDomElement dyn = doc.createElement(QStringLiteral("dynamic_text"));
	dyn.setAttribute(QStringLiteral("Halignment"), QStringLiteral("AlignLeft"));
	dyn.setAttribute(QStringLiteral("Valignment"), QStringLiteral("AlignTop"));
	dyn.setAttribute(QStringLiteral("font"), font(8));
	dyn.setAttribute(QStringLiteral("frame"), QStringLiteral("false"));
	dyn.setAttribute(QStringLiteral("keep_visual_rotation"), QStringLiteral("false"));
	dyn.setAttribute(QStringLiteral("rotation"), QStringLiteral("0"));
	dyn.setAttribute(QStringLiteral("text_from"), QStringLiteral("ElementInfo"));
	dyn.setAttribute(QStringLiteral("text_width"), QStringLiteral("-1"));
	dyn.setAttribute(QStringLiteral("uuid"), uuidStr());
	dyn.setAttribute(QStringLiteral("x"), num(left_all));
	dyn.setAttribute(QStringLiteral("y"), num(top_all - 20));
	dyn.setAttribute(QStringLiteral("z"), QStringLiteral("20"));
	dyn.appendChild(doc.createElement(QStringLiteral("text")));
	QDomElement info_name = doc.createElement(QStringLiteral("info_name"));
	info_name.appendChild(doc.createTextNode(QStringLiteral("label")));
	dyn.appendChild(info_name);
	desc.appendChild(dyn);

	// Writable fields, filled in per placement (double click in the diagram).
	// A fixed width makes QET wrap instead of writing into the next channel.
	if (!data.fields.isEmpty()) {
		// EPLAN's own empty fields at their original position; width up to the
		// next field of the row or to the frame
		pin_texts.clear();
		const bool has_channel_fields = std::any_of(data.fields.cbegin(), data.fields.cend(),
							    [](const EplanDxfField &f) { return f.of_pin; });
		QVector<FieldPx> conv;
		for (const EplanDxfField &f : data.fields) {
			// device fields go when there are channel fields: the device
			// function is in the element information, and they would sit
			// on a terminal next to channel 1
			if (has_channel_fields && !f.of_pin) continue;
			// EPLAN shows the PLC address or the channel name at the same
			// spot; keep the channel name ("DI1") readable
			if (f.tag.startsWith(QLatin1String("PLC_Address"))
			    && std::any_of(pins.cbegin(), pins.cend(), [&](const EplanDxfPin &p) {
				    return qAbs(f.anchor.x() - p.pos.x()) < 3 && qAbs(f.anchor.y() - p.pos.y()) < 3;
			    }))
				continue;
			const QPointF q = T(f.anchor);
			conv.append({f.tag, q.x(), q.y(), f.halign, f.valign});
		}
		// top to bottom, left to right
		std::stable_sort(conv.begin(), conv.end(), [](const FieldPx &a, const FieldPx &b) {
			const double ya = rnd(a.y), yb = rnd(b.y);
			return ya != yb ? ya < yb : a.x < b.x;
		});
		// a field inside a row of the frame starts at the top of that row
		// (rather than centred on its anchor) and grows down into it
		auto cell = [&](double fx, double fy, double *row_top, double *row_bottom) {
			double above = -std::numeric_limits<double>::infinity();
			double below = std::numeric_limits<double>::infinity();
			for (const HLine &h : std::as_const(hlines)) {
				if (h.x1 - 1 > fx || fx > h.x2 + 1) continue;
				if (h.y <= fy) above = qMax(above, h.y);
				else below = qMin(below, h.y);
			}
			if (std::isinf(above) || std::isinf(below) || below - above > 40) return false;
			*row_top = above;
			*row_bottom = below;
			return true;
		};

		for (const FieldPx &f : std::as_const(conv)) {
			QVector<FieldPx> row;
			for (const FieldPx &c : std::as_const(conv)) {
				if (qAbs(c.y - f.y) < 1 && !(c.tag == f.tag && c.x == f.x)) row << c;
			}
			const bool inside = left - 1 <= f.x && f.x <= right + 1;
			double w, tx;
			if (f.ha == 0) {         // left aligned: up to the next field on the right
				double nxt = std::numeric_limits<double>::infinity();
				for (const FieldPx &c : row) if (c.x > f.x + 1) nxt = qMin(nxt, c.x);
				w = !std::isinf(nxt) ? nxt - f.x - 6 : (inside ? right - f.x - 4 : 160);
				tx = f.x;
			} else if (f.ha == 2) {  // right aligned: up to the next field on the left
				double prv = -std::numeric_limits<double>::infinity();
				for (const FieldPx &c : row) if (c.x < f.x - 1) prv = qMax(prv, c.x);
				w = !std::isinf(prv) ? f.x - prv - 6 : (inside ? f.x - left - 4 : 160);
				tx = f.x - w;
			} else {                 // centred: channel spacing, limited by the frame
				double same = std::numeric_limits<double>::infinity();
				for (const FieldPx &c : row) if (c.tag == f.tag) same = qMin(same, qAbs(c.x - f.x));
				w = !std::isinf(same) ? same - 6 : 120;
				tx = f.x - w / 2;
				if (inside) {    // shift away from the frame rather than narrowing
					w = qMin(w, right - left - 4);
					tx = qMax(left + 2, qMin(tx, right - 2 - w));
				}
			}
			w = qMax(30.0, w);
			double row_top, row_bottom, ty;
			if (cell(f.x, f.y, &row_top, &row_bottom)) {
				// a single text row (10 px): centred; a taller row: from the top
				ty = row_bottom - row_top <= 12 ? (row_top + row_bottom) / 2 - OFF_MID
								: row_top + 1.5 - OFF_TOP;
			} else {
				ty = f.y - (f.va == 3 ? OFF_TOP : f.va == 2 ? OFF_MID : OFF_BOT);
			}
			const QString halign = f.ha == 0 ? QStringLiteral("AlignLeft")
					     : f.ha == 2 ? QStringLiteral("AlignRight")
							 : QStringLiteral("AlignHCenter");
			desc.appendChild(makeUserText(doc, tx, ty, w, halign, QStringLiteral("AlignTop"), 5));
		}
	}

	// no EPLAN fields: one function text per terminal, growing away from it
	// (a macro already shows what its terminals are for)
	if (!data.pin_fields) pin_texts.clear();
	for (const PinText &u : std::as_const(pin_texts)) {
		QVector<double> neighbours;
		for (const PinText &o : std::as_const(pin_texts)) {
			if (o.ori == u.ori && !(o.x == u.x && o.y == u.y) && o.y == u.y)
				neighbours << qAbs(o.x - u.x);
		}
		const bool vertical = u.ori == QLatin1String("n") || u.ori == QLatin1String("s");
		const bool n = u.ori == QLatin1String("n");
		double w, tx, ty;
		QString halign, valign;
		if (vertical && neighbours.isEmpty()) {
			// single terminal (EPLAN PLC box): its rows are only 10 px high at
			// this scale, so the function text goes to the right of the box
			w = 120;
			tx = right + 4;
			ty = n ? u.y : u.y - 20;
			halign = QStringLiteral("AlignLeft");
			valign = n ? QStringLiteral("AlignTop") : QStringLiteral("AlignBottom");
		} else if (vertical) {
			const double gap = *std::min_element(neighbours.cbegin(), neighbours.cend());
			w = qMax(40.0, qMin(gap - 8, 160.0));
			tx = u.x - w / 2;
			ty = n ? u.y + 10 : u.y - 30;
			halign = QStringLiteral("AlignHCenter");
			valign = n ? QStringLiteral("AlignTop") : QStringLiteral("AlignBottom");
		} else {
			const bool e = u.ori == QLatin1String("e");
			w = 120;
			tx = e ? u.x + 6 + 3.2 * u.name_len : u.x - 6 - 3.2 * u.name_len - w;
			ty = u.y - 9;
			halign = e ? QStringLiteral("AlignLeft") : QStringLiteral("AlignRight");
			valign = QStringLiteral("AlignTop");
		}
		desc.appendChild(makeUserText(doc, tx, ty, w, halign, valign, 6));
	}

	for (const Terminal &t : std::as_const(terminals)) {
		QDomElement e = doc.createElement(QStringLiteral("terminal"));
		e.setAttribute(QStringLiteral("name"), t.name);
		e.setAttribute(QStringLiteral("orientation"), t.ori);
		e.setAttribute(QStringLiteral("type"), QStringLiteral("Generic"));
		e.setAttribute(QStringLiteral("uuid"), uuidStr());
		e.setAttribute(QStringLiteral("x"), num(t.x));
		e.setAttribute(QStringLiteral("y"), num(t.y));
		desc.appendChild(e);
	}
	return doc;
}
