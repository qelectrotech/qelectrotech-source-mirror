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
const QString LABEL_FONT =
	QStringLiteral("Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular");

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
		pins.append(EdzPin{QStringLiteral("1"), QString(), QString()});
	}
	const int n = pins.size();
	const int pitch = 10;
	const int group_gap = 10; // one full slot between named connector groups

	// Pins arrive sorted by functional group then by designation within each
	// group (see EdzPart::parse).  A group break occurs when the
	// functiondefinition block changes; fall back to designation comparison for
	// parts that carry no functiondefinition information.
	// No gap is inserted for power/busbar pins that carry no group label —
	// they flow consecutively so a 3-phase AC source stays connected.
	auto groupKey = [](const EdzPin &p) {
		return p.group.isEmpty() ? p.designation : p.group;
	};
	QVector<bool> is_group_break(n, false);
	for (int i = 1; i < n; ++i)
		is_group_break[i] = (groupKey(pins.at(i)) != groupKey(pins.at(i - 1)))
		                    && (!pins.at(i).group.isEmpty());

	// Place every terminal on a multiple-of-10 y coordinate so they align
	// cleanly with QET's default grid.  The gap slot is also 10 px, giving
	// one empty grid row between connector groups.
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

	// Per-pin labels.
	for (int i = 0; i < n; ++i) {
		QString label = pins.at(i).designation;
		if (!pins.at(i).description.isEmpty()) {
			label += QStringLiteral("  ") + pins.at(i).description;
		}
		desc.appendChild(makeText(doc, 6, pin_y[i] + 2, label, FONT));
	}

	// Connector group header labels — one per named group, in the gap above
	// the group's first pin so the electrician can see the terminal block name
	// (e.g. "XDI", "XPOW") without having to read individual terminal names.
	for (int i = 0; i < n; ++i) {
		if (pins.at(i).group.isEmpty()) continue;
		if (i > 0 && groupKey(pins.at(i)) == groupKey(pins.at(i - 1))) continue;
		// Place the label centred in the gap slot above the group's first pin.
		const int label_y = (i == 0) ? (pin_y[0] - group_gap / 2)
		                              : (pin_y[i] - group_gap / 2);
		desc.appendChild(makeText(doc, body_left + 2, label_y,
		                          pins.at(i).group, FONT));
	}

	// Device-tag label (per-instance, above the body).
	QDomElement dyn = doc.createElement(QStringLiteral("dynamic_text"));
	dyn.setAttribute(QStringLiteral("x"), min_x + pad);
	dyn.setAttribute(QStringLiteral("y"), min_y - 9);
	dyn.setAttribute(QStringLiteral("z"), QStringLiteral("5"));
	dyn.setAttribute(QStringLiteral("text_width"), QStringLiteral("-1"));
	dyn.setAttribute(QStringLiteral("Halignment"), QStringLiteral("AlignLeft"));
	dyn.setAttribute(QStringLiteral("Valignment"), QStringLiteral("AlignTop"));
	dyn.setAttribute(QStringLiteral("frame"), QStringLiteral("false"));
	dyn.setAttribute(QStringLiteral("rotation"), QStringLiteral("0"));
	dyn.setAttribute(QStringLiteral("keep_visual_rotation"), QStringLiteral("false"));
	dyn.setAttribute(QStringLiteral("text_from"), QStringLiteral("ElementInfo"));
	dyn.setAttribute(QStringLiteral("uuid"), uuidStr());
	dyn.setAttribute(QStringLiteral("font"), LABEL_FONT);
	dyn.appendChild(doc.createElement(QStringLiteral("text")));
	QDomElement info_name = doc.createElement(QStringLiteral("info_name"));
	info_name.appendChild(doc.createTextNode(QStringLiteral("label")));
	dyn.appendChild(info_name);
	desc.appendChild(dyn);

	// Build unique terminal names that an electrician can read directly on
	// a wiring list.  When a terminalNr connector label is present, prefix it:
	//   "XDI.2"  (terminal block XDI, position 2)
	//   "XRO1.3" (relay output block XRO1, position 3)
	// Power/busbar connections that carry no terminalNr use their designation
	// as-is — these are already globally unique ("L1/U1", "UDC+", "PE", …).
	// A numeric suffix is appended only if a collision still occurs, which
	// should not happen with well-formed EPLAN data.
	QSet<QString> used_names;
	QVector<QString> terminal_names(n);

	for (int i = 0; i < n; ++i) {
		const EdzPin &pin = pins.at(i);
		QString name = pin.group.isEmpty()
			? pin.designation
			: pin.group + QLatin1Char('.') + pin.designation;
		// Collision guard (malformed data safety net).
		if (used_names.contains(name)) {
			int suffix = 2;
			while (used_names.contains(name + QLatin1Char('_') + QString::number(suffix)))
				++suffix;
			name += QLatin1Char('_') + QString::number(suffix);
		}
		terminal_names[i] = name;
		used_names.insert(name);
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
