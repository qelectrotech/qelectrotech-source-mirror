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
#include "eplanmacroreader.h"

#include <QDomDocument>
#include <QFile>
#include <QHash>
#include <QMap>
#include <QRegularExpression>
#include <QHash>
#include <QTransform>
#include <QtMath>

#include <cmath>

namespace {
	/// "x/y" or "x/y/z" -> point (EPLAN mm, y up)
	bool point(const QString &s, QPointF *p)
	{
		const QStringList v = s.split(QLatin1Char('/'));
		if (v.size() < 2) return false;
		bool ok_x = false, ok_y = false;
		const double x = v.at(0).toDouble(&ok_x), y = v.at(1).toDouble(&ok_y);
		if (!ok_x || !ok_y) return false;
		*p = QPointF(x, y);
		return true;
	}

	/// EPLAN multilingual text "de_DE@Text;en_US@Text;" -> German, else
	/// English, else the first language; plain text is kept as it is
	QString language(const QString &s)
	{
		static const QRegularExpression part(QStringLiteral("([a-z?]{2}_[A-Z?]{2})@([^;]*)(;|$)"));
		QMap<QString, QString> by_lang;
		QString first;
		auto it = part.globalMatch(s);
		while (it.hasNext()) {
			const auto m = it.next();
			by_lang.insert(m.captured(1), m.captured(2));
			if (first.isEmpty()) first = m.captured(2);
		}
		if (by_lang.isEmpty()) return s;
		if (by_lang.contains(QStringLiteral("de_DE"))) return by_lang.value(QStringLiteral("de_DE"));
		if (by_lang.contains(QStringLiteral("en_US"))) return by_lang.value(QStringLiteral("en_US"));
		return first;
	}

	void addRect(QVector<QLineF> &lines, const QPointF &a, const QPointF &b)
	{
		lines << QLineF(a.x(), a.y(), b.x(), a.y()) << QLineF(b.x(), a.y(), b.x(), b.y())
		      << QLineF(b.x(), b.y(), a.x(), b.y()) << QLineF(a.x(), b.y(), a.x(), a.y());
	}

	EplanDxfText text(const QString &s, const QPointF &at, int halign)
	{
		EplanDxfText t;
		t.text = s;
		t.anchor = at;
		t.insert = at;
		t.height = 2.5;
		t.halign = halign;
		return t;
	}

	/// a number of the PXF file; "L" (inherited from the layer) and the
	/// like give @p fallback
	double number(const QString &s, double fallback)
	{
		bool ok = false;
		const double v = s.toDouble(&ok);
		return ok && std::isfinite(v) ? v : fallback;
	}

	/// PXF leaves out positions that are zero: a missing point is 0/0
	bool pointOrZero(const QDomElement &e, const QString &key, QPointF *p)
	{
		if (!e.hasAttribute(key)) {
			*p = QPointF();
			return true;
		}
		return point(e.attribute(key), p);
	}

	/**
		Placement of a drawing: origin and the two axes in the parent's
		coordinates. Objects of the macro carry page coordinates; only the
		drawings (O5) an instance (O144, S144x5) places are local to it,
		moved by its S40x1201/A762 and turned by S39x761 (A751..A754).
	*/
	struct Frame
	{
		QPointF origin;
		QPointF x {1, 0};
		QPointF y {0, 1};
		QPointF at(const QPointF &q) const { return origin + x * q.x() + y * q.y(); }
		double scale() const { return std::hypot(x.x(), x.y()); }
		/// direction of the local x axis, degrees counter-clockwise
		double angle() const { return qRadiansToDegrees(std::atan2(x.y(), x.x())); }
	};

	Frame placement(const QDomElement &e, const Frame &parent)
	{
		const QDomElement t = e.firstChildElement(QStringLiteral("S40x1201"));
		if (t.isNull()) return parent;
		Frame f = parent;
		QPointF o;
		if (pointOrZero(t, QStringLiteral("A762"), &o)) f.origin = parent.at(o);
		const QDomElement m = t.firstChildElement(QStringLiteral("S39x761"));
		if (!m.isNull()) {
			auto n = [&](const char *k, double d) { return number(m.attribute(QLatin1String(k)), d); };
			f.x = parent.x * n("A751", 1) + parent.y * n("A752", 0);
			f.y = parent.x * n("A753", 0) + parent.y * n("A754", 1);
		}
		return f;
	}

	/**
		Elliptic arc (O32, S32x5): centre A555, major semi-axis A556 as a
		vector, ratio of the minor axis A557, start and end angle A553/A554
		(rad, counter-clockwise, seen from the centre). Equal angles: the
		whole ellipse.
	*/
	void addArc(EplanSymbolData &sym, const QDomElement &e, const Frame &f)
	{
		QPointF c, r;
		if (!pointOrZero(e, QStringLiteral("A555"), &c) || !point(e.attribute(QStringLiteral("A556")), &r)) return;
		const double ratio = number(e.attribute(QStringLiteral("A557")), 1);
		const double from = number(e.attribute(QStringLiteral("A553")), 0);
		const double to = number(e.attribute(QStringLiteral("A554")), 0);
		const double radius = std::hypot(r.x(), r.y());
		if (radius <= 0) return;
		double sweep_angle = std::fmod(to - from, 2 * M_PI);
		if (sweep_angle < 0) sweep_angle += 2 * M_PI;
		const bool full = sweep_angle < 1e-5;
		if (full && qAbs(ratio - 1) < 1e-6) {
			sym.circles << qMakePair(f.at(c), radius * f.scale());
			return;
		}
		// the parameter of the point an angle points to
		auto parameter = [ratio](double a) { return std::atan2(std::sin(a), ratio * std::cos(a)); };
		const double start = parameter(from);
		double sweep = full ? 2 * M_PI : std::fmod(parameter(to) - start, 2 * M_PI);
		if (sweep < 0) sweep += 2 * M_PI;
		const QPointF minor(-r.y() * ratio, r.x() * ratio);
		const int n = qMax(2, int(std::ceil(sweep / (M_PI / 18))));  // 10° steps
		QPointF prev;
		for (int k = 0; k <= n; ++k) {
			const double t = start + sweep * k / n;
			const QPointF p = f.at(c + r * std::cos(t) + minor * std::sin(t));
			if (k) sym.lines << QLineF(prev, p);
			prev = p;
		}
	}

