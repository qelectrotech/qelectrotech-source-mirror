/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "cli_export.h"

#include "bordertitleblock.h"
#include "conductornumexport.h"
#include "conductorproperties.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "diagramcontext.h"
#include "pdf_links.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "wiringlistexport.h"

// Private Qt PDF engine for drawHyperlink() — see pdf_links / projectprintwindow.
#include <private/qpdf_p.h>

#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QPageLayout>
#include <QPainter>
#include <QPdfWriter>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QSvgGenerator>
#include <QTextStream>
#include <QTransform>

namespace {

QTextStream out(stdout);
QTextStream err(stderr);

/// All CLI option flags, mapped to a short format name.
const QHash<QString, QString> &exportFlags()
{
	static const QHash<QString, QString> flags {
		{"--export-pdf", "pdf"},
		{"--export-png", "png"},
		{"--export-svg", "svg"},
		{"--export-cables", "cables"},
		{"--export-wires", "wires"},
		{"--export-bom", "bom"},
		{"--export-nets", "nets"},
		{"--export-links", "links"},
		{"--info", "info"},
		{"--check-elements", "check"},
		{"--resave", "resave"},
	};
	return flags;
}

/// Device tag of an element ("K1", "Q55"), falling back to its name.
QString elementLabel(Element *element)
{
	const QString label = element->elementInformations()["label"].toString();
	return label.isEmpty() ? element->name() : label;
}

/// Pixel rect of a diagram's border + title block (the printable page area).
QRect diagramRect(Diagram *diagram)
{
	QRectF r = diagram->border_and_titleblock.borderAndTitleBlockRect();
	r.adjust(0, 0, 1, 1); // include the 1px border line
	return r.toAlignedRect();
}

/// A filesystem-safe per-diagram file stem: "01_Title".
QString diagramStem(Diagram *diagram, int index)
{
	QString title = diagram->title();
	title.replace(QRegularExpression("[^\\w \\-]"), "_");
	title = title.simplified();
	if (title.isEmpty())
		title = "diagram";
	return QStringLiteral("%1_%2")
		.arg(index, 2, 10, QChar('0'))
		.arg(title);
}

/// Render @p diagram into @p painter, fitting @p target to the page rect.
void renderDiagram(Diagram *diagram, QPainter &painter, const QRectF &target)
{
	const QRect source = diagramRect(diagram);
	// Export without the editor grid: drawBackground() only paints it when
	// draw_grid_ is set (default true), so toggle it off around the render
	// and restore it afterwards.
	const bool was_drawing_grid = diagram->displayGrid();
	diagram->setDisplayGrid(false);
	diagram->render(&painter, target, source, Qt::KeepAspectRatio);
	diagram->setDisplayGrid(was_drawing_grid);
}

int exportPdf(QETProject &project, const QString &output)
{
	const QList<Diagram *> diagrams = project.diagrams();
	if (diagrams.isEmpty()) {
		err << "No diagrams to export.\n";
		return 1;
	}

	// Page numbers (1-based) for cross-reference hyperlink targets: each
	// diagram is exactly one page in the CLI export (no tiling).
	QMap<Diagram *, int> pageMap;
	for (int i = 0; i < diagrams.size(); ++i)
		pageMap.insert(diagrams.at(i), i + 1);

	QPdfWriter writer(output);
	writer.setCreator("QElectroTech");
	writer.setResolution(96);

	QPainter painter;
	bool first = true;
	for (Diagram *diagram : diagrams) {
		const QRect r = diagramRect(diagram);
		// Match the page to the diagram (in points: 1px @ 96dpi = 0.75pt).
		const QPageSize page(QSizeF(r.width() * 72.0 / 96.0,
									r.height() * 72.0 / 96.0),
							 QPageSize::Point);
		writer.setPageSize(page);
		writer.setPageMargins(QMarginsF(0, 0, 0, 0));

		if (first) {
			if (!painter.begin(&writer)) {
				err << "Cannot open '" << output << "' for writing.\n";
				return 1;
			}
			first = false;
		} else {
			writer.newPage();
		}
		const QRectF target(0, 0,
							writer.width(), writer.height());
		renderDiagram(diagram, painter, target);

		// Inject clickable cross-reference / folio-report hyperlinks for this
		// page.  The geometry is rebuilt from the QPdfWriter (not a QPrinter):
		// render() anchors the diagram top-left with KeepAspectRatio, and the
		// page is sized to the diagram so the scale is ~1.
		if (auto *engine = dynamic_cast<QPdfEngine *>(painter.paintEngine())) {
			const QRectF source(r);
			const qreal s = qMin(target.width()  / source.width(),
								 target.height() / source.height());
			QTransform fit;
			fit.translate(target.x(), target.y());
			fit.scale(s, s);
			fit.translate(-source.x(), -source.y());

			// Device pixels -> PDF points, replicating the engine's page matrix
			// (72/resolution scale + Y flip; zero margins -> no paint offset).
			const qreal pt_scale = 72.0 / writer.resolution();
			const qreal fullH_pt = writer.pageLayout().fullRectPoints().height();
			const bool  fullPageMode =
				(writer.pageLayout().mode() == QPageLayout::FullPageMode);
			const QRect paintPx =
				writer.pageLayout().paintRectPixels(writer.resolution());

			PdfLinks::PageGeometry geom;
			geom.sceneToDevice = fit;
			geom.target        = target;
			geom.pageBounds    = QRectF(0, 0, target.width(), target.height());
			geom.devToPdf = [=](const QPointF &d) -> QPointF {
				qreal dx = d.x(), dy = d.y();
				if (!fullPageMode) { dx += paintPx.left(); dy += paintPx.top(); }
				return QPointF(pt_scale * dx, fullH_pt - pt_scale * dy);
			};
			geom.sourceRectOf = [](Diagram *dg) {
				return QRectF(diagramRect(dg));
			};
			PdfLinks::injectCrossRefLinks(engine, diagram, geom, pageMap, output);
		}
	}
	painter.end();

	// Rewrite the URI link annotations into native internal GoTo actions, so
	// the cross-references jump inside the document in any PDF viewer.
	PdfLinks::convertUriToGoTo(output);

	out << "Exported " << diagrams.size() << " page(s) -> " << output << "\n";
	return 0;
}

int exportImages(QETProject &project, const QString &format,
				 const QString &out_dir)
{
	const QList<Diagram *> diagrams = project.diagrams();
	if (diagrams.isEmpty()) {
		err << "No diagrams to export.\n";
		return 1;
	}
	QDir().mkpath(out_dir);

	int index = 0;
	for (Diagram *diagram : diagrams) {
		++index;
		const QRect r = diagramRect(diagram);
		const QString path = QDir(out_dir).filePath(
			diagramStem(diagram, index) + "." + format);

		if (format == "svg") {
			QSvgGenerator gen;
			gen.setFileName(path);
			gen.setSize(r.size());
			gen.setViewBox(QRect(0, 0, r.width(), r.height()));
			gen.setTitle(diagram->title());
			QPainter painter(&gen);
			renderDiagram(diagram, painter, QRectF(QPointF(0, 0), r.size()));
			painter.end();
		} else { // png
			QImage image(r.size(), QImage::Format_ARGB32);
			image.fill(Qt::white);
			QPainter painter(&image);
			painter.setRenderHint(QPainter::Antialiasing, true);
			renderDiagram(diagram, painter, QRectF(QPointF(0, 0), r.size()));
			painter.end();
			if (!image.save(path)) {
				err << "Failed to write '" << path << "'.\n";
				return 1;
			}
		}
		out << "  " << path << "\n";
	}
	out << "Exported " << diagrams.size() << " diagram(s) -> " << out_dir << "\n";
	return 0;
}

int exportCsv(QETProject &project, const QString &format, const QString &output)
{
	QString csv;
	if (format == "cables") {
		WiringListExport wle(&project, nullptr);
		csv = wle.toCsvString();
	} else { // wires
		ConductorNumExport cne(&project, nullptr);
		csv = cne.wiresNum();
	}
	if (csv.isEmpty()) {
		err << "Nothing to export (empty list).\n";
		return 1;
	}

	QFile file(output);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		err << "Cannot open '" << output << "' for writing.\n";
		return 1;
	}
	QTextStream fout(&file);
	fout << csv;
	file.close();
	out << "Exported " << format << " list -> " << output << "\n";
	return 0;
}

