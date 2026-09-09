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
#include <QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include <QSqlError>
#include <algorithm>

#include "ElementsCollection/xmlelementcollection.h"
#include "dataBase/projectdatabase.h"
#include "devicebomexport.h"
#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "qetresult.h"
#include "ui/elementinfopartwidget.h"
#include "ui/elementinfowidget.h"

class SmartDeviceTest : public QObject
{
	Q_OBJECT
	QTemporaryDir m_settings;

	Element *addElement(QETProject &project, Diagram *diagram, const QString &tag,
						ElementData::Type type = ElementData::Simple)
	{
		QDomDocument doc;
		doc.setContent(QStringLiteral(
			"<definition version=\"0.100.0\" type=\"element\" link_type=\"simple\" "
			"width=\"20\" height=\"20\" hotspot_x=\"0\" hotspot_y=\"0\">"
			"<names><name lang=\"en\">Test device</name></names>"
			"<description><rect x=\"0\" y=\"0\" width=\"20\" height=\"20\" "
			"antialias=\"false\" style=\"line-style:normal;line-weight:normal;"
			"filling:none;color:black\"/></description></definition>"));
		const QString name = QUuid::createUuid().toString(QUuid::WithoutBraces) + ".elmt";
		if (!project.embeddedElementCollection()->addElementDefinition("import", name, doc.documentElement()))
			return nullptr;
		int state = -1;
		auto *element = new Element(ElementsLocation("embed://import/" + name, &project), nullptr, &state);
		if (state != 0) {
			delete element;
			return nullptr;
		}
		diagram->addItem(element);
		auto data = element->elementData();
		data.m_type = type;
		data.m_informations.addValue("label", tag);
		element->setElementData(data);
		return element;
	}

