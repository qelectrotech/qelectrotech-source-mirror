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
#include "macropackage.h"

#include "../../container/miniz/miniz.h"
#include "../../container/qetzip.h"
#include "../eplandxf/eplanpartimporter.h"

#include <QCollator>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QStringDecoder>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>

namespace {
	/// A single file of a package is a macro or article data: megabytes
	constexpr qint64 MaxEntrySize = qint64(256) * 1024 * 1024;
	/// raise when the stored list of articles changes its form or its rules
	constexpr int IndexVersion = 1;

	/// EPLAN multilingual text "de_DE@Text;en_US@Text;" -> German, else
	/// English, else the first language; plain text is kept as it is
	QString language(const QString &value)
	{
		static const QRegularExpression mark(QStringLiteral("[a-z?]{2}_[A-Z?]{2}@"));
		QString german, english, first;
		QRegularExpressionMatchIterator it = mark.globalMatch(value);
		if (!it.hasNext()) return value.trimmed();
		QRegularExpressionMatch m = it.next();
		while (true) {
			const bool more = it.hasNext();
			const QRegularExpressionMatch next = more ? it.next() : QRegularExpressionMatch();
			QString text = value.mid(m.capturedEnd(), (more ? next.capturedStart() : value.size()) - m.capturedEnd()).trimmed();
			while (text.endsWith(QLatin1Char(';'))) text.chop(1);
			text = text.trimmed();
			const QString code = m.captured();
			if (!text.isEmpty()) {
				if (first.isEmpty()) first = text;
				if (german.isEmpty() && code.startsWith(QLatin1String("de_"))) german = text;
				if (english.isEmpty() && code.startsWith(QLatin1String("en_"))) english = text;
			}
			if (!more) break;
			m = next;
		}
		return !german.isEmpty() ? german : !english.isEmpty() ? english : first;
	}

	QString norm(QString s) { return s.remove(QLatin1Char(' ')).toUpper(); }

	/**
		Collection folders for the product families of the Beckhoff package
		(start of the macro's path below "Beckhoff/", lower case; the first
		match counts). Any other folder, and any other manufacturer's
		package, keeps the name of its folder.
	*/
	const QList<QPair<QString, QString>> FAMILIES {
		{QStringLiteral("io/ethercat terminals"), QStringLiteral("EtherCAT Terminals")},
		{QStringLiteral("io/ethercat coupler"), QStringLiteral("EtherCAT Couplers")},
		{QStringLiteral("io/ethercat box"), QStringLiteral("EtherCAT Box")},
		{QStringLiteral("io/ethercat p box"), QStringLiteral("EtherCAT P Box")},
		{QStringLiteral("io/ethernet and ethercat components"), QStringLiteral("Infrastructure")},
		{QStringLiteral("io/bus terminals"), QStringLiteral("Bus Terminals")},
		{QStringLiteral("io/bus terminal controller"), QStringLiteral("Bus Terminal Controllers")},
		{QStringLiteral("io/bus coupler"), QStringLiteral("Bus Couplers")},
		{QStringLiteral("io/fieldbus box"), QStringLiteral("Fieldbus Box")},
		{QStringLiteral("io/fieldbus modules"), QStringLiteral("Fieldbus Modules")},
		{QStringLiteral("io/io-link-box"), QStringLiteral("IO-Link Box")},
		{QStringLiteral("io/measuring transformers"), QStringLiteral("Measuring Transformers")},
		{QStringLiteral("io/power supplies"), QStringLiteral("Power Supplies")},
		{QStringLiteral("io/accessories"), QStringLiteral("Accessories")},
		{QStringLiteral("embedded pc/"), QStringLiteral("Embedded PC")},
		{QStringLiteral("ipc/control cabinet ipc"), QStringLiteral("Industrial PC")},
		{QStringLiteral("ipc/control-panel"), QStringLiteral("Panel")},
		{QStringLiteral("ipc/panel-pc"), QStringLiteral("Panel")},
		{QStringLiteral("ipc/pc-fieldbus cards"), QStringLiteral("Fieldbus Cards")},
		{QStringLiteral("ipc/options"), QStringLiteral("Industrial PC Options")},
		{QStringLiteral("motion/"), QStringLiteral("Motion")},
		{QStringLiteral("mx-system/"), QStringLiteral("MX-System")},
		{QStringLiteral("vision/"), QStringLiteral("Vision")}};

