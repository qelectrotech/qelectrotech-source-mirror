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
#include "plcchannels.h"

#include "elementfingerprint.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

#include <algorithm>
#include <optional>

namespace {
	const QRegularExpression &channelRe()
	{
		static const QRegularExpression re(QStringLiteral("^(DI|DQ|DO|AI|AQ|AO)\\s*(\\d+)$"),
						   QRegularExpression::CaseInsensitiveOption);
		return re;
	}

	const QRegularExpression &referenceRe()
	{
		static const QRegularExpression re(
			QStringLiteral("^(M|L\\+|L-|0V|0VDC|24V|24VDC|\\+|-|1M|2M|1L\\+|2L\\+)$"),
			QRegularExpression::CaseInsensitiveOption);
		return re;
	}

	QString ioType(const QString &prefix)
	{
		const QString p = prefix.toUpper();
		if (p == QLatin1String("DI")) return QStringLiteral("entree_digitale");
		if (p == QLatin1String("DQ") || p == QLatin1String("DO")) return QStringLiteral("sortie_digitale");
		if (p == QLatin1String("AI")) return QStringLiteral("entree_analogique");
		return QStringLiteral("sortie_analogique");
	}

	struct Spot { QString text; double x; double y; };

	double num(const QDomElement &e, const char *attr)
	{
		return e.attribute(QLatin1String(attr), QStringLiteral("0")).toDouble();
	}

	QVector<Spot> terminals(const QDomDocument &doc)
	{
		QVector<Spot> out;
		const QDomNodeList list = doc.elementsByTagName(QStringLiteral("terminal"));
		for (int i = 0; i < list.size(); ++i) {
			const QDomElement t = list.at(i).toElement();
			// <terminal> inside a plcIO is a channel's terminal name, not a terminal
			if (!t.hasAttribute(QStringLiteral("x"))) continue;
			out.append({t.attribute(QStringLiteral("name")), num(t, "x"), num(t, "y")});
		}
		return out;
	}

	/// static texts and user texts of the drawing
	QVector<Spot> texts(const QDomDocument &doc)
	{
		QVector<Spot> out;
		const QDomNodeList list = doc.elementsByTagName(QStringLiteral("text"));
		for (int i = 0; i < list.size(); ++i) {
			const QDomElement t = list.at(i).toElement();
			if (t.hasAttribute(QStringLiteral("text")))
				out.append({t.attribute(QStringLiteral("text")).trimmed(), num(t, "x"), num(t, "y")});
		}
		const QDomNodeList dyn = doc.elementsByTagName(QStringLiteral("dynamic_text"));
		for (int i = 0; i < dyn.size(); ++i) {
			const QDomElement d = dyn.at(i).toElement();
			if (d.attribute(QStringLiteral("text_from"), QStringLiteral("UserText")) != QLatin1String("UserText"))
				continue;
			const QString s = d.firstChildElement(QStringLiteral("text")).text().trimmed();
			if (!s.isEmpty()) out.append({s, num(d, "x"), num(d, "y")});
		}
		return out;
	}

	/// the channel or reference text beside a terminal: left of it, just
	/// below its height, where the element builder puts the EPLAN
	/// connection description
	std::optional<QString> functionText(const Spot &terminal, const QVector<Spot> &txt)
	{
		double best_d = 1e9;
		std::optional<QString> best;
		for (const Spot &s : txt) {
			if (s.text == terminal.text) continue;
			if (!channelRe().match(s.text).hasMatch() && !referenceRe().match(s.text).hasMatch()) continue;
			if (s.x > terminal.x + 1 || s.x < terminal.x - 40) continue;
			const double d = qAbs(s.y - (terminal.y + 6));
			if (d <= 8 && d < best_d) {
				best_d = d;
				best = s.text;
			}
		}
		return best;
	}

	int channelNumber(const QString &name)
	{
		static const QRegularExpression digits(QStringLiteral("\\d+"));
		const auto m = digits.match(name);
		return m.hasMatch() ? m.captured().toInt() : 0;
	}

	QString serialized(const QDomElement &e)
	{
		QString s;
		QTextStream ts(&s);
		e.save(ts, -1);
		return s;
	}
}

