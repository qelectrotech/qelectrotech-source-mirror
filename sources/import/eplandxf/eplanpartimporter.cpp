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
#include "eplanpartimporter.h"

#include "../elementfingerprint.h"
#include "../plcchannels.h"

#include "../../container/miniz/miniz.h"
#include "../../container/qetzip.h"
#include "../edz/edzarchive.h"
#include "../edz/edzimporter.h"
#include "../edz/edzpart.h"
#include "dwgreader.h"
#include "eplandxfreader.h"
#include "eplanelementbuilder.h"
#include "eplanmacroreader.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringDecoder>

#include <algorithm>
#include <cmath>

namespace {
	using Files = QMap<QString, QByteArray>;  // path in the package -> content

	using Article = QMap<QString, QString>;   // commercialdata.csv column -> value

	/// representation from the file suffix (_A1, _B4 ...): 1 multi-line,
	/// 2 single-line, 4 overview, 6 panel layout (front view)
	const QMap<QString, QString> REPRESENTATION {
		{QStringLiteral("1"), QString()}, {QStringLiteral("2"), QStringLiteral("Single-line")},
		{QStringLiteral("4"), QStringLiteral("Overview")}, {QStringLiteral("6"), QStringLiteral("Panel layout")}};

	/// macro of a package file: the folder below "dxf" (dxf/<macro>/<representation>/...)
	QString macroOf(const QString &path)
	{
		const QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
		for (int i = 0; i + 1 < parts.size(); ++i) {
			if (parts.at(i).compare(QLatin1String("dxf"), Qt::CaseInsensitive) == 0) return parts.at(i + 1);
		}
		return QString();
	}

	/// the article of a macro: the row whose order or type number is the macro name
	Article articleFor(const QList<Article> &articles, const QString &macro)
	{
		auto norm = [](QString s) { return s.remove(QLatin1Char(' ')).toUpper(); };
		const QString m = norm(macro);
		for (const Article &a : articles) {
			if (norm(a.value(QStringLiteral("Order number"))) == m
			    || norm(a.value(QStringLiteral("Type number"))) == m)
				return a;
		}
		for (const Article &a : articles) {
			const QString order = norm(a.value(QStringLiteral("Order number")));
			if (!order.isEmpty() && m.contains(order)) return a;
		}
		return articles.value(0);
	}