	/// "$(MD_MACROS)\Beckhoff\IO\EK9300.ema" -> "beckhoff/io/ek9300.ema"
	QString macroKey(QString reference)
	{
		static const QRegularExpression variable(QStringLiteral("^\\$\\([^)]*\\)[\\\\/]*"));
		reference.remove(variable);
		reference.replace(QLatin1Char('\\'), QLatin1Char('/'));
		while (reference.startsWith(QLatin1Char('/'))) reference.remove(0, 1);
		return reference.toLower();
	}
}

struct MacroPackage::Zip
{
	QByteArray bytes;
	mz_zip_archive archive;
	QHash<QString, mz_uint> index;   ///< path in the package -> entry
	bool open = false;
};

MacroPackage::MacroPackage() = default;

MacroPackage::~MacroPackage()
{
	if (m_zip && m_zip->open) mz_zip_reader_end(&m_zip->archive);
	delete m_zip;
}

/**
	Open the package at @p path, a ZIP file or an unpacked folder, and read
	its list of articles.
*/
bool MacroPackage::open(const QString &path)
{
	if (m_zip && m_zip->open) mz_zip_reader_end(&m_zip->archive);
	delete m_zip;
	m_zip = nullptr;
	m_path = path;
	m_error.clear();
	m_top_folder.clear();
	m_entries.clear();
	m_articles.clear();
	m_by_number.clear();
	m_macro_count = 0;
	m_loaded = false;

	if (!QFileInfo::exists(path)) {
		m_error = QStringLiteral("Cannot read %1").arg(path);
		return false;
	}
	// the list of articles of a ZIP is kept: the archive itself is then
	// only read when a macro is asked for
	if (readIndexCache()) return true;
	if (!loadArchive()) return false;
	if (!buildIndex()) return false;
	writeIndexCache();
	return true;
}

/// "<cache>/macropackages/<hash of the path>.json", empty for a folder
QString MacroPackage::indexCacheFile() const
{
	if (QFileInfo(m_path).isDir()) return QString();
	const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	if (dir.isEmpty()) return QString();
	const QByteArray hash = QCryptographicHash::hash(QFileInfo(m_path).absoluteFilePath().toUtf8(),
							   QCryptographicHash::Sha1).toHex();
	return dir + QStringLiteral("/macropackages/") + QString::fromLatin1(hash) + QStringLiteral(".json");
}

/// the list of articles as it was read last time, if the ZIP is still the same file
bool MacroPackage::readIndexCache()
{
	QFile f(indexCacheFile());
	if (f.fileName().isEmpty() || !f.open(QIODevice::ReadOnly)) return false;
	const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
	const QFileInfo zip(m_path);
	if (o.value(QStringLiteral("version")).toInt() != IndexVersion
	    || qint64(o.value(QStringLiteral("size")).toDouble()) != zip.size()
	    || qint64(o.value(QStringLiteral("modified")).toDouble()) != zip.lastModified().toMSecsSinceEpoch())
		return false;
	m_top_folder = o.value(QStringLiteral("top")).toString();
	m_macro_count = o.value(QStringLiteral("macros")).toInt();
	const QJsonArray list = o.value(QStringLiteral("articles")).toArray();
	m_articles.reserve(list.size());
	for (const QJsonValue &v : list) {
		const QJsonArray r = v.toArray();
		MacroPackageArticle a;
		a.part_number = r.at(0).toString();
		a.order_number = r.at(1).toString();
		a.type_number = r.at(2).toString();
		a.manufacturer = r.at(3).toString();
		a.description = r.at(4).toString();
		a.macro = r.at(5).toString();
		a.category = r.at(6).toString();
		a.source = r.at(7).toString();
		m_articles << a;
	}
	return !m_articles.isEmpty();
}

