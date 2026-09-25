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

/*
	SpaceMouseHid -- decoding a 3D mouse's raw USB reports, for the backend
	that reads the device directly (Windows, macOS, and Linux without
	spacenavd).

	The descriptors below are written by hand from the HID specification,
	in the shape 3Dconnexion devices use. Recordings from real devices,
	made with misc/spacemouse-capture.py, go in fixtures/spacemouse/ and are
	checked by recordedDevices(): each step says what the user did, so the
	decoded motion must point the right way.
*/

#include "spacemouse/spacemousehid.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

using namespace SpaceMouseHid;

namespace {
	QByteArray hex(const char *text) { return QByteArray::fromHex(text); }

		// Report 1: X, Y, Z; report 2: Rx, Ry, Rz; each 16-bit, -350..350,
		// Input(Data,Var,<rel_or_abs>). Report 3: two buttons and padding.
	QByteArray classicDescriptor(bool relative)
	{
		const char *input = relative ? "8106" : "8102";
		return hex(QByteArray("05010908a101"
			"a100" "8501" "16a2fe" "265e01" "093009310932" "7510" "9503") + input + "c0"
			"a100" "8502" "093309340935" "7510" "9503" + input + "c0"
			"a102" "8503" "0509" "1901" "2902" "1500" "2501" "7501" "9502" "8102"
			"950e" "8103" "c0"
			"c0");
	}

	QByteArray report(int id, std::initializer_list<qint16> values)
	{
		QByteArray r(1, char(id));
		for (qint16 v : values) {
			r.append(char(v & 0xff));
			r.append(char((v >> 8) & 0xff));
		}
		return r;
	}
}

class TstSpaceMouseHid : public QObject
{
		Q_OBJECT

	private slots:
		void recognisesOnly3DMice_data();
		void recognisesOnly3DMice();
		void parsesTheClassicDescriptor();
		void parsesADescriptorWithoutReportIds();
		void survivesBrokenDescriptors_data();
		void survivesBrokenDescriptors();
		void separateTranslationAndRotationReports();
		void combinedReportOnTheFallbackLayout();
		void absoluteAxesAreScaledLikeSpacenavd();
		void relativeAxesArePassedThrough();
		void buttonBitmaskReportsEachPressOnce();
		void buttonListReport();
		void ignoresShortAndUnknownReports();
		void recordedDevices_data();
		void recordedDevices();
};

void TstSpaceMouseHid::recognisesOnly3DMice_data()
{
	QTest::addColumn<int>("vendor");
	QTest::addColumn<int>("product");
	QTest::addColumn<int>("page");
	QTest::addColumn<int>("usage");
	QTest::addColumn<bool>("match");
	QTest::newRow("3Dconnexion, multi-axis") << 0x256f << 0xc635 << 1 << 8 << true;
	QTest::newRow("3Dconnexion, usage unknown") << 0x256f << 0xc652 << 0 << 0 << true;
	QTest::newRow("3Dconnexion receiver's mouse interface") << 0x256f << 0xc652 << 1 << 2 << false;
	QTest::newRow("Logitech SpaceNavigator, usage unknown") << 0x046d << 0xc626 << 0 << 0 << true;
	QTest::newRow("Logitech, multi-axis") << 0x046d << 0xc629 << 1 << 8 << true;
	QTest::newRow("Logitech receiver, usage unknown") << 0x046d << 0xc52b << 0 << 0 << false;
	QTest::newRow("other vendor, multi-axis") << 0x045e << 0x0001 << 1 << 8 << false;
}

void TstSpaceMouseHid::recognisesOnly3DMice()
{
	QFETCH(int, vendor);
	QFETCH(int, product);
	QFETCH(int, page);
	QFETCH(int, usage);
	QFETCH(bool, match);
	QCOMPARE(isSpaceMouse(vendor, product, page, usage), match);
}

void TstSpaceMouseHid::parsesTheClassicDescriptor()
{
	const Layout layout = parseDescriptor(classicDescriptor(true));
	QVERIFY(layout.numbered_reports);
	QVERIFY(layout.hasAxes());
	for (int a = 0; a < 6; ++a) {
		const Field &f = layout.axes[a];
		QCOMPARE(f.report_id, a < 3 ? 1 : 2);
		QCOMPARE(f.bit_offset, (a % 3) * 16);
		QCOMPARE(f.bit_size, 16);
		QCOMPARE(f.logical_min, -350);
		QCOMPARE(f.logical_max, 350);
		QVERIFY(f.relative);
	}
	QCOMPARE(layout.buttons.size(), 2);
	QCOMPARE(layout.buttons[0].report_id, 3);
	QCOMPARE(layout.buttons[0].bit_offset, 0);
	QCOMPARE(layout.buttons[1].bit_offset, 1);
	QCOMPARE(layout.buttons[1].bit_size, 1);

	QVERIFY(!parseDescriptor(classicDescriptor(false)).axes[0].relative);
}