QVector<PlcChannels::Channel> PlcChannels::fromElement(const QDomDocument &element)
{
	QVector<Spot> pins = terminals(element);
	const QVector<Spot> txt = texts(element);
	std::stable_sort(pins.begin(), pins.end(), [](const Spot &a, const Spot &b) {
		return a.y != b.y ? a.y < b.y : a.x < b.x;
	});

	struct Found { QString prefix; Channel channel; double y; };
	QVector<Found> found;
	int last = -1;		// the channel a reference terminal may follow
	for (const Spot &p : std::as_const(pins)) {
		const auto f = functionText(p, txt);
		const auto m = channelRe().match(f.value_or(QString()));
		if (m.hasMatch()) {
			const QString prefix = m.captured(1).toUpper();
			Channel c{ioType(prefix), prefix + QString::number(m.captured(2).toInt()), {p.text}};
			found.append({prefix, c, p.y});
			last = found.size() - 1;
		} else if (f && referenceRe().match(*f).hasMatch() && last >= 0
			   && p.y - found.at(last).y > 0 && p.y - found.at(last).y <= 25) {
			found[last].channel.terminals << p.text;
			last = -1;	// one reference terminal per channel
		} else {
			last = -1;
		}
	}
	std::stable_sort(found.begin(), found.end(), [](const Found &a, const Found &b) {
		return a.prefix != b.prefix ? a.prefix < b.prefix
					    : channelNumber(a.channel.name) < channelNumber(b.channel.name);
	});
	QVector<Channel> out;
	for (const Found &f : std::as_const(found)) out << f.channel;
	return out;
}

QVector<PlcChannels::Channel> PlcChannels::fromMacro(const QVector<EplanDxfPin> &pins,
						      const QStringList &terminal_names)
{
	struct Found { QString prefix; QString key; Channel channel; };
	QVector<Found> found;
	QHash<QString, int> by_name;
	for (int i = 0; i < pins.size() && i < terminal_names.size(); ++i) {
		const EplanPlcPoint &p = pins.at(i).plc;
		if (p.role != EplanPlcPoint::Signal || p.channel.isEmpty()) continue;
		const int at = by_name.value(p.channel, -1);
		if (at >= 0) {
			// one channel on several terminals (U+ and I+ of an analog
			// input, NC, NO and COM of a relay output); a second channel of
			// the same name the macro numbers apart is a slip of the macro
			if (found.at(at).key == p.key) found[at].channel.terminals << terminal_names.at(i);
			continue;
		}
		by_name.insert(p.channel, found.size());
		const QString prefix = QString(p.channel).remove(QRegularExpression(QStringLiteral("\\d+$")));
		const QString type = p.analog ? (p.input ? QStringLiteral("entree_analogique") : QStringLiteral("sortie_analogique"))
					      : (p.input ? QStringLiteral("entree_digitale") : QStringLiteral("sortie_digitale"));
		found.append({prefix, p.key, Channel{type, p.channel, {terminal_names.at(i)}}});
	}

	// the other terminals of a channel share its key, when no other channel has it
	QHash<QString, int> by_key, users;
	for (int k = 0; k < found.size(); ++k) {
		if (found.at(k).key.isEmpty()) continue;
		by_key.insert(found.at(k).key, k);
		++users[found.at(k).key];
	}
	for (const EplanPlcPoint::Role role : {EplanPlcPoint::Part, EplanPlcPoint::Supply}) {
		for (int i = 0; i < pins.size() && i < terminal_names.size(); ++i) {
			const EplanPlcPoint &p = pins.at(i).plc;
			if (p.role != role || p.key.isEmpty() || users.value(p.key) != 1) continue;
			Channel &c = found[by_key.value(p.key)].channel;
			if (c.terminals.size() < 4) c.terminals << terminal_names.at(i);
		}
	}
	for (Found &f : found)
		while (f.channel.terminals.size() > 4) f.channel.terminals.removeLast();

	std::stable_sort(found.begin(), found.end(), [](const Found &a, const Found &b) {
		return a.prefix != b.prefix ? a.prefix < b.prefix
					    : channelNumber(a.channel.name) < channelNumber(b.channel.name);
	});
	QVector<Channel> out;
	for (const Found &f : std::as_const(found)) out << f.channel;
	return out;
}