void MacroPackage::writeIndexCache() const
{
	const QString file = indexCacheFile();
	if (file.isEmpty() || !QDir().mkpath(QFileInfo(file).absolutePath())) return;
	QJsonArray list;
	for (const MacroPackageArticle &a : m_articles)
		list << QJsonArray {a.part_number, a.order_number, a.type_number, a.manufacturer,
				    a.description, a.macro, a.category, a.source};
	const QFileInfo zip(m_path);
	const QJsonObject o {
		{QStringLiteral("version"), IndexVersion},
		{QStringLiteral("size"), double(zip.size())},
		{QStringLiteral("modified"), double(zip.lastModified().toMSecsSinceEpoch())},
		{QStringLiteral("top"), m_top_folder},
		{QStringLiteral("macros"), m_macro_count},
		{QStringLiteral("articles"), list}};
	QSaveFile out(file);
	if (!out.open(QIODevice::WriteOnly)) return;
	out.write(QJsonDocument(o).toJson(QJsonDocument::Compact));
	out.commit();
}

/// read the articles out of the files of the package
bool MacroPackage::buildIndex()
{
	const QString path = m_path;
	// the one folder everything lies in names the manufacturer ("Beckhoff/...")
	QSet<QString> tops;
	for (const QString &entry : std::as_const(m_entries))
		tops.insert(entry.contains(QLatin1Char('/')) ? entry.section(QLatin1Char('/'), 0, 0) : QString());
	if (tops.size() == 1) m_top_folder = *tops.cbegin();

	QStringList xml_files;
	for (const QString &entry : std::as_const(m_entries)) {
		if (entry.endsWith(QLatin1String(".ema"), Qt::CaseInsensitive)) ++m_macro_count;
		else if (entry.endsWith(QLatin1String(".xml"), Qt::CaseInsensitive)) xml_files << entry;
	}
	if (m_macro_count == 0) {
		m_error = QStringLiteral("No EPLAN macro (*.ema) in %1: not a macro package").arg(path);
		return false;
	}
	xml_files.sort();
	for (const QString &entry : std::as_const(xml_files)) {
		const QByteArray data = read(entry);
		if (!data.isEmpty()) indexParts(entry, data);
	}

	// the macro of each article, and the macros no article names: those
	// are offered by their file name, without article data
	QSet<QString> named;
	for (MacroPackageArticle &a : m_articles) {
		const QString key = macroKey(a.macro);
		QString found;
		if (!key.isEmpty()) {
			const QStringList tries {
				key,
				m_top_folder.isEmpty() ? key : m_top_folder.toLower() + QLatin1Char('/') + key,
				key.section(QLatin1Char('/'), 1)};
			for (const QString &t : tries) {
				if (!t.isEmpty() && m_entries.contains(t)) {
					found = m_entries.value(t);
					break;
				}
			}
		}
		if (found.isEmpty()) {
			// a macro beside the article data, named like the article
			const QString dir = a.source.contains(QLatin1Char('/'))
						    ? a.source.section(QLatin1Char('/'), 0, -2) + QLatin1Char('/') : QString();
			for (const QString &name : {a.type_number, a.order_number}) {
				const QString t = (dir + name + QStringLiteral(".ema")).toLower();
				if (!name.isEmpty() && m_entries.contains(t)) {
					found = m_entries.value(t);
					break;
				}
			}
		}
		a.macro = found;
		if (!found.isEmpty()) named.insert(found);
	}
	for (const QString &entry : std::as_const(m_entries)) {
		if (!entry.endsWith(QLatin1String(".ema"), Qt::CaseInsensitive) || named.contains(entry)) continue;
		MacroPackageArticle a;
		a.type_number = QFileInfo(entry).completeBaseName();
		a.macro = entry;
		m_articles << a;
	}
	for (MacroPackageArticle &a : m_articles) {
		if (a.macro.isEmpty()) continue;
		// a short name for the product families of a known package
		QString below_top = a.macro.toLower();
		if (!m_top_folder.isEmpty() && below_top.startsWith(m_top_folder.toLower() + QLatin1Char('/')))
			below_top.remove(0, m_top_folder.size() + 1);
		for (const auto &family : FAMILIES) {
			if (below_top.startsWith(family.first)) {
				a.category = family.second;
				break;
			}
		}
		if (!a.category.isEmpty()) continue;
		// "IO/EtherCAT Terminals BECKHOFF ELxxxx/EL1809.ema" -> "EtherCAT Terminals ELxxxx"
		QStringList words = a.macro.section(QLatin1Char('/'), -2, -2).simplified().split(QLatin1Char(' '));
		const QString maker = manufacturerName(a);
		words.removeIf([&](const QString &w) {
			return w.compare(maker, Qt::CaseInsensitive) == 0
			       || (!m_top_folder.isEmpty() && w.compare(m_top_folder, Qt::CaseInsensitive) == 0);
		});
		a.category = words.join(QLatin1Char(' '));
	}

	QCollator order;
	order.setNumericMode(true);
	order.setCaseSensitivity(Qt::CaseInsensitive);
	std::stable_sort(m_articles.begin(), m_articles.end(),
			 [&order](const MacroPackageArticle &a, const MacroPackageArticle &b) {
				 return order.compare(a.type(), b.type()) < 0;
			 });
	m_by_number.clear();   // the indexes moved; article() looks through the list
	return true;
}