	/**
		Draws one graphic object of the macro, as an object of the page (O..)
		or of a drawing (S..x5): line 31, polyline 34, rectangle 89, ellipse
		90, arc 32, text 30/83. Returns false for anything else. Objects
		EPLAN hides (A406 = 1) count as drawn and add nothing.
	*/
	bool addGraphic(EplanSymbolData &sym, const QDomElement &e, const Frame &f)
	{
		const QString tag = e.tagName();
		if (!tag.startsWith(QLatin1Char('O')) && !(tag.startsWith(QLatin1Char('S')) && tag.endsWith(QLatin1String("x5"))))
			return false;
		const QString kind = tag.mid(1).section(QLatin1Char('x'), 0, 0);
		static const QStringList kinds {
			QStringLiteral("30"), QStringLiteral("31"), QStringLiteral("32"), QStringLiteral("34"),
			QStringLiteral("83"), QStringLiteral("89"), QStringLiteral("90")};
		if (!kinds.contains(kind)) return false;
		if (e.attribute(QStringLiteral("A406")) == QLatin1String("1")) return true;

		if (kind == QLatin1String("31")) {  // line
			QPointF a, b;
			if (pointOrZero(e, QStringLiteral("A531"), &a) && pointOrZero(e, QStringLiteral("A532"), &b))
				sym.lines << QLineF(f.at(a), f.at(b));
		} else if (kind == QLatin1String("34")) {  // polyline, closed when A623 = 1
			QStringList parts = e.attribute(QStringLiteral("A621")).split(QLatin1Char(';'));
			while (!parts.isEmpty() && parts.last().isEmpty()) parts.removeLast();
			QVector<QPointF> pts;
			for (const QString &s : std::as_const(parts)) {
				QPointF p;  // an empty entry is 0/0
				if (s.isEmpty() || point(s, &p)) pts << f.at(p);
			}
			if (e.attribute(QStringLiteral("A623")) == QLatin1String("1") && pts.size() > 2 && pts.first() != pts.last())
				pts << pts.first();
			for (int k = 1; k < pts.size(); ++k) sym.lines << QLineF(pts.at(k - 1), pts.at(k));
		} else if (kind == QLatin1String("89")) {  // rectangle
			QPointF a, b;
			if (!pointOrZero(e, QStringLiteral("A1651"), &a) || !pointOrZero(e, QStringLiteral("A1652"), &b))
				return true;
			const QPointF c[4] = {f.at(a), f.at(QPointF(b.x(), a.y())), f.at(b), f.at(QPointF(a.x(), b.y()))};
			const bool upright = qAbs(f.x.y()) < 1e-9 && qAbs(f.y.x()) < 1e-9;
			if (e.attribute(QStringLiteral("A1656")) == QLatin1String("1") && upright)
				sym.filled << QRectF(c[0], c[2]).normalized();
			else
				for (int k = 0; k < 4; ++k) sym.lines << QLineF(c[k], c[(k + 1) % 4]);
		} else if (kind == QLatin1String("90")) {  // ellipse in its bounding box
			QPointF a, b;
			if (!pointOrZero(e, QStringLiteral("A1661"), &a) || !pointOrZero(e, QStringLiteral("A1662"), &b))
				return true;
			const QPointF c = (a + b) / 2;
			const double rx = qAbs(b.x() - a.x()) / 2, ry = qAbs(b.y() - a.y()) / 2;
			if (rx <= 0 || ry <= 0) return true;
			if (qAbs(rx - ry) < 1e-6) {
				sym.circles << qMakePair(f.at(c), rx * f.scale());
			} else {
				QPointF prev = f.at(c + QPointF(rx, 0));
				for (int k = 1; k <= 36; ++k) {
					const double t = 2 * M_PI * k / 36;
					const QPointF p = f.at(c + QPointF(rx * std::cos(t), ry * std::sin(t)));
					sym.lines << QLineF(prev, p);
					prev = p;
				}
			}
		} else if (kind == QLatin1String("32")) {
			addArc(sym, e, f);
		} else {  // text 30, 83
			const QString s = language(e.attribute(QStringLiteral("A511"))).trimmed();
			QPointF at;
			if (s.isEmpty() || !pointOrZero(e, QStringLiteral("A501"), &at)) return true;
			const QDomElement style = e.firstChildElement(QStringLiteral("S54x505"));
			EplanDxfText t = text(s, f.at(at), 0);
			const double h = number(style.attribute(QStringLiteral("A961")), 2.5);
			t.height = (h > 0 ? h : 2.5) * f.scale();
			t.rotation = f.angle() + qRadiansToDegrees(number(style.attribute(QStringLiteral("A962")), 0));
			// A966 1..9: top, middle and bottom row of left, centred and right
			const int j = int(number(style.attribute(QStringLiteral("A966")), 0));
			if (j >= 1 && j <= 9) {
				t.halign = (j - 1) % 3;
				t.valign = (j - 1) / 3 == 0 ? 3 : (j - 1) / 3 == 1 ? 2 : 0;
			}
			sym.texts << t;
		}
		return true;
	}

