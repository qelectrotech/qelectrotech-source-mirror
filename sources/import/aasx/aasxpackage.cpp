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
#include "aasxpackage.h"

#include "../../container/qetzip.h"

#include <QDomDocument>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPair>
#include <QSet>

namespace {
		/// the element name without its namespace prefix ("aas:property" -> "property")
	QString local(const QDomNode &n)
	{
		return n.nodeName().section(QLatin1Char(':'), -1);
	}

	QDomElement child(const QDomElement &e, const QString &name)
	{
		for (QDomElement c = e.firstChildElement(); !c.isNull(); c = c.nextSiblingElement())
			if (local(c) == name) return c;
		return {};
	}

	QString text(const QDomElement &e, const QString &name)
	{
		return child(e, name).text().trimmed();
	}

		/// the first key of a semanticId: v3 <keys><key><value>, v2 <keys><key>text
	QString semantic(const QDomElement &e)
	{
		const QDomElement keys = child(child(e, QStringLiteral("semanticId")), QStringLiteral("keys"));
		const QDomElement key = child(keys, QStringLiteral("key"));
		if (key.isNull()) return {};
		const QDomElement v = child(key, QStringLiteral("value"));
		return (v.isNull() ? key.text() : v.text()).trimmed();
	}

		/// one text of a multi-language value: English, then German, then the first
	QString pickLanguage(const QList<QPair<QString, QString>> &texts)
	{
		for (const QString &want : {QStringLiteral("en"), QStringLiteral("de")})
			for (const auto &t : texts)
				if (t.first.toLower().startsWith(want) && !t.second.trimmed().isEmpty())
					return t.second.trimmed();
		for (const auto &t : texts)
			if (!t.second.trimmed().isEmpty()) return t.second.trimmed();
		return {};
	}

	QString multiLanguageXml(const QDomElement &value)
	{
		QList<QPair<QString, QString>> texts;
		for (QDomElement s = value.firstChildElement(); !s.isNull(); s = s.nextSiblingElement()) {
			if (local(s) == QLatin1String("langString"))	// v2: <langString lang="EN">text
				texts << qMakePair(s.attribute(QStringLiteral("lang")), s.text());
			else	// v3: <langStringTextType><language/><text/>
				texts << qMakePair(text(s, QStringLiteral("language")), text(s, QStringLiteral("text")));
		}
		return pickLanguage(texts);
	}

	const QSet<QString> &leaves()
	{
		static const QSet<QString> s {QStringLiteral("property"), QStringLiteral("multiLanguageProperty"),
					      QStringLiteral("range"), QStringLiteral("file"), QStringLiteral("blob")};
		return s;
	}

	const QSet<QString> &containers()
	{
		static const QSet<QString> s {QStringLiteral("submodelElementCollection"),
					      QStringLiteral("submodelElementList"), QStringLiteral("entity")};
		return s;
	}

		/// metadata of an element, never walked into
	const QSet<QString> &skipped()
	{
		static const QSet<QString> s {QStringLiteral("semanticId"), QStringLiteral("supplementalSemanticIds"),
					      QStringLiteral("qualifiers"), QStringLiteral("qualifier"), QStringLiteral("description"),
					      QStringLiteral("displayName"), QStringLiteral("embeddedDataSpecifications"),
					      QStringLiteral("embeddedDataSpecification"), QStringLiteral("extensions"),
					      QStringLiteral("idShort"), QStringLiteral("category"), QStringLiteral("kind"),
					      QStringLiteral("administration"), QStringLiteral("identification"),
					      QStringLiteral("id"), QStringLiteral("valueId"), QStringLiteral("valueType")};
		return s;
	}

