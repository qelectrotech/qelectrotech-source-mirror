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
#include "elementfingerprint.h"

#include <QCryptographicHash>
#include <QHash>
#include <QSet>
#include <QStringList>

#include <algorithm>

namespace {
	const QString KEY = QStringLiteral("import_hash");

	bool isFingerprint(const QDomElement &e)
	{
		return e.tagName() == QLatin1String("elementInformation") && e.attribute(QStringLiteral("name")) == KEY;
	}

	void add(QByteArray &out, char tag, const QString &s)
	{
		const QByteArray b = s.toUtf8();
		out += tag;
		out += QByteArray::number(b.size());
		out += ':';
		out += b;
	}

	/// tags, attributes sorted by name and texts with their lengths: the
	/// same for the element in memory and read back from its file
	void canonical(const QDomNode &node, QByteArray &out)
	{
		for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
			if (n.isElement()) {
				const QDomElement e = n.toElement();
				if (isFingerprint(e)) continue;
				add(out, '<', e.tagName());
				const QDomNamedNodeMap attrs = e.attributes();
				QVector<QPair<QString, QString>> sorted;
				for (int i = 0; i < attrs.count(); ++i) {
					const QDomAttr a = attrs.item(i).toAttr();
					sorted.append({a.name(), a.value()});
				}
				std::sort(sorted.begin(), sorted.end());
				for (const auto &a : std::as_const(sorted)) {
					add(out, '@', a.first);
					add(out, '=', a.second);
				}
				canonical(e, out);
				out += '>';
			} else if (n.isText() || n.isCDATASection()) {
				// the parser drops whitespace between elements and turns CR LF into LF
				QString s = n.toCharacterData().data();
				s.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
				if (!s.trimmed().isEmpty()) add(out, 'T', s);
			}
		}
	}

	QDomElement fingerprintEntry(const QDomDocument &element)
	{
		const QDomElement infos = element.documentElement().firstChildElement(QStringLiteral("elementInformations"));
		for (QDomElement e = infos.firstChildElement(QStringLiteral("elementInformation")); !e.isNull();
		     e = e.nextSiblingElement(QStringLiteral("elementInformation")))
			if (isFingerprint(e)) return e;
		return QDomElement();
	}
}

QString ElementFingerprint::compute(const QDomDocument &element)
{
	QByteArray data;
	canonical(element, data);
	return QStringLiteral("v1-") + QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

ElementFingerprint::State ElementFingerprint::state(const QDomDocument &element)
{
	const QDomElement entry = fingerprintEntry(element);
	if (entry.isNull() || entry.text().trimmed().isEmpty()) return Missing;
	return entry.text().trimmed() == compute(element) ? Unedited : Edited;
}

void ElementFingerprint::stamp(QDomDocument &element)
{
	QDomElement defn = element.documentElement();
	if (defn.isNull()) return;
	QDomElement infos = defn.firstChildElement(QStringLiteral("elementInformations"));
	if (infos.isNull()) {
		infos = element.createElement(QStringLiteral("elementInformations"));
		const QDomElement names = defn.firstChildElement(QStringLiteral("names"));
		if (!names.isNull()) defn.insertAfter(infos, names);
		else defn.appendChild(infos);
	}
	for (QDomElement old = fingerprintEntry(element); !old.isNull(); old = fingerprintEntry(element))
		old.parentNode().removeChild(old);
	const QString hash = compute(element);
	QDomElement entry = element.createElement(QStringLiteral("elementInformation"));
	entry.setAttribute(QStringLiteral("name"), KEY);
	entry.setAttribute(QStringLiteral("show"), QStringLiteral("0"));
	entry.appendChild(element.createTextNode(hash));
	infos.appendChild(entry);
}

void ElementFingerprint::keepIdentity(QDomDocument &element, const QDomDocument &old)
{
	QDomElement defn = element.documentElement();
	const QDomElement old_defn = old.documentElement();
	if (defn.isNull() || old_defn.isNull()) return;

	const QString uuid = old_defn.firstChildElement(QStringLiteral("uuid")).attribute(QStringLiteral("uuid"));
	if (!uuid.isEmpty()) {
		QDomElement u = defn.firstChildElement(QStringLiteral("uuid"));
		if (u.isNull()) {
			u = element.createElement(QStringLiteral("uuid"));
			defn.insertBefore(u, defn.firstChild());
		}
		u.setAttribute(QStringLiteral("uuid"), uuid);
	}

	const QDomElement old_names = old_defn.firstChildElement(QStringLiteral("names"));
	if (!old_names.isNull()) {
		const QDomNode names = element.importNode(old_names, true);
		const QDomElement current = defn.firstChildElement(QStringLiteral("names"));
		if (current.isNull()) defn.insertAfter(names, defn.firstChildElement(QStringLiteral("uuid")));
		else defn.replaceChild(names, current);
	}

	// the old information first, as the user may have filled it in;
	// what the import adds since comes after it
	const QDomElement old_infos = old_defn.firstChildElement(QStringLiteral("elementInformations"));
	QDomElement infos = defn.firstChildElement(QStringLiteral("elementInformations"));
	if (!old_infos.isNull() && !infos.isNull()) {
		QDomElement merged = element.createElement(QStringLiteral("elementInformations"));
		QSet<QString> have;
		for (QDomElement e = old_infos.firstChildElement(QStringLiteral("elementInformation")); !e.isNull();
		     e = e.nextSiblingElement(QStringLiteral("elementInformation"))) {
			if (isFingerprint(e)) continue;
			have.insert(e.attribute(QStringLiteral("name")));
			merged.appendChild(element.importNode(e, true));
		}
		for (QDomElement e = infos.firstChildElement(QStringLiteral("elementInformation")); !e.isNull();
		     e = e.nextSiblingElement(QStringLiteral("elementInformation")))
			if (!isFingerprint(e) && !have.contains(e.attribute(QStringLiteral("name"))))
				merged.appendChild(e.cloneNode(true));
		defn.replaceChild(merged, infos);
	}

	// terminals are told apart from the channel names of a plcIO by their position
	QHash<QString, QString> terminal_uuid;
	const QDomNodeList old_terminals = old.elementsByTagName(QStringLiteral("terminal"));
	for (int i = 0; i < old_terminals.size(); ++i) {
		const QDomElement t = old_terminals.at(i).toElement();
		if (t.hasAttribute(QStringLiteral("x")) && t.hasAttribute(QStringLiteral("uuid")))
			terminal_uuid.insert(t.attribute(QStringLiteral("name")), t.attribute(QStringLiteral("uuid")));
	}
	const QDomNodeList terminals = element.elementsByTagName(QStringLiteral("terminal"));
	for (int i = 0; i < terminals.size(); ++i) {
		QDomElement t = terminals.at(i).toElement();
		const QString name = t.attribute(QStringLiteral("name"));
		if (t.hasAttribute(QStringLiteral("x")) && terminal_uuid.contains(name))
			t.setAttribute(QStringLiteral("uuid"), terminal_uuid.value(name));
	}
}