bool PlcChannels::setChannels(QDomDocument &element, const QVector<Channel> &channels)
{
	QDomElement defn = element.documentElement();
	if (defn.isNull()) return false;

	QDomElement plc = element.createElement(QStringLiteral("plcMasterData"));
	plc.setAttribute(QStringLiteral("rowHeight"), QStringLiteral("8.00"));
	QDomElement ios = element.createElement(QStringLiteral("plcIOs"));
	plc.appendChild(ios);
	for (const Channel &c : channels) {
		QDomElement io = element.createElement(QStringLiteral("plcIO"));
		io.setAttribute(QStringLiteral("type"), c.type);
		io.setAttribute(QStringLiteral("address"), c.name);
		io.setAttribute(QStringLiteral("functionText"), QString());
		io.setAttribute(QStringLiteral("comment"), QString());
		io.setAttribute(QStringLiteral("crossRef"), QString());
		io.setAttribute(QStringLiteral("terminalCount"), QString::number(c.terminals.size()));
		for (const QString &t : c.terminals) {
			QDomElement te = element.createElement(QStringLiteral("terminal"));
			te.appendChild(element.createTextNode(t));
			io.appendChild(te);
		}
		ios.appendChild(io);
	}

	QDomElement kind = defn.firstChildElement(QStringLiteral("kindInformations"));
	if (defn.attribute(QStringLiteral("link_type")) == QLatin1String("master") && !kind.isNull()) {
		QString type;
		for (QDomElement k = kind.firstChildElement(QStringLiteral("kindInformation")); !k.isNull();
		     k = k.nextSiblingElement(QStringLiteral("kindInformation")))
			if (k.attribute(QStringLiteral("name")) == QLatin1String("type")) type = k.text();
		if (type == QLatin1String("plc")
		    && serialized(kind.firstChildElement(QStringLiteral("plcMasterData"))) == serialized(plc))
			return false;
	}

	defn.setAttribute(QStringLiteral("link_type"), QStringLiteral("master"));
	if (kind.isNull()) {
		kind = element.createElement(QStringLiteral("kindInformations"));
		const QDomElement infos = defn.firstChildElement(QStringLiteral("elementInformations"));
		if (!infos.isNull()) defn.insertAfter(kind, infos);
		else defn.insertBefore(kind, defn.firstChildElement(QStringLiteral("description")));
	}
	for (QDomElement k = kind.firstChildElement(); !k.isNull();) {
		const QDomElement next = k.nextSiblingElement();
		if (k.tagName() == QLatin1String("plcMasterData")
		    || (k.tagName() == QLatin1String("kindInformation")
			&& k.attribute(QStringLiteral("name")) == QLatin1String("type")))
			kind.removeChild(k);
		k = next;
	}
	QDomElement type = element.createElement(QStringLiteral("kindInformation"));
	type.setAttribute(QStringLiteral("name"), QStringLiteral("type"));
	type.appendChild(element.createTextNode(QStringLiteral("plc")));
	kind.insertBefore(type, kind.firstChild());
	kind.appendChild(plc);
	return true;
}

