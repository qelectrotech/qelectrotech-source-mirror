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
#include "edzelementbuilder.h"

#include "edzpart.h"

#include <QMap>
#include <QSet>
#include <QUuid>
#include <QVector>
#include <algorithm>
#include <cmath>

namespace {

const QString STYLE =
	QStringLiteral("line-style:normal;line-weight:normal;filling:none;color:black");
const QString FONT =
	QStringLiteral("Liberation Sans,5,-1,5,50,0,0,0,0,0,Regular");
const QString TITLE_FONT =
	QStringLiteral("Liberation Sans,5,-1,5,75,0,0,0,0,0,Bold");

QString uuidStr()
{
	return QUuid::createUuid().toString(); // "{....}"
}

QDomElement makeText(QDomDocument &doc, int x, int y, const QString &text,
		     const QString &font)
{
	QDomElement e = doc.createElement(QStringLiteral("text"));
	e.setAttribute(QStringLiteral("x"), x);
	e.setAttribute(QStringLiteral("y"), y);
	e.setAttribute(QStringLiteral("text"), text);
	e.setAttribute(QStringLiteral("rotation"), QStringLiteral("0"));
	e.setAttribute(QStringLiteral("font"), font);
	e.setAttribute(QStringLiteral("color"), QStringLiteral("#000000"));
	return e;
}

int ceilTo10(int v)
{
	if (v <= 0) {
		return 0;
	}
	return ((v + 9) / 10) * 10;
}

} // namespace

