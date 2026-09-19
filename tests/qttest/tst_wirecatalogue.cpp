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

#include "custom/wirecatalogue/wirecataloguedb.h"
#include "custom/wirecatalogue/wirespec.h"
#include "custom/wirecatalogue/iec60757.h"

/**
	The wire/cable catalogue's storage layer: CRUD, search, CSV round trip,
	multi-core cables, and the IEC 60757 colour table.

	Everything here runs headless against a database in a temporary folder —
	the catalogue's own default path is the user's data directory, which a
	test must not touch.

	Two of these cases are regressions with a history. The search once looked
	only at the text columns, so a wire recorded as 2.5 mm² was unfindable by
	typing "2.5" unless its name happened to contain it; and Green-Yellow
	carried the same RGB as plain Green, which made a protective conductor
	indistinguishable from a green one. Both are asserted below rather than
	left to be rediscovered.
*/
class tst_wirecatalogue : public QObject
{
	Q_OBJECT

	private:
		QTemporaryDir m_dir;
		QString dbPath(const char *name) const
		{
			return m_dir.path() + QStringLiteral("/") + QString::fromLatin1(name);
		}

		static WireSpec wire(const QString &id, double section = 1.5,
							 const QString &colour = QStringLiteral("Black"))
		{
			WireSpec w;
			w.wireId = id;
			w.crossSectionMm2 = section;
			w.colorPrimary = colour;
			w.manufacturerName = QStringLiteral("Acme");
			return w;
		}

	private slots:
		void initTestCase()
		{
			QVERIFY2(m_dir.isValid(), "could not create a temporary directory");
		}

		/** A fresh database opens, creates its schema and seeds itself. */
		void opensAndSeeds()
		{
			WireCatalogueDb db;
			QVERIFY2(db.open(dbPath("seed.sqlite")), qPrintable(db.lastError()));
			QVERIFY(db.isOpen());
			QVERIFY2(db.count() > 0, "a new catalogue should seed itself with common wires");
		}

		/** Add, read back, update, delete. */
		void crudRoundTrip()
		{
			WireCatalogueDb db;
			QVERIFY2(db.open(dbPath("crud.sqlite")), qPrintable(db.lastError()));
			const int before = db.count();

			QVERIFY(db.addWire(wire(QStringLiteral("TST-1"), 2.5, QStringLiteral("Blue"))));
			QCOMPARE(db.count(), before + 1);
			QVERIFY(db.contains(QStringLiteral("TST-1")));

			WireSpec got = db.wire(QStringLiteral("TST-1"));
			QVERIFY(got.isValid());
			QCOMPARE(got.crossSectionMm2, 2.5);
			QCOMPARE(got.colorPrimary, QStringLiteral("Blue"));

			got.manufacturerName = QStringLiteral("Changed");
			QVERIFY(db.updateWire(got));
			QCOMPARE(db.wire(QStringLiteral("TST-1")).manufacturerName,
					 QStringLiteral("Changed"));

			QVERIFY(db.removeWire(QStringLiteral("TST-1")));
			QVERIFY(!db.contains(QStringLiteral("TST-1")));
			QCOMPARE(db.count(), before);
		}

		/** A duplicate id is refused rather than silently overwriting. */
		void addRefusesDuplicateId()
		{
			WireCatalogueDb db;
			QVERIFY2(db.open(dbPath("dup.sqlite")), qPrintable(db.lastError()));
			QVERIFY(db.addWire(wire(QStringLiteral("DUP-1"))));
			QVERIFY2(!db.addWire(wire(QStringLiteral("DUP-1"))),
					 "adding an existing wire id must fail");
			QVERIFY2(!db.lastError().isEmpty(),
					 "a refused insert should say why");
		}

		/** upsert is the one that is allowed to replace. */
		void upsertReplaces()
		{
			WireCatalogueDb db;
			QVERIFY2(db.open(dbPath("upsert.sqlite")), qPrintable(db.lastError()));
			QVERIFY(db.addWire(wire(QStringLiteral("UP-1"), 1.5)));
			const int n = db.count();
			QVERIFY(db.upsertWire(wire(QStringLiteral("UP-1"), 6.0)));
			QCOMPARE(db.count(), n);
			QCOMPARE(db.wire(QStringLiteral("UP-1")).crossSectionMm2, 6.0);
		}

		/** Search matches the text columns. */
		void searchMatchesText()
		{
			WireCatalogueDb db;
			QVERIFY2(db.open(dbPath("search1.sqlite")), qPrintable(db.lastError()));
			WireSpec w = wire(QStringLiteral("SRCH-ID"), 1.0, QStringLiteral("Red"));
			w.manufacturerName = QStringLiteral("Lapp");
			QVERIFY(db.addWire(w));

			auto ids = [](const QVector<WireSpec> &v) {
				QStringList out;
				for (const WireSpec &s : v) out << s.wireId;
				return out;
			};
			QVERIFY(ids(db.search(QStringLiteral("SRCH"))).contains(QStringLiteral("SRCH-ID")));
			QVERIFY(ids(db.search(QStringLiteral("Lapp"))).contains(QStringLiteral("SRCH-ID")));
			QVERIFY(ids(db.search(QStringLiteral("Red"))).contains(QStringLiteral("SRCH-ID")));
		}