	/// file name without umlauts and special characters
	QString safe(QString name)
	{
		static const QList<QPair<QString, QString>> umlauts {
			{QStringLiteral("ä"), QStringLiteral("ae")}, {QStringLiteral("ö"), QStringLiteral("oe")},
			{QStringLiteral("ü"), QStringLiteral("ue")}, {QStringLiteral("Ä"), QStringLiteral("Ae")},
			{QStringLiteral("Ö"), QStringLiteral("Oe")}, {QStringLiteral("Ü"), QStringLiteral("Ue")},
			{QStringLiteral("ß"), QStringLiteral("ss")}};
		for (const auto &u : umlauts) name.replace(u.first, u.second);
		name.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._+-]+")), QStringLiteral("_"));
		while (name.startsWith(QLatin1Char('_'))) name.remove(0, 1);
		while (name.endsWith(QLatin1Char('_'))) name.chop(1);
		return name;
	}

	/// folder chain with qet_directory files (display name per level)
	bool ensureDirs(const QString &base, const QStringList &levels, QString *out, QString *error)
	{
		QDir dir(base);
		for (const QString &name : levels) {
			QString folder = name;
			folder.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*]")), QStringLiteral("-"));
			if (!dir.mkpath(folder) || !dir.cd(folder)) {
				*error = QStringLiteral("Cannot create folder %1").arg(dir.filePath(folder));
				return false;
			}
			QFile f(dir.filePath(QStringLiteral("qet_directory")));
			if (f.exists()) continue;
			QDomDocument doc;
			QDomElement root = doc.createElement(QStringLiteral("qet-directory"));
			QDomElement names = doc.createElement(QStringLiteral("names"));
			for (const QString &lang : {QStringLiteral("de"), QStringLiteral("en")}) {
				QDomElement n = doc.createElement(QStringLiteral("name"));
				n.setAttribute(QStringLiteral("lang"), lang);
				n.appendChild(doc.createTextNode(name));
				names.appendChild(n);
			}
			root.appendChild(names);
			doc.appendChild(root);
			if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
				*error = QStringLiteral("Cannot write %1").arg(f.fileName());
				return false;
			}
			f.write(doc.toString(4).toUtf8());
		}
		*out = dir.absolutePath();
		return true;
	}

	bool writeElement(const QDomDocument &doc, const QString &path, QString *error, int indent)
	{
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			*error = QStringLiteral("Cannot write %1").arg(path);
			return false;
		}
		file.write(doc.toString(indent).toUtf8());
		return true;
	}

	/// The packages are read into memory only, never unpacked to disk, so their
	/// entry names (EPLAN writes them with backslashes) need no checking
	/// beyond turning them into '/'-separated paths.
	bool readZip(const QString &path, Files *files, QString *error)
	{
		QFile f(path);
		if (!f.open(QIODevice::ReadOnly)) {
			*error = QStringLiteral("Cannot read %1").arg(path);
			return false;
		}
		const QByteArray bytes = f.readAll();
		mz_zip_archive zip;
		mz_zip_zero_struct(&zip);
		if (!mz_zip_reader_init_mem(&zip, bytes.constData(), size_t(bytes.size()), 0)) {
			*error = QStringLiteral("Not a zip archive: %1").arg(path);
			return false;
		}
		qint64 total = 0;
		const mz_uint count = mz_zip_reader_get_num_files(&zip);
		for (mz_uint i = 0; i < count && i < mz_uint(QetZip::MaxEntries); ++i) {
			mz_zip_archive_file_stat stat;
			if (!mz_zip_reader_file_stat(&zip, i, &stat) || stat.m_is_directory
			    || stat.m_is_encrypted || !stat.m_is_supported)
				continue;
			total += qint64(stat.m_uncomp_size);
			if (total > QetZip::MaxUncompressedSize) {
				*error = QStringLiteral("The archive is too large once unpacked");
				mz_zip_reader_end(&zip);
				return false;
			}
			size_t size = 0;
			void *data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
			if (!data) continue;
			QString name = QString::fromUtf8(stat.m_filename);
			name.replace(QLatin1Char('\\'), QLatin1Char('/'));
			files->insert(name, QByteArray(static_cast<const char *>(data), qsizetype(size)));
			mz_free(data);
		}
		mz_zip_reader_end(&zip);
		return true;
	}

	bool readFiles(const QString &path, Files *files, QString *error)
	{
		if (QFileInfo(path).isDir()) {
			QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
			const QDir root(path);
			while (it.hasNext()) {
				QFile f(it.next());
				if (f.open(QIODevice::ReadOnly))
					files->insert(root.relativeFilePath(f.fileName()), f.readAll());
			}
			return true;
		}
		return readZip(path, files, error);
	}

	/// The readme of the Data Portal says UCS-2; in practice UTF-8 or ANSI too.
	QString decodeCsv(const QByteArray &raw)
	{
		if (raw.startsWith("\xff\xfe") || raw.startsWith("\xfe\xff")) {
			QStringDecoder dec(raw.startsWith("\xff\xfe") ? QStringConverter::Utf16LE
								       : QStringConverter::Utf16BE);
			return dec(raw.mid(2));
		}
		QStringDecoder utf8(QStringConverter::Utf8);
		const QString text = utf8(raw);
		if (!utf8.hasError()) return text.startsWith(QChar(0xfeff)) ? text.mid(1) : text;
		QStringDecoder ansi("windows-1252");
		return ansi.isValid() ? QString(ansi(raw)) : QString::fromLatin1(raw);
	}

	/// rows of a comma-separated file with double-quoted fields
	QList<QStringList> parseCsv(const QString &text)
	{
		QList<QStringList> rows;
		QStringList row;
		QString field;
		bool quoted = false;
		for (int i = 0; i < text.size(); ++i) {
			const QChar c = text.at(i);
			if (quoted) {
				if (c == QLatin1Char('"')) {
					if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('"')) { field += c; ++i; }
					else quoted = false;
				} else {
					field += c;
				}
			} else if (c == QLatin1Char('"')) {
				quoted = true;
			} else if (c == QLatin1Char(',')) {
				row << field;
				field.clear();
			} else if (c == QLatin1Char('\n') || c == QLatin1Char('\r')) {
				if (c == QLatin1Char('\r') && i + 1 < text.size() && text.at(i + 1) == QLatin1Char('\n')) ++i;
				row << field;
				field.clear();
				if (!(row.size() == 1 && row.first().isEmpty())) rows << row;
				row.clear();
			} else {
				field += c;
			}
		}
		if (!field.isEmpty() || !row.isEmpty()) { row << field; rows << row; }
		return rows;
	}

	/// every article of the package (one per data row of commercialdata.csv)
	bool readPartData(const Files &files, QList<Article> *articles, QString *error)
	{
		const auto csv = std::find_if(files.keyBegin(), files.keyEnd(), [](const QString &n) {
			return n.endsWith(QLatin1String("commercialdata.csv"), Qt::CaseInsensitive);
		});
		if (csv == files.keyEnd()) {
			*error = QStringLiteral("No commercialdata.csv: not an EPLAN Data Portal DXF package");
			return false;
		}
		QList<QStringList> rows;
		for (const QStringList &r : parseCsv(decodeCsv(files.value(*csv)))) {
			if (!r.isEmpty() && !r.first().startsWith(QLatin1Char('#'))) rows << r;
		}
		if (rows.size() < 2) {
			*error = QStringLiteral("commercialdata.csv holds no part");
			return false;
		}
		for (int r = 1; r < rows.size(); ++r) {
			Article a;
			for (int i = 0; i < rows.at(0).size() && i < rows.at(r).size(); ++i)
				a.insert(rows.at(0).at(i), rows.at(r).at(i));
			articles->append(a);
		}
		return true;
	}

	/// first letters of the EPLAN device tag ("-KF1" -> "KF")
	QString codeLetters(const QString &device)
	{
		static const QRegularExpression re(QStringLiteral("^-?([A-Z]+)"));
		const auto m = re.match(device);
		return m.hasMatch() ? m.captured(1) : QStringLiteral("A");
	}

	double round1(double v) { return std::nearbyint(v * 10) / 10; }

	/// top left to bottom right
	void sortPins(QVector<EplanDxfPin> &pins)
	{
		std::stable_sort(pins.begin(), pins.end(), [](const EplanDxfPin &a, const EplanDxfPin &b) {
			const double ya = round1(a.pos.y()), yb = round1(b.pos.y());
			return ya != yb ? ya > yb : round1(a.pos.x()) < round1(b.pos.x());
		});
	}

	/// the largest Data/part.json read from a .wspak (a real one has a few KB)
	constexpr qint64 MaxPartJsonSize = 4 * 1024 * 1024;

	/// part data of a WSCAD Universe package (.wspak: a zip with "ws" in front, Data/part.json);
	/// its offsets count from the start of the file, prefix included, so it is read as it is
	QMap<QString, QString> wspakInfo(const QString &path)
	{
		QMap<QString, QString> out;
		QFile f(path);
		if (!f.open(QIODevice::ReadOnly)) return out;
		const QByteArray bytes = f.readAll();
		mz_zip_archive zip;
		mz_zip_zero_struct(&zip);
		if (!mz_zip_reader_init_mem(&zip, bytes.constData(), size_t(bytes.size()), 0)) return out;
		// WSCAD writes the entry names with backslashes ("Data\part.json")
		size_t size = 0;
		void *data = nullptr;
		for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip) && !data; ++i) {
			mz_zip_archive_file_stat stat;
			if (!mz_zip_reader_file_stat(&zip, i, &stat) || stat.m_is_directory) continue;
			const QString name = QString::fromUtf8(stat.m_filename).replace(QLatin1Char('\\'), QLatin1Char('/'));
			if (name.compare(QLatin1String("Data/part.json"), Qt::CaseInsensitive) != 0) continue;
				// the size the archive declares, before anything is unpacked: a
				// real part.json has a few KB, a crafted one could ask for GBs
			if (stat.m_uncomp_size > quint64(MaxPartJsonSize)) break;
			data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
		}
		if (data && size <= size_t(MaxPartJsonSize)) {
			const QJsonObject o = QJsonDocument::fromJson(
				QByteArray(static_cast<const char *>(data), qsizetype(size))).object();
			out.insert(QStringLiteral("manufacturer"), o.value(QStringLiteral("manufacturerName")).toString());
			out.insert(QStringLiteral("order"), o.value(QStringLiteral("number")).toString());
			out.insert(QStringLiteral("type"), o.value(QStringLiteral("typeName")).toString());
			// the translated name in English, else German, else the first description
			const QJsonObject names = o.value(QStringLiteral("translateName")).toObject();
			QString name = names.value(QStringLiteral("En")).toString();
			if (name.isEmpty()) name = names.value(QStringLiteral("De")).toString();
			if (name.isEmpty()) name = o.value(QStringLiteral("descriptionOne")).toString();
			out.insert(QStringLiteral("description"), name);
		}
		if (data) mz_free(data);
		mz_zip_reader_end(&zip);
		return out;
	}

	/// EPLAN cross references ("/7.1") and page marks ("1/2") do not belong in a symbol
	QVector<EplanDxfText> withoutXrefs(const QVector<EplanDxfText> &texts)
	{
		static const QRegularExpression xref(
			QStringLiteral("^(?:/\\d+(?:\\.\\d+)?|\\d+/\\d+)$"));
		QVector<EplanDxfText> out;
		for (const EplanDxfText &t : texts) {
			if (!xref.match(t.text.trimmed()).hasMatch()) out << t;
		}
		return out;
	}

	QString symbolName(const QMap<QString, QString> &art, const QString &var, const QString &repr,
			   const QVector<EplanDxfPin> &pins, int variant_count)
	{
		QString type = art.value(QStringLiteral("Type number"));
		if (type.isEmpty()) type = art.value(QStringLiteral("Order number"));
		QStringList parts {type};
		if (!REPRESENTATION.value(repr).isEmpty()) parts << REPRESENTATION.value(repr);
		QStringList funcs;
		for (const EplanDxfPin &p : pins) {
			if (!p.function.isEmpty()) funcs << p.function;
		}
		if (variant_count > 1 && !funcs.isEmpty())
			parts << (funcs.size() > 1 ? funcs.first() + QLatin1Char('-') + funcs.last() : funcs.first());
		if (variant_count > 1) parts << QStringLiteral("(%1%2)").arg(var, repr);
		return parts.join(QLatin1Char(' '));
	}

	/// name after the function of a device part, e.g. "CPU 1212FC Inputs + Supply"
	QString partName(const QString &base, const QVector<EplanDxfPin> &pins)
	{
		auto anyFunc = [&](const QStringList &prefixes) {
			return std::any_of(pins.cbegin(), pins.cend(), [&](const EplanDxfPin &p) {
				return std::any_of(prefixes.cbegin(), prefixes.cend(),
						   [&](const QString &x) { return p.function.startsWith(x); });
			});
		};
		QStringList labels;
		if (anyFunc({QStringLiteral("DI")})) labels << QStringLiteral("Inputs");
		if (anyFunc({QStringLiteral("DQ"), QStringLiteral("DO")})) labels << QStringLiteral("Outputs");
		if (anyFunc({QStringLiteral("AI")})) labels << QStringLiteral("Analog inputs");
		if (anyFunc({QStringLiteral("AQ")})) labels << QStringLiteral("Analog outputs");
		if (std::any_of(pins.cbegin(), pins.cend(),
				[](const EplanDxfPin &p) { return p.name.startsWith(QLatin1String("X1P")); }))
			labels << QStringLiteral("PROFINET");
		static const QRegularExpression digits(QStringLiteral("^[0-9]+"));
		if (std::any_of(pins.cbegin(), pins.cend(), [](const EplanDxfPin &p) {
			    const QString f = QString(p.function).remove(digits);
			    return f == QLatin1String("L+") || f == QLatin1String("M");
		    }))
			labels << QStringLiteral("Supply");
		return labels.isEmpty() ? base : base + QLatin1Char(' ') + labels.join(QStringLiteral(" + "));
	}
}