	DeviceInformation sample() const
	{
		DeviceInformation info;
		info.m_device_tag = "-K1";
		info.m_manufacturer = QString::fromUtf8("\xC3\x96rnek Elektrik");
		info.m_part_number = "001234";
		info.m_model = "RX-24";
		info.m_description = QString::fromUtf8("G\xC3\xBC\xC3\xA7 kontakt\xC3\xB6r\xC3\xBC");
		info.m_category = "Contactor";
		info.m_quantity = "2.5";
		info.m_voltage_rating = "24 V DC";
		info.m_current_rating = "16 A";
		info.m_notes = "Panel A, \"spare\"; check\nSecond line";
		return info;
	}

private slots:
	void initTestCase()
	{
		QCoreApplication::setOrganizationName("QET-SmartDeviceTests");
		QCoreApplication::setApplicationName("IsolatedTests");
		QSettings::setDefaultFormat(QSettings::IniFormat);
		QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
		QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, m_settings.path());
		QETProject::setBackupEnabled(false);
	}

	void emptyAndLegacyContext()
	{
		const auto empty = DeviceInformation::fromContext(DiagramContext());
		QVERIFY(empty.toContext().keys().isEmpty());
		QVERIFY(empty.m_quantity.isEmpty());
		QDomDocument doc;
		QVERIFY(doc.setContent(QStringLiteral(
			"<elementInformations><elementInformation name=\"label\">K7</elementInformation>"
			"<elementInformation name=\"manufacturer_reference\">0007</elementInformation>"
			"</elementInformations>")));
		DiagramContext context;
		context.fromXml(doc.documentElement(), "elementInformation");
		const auto legacy = DeviceInformation::fromContext(context);
		QCOMPARE(legacy.m_device_tag, QString("K7"));
		QCOMPARE(legacy.m_part_number, QString("0007"));
		QVERIFY(legacy.m_model.isEmpty());
		QVERIFY(legacy.m_voltage_rating.isEmpty());
		QVERIFY(legacy.m_current_rating.isEmpty());
		QVERIFY(legacy.m_notes.isEmpty());
	}

	void contextRoundTripAndMerge()
	{
		DiagramContext original;
		original.addValue("custom_value", "Keep", false);
		original.addValue("manufacturer", "Old", false);
		original.addValue("exclude_from_bom", "true");
		auto info = sample();
		auto context = info.toContext(original);
		QVERIFY(!context.keyMustShow("manufacturer"));
		QVERIFY(!context.keyMustShow("custom_value"));
		QCOMPARE(context.value("custom_value").toString(), QString("Keep"));
		QDomDocument doc;
		auto xml = doc.createElement("elementInformations");
		doc.appendChild(xml);
		context.toXml(xml, "elementInformation");
		QDomDocument reparsed;
		QVERIFY(reparsed.setContent(doc.toByteArray()));
		DiagramContext restored;
		restored.fromXml(reparsed.documentElement(), "elementInformation");
		QVERIFY(restored == context);
		QVERIFY(DeviceInformation::fromContext(restored).toContext() == info.toContext());
		info.m_model.clear();
		const auto cleared = info.toContext(restored);
		QVERIFY(!cleared.contains("model"));
		QCOMPARE(cleared.value("exclude_from_bom").toString(), QString("true"));
	}

	void projectSaveLoad()
	{
		QTemporaryDir temp;
		QETProject project;
		auto *diagram = project.addNewDiagram();
		diagram->border_and_titleblock.setFolio("A-01");
		auto *element = addElement(project, diagram, "-K1");
		QVERIFY(element);
		element->setElementInformations(sample().toContext());
		auto *second_diagram = project.addNewDiagram();
		second_diagram->border_and_titleblock.setFolio("A-02");
		diagram->border_and_titleblock.setFolioData(1, 2);
		second_diagram->border_and_titleblock.setFolioData(2, 2);
		auto *legacy = addElement(project, second_diagram, "-K2");
		QVERIFY(legacy);
		const auto before = DeviceBomExport::toCsv(DeviceBomExport::collect(project));
		project.setFilePath(temp.filePath("roundtrip.qet"));
		const auto result = project.write();
		QVERIFY2(result.isOk(), qPrintable(result.errorMessage()));
		QETProject restored(project.filePath());
		QCOMPARE(restored.state(), QETProject::Ok);
		QCOMPARE(restored.diagrams().size(), 2);
		const auto rows = DeviceBomExport::collect(restored);
		QCOMPARE(rows.size(), 2);
		QCOMPARE(rows.first().m_page, QString("A-01"));
		QCOMPARE(rows.last().m_page, QString("A-02"));
		QVERIFY(rows.first().m_device.toContext() == sample().toContext());
		QVERIFY(rows.last().m_device.m_model.isEmpty());
		QVERIFY(rows.last().m_device.m_quantity.isEmpty());
		QCOMPARE(DeviceBomExport::toCsv(rows), before);

		const QString artifacts = qEnvironmentVariable("QET_SMART_DEVICE_ARTIFACT_DIR");
		if (!artifacts.isEmpty()) {
			QDir().mkpath(artifacts);
			restored.setFilePath(QDir(artifacts).filePath("sample.qet"));
			QVERIFY(restored.write().isOk());
			QVERIFY(DeviceBomExport::writeCsv(restored, QDir(artifacts).filePath("sample.csv")));
		}
	}

	void collectDevices()
	{
		QETProject project;
		auto *first = project.addNewDiagram();
		auto *second = project.addNewDiagram();
		first->border_and_titleblock.setFolio("DEV-01");
		second->border_and_titleblock.setFolio("DEV-10");
		first->border_and_titleblock.setFolioData(1, 2);
		second->border_and_titleblock.setFolioData(2, 2);
		for (auto type : {ElementData::Simple, ElementData::Master, ElementData::Terminal,
				ElementData::Slave, ElementData::NextReport, ElementData::PreviousReport,
				ElementData::Thumbnail, ElementData::ConductorDefinition}) {
			auto *element = addElement(project, first, QString::number(type), type);
			QVERIFY(element);
			if (type == ElementData::Master) {
				auto data = element->elementData();
				data.m_master_type = ElementData::PLC;
				data.m_plc_master_data.ios.resize(2);
				element->setElementData(data);
			}
		}
		auto *excluded = addElement(project, first, "Excluded");
		QVERIFY(excluded);
		auto context = excluded->elementInformations();
		context.addValue("exclude_from_bom", "true");
		excluded->setElementInformations(context);
		for (const QString tag : {"Z2", "A2"}) {
			auto *element = addElement(project, second, tag);
			QVERIFY(element);
			auto info = sample();
			info.m_device_tag = tag;
			element->setElementInformations(info.toContext());
		}
		const auto rows = DeviceBomExport::collect(project);
		QCOMPARE(rows.size(), 4);
		QCOMPARE(rows.at(0).m_page, QString("DEV-01"));
		QCOMPARE(rows.at(0).m_device.m_device_tag, QString("1"));
		QCOMPARE(rows.at(1).m_device.m_device_tag, QString("8"));
		QCOMPARE(rows.at(2).m_page, QString("DEV-10"));
		QCOMPARE(rows.at(2).m_device.m_device_tag, QString("A2"));
		QCOMPARE(rows.at(3).m_device.m_device_tag, QString("Z2"));
		QCOMPARE(rows.at(2).m_device.m_part_number, rows.at(3).m_device.m_part_number);
		QCOMPARE(rows.at(2).m_device.m_quantity, QString("2.5"));
		QCOMPARE(DeviceBomExport::toCsv(rows), DeviceBomExport::toCsv(DeviceBomExport::collect(project)));
	}

	void emptyFolioFallsBackToPagePosition()
	{
		QETProject project;
		auto *diagram = project.addNewDiagram();
		diagram->border_and_titleblock.setFolio(QString());
		diagram->border_and_titleblock.setFolioData(1, 1);
		QVERIFY(addElement(project, diagram, "K1"));
		const auto rows = DeviceBomExport::collect(project);
		QCOMPARE(rows.size(), 1);
		QCOMPARE(rows.first().m_page, QString("1"));
	}

	void csvEscapingAndEncoding()
	{
		const auto csv = DeviceBomExport::toCsv({{sample(), "F-03"}});
		const QByteArray header("\xEF\xBB\xBF\"No\",\"Device tag\",\"Manufacturer\",\"Part number\","
			"\"Model\",\"Description\",\"Category\",\"Quantity\",\"Page\",\"Notes\"\r\n");
		QVERIFY(csv.startsWith(header));
		QVERIFY(csv.contains(sample().m_manufacturer.toUtf8()));
		QVERIFY(csv.contains("\"001234\""));
		QVERIFY(!csv.contains("24 V DC"));
		QVERIFY(!csv.contains("16 A"));
		QVERIFY(csv.endsWith("\"2.5\",\"F-03\",\"Panel A, \"\"spare\"\"; check\nSecond line\"\r\n"));
		QCOMPARE(DeviceBomExport::toCsv({}), header);
	}

	void fileSuccessAndFailure()
	{
		QTemporaryDir temp;
		QETProject project;
		QString error = "old error";
		QVERIFY(!DeviceBomExport::writeCsv(project, temp.filePath("absent/bom.csv"), &error));
		QVERIFY(!error.isEmpty());
		const auto path = temp.filePath("bom.csv");
		QVERIFY(DeviceBomExport::writeCsv(project, path, &error));
		QVERIFY(error.isEmpty());
		QVERIFY(DeviceBomExport::writeCsv(project, path, &error));
		QFile file(path);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QCOMPARE(file.readAll(), DeviceBomExport::toCsv({}));
	}

	void propertyEditorUndoRedo()
	{
		QETProject project;
		auto *diagram = project.addNewDiagram();
		auto *element = addElement(project, diagram, "-K1");
		QVERIFY(element);
		ElementInfoWidget editor(element);
		const auto context = sample().toContext();
		const auto widgets = editor.findChildren<ElementInfoPartWidget *>();
		for (const QString &key : context.keys()) {
			auto it = std::find_if(widgets.begin(), widgets.end(), [&key](ElementInfoPartWidget *widget) {
				return widget->key() == key;
			});
			QVERIFY2(it != widgets.end(), qPrintable(key));
			(*it)->setText(context.value(key).toString().replace('\n', ' '));
		}
		const int undo_count = diagram->undoStack().count();
		editor.apply();
		QCOMPARE(element->elementInformations().value("model").toString(), QString("RX-24"));
		QCOMPARE(diagram->undoStack().count(), undo_count + 1);
		diagram->undoStack().undo();
		QVERIFY(element->elementInformations().value("model").toString().isEmpty());
		diagram->undoStack().redo();
		QCOMPARE(element->elementInformations().value("voltage_rating").toString(), QString("24 V DC"));
		QCOMPARE(DeviceBomExport::collect(project).first().m_device.m_model, QString("RX-24"));
		editor.resize(600, 750);
		editor.show();
		QCoreApplication::processEvents();
		const QString artifacts = qEnvironmentVariable("QET_SMART_DEVICE_ARTIFACT_DIR");
		if (!artifacts.isEmpty()) {
			QVERIFY(editor.grab().save(QDir(artifacts).filePath("properties.png")));
		}
	}

	void existingDatabaseReports()
	{
		QETProject project;
		auto *element = addElement(project, project.addNewDiagram(), "-K1");
		QVERIFY(element);
		element->setElementInformations(sample().toContext());
		project.dataBase()->updateDB();
		auto query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT model,category,voltage_rating,current_rating,notes FROM element_nomenclature_view"));
		QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
		QVERIFY(query.next());
		QCOMPARE(query.value(0).toString(), sample().m_model);
		QCOMPARE(query.value(1).toString(), sample().m_category);
		QCOMPARE(query.value(2).toString(), sample().m_voltage_rating);
		QCOMPARE(query.value(3).toString(), sample().m_current_rating);
		QCOMPARE(query.value(4).toString(), sample().m_notes);
	}
};

QTEST_MAIN(SmartDeviceTest)
#include "tst_smart_device.moc"