/// Quote a field for CSV output (RFC-4180 style, ';' delimiter).
QString csvField(const QString &value)
{
	if (value.contains(';') || value.contains('"')
		|| value.contains('\n') || value.contains('\r')) {
		QString v = value;
		v.replace('"', "\"\"");
		return '"' % v % '"';
	}
	return value;
}

/// Bill of materials: one row per element, key component-data fields.
/// Pulls from QET's own project database (the same source as the GUI BOM
/// export), so the output matches what the editor produces.
int exportBom(QETProject &project, const QString &output)
{
	// The project database is built lazily; force a (re)build before querying.
	project.dataBase()->updateDB();

	static const QStringList columns {
		"label", "designation", "manufacturer", "manufacturer_reference",
		"quantity", "location", "function", "title", "folio"
	};

	QSqlQuery query = project.dataBase()->newQuery(
		"SELECT " % columns.join(", ") %
		" FROM element_nomenclature_view ORDER BY label");
	if (!query.exec()) {
		err << "BOM query failed: " << query.lastError().text() << "\n";
		return 1;
	}

	QString csv = columns.join(";") % "\n";
	int rows = 0;
	while (query.next()) {
		QStringList values;
		for (int i = 0; i < columns.size(); ++i)
			values << csvField(query.value(i).toString());
		csv += values.join(";") % "\n";
		++rows;
	}

	QFile file(output);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		err << "Cannot open '" << output << "' for writing.\n";
		return 1;
	}
	QTextStream fout(&file);
	fout << csv;
	file.close();
	out << "Exported " << rows << " component(s) -> " << output << "\n";
	return 0;
}