QDomDocument EdzElementBuilder::build(const EdzPart &part)
{
	QList<EdzPin> pins = part.pins();
	if (pins.isEmpty()) {
		pins.append(EdzPin{QStringLiteral("1"), QString()});
	}
	const int n = pins.size();
	const int pitch = 10;
	const int group_gap = 5; // extra px inserted between designation groups

	// Pins arrive sorted by functional group then by designation within each
	// group (see EdzPart::parse).  A group break occurs when the
	// functiondefinition block changes; fall back to designation comparison for
	// parts that carry no functiondefinition information.
	auto groupKey = [](const EdzPin &p) {
		return p.group.isEmpty() ? p.designation : p.group;
	};
	QVector<bool> is_group_break(n, false);
	for (int i = 1; i < n; ++i)
		is_group_break[i] = (groupKey(pins.at(i)) != groupKey(pins.at(i - 1)));

	QVector<int> pin_y(n);
	int cur_y = 0;
	for (int i = 0; i < n; ++i) {
		if (is_group_break[i])
			cur_y += group_gap;
		pin_y[i] = cur_y;
		cur_y += pitch;
	}

	const int body_left = 0, body_right = 90, body_top = -20;
	const int body_bottom = cur_y; // one pitch below the last pin

	// Coordinate accumulators for the bounding box (origin always included).
	QList<int> xs{0}, ys{0};
	xs << body_left << body_right;
	ys << body_top << body_bottom;
	xs << (body_left + 6);          // title x
	ys << (body_top + 7);           // title y
	for (int i = 0; i < n; ++i) {
		xs << 6 << 0;               // pin label x, terminal x
		ys << (pin_y[i] + 2) << pin_y[i]; // pin label y, terminal y
	}

	const int pad = 10;
	const int min_x = *std::min_element(xs.begin(), xs.end()) - pad;
	const int max_x = *std::max_element(xs.begin(), xs.end()) + pad;
	const int min_y = *std::min_element(ys.begin(), ys.end()) - pad;
	const int max_y = *std::max_element(ys.begin(), ys.end()) + pad;
	const int width  = std::max(10, ceilTo10(max_x - min_x));
	const int height = std::max(10, ceilTo10(max_y - min_y));
	const int hotspot_x = -min_x;
	const int hotspot_y = -min_y;

	QDomDocument doc;
	doc.appendChild(doc.createProcessingInstruction(
		QStringLiteral("xml"),
		QStringLiteral("version=\"1.0\" encoding=\"utf-8\"")));

	QDomElement defn = doc.createElement(QStringLiteral("definition"));
	defn.setAttribute(QStringLiteral("version"), QStringLiteral("0.100.0"));
	defn.setAttribute(QStringLiteral("type"), QStringLiteral("element"));
	defn.setAttribute(QStringLiteral("link_type"), QStringLiteral("simple"));
	defn.setAttribute(QStringLiteral("width"), width);
	defn.setAttribute(QStringLiteral("height"), height);
	defn.setAttribute(QStringLiteral("hotspot_x"), hotspot_x);
	defn.setAttribute(QStringLiteral("hotspot_y"), hotspot_y);
	doc.appendChild(defn);

	QDomElement uuid_el = doc.createElement(QStringLiteral("uuid"));
	uuid_el.setAttribute(QStringLiteral("uuid"), uuidStr());
	defn.appendChild(uuid_el);

	// Localized names: fr/en first, then the rest alphabetically.
	QDomElement names = doc.createElement(QStringLiteral("names"));
	QMap<QString, QString> name_map = part.names();
	const QString fallback = !part.orderNumber().isEmpty() ? part.orderNumber()
				 : (!part.partNumber().isEmpty() ? part.partNumber()
								 : QStringLiteral("part"));
	if (name_map.isEmpty()) {
		name_map.insert(QStringLiteral("en"),
				!part.description().isEmpty() ? part.description()
							      : fallback);
	}
	QStringList ordered;
	for (const QString &c : {QStringLiteral("fr"), QStringLiteral("en")}) {
		if (name_map.contains(c)) ordered << c;
	}
	QStringList rest;
	for (const QString &c : name_map.keys()) {
		if (c != QLatin1String("fr") && c != QLatin1String("en")) rest << c;
	}
	std::sort(rest.begin(), rest.end());
	ordered << rest;
	for (const QString &lang : ordered) {
		QDomElement nm = doc.createElement(QStringLiteral("name"));
		nm.setAttribute(QStringLiteral("lang"), lang);
		nm.appendChild(doc.createTextNode(name_map.value(lang)));
		names.appendChild(nm);
	}
	defn.appendChild(names);

	QDomElement infos = doc.createElement(QStringLiteral("informations"));
	infos.appendChild(doc.createTextNode(
		QStringLiteral("Imported from EPLAN .edz by edz2qet (%1)")
			.arg(part.partNumber())));
	defn.appendChild(infos);

	// Element information -> QET BOM / nomenclature.
	QDomElement einfos = doc.createElement(QStringLiteral("elementInformations"));
	const QList<QPair<QString, QString>> info_map{
		{QStringLiteral("manufacturer"), part.manufacturer()},
		{QStringLiteral("manufacturer_reference"), part.orderNumber()},
		{QStringLiteral("designation"), part.description()},
		{QStringLiteral("comment"), part.comment()},
	};
	for (const auto &kv : info_map) {
		if (kv.second.isEmpty()) {
			continue;
		}
		QDomElement e = doc.createElement(QStringLiteral("elementInformation"));
		e.setAttribute(QStringLiteral("name"), kv.first);
		e.setAttribute(QStringLiteral("show"), QStringLiteral("1"));
		e.appendChild(doc.createTextNode(kv.second));
		einfos.appendChild(e);
	}
	defn.appendChild(einfos);

	QDomElement desc = doc.createElement(QStringLiteral("description"));

	// Body rectangle.
	QDomElement rect = doc.createElement(QStringLiteral("rect"));
	rect.setAttribute(QStringLiteral("x"), body_left);
	rect.setAttribute(QStringLiteral("y"), body_top);
	rect.setAttribute(QStringLiteral("width"), body_right - body_left);
	rect.setAttribute(QStringLiteral("height"), body_bottom - body_top);
	rect.setAttribute(QStringLiteral("style"), STYLE);
	rect.setAttribute(QStringLiteral("antialias"), QStringLiteral("false"));
	desc.appendChild(rect);

	// Title (order number) in its own band at the top.
	const QString title = !part.orderNumber().isEmpty() ? part.orderNumber()
			      : (!part.partNumber().isEmpty() ? part.partNumber()
							      : QStringLiteral("PART"));
	desc.appendChild(makeText(doc, body_left + 6, body_top + 7, title, TITLE_FONT));

	// Group separator lines — thin dashed line halfway through each inter-group
	// gap so the eye can immediately see which terminals belong together.
	const QString SEP_STYLE =
		QStringLiteral("line-style:dashed;line-weight:thin;filling:none;color:black");
	for (int i = 1; i < n; ++i) {
		if (!is_group_break[i]) continue;
		const int sep_y = (pin_y[i - 1] + pitch + pin_y[i]) / 2;
		QDomElement sep = doc.createElement(QStringLiteral("line"));
		sep.setAttribute(QStringLiteral("x1"), body_left + 2);
		sep.setAttribute(QStringLiteral("y1"), sep_y);
		sep.setAttribute(QStringLiteral("x2"), body_right - 2);
		sep.setAttribute(QStringLiteral("y2"), sep_y);
		sep.setAttribute(QStringLiteral("style"), SEP_STYLE);
		sep.setAttribute(QStringLiteral("antialias"), QStringLiteral("false"));
		desc.appendChild(sep);
	}

	// Per-pin labels.
	for (int i = 0; i < n; ++i) {
		QString label = pins.at(i).designation;
		if (!pins.at(i).description.isEmpty()) {
			label += QStringLiteral("  ") + pins.at(i).description;
		}
		desc.appendChild(makeText(doc, 6, pin_y[i] + 2, label, FONT));
	}

	// Device-tag label (per-instance, above the body).
	QDomElement dyn = doc.createElement(QStringLiteral("dynamic_text"));
	dyn.setAttribute(QStringLiteral("x"), min_x + pad);
	dyn.setAttribute(QStringLiteral("y"), min_y - 2);
	dyn.setAttribute(QStringLiteral("z"), QStringLiteral("5"));
	dyn.setAttribute(QStringLiteral("text_width"), QStringLiteral("-1"));
	dyn.setAttribute(QStringLiteral("Halignment"), QStringLiteral("AlignLeft"));
	dyn.setAttribute(QStringLiteral("Valignment"), QStringLiteral("AlignTop"));
	dyn.setAttribute(QStringLiteral("frame"), QStringLiteral("false"));
	dyn.setAttribute(QStringLiteral("rotation"), QStringLiteral("0"));
	dyn.setAttribute(QStringLiteral("keep_visual_rotation"), QStringLiteral("false"));
	dyn.setAttribute(QStringLiteral("text_from"), QStringLiteral("ElementInfo"));
	dyn.setAttribute(QStringLiteral("uuid"), uuidStr());
	dyn.setAttribute(QStringLiteral("font"), FONT);
	dyn.appendChild(doc.createElement(QStringLiteral("text")));
	QDomElement info_name = doc.createElement(QStringLiteral("info_name"));
	info_name.appendChild(doc.createTextNode(QStringLiteral("label")));
	dyn.appendChild(info_name);
	desc.appendChild(dyn);

	// Build unique terminal names.  QET requires every terminal in an element
	// to have a distinct name for wiring and terminal-diagram generation to
	// work.  EPLAN parts sometimes repeat the same connectionDesignation across
	// multiple function templates (e.g. a drive with several connections named
	// "1", "2", "3").  Resolve duplicates:
	//   1. If a designation is unique, keep it as-is.
	//   2. If duplicated, append "_" + sanitised description when that yields
	//      a unique result (e.g. "1_L+_P" -> "1_L_P").
	//   3. Fall back to appending "_2", "_3", … when descriptions are missing
	//      or still collide.
	auto sanitise = [](const QString &s) -> QString {
		QString out;
		for (const QChar c : s) {
			out += (c.isLetterOrNumber() || c == QLatin1Char('-')) ? c
				: QLatin1Char('_');
		}
		// Collapse consecutive underscores and strip trailing ones.
		while (out.contains(QStringLiteral("__")))
			out.replace(QStringLiteral("__"), QStringLiteral("_"));
		while (out.endsWith(QLatin1Char('_')))
			out.chop(1);
		return out;
	};

	// Count occurrences of each raw designation.
	QMap<QString, int> desig_count;
	for (const EdzPin &p : pins)
		desig_count[p.designation]++;

	QSet<QString> used_names;
	QVector<QString> terminal_names(n);
	// Per-designation occurrence counter for the fallback suffix.
	QMap<QString, int> desig_seen;

	for (int i = 0; i < n; ++i) {
		const EdzPin &pin = pins.at(i);
		desig_seen[pin.designation]++;

		if (desig_count[pin.designation] == 1) {
			// Unique designation — use directly.
			terminal_names[i] = pin.designation;
		} else {
			// Try designation + sanitised description first.
			const QString candidate = pin.description.isEmpty()
				? QString()
				: pin.designation + QLatin1Char('_') + sanitise(pin.description);
			if (!candidate.isEmpty() && !used_names.contains(candidate)) {
				terminal_names[i] = candidate;
			} else {
				// Numeric suffix fallback: "1", "1_2", "1_3", …
				const int occ = desig_seen[pin.designation];
				terminal_names[i] = (occ == 1)
					? pin.designation
					: pin.designation + QLatin1Char('_') + QString::number(occ);
			}
		}
		used_names.insert(terminal_names[i]);
	}

	// Terminals.
	for (int i = 0; i < n; ++i) {
		QDomElement t = doc.createElement(QStringLiteral("terminal"));
		t.setAttribute(QStringLiteral("uuid"), uuidStr());
		t.setAttribute(QStringLiteral("name"), terminal_names[i]);
		t.setAttribute(QStringLiteral("x"), 0);
		t.setAttribute(QStringLiteral("y"), pin_y[i]);
		t.setAttribute(QStringLiteral("orientation"), QStringLiteral("w"));
		t.setAttribute(QStringLiteral("type"), QStringLiteral("Generic"));
		desc.appendChild(t);
	}

	defn.appendChild(desc);
	return doc;
}

QString EdzElementBuilder::toElmtString(const QDomDocument &doc)
{
	return doc.toString(0);
}
