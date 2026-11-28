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
#include "epdzimport.h"
#include "epdzmanifest.h"

#include "../../diagram.h"
#include "../../qetgraphicsitem/diagramimageitem.h"
#include "../../qetproject.h"
#include "../../qetresult.h"
#include "../../titleblockproperties.h"
#include "../../undocommand/addgraphicsobjectcommand.h"
#include "../edz/edzsevenzip.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QSaveFile>
#include <QSvgRenderer>
#include <QTemporaryDir>

namespace {

bool writeText(const QString &path, const QString &text)
{
	QSaveFile f(path);
	if (!f.open(QIODevice::WriteOnly)) return false;
	f.write("\xEF\xBB\xBF");   // UTF-8 with BOM: spreadsheet programs then read it as UTF-8
	f.write(text.toUtf8());
	return f.commit();
}

QString row(const QStringList &cells)
{
	QStringList out;
	for (const QString &c : cells) out << EpdzManifest::csvCell(c);
	return out.join(QLatin1Char(';'));
}

} // namespace

EpdzImport::Result EpdzImport::run(const QString &epdz, const QString &target_qet)
{
	Result r;
	QTemporaryDir dir;
	if (!dir.isValid()) {
		r.error = QCoreApplication::translate("EpdzImport", "No temporary folder");
		return r;
	}
	QString error;
	if (!sevenZipExtract(epdz, dir.path(), error)) {
		r.error = QCoreApplication::translate("EpdzImport", "%1 cannot be unpacked: %2")
				.arg(QFileInfo(epdz).fileName(), error);
		return r;
	}
	return runUnpacked(dir.path(), QFileInfo(epdz).completeBaseName(), target_qet);
}

EpdzImport::Result EpdzImport::runUnpacked(const QString &folder, const QString &name,
										   const QString &target_qet)
{
	Result r;
	EpdzManifest::Manifest m;
	if (!EpdzManifest::read(folder, m, r.error)) return r;

		// the project: one folio per page, the page as a picture
	QETProject project;
	project.setTitle(m.project_name.isEmpty() ? name : m.project_name);
	for (const EpdzManifest::Page &p : std::as_const(m.pages)) {
		Diagram *d = project.addNewDiagram();
		if (!d) continue;
		TitleBlockProperties tb = d->border_and_titleblock.exportTitleBlock();
		tb.title = p.title;
		tb.folio = p.folio;
		tb.plant = p.plant;
		tb.locmach = p.location;
		d->border_and_titleblock.importTitleBlock(tb);
		++r.pages;
		if (p.svg.isEmpty()) {
			r.notes << QCoreApplication::translate("EpdzImport", "Page %1: no drawing in the archive").arg(p.name);
			continue;
		}
		QSvgRenderer svg(p.svg);
		if (!svg.isValid()) {
			r.notes << QCoreApplication::translate("EpdzImport", "Page %1: the drawing cannot be read").arg(p.name);
			continue;
		}
		const QRectF area = d->border_and_titleblock.insideBorderRect();
		QSizeF size = svg.viewBoxF().size();
		if (size.isEmpty()) size = svg.defaultSize();
		if (size.isEmpty()) continue;
			// drawn at three times the folio's resolution, so it stays sharp when zoomed
		const double fit = qMin(area.width() / size.width(), area.height() / size.height());
		const double k = 3.0;
		QImage image((size * fit * k).toSize(), QImage::Format_ARGB32_Premultiplied);
		image.fill(Qt::white);
		{
			QPainter painter(&image);
			painter.setRenderHint(QPainter::Antialiasing);
			svg.render(&painter, QRectF(QPointF(0, 0), size * fit * k));
		}
		auto *item = new DiagramImageItem(QPixmap::fromImage(image));
		d->undoStack().push(new AddGraphicsObjectCommand(item, d, area.topLeft()));
		item->setScaleFactorX(1.0 / k);
		item->setScaleFactorY(1.0 / k);
			// the picture scales about its centre: put its corner back on the folio's
		item->setPos(item->pos() + (area.topLeft() - item->sceneBoundingRect().topLeft()));
	}

		// the lists and the 3D beside the project
	const QFileInfo target(target_qet);
	const QString base = target.absoluteDir().filePath(target.completeBaseName());
	if (!m.devices.isEmpty()) {
		QStringList rows{row({QStringLiteral("Function"), QStringLiteral("Device tag (20001)"),
							  QStringLiteral("Article (20100)"), QStringLiteral("Designation (20193)")})};
		for (const EpdzManifest::Device &dv : std::as_const(m.devices))
			rows << row({dv.function, dv.designation, dv.article, dv.description});
		writeText(base + QStringLiteral("_devices.csv"), rows.join(QLatin1Char('\n')) + QLatin1Char('\n'));
		r.functions = int(m.devices.size());
	}
	if (!m.connections.isEmpty()) {
		QStringList rows{row({QStringLiteral("Source"), QStringLiteral("Target"), QStringLiteral("Potential"),
							  QStringLiteral("Colour"), QStringLiteral("Section"), QStringLiteral("Length")})};
		for (const EpdzManifest::Connection &c : std::as_const(m.connections))
			rows << row({c.source, c.target, c.potential, c.colour, c.section, c.length});
		writeText(base + QStringLiteral("_connections.csv"), rows.join(QLatin1Char('\n')) + QLatin1Char('\n'));
		r.connections = int(m.connections.size());
	}
	if (!m.spaces.isEmpty()) {
		const QString dir3d = base + QStringLiteral("_3D");
		QDir().mkpath(dir3d);
		for (const auto &s : std::as_const(m.spaces)) {
			if (s.second.isEmpty()) continue;
			const QString to = QDir(dir3d).filePath(QFileInfo(s.second).fileName());
			QFile::remove(to);
			if (QFile::copy(s.second, to)) ++r.spaces;
		}
	}

	project.setFilePath(target_qet);
	const QETResult written = project.write();
	if (!written.isOk()) {
		r.error = written.errorMessage();
		return r;
	}
	r.notes.prepend(QCoreApplication::translate("EpdzImport",
		"%1 page(s), %2 function(s), %3 connection(s), %4 3D installation space(s)")
						.arg(r.pages).arg(r.functions).arg(r.connections).arg(r.spaces));
	r.ok = true;
	return r;
}