	void walkXml(const QDomElement &node, const QString &prefix, QVector<AasxElement> *out)
	{
		for (QDomElement c = node.firstChildElement(); !c.isNull(); c = c.nextSiblingElement()) {
			const QString ln = local(c);
			if (skipped().contains(ln)) continue;
			if (leaves().contains(ln)) {
				AasxElement e;
				e.id_short = text(c, QStringLiteral("idShort"));
				e.path = prefix + e.id_short;
				e.kind = ln.at(0).toUpper() + ln.mid(1);
				e.semantic_id = semantic(c);
				const QDomElement v = child(c, QStringLiteral("value"));
				if (ln == QLatin1String("multiLanguageProperty")) {
					e.value = multiLanguageXml(v);
				} else if (ln == QLatin1String("range")) {
					const QString lo = text(c, QStringLiteral("min")), hi = text(c, QStringLiteral("max"));
					e.value = lo.isEmpty() && hi.isEmpty() ? QString() : lo + QStringLiteral(" … ") + hi;
				} else {
					e.value = v.text().trimmed();
				}
				e.content_type = text(c, QStringLiteral("contentType"));
				if (e.content_type.isEmpty()) e.content_type = text(c, QStringLiteral("mimeType"));
				*out << e;
			} else if (containers().contains(ln)) {
				const QString id = text(c, QStringLiteral("idShort"));
				const QString sub = prefix + (id.isEmpty() ? QString() : id + QLatin1Char('/'));
				walkXml(child(c, QStringLiteral("value")), sub, out);
				walkXml(child(c, QStringLiteral("statements")), sub, out);	// entity
			} else {
				walkXml(c, prefix, out);	// wrappers: submodelElements, submodelElement, value
			}
		}
	}

		/// all <submodel> elements holding <submodelElements>, at any depth
	void findSubmodels(const QDomElement &e, QList<QDomElement> *out)
	{
		for (QDomElement c = e.firstChildElement(); !c.isNull(); c = c.nextSiblingElement()) {
			if (local(c) == QLatin1String("submodel") && !child(c, QStringLiteral("submodelElements")).isNull())
				*out << c;
			else
				findSubmodels(c, out);
		}
	}

	QString semanticJson(const QJsonObject &o)
	{
		const QJsonArray keys = o.value(QStringLiteral("semanticId")).toObject().value(QStringLiteral("keys")).toArray();
		return keys.isEmpty() ? QString() : keys.first().toObject().value(QStringLiteral("value")).toString();
	}

		/// modelType: v3 "Property", v2 {"name": "Property"}
	QString modelType(const QJsonObject &o)
	{
		const QJsonValue t = o.value(QStringLiteral("modelType"));
		return t.isObject() ? t.toObject().value(QStringLiteral("name")).toString() : t.toString();
	}

	QString multiLanguageJson(const QJsonValue &v)
	{
		const QJsonArray list = v.isArray() ? v.toArray() : v.toObject().value(QStringLiteral("langString")).toArray();
		QList<QPair<QString, QString>> texts;
		for (const QJsonValue &s : list) {
			const QJsonObject o = s.toObject();
			texts << qMakePair(o.value(QStringLiteral("language")).toString(),
					   o.value(QStringLiteral("text")).toString());
		}
		return pickLanguage(texts);
	}

	void walkJson(const QJsonArray &elements, const QString &prefix, QVector<AasxElement> *out)
	{
		for (const QJsonValue &v : elements) {
			QJsonObject o = v.toObject();
				// v2 wraps some lists in {"submodelElement": {...}}
			if (o.contains(QStringLiteral("submodelElement")))
				o = o.value(QStringLiteral("submodelElement")).toObject();
			const QString type = modelType(o);
			const QString id = o.value(QStringLiteral("idShort")).toString();
			if (type == QLatin1String("SubmodelElementCollection") || type == QLatin1String("SubmodelElementList")
			    || type == QLatin1String("Entity")) {
				const QString sub = prefix + (id.isEmpty() ? QString() : id + QLatin1Char('/'));
				walkJson(o.value(QStringLiteral("value")).toArray(), sub, out);
				walkJson(o.value(QStringLiteral("statements")).toArray(), sub, out);
				continue;
			}
			if (type != QLatin1String("Property") && type != QLatin1String("MultiLanguageProperty")
			    && type != QLatin1String("Range") && type != QLatin1String("File") && type != QLatin1String("Blob"))
				continue;
			AasxElement e;
			e.id_short = id;
			e.path = prefix + id;
			e.kind = type;
			e.semantic_id = semanticJson(o);
			if (type == QLatin1String("MultiLanguageProperty")) {
				e.value = multiLanguageJson(o.value(QStringLiteral("value")));
			} else if (type == QLatin1String("Range")) {
				const QString lo = o.value(QStringLiteral("min")).toVariant().toString();
				const QString hi = o.value(QStringLiteral("max")).toVariant().toString();
				e.value = lo.isEmpty() && hi.isEmpty() ? QString() : lo + QStringLiteral(" … ") + hi;
			} else {
				e.value = o.value(QStringLiteral("value")).toVariant().toString().trimmed();
			}
			e.content_type = o.value(QStringLiteral("contentType")).toString();
			if (e.content_type.isEmpty()) e.content_type = o.value(QStringLiteral("mimeType")).toString();
			*out << e;
		}
	}