QString EplanPartImporter::manufacturerName(const QString &name)
{
	static const QMap<QString, QString> map {
		{QStringLiteral("Weidmueller"), QStringLiteral("Weidmüller")},
		{QStringLiteral("WEI"), QStringLiteral("Weidmüller")},
		{QStringLiteral("SIE"), QStringLiteral("Siemens")},
		{QStringLiteral("BEC"), QStringLiteral("Beckhoff")},
		{QStringLiteral("WAGO"), QStringLiteral("WAGO")},
		{QStringLiteral("ETN"), QStringLiteral("Eaton")}};
	return map.value(name, name);
}

/**
	@param macro the article of this macro; empty: the first article
	@return its commercialdata.csv columns, plus "Articles": the order
	numbers of all articles of the package, comma-separated
*/
QMap<QString, QString> EplanPartImporter::packageInfo(const QString &path, QString *error,
						      const QString &macro)
{
	Files files;
	QList<Article> articles;
	QString err;
	if (!readFiles(path, &files, &err) || !readPartData(files, &articles, &err)) {
		if (error) *error = err;
		return {};
	}
	Article a = articleFor(articles, macro);
	QStringList orders;
	for (const Article &x : std::as_const(articles)) orders << x.value(QStringLiteral("Order number"));
	a.insert(QStringLiteral("Articles"), orders.join(QStringLiteral(", ")));
	return a;
}