/// Count terminals on @p element that no conductor connects to.
int freeTerminals(Element *element)
{
	int free = 0;
	const QList<Terminal *> terminals = element->terminals();
	for (Terminal *t : terminals)
		if (t->conductorsCount() == 0)
			++free;
	return free;
}

/// Structural ground-truth dump of a project, as JSON, to stdout (or a file).
/// Uses QET's own loaded model, so it reports what the editor actually sees:
/// per-page element / conductor counts and unconnected terminals.
int exportInfo(QETProject &project, const QString &output)
{
	const QList<Diagram *> diagrams = project.diagrams();

	int total_elements = 0, total_conductors = 0, total_free = 0;
	QJsonArray pages;
	int index = 0;
	for (Diagram *diagram : diagrams) {
		++index;
		const QList<Element *> elements = diagram->elements();
		const int conductors = diagram->conductors().size();
		int page_free = 0;
		for (Element *e : elements)
			page_free += freeTerminals(e);

		const QRect r = diagramRect(diagram);
		QJsonObject page;
		page["index"]             = index;
		page["title"]             = diagram->title();
		page["folio"]             = QStringLiteral("%1 of %2")
									 .arg(index).arg(diagrams.size());
		page["width_px"]          = r.width();
		page["height_px"]         = r.height();
		page["elements"]          = elements.size();
		page["conductors"]        = conductors;
		page["free_terminals"]    = page_free;
		pages.append(page);

		total_elements   += elements.size();
		total_conductors += conductors;
		total_free       += page_free;
	}

	QJsonObject root;
	root["project"]    = project.title();
	root["diagrams"]   = diagrams.size();
	root["elements"]   = total_elements;
	root["conductors"] = total_conductors;
	root["free_terminals"] = total_free;
	root["pages"]      = pages;

	const QByteArray json =
		QJsonDocument(root).toJson(QJsonDocument::Indented);

	if (output.isEmpty()) {
		out << QString::fromUtf8(json);
	} else {
		QFile file(output);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			err << "Cannot open '" << output << "' for writing.\n";
			return 1;
		}
		file.write(json);
		file.close();
		out << "Wrote project info -> " << output << "\n";
	}
	return 0;
}

/// Validate one .elmt file against QET's element schema.
/// @return 0 = OK, 1 = warning (loads but suspicious), 2 = failure.
int checkOneElement(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		out << "FAIL  " << path << "  (cannot open)\n";
		return 2;
	}
	QDomDocument doc;
	QString error;
	int line = 0;
	if (!doc.setContent(&file, &error, &line)) {
		file.close();
		out << "FAIL  " << path << "  (XML error line "
			<< line << ": " << error << ")\n";
		return 2;
	}
	file.close();

	const QDomElement root = doc.documentElement();
	if (root.tagName() != "definition" || root.attribute("type") != "element") {
		out << "FAIL  " << path << "  (root is not <definition type=\"element\">)\n";
		return 2;
	}

	bool w_ok = false, h_ok = false;
	const double w = root.attribute("width").toDouble(&w_ok);
	const double h = root.attribute("height").toDouble(&h_ok);
	if (!w_ok || !h_ok || w == 0 || h == 0) {
		out << "FAIL  " << path << "  (missing/zero bounding box "
			<< root.attribute("width") << "x"
			<< root.attribute("height") << ")\n";
		return 2;
	}

	const int terminals = root.elementsByTagName("terminal").count();

	// Negative dimensions are malformed but QET still loads them; surface as a
	// warning rather than a failure so this agrees with QET's own loader.
	if (w < 0 || h < 0) {
		out << "WARN  " << path << "  (negative bounding box "
			<< w << "x" << h << ", " << terminals << " terminals)\n";
		return 1;
	}

	if (terminals == 0) {
		out << "WARN  " << path << "  (loads, but 0 terminals)\n";
		return 1;
	}

	out << "OK    " << path << "  (" << terminals << " terminals)\n";
	return 0;
}

