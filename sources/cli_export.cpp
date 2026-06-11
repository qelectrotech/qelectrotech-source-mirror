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
#include "diagram.h"
#include "qetproject.h"
#include "wiringlistexport.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPdfWriter>
#include <QSvgGenerator>
#include <QTextStream>

namespace {

QTextStream out(stdout);
QTextStream err(stderr);

/// All export option flags, mapped to a short format name.
const QHash<QString, QString> &exportFlags()
{
	static const QHash<QString, QString> flags {
		{"--export-pdf", "pdf"},
		{"--export-png", "png"},
		{"--export-svg", "svg"},
		{"--export-cables", "cables"},
		{"--export-wires", "wires"},
	};
	return flags;
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
	}
	painter.end();
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
	QString flag, input, output;
	for (int i = 0; i < args.size(); ++i) {
		if (exportFlags().contains(args.at(i))) {
			flag = args.at(i);
			if (i + 2 < args.size()) {
				input = args.at(i + 1);
				output = args.at(i + 2);
			}
			break;
		}
	}

	if (input.isEmpty() || output.isEmpty()) {
		err << "Usage: qelectrotech " << flag
			<< " <project.qet> <output>\n";
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

	const QString format = exportFlags().value(flag);
	if (format == "pdf")
		return exportPdf(project, output);
	if (format == "cables" || format == "wires")
		return exportCsv(project, format, output);
	return exportImages(project, format, output);
}

} // namespace CLIExport