/**
	Import every variant of an EPLAN Data Portal DXF package.
	@param category folder below the manufacturer; empty: the product group
*/
bool EplanPartImporter::importPackage(const QString &path, const QString &category,
				      const QString &collection_dir)
{
	clear();

	Files files;
	QList<Article> articles;
	if (!readFiles(path, &files, &m_error) || !readPartData(files, &articles, &m_error)) return false;

	// a package may hold several articles, one dxf/<macro>/ folder each
	struct Variant { QString macro, var, repr; QByteArray data; };
	QVector<Variant> variants;
	QMap<QString, int> per_macro;  // variants of one representation of one article
	static const QRegularExpression dxf_name(QStringLiteral("_([A-Z])(\\d+)\\.dxf$"),
						 QRegularExpression::CaseInsensitiveOption);
	for (auto it = files.cbegin(); it != files.cend(); ++it) {
		const auto m = dxf_name.match(it.key());
		if (!m.hasMatch() || !REPRESENTATION.contains(m.captured(2))) continue;
		const QString macro = macroOf(it.key());
		variants.append({macro, m.captured(1).toUpper(), m.captured(2), it.value()});
		++per_macro[macro + QLatin1Char('/') + m.captured(2)];
	}
	if (variants.isEmpty()) {
		m_error = QStringLiteral("The package holds no schematic DXF");
		return false;
	}

	for (const Variant &v : std::as_const(variants)) {
		const Article art = articleFor(articles, v.macro);
		const QString full = art.value(QStringLiteral("Manufacturer (full name)"));
		const QString manufacturer = EplanPartImporter::manufacturerName(
			full.isEmpty() ? QStringLiteral("Unknown") : full);
		QString type = art.value(QStringLiteral("Type number"));
		if (type.isEmpty()) type = art.value(QStringLiteral("Order number"));
		if (type.isEmpty()) type = v.macro;
		QString descr = art.value(QStringLiteral("Description 2"));
		if (descr.isEmpty()) descr = art.value(QStringLiteral("Description 1"));
		if (descr.isEmpty()) descr = type;
		descr = descr.replace(QRegularExpression(QStringLiteral("\\s*,\\s+")), QStringLiteral(" ")).trimmed();
		QString folder_category = category;
		if (folder_category.isEmpty()) folder_category = art.value(QStringLiteral("Product group"));
		if (folder_category.isEmpty()) folder_category = QStringLiteral("General");

		QString folder;
		if (!ensureDirs(collection_dir, {manufacturer, folder_category, type}, &folder, &m_error))
			return false;

		EplanDxfReader reader;
		if (!reader.read(v.data)) {
			m_messages << QStringLiteral("%1 %2%3: %4").arg(type, v.var, v.repr, reader.errorString());
			continue;
		}
		EplanSymbolData data;
		data.lines = reader.lines();
		// EPLAN black-box frames (SPECIAL symbol 14) are kept apart by the reader;
		// in a package every DXF is one element, so its frames are drawn as they are
		for (const QRectF &bx : reader.boxes()) {
			data.lines << QLineF(bx.topLeft(), bx.topRight()) << QLineF(bx.topRight(), bx.bottomRight())
				   << QLineF(bx.bottomRight(), bx.bottomLeft()) << QLineF(bx.bottomLeft(), bx.topLeft());
		}
		data.circles = reader.circles();
		data.texts = withoutXrefs(reader.texts());
		data.pins = reader.pins();
		data.fields = reader.fields();
		// a schematic symbol needs its terminals; overview, single-line and
		// panel layout drawings are of use as graphics alone
		if (data.pins.isEmpty() && (v.repr == QLatin1String("1") || data.lines.isEmpty())) {
			m_messages << QStringLiteral("%1 %2%3: no terminals, skipped").arg(type, v.var, v.repr);
			continue;
		}
		sortPins(data.pins);
		const QString name = symbolName(art.isEmpty() ? Article {{QStringLiteral("Type number"), type}} : art,
						 v.var, v.repr, data.pins, per_macro.value(v.macro + QLatin1Char('/') + v.repr));
		const QVector<QPair<QString, QString>> info {
			{QStringLiteral("manufacturer"), manufacturer},
			{QStringLiteral("manufacturer_reference"), art.value(QStringLiteral("Order number"))},
			{QStringLiteral("designation"), type},
			{QStringLiteral("description"), descr},
			{QStringLiteral("label"), QLatin1Char('-') + codeLetters(reader.deviceTag())}};
		QDomDocument doc = EplanElementBuilder::build(
			data, {{QStringLiteral("de"), name}, {QStringLiteral("en"), name}}, info);
		const QString out = QDir(folder).filePath(safe(name) + QStringLiteral(".elmt"));
		const Stored stored = store(doc, out);
		if (stored == Stored::Failed) return false;
		if (stored == Stored::Written)
			m_messages << QStringLiteral("%1: %2 terminals").arg(name).arg(data.pins.size());
	}
	if (m_written.isEmpty() && m_kept.isEmpty()) {
		m_error = QStringLiteral("No element could be built from the package");
		return false;
	}
	return finish();
}

