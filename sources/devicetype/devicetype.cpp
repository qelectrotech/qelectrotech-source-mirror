/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "devicetype.h"

#include <qetapp.h>

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>

/**
	@brief DeviceType::text
	@return the type's text in @p lang, else in English, else any
*/
QString DeviceType::text(const QString &lang) const
{
		//pt_BR, de_DE: then the base language, as NamesList::name(); an
		//empty name is no name
	const QString base = lang.section(QLatin1Char('_'), 0, 0);
	for (const QString &key : {lang, base, QStringLiteral("en")})
		if (!names.value(key).isEmpty()) return names.value(key);
	return names.isEmpty() ? QString() : names.first();
}

bool DeviceType::operator==(const DeviceType &o) const
{
	return name == o.name && version == o.version && names == o.names
			&& informations == o.informations && parts == o.parts;
}

/**
	@brief DeviceType::toXml
	@return the type as a <devicetype> element of @p document
*/
QDomElement DeviceType::toXml(QDomDocument &document) const
{
	QDomElement root = document.createElement(QStringLiteral("devicetype"));
	root.setAttribute(QStringLiteral("name"), name);
	root.setAttribute(QStringLiteral("version"), version);
	if (!names.isEmpty()) {
		QDomElement xml_names = document.createElement(QStringLiteral("names"));
		for (auto it = names.constBegin() ; it != names.constEnd() ; ++it) {
			QDomElement n = document.createElement(QStringLiteral("name"));
			n.setAttribute(QStringLiteral("lang"), it.key());
			n.appendChild(document.createTextNode(it.value()));
			xml_names.appendChild(n);
		}
		root.appendChild(xml_names);
	}
	if (!informations.isEmpty()) {
		QDomElement xml_infos = document.createElement(QStringLiteral("informations"));
		for (const auto &info : informations) {
			QDomElement i = document.createElement(QStringLiteral("information"));
			i.setAttribute(QStringLiteral("name"), info.first);
			i.appendChild(document.createTextNode(info.second));
			xml_infos.appendChild(i);
		}
		root.appendChild(xml_infos);
	}
	QDomElement xml_parts = document.createElement(QStringLiteral("parts"));
	for (const Part &part : parts) {
		QDomElement p = document.createElement(QStringLiteral("part"));
		p.setAttribute(QStringLiteral("role"), part.role);
		if (!part.kind.isEmpty()) p.setAttribute(QStringLiteral("kind"), part.kind);
		if (!part.state.isEmpty()) p.setAttribute(QStringLiteral("state"), part.state);
		for (const Symbol &symbol : part.symbols) {
			QDomElement s = document.createElement(QStringLiteral("symbol"));
			s.setAttribute(QStringLiteral("location"), symbol.location);
			s.setAttribute(QStringLiteral("pins"), symbol.pins.join(QLatin1Char(' ')));
			if (!symbol.state.isEmpty()) s.setAttribute(QStringLiteral("state"), symbol.state);
			p.appendChild(s);
		}
		xml_parts.appendChild(p);
	}
	root.appendChild(xml_parts);
	return root;
}