	/**
		The PLC data of a connection point (O17) from its function
		definition: category A1381 300 is a PLC connection, group A1382 1 or
		99 a channel with kind A1383 2 DI, 3 DQ, 4 AI, 5 AQ; groups 2 and 3
		are supply and reference terminals. The channel name comes from the
		macro's own text where it agrees with the kind ("DI1", "AI 0+",
		"DO 0 NC"), else from the address ("DQx.1") or the channel property
		P20407 ("3", "Channel 3"), numbered as the macro numbers it.
	*/
	EplanPlcPoint plcPoint(const QDomElement &e, const QDomElement &props, const QString &name,
			       const QString &description)
	{
		EplanPlcPoint p;
		if (e.attribute(QStringLiteral("A1381")) != QLatin1String("300")) return p;
		p.key = props.attribute(QStringLiteral("P20407")).trimmed();
		p.data_type = props.attribute(QStringLiteral("P20405")).trimmed();
		p.address = props.attribute(QStringLiteral("P20400")).trimmed();
		p.function_text = language(props.attribute(QStringLiteral("P20011"))).trimmed();

		static const QRegularExpression label(QStringLiteral("^(DI|DQ|DO|AI|AQ|AO)\\s*(\\d+)(?!\\d)"),
						      QRegularExpression::CaseInsensitiveOption);
		static const QRegularExpression strict(QStringLiteral("^(DI|DQ|DO|AI|AQ|AO)\\s*(\\d+)$"),
						       QRegularExpression::CaseInsensitiveOption);
		auto isInput = [](const QString &prefix) { return prefix.endsWith(QLatin1Char('I')); };
		auto isAnalog = [](const QString &prefix) { return prefix.startsWith(QLatin1Char('A')); };

		const QString group = e.attribute(QStringLiteral("A1382"));
		const int kind = QStringList {QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4"), QStringLiteral("5")}
					 .indexOf(e.attribute(QStringLiteral("A1383")));
		const bool channel_group = group == QLatin1String("1") || group == QLatin1String("99");
		const auto own = strict.match(description.trimmed());
		if (channel_group && kind >= 0) {
			p.input = kind == 0 || kind == 2;
			p.analog = kind >= 2;
		} else if (own.hasMatch()) {
			// a channel the macro files under another group ("DI2" as a supply)
			const QString prefix = own.captured(1).toUpper();
			p.input = isInput(prefix);
			p.analog = isAnalog(prefix);
		} else {
			p.role = channel_group ? EplanPlcPoint::Part
				: group == QLatin1String("2") || group == QLatin1String("3") ? EplanPlcPoint::Supply
											     : EplanPlcPoint::None;
			return p;
		}
		p.role = EplanPlcPoint::Signal;

		// the macro's name of the channel, unless it contradicts the kind
		for (const QString &s : {description, name}) {
			const auto m = label.match(s.trimmed());
			if (!m.hasMatch()) continue;
			const QString prefix = m.captured(1).toUpper();
			if (isInput(prefix) != p.input || isAnalog(prefix) != p.analog) continue;
			p.number = m.captured(2).toInt();
			p.channel = prefix + QString::number(p.number);
			return p;
		}
		static const QRegularExpression address(QStringLiteral("^(?:DI|DQ|AI|AQ)x?\\.(\\d+)$"),
							QRegularExpression::CaseInsensitiveOption);
		static const QRegularExpression channel(QStringLiteral("^(?:(?:Channel|Ch\\.?)\\s*)?(\\d+)\\b"),
							QRegularExpression::CaseInsensitiveOption);
		auto m = address.match(p.address);
		if (!m.hasMatch()) m = channel.match(p.key);
		if (m.hasMatch()) {
			p.number = m.captured(1).toInt();
			p.channel = QString(QLatin1Char(p.analog ? 'A' : 'D')) + QLatin1Char(p.input ? 'I' : 'Q')
				    + QString::number(p.number);
		}
		return p;
	}

	/// draws what an instance (O144, S144x5) places: the drawing O5 named by R9
	void addInstance(EplanSymbolData &sym, const QDomElement &e, const Frame &parent,
			 const QHash<QString, QDomElement> &drawings, QMap<QString, int> &skipped, int depth = 0)
	{
		const QDomElement def = drawings.value(e.attribute(QStringLiteral("R9")));
		if (def.isNull() || depth > 16) {
			++skipped[QStringLiteral("instance without drawing")];
			return;
		}
		const Frame f = placement(e, parent);
		for (QDomElement c = def.firstChildElement(); !c.isNull(); c = c.nextSiblingElement()) {
			const QString tag = c.tagName();
			if (tag == QLatin1String("O144") || tag == QLatin1String("S144x5"))
				addInstance(sym, c, f, drawings, skipped, depth + 1);
			else if (!addGraphic(sym, c, f) && !tag.startsWith(QLatin1Char('P'))
				 && tag != QLatin1String("S40x1201") && tag != QLatin1String("S54x505"))
				++skipped[tag];
		}
	}
}

int EplanMacroReader::variantCount(const QString &path)
{
	QFile f(path);
	QDomDocument doc;
	if (!f.open(QIODevice::ReadOnly) || !doc.setContent(&f)) return 0;
	int n = 0;
	for (QDomElement v = doc.documentElement().firstChildElement(QStringLiteral("MacroVariant")); !v.isNull();
	     v = v.nextSiblingElement(QStringLiteral("MacroVariant")))
		if (v.attribute(QStringLiteral("MacroFuncType")) == QLatin1String("1")) ++n;
	return n;
}

bool EplanMacroReader::read(const QString &path, int index)
{
	m_symbol = EplanSymbolData();
	m_letters.clear();
	m_description.clear();
	m_variant_name.clear();
	m_skipped.clear();
	m_error.clear();

	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		m_error = QStringLiteral("Cannot read %1").arg(path);
		return false;
	}
	QDomDocument doc;
	if (!doc.setContent(&f)) {
		m_error = QStringLiteral("%1 is not an EPLAN macro (XML)").arg(path);
		return false;
	}
	const QDomElement root = doc.documentElement();
	if (root.tagName() != QLatin1String("EplanPxfRoot")) {
		m_error = QStringLiteral("%1 is not an EPLAN macro").arg(path);
		return false;
	}
	// the index-th multi-line schematic (variant A, B, ...); the other
	// representations are panel layout, single-line, overview and the like
	QDomElement variant;
	int seen = 0;
	for (QDomElement v = root.firstChildElement(QStringLiteral("MacroVariant")); !v.isNull();
	     v = v.nextSiblingElement(QStringLiteral("MacroVariant"))) {
		if (v.attribute(QStringLiteral("MacroFuncType")) == QLatin1String("1") && seen++ == index) {
			variant = v;
			break;
		}
	}
	if (variant.isNull()) {
		m_error = QStringLiteral("The macro has no multi-line schematic variant");
		return false;
	}
	const QString full = language(variant.attribute(QStringLiteral("Description"),
							root.attribute(QStringLiteral("Description"))));
	m_description = full.section(QLatin1Char('\n'), 0, 0).trimmed();
	// what tells the variants apart: Siemens writes the module on the first
	// line and the part shown ("Power Supply", "Rack layout A") below it
	const QStringList lines = full.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
	m_variant_name = (lines.size() > 1 ? lines.mid(1).join(QStringLiteral(", ")) : lines.value(0)).trimmed();

