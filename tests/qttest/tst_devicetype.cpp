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
#include "devicetype/devicetype.h"
#include "diagram.h"
#include "qetapptest.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

/**
	Device types: what a kind of device is made of (a CONTR10 is a coil,
	three power contacts and an auxiliary NO), kept in a project and in the
	user's library of .qetdt files.
*/
class tst_devicetype : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;

		//A relay RELAY02-G2R: a coil and two contacts, each drawn as NO,
		//NC or changeover with its own pins
	static DeviceType relay()
	{
		DeviceType type;
		type.name = QStringLiteral("RELAY02-G2R");
		type.names.insert(QStringLiteral("en"), QStringLiteral("RELAY WITH 2 C/O CONTACTS (G2R)"));
		type.informations = {{QStringLiteral("model"), QStringLiteral("RELAY02-G2R")},
							 {QStringLiteral("description"), QStringLiteral("RELAY WITH 2 C/O CONTACTS (G2R)")}};
		DeviceType::Part coil;
		coil.role = QStringLiteral("master");
		coil.kind = QStringLiteral("coil");
		coil.symbols = {{QStringLiteral("embed://import/RELAY02.elmt"), {QStringLiteral("8"), QStringLiteral("1")}, {}}};
		DeviceType::Part contact;
		contact.role = QStringLiteral("slave");
		contact.kind = QStringLiteral("simple");
		contact.symbols = {
			{QStringLiteral("embed://import/RELAY-NO.elmt"), {QStringLiteral("4"), QStringLiteral("3")}, QStringLiteral("NO")},
			{QStringLiteral("embed://import/RELAY-NC.elmt"), {QStringLiteral("2"), QStringLiteral("3")}, QStringLiteral("NC")},
			{QStringLiteral("embed://import/RELAY-CO.elmt"),
			 {QStringLiteral("3"), QStringLiteral("4"), QStringLiteral("2")}, QStringLiteral("SW")}};
			//A part with no symbol yet is still part of the type
		DeviceType::Part unknown;
		unknown.role = QStringLiteral("slave");
		type.parts = {coil, contact, contact, unknown};
		return type;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
	}

	void xmlKeepsEverything()
	{
		const DeviceType type = relay();
		QDomDocument doc;
		const QDomElement e = type.toXml(doc);
		QCOMPARE(e.tagName(), QStringLiteral("devicetype"));
		const DeviceType read = DeviceType::fromXml(e);
		QVERIFY(read == type);
		QCOMPARE(read.parts.at(1).symbols.size(), 3);
		QCOMPARE(read.parts.at(1).symbols.at(2).pins,
				 QStringList({QStringLiteral("3"), QStringLiteral("4"), QStringLiteral("2")}));
		QCOMPARE(read.text(), QStringLiteral("RELAY WITH 2 C/O CONTACTS (G2R)"));
		QVERIFY(read.parts.at(3).symbols.isEmpty());
			//Not a device type
		QVERIFY(DeviceType::fromXml(doc.createElement(QStringLiteral("element"))).isNull());
	}

	void projectKeepsItsTypes_data()
	{
		QTest::addColumn<QString>("file");
		QTest::newRow("qet") << QStringLiteral("types.qet");
		QTest::newRow("qetdb") << QStringLiteral("types.qetdb");
	}

	void projectKeepsItsTypes()
	{
		QFETCH(QString, file);
		const QString path = m_dir.filePath(file);
		{
			QETProject project;
			project.addNewDiagram();
			project.setDeviceType(relay());
			QCOMPARE(project.deviceTypes().size(), 1);
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject again(path);
		QVERIFY(again.deviceType(QStringLiteral("RELAY02-G2R")) == relay());
		again.removeDeviceType(QStringLiteral("RELAY02-G2R"));
		QVERIFY(again.deviceTypes().isEmpty());
	}

	void projectWithoutTypesSavesAsBefore()
	{
		QETProject project;
		project.addNewDiagram();
		const QString path = m_dir.filePath(QStringLiteral("none.qet"));
		project.setFilePath(path);
		QVERIFY(project.write().isOk());
		QFile f(path);
		QVERIFY(f.open(QIODevice::ReadOnly));
		QVERIFY(!f.readAll().contains("<devicetypes"));
	}

		//A device names its type in an element information of its own,
		//so its model stays the manufacturer's
	void deviceTypeIsAnElementInformation()
	{
		QVERIFY(QETInformation::elementInfoKeys().contains(QStringLiteral("device_type")));
		QCOMPARE(QETInformation::translatedInfoKey(QStringLiteral("device_type")), QStringLiteral("Device type"));
		QVERIFY(QETInformation::elementInfoKeys().contains(QStringLiteral("model")));
	}

	void userLibrary()
	{
		const QString folder = m_dir.filePath(QStringLiteral("library"));
		QVERIFY(QDir().mkpath(folder));
		QVERIFY(DeviceTypeLibrary::save(relay(), folder));
		DeviceType odd = relay();
		odd.name = QStringLiteral("1764-24BWA,1764/MM1");   //a name with a slash
		QVERIFY(DeviceTypeLibrary::save(odd, folder));
		QCOMPARE(DeviceTypeLibrary::names(folder),
				 QStringList({QStringLiteral("1764-24BWA,1764/MM1"), QStringLiteral("RELAY02-G2R")}));
		QVERIFY(DeviceTypeLibrary::load(QStringLiteral("RELAY02-G2R"), folder) == relay());
		QCOMPARE(DeviceTypeLibrary::load(QStringLiteral("1764-24BWA,1764/MM1"), folder).name,
				 QStringLiteral("1764-24BWA,1764/MM1"));
		QVERIFY(DeviceTypeLibrary::remove(QStringLiteral("RELAY02-G2R"), folder));
		QCOMPARE(DeviceTypeLibrary::names(folder).size(), 1);
		QVERIFY(DeviceTypeLibrary::load(QStringLiteral("nothing"), folder).isNull());
	}
};

QET_APP_TEST_MAIN(tst_devicetype)
#include "tst_devicetype.moc"