/**
	Import a single EPLAN DXF. Each black-box frame becomes an element of its
	own; a drawing without frames becomes one element.
	@param edz_path optional EPLAN part (.edz) with the part data
*/
bool EplanPartImporter::importDxf(const QString &dxf_path, const QString &edz_path,
				  const QString &manufacturer, const QString &category,
				  const QString &type, const QString &collection_dir)
{
	clear();

	QFile f(dxf_path);
	if (!f.open(QIODevice::ReadOnly)) {
		m_error = QStringLiteral("Cannot read %1").arg(dxf_path);
		return false;
	}
	EplanDxfReader reader;
	if (!reader.read(f.readAll())) {
		m_error = reader.errorString();
		return false;
	}

	QString name = type, type_nr = type, order_nr;
	EdzPart part;
	bool has_part = false;
	if (!edz_path.isEmpty()) {
		EdzArchive archive;
		if (!archive.extract(edz_path) || !(has_part = part.parse(archive.partXmlPath()))) {
			m_messages << QStringLiteral("No part data from %1").arg(QFileInfo(edz_path).fileName());
		} else {
			// EPLAN multilingual text: "??_??@{{text}};de_DE@text;..."
			static const QRegularExpression braces(QStringLiteral("\\{\\{(.*?)\\}\\}"));
			static const QRegularExpression lang(QStringLiteral("@([^;]*);"));
			const QString descr = part.description();
			auto m = braces.match(descr);
			if (!m.hasMatch()) m = lang.match(descr);
			name = m.hasMatch() ? m.captured(1) : descr;
			name.replace(QRegularExpression(QStringLiteral("\\s*,\\s*")), QStringLiteral(" "));
			name = name.remove(QStringLiteral("SIMATIC ")).trimmed();
			if (name.isEmpty()) name = type;
			if (!part.typeNumber().isEmpty()) type_nr = part.typeNumber();
			order_nr = part.orderNumber();
		}
	}
	const QString device = reader.deviceTag();
	const QVector<QPair<QString, QString>> info {
		{QStringLiteral("manufacturer"), manufacturer},
		{QStringLiteral("manufacturer_reference"), order_nr.isEmpty() ? type : order_nr},
		{QStringLiteral("designation"), type_nr},
		{QStringLiteral("description"), name},
		{QStringLiteral("label"), QLatin1Char('-') + (device.isEmpty() ? QStringLiteral("KF")
									       : codeLetters(device))}};

	QString folder;
	if (!ensureDirs(collection_dir, {manufacturer, category, type}, &folder, &m_error)) return false;

	auto write = [&](const EplanSymbolData &data, const QString &element_name) {
		QDomDocument doc = EplanElementBuilder::build(
			data, {{QStringLiteral("de"), element_name}, {QStringLiteral("en"), element_name}}, info);
		const QString out = QDir(folder).filePath(safe(element_name) + QStringLiteral(".elmt"));
		const Stored stored = store(doc, out);
		if (stored == Stored::Failed) return false;
		if (stored == Stored::Written)
			m_messages << QStringLiteral("%1: %2 terminals").arg(element_name).arg(data.pins.size());
		return true;
	};

	QVector<QRectF> boxes = reader.boxes();
	if (boxes.isEmpty()) {
		EplanSymbolData data;
		data.lines = reader.lines();
		data.circles = reader.circles();
		data.texts = withoutXrefs(reader.texts());
		data.pins = reader.pins();
		data.fields = reader.fields();
		// without connection points the drawing is still of use as a graphic
		// (overview, panel layout)
		if (data.pins.isEmpty() && data.lines.isEmpty()) {
			m_error = QStringLiteral("The DXF holds neither EPLAN connection points nor a drawing");
			return false;
		}
		sortPins(data.pins);
		// a drawing without connection points is a view of the device (a
		// manufacturer's front view), named like the panel layout of an
		// EPLAN macro
		return write(data, data.pins.isEmpty() ? QStringLiteral("%1 Panel layout").arg(type)
							 : partName(type, data.pins));
	}

	std::sort(boxes.begin(), boxes.end(), [](const QRectF &a, const QRectF &b) {
		if (a.left() != b.left()) return a.left() < b.left();
		if (a.top() != b.top()) return a.top() < b.top();
		if (a.right() != b.right()) return a.right() < b.right();
		return a.bottom() < b.bottom();
	});
	for (const QRectF &bx : std::as_const(boxes)) {
		// QRectF top() is the smaller y, here the lower edge (y up)
		const double x0 = bx.left(), y0 = bx.top(), x1 = bx.right(), y1 = bx.bottom();
		auto inside = [&](const QPointF &p) {
			return x0 - 0.5 <= p.x() && p.x() <= x1 + 0.5 && y0 - 0.5 <= p.y() && p.y() <= y1 + 0.5;
		};
		EplanSymbolData data;
		for (const EplanDxfPin &p : reader.pins()) {
			if (inside(p.pos)) data.pins << p;
		}
		if (data.pins.isEmpty()) continue;
		std::stable_sort(data.pins.begin(), data.pins.end(),
				 [](const EplanDxfPin &a, const EplanDxfPin &b) { return a.pos.y() > b.pos.y(); });
		// shorten the frame below the last terminal
		double lowest = data.pins.first().pos.y();
		for (const EplanDxfPin &p : std::as_const(data.pins)) lowest = qMin(lowest, p.pos.y());
		const double bottom = qMax(lowest - 8, y0);
		for (const QLineF &l : reader.modelLines()) {
			if (!inside(l.p1()) || !inside(l.p2())) continue;
			const double lx0 = qMin(l.x1(), l.x2()), lx1 = qMax(l.x1(), l.x2());
			const double ly0 = qMin(l.y1(), l.y2());
			// the old frame goes: it is drawn anew below
			if (ly0 >= bottom - 0.5 && !(lx0 > x1 - 0.5 || lx1 < x0 + 0.5 || ly0 > y1 - 0.5))
				data.lines << l;
		}
		data.lines << QLineF(x0, y1, x1, y1) << QLineF(x1, y1, x1, bottom)
			   << QLineF(x1, bottom, x0, bottom) << QLineF(x0, bottom, x0, y1);
		for (const auto &c : reader.modelCircles()) {
			if (inside(c.first) && c.first.y() >= bottom) data.circles << c;
		}
		for (const EplanDxfText &t : reader.texts()) {
			if (!t.tag.isEmpty() || !inside(t.insert) || t.insert.y() < bottom) continue;
			EplanDxfText plain;  // left on its insertion point, as the frame is redrawn
			plain.text = t.text;
			plain.anchor = t.insert;
			data.texts << plain;
		}
		if (!write(data, partName(type, data.pins))) return false;
	}
	if (m_written.isEmpty() && m_kept.isEmpty()) {
		m_error = QStringLiteral("No black-box part of the DXF has connection points");
		return false;
	}
	return finish();
}