/**
	@brief DeviceType::fromXml
	@return the type a <devicetype> element describes; a null type when
	@p element is not one or has no name
*/
DeviceType DeviceType::fromXml(const QDomElement &element)
{
	DeviceType type;
	if (element.tagName() != QLatin1String("devicetype")) return type;
	type.name = element.attribute(QStringLiteral("name")).trimmed();
	type.version = element.attribute(QStringLiteral("version"), QStringLiteral("1")).toInt();
	const QDomElement xml_names = element.firstChildElement(QStringLiteral("names"));
	for (QDomElement n = xml_names.firstChildElement(QStringLiteral("name")) ; !n.isNull()
		 ; n = n.nextSiblingElement(QStringLiteral("name")))
		type.names.insert(n.attribute(QStringLiteral("lang"), QStringLiteral("en")), n.text());
	const QDomElement xml_infos = element.firstChildElement(QStringLiteral("informations"));
	for (QDomElement i = xml_infos.firstChildElement(QStringLiteral("information")) ; !i.isNull()
		 ; i = i.nextSiblingElement(QStringLiteral("information")))
		type.informations.append({i.attribute(QStringLiteral("name")), i.text()});
	const QDomElement xml_parts = element.firstChildElement(QStringLiteral("parts"));
	for (QDomElement p = xml_parts.firstChildElement(QStringLiteral("part")) ; !p.isNull()
		 ; p = p.nextSiblingElement(QStringLiteral("part"))) {
		Part part;
		part.role = p.attribute(QStringLiteral("role"), QStringLiteral("simple"));
		part.kind = p.attribute(QStringLiteral("kind"));
		part.state = p.attribute(QStringLiteral("state"));
		for (QDomElement s = p.firstChildElement(QStringLiteral("symbol")) ; !s.isNull()
			 ; s = s.nextSiblingElement(QStringLiteral("symbol"))) {
			Symbol symbol;
			symbol.location = s.attribute(QStringLiteral("location"));
			symbol.pins = s.attribute(QStringLiteral("pins")).split(QLatin1Char(' '), Qt::SkipEmptyParts);
			symbol.state = s.attribute(QStringLiteral("state"));
			part.symbols.append(symbol);
		}
		type.parts.append(part);
	}
	return type;
}

namespace DeviceTypeLibrary
{

	/**
		@return the user's device type folder, made when missing
	*/
	QString folder()
	{
		const QString path = QETApp::configDir() + QStringLiteral("/devicetypes");
		QDir().mkpath(path);
		return path;
	}

	/**
		@return the file name a type is kept in: its name, with the
		characters a file name may not hold replaced, and ".qetdt"
	*/
	QString fileName(const QString &type_name)
	{
		QString safe = type_name;
		for (const QChar c : {QChar('/'), QChar('\\'), QChar(':'), QChar('*'), QChar('?'),
							  QChar('"'), QChar('<'), QChar('>'), QChar('|')})
			safe.replace(c, QLatin1Char('_'));
		return safe + QStringLiteral(".qetdt");
	}

	static QString folderOr(const QString &given)
	{
		return given.isEmpty() ? folder() : given;
	}

	/**
		@return the names of the types in @p folder (the user's by default),
		sorted
	*/
	QStringList names(const QString &in)
	{
		QStringList list;
		const QDir dir(folderOr(in));
		for (const QString &file : dir.entryList({QStringLiteral("*.qetdt")}, QDir::Files)) {
			const DeviceType type = [&]() {
				QFile f(dir.filePath(file));
				QDomDocument doc;
				if (!f.open(QIODevice::ReadOnly) || !doc.setContent(&f)) return DeviceType();
				return DeviceType::fromXml(doc.documentElement());
			}();
			if (!type.isNull()) list << type.name;
		}
		list.sort(Qt::CaseInsensitive);
		return list;
	}

	DeviceType load(const QString &type_name, const QString &in)
	{
		QFile f(QDir(folderOr(in)).filePath(fileName(type_name)));
		QDomDocument doc;
		if (!f.open(QIODevice::ReadOnly) || !doc.setContent(&f)) return DeviceType();
		return DeviceType::fromXml(doc.documentElement());
	}

	bool save(const DeviceType &type, const QString &in)
	{
		if (type.isNull()) return false;
		QDomDocument doc;
		doc.appendChild(doc.createProcessingInstruction(
				QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));
		doc.appendChild(type.toXml(doc));
		QSaveFile f(QDir(folderOr(in)).filePath(fileName(type.name)));
		if (!f.open(QIODevice::WriteOnly)) return false;
		QTextStream out(&f);
		out << doc.toString(4);
		out.flush();
		return f.commit();
	}

	bool remove(const QString &type_name, const QString &in)
	{
		return QFile::remove(QDir(folderOr(in)).filePath(fileName(type_name)));
	}
}