/// Validate a single .elmt file or every .elmt under a directory.
int checkElements(const QString &path)
{
	QStringList files;
	const QFileInfo info(path);
	if (info.isDir()) {
		QDirIterator it(path, {"*.elmt"}, QDir::Files,
						QDirIterator::Subdirectories);
		while (it.hasNext())
			files << it.next();
		files.sort();
	} else if (info.isFile()) {
		files << path;
	} else {
		err << "Not found: " << path << "\n";
		return 2;
	}

	if (files.isEmpty()) {
		err << "No .elmt files found under: " << path << "\n";
		return 2;
	}

	int warnings = 0, failures = 0;
	for (const QString &f : files) {
		const int r = checkOneElement(f);
		if (r == 1) ++warnings;
		else if (r == 2) ++failures;
	}
	out << files.size() << " file(s), " << warnings
		<< " warning(s), " << failures << " failure(s)\n";
	return failures > 0 ? 1 : 0;
}

/// Map every element in the project to its 1-based folio (page) position.
QHash<Element *, int> folioIndex(QETProject &project)
{
	QHash<Element *, int> folio;
	int index = 0;
	const QList<Diagram *> diagrams = project.diagrams();
	for (Diagram *diagram : diagrams) {
		++index;
		const QList<Element *> elements = diagram->elements();
		for (Element *e : elements)
			folio.insert(e, index);
	}
	return folio;
}

/// Electrical nets: groups of terminals joined into one potential.
/// Walks QET's own potential graph, so each net is a connected component
/// of terminals across all folios. The ground truth for connectivity.
int exportNets(QETProject &project, const QString &output)
{
	const QHash<Element *, int> folio = folioIndex(project);

	QList<Conductor *> all_conductors;
	const QList<Diagram *> diagrams = project.diagrams();
	for (Diagram *diagram : diagrams)
		all_conductors << diagram->conductors();

	QSet<Conductor *> visited;
	QJsonArray nets;
	int net_no = 0;
	for (Conductor *c : all_conductors) {
		if (visited.contains(c))
			continue;

		// The whole potential this conductor belongs to. relatedPotential-
		// Conductors() also fills t_list with every terminal in the net
		// (following folio reports and terminal blocks too).
		QList<Terminal *> t_list;
		QSet<Conductor *> group = c->relatedPotentialConductors(true, &t_list);
		group.insert(c);
		for (Conductor *g : group)
			visited.insert(g);
		if (c->terminal1) t_list << c->terminal1;
		if (c->terminal2) t_list << c->terminal2;

		// Wire number: smallest non-empty conductor text (deterministic).
		QStringList wire_nos;
		for (Conductor *g : group)
			if (!g->properties().text.isEmpty())
				wire_nos << g->properties().text;
		wire_nos.sort();

		++net_no;
		QJsonArray terminals;
		QSet<Terminal *> seen;
		for (Terminal *t : t_list) {
			if (!t || seen.contains(t))
				continue;
			seen.insert(t);
			Element *pe = t->parentElement();
			QJsonObject to;
			to["element"]  = pe ? elementLabel(pe) : QString();
			to["terminal"] = t->name();
			to["folio"]    = pe ? folio.value(pe, 0) : 0;
			terminals.append(to);
		}
		QJsonObject net;
		net["net"]       = net_no;
		net["wire_no"]   = wire_nos.value(0);
		net["terminals"] = terminals;
		nets.append(net);
	}

	QJsonObject root;
	root["project"] = project.title();
	root["nets"]    = nets.size();
	root["list"]    = nets;

	QFile file(output);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		err << "Cannot open '" << output << "' for writing.\n";
		return 1;
	}
	file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	file.close();
	out << "Exported " << nets.size() << " net(s) -> " << output << "\n";
	return 0;
}