/**
	Import an AutoCAD DWG drawing as an element, e.g. the "Download DWG" of
	WSCAD Universe: the block inserted in model space with its connection
	stubs as terminals. A drawing without terminals (a panel layout view)
	is still imported as a graphic.
	@param wspak_path optional WSCAD Universe package (.wspak) with the part
	data; manufacturer and type are taken from it where not given
*/
bool EplanPartImporter::importDwg(const QString &dwg_path, const QString &wspak_path,
				  const QString &manufacturer, const QString &category,
				  const QString &type, const QString &collection_dir)
{
	clear();

	QFile f(dwg_path);
	if (!f.open(QIODevice::ReadOnly)) {
		m_error = QStringLiteral("Cannot read %1").arg(dwg_path);
		return false;
	}
	DwgReader reader;
	if (!reader.read(f.readAll())) {
		m_error = reader.errorString();
		return false;
	}
	const QMap<QString, QString> part = wspak_path.isEmpty() ? QMap<QString, QString>() : wspakInfo(wspak_path);
	if (!wspak_path.isEmpty() && part.isEmpty())
		m_messages << QStringLiteral("No part data from %1").arg(QFileInfo(wspak_path).fileName());
	const QString maker = manufacturer.isEmpty() ? part.value(QStringLiteral("manufacturer")) : manufacturer;
	const QString type_nr = type.isEmpty() ? part.value(QStringLiteral("type")) : type;
	if (maker.isEmpty() || type_nr.isEmpty()) {
		m_error = QStringLiteral("A DWG needs the manufacturer and the type (or a .wspak with the part data)");
		return false;
	}

	EplanSymbolData data;
	data.pins = reader.pins();
	// a schematic: WSCAD draws on a 5 mm grid, 5 mm -> 10 px keeps the terminals on the QET grid;
	// a layout view (no terminals): the scale of the EPLAN layout macros, so devices side by side
	// in a panel layout are true to each other in size
	data.scale = data.pins.isEmpty() ? 2.5 : 2.0;
	data.pin_fields = false;                          // the drawing names each terminal's function itself
	data.lines = reader.lines();
	data.circles = reader.circles();
	// the device tag ("-K") is the element label, placeholders ("...") stay empty
	QString letters = QStringLiteral("A");
	static const QRegularExpression tag(QStringLiteral("^-([A-Z]{1,3})$"));
	for (const EplanDxfText &t : reader.texts()) {
		const QString s = t.text.trimmed();
		const auto m = tag.match(s);
		if (m.hasMatch()) { letters = m.captured(1); continue; }
		if (s == QLatin1String("...") || s == QStringLiteral("…")) continue;
		data.texts << t;
	}
	const QString kind = data.pins.isEmpty() ? QStringLiteral("Panel layout") : QStringLiteral("Schematic");
	const QString element_name = type_nr + QLatin1Char(' ') + kind;
	const QString description = part.value(QStringLiteral("description"), reader.blockName());
	const QVector<QPair<QString, QString>> info {
		{QStringLiteral("manufacturer"), maker},
		{QStringLiteral("manufacturer_reference"), part.value(QStringLiteral("order"))},
		{QStringLiteral("designation"), type_nr},
		{QStringLiteral("description"), description},
		{QStringLiteral("label"), QLatin1Char('-') + letters}};
	data.source = QStringLiteral("DWG %1 (%2)").arg(QFileInfo(dwg_path).fileName(), reader.version());

	QString folder;
	if (!ensureDirs(collection_dir, {maker, category, type_nr}, &folder, &m_error)) return false;
	QDomDocument doc = EplanElementBuilder::build(
		data, {{QStringLiteral("de"), element_name}, {QStringLiteral("en"), element_name}}, info);
	const QString out = QDir(folder).filePath(safe(element_name) + QStringLiteral(".elmt"));
	const Stored stored = store(doc, out);
	if (stored == Stored::Failed) return false;
	if (stored == Stored::Written)
		m_messages << QStringLiteral("%1: %2 terminals").arg(element_name).arg(data.pins.size());
	return finish();
}