		/**
			Regression: searching a cross-section found only wires whose *name*
			contained the number. A wire recorded as 2.5 mm² under a name that
			says nothing about it was invisible.
		*/
		void searchMatchesCrossSection()
		{
			WireCatalogueDb db;
			QVERIFY2(db.open(dbPath("search2.sqlite")), qPrintable(db.lastError()));
			QVERIFY(db.addWire(wire(QStringLiteral("NAMELESS"), 2.5)));

			bool found = false;
			for (const WireSpec &s : db.search(QStringLiteral("2.5")))
				if (s.wireId == QStringLiteral("NAMELESS")) found = true;
			QVERIFY2(found, "a 2.5 mm² wire must be findable by searching \"2.5\", "
							"even when its id does not contain the number");
		}

		/** Regression: the same, for the number of cores. */
		void searchMatchesCoreCount()
		{
			WireCatalogueDb db;
			QVERIFY2(db.open(dbPath("search3.sqlite")), qPrintable(db.lastError()));
			WireSpec c = wire(QStringLiteral("BUNDLE"), 0.75);
			c.numCores = 7;
			QVERIFY(db.addWire(c));

			bool found = false;
			for (const WireSpec &s : db.search(QStringLiteral("7")))
				if (s.wireId == QStringLiteral("BUNDLE")) found = true;
			QVERIFY2(found, "a seven-core cable must be findable by searching \"7\"");
		}

		/** An empty search is not a filter: it returns everything. */
		void emptySearchReturnsAll()
		{
			WireCatalogueDb db;
			QVERIFY2(db.open(dbPath("search4.sqlite")), qPrintable(db.lastError()));
			QCOMPARE(db.search(QString()).size(), db.allWires().size());
			QCOMPARE(db.search(QStringLiteral("   ")).size(), db.allWires().size());
		}

		/** A multi-core cable keeps its per-core colours and sections. */
		void multiCoreCableRoundTrip()
		{
			WireCatalogueDb db;
			QVERIFY2(db.open(dbPath("cores.sqlite")), qPrintable(db.lastError()));

			WireSpec c = wire(QStringLiteral("CAB-4"), 1.5);
			c.numCores = 4;
			c.coreColors = {{QStringLiteral("Brown")}, {QStringLiteral("Black")},
							{QStringLiteral("Grey")},  {QStringLiteral("Blue")}};
			c.coreSections = {1.5, 1.5, 1.5, 0.75};
			c.hasShield = true;
			c.shieldType = QStringLiteral("Braid");
			QVERIFY(db.addWire(c));

			const WireSpec got = db.wire(QStringLiteral("CAB-4"));
			QVERIFY(got.isCable());
			QCOMPARE(got.numCores, 4);
			QCOMPARE(got.coreColors.size(), 4);
			QCOMPARE(got.coreColors.at(0).value(0), QStringLiteral("Brown"));
			QCOMPARE(got.coreColors.at(3).value(0), QStringLiteral("Blue"));
			QVERIFY(got.hasShield);
			QCOMPARE(got.shieldType, QStringLiteral("Braid"));

				//A core with its own section keeps it; one without falls back
				//to the cable's nominal section.
			QCOMPARE(got.coreSection(3), 0.75);
			QCOMPARE(got.coreSection(0), 1.5);
		}

		/** A single wire is not a cable, whatever else it carries. */
		void singleWireIsNotACable()
		{
			WireSpec w = wire(QStringLiteral("ONE"));
			QVERIFY(!w.isCable());
			QCOMPARE(w.numCores, 1);
		}

		/** Export then import into a second database reproduces the rows. */
		void csvRoundTrip()
		{
			const QString csv = m_dir.path() + QStringLiteral("/catalogue.csv");

			WireCatalogueDb src;
			QVERIFY2(src.open(dbPath("csv-src.sqlite")), qPrintable(src.lastError()));
			WireSpec c = wire(QStringLiteral("CSV-CAB"), 1.0);
			c.numCores = 3;
			c.coreColors = {{QStringLiteral("Brown")}, {QStringLiteral("Blue")},
							{QStringLiteral("Green-Yellow")}};
			QVERIFY(src.addWire(c));
			QVERIFY(src.addWire(wire(QStringLiteral("CSV-WIRE"), 4.0,
									 QStringLiteral("Grey"))));

			const int exported = src.exportCsv(csv);
			QVERIFY2(exported >= 2, qPrintable(QStringLiteral("exported %1 rows: %2")
											   .arg(exported).arg(src.lastError())));
			QVERIFY(QFile::exists(csv));

			WireCatalogueDb dst;
			QVERIFY2(dst.open(dbPath("csv-dst.sqlite")), qPrintable(dst.lastError()));
			const int imported = dst.importCsv(csv);
			QVERIFY2(imported >= 2, qPrintable(QStringLiteral("imported %1 rows: %2")
											   .arg(imported).arg(dst.lastError())));

			QVERIFY(dst.contains(QStringLiteral("CSV-WIRE")));
			QVERIFY(dst.contains(QStringLiteral("CSV-CAB")));
			QCOMPARE(dst.wire(QStringLiteral("CSV-WIRE")).crossSectionMm2, 4.0);

			const WireSpec cab = dst.wire(QStringLiteral("CSV-CAB"));
			QCOMPARE(cab.numCores, 3);
			QCOMPARE(cab.coreColors.value(2).value(0), QStringLiteral("Green-Yellow"));
		}