void TstSpaceMouseHid::parsesADescriptorWithoutReportIds()
{
		// one report: X, Y, Z as 8-bit signed, then 8 buttons
	const QByteArray d = hex("05010908a101" "1581" "257f" "093009310932" "7508" "9503" "8106"
				 "0509" "1901" "2908" "1500" "2501" "7501" "9508" "8102" "c0");
	const Layout layout = parseDescriptor(d);
	QVERIFY(!layout.numbered_reports);
	QCOMPARE(layout.axes[2].bit_offset, 16);
	QCOMPARE(layout.buttons.size(), 8);
	QCOMPARE(layout.buttons[7].bit_offset, 24 + 7);

	Decoder decoder(layout);
	const Decoder::Result r = decoder.feed(hex("05fb7f80"));	// 5, -5, 127, button 8
	QVERIFY(r.motion);
	QCOMPARE(r.sample.x, 5);
	QCOMPARE(r.sample.y, -5);
	QCOMPARE(r.sample.z, 127);
	QCOMPARE(r.pressed, QList<int>{7});
}

void TstSpaceMouseHid::survivesBrokenDescriptors_data()
{
	QTest::addColumn<QByteArray>("descriptor");
	QTest::newRow("empty") << QByteArray();
	QTest::newRow("truncated item") << hex("0501090816");
	QTest::newRow("long item, truncated") << hex("fe");
	QTest::newRow("pop with empty stack") << hex("b4b4b4");
	QTest::newRow("huge report count") << hex("05010930" "7510" "97ffffff7f" "8102");
	QTest::newRow("classic, cut in half") << classicDescriptor(true).left(30);
}

void TstSpaceMouseHid::survivesBrokenDescriptors()
{
	QFETCH(QByteArray, descriptor);
	QElapsedTimer timer;
	timer.start();
	const Layout layout = parseDescriptor(descriptor);
	QVERIFY2(timer.elapsed() < 500, "a broken descriptor must not stall the parser");
	Decoder decoder(layout);
	decoder.feed(report(1, {1, 2, 3}));		// must not crash or read out of bounds
}

void TstSpaceMouseHid::separateTranslationAndRotationReports()
{
	Decoder decoder(parseDescriptor(classicDescriptor(true)));

	Decoder::Result r = decoder.feed(report(1, {100, -5, 20}));
	QVERIFY(r.motion);
	QCOMPARE(r.sample.x, 100);
	QCOMPARE(r.sample.y, -5);
	QCOMPARE(r.sample.z, 20);
	QCOMPARE(r.sample.rz, 0);

	r = decoder.feed(report(2, {7, 8, -9}));
	QVERIFY(r.motion);
	QCOMPARE(r.sample.x, 100);		// translation remembered
	QCOMPARE(r.sample.rx, 7);
	QCOMPARE(r.sample.ry, 8);
	QCOMPARE(r.sample.rz, -9);
}

void TstSpaceMouseHid::combinedReportOnTheFallbackLayout()
{
	Decoder decoder;
	const Decoder::Result r = decoder.feed(report(1, {1, 2, 3, 4, 5, 6}));
	QVERIFY(r.motion);
	QCOMPARE(r.sample.x, 1);
	QCOMPARE(r.sample.z, 3);
	QCOMPARE(r.sample.rx, 4);
	QCOMPARE(r.sample.rz, 6);
}

void TstSpaceMouseHid::absoluteAxesAreScaledLikeSpacenavd()
{
	Decoder decoder(parseDescriptor(classicDescriptor(false)));
	const Decoder::Result r = decoder.feed(report(1, {350, -350, 0}));
	QCOMPARE(r.sample.x, 500);
	QCOMPARE(r.sample.y, -500);
	QCOMPARE(r.sample.z, 0);
}

void TstSpaceMouseHid::relativeAxesArePassedThrough()
{
	Decoder decoder(parseDescriptor(classicDescriptor(true)));
	const Decoder::Result r = decoder.feed(report(1, {350, -350, 0}));
	QCOMPARE(r.sample.x, 350);
	QCOMPARE(r.sample.y, -350);
}

void TstSpaceMouseHid::buttonBitmaskReportsEachPressOnce()
{
	Decoder decoder;
	QCOMPARE(decoder.feed(hex("0301000000")).pressed, QList<int>{0});
	QCOMPARE(decoder.feed(hex("0301000000")).pressed, QList<int>{});		// still held
	QCOMPARE(decoder.feed(hex("0303000000")).pressed, QList<int>{1});
	QCOMPARE(decoder.feed(hex("0300000000")).pressed, QList<int>{});		// released
	QCOMPARE(decoder.feed(hex("0302000000")).pressed, QList<int>{1});
	QCOMPARE(decoder.feed(hex("0300010000")).pressed, QList<int>{8});
	QVERIFY(!decoder.feed(hex("0300000000")).motion);
}