	// containers and settings rather than drawing objects
	static const QStringList structural {
		QStringLiteral("O4"), QStringLiteral("O6"), QStringLiteral("O14"), QStringLiteral("O26"),
		QStringLiteral("O37"), QStringLiteral("O76")};
	QMap<QString, int> skipped;
	// the drawings (O5) instances place; their objects are local to the
	// instance and drawn only through it
	QHash<QString, QDomElement> drawings;
	{
		const QDomNodeList defs = variant.elementsByTagName(QStringLiteral("O5"));
		for (int k = 0; k < defs.size(); ++k) {
			const QDomElement d = defs.at(k).toElement();
			drawings.insert(d.attribute(QStringLiteral("A1")), d);
		}
	}
	// every element below the variant, in document order (QDom has no "*"),
	// without the insides of the drawings
	QVector<QDomElement> all;
	for (QDomElement e = variant.firstChildElement(); !e.isNull();) {
		all << e;
		if (e.tagName() != QLatin1String("O5") && !e.firstChildElement().isNull()) {
			e = e.firstChildElement();
			continue;
		}
		while (!e.isNull() && e != variant && e.nextSiblingElement().isNull())
			e = e.parentNode().toElement();
		if (e.isNull() || e == variant) break;
		e = e.nextSiblingElement();
	}
	for (const QDomElement &e : std::as_const(all)) {
		const QString tag = e.tagName();
		if (tag == QLatin1String("O144")) {  // instance of a drawing
			addInstance(m_symbol, e, Frame(), drawings, skipped);
			continue;
		}
		if (!tag.startsWith(QLatin1Char('O')) || structural.contains(tag) || tag == QLatin1String("O5")) continue;

		// lines, polylines, rectangles, ellipses, arcs and texts carry page
		// coordinates
		if (addGraphic(m_symbol, e, Frame())) continue;
		if (tag == QLatin1String("O17")) {  // function: connection point or black box
			QPointF pos;
			const QString where = e.firstChildElement(QStringLiteral("S40x1201")).attribute(QStringLiteral("A762"));
			if (!point(where, &pos)) continue;
			const QDomElement props = e.firstChildElement(QStringLiteral("P11"));
			const QDomElement frame = e.firstChildElement(QStringLiteral("S89x5"));
			if (!frame.isNull()) {
				QPointF a, b;
				if (!point(frame.attribute(QStringLiteral("A1651")), &a)
				    || !point(frame.attribute(QStringLiteral("A1652")), &b))
					continue;
				const QRectF box = QRectF(pos + a, pos + b).normalized();
				addRect(m_symbol.lines, box.topLeft(), box.bottomRight());
				for (const QString &t : {QStringLiteral("P150"), QStringLiteral("P144")}) {
					const QString letters = e.firstChildElement(t).attribute(QStringLiteral("P20013"));
					if (!letters.isEmpty() && m_letters.isEmpty()) m_letters = letters;
				}
				// technical data "P: 2000VA IN: 208...550VAC OUT: 2x115VAC":
				// one line per "KEY:" as EPLAN shows it, right-aligned in the
				// upper right of the box (QRectF bottom() is the upper edge, y up)
				const QString data = language(props.attribute(QStringLiteral("P20027")));
				static const QRegularExpression keys(QStringLiteral("\\s+(?=[A-Za-z]{1,6}:)|\\n"));
				// QET's font runs wider than EPLAN's: keep clear of the frame
				double y = box.bottom() - 6;
				for (const QString &line : data.split(keys, Qt::SkipEmptyParts)) {
					m_symbol.texts << text(line.trimmed(), QPointF(box.right() - 5, y), 2);
					y -= 3.5;
				}
				// part number ("MURR.86153" -> "86153") at the lower right corner
				const QString number = props.attribute(QStringLiteral("P20100_1")).section(QLatin1Char('.'), 1);
				if (!number.isEmpty())
					m_symbol.texts << text(number, QPointF(box.right() - 2, box.top() + 2), 2);
			} else if (e.hasAttribute(QStringLiteral("A185"))) {
				EplanDxfPin pin;
				pin.pos = pos;
				pin.name = e.attribute(QStringLiteral("A185"));
				// A185 = connection designation (FUNC_TERMINALDESIGNATION 20028),
				// A186 = connection description (20029), e.g. "DI a.0", "2L+".
				// P20406 is the plug designation ("X10"), not the description.
				pin.function = language(e.attribute(QStringLiteral("A186")));
				// Without a description, show the plug designation where the
				// macro itself places it as a property text (A771 = 20406),
				// e.g. "IN"/"OUT" on a transformer, as EPLAN draws it.
				if (pin.function.isEmpty()) {
					for (QDomElement c = e.firstChildElement(); !c.isNull();
					     c = c.nextSiblingElement()) {
						if (c.attribute(QStringLiteral("A771")) == QLatin1String("20406")) {
							pin.function = language(props.attribute(QStringLiteral("P20406")));
							break;
						}
					}
				}
				pin.plc = plcPoint(e, props, pin.name, language(e.attribute(QStringLiteral("A186"))));
				m_symbol.pins << pin;
				// EPLAN's connection point: a small circle the wire ends at
				m_symbol.circles << qMakePair(pos, 1.0);
			}
		} else if (tag == QLatin1String("O91")) {
			++skipped[QStringLiteral("picture")];
		} else if (e.hasAttribute(QStringLiteral("Build"))) {
			++skipped[tag];
		}
	}
	for (auto it = skipped.cbegin(); it != skipped.cend(); ++it)
		m_skipped << QStringLiteral("%1 x %2").arg(it.value()).arg(it.key());

