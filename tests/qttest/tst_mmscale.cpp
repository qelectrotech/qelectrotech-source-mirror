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
#include "dataBase/projectdatabase.h"
#include "qetapptest.h"
#include "qetproject.h"
#include "qetresult.h"
#include "qetunits.h"
#include "ui/configpage/projectconfigpages.h"
#include "ui/configpage/configpages.h"
#include "ui/borderpropertieswidget.h"
#include "ui/diagramcontextwidget.h"
#include "ui/projectpropertiesdialog.h"

#include <QComboBox>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QtTest>

/**
	Step 1 of docs/mm-scale.md: the QetUnits conversions and the project's
	drawing scale, As before (pixels) or millimetres. Pixels is the default
	and no data at all, so every existing project stays as it is; a value
	this QElectroTech does not know is kept as typed and read as Px. It
	changes nothing by itself.
*/
class tst_mmscale : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;

	using Scale = QETProject::Scale;

	static QString savedXml(QETProject &project)
	{
		return project.toXml().toString();
	}

private slots:
	/// The paper sizes of the new folios follow the drawing scale and
	/// the module M picked on the main page of the same dialog, not the
	/// ones the project had when the dialog was opened
	void newFolioPaperFollowsTheScaleOfTheDialog()
	{
		QETProject project;
		project.addNewDiagram();
		QWidget parent;
		ProjectPropertiesDialog dialog(&project, &parent);
		auto *main_page = parent.findChild<ProjectMainConfigPage *>();
		auto *folio_page = parent.findChild<NewDiagramPage *>();
		QVERIFY(main_page && folio_page);
		auto *scale = main_page->findChild<QComboBox *>(QStringLiteral("drawingScale"));
		auto *module = main_page->findChild<QComboBox *>(QStringLiteral("drawingModule"));
		auto *paper = folio_page->findChild<QComboBox *>(QStringLiteral("m_paper_cb"));
		auto *orient = folio_page->findChild<QComboBox *>(QStringLiteral("m_orientation_cb"));
		auto *column_width = folio_page->findChild<QSpinBox *>(QStringLiteral("m_columns_width_sp"));
		auto *columns = folio_page->findChild<QSpinBox *>(QStringLiteral("m_colums_count_sp"));
		auto *border = folio_page->findChild<BorderPropertiesWidget *>();
		QVERIFY(scale && module && paper && orient && column_width && columns && border);

		auto paperWidth = [&]() {
			paper->setCurrentIndex(paper->findText(QStringLiteral("A3")));
			orient->setCurrentIndex(0);
			emit paper->activated(paper->currentIndex());
			const BorderProperties &p = border->properties();
			return qRound(p.rows_header_width) + 1 + columns->value() * column_width->value();
		};
		QVERIFY(qAbs(paperWidth() - 1587) <= 3);

		scale->setCurrentIndex(scale->findData(QETProject::scaleName(Scale::Mm)));
		QVERIFY(scale->currentData().toString() == QETProject::scaleName(Scale::Mm));
		QVERIFY2(qAbs(paperWidth() - 1680) <= 3, qPrintable(QString::number(paperWidth())));

			//M = 5 mm is 2 px/mm: A3 is 840 px wide
		module->setCurrentIndex(module->findData(5.0));
		QVERIFY2(qAbs(paperWidth() - 840) <= 3, qPrintable(QString::number(paperWidth())));

		scale->setCurrentIndex(0);
		QVERIFY(qAbs(paperWidth() - 1587) <= 3);
	}

	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
	}

		/// scale(), moduleMm() and pxPerMm() are kept, not parsed at each
		/// call: they still follow every way the properties change
	void cachedScaleFollowsTheProperties()
	{
		QETProject project;
		project.addNewDiagram();
		QCOMPARE(project.scale(), QETProject::Scale::Px);
		QCOMPARE(project.moduleMm(), 2.5);
		QCOMPARE(project.pxPerMm(), 4.0);

		DiagramContext properties = project.projectProperties();
		properties.addValue(QStringLiteral("drawing_scale"), QStringLiteral("mm"));
		properties.addValue(QStringLiteral("drawing_module"), QStringLiteral("5.0"));
		project.setProjectProperties(properties);
		QCOMPARE(project.scale(), QETProject::Scale::Mm);
		QCOMPARE(project.moduleMm(), 5.0);
		QCOMPARE(project.pxPerMm(), 2.0);

			//Another way in: the properties no folio shows
		properties.addValue(QStringLiteral("drawing_module"), QStringLiteral("1.8"));
		project.setUnshownProjectProperties(properties);
		QCOMPARE(project.moduleMm(), 1.8);
		QVERIFY(qAbs(project.pxPerMm() - 10 / 1.8) < 1e-9);

			//A value no module has reads as 2.5, a scale not known as Px
		properties.addValue(QStringLiteral("drawing_module"), QStringLiteral("7"));
		properties.addValue(QStringLiteral("drawing_scale"), QStringLiteral("furlongs"));
		project.setProjectProperties(properties);
		QCOMPARE(project.scale(), QETProject::Scale::Px);
		QCOMPARE(project.moduleMm(), 2.5);

		project.setScale(QETProject::Scale::Mm);
		project.setModuleMm(3.5);
		QCOMPARE(project.scale(), QETProject::Scale::Mm);
		QCOMPARE(project.moduleMm(), 3.5);

			//And a project opened from its file
		const QString path = m_dir.filePath(QStringLiteral("cached.qet"));
		project.setFilePath(path);
		QVERIFY(project.write().isOk());
		QETProject loaded(path);
		QCOMPARE(loaded.state(), QETProject::Ok);
		QCOMPARE(loaded.scale(), QETProject::Scale::Mm);
		QCOMPARE(loaded.moduleMm(), 3.5);
		QVERIFY(qAbs(loaded.pxPerMm() - 10 / 3.5) < 1e-9);
	}

	void unitsConvert()
	{
		const qreal ppm = QetUnits::defaultPxPerMm;
		QCOMPARE(ppm, 4.0);
		QCOMPARE(QetUnits::mmToPx(2.5, ppm), 10.0);
		QCOMPARE(QetUnits::pxToMm(10.0, ppm), 2.5);
		QCOMPARE(QetUnits::defaultModuleMm, 2.5);
		QCOMPARE(QetUnits::mmToPx(QetUnits::defaultModuleMm, ppm), 10.0);
			//A3 landscape
		QCOMPARE(QetUnits::mmToPx(420.0, ppm), 1680.0);
		QCOMPARE(QetUnits::mmToPx(297.0, ppm), 1188.0);
		QCOMPARE(QetUnits::pxToMm(QetUnits::mmToPx(123.4, ppm), ppm), 123.4);
	}

		//10 px are M whatever M is
	void unitsFollowTheModule_data()
	{
		QTest::addColumn<qreal>("module");
		QTest::newRow("1.8") << 1.8;
		QTest::newRow("2.0") << 2.0;
		QTest::newRow("2.5") << 2.5;
		QTest::newRow("3.5") << 3.5;
		QTest::newRow("5.0") << 5.0;
	}

	void unitsFollowTheModule()
	{
		QFETCH(qreal, module);
		const qreal ppm = QetUnits::pxPerMmFor(module);
		QVERIFY(qFuzzyCompare(ppm, 10.0 / module));
		QVERIFY(qFuzzyCompare(QetUnits::pxToMm(10.0, ppm), module));
		QVERIFY(qFuzzyCompare(QetUnits::mmToPx(module, ppm), 10.0));
	}

		//2.5 mm is the default and no data, so every project and file
		//made before the module existed stays as it is
	void theModuleIsTwoAndAHalfAndNoData()
	{
		QETProject project;
		project.addNewDiagram();
		QCOMPARE(project.moduleMm(), 2.5);
		QVERIFY(project.moduleText().isEmpty());
		QCOMPARE(project.pxPerMm(), 4.0);
		QVERIFY(!project.projectProperties().contains(QStringLiteral("drawing_module")));
		const QString before = savedXml(project);
		project.setScale(Scale::Mm);
		project.setModuleMm(2.5);
		QVERIFY(!project.projectProperties().contains(QStringLiteral("drawing_module")));
		QVERIFY(!savedXml(project).contains(QLatin1String("drawing_module")));
		QVERIFY(savedXml(project) != before); //only the scale differs
		project.setScale(Scale::Px);
		QCOMPARE(savedXml(project), before);
	}

	void moduleNamesRoundTrip()
	{
		QCOMPARE(QETProject::modulesMm(), (QList<qreal>{1.8, 2.0, 2.5, 3.5, 5.0}));
		for (qreal module : QETProject::modulesMm())
			QCOMPARE(QETProject::moduleFromName(QETProject::moduleName(module)), module);
		QCOMPARE(QETProject::moduleName(2.5), QString());
		QCOMPARE(QETProject::moduleName(1.8), QStringLiteral("1.8"));
		QCOMPARE(QETProject::moduleName(5.0), QStringLiteral("5.0"));
		bool known = false;
		QCOMPARE(QETProject::moduleFromName(QStringLiteral(" 2 "), &known), 2.0);
		QVERIFY(known);
		QCOMPARE(QETProject::moduleFromName(QStringLiteral("3,5"), &known), 3.5);
		QVERIFY(known);
		QCOMPARE(QETProject::moduleFromName(QStringLiteral("4"), &known), 2.5);
		QVERIFY(!known);
		QCOMPARE(QETProject::moduleFromName(QStringLiteral("big"), &known), 2.5);
		QVERIFY(!known);
		QCOMPARE(QETProject::moduleFromName(QString(), &known), 2.5);
		QVERIFY(known);
	}

	void storedAsAProjectPropertyToo()
	{
		QETProject project;
		project.addNewDiagram();
		project.setScale(Scale::Mm);
		project.setModuleMm(1.8);
		QCOMPARE(project.moduleMm(), 1.8);
		QCOMPARE(project.pxPerMm(), 10.0 / 1.8);
		QCOMPARE(project.projectProperties().value(QStringLiteral("drawing_module")).toString(),
				 QStringLiteral("1.8"));
		QVERIFY(savedXml(project).contains(QLatin1String("drawing_module")));
		project.setModuleMm(2.5);
		QVERIFY(!project.projectProperties().contains(QStringLiteral("drawing_module")));
	}

	void moduleProjectKeepsIt_data()
	{
		QTest::addColumn<QString>("file");
		QTest::addColumn<QString>("stored");
		QTest::addColumn<qreal>("module");
		for (const QString &ext : {QStringLiteral("qet"), QStringLiteral("qetz"), QStringLiteral("qetdb")}) {
			QTest::newRow(qPrintable(ext + "-1.8")) << QStringLiteral("m18.") + ext << QStringLiteral("1.8") << 1.8;
			QTest::newRow(qPrintable(ext + "-5.0")) << QStringLiteral("m50.") + ext << QStringLiteral("5.0") << 5.0;
				//A value it does not know is kept as typed, read as 2.5
			QTest::newRow(qPrintable(ext + "-unknown")) << QStringLiteral("modd.") + ext << QStringLiteral("4.2") << 2.5;
		}
	}

	void moduleProjectKeepsIt()
	{
		QFETCH(QString, file);
		QFETCH(QString, stored);
		QFETCH(qreal, module);
		QETProject project;
		project.addNewDiagram();
		DiagramContext properties = project.projectProperties();
		properties.addValue(QStringLiteral("drawing_scale"), QStringLiteral("mm"));
		properties.addValue(QStringLiteral("drawing_module"), stored);
		project.setProjectProperties(properties);
		project.setFilePath(m_dir.filePath(file));
		const QETResult written = project.write();
		QVERIFY2(written.isOk(), qPrintable(written.errorMessage()));
		QETProject again(m_dir.filePath(file));
		QCOMPARE(again.moduleText(), stored);
		QCOMPARE(again.moduleMm(), module);
		QCOMPARE(again.scale(), Scale::Mm);
	}

		//Project properties > General: enabled only in millimetres, its own
		//list (not a variable), unknown values shown as typed and kept
	void theGeneralPageEditsTheModule()
	{
		QETProject project;
		project.addNewDiagram();
		ProjectMainConfigPage page(&project);
		auto *scale = page.findChild<QComboBox *>(QStringLiteral("drawingScale"));
		auto *module = page.findChild<QComboBox *>(QStringLiteral("drawingModule"));
		auto *variables = page.findChild<DiagramContextWidget *>();
		QVERIFY(scale && module && variables);
		QCOMPARE(module->count(), 5);
		QCOMPARE(module->currentText(), QStringLiteral("2.5 mm (recommended)"));
		QVERIFY(!module->isEnabled());

		scale->setCurrentIndex(scale->findData(QStringLiteral("mm")));
		QVERIFY(module->isEnabled());
		module->setCurrentIndex(module->findData(3.5));
		page.applyProjectConf();
		QCOMPARE(project.scale(), Scale::Mm);
		QCOMPARE(project.moduleMm(), 3.5);
		QVERIFY(!variables->context().contains(QStringLiteral("drawing_module")));

		scale->setCurrentIndex(scale->findData(QString()));
		QVERIFY(!module->isEnabled());
		scale->setCurrentIndex(scale->findData(QStringLiteral("mm")));
		module->setCurrentIndex(module->findData(2.5));
		page.applyProjectConf();
		QVERIFY(!project.projectProperties().contains(QStringLiteral("drawing_module")));

			//An unknown value shows as typed and stays
		DiagramContext properties = project.projectProperties();
		properties.addValue(QStringLiteral("drawing_module"), QStringLiteral("4.2"));
		project.setProjectProperties(properties);
		ProjectMainConfigPage other(&project);
		auto *list = other.findChild<QComboBox *>(QStringLiteral("drawingModule"));
		QVERIFY(list);
		QCOMPARE(list->currentText(), QStringLiteral("4.2"));
		QCOMPARE(list->count(), 6);
		other.applyProjectConf();
		QCOMPARE(project.moduleText(), QStringLiteral("4.2"));
		QCOMPARE(project.moduleMm(), 2.5);
	}

	void pixelsIsTheDefaultAndNoData()
	{
		QETProject project;
		project.addNewDiagram();
		QCOMPARE(project.scale(), Scale::Px);
		QVERIFY(project.scaleText().isEmpty());
		QVERIFY(!project.projectProperties().contains(QStringLiteral("drawing_scale")));
	}

	void namesRoundTrip()
	{
		for (Scale s : {Scale::Px, Scale::Mm})
			QCOMPARE(QETProject::scaleFromName(QETProject::scaleName(s)), s);
		bool known = false;
		QCOMPARE(QETProject::scaleFromName(QStringLiteral(" MM "), &known), Scale::Mm);
		QVERIFY(known);
		QCOMPARE(QETProject::scaleFromName(QStringLiteral("1:50"), &known), Scale::Px);
		QVERIFY(!known);
		QCOMPARE(QETProject::scaleFromName(QString(), &known), Scale::Px);
		QVERIFY(known);
	}

		//In the project's <properties>; Px takes it out again
	void storedAsAProjectProperty()
	{
		QETProject project;
		project.addNewDiagram();
		project.setScale(Scale::Mm);
		QCOMPARE(project.scale(), Scale::Mm);
		QCOMPARE(project.projectProperties().value(QStringLiteral("drawing_scale")).toString(),
				 QStringLiteral("mm"));
		QDomDocument document = project.toXml();
		const QDomElement properties = document.documentElement().firstChildElement(QStringLiteral("properties"));
		QVERIFY(!properties.isNull());
		QVERIFY(properties.text().contains(QLatin1String("mm")));
		project.setScale(Scale::Px);
		QVERIFY(!project.projectProperties().contains(QStringLiteral("drawing_scale")));
		QVERIFY(!savedXml(project).contains(QLatin1String("name=\"scale\"")));
	}

	void projectKeepsIt_data()
	{
		QTest::addColumn<QString>("file");
		QTest::addColumn<QString>("stored");
		QTest::addColumn<int>("scale");
		for (const QString &ext : {QStringLiteral("qet"), QStringLiteral("qetz"), QStringLiteral("qetdb")}) {
			QTest::newRow(qPrintable(ext + "-mm")) << QStringLiteral("mm.") + ext << QStringLiteral("mm") << int(Scale::Mm);
				//A value it does not know is kept as typed
			QTest::newRow(qPrintable(ext + "-unknown")) << QStringLiteral("odd.") + ext << QStringLiteral("1:50") << int(Scale::Px);
		}
	}

	void projectKeepsIt()
	{
		QFETCH(QString, file);
		QFETCH(QString, stored);
		QFETCH(int, scale);
		QETProject project;
		project.addNewDiagram();
		DiagramContext properties = project.projectProperties();
		properties.addValue(QStringLiteral("drawing_scale"), stored);
		project.setProjectProperties(properties);
		project.setFilePath(m_dir.filePath(file));
		const QETResult written = project.write();
		QVERIFY2(written.isOk(), qPrintable(written.errorMessage()));
		QETProject again(m_dir.filePath(file));
		QCOMPARE(again.scaleText(), stored);
		QCOMPARE(int(again.scale()), scale);
	}

		//Project properties > General: its own list, not a row of the
		//variables; a value it does not know shows as typed and is kept
	void theGeneralPageEditsIt()
	{
		QETProject project;
		project.addNewDiagram();
		DiagramContext properties = project.projectProperties();
		properties.addValue(QStringLiteral("customer"), QStringLiteral("ACME"));
		properties.addValue(QStringLiteral("drawing_scale"), QStringLiteral("1:50"));
		project.setProjectProperties(properties);

		ProjectMainConfigPage page(&project);
		auto *list = page.findChild<QComboBox *>(QStringLiteral("drawingScale"));
		auto *variables = page.findChild<DiagramContextWidget *>();
		QVERIFY(list && variables);
		QVERIFY(!variables->context().contains(QStringLiteral("drawing_scale")));
		QVERIFY(variables->context().contains(QStringLiteral("customer")));
		QCOMPARE(list->currentText(), QStringLiteral("1:50"));

			//Applied unchanged: kept as typed, the other variables too
		page.applyProjectConf();
		QCOMPARE(project.scaleText(), QStringLiteral("1:50"));
		QCOMPARE(project.projectProperties().value(QStringLiteral("customer")).toString(),
				 QStringLiteral("ACME"));

			//Millimetres chosen
		list->setCurrentIndex(list->findData(QStringLiteral("mm")));
		page.applyProjectConf();
		QCOMPARE(project.scale(), Scale::Mm);
		QCOMPARE(project.projectProperties().value(QStringLiteral("customer")).toString(),
				 QStringLiteral("ACME"));

			//As before: no property left
		list->setCurrentIndex(list->findData(QString()));
		page.applyProjectConf();
		QVERIFY(!project.projectProperties().contains(QStringLiteral("drawing_scale")));
		QCOMPARE(project.scale(), Scale::Px);
	}

		//A title block variable named "scale" (e.g. "1:1") is the user's
		//own: it is not the drawing scale and stays a variable
	void aScaleVariableStaysAVariable()
	{
		QETProject project;
		project.addNewDiagram();
		DiagramContext properties = project.projectProperties();
		properties.addValue(QStringLiteral("scale"), QStringLiteral("1:1"));
		project.setProjectProperties(properties);
		QCOMPARE(project.scale(), Scale::Px);
		QVERIFY(project.scaleText().isEmpty());

		ProjectMainConfigPage page(&project);
		auto *list = page.findChild<QComboBox *>(QStringLiteral("drawingScale"));
		auto *variables = page.findChild<DiagramContextWidget *>();
		QVERIFY(list && variables);
		QCOMPARE(variables->context().value(QStringLiteral("scale")).toString(),
				 QStringLiteral("1:1"));
		QCOMPARE(list->count(), 2);

		list->setCurrentIndex(list->findData(QStringLiteral("mm")));
		page.applyProjectConf();
		QCOMPARE(project.scale(), Scale::Mm);
		QCOMPARE(project.projectProperties().value(QStringLiteral("scale")).toString(),
				 QStringLiteral("1:1"));
	}
};

QET_APP_TEST_MAIN(tst_mmscale)
#include "tst_mmscale.moc"