		/** Importing the same file twice must not duplicate rows. */
		void csvImportIsIdempotent()
		{
			const QString csv = m_dir.path() + QStringLiteral("/idem.csv");

			WireCatalogueDb src;
			QVERIFY2(src.open(dbPath("idem-src.sqlite")), qPrintable(src.lastError()));
			QVERIFY(src.addWire(wire(QStringLiteral("IDEM-1"))));
			QVERIFY(src.exportCsv(csv) > 0);

			WireCatalogueDb dst;
			QVERIFY2(dst.open(dbPath("idem-dst.sqlite")), qPrintable(dst.lastError()));
			dst.importCsv(csv);
			const int after_first = dst.count();
			dst.importCsv(csv);
			QCOMPARE(dst.count(), after_first);
		}

		/** Every standard colour name resolves to a usable colour. */
		void iecColourNamesResolve()
		{
			const QStringList names = Iec60757::standardNames();
			QVERIFY2(!names.isEmpty(), "the IEC 60757 table should not be empty");
			for (const QString &n : names) {
				const QColor c = Iec60757::colorForName(n);
				QVERIFY2(c.isValid(), qPrintable(QStringLiteral("no colour for %1").arg(n)));
			}
			QVERIFY(!Iec60757::colorForName(QStringLiteral("NotAColour")).isValid());
		}

		/**
			Regression: Green-Yellow returned the same RGB as plain Green, so a
			protective conductor was drawn identically to a green one.
		*/
		void greenYellowIsNotGreen()
		{
			const QColor gnye = Iec60757::colorForName(QStringLiteral("Green-Yellow"));
			const QColor gn   = Iec60757::colorForName(QStringLiteral("Green"));
			QVERIFY(gnye.isValid());
			QVERIFY(gn.isValid());
			QVERIFY2(gnye != gn,
					 "Green-Yellow must not resolve to the same colour as Green");
			QVERIFY2(Iec60757::isGreenYellow(QStringLiteral("Green-Yellow")),
					 "isGreenYellow() should recognise the standard name");
			QVERIFY(!Iec60757::isGreenYellow(QStringLiteral("Green")));
		}

		/** Reopening a database keeps what was stored. */
		void dataSurvivesReopen()
		{
			const QString path = dbPath("persist.sqlite");
			{
				WireCatalogueDb db;
				QVERIFY2(db.open(path), qPrintable(db.lastError()));
				QVERIFY(db.addWire(wire(QStringLiteral("KEEP-1"), 10.0)));
			}
			WireCatalogueDb again;
			QVERIFY2(again.open(path), qPrintable(again.lastError()));
			QVERIFY(again.contains(QStringLiteral("KEEP-1")));
			QCOMPARE(again.wire(QStringLiteral("KEEP-1")).crossSectionMm2, 10.0);
		}

		/**
			A note with a line break in it survives export and re-import.
			csvEscape() already quotes such a field, so the exported file is
			valid CSV; the importer has to read records rather than lines or
			the record is torn in half and everything after it shifts.
		*/
		void csvRoundTripsEmbeddedNewline()
		{
			const QString csv = dbPath("multiline.csv");
			const QString note = QStringLiteral("first line\nsecond line, with a comma\n\"quoted\"");
			{
				WireCatalogueDb db;
				QVERIFY2(db.open(dbPath("ml1.sqlite")), qPrintable(db.lastError()));
				WireSpec w = wire(QStringLiteral("ML-1"), 1.5);
				w.notes = note;
				QVERIFY(db.addWire(w));
				WireSpec after = wire(QStringLiteral("ML-2"), 4.0);
				QVERIFY(db.addWire(after));
				QVERIFY(db.exportCsv(csv) >= 2);
			}
			WireCatalogueDb db2;
			QVERIFY2(db2.open(dbPath("ml2.sqlite")), qPrintable(db2.lastError()));
			QVERIFY(db2.importCsv(csv) > 0);
			QCOMPARE(db2.wire(QStringLiteral("ML-1")).notes, note);
			QVERIFY2(db2.contains(QStringLiteral("ML-2")),
					 "the row after a multi-line note must still import");
		}

		/** Asking for a wire that is not there is empty, not a crash. */
		void unknownWireIsEmpty()
		{
			WireCatalogueDb db;
			QVERIFY2(db.open(dbPath("unknown.sqlite")), qPrintable(db.lastError()));
			QVERIFY(!db.contains(QStringLiteral("NOPE")));
			QVERIFY(!db.wire(QStringLiteral("NOPE")).isValid());
			QVERIFY(!db.removeWire(QStringLiteral("NOPE")));
		}
};

QTEST_MAIN(tst_wirecatalogue)
#include "tst_wirecatalogue.moc"