	if (m_symbol.pins.isEmpty() && m_symbol.lines.isEmpty()) {
		m_error = QStringLiteral("The schematic of the macro holds no drawing");
		return false;
	}
	m_symbol.pin_fields = false;
	m_symbol.source = QStringLiteral("Generated from the manufacturer's EPLAN macro");
	return true;
}

/**
	A function of the macro (O17): a black box (it has a frame) with its
	identifying letters and texts, or a connection point.
*/
void EplanMacroReader::addFunction(const QDomElement &e)
{
	QPointF pos;
	const QString where = e.firstChildElement(QStringLiteral("S40x1201")).attribute(QStringLiteral("A762"));
	if (!point(where, &pos)) return;
	const QDomElement props = e.firstChildElement(QStringLiteral("P11"));
	const QDomElement frame = e.firstChildElement(QStringLiteral("S89x5"));
	if (!frame.isNull()) {
		QPointF a, b;
		if (!point(frame.attribute(QStringLiteral("A1651")), &a)
		    || !point(frame.attribute(QStringLiteral("A1652")), &b))
			return;
		const QRectF box = QRectF(pos + a, pos + b).normalized();
		addRect(m_symbol.lines, box.topLeft(), box.bottomRight());
		for (const QString &t : {QStringLiteral("P150"), QStringLiteral("P144")}) {
			const QString letters = e.firstChildElement(t).attribute(QStringLiteral("P20013"));
			if (!letters.isEmpty() && m_letters.isEmpty()) m_letters = letters;
		}
		// technical data "P: 2000VA IN: 208...550VAC OUT: 2x115VAC":
		// one line per "KEY:" as EPLAN shows it, right-aligned in the
		// upper right of the box (QRectF bottom() is the upper edge, y up)
		const QString data = language(props.attribute(QStringLiteral("P20027")));
		static const QRegularExpression keys(QStringLiteral("\\s+(?=[A-Za-z]{1,6}:)|\\n"));
		// QET's font runs wider than EPLAN's: keep clear of the frame
		double y = box.bottom() - 6;
		for (const QString &line : data.split(keys, Qt::SkipEmptyParts)) {
			m_symbol.texts << text(line.trimmed(), QPointF(box.right() - 5, y), 2);
			y -= 3.5;
		}
		// part number ("MURR.86153" -> "86153") at the lower right corner
		const QString number = props.attribute(QStringLiteral("P20100_1")).section(QLatin1Char('.'), 1);
		if (!number.isEmpty())
			m_symbol.texts << text(number, QPointF(box.right() - 2, box.top() + 2), 2);
	} else if (e.hasAttribute(QStringLiteral("A185"))) {
		EplanDxfPin pin;
		pin.pos = pos;
		pin.name = e.attribute(QStringLiteral("A185"));
		// A185 = connection designation (FUNC_TERMINALDESIGNATION 20028),
		// A186 = connection description (20029), e.g. "DI a.0", "2L+".
		// P20406 is the plug designation ("X10"), not the description.
		pin.function = language(e.attribute(QStringLiteral("A186")));
		// Without a description, show the plug designation where the
		// macro itself places it as a property text (A771 = 20406),
		// e.g. "IN"/"OUT" on a transformer, as EPLAN draws it.
		if (pin.function.isEmpty()) {
			for (QDomElement c = e.firstChildElement(); !c.isNull();
			     c = c.nextSiblingElement()) {
				if (c.attribute(QStringLiteral("A771")) == QLatin1String("20406")) {
					pin.function = language(props.attribute(QStringLiteral("P20406")));
					break;
				}
			}
		}
		pin.plc = plcPoint(e, props, pin.name, language(e.attribute(QStringLiteral("A186"))));
		m_symbol.pins << pin;
		// EPLAN's connection point: a small circle the wire ends at
		m_symbol.circles << qMakePair(pos, 1.0);
	}
}

