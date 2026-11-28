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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "cli_export.h"
#include "container/miniz/miniz.h"
#include "container/qetzip.h"
#include "import/edz/edzpart.h"
#include "import/elementfingerprint.h"
#include "import/macropackage/macropackagetree.h"
#include "import/macropackage/macropackage.h"
#include "import/aasx/aasxpackage.h"
#include "import/eplandxf/dwgreader.h"
#include "import/eplandxf/eplandxfreader.h"
#include "import/eplandxf/eplanelementbuilder.h"
#include "import/eplandxf/eplanmacroreader.h"
#include "import/eplandxf/eplanpartimporter.h"
#include "import/plcchannels.h"

#include <QDirIterator>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QFile>
#include <QRegularExpression>

/**
	The part import of manufacturer EPLAN data (Part import > Import
	part...): an EPLAN Data Portal DXF package, zipped or unpacked, a
	single DXF with black-box frames, the part data of an .edz, and the
	element fingerprint that keeps elements edited since their import.
	All the data is made up here: a few lines of ASCII DXF and a
	commercialdata.csv.
*/
class TstEplanPartImport : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	/// one DXF group code and its value
	static QByteArray code(int c, const QByteArray &value)
	{
		return QByteArray::number(c) + "\n" + value + "\n";
	}

	static QByteArray line(double x1, double y1, double x2, double y2)
	{
		return code(0, "LINE") + code(8, "0")
			+ code(10, QByteArray::number(x1)) + code(20, QByteArray::number(y1))
			+ code(11, QByteArray::number(x2)) + code(21, QByteArray::number(y2));
	}

	static QByteArray attrib(const QByteArray &tag, const QByteArray &text)
	{
		return code(0, "ATTRIB") + code(2, tag) + code(1, text);
	}

	/// an EPLAN connection point: an INSERT with its designation and
	/// connection description
	static QByteArray pin(double x, double y, const QByteArray &name, const QByteArray &function)
	{
		return code(0, "INSERT") + code(2, "CP")
			+ code(10, QByteArray::number(x)) + code(20, QByteArray::number(y))
			+ attrib("Connection_Name", name) + attrib("Connection_Description", function)
			+ code(0, "SEQEND");
	}

	/**
		A device of 16 x 24 mm: its frame, the device tag -KF1 and three
		connection points on its left side, two PLC inputs and their
		reference, as EPLAN exports a schematic macro.
	*/
	static QByteArray deviceDxf()
	{
		QByteArray dxf;
		dxf += code(0, "SECTION") + code(2, "BLOCKS");
		dxf += code(0, "BLOCK") + code(2, "CP") + code(10, "0") + code(20, "0") + code(0, "ENDBLK");
		dxf += code(0, "ENDSEC");
		dxf += code(0, "SECTION") + code(2, "ENTITIES");
		dxf += line(0, 0, 16, 0) + line(16, 0, 16, 24) + line(16, 24, 0, 24) + line(0, 24, 0, 0);
		dxf += pin(0, 20, "1", "DI0") + pin(0, 12, "2", "DI1") + pin(0, 4, "3", "M");
		dxf += code(0, "INSERT") + code(2, "CP") + code(10, "8") + code(20, "28")
			+ attrib("DT_Name", "-KF1") + code(0, "SEQEND");
		dxf += code(0, "ENDSEC") + code(0, "EOF");
		return dxf;
	}

	/**
		Two black-box frames (EPLAN "Symbol14" blocks) side by side, one
		with two inputs, one with two outputs: a single DXF of a device
		in two parts.
	*/
	static QByteArray blackBoxDxf()
	{
		QByteArray dxf;
		dxf += code(0, "SECTION") + code(2, "BLOCKS");
		dxf += code(0, "BLOCK") + code(2, "CP") + code(10, "0") + code(20, "0") + code(0, "ENDBLK");
		dxf += code(0, "BLOCK") + code(2, "Symbol14_box") + code(10, "0") + code(20, "0");
		dxf += line(0, 0, 20, 0) + line(20, 0, 20, 40) + line(20, 40, 0, 40) + line(0, 40, 0, 0);
		dxf += code(0, "ENDBLK");
		dxf += code(0, "ENDSEC");
		dxf += code(0, "SECTION") + code(2, "ENTITIES");
		for (const double x : {0.0, 40.0})
			dxf += code(0, "INSERT") + code(2, "Symbol14_box")
				+ code(10, QByteArray::number(x)) + code(20, "0");
		dxf += pin(0, 32, "1", "DI0") + pin(0, 24, "2", "DI1");
		dxf += pin(40, 32, "11", "DQ0") + pin(40, 24, "12", "DQ1");
		dxf += code(0, "ENDSEC") + code(0, "EOF");
		return dxf;
	}

	/**
		An EPLAN window macro with one multi-line schematic variant: a
		black box of 16 x 24 mm with the letters KF and two connection
		points on its left side.
	*/
	static QByteArray smallMacro()
	{
		return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		       "<EplanPxfRoot Type=\"WindowMacro\" Description=\"en_US@Input module;\">\n"
		       " <MacroVariant MacroFuncType=\"1\" VariantId=\"0\">\n"
		       "  <O17><S40x1201 A762=\"0/0\"/><S89x5 A1651=\"0/0\" A1652=\"16/24\"/>"
		       "<P150 P20013=\"KF\"/></O17>\n"
		       "  <O17 A185=\"1\" A186=\"en_US@DI0;\"><S40x1201 A762=\"0/16\"/></O17>\n"
		       "  <O17 A185=\"2\" A186=\"en_US@DI1;\"><S40x1201 A762=\"0/8\"/></O17>\n"
		       " </MacroVariant>\n"
		       "</EplanPxfRoot>\n";
	}

	static QByteArray commercialData()
	{
		return "\"Order number\",\"Type number\",\"Manufacturer (full name)\","
		       "\"Description 1\",\"Product group\"\r\n"
		       "\"1234567\",\"TYPE-1\",\"Acme\",\"Input module, 2 channels\",\"PLC\"\r\n";
	}

	/// the files of a Data Portal package, by their path in it
	static QList<QetZip::Entry> packageEntries()
	{
		return {{QStringLiteral("commercialdata.csv"), commercialData()},
			{QStringLiteral("dxf/1234567/1/1234567_A1.dxf"), deviceDxf()}};
	}

	static QDomDocument readXml(const QString &path)
	{
		QDomDocument doc;
		QFile file(path);
		if (file.open(QIODevice::ReadOnly))
			doc.setContent(&file);
		return doc;
	}

	static QString info(const QDomDocument &element, const QString &name)
	{
		const QDomNodeList list = element.elementsByTagName(QStringLiteral("elementInformation"));
		for (int i = 0; i < list.size(); ++i) {
			const QDomElement e = list.at(i).toElement();
			if (e.attribute(QStringLiteral("name")) == name)
				return e.text();
		}
		return QString();
	}

	static QStringList terminalNames(const QDomDocument &element)
	{
		QStringList names;
		const QDomNodeList list = element.elementsByTagName(QStringLiteral("terminal"));
		for (int i = 0; i < list.size(); ++i) {
			const QDomElement t = list.at(i).toElement();
			if (t.hasAttribute(QStringLiteral("x")))
				names << t.attribute(QStringLiteral("name"));
		}
		return names;
	}

	QString writeFile(const QString &relative, const QByteArray &content)
	{
		const QString path = m_dir.filePath(relative);
		QDir().mkpath(QFileInfo(path).absolutePath());
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly))
			return QString();
		file.write(content);
		return path;
	}

	private slots:
		/// The reader finds the connection points, their descriptions and
		/// the device tag by the EPLAN attribute tags
		void dxfReader()
		{
			EplanDxfReader reader;
			QVERIFY2(reader.read(deviceDxf()), qPrintable(reader.errorString()));
			QCOMPARE(reader.pins().size(), 3);
			QCOMPARE(reader.pins().at(0).name, QStringLiteral("1"));
			QCOMPARE(reader.pins().at(0).function, QStringLiteral("DI0"));
			QCOMPARE(reader.pins().at(0).pos, QPointF(0, 20));
			QCOMPARE(reader.lines().size(), 4);
			QCOMPARE(reader.deviceTag(), QStringLiteral("-KF1"));
			QVERIFY(reader.boxes().isEmpty());

			QVERIFY(!reader.read("no dxf at all\n"));
			QVERIFY(!reader.errorString().isEmpty());
		}

		/// A zipped Data Portal package: manufacturer, product group and
		/// type from commercialdata.csv, one element per variant
		void package()
		{
			const QString zip = m_dir.filePath(QStringLiteral("MAC_1234567.zip"));
			QString error;
			QVERIFY2(QetZip::write(zip, packageEntries(), &error), qPrintable(error));

			const QMap<QString, QString> part = EplanPartImporter::packageInfo(zip, &error);
			QCOMPARE(part.value(QStringLiteral("Type number")), QStringLiteral("TYPE-1"));
			QCOMPARE(part.value(QStringLiteral("Articles")), QStringLiteral("1234567"));

			const QString collection = m_dir.filePath(QStringLiteral("zipped"));
			EplanPartImporter importer;
			QVERIFY2(importer.importPackage(zip, QString(), collection),
				 qPrintable(importer.errorString()));
			const QString element = collection + QStringLiteral("/Acme/PLC/TYPE-1/TYPE-1.elmt");
			QCOMPARE(importer.writtenPaths(), QStringList{element});
			for (const QString &folder : {QStringLiteral("/Acme"), QStringLiteral("/Acme/PLC"),
						      QStringLiteral("/Acme/PLC/TYPE-1")})
				QVERIFY(QFile::exists(collection + folder + QStringLiteral("/qet_directory")));

			const QDomDocument doc = readXml(element);
			QCOMPARE(doc.documentElement().tagName(), QStringLiteral("definition"));
			QCOMPARE(terminalNames(doc), (QStringList{"1", "2", "3"}));
			QCOMPARE(info(doc, QStringLiteral("manufacturer")), QStringLiteral("Acme"));
			QCOMPARE(info(doc, QStringLiteral("manufacturer_reference")), QStringLiteral("1234567"));
			QCOMPARE(info(doc, QStringLiteral("designation")), QStringLiteral("TYPE-1"));
			QCOMPARE(info(doc, QStringLiteral("description")), QStringLiteral("Input module 2 channels"));
			QCOMPARE(ElementFingerprint::state(doc), ElementFingerprint::Unedited);
		}

		/// The unpacked package, a folder, gives the same element
		void unpackedPackage()
		{
			const QString folder = m_dir.filePath(QStringLiteral("unpacked"));
			for (const QetZip::Entry &e : packageEntries())
				QVERIFY(!writeFile(QStringLiteral("unpacked/") + e.name, e.data).isEmpty());
			const QString collection = m_dir.filePath(QStringLiteral("from_folder"));
			EplanPartImporter importer;
			QVERIFY2(importer.importPackage(folder, QStringLiteral("Inputs"), collection),
				 qPrintable(importer.errorString()));
			QCOMPARE(importer.writtenPaths(),
				 QStringList{collection + QStringLiteral("/Acme/Inputs/TYPE-1/TYPE-1.elmt")});
		}

		/// A ZIP without commercialdata.csv is no Data Portal package
		void notAPackage()
		{
			const QString zip = m_dir.filePath(QStringLiteral("other.zip"));
			QVERIFY(QetZip::write(zip, {{QStringLiteral("readme.txt"), "hello"}}));
			EplanPartImporter importer;
			QVERIFY(!importer.importPackage(zip, QString(), m_dir.filePath(QStringLiteral("none"))));
			QVERIFY(importer.errorString().contains(QStringLiteral("commercialdata.csv")));
			QVERIFY(!QFileInfo::exists(m_dir.filePath(QStringLiteral("none"))));
		}

		/// An element nobody touched is written anew and keeps its uuid;
		/// one edited since its import is kept unless forced
		void editedElementKept()
		{
			const QString zip = m_dir.filePath(QStringLiteral("again.zip"));
			QVERIFY(QetZip::write(zip, packageEntries()));
			const QString collection = m_dir.filePath(QStringLiteral("again"));
			const QString element = collection + QStringLiteral("/Acme/PLC/TYPE-1/TYPE-1.elmt");

			EplanPartImporter importer;
			QVERIFY(importer.importPackage(zip, QString(), collection));
			const QString uuid = readXml(element).documentElement()
						     .firstChildElement(QStringLiteral("uuid")).attribute(QStringLiteral("uuid"));
			QVERIFY(!uuid.isEmpty());

			QVERIFY(importer.importPackage(zip, QString(), collection));
			QCOMPARE(importer.writtenPaths(), QStringList{element});
			QCOMPARE(readXml(element).documentElement().firstChildElement(QStringLiteral("uuid"))
					 .attribute(QStringLiteral("uuid")), uuid);

				// edited by hand: another description
			QDomDocument doc = readXml(element);
			QDomElement description = doc.documentElement().firstChildElement(QStringLiteral("description"));
			QDomElement extra = doc.createElement(QStringLiteral("line"));
			for (const char *a : {"x1", "y1", "x2", "y2"})
				extra.setAttribute(QLatin1String(a), QStringLiteral("0"));
			description.appendChild(extra);
			QCOMPARE(ElementFingerprint::state(doc), ElementFingerprint::Edited);
			QFile file(element);
			QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
			file.write(doc.toByteArray(4));
			file.close();
			const QByteArray edited = doc.toByteArray(4);

			QVERIFY(importer.importPackage(zip, QString(), collection));
			QVERIFY(importer.writtenPaths().isEmpty());
			QCOMPARE(importer.keptPaths(), QStringList{element});
			{
				QFile again(element);
				QVERIFY(again.open(QIODevice::ReadOnly));
				QCOMPARE(again.readAll(), edited);
			}

			importer.setForce(true);
			QVERIFY(importer.importPackage(zip, QString(), collection));
			QCOMPARE(importer.writtenPaths(), QStringList{element});
			QCOMPARE(ElementFingerprint::state(readXml(element)), ElementFingerprint::Unedited);
		}

		/// A single DXF with two black-box frames gives an element per
		/// frame, named after what its terminals do
		void singleDxfBlackBoxes()
		{
			const QString dxf = writeFile(QStringLiteral("device.dxf"), blackBoxDxf());
			QVERIFY(!dxf.isEmpty());
			const QString collection = m_dir.filePath(QStringLiteral("single"));
			EplanPartImporter importer;
			QVERIFY2(importer.importDxf(dxf, QString(), QStringLiteral("Acme"), QStringLiteral("PLC"),
						    QStringLiteral("CPU-9"), collection),
				 qPrintable(importer.errorString()));
			const QString folder = collection + QStringLiteral("/Acme/PLC/CPU-9/");
			QStringList written = importer.writtenPaths();
			written.sort();
			QCOMPARE(written, (QStringList{folder + QStringLiteral("CPU-9_Inputs.elmt"),
						       folder + QStringLiteral("CPU-9_Outputs.elmt")}));
			QCOMPARE(terminalNames(readXml(folder + QStringLiteral("CPU-9_Inputs.elmt"))),
				 (QStringList{"1", "2"}));
			QCOMPARE(terminalNames(readXml(folder + QStringLiteral("CPU-9_Outputs.elmt"))),
				 (QStringList{"11", "12"}));
			QCOMPARE(info(readXml(folder + QStringLiteral("CPU-9_Inputs.elmt")),
				      QStringLiteral("manufacturer_reference")), QStringLiteral("CPU-9"));
		}

		/// One function template of an EPLAN part lists all connection
		/// points of its function, one per line (a changeover contact)
		void edzConnectionPoints()
		{
			const QString xml = writeFile(
				QStringLiteral("edz/part.xml"),
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<partsmanagement type=\"EPLAN.PartsManagement\">\n"
				" <part P_ARTICLE_PARTNR=\"ACME.R1\" P_ARTICLE_MANUFACTURER=\"ACME\""
				" P_ARTICLE_ORDERNR=\"R1\" P_ARTICLE_TYPENR=\"RELAY-1\""
				" P_ARTICLE_DESCR1=\"en_US@Relay;\">\n"
				"  <functiontemplate connectionDesignation=\"A1&#10;A2\""
				" connectiondescription=\"+&#10;-\" terminalNr=\"X1\"/>\n"
				"  <functiontemplate connectionDesignation=\"12&#10;11&#10;14\""
				" connectiondescription=\"\" terminalNr=\"X2\"/>\n"
				" </part>\n"
				"</partsmanagement>\n");
			QVERIFY(!xml.isEmpty());
			EdzPart part;
			QVERIFY2(part.parse(xml), qPrintable(part.errorString()));
			QCOMPARE(part.orderNumber(), QStringLiteral("R1"));
			QCOMPARE(part.typeNumber(), QStringLiteral("RELAY-1"));
			QStringList designations;
			for (const EdzPin &p : part.pins()) designations << p.designation;
			designations.sort();
			QCOMPARE(designations, (QStringList{"11", "12", "14", "A1", "A2"}));
			for (const EdzPin &p : part.pins())
				if (p.designation == QLatin1String("A2"))
					QCOMPARE(p.description, QStringLiteral("-"));
		}

		/// A function with one connection point keeps its description
		/// whole, also over several lines
		void edzMultiLineDescription()
		{
			const QString xml = writeFile(
				QStringLiteral("edz_descr/part.xml"),
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<partsmanagement type=\"EPLAN.PartsManagement\">\n"
				" <part P_ARTICLE_PARTNR=\"ACME.T1\" P_ARTICLE_MANUFACTURER=\"ACME\""
				" P_ARTICLE_ORDERNR=\"T1\" P_ARTICLE_TYPENR=\"TERM-1\">\n"
				"  <functiontemplate connectionDesignation=\"PE\""
				" connectiondescription=\"Protective&#10;earth\" terminalNr=\"X1\"/>\n"
				" </part>\n"
				"</partsmanagement>\n");
			QVERIFY(!xml.isEmpty());
			EdzPart part;
			QVERIFY2(part.parse(xml), qPrintable(part.errorString()));
			QCOMPARE(part.pins().size(), 1);
			QCOMPARE(part.pins().first().designation, QStringLiteral("PE"));
			QCOMPARE(part.pins().first().description, QStringLiteral("Protective\nearth"));
		}

		/// Folder names from the imported data or the command line that
		/// are "." or ".." would leave the collection: refused, and
		/// nothing is written, neither inside nor above it
		void dotFoldersRefused()
		{
			const QString jail = m_dir.filePath(QStringLiteral("dots/a/b"));
			QVERIFY(QDir().mkpath(jail));
			const QString collection = jail + QStringLiteral("/collection");

			const QString zip = m_dir.filePath(QStringLiteral("dots.zip"));
			const QByteArray csv =
				"\"Order number\",\"Type number\",\"Manufacturer (full name)\","
				"\"Description 1\",\"Product group\"\r\n"
				"\"1234567\",\"..\",\"..\",\"Input module\",\"..\"\r\n";
			QVERIFY(QetZip::write(zip, {{QStringLiteral("commercialdata.csv"), csv},
						    {QStringLiteral("dxf/1234567/1/1234567_A1.dxf"), deviceDxf()}}));
			EplanPartImporter importer;
			QVERIFY(!importer.importPackage(zip, QString(), collection));
			QVERIFY(!importer.errorString().isEmpty());
			QVERIFY(importer.writtenPaths().isEmpty());

			const QString dxf = writeFile(QStringLiteral("dots.dxf"), deviceDxf());
			for (const QStringList &levels : QList<QStringList>{
				     {QStringLiteral(".."), QStringLiteral("PLC"), QStringLiteral("DEV")},
				     {QStringLiteral("Acme"), QStringLiteral(".."), QStringLiteral("..")},
				     {QStringLiteral("Acme"), QStringLiteral("PLC"), QStringLiteral(".")},
				     {QStringLiteral("Acme"), QStringLiteral(" "), QStringLiteral("DEV")},
				     {QStringLiteral("..."), QStringLiteral("PLC"), QStringLiteral("DEV")}}) {
				QVERIFY2(!importer.importDxf(dxf, QString(), levels.at(0), levels.at(1),
							     levels.at(2), collection),
					 qPrintable(levels.join(QLatin1Char('|'))));
				QVERIFY(importer.writtenPaths().isEmpty());
			}
			QCOMPARE(CLIExport::run({QStringLiteral("--import-eplan"), dxf, collection,
						 QStringLiteral("--manufacturer"), QStringLiteral(".."),
						 QStringLiteral("--type"), QStringLiteral("..")}), 1);

			QStringList files;
			for (QDirIterator it(m_dir.filePath(QStringLiteral("dots")), QDir::Files | QDir::Hidden,
					     QDirIterator::Subdirectories);
			     it.hasNext();)
				files << it.next();
			QVERIFY2(files.isEmpty(), qPrintable(files.join(QLatin1Char(' '))));
		}

		/// A drawing of one zero-length line has no size: no element, no
		/// crash
		void zeroLengthLine()
		{
			const QByteArray dxf = code(0, "SECTION") + code(2, "ENTITIES") + line(5, 5, 5, 5)
					       + code(0, "ENDSEC") + code(0, "EOF");
			EplanDxfReader reader;
			QVERIFY2(reader.read(dxf), qPrintable(reader.errorString()));
			EplanSymbolData data;
			data.lines = reader.lines();
			QVERIFY(EplanElementBuilder::build(data, {{QStringLiteral("en"), QStringLiteral("x")}}, {})
					.documentElement().isNull());
			QVERIFY(EplanElementBuilder::build(EplanSymbolData(), {}, {}).documentElement().isNull());

			// as the panel layout of a Data Portal package
			const QString zip = m_dir.filePath(QStringLiteral("zero.zip"));
			QVERIFY(QetZip::write(zip, {{QStringLiteral("commercialdata.csv"), commercialData()},
						    {QStringLiteral("dxf/1234567/6/1234567_A6.dxf"), dxf}}));
			const QString collection = m_dir.filePath(QStringLiteral("zero"));
			EplanPartImporter importer;
			QVERIFY(!importer.importPackage(zip, QString(), collection));
			QVERIFY(importer.writtenPaths().isEmpty());
		}

		/// The sweep of an ARC is at most a full turn and its segments are
		/// bounded, whatever the angles and the radius in the file
		void hugeArc()
		{
			for (const QByteArray &radius : {QByteArrayLiteral("10"), QByteArrayLiteral("1e9")}) {
				const QByteArray dxf = code(0, "SECTION") + code(2, "ENTITIES")
					+ code(0, "ARC") + code(8, "0") + code(10, "0") + code(20, "0")
					+ code(40, radius) + code(50, "0") + code(51, "1e10")
					+ code(0, "ENDSEC") + code(0, "EOF");
				QElapsedTimer timer;
				timer.start();
				EplanDxfReader reader;
				QVERIFY2(reader.read(dxf), qPrintable(reader.errorString()));
				QVERIFY(!reader.lines().isEmpty());
				QVERIFY(reader.lines().size() <= 360);
				QVERIFY(timer.elapsed() < 5000);
			}
		}

		/// Block references nested deep and wide are cut off at a total
		/// budget instead of expanding without end
		void insertFanOut()
		{
			QByteArray dxf = code(0, "SECTION") + code(2, "BLOCKS");
			for (int level = 0; level < 8; ++level) {
				dxf += code(0, "BLOCK") + code(2, "B" + QByteArray::number(level))
					+ code(10, "0") + code(20, "0");
				for (int k = 0; k < 20; ++k)
					dxf += code(0, "INSERT") + code(2, "B" + QByteArray::number(level + 1))
						+ code(10, QByteArray::number(k)) + code(20, "0");
				dxf += code(0, "ENDBLK");
			}
			dxf += code(0, "BLOCK") + code(2, "B8") + code(10, "0") + code(20, "0")
				+ line(0, 0, 1, 1) + code(0, "ENDBLK");
			dxf += code(0, "ENDSEC") + code(0, "SECTION") + code(2, "ENTITIES")
				+ code(0, "INSERT") + code(2, "B0") + code(10, "0") + code(20, "0")
				+ code(0, "ENDSEC") + code(0, "EOF");
			QElapsedTimer timer;
			timer.start();
			EplanDxfReader reader;
			QVERIFY(!reader.read(dxf));
			QVERIFY(!reader.errorString().isEmpty());
			QVERIFY(timer.elapsed() < 30000);
		}

		/// Coordinates that are not finite numbers never reach the element
		void notFiniteCoordinates()
		{
			QByteArray dxf;
			dxf += code(0, "SECTION") + code(2, "BLOCKS");
			dxf += code(0, "BLOCK") + code(2, "CP") + code(10, "0") + code(20, "0") + code(0, "ENDBLK");
			dxf += code(0, "ENDSEC");
			dxf += code(0, "SECTION") + code(2, "ENTITIES");
			dxf += line(0, 0, 16, 0) + line(16, 0, 16, 24) + line(16, 24, 0, 24) + line(0, 24, 0, 0);
			dxf += code(0, "LINE") + code(8, "0") + code(10, "nan") + code(20, "0")
				+ code(11, "4") + code(21, "inf");
			dxf += code(0, "CIRCLE") + code(8, "0") + code(10, "8") + code(20, "-inf") + code(40, "2");
			dxf += code(0, "TEXT") + code(8, "0") + code(10, "nan") + code(20, "2") + code(40, "2.5")
				+ code(1, "LOST");
			dxf += pin(0, 20, "1", "DI0") + pin(0, 12, "2", "DI1");
			dxf += code(0, "INSERT") + code(2, "CP") + code(10, "nan") + code(20, "4")
				+ attrib("Connection_Name", "3") + code(0, "SEQEND");
			dxf += code(0, "ENDSEC") + code(0, "EOF");

			EplanDxfReader reader;
			QVERIFY2(reader.read(dxf), qPrintable(reader.errorString()));
			QCOMPARE(reader.lines().size(), 4);
			QVERIFY(reader.circles().isEmpty());
			QCOMPARE(reader.pins().size(), 2);
			for (const EplanDxfText &t : reader.texts())
				QVERIFY(t.text != QLatin1String("LOST"));

			const QString path = writeFile(QStringLiteral("nan.dxf"), dxf);
			const QString collection = m_dir.filePath(QStringLiteral("nan"));
			EplanPartImporter importer;
			QVERIFY2(importer.importDxf(path, QString(), QStringLiteral("Acme"), QStringLiteral("PLC"),
						    QStringLiteral("NAN-1"), collection),
				 qPrintable(importer.errorString()));
			QCOMPARE(importer.writtenPaths().size(), 1);
			QFile element(importer.writtenPaths().first());
			QVERIFY(element.open(QIODevice::ReadOnly));
			const QString xml = QString::fromUtf8(element.readAll());
			static const QRegularExpression not_finite(QStringLiteral("=\"[-+]?(nan|inf)"),
								   QRegularExpression::CaseInsensitiveOption);
			QVERIFY2(!xml.contains(not_finite), qPrintable(xml));

			// the builder drops them as well, whatever reader the data comes from
			EplanSymbolData data;
			data.lines = {QLineF(0, 0, 16, 0), QLineF(qQNaN(), 0, 1, 1)};
			data.circles = {{QPointF(qInf(), 0), 1.0}};
			EplanDxfPin good;
			good.pos = QPointF(0, 0);
			good.name = QStringLiteral("1");
			EplanDxfPin bad = good;
			bad.pos = QPointF(qQNaN(), 0);
			bad.name = QStringLiteral("2");
			data.pins = {good, bad};
			const QDomDocument doc = EplanElementBuilder::build(
				data, {{QStringLiteral("en"), QStringLiteral("x")}}, {});
			QVERIFY(!doc.documentElement().isNull());
			QVERIFY2(!doc.toString().contains(not_finite), qPrintable(doc.toString()));
			QCOMPARE(terminalNames(doc), QStringList{"1"});
		}

		/// The schematic of an EPLAN window macro (.ema, as an .edz
		/// carries it): its black box with identifying letters and its
		/// connection points with their descriptions
		void macroReader()
		{
			const QString ema = writeFile(QStringLiteral("macro/module.ema"), smallMacro());
			QVERIFY(!ema.isEmpty());
			QCOMPARE(EplanMacroReader::variantCount(ema), 1);
			EplanMacroReader reader;
			QVERIFY2(reader.read(ema), qPrintable(reader.errorString()));
			QCOMPARE(reader.deviceLetters(), QStringLiteral("KF"));
			QCOMPARE(reader.description(), QStringLiteral("Input module"));
			const QVector<EplanDxfPin> pins = reader.symbol().pins;
			QCOMPARE(pins.size(), 2);
			QCOMPARE(pins.at(0).name, QStringLiteral("1"));
			QCOMPARE(pins.at(0).function, QStringLiteral("DI0"));
			QCOMPARE(pins.at(0).pos, QPointF(0, 16));
			QVERIFY(reader.symbol().lines.size() >= 4);	// the frame
			QVERIFY(!reader.read(ema, 1));	// no second schematic variant
			QVERIFY(!reader.readLayout(ema));	// no panel layout
		}

		/// The channel table makes an element a PLC module, once
		void plcChannels()
		{
			QDomDocument doc;
			QVERIFY(doc.setContent(QByteArray(
				"<definition type=\"element\" link_type=\"simple\">"
				"<elementInformations/><description/></definition>")));
			const QVector<PlcChannels::Channel> channels {
				{QStringLiteral("entree_digitale"), QStringLiteral("DI0"), {QStringLiteral("1"), QStringLiteral("3")}},
				{QStringLiteral("entree_digitale"), QStringLiteral("DI1"), {QStringLiteral("2"), QStringLiteral("3")}}};
			QVERIFY(PlcChannels::setChannels(doc, channels));
			QCOMPARE(doc.documentElement().attribute(QStringLiteral("link_type")), QStringLiteral("master"));
			const QDomElement ios = doc.documentElement().firstChildElement(QStringLiteral("kindInformations"))
							.firstChildElement(QStringLiteral("plcMasterData"))
							.firstChildElement(QStringLiteral("plcIOs"));
			QCOMPARE(ios.elementsByTagName(QStringLiteral("plcIO")).size(), 2);
			QCOMPARE(ios.firstChildElement().attribute(QStringLiteral("address")), QStringLiteral("DI0"));
				// already exactly that
			QVERIFY(!PlcChannels::setChannels(doc, channels));
		}

		/// The fingerprint covers the content, not the indentation
		void fingerprint()
		{
			QDomDocument doc;
			QVERIFY(doc.setContent(QByteArray(
				"<definition type=\"element\"><elementInformations/>"
				"<description><line x1=\"0\" y1=\"0\" x2=\"10\" y2=\"0\"/></description></definition>")));
			QCOMPARE(ElementFingerprint::state(doc), ElementFingerprint::Missing);
			ElementFingerprint::stamp(doc);
			QCOMPARE(ElementFingerprint::state(doc), ElementFingerprint::Unedited);
			QDomDocument reindented;
			QVERIFY(reindented.setContent(doc.toByteArray(1)));
			QCOMPARE(ElementFingerprint::state(reindented), ElementFingerprint::Unedited);
			doc.documentElement().firstChildElement(QStringLiteral("description"))
				.firstChildElement().setAttribute(QStringLiteral("x2"), QStringLiteral("20"));
			QCOMPARE(ElementFingerprint::state(doc), ElementFingerprint::Edited);
		}

		/// qelectrotech --import-eplan
		void commandLine()
		{
			const QString zip = m_dir.filePath(QStringLiteral("cli.zip"));
			QVERIFY(QetZip::write(zip, packageEntries()));
			const QString collection = m_dir.filePath(QStringLiteral("cli"));
			QCOMPARE(CLIExport::run({QStringLiteral("--import-eplan"), zip, collection}), 0);
			QVERIFY(QFile::exists(collection + QStringLiteral("/Acme/PLC/TYPE-1/TYPE-1.elmt")));

			const QString dxf = writeFile(QStringLiteral("cli.dxf"), deviceDxf());
			QCOMPARE(CLIExport::run({QStringLiteral("--import-eplan"), dxf, collection,
						 QStringLiteral("--manufacturer"), QStringLiteral("Acme"),
						 QStringLiteral("--type"), QStringLiteral("DEV-2")}), 0);
			QVERIFY(QFile::exists(collection + QStringLiteral("/Acme/General/DEV-2/DEV-2_Inputs_+_Supply.elmt")));

				//A single DXF needs manufacturer and type, a collection folder
			QCOMPARE(CLIExport::run({QStringLiteral("--import-eplan"), dxf, collection}), 2);
			QCOMPARE(CLIExport::run({QStringLiteral("--import-eplan"), zip}), 2);
				//Not a package
			QCOMPARE(CLIExport::run({QStringLiteral("--import-eplan"),
						 m_dir.filePath(QStringLiteral("no_such.zip")), collection}), 1);
		}

		/// The part data of a WSCAD Universe package (.wspak): "ws" in
		/// front of a ZIP whose offsets count that prefix in, entry names
		/// with backslashes
		void wspakPartData()
		{
			const QByteArray json =
				"{\"manufacturerName\": \"Acme\", \"number\": \"9876\", \"typeName\": \"REL-5\","
				" \"translateName\": {\"En\": \"Relay\"}, \"descriptionOne\": \"Relay, 1 changeover\"}";
			mz_zip_archive zip;
			mz_zip_zero_struct(&zip);
			QVERIFY(mz_zip_writer_init_heap(&zip, 2, 0));
			QVERIFY(mz_zip_writer_add_mem(&zip, "Data\\part.json", json.constData(), size_t(json.size()),
						      MZ_DEFAULT_COMPRESSION));
			void *data = nullptr;
			size_t size = 0;
			QVERIFY(mz_zip_writer_finalize_heap_archive(&zip, &data, &size));
			QByteArray bytes(static_cast<const char *>(data), qsizetype(size));
			mz_zip_writer_end(&zip);
			bytes.replace(0, 2, "ws");
			const QString wspak = writeFile(QStringLiteral("part.wspak"), bytes);
			QVERIFY(!wspak.isEmpty());
			const QMap<QString, QString> part = EplanPartImporter::wspakPartInfo(wspak);
			QCOMPARE(part.value(QStringLiteral("manufacturer")), QStringLiteral("Acme"));
			QCOMPARE(part.value(QStringLiteral("order")), QStringLiteral("9876"));
			QCOMPARE(part.value(QStringLiteral("type")), QStringLiteral("REL-5"));
			QCOMPARE(part.value(QStringLiteral("description")), QStringLiteral("Relay"));
			QVERIFY(EplanPartImporter::wspakPartInfo(m_dir.filePath(QStringLiteral("none.wspak"))).isEmpty());

				// a part.json larger than any real one (5 MB of spaces, packed
				// to a few KB) is refused by its declared size, not unpacked
			QByteArray big = json;
			big.append(QByteArray(5 * 1024 * 1024, ' '));
			mz_zip_zero_struct(&zip);
			QVERIFY(mz_zip_writer_init_heap(&zip, 2, 0));
			QVERIFY(mz_zip_writer_add_mem(&zip, "Data\\part.json", big.constData(), size_t(big.size()),
						      MZ_DEFAULT_COMPRESSION));
			data = nullptr;
			size = 0;
			QVERIFY(mz_zip_writer_finalize_heap_archive(&zip, &data, &size));
			QByteArray packed(static_cast<const char *>(data), qsizetype(size));
			mz_zip_writer_end(&zip);
			packed.replace(0, 2, "ws");
			QVERIFY(packed.size() < 1024 * 1024);
			const QString bomb = writeFile(QStringLiteral("big.wspak"), packed);
			QVERIFY(!bomb.isEmpty());
			QVERIFY(EplanPartImporter::wspakPartInfo(bomb).isEmpty());
		}

		/// A DWG of another AutoCAD version, or one cut short, is refused
		/// with a reason, and so is anything else
		void dwgRefused()
		{
			DwgReader reader;
			QByteArray r2000("AC1015");
			r2000.append(QByteArray(200, '\0'));
			QVERIFY(!reader.read(r2000));
			QVERIFY(reader.errorString().contains(QStringLiteral("not supported")));
			QByteArray cut("AC1018");
			cut.append(QByteArray(40, '\0'));
			QVERIFY(!reader.read(cut));
			QVERIFY(!reader.errorString().isEmpty());
				// noise must not crash the reader
			QByteArray noise("AC1032");
			for (int i = 0; i < 4096; ++i) noise.append(char((i * 7919 + 13) % 251));
			QVERIFY(!reader.read(noise));
			QVERIFY(!reader.read(QByteArray()));

			const QString dwg = writeFile(QStringLiteral("broken.dwg"), cut);
			const QString collection = m_dir.filePath(QStringLiteral("dwg"));
			EplanPartImporter importer;
			QVERIFY(!importer.importDwg(dwg, QString(), QStringLiteral("Acme"), QStringLiteral("General"),
						    QStringLiteral("REL-5"), collection));
			QVERIFY(!importer.errorString().isEmpty());
			QVERIFY(!QFileInfo::exists(collection));
			QCOMPARE(CLIExport::run({QStringLiteral("--import-eplan"), dwg, collection,
						 QStringLiteral("--manufacturer"), QStringLiteral("Acme"),
						 QStringLiteral("--type"), QStringLiteral("REL-5")}), 1);
		}

		/// An Asset Administration Shell package: the Digital Nameplate
		/// names the part, its DXF becomes the element, its STEP model
		/// and its documents go into folders next to the collection
		void aasxPackage()
		{
			const QByteArray rels =
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
				"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
				"<Relationship Type=\"http://www.admin-shell.io/aasx/relationships/aasx-origin\""
				" Target=\"/aasx/aasx-origin\" Id=\"r1\"/></Relationships>";
			const QByteArray origin_rels =
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
				"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
				"<Relationship Type=\"http://www.admin-shell.io/aasx/relationships/aas-spec\""
				" Target=\"/aasx/json/content.json\" Id=\"r2\"/></Relationships>";
			const QByteArray content =
				"{\"submodels\": [{\"idShort\": \"Nameplate\", \"id\": \"urn:example:nameplate\","
				" \"submodelElements\": ["
				"{\"modelType\": \"MultiLanguageProperty\", \"idShort\": \"ManufacturerName\","
				" \"value\": [{\"language\": \"en\", \"text\": \"Acme GmbH\"}]},"
				"{\"modelType\": \"Property\", \"idShort\": \"OrderCodeOfManufacturer\", \"value\": \"AX-100\"},"
				"{\"modelType\": \"Property\", \"idShort\": \"ManufacturerProductType\", \"value\": \"AX 100\"},"
				"{\"modelType\": \"MultiLanguageProperty\", \"idShort\": \"ManufacturerProductDesignation\","
				" \"value\": [{\"language\": \"en\", \"text\": \"Input module\"}]}]}]}";
			const QString aasx = m_dir.filePath(QStringLiteral("part.aasx"));
			QVERIFY(QetZip::write(aasx, {
				{QStringLiteral("_rels/.rels"), rels},
				{QStringLiteral("aasx/aasx-origin"), "Intentionally empty."},
				{QStringLiteral("aasx/_rels/aasx-origin.rels"), origin_rels},
				{QStringLiteral("aasx/json/content.json"), content},
				{QStringLiteral("aasx/files/AX-100.dxf"), deviceDxf()},
				{QStringLiteral("aasx/files/AX-100.stp"), "ISO-10303-21;\nEND-ISO-10303-21;\n"},
				{QStringLiteral("aasx/files/manual.pdf"), "%PDF-1.4\n"}}));

			AasxPackage package;
			QVERIFY2(package.open(aasx), qPrintable(package.errorString()));
			QCOMPARE(package.manufacturer(), QStringLiteral("Acme GmbH"));
			QCOMPARE(package.orderNumber(), QStringLiteral("AX-100"));
			QCOMPARE(package.typeNumber(), QStringLiteral("AX 100"));
			QCOMPARE(EplanPartImporter::shortManufacturer(package.manufacturer()), QStringLiteral("Acme"));

			const QString base = m_dir.filePath(QStringLiteral("aasx"));
			const QString collection = base + QStringLiteral("/collection");
			EplanPartImporter importer;
			QVERIFY2(importer.importAasx(aasx, QStringLiteral("PLC"), collection),
				 qPrintable(importer.errorString()));
			const QString element = collection + QStringLiteral("/Acme/PLC/AX 100/AX_100_Inputs_+_Supply.elmt");
			QVERIFY2(importer.writtenPaths().contains(element), qPrintable(importer.writtenPaths().join('\n')));
			QCOMPARE(info(readXml(element), QStringLiteral("manufacturer")), QStringLiteral("Acme"));
			QVERIFY(QFile::exists(base + QStringLiteral("/3D/Acme/AX-100.stp")));
			QVERIFY(QFile::exists(base + QStringLiteral("/manufacturer_data/Acme/AX-100/manual.pdf")));
			QVERIFY(QFile::exists(base + QStringLiteral("/manufacturer_data/Acme/AX-100/aasx.json")));

				//The command line does the same; no Digital Nameplate, no import
			QCOMPARE(CLIExport::run({QStringLiteral("--import-eplan"), aasx,
						 m_dir.filePath(QStringLiteral("aasx_cli/collection"))}), 0);
			QVERIFY(QFile::exists(m_dir.filePath(QStringLiteral("aasx_cli/3D/Acme/AX-100.stp"))));
			const QString bare = m_dir.filePath(QStringLiteral("bare.aasx"));
			QVERIFY(QetZip::write(bare, {{QStringLiteral("aasx/json/content.json"),
				"{\"submodels\": [{\"idShort\": \"TechnicalData\", \"submodelElements\": []}]}"}}));
			QVERIFY(!importer.importAasx(bare, QStringLiteral("PLC"), collection));
			QVERIFY(!importer.errorString().isEmpty());

				//A nameplate whose manufacturer or order number is ".." writes
				//nothing outside the data folders: it is refused before anything
			for (const auto &names : {std::pair<const char *, const char *>{"..", "AX-100"},
						  std::pair<const char *, const char *>{"Acme", ".."}}) {
				const QByteArray crafted = QByteArray(
					"{\"submodels\": [{\"idShort\": \"Nameplate\", \"submodelElements\": ["
					"{\"modelType\": \"Property\", \"idShort\": \"ManufacturerName\", \"value\": \"")
					+ names.first + "\"},"
					"{\"modelType\": \"Property\", \"idShort\": \"OrderCodeOfManufacturer\", \"value\": \""
					+ names.second + "\"}]}]}";
				const QString root = m_dir.filePath(QStringLiteral("aasx_dots/%1_%2")
								    .arg(qstrlen(names.first)).arg(qstrlen(names.second)));
				QVERIFY(QDir().mkpath(root + QStringLiteral("/inner")));
				const QString evil = root + QStringLiteral("/evil.aasx");
				QVERIFY(QetZip::write(evil, {
					{QStringLiteral("aasx/json/content.json"), crafted},
					{QStringLiteral("aasx/files/AX-100.stp"), "ISO-10303-21;\nEND-ISO-10303-21;\n"},
					{QStringLiteral("aasx/files/manual.pdf"), "%PDF-1.4\n"}}));
				const QString inner = root + QStringLiteral("/inner/collection");
				QVERIFY(!importer.importAasx(evil, QStringLiteral("PLC"), inner));
				QVERIFY(!importer.errorString().isEmpty());
				QStringList files;
				QDirIterator it(root, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
				while (it.hasNext()) files << it.next();
				QCOMPARE(files, QStringList{evil});
			}
		}

		/// A manufacturer's macro package: macros sorted by product family
		/// and the article data in EPLAN parts management XML. Of an
		/// article, the generic element from its part data and one element
		/// per macro variant; its PLC channels from the function definitions
		void macroPackage()
		{
			const QByteArray parts =
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<partsmanagement type=\"EPLAN.PartsManagement\">\n"
				" <part P_ARTICLE_PARTNR=\"ACME.AX200\" P_ARTICLE_MANUFACTURER=\"ACME\""
				" P_ARTICLE_ORDERNR=\"AX200\" P_ARTICLE_TYPENR=\"AX200\""
				" P_ARTICLE_DESCR1=\"en_US@Input module, 2 channels;\""
				" P_ARTICLE_GROUPSYMBOLMACRO=\"$(MD_MACROS)\\Acme\\IO\\Input Modules ACME AXxxxx\\AX200.ema\">\n"
				"  <functiontemplate connectionDesignation=\"1\" connectiondescription=\"DI0\""
				" functiondefcategory=\"300\" functiondefgroup=\"1\" functiondefid=\"2\" idx1=\"Channel 1\"/>\n"
				"  <functiontemplate connectionDesignation=\"2\" connectiondescription=\"DI1\""
				" functiondefcategory=\"300\" functiondefgroup=\"1\" functiondefid=\"2\" idx1=\"Channel 2\"/>\n"
				" </part>\n"
				"</partsmanagement>\n";
			const QString zip = m_dir.filePath(QStringLiteral("macros.zip"));
			QVERIFY(QetZip::write(zip, {
				{QStringLiteral("Acme/Article_DB.xml"), parts},
				{QStringLiteral("Acme/IO/Input Modules ACME AXxxxx/AX200.ema"), smallMacro()},
				{QStringLiteral("Acme/IO/Input Modules ACME AXxxxx/AX300.ema"), smallMacro()}}));

			MacroPackage package;
			QVERIFY2(package.open(zip), qPrintable(package.errorString()));
			QCOMPARE(package.macroCount(), 2);
				// AX300: a macro no article names, offered by its file name
			QCOMPARE(package.articles().size(), 2);
			const MacroPackageArticle *article = package.article(QStringLiteral("ax 200"));
			QVERIFY(article);
			QCOMPARE(article->order_number, QStringLiteral("AX200"));
			QCOMPARE(article->description, QStringLiteral("Input module, 2 channels"));
			QCOMPARE(article->category, QStringLiteral("Input Modules AXxxxx"));
			QCOMPARE(package.find(QStringLiteral("input 2 channels")).size(), 1);
			QVERIFY(!package.macro(*article).isEmpty());

			EplanMacroReader reader;
			QVERIFY(reader.load(package.macro(*article)));
			QCOMPARE(reader.variants().size(), 1);
			QCOMPARE(reader.variants().first().pins, 2);
			QVERIFY(reader.readVariant(1, 0));
			QCOMPARE(reader.symbol().pins.size(), 2);

			const QString collection = m_dir.filePath(QStringLiteral("package"));
			const QString folder = EplanPartImporter::macroPackageFolder(package, *article, QString(), collection);
			EplanPartImporter importer;
			QVERIFY2(importer.importMacroPackage(package, {QStringLiteral("AX200")}, QString(), collection),
				 qPrintable(importer.errorString()));
			QCOMPARE(importer.importedCount(), 1);
			QStringList written = importer.writtenPaths();
			written.sort();
			QCOMPARE(written, (QStringList{folder + QStringLiteral("/AX200.elmt"),
						       folder + QStringLiteral("/AX200_Schematic.elmt")}));
				// the part data names the PLC channels
			const QDomDocument generic = readXml(folder + QStringLiteral("/AX200.elmt"));
			QCOMPARE(generic.elementsByTagName(QStringLiteral("plcIO")).size(), 2);

				// what is there stays, a type that is not in the package fails
			QVERIFY(importer.importMacroPackage(package, {QStringLiteral("AX200"), QStringLiteral("NOPE")},
							    QString(), collection));
			QCOMPARE(importer.importedCount(), 0);
			QCOMPARE(importer.skippedCount(), 1);
			QCOMPARE(importer.failed().size(), 1);

				//The command line: every article of the package
			const QString cli = m_dir.filePath(QStringLiteral("package_cli"));
			QCOMPARE(CLIExport::run({QStringLiteral("--import-eplan"), cli,
						 QStringLiteral("--macro-package"), zip, QStringLiteral("--all")}), 0);
			QVERIFY(QFile::exists(EplanPartImporter::macroPackageFolder(package, *article, QString(), cli)
					      + QStringLiteral("/AX200_Schematic.elmt")));
			QCOMPARE(CLIExport::run({QStringLiteral("--import-eplan"), cli,
						 QStringLiteral("--macro-package"), zip}), 2);
		}

		/// The elements panel has the tab of the macro package
		void macroPackageTab()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			QVERIFY(editor->findChild<MacroPackageTree *>());
			QVERIFY(editor->findChild<QAction *>(QStringLiteral("import_macro_package")));
		}

		/// The editor has the menu entry
		void menuEntry()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			QAction *action = editor->findChild<QAction *>(QStringLiteral("import_part"));
			QVERIFY(action);
			QVERIFY(action->isEnabled());
			QMenu *menu = editor->findChild<QMenu *>(QStringLiteral("part_import_menu"));
			QVERIFY(menu);
			QVERIFY(menu->actions().contains(action));
		}
};

QET_APP_TEST_MAIN(TstEplanPartImport)
#include "tst_eplanpartimport.moc"