/// Cross-references: each linkable element (coil / contact / report) and the
/// elements it links to, flagging masters/slaves with no link as unresolved.
int exportLinks(QETProject &project, const QString &output)
{
	const QHash<Element *, int> folio = folioIndex(project);

	QString csv("element;link_type;linked_to;folio;status\n");
	int linkable = 0, unresolved = 0;

	const QList<Diagram *> diagrams = project.diagrams();
	for (Diagram *diagram : diagrams) {
		const QList<Element *> elements = diagram->elements();
		for (Element *e : elements) {
			if (e->linkType() == Element::Simple)
				continue;
			++linkable;

			const QList<Element *> linked = e->linkedElements();
			QStringList names;
			for (Element *le : linked)
				names << elementLabel(le) % "(f"
					   % QString::number(folio.value(le, 0)) % ")";

			QString status = "linked";
			if ((e->linkType() == Element::Master
				 || e->linkType() == Element::Slave)
				&& linked.isEmpty()) {
				status = "UNRESOLVED";
				++unresolved;
			}

			csv += csvField(elementLabel(e)) % ";"
				 % e->linkTypeToString() % ";"
				 % csvField(names.join(", ")) % ";"
				 % QString::number(folio.value(e, 0)) % ";"
				 % status % "\n";
		}
	}

	QFile file(output);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		err << "Cannot open '" << output << "' for writing.\n";
		return 1;
	}
	QTextStream fout(&file);
	fout << csv;
	file.close();
	out << "Exported " << linkable << " linkable element(s), "
		<< unresolved << " unresolved -> " << output << "\n";
	return 0;
}

/// Round-trip: load the project and write its XML back out, so an external
/// diff can reveal markup QET silently normalises (tolerated-but-invalid XML).
int resaveProject(QETProject &project, const QString &output)
{
	const QDomDocument doc = project.toXml();
	QFile file(output);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		err << "Cannot open '" << output << "' for writing.\n";
		return 1;
	}
	QTextStream fout(&file);
	fout << doc.toString(4);
	file.close();
	out << "Re-saved project -> " << output << "\n";
	return 0;
}

} // anonymous namespace

namespace CLIExport {

bool isExportRequest(const QStringList &args)
{
	for (const QString &a : args)
		if (exportFlags().contains(a))
			return true;
	return false;
}

int run(const QStringList &args)
{
	QString flag;
	QStringList rest;
	for (int i = 0; i < args.size(); ++i) {
		if (exportFlags().contains(args.at(i))) {
			flag = args.at(i);
			for (int j = i + 1; j < args.size(); ++j)
				rest << args.at(j);
			break;
		}
	}
	const QString format = exportFlags().value(flag);

	// --check-elements operates on an element file/directory, not a project.
	if (format == "check") {
		if (rest.isEmpty()) {
			err << "Usage: qelectrotech --check-elements "
				   "<element.elmt | directory>\n";
			return 2;
		}
		return checkElements(rest.at(0));
	}

	const QString input = rest.value(0);
	if (input.isEmpty()) {
		err << "Usage: qelectrotech " << flag << " <project.qet> <output>\n";
		return 2;
	}
	if (!QFileInfo::exists(input)) {
		err << "Project not found: " << input << "\n";
		return 2;
	}

	QETProject project(input);
	if (project.state() != QETProject::Ok) {
		err << "Failed to open project: " << input
			<< " (state " << project.state() << ")\n";
		return 1;
	}

	// --info writes JSON to stdout, or to an optional output file.
	if (format == "info")
		return exportInfo(project, rest.value(1));

	const QString output = rest.value(1);
	if (output.isEmpty()) {
		err << "Usage: qelectrotech " << flag
			<< " <project.qet> <output>\n";
		return 2;
	}
	if (format == "pdf")
		return exportPdf(project, output);
	if (format == "cables" || format == "wires")
		return exportCsv(project, format, output);
	if (format == "bom")
		return exportBom(project, output);
	if (format == "nets")
		return exportNets(project, output);
	if (format == "links")
		return exportLinks(project, output);
	if (format == "resave")
		return resaveProject(project, output);
	return exportImages(project, format, output);
}

} // namespace CLIExport