bool EplanMacroReader::readLayout(const QString &path)
{
	m_symbol = EplanSymbolData();
	m_letters.clear();
	m_description.clear();
	m_skipped.clear();
	m_error.clear();

	QFile f(path);
	QDomDocument doc;
	if (!f.open(QIODevice::ReadOnly) || !doc.setContent(&f)
	    || doc.documentElement().tagName() != QLatin1String("EplanPxfRoot")) {
		m_error = QStringLiteral("%1 is not an EPLAN macro").arg(path);
		return false;
	}
	const QDomElement root = doc.documentElement();
	// MacroFuncType 6 = "2D-Montageaufbau" (panel layout); Siemens ships it
	// as "<type>_2D.ema" next to the 3D macro
	QDomElement variant;
	for (QDomElement v = root.firstChildElement(QStringLiteral("MacroVariant")); !v.isNull();
	     v = v.nextSiblingElement(QStringLiteral("MacroVariant"))) {
		if (v.attribute(QStringLiteral("MacroFuncType")) == QLatin1String("6")) {
			variant = v;
			break;
		}
	}
	if (variant.isNull()) {
		m_error = QStringLiteral("The macro has no panel layout variant");
		return false;
	}
	m_description = language(variant.attribute(QStringLiteral("Description"),
						    root.attribute(QStringLiteral("Description"))))
			.section(QLatin1Char('\n'), 0, 0).trimmed();

	// every element below the variant, in document order
	QVector<QDomElement> all;
	for (QDomElement e = variant.firstChildElement(); !e.isNull();) {
		all << e;
		if (!e.firstChildElement().isNull()) {
			e = e.firstChildElement();
			continue;
		}
		while (!e.isNull() && e != variant && e.nextSiblingElement().isNull())
			e = e.parentNode().toElement();
		if (e.isNull() || e == variant) break;
		e = e.nextSiblingElement();
	}
	// the view is drawn by the graphic objects (O5); the function's frame and
	// the macro box (O37) are not part of it
	QStringList owners;
	for (const QDomElement &e : std::as_const(all))
		if (e.tagName() == QLatin1String("O5")) owners << e.attribute(QStringLiteral("A1"));

	QMap<QString, int> skipped;
	for (const QDomElement &e : std::as_const(all)) {
		const QString tag = e.tagName();
		if (!tag.startsWith(QLatin1Char('S')) || !owners.contains(e.attribute(QStringLiteral("R7")))) continue;
		// lines, polylines, rectangles, ellipses, arcs and texts
		if (!addGraphic(m_symbol, e, Frame())) ++skipped[tag];
	}
	for (auto it = skipped.cbegin(); it != skipped.cend(); ++it)
		m_skipped << QStringLiteral("%1 x %2").arg(it.value()).arg(it.key());
	if (m_symbol.lines.isEmpty() && m_symbol.circles.isEmpty()) {
		m_error = QStringLiteral("The panel layout of the macro holds no drawing");
		return false;
	}
	m_symbol.pin_fields = false;
	// the scale of the panel layout views (Data Portal, DWG): true size side by side
	m_symbol.scale = 2.5;
	m_symbol.source = QStringLiteral("Generated from the manufacturer's EPLAN panel layout macro");
	return true;
}

namespace {
	/// every element below @p top, in document order (QDom has no "*")
	QVector<QDomElement> below(const QDomElement &top)
	{
		QVector<QDomElement> all;
		for (QDomElement e = top.firstChildElement(); !e.isNull();) {
			all << e;
			if (!e.firstChildElement().isNull()) {
				e = e.firstChildElement();
				continue;
			}
			while (!e.isNull() && e != top && e.nextSiblingElement().isNull())
				e = e.parentNode().toElement();
			if (e.isNull() || e == top) break;
			e = e.nextSiblingElement();
		}
		return all;
	}

	/// one language of a text, without the "{{ }}" EPLAN puts around
	/// text that is not to be translated
	QString plain(const QString &s)
	{
		const QString t = language(s);
		static const QRegularExpression braces(QStringLiteral("^\\{\\{(.*)\\}\\}$"),
						       QRegularExpression::DotMatchesEverythingOption);
		const auto m = braces.match(t);
		return (m.hasMatch() ? m.captured(1) : t).trimmed();
	}

