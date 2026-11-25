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

#include "ElementsCollection/librarylisting.h"
#include "ElementsCollection/qetlabelsfile.h"

#include <QDirIterator>
#include <QFile>

/**
	The library listing (#1878 A8): a row per symbol and macro of a
	collection folder, with its terminal names and class letter.
*/
class TstLibraryListing : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	void writeFile(const QString &relative, const QByteArray &content)
	{
		const QString path = m_dir.filePath(relative);
		QDir().mkpath(QFileInfo(path).absolutePath());
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly));
		file.write(content);
	}

	private slots:
		/// A small library: a labels file, a symbol in a folder it names,
		/// in a sub-folder it names without a letter, in one it does not
		/// name, a macro, a broken file
		void initTestCase()
		{
			QVERIFY(m_dir.isValid());
			writeFile(QStringLiteral("lib/qet_labels.xml"),
					  "<labels><category name=\"relays\"><category name=\"10_more\"></category><prefix>K</prefix></category>"
					  "<category name=\"plain\"></category></labels>");
			writeFile(QStringLiteral("lib/relays/coil.elmt"),
					  "<definition type=\"element\" link_type=\"master\"><names>"
					  "<name lang=\"en\">Coil</name><name lang=\"fr\">Bobine</name></names>"
					  "<description><terminal name=\"A1\"/><terminal name=\"A2\"/></description></definition>");
			writeFile(QStringLiteral("lib/relays/10_more/contact.elmt"),
					  "<definition type=\"element\" link_type=\"slave\"><names>"
					  "<name lang=\"en\">Contact, NO</name></names>"
					  "<description><terminal name=\"13\"/><terminal name=\"\"/></description></definition>");
			writeFile(QStringLiteral("lib/relays/2_less/lamp.elmt"),
					  "<definition type=\"element\"><names><name lang=\"en\">Lamp</name></names>"
					  "<description/></definition>");
			writeFile(QStringLiteral("lib/plain/broken.elmt"), "<definition");
			writeFile(QStringLiteral("lib/relays 2/other.elmt"),
					  "<definition type=\"element\"><names><name lang=\"en\">Other</name></names></definition>");
			writeFile(QStringLiteral("lib/plain/starter.qetmak"),
					  "<qet_macro><collection><element path=\"arrows/go.elmt\">"
					  "<definition link_type=\"next_report\"/></element></collection>"
					  "<diagram_content><diagram>"
					  "<element type=\"macro://arrows/go.elmt\"><elementInformations>"
					  "<elementInformation name=\"label\">ARROW</elementInformation></elementInformations></element>"
					  "<element><elementInformations><elementInformation name=\"label\">-Q1</elementInformation>"
					  "</elementInformations></element>"
					  "<element><elementInformations><elementInformation name=\"label\">-K1</elementInformation>"
					  "</elementInformations></element>"
					  "</diagram></diagram_content></qet_macro>");
		}

		void rows()
		{
			const QList<LibraryListing::Row> rows = LibraryListing().scan(m_dir.filePath(QStringLiteral("lib")));
			QCOMPARE(rows.size(), 6);
				//Folder by folder, 2 before 10
			QStringList files;
			for (const auto &row : rows) files << row.folder + QLatin1Char('/') + row.file;
			QCOMPARE(files, (QStringList{"plain/broken.elmt", "plain/starter.qetmak", "relays/coil.elmt",
										 "relays/2_less/lamp.elmt", "relays/10_more/contact.elmt",
										 "relays 2/other.elmt"}));

			QVERIFY(!rows.at(0).error.isEmpty());

			const LibraryListing::Row &macro = rows.at(1);
			QVERIFY(macro.macro);
			QCOMPARE(macro.name, QStringLiteral("starter"));
			QCOMPARE(macro.labels, (QStringList{"-Q1", "-K1"}));

			const LibraryListing::Row &coil = rows.at(2);
			QCOMPARE(coil.name, QStringLiteral("Coil"));
			QCOMPARE(coil.type, QStringLiteral("master"));
			QCOMPARE(coil.class_letter, QStringLiteral("K"));
			QCOMPARE(coil.terminal_names, (QStringList{"A1", "A2"}));
			QCOMPARE(coil.unnamed_terminals, 0);

				//A folder the labels file does not describe has none, as
				//when placing it
			QCOMPARE(rows.at(3).class_letter, QString());
			QCOMPARE(rows.at(3).type, QStringLiteral("simple"));
			QCOMPARE(rows.at(3).terminals, 0);

			const LibraryListing::Row &contact = rows.at(4);
			QCOMPARE(contact.terminals, 2);
			QCOMPARE(contact.terminal_names, QStringList{"13"});
			QCOMPARE(contact.unnamed_terminals, 1);
				//One it describes with no letter of its own takes its parent's
			QCOMPARE(contact.class_letter, QStringLiteral("K"));

				//A folder the file names without a letter has none
			QCOMPARE(rows.at(0).class_letter, QString());

			QCOMPARE(rows.at(0).name, QStringLiteral("broken"));

			const QString csv = QString::fromUtf8(LibraryListing::toCsv(rows));
			QVERIFY(csv.startsWith(QStringLiteral("\"name\";\"folder\";\"file\";\"kind\";\"type\";\"class_letter\";")));
			QVERIFY(csv.contains(QStringLiteral("\"Contact, NO\";\"relays/10_more\";\"contact.elmt\";\"symbol\";\"slave\";\"K\";\"2\";\"13\";\"1\";\"\";\"\"\n")));
			QVERIFY(csv.contains(QStringLiteral("\"Coil\";\"relays\";\"coil.elmt\";\"symbol\";\"master\";\"K\";\"2\";\"A1, A2\";\"0\";\"\";\"\"\n")));
			QVERIFY(csv.contains(QStringLiteral("\"starter\";\"plain\";\"starter.qetmak\";\"macro\";\"\";\"\";\"\";\"\";\"\";\"-Q1, -K1\";\"\"\n")));
			QVERIFY(csv.contains(QStringLiteral("\"broken\";\"plain\";\"broken.elmt\";\"symbol\";\"\";\"\";\"\";\"\";\"\";\"\";")));
		}

		/// Every symbol of the shipped collection gets the class letter
		/// placing it gives it, read the way autonum::elementPrefixForLocation()
		/// reads the common collection's labels file
		void shippedCollectionAsPlacing()
		{
			const QString tree = QStringLiteral(QET_ELEMENTS_DIR "/10_electric");
			const QString labels = tree + QStringLiteral("/qet_labels.xml");
			QVERIFY(QFileInfo::exists(labels));
			const QList<LibraryListing::Row> rows = LibraryListing().scan(tree);
			QVERIFY(rows.size() > 1000);
			int lettered = 0;
			for (const auto &row : rows) {
				if (row.macro) continue;
					//The file name first, its folders up to the tree last
				QStringList path {row.file};
				const QStringList folders = row.folder.split(QLatin1Char('/'), Qt::SkipEmptyParts);
				for (auto it = folders.crbegin() ; it != folders.crend() ; ++it) path << *it;
				const QString expected = QetLabelsFile::prefixForPath(labels, path, path.size() - 1);
				QVERIFY2(row.class_letter == (expected.isNull() ? QString() : expected),
						 qPrintable(row.folder + "/" + row.file + ": " + row.class_letter + " vs " + expected));
				if (!row.class_letter.isEmpty()) ++lettered;
			}
			QVERIFY(lettered > 100);
		}

		void writesUtf8WithByteOrderMark()
		{
			const QString name = m_dir.filePath(QStringLiteral("list.csv"));
			QVERIFY(LibraryListing::write({}, name));
			QFile file(name);
			QVERIFY(file.open(QIODevice::ReadOnly));
			QVERIFY(file.readAll().startsWith("\xEF\xBB\xBF" "\"name\";"));
		}
};

QET_APP_TEST_MAIN(TstLibraryListing)
#include "tst_librarylisting.moc"
