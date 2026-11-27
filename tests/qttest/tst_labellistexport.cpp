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

#include "conductornumexport.h"
#include "labellistexport.h"
#include "qetproject.h"

#include <QCheckBox>
#include <QFile>
#include <QLabel>
#include <QRadioButton>

/**
	Label lists for printers (#1878 A7): device tags, terminal markers,
	cable and wire numbers, one per line, read from the project database.
	The folios are left unbuilt, as a project opens.
*/
class TstLabelListExport : public QObject
{
	Q_OBJECT

	private slots:
		void initTestCase()
		{
			QETProject::setBackupEnabled(false);
		}

		/// One tag per device, folio after folio, column after column on a
		/// folio; no terminal among them, they have their own list
		void devicesInFolioOrder()
		{
			QETProject project(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
			QVERIFY(project.state() == QETProject::Ok);
			const QStringList list = LabelListExport::labels(&project, LabelListExport::Devices, false);
			QVERIFY(!list.isEmpty());
			QCOMPARE(QStringList(list).removeDuplicates(), 0);
				//Folio 4: 4SL in G3, 4Q3 in C4, 4EH in G4, 4Q5 in E7, 4Q1 in A12
			const QStringList folio4 {"4SL", "4Q3", "4EH", "4Q5", "4Q1"};
			for (int i = 1 ; i < folio4.size() ; ++i) {
				QVERIFY2(list.indexOf(folio4.at(i - 1)) < list.indexOf(folio4.at(i)),
						 qPrintable(folio4.at(i - 1) + " after " + folio4.at(i)));
			}
			for (const QString &terminal : {"XPE", "XE:1", "X2:1"}) {
				QVERIFY2(!list.contains(terminal), qPrintable(terminal));
			}

			const QStringList full = LabelListExport::labels(&project, LabelListExport::Devices, true);
			QCOMPARE(full.size(), list.size());
			QVERIFY(full.contains(QStringLiteral("+-IGC-4Q1")));
		}

		/// A marker per terminal; those on no strip by label, 2 before 10
		void looseTerminalsByLabel()
		{
			QETProject project(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
			QVERIFY(project.state() == QETProject::Ok);
			const QStringList list = LabelListExport::labels(&project, LabelListExport::Terminals, false);
			QCOMPARE(list.size(), 96);
			QCOMPARE(list.first(), QStringLiteral("X2:1"));
			QVERIFY(list.indexOf("X2:9") < list.indexOf("X2:10"));
			QVERIFY(list.indexOf("XE:4") < list.indexOf("XPE"));
		}

		/// The terminals of a strip, in strip order; one with no label has
		/// nothing to print
		void stripTerminals()
		{
			QETProject project(QStringLiteral(QET_FIXTURES_DIR "/every_feature.qet"));
			QVERIFY(project.state() == QETProject::Ok);
			QCOMPARE(LabelListExport::labels(&project, LabelListExport::Terminals, false),
					 QStringList{"KXPlant 0Cabinet 0E5F12202002b"});
			QCOMPARE(LabelListExport::labels(&project, LabelListExport::Terminals, true),
					 QStringList{"=Plant 0+Cabinet 0-KXPlant 0Cabinet 0E5F12202002b"});
		}

		void cables()
		{
			QETProject project(QStringLiteral(QET_FIXTURES_DIR "/cable_part_number.qet"));
			QVERIFY(project.state() == QETProject::Ok);
			QCOMPARE(LabelListExport::labels(&project, LabelListExport::Cables, false), QStringList{"W1"});
			QCOMPARE(LabelListExport::labels(&project, LabelListExport::Cables, true), QStringList{"-W1"});
		}

		/// The same numbers as "Export the list of names of wires", 2 before
		/// 10; a wire has no full designation
		void wiresAsTheWireExport()
		{
			QETProject project(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
			QVERIFY(project.state() == QETProject::Ok);
			const QStringList list = LabelListExport::labels(&project, LabelListExport::Wires, true);
			QVERIFY(!list.isEmpty());
			QStringList same = ConductorNumExport(&project).wiresNum().split('\n', Qt::SkipEmptyParts);
			QStringList sorted = list;
			same.sort();
			sorted.sort();
			QCOMPARE(sorted, same);
				//In natural order: 2 before 10
				//(a key with the numbers padded, not QCollator: that depends on the locale)
			const auto key = [](const QString &label) {
				QString out;
				static const QRegularExpression digits(QStringLiteral("\\d+"));
				int at = 0;
				for (auto m = digits.globalMatch(label); m.hasNext();) {
					const auto hit = m.next();
					out += label.mid(at, hit.capturedStart() - at).toCaseFolded()
						+ hit.captured().rightJustified(12, QLatin1Char('0'));
					at = hit.capturedEnd();
				}
				return out + label.mid(at).toCaseFolded();
			};
			for (int i = 1 ; i < list.size() ; ++i) {
				QVERIFY2(key(list.at(i - 1)) <= key(list.at(i)),
						 qPrintable(list.at(i - 1) + " before " + list.at(i)));
			}
		}

		/// The dialog counts the list picked; a wire has no full designation
		void dialog()
		{
			QETProject project(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
			QVERIFY(project.state() == QETProject::Ok);
			LabelListExportDialog dialog(&project);
			const QList<QRadioButton *> kinds = dialog.findChildren<QRadioButton *>();
			QCOMPARE(kinds.size(), 4);
			QCheckBox *full = dialog.findChild<QCheckBox *>();
			QVERIFY(full);
			QLabel *count = nullptr;
			for (QLabel *label : dialog.findChildren<QLabel *>())
				if (label->text().contains(QStringLiteral("label")) && label->text().contains(QStringLiteral("first")))
					count = label;
			QVERIFY(count);

			kinds.at(1)->click();
			QVERIFY(count->text().startsWith(QStringLiteral("96 ")));
			QVERIFY(count->text().endsWith(QStringLiteral("X2:1")));
			QVERIFY(full->isEnabled());
			kinds.at(3)->click();
			QVERIFY(!full->isEnabled());
			kinds.at(0)->click();
			full->setChecked(true);
			QVERIFY(count->text().contains(QStringLiteral("+-IGC-")));
			if (!qEnvironmentVariableIsEmpty("QET_TEST_GRAB")) {
				full->setChecked(false);
				dialog.show();
				QVERIFY(QTest::qWaitForWindowExposed(&dialog));
				dialog.grab().save(qEnvironmentVariable("QET_TEST_GRAB"));
			}
		}

		void csvQuotesWhatItMust()
		{
			QCOMPARE(LabelListExport::toCsv({"K1", "a;b", "a,b", "say \"hi\""}),
					 QByteArray("\"K1\"\n\"a;b\"\n\"a,b\"\n\"say \"\"hi\"\"\"\n"));
		}

		void writesUtf8WithByteOrderMark()
		{
			QTemporaryDir dir;
			const QString name = dir.filePath(QStringLiteral("labels.csv"));
			QVERIFY(LabelListExport::write({QStringLiteral("–K1"), QStringLiteral("X1:2")}, name));
			QFile file(name);
			QVERIFY(file.open(QIODevice::ReadOnly));
			QCOMPARE(file.readAll(), QByteArray("\xEF\xBB\xBF") + QStringLiteral("\"–K1\"\n\"X1:2\"\n").toUtf8());
		}

		void kindNames()
		{
			for (const auto kind : {LabelListExport::Devices, LabelListExport::Terminals,
									LabelListExport::Cables, LabelListExport::Wires}) {
				LabelListExport::Kind read = LabelListExport::Wires;
				QVERIFY(LabelListExport::kindFromString(LabelListExport::kindName(kind), &read));
				QCOMPARE(read, kind);
			}
			LabelListExport::Kind read;
			QVERIFY(!LabelListExport::kindFromString(QStringLiteral("symbols"), &read));
		}
};

QET_APP_TEST_MAIN(TstLabelListExport)
#include "tst_labellistexport.moc"
