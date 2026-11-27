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
#include "qetapptest.h"
#include "createdxf.h"
#include "diagram.h"
#include "dxfexport.h"
#include "exportproperties.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "qetproject.h"
#include "qetunits.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

/**
	Step 4 of docs/mm-scale.md: a project drawn to scale is exported to DXF
	in millimetres, 1 px = 0.25 mm, with $INSUNITS = 4. A project in pixels
	is fitted to the 4000 x 2700 sheet and carries no units, as before.
*/
class tst_dxfmm : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;

	using Scale = QETProject::Scale;
	using Pairs = QList<QPair<int, QString>>;

		/// The DXF file as (group code, value) pairs
	static Pairs pairs(const QString &path)
	{
		Pairs result;
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly))
			return result;
		const QStringList lines =
			QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
		for (int i = 0; i + 1 < lines.size(); i += 2)
			result << qMakePair(lines.at(i).trimmed().toInt(),
								lines.at(i + 1).trimmed());
		return result;
	}

		/// The value of the header variable @a name, as the pairs that
		/// follow it up to the next variable
	static Pairs header(const Pairs &p, const QString &name)
	{
		Pairs result;
		int i = 0;
		while (i < p.size() && !(p.at(i).first == 9 && p.at(i).second == name))
			++i;
		for (++i; i < p.size() && p.at(i).first != 9 && p.at(i).first != 0; ++i)
			result << p.at(i);
		return result;
	}

		/// The first LINE on layer QET_SHAPES: x1, y1, x2, y2
	static QVector<double> shapeLine(const Pairs &p)
	{
		for (int i = 0; i < p.size(); ++i) {
			if (p.at(i).first != 0 || p.at(i).second != QLatin1String("LINE"))
				continue;
			QVector<double> v(4, qQNaN());
			bool on_layer = false;
			for (++i; i < p.size() && p.at(i).first != 0; ++i) {
				const auto &g = p.at(i);
				if (g.first == 8)
					on_layer = g.second == DxfExport::Layer::Shapes;
				else if (g.first == 10) v[0] = g.second.toDouble();
				else if (g.first == 20) v[1] = g.second.toDouble();
				else if (g.first == 11) v[2] = g.second.toDouble();
				else if (g.first == 21) v[3] = g.second.toDouble();
			}
			if (on_layer)
				return v;
		}
		return {};
	}

		/// Export a folio with a line from (100, 200) to (300, 200) px
	Pairs exportFolio(Scale scale, QSize *size = nullptr, qreal module = 2.5)
	{
		QETProject project;
		project.setScale(scale);
		project.setModuleMm(module);
		Diagram *diagram = project.addNewDiagram();
		diagram->addItem(new QetShapeItem(QPointF(100, 200), QPointF(300, 200)));
		ExportProperties properties;
		properties.dxf_blocks = false;
		properties.dxf_attributes = false;
		const QSize folio = DxfExport::folioSize(diagram, properties);
		if (size)
			*size = folio;
		const QString path = m_dir.filePath(QStringLiteral("folio.dxf"));
		DxfExport::write(diagram, folio.width(), folio.height(), path, properties);
		return pairs(path);
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
	}

		//The height of an export in millimetres does not outlive it
	void sheetHeightIsResetAfterWrite()
	{
		const Pairs p = exportFolio(Scale::Mm);
		QVERIFY(!header(p, QStringLiteral("$INSUNITS")).isEmpty());
		QCOMPARE(Createdxf::sheetHeight, Createdxf::fitSheetHeight);
	}

		//Units 4 (millimetres) and the metric measurement, and the
		//drawing in mm: 100 px = 25 mm
	void millimetres()
	{
		QSize folio;
		const Pairs p = exportFolio(Scale::Mm, &folio);
		QCOMPARE(header(p, QStringLiteral("$INSUNITS")),
				 (Pairs{{70, QStringLiteral("4")}}));
		QCOMPARE(header(p, QStringLiteral("$MEASUREMENT")),
				 (Pairs{{70, QStringLiteral("1")}}));

		const QVector<double> line = shapeLine(p);
		QCOMPARE(line.size(), 4);
		QCOMPARE(line[0], 25.0);
		QCOMPARE(line[2], 75.0);
			//DXF is y up: from the bottom of the folio, 200 px = 50 mm down
		const double height_mm = QetUnits::pxToMm(folio.height() - 2 * Diagram::margin, QetUnits::defaultPxPerMm);
		QCOMPARE(line[1], height_mm - 50.0);
		QCOMPARE(line[3], line[1]);

			//The extents are the folio, in mm
		const Pairs extmax = header(p, QStringLiteral("$EXTMAX"));
		QCOMPARE(extmax.value(0).second.toDouble(),
				 QetUnits::pxToMm(folio.width() - 2 * Diagram::margin, QetUnits::defaultPxPerMm));
		QCOMPARE(extmax.value(1).second.toDouble(), height_mm);
	}

		//A px is M / 10 mm: x = 100 px is 100 * M / 10 mm
	void millimetresFollowTheModule_data()
	{
		QTest::addColumn<qreal>("module");
		QTest::newRow("M 1.8") << 1.8;
		QTest::newRow("M 2.5") << 2.5;
		QTest::newRow("M 5.0") << 5.0;
	}

	void millimetresFollowTheModule()
	{
		QFETCH(qreal, module);
		QSize folio;
		const Pairs p = exportFolio(Scale::Mm, &folio, module);
		const QVector<double> line = shapeLine(p);
		QCOMPARE(line.size(), 4);
		QVERIFY(qAbs(line[0] - 100 * module / 10) < 1e-6);
		QVERIFY(qAbs(line[2] - 300 * module / 10) < 1e-6);
		const double height_mm = (folio.height() - 2 * Diagram::margin) * module / 10;
		QVERIFY(qAbs(line[1] - (height_mm - 200 * module / 10)) < 1e-6);
		const Pairs extmax = header(p, QStringLiteral("$EXTMAX"));
		QVERIFY(qAbs(extmax.value(0).second.toDouble()
			     - (folio.width() - 2 * Diagram::margin) * module / 10) < 1e-6);
			//A project in pixels ignores the module
		const Pairs px = exportFolio(Scale::Px, nullptr, module);
		QVERIFY(header(px, QStringLiteral("$INSUNITS")).isEmpty());
	}

		//The view is centred on the folio, also when it is higher than wide
	void viewIsCentredOnPortraitFolio()
	{
		QETProject project;
		project.setScale(Scale::Mm);
		Diagram *diagram = project.addNewDiagram();
		diagram->border_and_titleblock.setRowsCount(30);
		diagram->border_and_titleblock.setColumnsCount(6);
		ExportProperties properties;
		properties.dxf_blocks = false;
		properties.dxf_attributes = false;
		const QSize folio = DxfExport::folioSize(diagram, properties);
		QVERIFY(folio.height() > folio.width());
		const QString path = m_dir.filePath(QStringLiteral("portrait.dxf"));
		DxfExport::write(diagram, folio.width(), folio.height(), path, properties);
		const Pairs p = pairs(path);

		const Pairs extmax = header(p, QStringLiteral("$EXTMAX"));
		const double w = extmax.value(0).second.toDouble();
		const double h = extmax.value(1).second.toDouble();
		double cx = qQNaN(), cy = qQNaN();
		bool in_vport = false;
		for (const auto &g : p) {
			if (g.first == 2 && g.second == QLatin1String("*ACTIVE")) in_vport = true;
			else if (in_vport && g.first == 12) cx = g.second.toDouble();
			else if (in_vport && g.first == 22) { cy = g.second.toDouble(); break; }
		}
		QVERIFY(qAbs(cx - w / 2) < 0.1);
		QVERIFY(qAbs(cy - h / 2) < 0.1);
	}

		//Pixels: no units, and the same fit to the sheet as before
	void pixelsAreFitted()
	{
		QSize folio;
		const Pairs p = exportFolio(Scale::Px, &folio);
		QVERIFY(header(p, QStringLiteral("$INSUNITS")).isEmpty());
		QVERIFY(header(p, QStringLiteral("$MEASUREMENT")).isEmpty());

		const double scale = qMin(
			Createdxf::sheetWidth / (folio.width() - 2 * Diagram::margin),
			Createdxf::fitSheetHeight / (folio.height() - 2 * Diagram::margin));
		const QVector<double> line = shapeLine(p);
		QCOMPARE(line.size(), 4);
			//The file keeps six significant digits
		QVERIFY(qAbs(line[0] - 100 * scale) < 0.01);
		QVERIFY(qAbs(line[1] - (Createdxf::fitSheetHeight - 200 * scale)) < 0.01);
		QCOMPARE(header(p, QStringLiteral("$EXTMAX")).value(0).second.toDouble(), 4000.0);
	}

		//An mm export does not leave the next pixel export shifted
	void backToPixels()
	{
		exportFolio(Scale::Mm);
		const Pairs p = exportFolio(Scale::Px);
		QCOMPARE(Createdxf::sheetHeight, Createdxf::fitSheetHeight);
		QVERIFY(header(p, QStringLiteral("$INSUNITS")).isEmpty());
	}
};

QET_APP_TEST_MAIN(tst_dxfmm)
#include "tst_dxfmm.moc"