		/// "/aasx/x.pdf", "aasx/x.pdf", "file:///aasx/x.pdf" -> "aasx/x.pdf"
	QString entryName(QString path)
	{
		path.remove(QStringLiteral("file://"));
		path.replace(QLatin1Char('\\'), QLatin1Char('/'));
		while (path.startsWith(QLatin1Char('/'))) path.remove(0, 1);
		return path;
	}

	bool isBookkeeping(const QString &name)
	{
		const QString n = name.toLower();
		return n.endsWith(QLatin1String(".rels")) || n.contains(QLatin1String("[content_types]"))
		       || n.contains(QLatin1String("aasx-origin")) || n.endsWith(QLatin1Char('/'));
	}
}

bool AasxPackage::open(const QString &path)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		m_error = QStringLiteral("%1: %2").arg(path, f.errorString());
		return false;
	}
	return openData(f.readAll());
}

bool AasxPackage::openData(const QByteArray &zip)
{
	m_entries.clear();
	m_submodels.clear();
	m_spec.clear();
	m_error.clear();
	QList<QetZip::Entry> entries;
	QString error;
	if (!QetZip::fromBytes(zip, &entries, &error)) {
		m_error = QStringLiteral("not an AASX package (ZIP): %1").arg(error);
		return false;
	}
	for (const QetZip::Entry &e : entries) m_entries.insert(entryName(e.name), e.data);

		// the AAS file: the target of an aas-spec relationship
	for (auto it = m_entries.constBegin(); it != m_entries.constEnd() && m_spec.isEmpty(); ++it) {
		if (!it.key().endsWith(QLatin1String(".rels"))) continue;
		QDomDocument rels;
		if (!rels.setContent(it.value())) continue;
		for (QDomElement r = rels.documentElement().firstChildElement(); !r.isNull(); r = r.nextSiblingElement()) {
			if (!r.attribute(QStringLiteral("Type")).endsWith(QLatin1String("aas-spec"))) continue;
			QString target = r.attribute(QStringLiteral("Target"));
			if (!target.startsWith(QLatin1Char('/'))) {
					// relative to the folder of the part the .rels belongs to:
					// aasx/_rels/aasx-origin.rels -> aasx/
				const QString dir = it.key().section(QLatin1Char('/'), 0, -3);
				if (!dir.isEmpty()) target = dir + QLatin1Char('/') + target;
			}
			m_spec = entryName(target);
			break;
		}
	}
		// else the first .xml/.json that is no OPC bookkeeping
	if (m_spec.isEmpty() || !m_entries.contains(m_spec)) {
		m_spec.clear();
		for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
			const QString n = it.key().toLower();
			if (isBookkeeping(n) || n.startsWith(QLatin1String("_rels"))) continue;
			if (n.endsWith(QLatin1String(".xml")) || n.endsWith(QLatin1String(".json"))) {
				m_spec = it.key();
				break;
			}
		}
	}
	if (m_spec.isEmpty()) {
		m_error = QStringLiteral("the package holds no AAS file");
		return false;
	}
	const QByteArray data = m_entries.value(m_spec);
	const bool ok = m_spec.endsWith(QLatin1String(".json"), Qt::CaseInsensitive) ? parseJson(data) : parseXml(data);
	if (ok && m_submodels.isEmpty()) {
		m_error = QStringLiteral("the AAS file %1 holds no submodel").arg(m_spec);
		return false;
	}
	return ok;
}

