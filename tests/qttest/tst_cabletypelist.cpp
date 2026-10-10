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
#include "cablelist/cabletypelist.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

/**
	The cable type catalogue is read back the way it was written, and the
	three columns it really uses are the only ones a new file gets:

	 - a catalogue carrying the material file's whole shape (ten columns,
	   two header lines) reads as ten columns, its machine header line is
	   not mistaken for a cable, and the type comes out of it : this is
	   the catalogue a user already has on disk;
	 - a file the program creates itself holds the two header lines and
	   the three columns, no more;
	 - a file written by hand with the label line alone still gives the
	   type, so dropping the machine header never costs a column;
	   - and a line of cables is never taken for the machine header, even
	   when it happens to be written in key shaped words.
*/
class tst_cabletypelist : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QString write(const QString &name, const QString &content)
	{
		const QString path = m_dir.filePath(name);
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			return {};
		}
		file.write(content.toUtf8());
		file.close();
		return path;
	}

	bool load(const QString &path, CableTypeListData *data)
	{
		QString error;
		const bool ok = CableTypeList::load(path, data, &error);
		if (!ok) {
			qWarning("%s", qPrintable(error));
		}
		return ok;
	}

	private slots:
	void tenColumnCatalogueReadsBack()
	{
		const QString path = write(QStringLiteral("ten.csv"),
								   QStringLiteral(
									   "Désignation;Nombre d'âmes;Couleurs des âmes;Section du câble;Couleur du fourreau;Fabricant;Numéro de commande;Fournisseur;Description textuelle;Notes\n"
									   "designation;cores;core_colors;section;color;manufacturer;manufacturer_reference;supplier;description;notes\n"
									   "H07V-K 3G1,5;3;wh|bn|bl;1.5;PVC;Nexans;X123;Rexel;courant fort;note\n"
									   "NYM-J 3G2,5;3;wh|bn|bl;2.5;PVC;;;;;\n"));
		QVERIFY(!path.isEmpty());

		CableTypeListData data;
		QVERIFY2(load(path, &data), "the catalogue could not be read");

		//The machine header line is not one of the cables...
		QCOMPARE(data.records.size(), 2);
		//... and every column it names is read under that name, even the
		//ones this program never gave a meaning to.
		QCOMPARE(data.columns, QStringList({
					 QStringLiteral("designation"), QStringLiteral("cores"),
					 QStringLiteral("core_colors"), QStringLiteral("section"),
					 QStringLiteral("color"), QStringLiteral("manufacturer"),
					 QStringLiteral("manufacturer_reference"),
					 QStringLiteral("supplier"), QStringLiteral("description"),
					 QStringLiteral("notes")}));

		QCOMPARE(CableTypeList::designation(data.records.at(0)),
				 QStringLiteral("H07V-K 3G1,5"));
		QCOMPARE(CableTypeList::coreCount(data.records.at(0)), 3);
		QCOMPARE(CableTypeList::coreColors(data.records.at(0)),
				 QStringList({QStringLiteral("wh"), QStringLiteral("bn"),
							  QStringLiteral("bl")}));
		//A column outside the three the program uses keeps its name and
		//its data.
		QCOMPARE(data.records.at(0).value(QStringLiteral("section")),
				 QStringLiteral("1.5"));
		QCOMPARE(CableTypeList::designation(data.records.at(1)),
				 QStringLiteral("NYM-J 3G2,5"));
	}

	void newFileHasThreeColumns()
	{
		const QString path = m_dir.filePath(QStringLiteral("new.csv"));
		QVERIFY2(CableTypeList::createFile(path), "the file was not created");

		CableTypeListData data;
		QVERIFY(load(path, &data));
		QCOMPARE(data.columns, QStringList({
					 QStringLiteral("designation"), QStringLiteral("cores"),
					 QStringLiteral("core_colors")}));
		QVERIFY(data.records.isEmpty());
		//The label line reads the way the cable form names the type.
		QCOMPARE(CableTypeList::translatedHeader(data.columns), QStringList({
					 QStringLiteral("Désignation"),
					 QStringLiteral("Nombre d'âmes"),
					 QStringLiteral("Couleurs des âmes")}));

		CableTypeRecord record;
		record.setValue(QStringLiteral("designation"),
						QStringLiteral("H07V-K 3G1,5"));
		record.setValue(QStringLiteral("cores"), QStringLiteral("3"));
		record.setValue(QStringLiteral("core_colors"),
						QStringLiteral("wh|bn|bl"));
		QVERIFY2(CableTypeList::appendRecord(path, record),
				 "the record was not appended");

		CableTypeListData read_back;
		QVERIFY(load(path, &read_back));
		QCOMPARE(read_back.columns, data.columns);
		QCOMPARE(read_back.records.size(), 1);
		QCOMPARE(CableTypeList::designation(read_back.records.first()),
				 QStringLiteral("H07V-K 3G1,5"));
		QCOMPARE(CableTypeList::coreCount(read_back.records.first()), 3);
		QCOMPARE(CableTypeList::coreColors(read_back.records.first()),
				 QStringList({QStringLiteral("wh"), QStringLiteral("bn"),
							  QStringLiteral("bl")}));
	}

	//The folder a cable type file goes into may not exist yet: a fresh
	//profile has no documents folder, and a path typed into the settings
	//may name one which was never created. The write makes the folder
	//rather than failing with a message which says nothing about why.
	void aMissingFolderIsCreated()
	{
		const QString path = m_dir.filePath(
			QStringLiteral("not/there/yet/qet_cable_types.csv"));
		QVERIFY(!QFile::exists(path));

		QString error;
		QVERIFY2(CableTypeList::createFile(path, &error), qPrintable(error));
		QVERIFY2(QFile::exists(path), "the file was not created");

		CableTypeListData data;
		QVERIFY2(load(path, &data), "the file just created could not be read");
		QVERIFY(data.records.isEmpty());
	}

	void labelLineAloneStillGivesTheType()
	{
		const QString path = write(QStringLiteral("hand.csv"),
								   QStringLiteral(
									   "Désignation;Nombre d'âmes;Couleurs des âmes\n"
									   "H07V-K 3G1,5;3;wh|bn|bl\n"));
		QVERIFY(!path.isEmpty());

		CableTypeListData data;
		QVERIFY2(load(path, &data), "the hand written file could not be read");
		QCOMPARE(data.columns, QStringList({
					 QStringLiteral("designation"), QStringLiteral("cores"),
					 QStringLiteral("core_colors")}));
		QCOMPARE(data.records.size(), 1);
		QCOMPARE(CableTypeList::designation(data.records.first()),
				 QStringLiteral("H07V-K 3G1,5"));
		QCOMPARE(CableTypeList::coreCount(data.records.first()), 3);
	}

	void aCableIsNeverMistakenForTheHeader()
	{
		//Three cells which all look like column keys, none of which means
		//anything here : far more likely to be a cable than a header.
		const QString path = write(QStringLiteral("unknown.csv"),
								   QStringLiteral(
									   "Désignation;Nombre d'âmes;Couleurs des âmes\n"
									   "ymj;;wh\n"
									   "H07V-K 3G1,5;3;wh|bn|bl\n"));
		QVERIFY(!path.isEmpty());

		CableTypeListData data;
		QVERIFY2(load(path, &data), "the file could not be read");
		QCOMPARE(data.records.size(), 2);
		QCOMPARE(CableTypeList::designation(data.records.first()),
				 QStringLiteral("ymj"));
		QCOMPARE(CableTypeList::designation(data.records.last()),
				 QStringLiteral("H07V-K 3G1,5"));
	}
};

QTEST_APPLESS_MAIN(tst_cabletypelist)
#include "tst_cabletypelist.moc"