/// list the files of the package; a ZIP is read into memory, never unpacked
bool MacroPackage::loadArchive()
{
	const QString path = m_path;
	m_entries.clear();
	const QFileInfo info(path);
	if (info.isDir()) {
		const QDir root(path);
		QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			const QString entry = root.relativeFilePath(it.filePath());
			m_entries.insert(entry.toLower(), entry);
		}
	} else {
		QFile f(path);
		if (!f.open(QIODevice::ReadOnly)) {
			m_error = QStringLiteral("Cannot read %1").arg(path);
			return false;
		}
		m_zip = new Zip;
		m_zip->bytes = f.readAll();
		mz_zip_zero_struct(&m_zip->archive);
		if (!mz_zip_reader_init_mem(&m_zip->archive, m_zip->bytes.constData(), size_t(m_zip->bytes.size()), 0)) {
			m_error = QStringLiteral("Not a zip archive: %1").arg(path);
			return false;
		}
		m_zip->open = true;
		const mz_uint count = mz_zip_reader_get_num_files(&m_zip->archive);
		for (mz_uint i = 0; i < count && i < mz_uint(QetZip::MaxEntries); ++i) {
			mz_zip_archive_file_stat stat;
			if (!mz_zip_reader_file_stat(&m_zip->archive, i, &stat) || stat.m_is_directory
			    || stat.m_is_encrypted || !stat.m_is_supported)
				continue;
			// names are UTF-8 or, in older archives, the DOS code page
			QStringDecoder utf8(QStringConverter::Utf8);
			QString entry = utf8(QByteArrayView(stat.m_filename));
			if (utf8.hasError()) entry = QString::fromLatin1(stat.m_filename);
			entry.replace(QLatin1Char('\\'), QLatin1Char('/'));
			m_entries.insert(entry.toLower(), entry);
			m_zip->index.insert(entry, i);
		}
	}
	m_loaded = true;
	return true;
}

/// the articles of one parts management document; any other XML is passed over
void MacroPackage::indexParts(const QString &entry, const QByteArray &xml_data)
{
	QXmlStreamReader xml(xml_data);
	if (!xml.readNextStartElement() || xml.name() != QLatin1String("partsmanagement")) return;
	while (xml.readNextStartElement()) {
		if (xml.name() == QLatin1String("part")) {
			const QXmlStreamAttributes at = xml.attributes();
			MacroPackageArticle a;
			a.part_number = at.value(QLatin1String("P_ARTICLE_PARTNR")).toString();
			a.order_number = at.value(QLatin1String("P_ARTICLE_ORDERNR")).toString();
			a.type_number = at.value(QLatin1String("P_ARTICLE_TYPENR")).toString();
			a.manufacturer = at.value(QLatin1String("P_ARTICLE_MANUFACTURER")).toString();
			a.description = language(at.value(QLatin1String("P_ARTICLE_DESCR1")).toString());
			a.macro = at.value(QLatin1String("P_ARTICLE_GROUPSYMBOLMACRO")).toString();
			if (a.macro.isEmpty()) a.macro = at.value(QLatin1String("P_ARTICLE_MACRO")).toString();
			a.source = entry;
			a.source_size = xml_data.size();
			const QString key = a.part_number.isEmpty() ? a.type() : a.part_number;
			if (!key.isEmpty()) {
				const auto known = m_by_number.constFind(key);
				if (known == m_by_number.constEnd()) {
					m_by_number.insert(key, int(m_articles.size()));
					m_articles << a;
				} else if (a.source_size < m_articles.at(*known).source_size) {
					// the same article in a smaller file: quicker to read from
					m_articles[*known].source = a.source;
					m_articles[*known].source_size = a.source_size;
				}
			}
		}
		xml.skipCurrentElement();
	}
}