bool AasxPackage::parseXml(const QByteArray &data)
{
	QDomDocument doc;
	if (!doc.setContent(data)) {
		m_error = QStringLiteral("%1: no readable XML").arg(m_spec);
		return false;
	}
	QList<QDomElement> found;
	findSubmodels(doc.documentElement(), &found);
	for (const QDomElement &s : found) {
		AasxSubmodel sm;
		sm.id_short = text(s, QStringLiteral("idShort"));
		sm.id = text(s, QStringLiteral("id"));
		if (sm.id.isEmpty()) sm.id = text(s, QStringLiteral("identification"));
		sm.semantic_id = semantic(s);
		walkXml(child(s, QStringLiteral("submodelElements")), QString(), &sm.elements);
		m_submodels << sm;
	}
	return true;
}

bool AasxPackage::parseJson(const QByteArray &data)
{
	QJsonParseError parse;
	const QJsonDocument doc = QJsonDocument::fromJson(data, &parse);
	if (parse.error != QJsonParseError::NoError) {
		m_error = QStringLiteral("%1: %2").arg(m_spec, parse.errorString());
		return false;
	}
	const QJsonArray submodels = doc.object().value(QStringLiteral("submodels")).toArray();
	for (const QJsonValue &v : submodels) {
		const QJsonObject s = v.toObject();
		AasxSubmodel sm;
		sm.id_short = s.value(QStringLiteral("idShort")).toString();
		sm.id = s.value(QStringLiteral("id")).toString();
		if (sm.id.isEmpty())
			sm.id = s.value(QStringLiteral("identification")).toObject().value(QStringLiteral("id")).toString();
		sm.semantic_id = semanticJson(s);
		walkJson(s.value(QStringLiteral("submodelElements")).toArray(), QString(), &sm.elements);
		m_submodels << sm;
	}
	return true;
}

QStringList AasxPackage::files() const
{
	QStringList list;
	for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it)
		if (it.key() != m_spec && !isBookkeeping(it.key())) list << it.key();
	list.sort();
	return list;
}

QByteArray AasxPackage::file(const QString &path) const
{
	return m_entries.value(entryName(path));
}

QString AasxPackage::value(const QStringList &id_shorts, const QString &submodel) const
{
	for (int pass = 0; pass < 2; ++pass) {
		for (const AasxSubmodel &s : m_submodels) {
			if (pass == 0 && (submodel.isEmpty() || !s.id_short.contains(submodel, Qt::CaseInsensitive)))
				continue;
			for (const QString &name : id_shorts)
				for (const AasxElement &e : s.elements)
					if (e.id_short.compare(name, Qt::CaseInsensitive) == 0 && !e.value.isEmpty())
						return e.value;
		}
	}
	return {};
}

QVector<AasxElement> AasxPackage::fileElements() const
{
	QVector<AasxElement> list;
	for (const AasxSubmodel &s : m_submodels)
		for (const AasxElement &e : s.elements)
			if (e.kind == QLatin1String("File") && !e.value.isEmpty()) list << e;
	return list;
}

QString AasxPackage::manufacturer() const
{
	return value({QStringLiteral("ManufacturerName")}, QStringLiteral("Nameplate"));
}

QString AasxPackage::orderNumber() const
{
	return value({QStringLiteral("OrderCodeOfManufacturer"), QStringLiteral("ManufacturerOrderCode"),
		      QStringLiteral("ProductArticleNumberOfManufacturer"), QStringLiteral("ManufacturerArticleNumber")},
		     QStringLiteral("Nameplate"));
}

QString AasxPackage::typeNumber() const
{
	return value({QStringLiteral("ProductArticleNumberOfManufacturer"), QStringLiteral("ManufacturerArticleNumber"),
		      QStringLiteral("ManufacturerProductType"), QStringLiteral("OrderCodeOfManufacturer"),
		      QStringLiteral("ManufacturerOrderCode")},
		     QStringLiteral("Nameplate"));
}

QString AasxPackage::designation() const
{
	return value({QStringLiteral("ManufacturerProductDesignation"), QStringLiteral("ManufacturerProductFamily")},
		     QStringLiteral("Nameplate"));
}