QStringList PlcChannels::applyToFolders(const QStringList &written_paths, const QStringList &from_macro,
					const QVector<Channel> &given)
{
	QStringList report;
	QSet<QString> folders;
	for (const QString &p : written_paths)
		if (p.endsWith(QStringLiteral(".elmt"), Qt::CaseInsensitive))
			folders.insert(QFileInfo(p).absolutePath());
	QSet<QString> fixed;
	for (const QString &p : from_macro) fixed.insert(QFileInfo(p).absoluteFilePath());

	struct Table { QVector<Channel> channels; QString source; };
	for (const QString &folder : std::as_const(folders)) {
		const QDir dir(folder);
		const QStringList files = dir.entryList({QStringLiteral("*.elmt")}, QDir::Files, QDir::Name);
		QVector<QPair<QString, QDomDocument>> docs;
		for (const QString &f : files) {
			QFile file(dir.filePath(f));
			if (!file.open(QIODevice::ReadOnly)) continue;
			QDomDocument doc;
			if (!doc.setContent(&file)) continue;
			docs.append({f, doc});
		}
		QVector<Channel> table = given;
		QString source = QStringLiteral("the part data");
		if (table.isEmpty()) {
			for (const auto &d : std::as_const(docs)) {
				const QVector<Channel> c = fromElement(d.second);
				if (c.size() > table.size()) {
					table = c;
					source = d.first;
				}
			}
		}

		// the tables of the macros, largest first, and all of them joined
		// (a 16-channel module drawn in two halves of eight)
		QVector<Table> tables;
		for (const auto &d : std::as_const(docs)) {
			if (!fixed.contains(QFileInfo(dir.filePath(d.first)).absoluteFilePath())) continue;
			Table t{{}, d.first};
			const QDomElement ios = d.second.documentElement().firstChildElement(QStringLiteral("kindInformations"))
							.firstChildElement(QStringLiteral("plcMasterData")).firstChildElement(QStringLiteral("plcIOs"));
			for (QDomElement io = ios.firstChildElement(QStringLiteral("plcIO")); !io.isNull();
			     io = io.nextSiblingElement(QStringLiteral("plcIO"))) {
				Channel c{io.attribute(QStringLiteral("type")), io.attribute(QStringLiteral("address")), {}};
				for (QDomElement te = io.firstChildElement(QStringLiteral("terminal")); !te.isNull();
				     te = te.nextSiblingElement(QStringLiteral("terminal")))
					c.terminals << te.text();
				t.channels << c;
			}
			if (t.channels.size() >= 2) tables << t;
		}
		std::stable_sort(tables.begin(), tables.end(), [](const Table &a, const Table &b) {
			return a.channels.size() > b.channels.size();
		});
		if (tables.size() > 1) {
			QVector<Channel> joined;
			QHash<QString, QStringList> seen;
			bool consistent = true;
			for (const Table &t : std::as_const(tables))
				for (const Channel &c : t.channels) {
					if (!seen.contains(c.name)) {
						seen.insert(c.name, c.terminals);
						joined << c;
					} else if (seen.value(c.name) != c.terminals) {
						consistent = false;
					}
				}
			if (consistent && joined.size() > tables.first().channels.size()) {
				std::stable_sort(joined.begin(), joined.end(), [](const Channel &a, const Channel &b) {
					const QString pa = QString(a.name).remove(QRegularExpression(QStringLiteral("\\d+$")));
					const QString pb = QString(b.name).remove(QRegularExpression(QStringLiteral("\\d+$")));
					return pa != pb ? pa < pb : channelNumber(a.name) < channelNumber(b.name);
				});
				tables.prepend({joined, QStringLiteral("the EPLAN macro")});
			}
		}
		// a single "DO1" is a signalling contact, not an I/O module
		if (table.size() >= 2) tables << Table{table, source};
		if (tables.isEmpty()) continue;

		for (auto &d : docs) {
			const QString link = d.second.documentElement().attribute(QStringLiteral("link_type"));
			if (d.first.contains(QStringLiteral("_Panel_layout"))
			    || (link != QLatin1String("simple") && link != QLatin1String("master"))
			    || fixed.contains(QFileInfo(dir.filePath(d.first)).absoluteFilePath())
			    || ElementFingerprint::state(d.second) != ElementFingerprint::Unedited)
				continue;
			QSet<QString> has;
			for (const Spot &t : terminals(d.second)) has.insert(t.text);
			const Table *use = nullptr;
			for (const Table &t : std::as_const(tables)) {
				QSet<QString> needed;
				for (const Channel &c : t.channels)
					for (const QString &n : c.terminals) needed.insert(n);
				if (has.contains(needed)) {
					use = &t;
					break;
				}
			}
			if (!use || !setChannels(d.second, use->channels)) continue;
			ElementFingerprint::stamp(d.second);
			QFile file(dir.filePath(d.first));
			if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) continue;
			file.write(d.second.toByteArray(4));
			report << QStringLiteral("%1: PLC module with %2 channels (%3..%4) from %5")
				.arg(d.first).arg(use->channels.size())
				.arg(use->channels.first().name, use->channels.last().name, use->source);
		}
	}
	return report;
}