QByteArray MacroPackage::read(const QString &entry)
{
	if (!m_loaded && !loadArchive()) return QByteArray();
	if (!m_zip) {
		QFile f(QDir(m_path).filePath(entry));
		if (f.size() > MaxEntrySize || !f.open(QIODevice::ReadOnly)) return QByteArray();
		return f.readAll();
	}
	const auto it = m_zip->index.constFind(entry);
	if (it == m_zip->index.constEnd()) return QByteArray();
	mz_zip_archive_file_stat stat;
	if (!mz_zip_reader_file_stat(&m_zip->archive, *it, &stat) || qint64(stat.m_uncomp_size) > MaxEntrySize)
		return QByteArray();
	size_t size = 0;
	void *data = mz_zip_reader_extract_to_heap(&m_zip->archive, *it, &size, 0);
	if (!data) return QByteArray();
	const QByteArray out(static_cast<const char *>(data), qsizetype(size));
	mz_free(data);
	return out;
}

QString MacroPackage::manufacturerName(const MacroPackageArticle &article) const
{
	const QString name = EplanPartImporter::manufacturerName(article.manufacturer);
	// a short code the import does not know: the package's folder says it
	if ((name.isEmpty() || name == article.manufacturer) && !m_top_folder.isEmpty()) return m_top_folder;
	return name.isEmpty() ? QStringLiteral("Unbekannt") : name;
}

QVector<MacroPackageArticle> MacroPackage::find(const QString &text) const
{
	const QStringList words = text.toLower().split(QLatin1Char(' '), Qt::SkipEmptyParts);
	if (words.isEmpty()) return m_articles;
	QVector<MacroPackageArticle> out;
	for (const MacroPackageArticle &a : m_articles) {
		const QString hay = (a.type_number + QLatin1Char(' ') + a.order_number + QLatin1Char(' ')
				     + a.description + QLatin1Char(' ') + a.category).toLower();
		if (std::all_of(words.cbegin(), words.cend(), [&hay](const QString &w) { return hay.contains(w); }))
			out << a;
	}
	return out;
}

const MacroPackageArticle *MacroPackage::article(const QString &type) const
{
	const QString key = norm(type);
	if (key.isEmpty()) return nullptr;
	for (const MacroPackageArticle &a : m_articles) {
		if (norm(a.type_number) == key || norm(a.order_number) == key || norm(a.part_number) == key)
			return &a;
	}
	return nullptr;
}

QByteArray MacroPackage::macro(const MacroPackageArticle &article)
{
	return article.macro.isEmpty() ? QByteArray() : read(article.macro);
}

QByteArray MacroPackage::partXml(const MacroPackageArticle &article)
{
	if (article.source.isEmpty()) return QByteArray();
	const QByteArray data = read(article.source);
	QXmlStreamReader xml(data);
	if (!xml.readNextStartElement()) return QByteArray();
	while (xml.readNextStartElement()) {
		const QXmlStreamAttributes at = xml.attributes();
		const bool wanted = xml.name() == QLatin1String("part")
				    && (article.part_number.isEmpty()
						? at.value(QLatin1String("P_ARTICLE_TYPENR")) == article.type_number
						: at.value(QLatin1String("P_ARTICLE_PARTNR")) == article.part_number);
		if (!wanted) {
			xml.skipCurrentElement();
			continue;
		}
		QByteArray out;
		QXmlStreamWriter writer(&out);
		writer.writeStartDocument();
		writer.writeStartElement(QStringLiteral("partsmanagement"));
		int depth = 0;
		do {
			if (xml.isStartElement()) ++depth;
			else if (xml.isEndElement()) --depth;
			writer.writeCurrentToken(xml);
			if (depth > 0) xml.readNext();
		} while (depth > 0 && !xml.atEnd());
		writer.writeEndElement();
		writer.writeEndDocument();
		return xml.hasError() ? QByteArray() : out;
	}
	return QByteArray();
}