void TstSpaceMouseHid::buttonListReport()
{
	Decoder decoder;
		// numbers start at 1 (0 = none); reported from 0, like the others
	QCOMPARE(decoder.feed(hex("1c0d000000")).pressed, QList<int>{12});
	QCOMPARE(decoder.feed(hex("1c0d000e00")).pressed, QList<int>{13});
	QCOMPARE(decoder.feed(hex("1c00000000")).pressed, QList<int>{});
	QCOMPARE(decoder.feed(hex("1c0d00")).pressed, QList<int>{12});
	QCOMPARE(decoder.feed(hex("1c01000000")).pressed, QList<int>{0});	// the lowest number, 1

}

void TstSpaceMouseHid::ignoresShortAndUnknownReports()
{
	Decoder decoder(parseDescriptor(classicDescriptor(true)));
	QVERIFY(!decoder.feed(QByteArray()).motion);
	QVERIFY(!decoder.feed(hex("01ab")).motion);
	QVERIFY(!decoder.feed(hex("55010203040506")).motion);
	QVERIFY(decoder.feed(hex("55010203040506")).pressed.isEmpty());
}

void TstSpaceMouseHid::recordedDevices_data()
{
	QTest::addColumn<QString>("path");
	const QDir dir(QFINDTESTDATA("fixtures/spacemouse"));
	const QStringList files = dir.exists()
			? dir.entryList({QStringLiteral("*.json")}, QDir::Files)
			: QStringList();
	for (const QString &f : files) {
		QTest::newRow(qPrintable(f)) << dir.filePath(f);
	}
	if (files.isEmpty()) {
		QTest::newRow("none") << QString();
	}
}

/*
	For each recording, the axis the user moved must be the one that moved
	most, in the right direction. The expected signs are the raw USB
	convention as Blender documents it for Windows, and match the Linux
	kernel's (so spacenavd's defaults): +x right, +y towards the user,
	+z down. A recording that disagrees is a finding about that device.
*/
void TstSpaceMouseHid::recordedDevices()
{
	QFETCH(QString, path);
	if (path.isEmpty()) {
		QSKIP("no recordings in fixtures/spacemouse yet");
	}

	QFile file(path);
	QVERIFY(file.open(QIODevice::ReadOnly));
	const QJsonObject capture = QJsonDocument::fromJson(file.readAll()).object();
	const QByteArray descriptor = QByteArray::fromHex(
			capture.value("report_descriptor").toString().toLatin1());
	Layout layout = parseDescriptor(descriptor);
	if (!layout.hasAxes()) {
		layout = fallbackLayout();
	}

	struct Expect { const char *step; int axis; int sign; };
	const Expect expectations[] = {
		{"right", 0, +1}, {"left", 0, -1},
		{"toward", 1, +1}, {"away", 1, -1},
		{"down", 2, +1}, {"up", 2, -1},
	};

	QHash<QString, QJsonArray> steps;
	for (const QJsonValue &s : capture.value("steps").toArray()) {
		steps.insert(s.toObject().value("step").toString(),
			     s.toObject().value("reports").toArray());
	}

	for (const Expect &e : expectations)
	{
		if (!steps.contains(e.step)) {
			continue;
		}
		Decoder decoder(layout);
		qint64 sum[6] = {0, 0, 0, 0, 0, 0};
		for (const QJsonValue &r : steps.value(e.step)) {
			const Decoder::Result res = decoder.feed(
					QByteArray::fromHex(r.toArray().at(1).toString().toLatin1()));
			if (res.motion) {
				const int v[6] = {res.sample.x, res.sample.y, res.sample.z,
						  res.sample.rx, res.sample.ry, res.sample.rz};
				for (int a = 0; a < 3; ++a) sum[a] += v[a];
			}
		}
		int strongest = 0;
		for (int a = 1; a < 3; ++a) {
			if (qAbs(sum[a]) > qAbs(sum[strongest])) strongest = a;
		}
		QVERIFY2(strongest == e.axis && (sum[e.axis] > 0) == (e.sign > 0),
			 qPrintable(QStringLiteral("step '%1': sums x=%2 y=%3 z=%4")
				    .arg(e.step).arg(sum[0]).arg(sum[1]).arg(sum[2])));
	}

		// every recorded button press must decode as a press
	if (steps.contains("buttons")) {
		Decoder decoder(layout);
		int presses = 0;
		for (const QJsonValue &r : steps.value("buttons")) {
			presses += decoder.feed(QByteArray::fromHex(
					r.toArray().at(1).toString().toLatin1())).pressed.size();
		}
		QVERIFY2(presses > 0, "the buttons step decoded no presses");
	}
}

QTEST_GUILESS_MAIN(TstSpaceMouseHid)
#include "tst_spacemousehid.moc"