	/// a point attribute; one that is left out is the origin
	QPointF at(const QDomElement &e, const char *attribute)
	{
		QPointF p;
		point(e.attribute(QLatin1String(attribute)), &p);
		return p;
	}

	/**
		Where a function or a graphic is placed: S40x1201 with the position
		(A762) and S39x761 with scale and rotation, the 2x2 matrix A751..A754
		column by column -- the macros hold "cos, sin, -sin, cos" of a
		quarter turn (0/1/-1/0 with cos(pi/2) = 6.1e-17, 0/-1/1/0 with
		cos(3pi/2) = -1.8e-16), which is the order of QTransform.
	*/
	QTransform placement(const QDomElement &e)
	{
		const QDomElement where = e.firstChildElement(QStringLiteral("S40x1201"));
		const QPointF p = at(where, "A762");
		const QDomElement m = where.firstChildElement(QStringLiteral("S39x761"));
		auto value = [&m](const char *a, double fallback) {
			bool ok = false;
			const double v = m.attribute(QLatin1String(a)).toDouble(&ok);
			return ok ? v : fallback;
		};
		return QTransform(value("A751", 1), value("A752", 0), value("A753", 0), value("A754", 1),
				  p.x(), p.y());
	}

	double factor(const QTransform &t) { return std::sqrt(qAbs(t.determinant())); }

	/// the number of a drawing object: "S31x5" (inside a graphic) and "O31"
	/// (on its own) are both a line
	int kind(const QString &tag)
	{
		return tag.mid(1).section(QLatin1Char('x'), 0, 0).toInt();
	}

	/**
		A drawing object, placed by @p t: line (31), arc (32), polyline (34),
		rectangle (89) or text (30).
		@return false for any other object
	*/
	bool addShape(EplanSymbolData &s, int kind, const QDomElement &e, const QTransform &t)
	{
		if (kind == 31) {
			s.lines << QLineF(t.map(at(e, "A531")), t.map(at(e, "A532")));
		} else if (kind == 32) {  // start/end angle (rad), centre, radius
			const QPointF c = at(e, "A555");
			const double radius = at(e, "A556").x();
			const double a0 = e.attribute(QStringLiteral("A553")).toDouble();
			double a1 = e.attribute(QStringLiteral("A554")).toDouble();
			while (a1 <= a0) a1 += 2 * M_PI;  // counter-clockwise from start to end
			const bool full = a1 - a0 >= 2 * M_PI - 1e-6;
			// scaled alike in both directions, a circle stays a circle
			const double sx = std::hypot(t.m11(), t.m12()), sy = std::hypot(t.m21(), t.m22());
			if (full && qAbs(sx - sy) < 1e-6 && qAbs(t.m11() * t.m21() + t.m12() * t.m22()) < 1e-6) {
				s.circles << qMakePair(t.map(c), radius * sx);
				return true;
			}
			const int n = qMax(2, int(std::ceil((a1 - a0) / (M_PI / 18))));  // 10° steps
			QPointF prev = t.map(QPointF(c.x() + radius * std::cos(a0), c.y() + radius * std::sin(a0)));
			for (int k = 1; k <= n; ++k) {
				const double a = a0 + (a1 - a0) * k / n;
				const QPointF p = t.map(QPointF(c.x() + radius * std::cos(a), c.y() + radius * std::sin(a)));
				s.lines << QLineF(prev, p);
				prev = p;
			}
		} else if (kind == 34) {  // A623 = 1: closed; an empty point ("13/0;;0/-2") lifts the pen
			QVector<QPointF> pts;
			bool broken = false;
			auto draw = [&]() {
				for (int k = 1; k < pts.size(); ++k) s.lines << QLineF(pts.at(k - 1), pts.at(k));
			};
			QString list = e.attribute(QStringLiteral("A621"));
			while (list.endsWith(QLatin1Char(';'))) list.chop(1);
			for (const QString &v : list.split(QLatin1Char(';'))) {
				QPointF p;
				if (point(v, &p)) {
					pts << t.map(p);
				} else {
					draw();
					pts.clear();
					broken = true;
				}
			}
			draw();
			if (!broken && pts.size() > 2 && e.attribute(QStringLiteral("A623")) == QLatin1String("1"))
				s.lines << QLineF(pts.last(), pts.first());
		} else if (kind == 89) {
			if (!e.hasAttribute(QStringLiteral("A1652"))) return true;
			const QPointF a = at(e, "A1651"), b = at(e, "A1652");
			const bool upright = qFuzzyIsNull(t.m12()) && qFuzzyIsNull(t.m21());
			if (upright && e.attribute(QStringLiteral("A1656")) == QLatin1String("1")) {
				s.filled << QRectF(t.map(a), t.map(b)).normalized();
			} else {
				const QPointF c(b.x(), a.y()), d(a.x(), b.y());
				s.lines << QLineF(t.map(a), t.map(c)) << QLineF(t.map(c), t.map(b))
					<< QLineF(t.map(b), t.map(d)) << QLineF(t.map(d), t.map(a));
			}
		} else if (kind == 30) {
			const QString label = plain(e.attribute(QStringLiteral("A511")));
			if (label.isEmpty()) return true;
			EplanDxfText txt = text(label, t.map(at(e, "A501")), 0);
			// the height, unless the layer gives it ("L")
			bool ok = false;
			const double h = e.firstChildElement(QStringLiteral("S54x505"))
						 .attribute(QStringLiteral("A961")).toDouble(&ok);
			if (ok && h > 0) txt.height = h * factor(t);
			s.texts << txt;
		} else {
			return false;
		}
		return true;
	}