/// part data of a WSCAD Universe package, for the dialog
QMap<QString, QString> EplanPartImporter::wspakPartInfo(const QString &path)
{
	return wspakInfo(path);
}

bool EplanPartImporter::importEdz(const QString &edz_path, const QString &manufacturer,
				  const QString &category, const QString &type,
				  const QString &collection_dir)
{
	clear();
	QString folder;
	if (!ensureDirs(collection_dir, {manufacturer, category, type}, &folder, &m_error)) return false;
	EdzImporter importer;
	QDomDocument generic;
	QString generic_name;
	if (!importer.build(edz_path, &generic, &generic_name)) {
		m_error = importer.errorString();
		return false;
	}
	if (store(generic, QDir(folder).filePath(generic_name), 0) == Stored::Failed) return false;

	// The EDZ may carry the manufacturer's EPLAN macro: its schematic is the
	// symbol the manufacturer draws, next to the generic one built from the
	// part data above
	EdzArchive archive;
	if (!archive.extract(edz_path)) return finish();
	EdzPart part;
	const bool has_part = part.parse(archive.partXmlPath());
	QStringList emas;
	for (QDirIterator it(archive.extractedDir(), {QStringLiteral("*.ema")}, QDir::Files,
			     QDirIterator::Subdirectories);
	     it.hasNext();)
		emas << it.next();
	// 3D macros first: where a macro of the same name without "_3D" exists,
	// its schematic is written last and stays
	std::sort(emas.begin(), emas.end(), [](const QString &a, const QString &b) {
		const bool a3 = a.contains(QStringLiteral("_3D"), Qt::CaseInsensitive);
		const bool b3 = b.contains(QStringLiteral("_3D"), Qt::CaseInsensitive);
		return a3 != b3 ? a3 : a < b;
	});
	// An EDZ may hold the macros of accessories as well (the base unit of an
	// ET 200SP module, 3D macros with a schematic of their own): only the
	// macro of the part itself is "<type> Schematic", the others are named
	// after their file, so that they do not overwrite each other.
	auto key = [](const QString &s) {
		return s.toUpper().remove(QRegularExpression(QStringLiteral("[^A-Z0-9]")));
	};
	auto own = [&](const QString &ema) {
		const QString base = QFileInfo(ema).completeBaseName()
					     .remove(QRegularExpression(QStringLiteral("_[23]D_?$"),
									QRegularExpression::CaseInsensitiveOption));
		// "3RM10xx-1xx04": a lower-case x stands for any character
		QString wild;
		for (const QChar c : base)
			if (c == QLatin1Char('x')) wild += QLatin1Char('.');
			else if (c.isLetterOrNumber()) wild += QRegularExpression::escape(QString(c.toUpper()));
		const QRegularExpression match(QStringLiteral("^") + wild);
		for (const QString &id : {type, has_part ? part.orderNumber() : QString(),
					  has_part ? part.typeNumber() : QString()})
			if (!key(id).isEmpty() && (match.match(key(id)).hasMatch() || key(base).startsWith(key(id))))
				return true;
		return false;
	};
	int schematics = 0;
	for (const QString &ema : std::as_const(emas))
		if (EplanMacroReader::variantCount(ema) > 0) ++schematics;
	for (const QString &ema : std::as_const(emas)) {
		const QString owner = schematics <= 1 || own(ema)
			? type
			: QFileInfo(ema).completeBaseName().remove(
				  QRegularExpression(QStringLiteral("_[23]D_?$"), QRegularExpression::CaseInsensitiveOption));
		// the 2D panel layout (front view) may sit in its own macro
		// ("<type>_2D.ema") or next to the schematic in the same one
		EplanMacroReader layout;
		const bool has_layout = layout.readLayout(ema);
		{
			if (has_layout) {
				const EplanSymbolData data = layout.symbol();
				const QString element_name = QStringLiteral("%1 Panel layout").arg(owner);
				const QVector<QPair<QString, QString>> info {
					{QStringLiteral("manufacturer"), manufacturer},
					{QStringLiteral("manufacturer_reference"), has_part ? part.orderNumber() : type},
					{QStringLiteral("designation"), has_part && !part.typeNumber().isEmpty() ? part.typeNumber() : type},
					{QStringLiteral("description"), layout.description()}};
				QDomDocument doc = EplanElementBuilder::build(
					data, {{QStringLiteral("de"), element_name}, {QStringLiteral("en"), element_name}}, info);
				const QString out = QDir(folder).filePath(safe(element_name) + QStringLiteral(".elmt"));
				const Stored stored = store(doc, out);
				if (stored == Stored::Failed) return false;
				if (stored == Stored::Written) {
					m_messages << QStringLiteral("%1: panel layout from the EPLAN macro (%2 lines)")
							      .arg(element_name).arg(data.lines.size());
					if (!layout.skipped().isEmpty())
						m_messages << QStringLiteral("not taken over: %1").arg(layout.skipped().join(QStringLiteral(", ")));
				}
			}
		}
		// every multi-line variant: A is the device as a whole and keeps the
		// plain name, B, C ... show parts of it (power supply, rack layout)
		const int variants = qMax(1, EplanMacroReader::variantCount(ema));
		for (int index = 0; index < variants; ++index) {
			EplanMacroReader reader;
			if (!reader.read(ema, index)) {
				if (!has_layout && index == 0)
					m_messages << QStringLiteral("%1: %2").arg(QFileInfo(ema).fileName(), reader.errorString());
				continue;
			}
			EplanSymbolData data = reader.symbol();
			sortPins(data.pins);
			const QString letters = reader.deviceLetters();
			const QVector<QPair<QString, QString>> info {
				{QStringLiteral("manufacturer"), manufacturer},
				{QStringLiteral("manufacturer_reference"), has_part ? part.orderNumber() : QString()},
				{QStringLiteral("designation"), has_part && !part.typeNumber().isEmpty() ? part.typeNumber() : type},
				{QStringLiteral("description"), reader.description()},
				{QStringLiteral("label"), QLatin1Char('-') + (letters.isEmpty() ? QStringLiteral("K") : letters)}};
			QString element_name = QStringLiteral("%1 Schematic").arg(owner);
			if (index > 0) {
				element_name += QLatin1Char(' ') + (index < 26 ? QString(QChar('A' + index)) : QString::number(index + 1));
				QString what = reader.variantName().simplified();
				if (what.size() > 40) what = what.left(39) + QChar(0x2026);
				if (!what.isEmpty() && what != reader.description()) element_name += QStringLiteral(" (%1)").arg(what);
			}
			QDomDocument doc = EplanElementBuilder::build(
				data, {{QStringLiteral("de"), element_name}, {QStringLiteral("en"), element_name}}, info);
			// the channels the macro defines (a single one is a channel macro
			// or a signalling contact, not an I/O module)
			const QVector<PlcChannels::Channel> channels =
				PlcChannels::fromMacro(data.pins, EplanElementBuilder::terminalNames(data.pins));
			if (channels.size() >= 2) PlcChannels::setChannels(doc, channels);
			const QString out = QDir(folder).filePath(safe(element_name) + QStringLiteral(".elmt"));
			const Stored stored = store(doc, out);
			if (stored == Stored::Failed) return false;
			if (stored == Stored::Kept) continue;
			m_from_macro.removeAll(out);
			m_messages << QStringLiteral("%1: %2 terminals from the EPLAN macro").arg(element_name).arg(data.pins.size());
			if (channels.size() >= 2) {
				m_from_macro << out;
				m_messages << QStringLiteral("%1: PLC module with %2 channels (%3..%4) from the EPLAN macro")
						      .arg(QFileInfo(out).fileName()).arg(channels.size())
						      .arg(channels.first().name, channels.last().name);
			}
			if (!reader.skipped().isEmpty())
				m_messages << QStringLiteral("not taken over: %1").arg(reader.skipped().join(QStringLiteral(", ")));
		}
	}
	return finish();
}

