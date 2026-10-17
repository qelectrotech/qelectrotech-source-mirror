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
#include <QDomDocument>
#include <QProcess>
#include <QTemporaryDir>

/**
	A terminal in a symbol's definition with no orientation attribute
	(hand-edited or generated symbols, a damaged file) used to crash
	QElectroTech on opening the project: Qet::orientationFromString() read
	the first character of an empty string. It now faces north, as any
	other unknown orientation does. Runs the real binary's --resave on
	fixtures/terminal_no_orientation.qet: unlinked_contact_label.qet with
	the orientation of its contact's top terminal removed.
*/
class tst_terminalnoorientation : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

private slots:
	void initTestCase()
	{
		QVERIFY2(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)), "qelectrotech binary not found");
		QVERIFY(m_dir.isValid());
	}

	void opensAndSavesFacingNorth()
	{
		const QString fixture = QFINDTESTDATA("fixtures/terminal_no_orientation.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");
		const QString out = m_dir.filePath(QStringLiteral("resaved.qet"));

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), fixture, out});
		QVERIFY(proc.waitForFinished(60000));
		QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
		QCOMPARE(proc.exitCode(), 0);

		QFile file(out);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QDomDocument doc;
		QVERIFY(doc.setContent(&file));

			//The placed contact keeps both terminals; the one whose
			//definition has no orientation faces north (0)
		const QDomNodeList elements = doc.elementsByTagName(QStringLiteral("element"));
		QDomElement contact;
		for (int i = 0; i < elements.size(); ++i) {
			const QDomElement e = elements.at(i).toElement();
			if (e.attribute(QStringLiteral("type")).endsWith(QStringLiteral("contact_relais_nf_esclave.elmt"))
					&& e.parentNode().nodeName() == QLatin1String("elements")) {
				contact = e;
				break;
			}
		}
		QVERIFY2(!contact.isNull(), "the contact is not in the saved project");
		const QDomNodeList terminals = contact.elementsByTagName(QStringLiteral("terminal"));
		QCOMPARE(terminals.size(), 2);
		QStringList orientations;
		for (int i = 0; i < terminals.size(); ++i)
			orientations << terminals.at(i).toElement().attribute(QStringLiteral("orientation"));
		orientations.sort();
		QCOMPARE(orientations, (QStringList{QStringLiteral("0"), QStringLiteral("2")}));
	}
};

QTEST_GUILESS_MAIN(tst_terminalnoorientation)
#include "tst_terminalnoorientation.moc"