	/// the objects of a graphic (O5), which may place further graphics (S144x5)
	void addGraphic(EplanSymbolData &s, const QHash<QString, QDomElement> &graphics, const QString &id,
			const QTransform &t, int depth, QMap<QString, int> &skipped)
	{
		if (depth > 8 || !graphics.contains(id)) return;
		for (QDomElement c = graphics.value(id).firstChildElement(); !c.isNull(); c = c.nextSiblingElement()) {
			const QString tag = c.tagName();
			if (!tag.startsWith(QLatin1Char('S'))) continue;
			if (tag == QLatin1String("S144x5"))
				addGraphic(s, graphics, c.attribute(QStringLiteral("R9")), placement(c) * t, depth + 1, skipped);
			else if (!addShape(s, kind(tag), c, t))
				++skipped[tag];
		}
	}
}

bool EplanMacroReader::load(const QByteArray &macro)
{
	m_error.clear();
	m_doc = QDomDocument();
	if (!m_doc.setContent(macro)
	    || m_doc.documentElement().tagName() != QLatin1String("EplanPxfRoot")) {
		m_doc = QDomDocument();
		m_error = QStringLiteral("Not an EPLAN macro");
		return false;
	}
	return true;
}

QVector<EplanMacroVariant> EplanMacroReader::variants() const
{
	QVector<EplanMacroVariant> out;
	const QDomElement root = m_doc.documentElement();
	for (QDomElement v = root.firstChildElement(QStringLiteral("MacroVariant")); !v.isNull();
	     v = v.nextSiblingElement(QStringLiteral("MacroVariant"))) {
		EplanMacroVariant variant;
		variant.func_type = v.attribute(QStringLiteral("MacroFuncType")).toInt();
		variant.id = v.attribute(QStringLiteral("VariantId")).toInt();
		variant.description = language(v.attribute(QStringLiteral("Description"))).trimmed()
					      .replace(QLatin1Char('\n'), QStringLiteral(" - "));
		const QDomNodeList functions = v.elementsByTagName(QStringLiteral("O17"));
		for (int i = 0; i < functions.size(); ++i) {
			const QDomElement f = functions.at(i).toElement();
			if (f.hasAttribute(QStringLiteral("A185")) && f.firstChildElement(QStringLiteral("S89x5")).isNull())
				++variant.pins;
		}
		out << variant;
	}
	return out;
}

bool EplanMacroReader::readVariant(int func_type, int variant_id)
{
	m_symbol = EplanSymbolData();
	m_letters.clear();
	m_description.clear();
	m_skipped.clear();
	m_error.clear();

	const QDomElement root = m_doc.documentElement();
	QDomElement variant;
	for (QDomElement v = root.firstChildElement(QStringLiteral("MacroVariant")); !v.isNull();
	     v = v.nextSiblingElement(QStringLiteral("MacroVariant"))) {
		if (v.attribute(QStringLiteral("MacroFuncType")).toInt() == func_type
		    && v.attribute(QStringLiteral("VariantId")).toInt() == variant_id) {
			variant = v;
			break;
		}
	}
	if (variant.isNull()) {
		m_error = QStringLiteral("The macro has no such variant");
		return false;
	}
	m_description = language(variant.attribute(QStringLiteral("Description"),
						    root.attribute(QStringLiteral("Description"))))
			.section(QLatin1Char('\n'), 0, 0).trimmed();

	const QVector<QDomElement> all = below(variant);
	QHash<QString, QDomElement> graphics;
	for (const QDomElement &e : all)
		if (e.tagName() == QLatin1String("O5")) graphics.insert(e.attribute(QStringLiteral("A1")), e);

	// containers, settings, the macro box and placeholder texts (path
	// function text, macro placeholders) rather than drawing objects
	static const QStringList structural {
		QStringLiteral("O4"), QStringLiteral("O5"), QStringLiteral("O6"), QStringLiteral("O14"),
		QStringLiteral("O26"), QStringLiteral("O37"), QStringLiteral("O76"), QStringLiteral("O83"),
		QStringLiteral("O106"), QStringLiteral("O212")};
	QMap<QString, int> skipped;
	for (const QDomElement &e : all) {
		const QString tag = e.tagName();
		if (!tag.startsWith(QLatin1Char('O')) || structural.contains(tag)) continue;
		if (tag == QLatin1String("O144")) {  // a graphic, placed
			addGraphic(m_symbol, graphics, e.attribute(QStringLiteral("R9")), placement(e), 0, skipped);
		} else if (tag == QLatin1String("O17")) {
			// the panel layout is the drawing alone, as readLayout() takes it
			if (func_type != 6) addFunction(e);
		} else if (tag == QLatin1String("O91")) {
			++skipped[QStringLiteral("picture")];
		} else if (!addShape(m_symbol, kind(tag), e, QTransform())) {
			++skipped[tag];
		}
	}
	for (auto it = skipped.cbegin(); it != skipped.cend(); ++it)
		m_skipped << QStringLiteral("%1 x %2").arg(it.value()).arg(it.key());

	if (m_symbol.pins.isEmpty() && m_symbol.lines.isEmpty() && m_symbol.circles.isEmpty()) {
		m_error = QStringLiteral("The variant holds no drawing");
		return false;
	}
	m_symbol.pin_fields = false;
	m_symbol.source = QStringLiteral("Generated from the manufacturer's EPLAN macro");
	return true;
}
