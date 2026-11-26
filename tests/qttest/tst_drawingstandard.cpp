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
#include "ui/configpage/projectconfigpages.h"
#include "ui/diagramcontextwidget.h"
#include "projectview.h"
#include "qetmessagebox.h"
#include "qetversion.h"

#include <QComboBox>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

/**
	Item C6 of #1878, agreed in #1289: a project records its drawing
	standard, As before / IEC / NFPA. As before is the default and no data
	at all, so every existing project stays as it is. It is a project
	property, so an older QElectroTech keeps it when it saves (rule 4); a
	value this one does not know is kept as typed (rule 5). It changes
	nothing by itself.
*/
class tst_drawingstandard : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;

	using Standard = QETProject::DrawingStandard;

	static QString savedXml(QETProject &project)
	{
		return project.toXml().toString();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
	}

	void asBeforeIsTheDefaultAndNoData()
	{
		QETProject project;
		project.addNewDiagram();
		QCOMPARE(project.drawingStandard(), Standard::AsBefore);
		QVERIFY(!savedXml(project).contains(QLatin1String("drawing_standard")));
	}

	void namesRoundTrip()
	{
		for (Standard s : {Standard::AsBefore, Standard::Iec, Standard::Nfpa})
			QCOMPARE(QETProject::drawingStandardFromName(QETProject::drawingStandardName(s)), s);
		bool known = false;
		QCOMPARE(QETProject::drawingStandardFromName(QStringLiteral(" IEC "), &known), Standard::Iec);
		QVERIFY(known);
		QCOMPARE(QETProject::drawingStandardFromName(QStringLiteral("DIN"), &known), Standard::AsBefore);
		QVERIFY(!known);
	}

		//In the project's <properties>, where an older QElectroTech keeps
		//it on save; As before takes it out again
	void storedAsAProjectProperty()
	{
		QETProject project;
		project.addNewDiagram();
		project.setDrawingStandard(Standard::Iec);
		QCOMPARE(project.drawingStandard(), Standard::Iec);
		QCOMPARE(project.projectProperties().value(QStringLiteral("drawing_standard")).toString(),
				 QStringLiteral("iec"));
		QDomDocument document = project.toXml();
		const QDomElement properties = document.documentElement().firstChildElement(QStringLiteral("properties"));
		QVERIFY(!properties.isNull());
		QVERIFY(properties.text().contains(QLatin1String("iec")));
		project.setDrawingStandard(Standard::AsBefore);
		QVERIFY(!project.projectProperties().contains(QStringLiteral("drawing_standard")));
		QVERIFY(!savedXml(project).contains(QLatin1String("drawing_standard")));
	}

	void projectKeepsIt_data()
	{
		QTest::addColumn<QString>("file");
		QTest::addColumn<QString>("stored");
		QTest::addColumn<int>("standard");
		for (const QString &ext : {QStringLiteral("qet"), QStringLiteral("qetz"), QStringLiteral("qetdb")}) {
			QTest::newRow(qPrintable(ext + "-iec")) << QStringLiteral("iec.") + ext << QStringLiteral("iec") << int(Standard::Iec);
			QTest::newRow(qPrintable(ext + "-nfpa")) << QStringLiteral("nfpa.") + ext << QStringLiteral("nfpa") << int(Standard::Nfpa);
				//Rule 5: a value it does not know is kept as typed
			QTest::newRow(qPrintable(ext + "-unknown")) << QStringLiteral("din.") + ext << QStringLiteral("DIN 2026") << int(Standard::AsBefore);
		}
	}

	void projectKeepsIt()
	{
		QFETCH(QString, file);
		QFETCH(QString, stored);
		QFETCH(int, standard);
		QETProject project;
		project.addNewDiagram();
		DiagramContext properties = project.projectProperties();
		properties.addValue(QStringLiteral("drawing_standard"), stored);
		project.setProjectProperties(properties);
		project.setFilePath(m_dir.filePath(file));
		const QETResult written = project.write();
		QVERIFY2(written.isOk(), qPrintable(written.errorMessage()));
		QETProject again(m_dir.filePath(file));
		QCOMPARE(again.drawingStandardText(), stored);
		QCOMPARE(int(again.drawingStandard()), standard);
	}

		//Project properties > General: its own list, not a row of the
		//variables; a value it does not know shows as typed and is kept
	void theGeneralPageEditsIt()
	{
		QETProject project;
		project.addNewDiagram();
		DiagramContext properties = project.projectProperties();
		properties.addValue(QStringLiteral("customer"), QStringLiteral("ACME"));
		properties.addValue(QStringLiteral("drawing_standard"), QStringLiteral("DIN 2026"));
		project.setProjectProperties(properties);

		ProjectMainConfigPage page(&project);
		auto *list = page.findChild<QComboBox *>(QStringLiteral("drawingStandard"));
		auto *variables = page.findChild<DiagramContextWidget *>();
		QVERIFY(list && variables);
		QVERIFY(!variables->context().contains(QStringLiteral("drawing_standard")));
		QVERIFY(variables->context().contains(QStringLiteral("customer")));
		QCOMPARE(list->currentText(), QStringLiteral("DIN 2026"));

			//Applied unchanged: kept as typed, the other variables too
		page.applyProjectConf();
		QCOMPARE(project.drawingStandardText(), QStringLiteral("DIN 2026"));
		QCOMPARE(project.projectProperties().value(QStringLiteral("customer")).toString(),
				 QStringLiteral("ACME"));

			//IEC chosen
		list->setCurrentIndex(list->findData(QStringLiteral("iec")));
		page.applyProjectConf();
		QCOMPARE(project.drawingStandard(), Standard::Iec);
		QCOMPARE(project.projectProperties().value(QStringLiteral("customer")).toString(),
				 QStringLiteral("ACME"));

			//As before: no property left
		list->setCurrentIndex(list->findData(QString()));
		page.applyProjectConf();
		QVERIFY(!project.projectProperties().contains(QStringLiteral("drawing_standard")));
		QCOMPARE(project.drawingStandard(), Standard::AsBefore);
	}

		//#1267: a project a newer QElectroTech saved is not saved over
		//without asking. With nobody to answer (the default, Cancel) the
		//file is left as it is; a project of this version saves as before.
	void savingOverANewerProjectAsks()
	{
		const QString path = m_dir.filePath(QStringLiteral("newer.qet"));
		{
			QETProject project;
			project.addNewDiagram();
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QFile file(path);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QByteArray xml = file.readAll();
		file.close();
		const QByteArray current = QetVersion::currentVersion().toString().toUtf8();
		QVERIFY(xml.contains("version=\"" + current + "\""));
		xml.replace("version=\"" + current + "\"", "version=\"99.0\"");
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(xml);
		file.close();

		QET::QetMessageBox::setNonInteractive(true);
		auto *project = new QETProject(path);
		QVERIFY(project->savedByNewerVersion());
		ProjectView view(project);
		view.save();
		QVERIFY(!project->newerVersionSaveAccepted());
		QVERIFY(file.open(QIODevice::ReadOnly));
		QVERIFY(file.readAll().contains("version=\"99.0\""));
		file.close();

			//A project of this version: no question, saved as before
		const QString plain = m_dir.filePath(QStringLiteral("plain.qet"));
		{
			QETProject ours;
			ours.addNewDiagram();
			ours.setFilePath(plain);
			QVERIFY(ours.write().isOk());
		}
		auto *ours = new QETProject(plain);
		QVERIFY(!ours->savedByNewerVersion());
		QET::QetMessageBox::setNonInteractive(false);
	}
};

QET_APP_TEST_MAIN(tst_drawingstandard)
#include "tst_drawingstandard.moc"