void EplanPartImporter::clear()
{
	m_written.clear();
	m_kept.clear();
	m_from_macro.clear();
	m_messages.clear();
	m_error.clear();
}

/**
	Writes @p doc to @p path with its fingerprint. An existing file is
	overwritten only while it is as the import wrote it, or when forced;
	its uuids, names and element information are kept (ElementFingerprint).
*/
EplanPartImporter::Stored EplanPartImporter::store(QDomDocument &doc, const QString &path, int indent)
{
	// a drawing the builder could not turn into an element: no empty file
	if (doc.documentElement().isNull()) {
		m_messages << QStringLiteral("nothing to draw, not written: %1").arg(QDir::toNativeSeparators(path));
		return Stored::Kept;
	}
	QFile existing(path);
	if (existing.exists()) {
		QDomDocument old;
		const bool parsed = existing.open(QIODevice::ReadOnly) && old.setContent(&existing);
		existing.close();
		const ElementFingerprint::State state = parsed ? ElementFingerprint::state(old) : ElementFingerprint::Missing;
		if (state != ElementFingerprint::Unedited && !m_force) {
			if (!m_kept.contains(path)) {
				m_kept << path;
				m_messages << (state == ElementFingerprint::Edited
					? QStringLiteral("edited, not overwritten: %1")
					: QStringLiteral("not from this import (no fingerprint), not overwritten: %1"))
						      .arg(QDir::toNativeSeparators(path));
			}
			return Stored::Kept;
		}
		if (parsed) ElementFingerprint::keepIdentity(doc, old);
	}
	ElementFingerprint::stamp(doc);
	if (!writeElement(doc, path, &m_error, indent)) return Stored::Failed;
	m_written << path;
	return Stored::Written;
}

/**
	The last step of every import: a PLC module among the written elements
	gets its channel table, and so do the other drawings of the module in
	its folder (see PlcChannels).
*/
bool EplanPartImporter::finish()
{
	m_messages << PlcChannels::applyToFolders(m_written, m_from_macro);
	return true;
}
