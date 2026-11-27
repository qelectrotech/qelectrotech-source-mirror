/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-2.0-or-later
#include "diagram.h"
#include "qetapptest.h"
#include "qetproject.h"
#include "qetresult.h"
#include "titleblockproperties.h"
#include "utils/projecttemplates.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

/**
	Item B13 of #1878, from #852: a new project made from a template has
	the template's folios, title block, paper size, numbering and drawing
	standard, no title and no file, and a project uuid of its own.
*/
class tst_projecttemplates : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;

	static void write(const QString &path, const QByteArray &text)
	{
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(text);
	}

		//A project made from the template as the editor makes it
	QETProject *fromTemplate(const QString &path, QString *copy_path = nullptr)
	{
		const QString copy = m_dir.filePath(QStringLiteral("copy-%1.qet").arg(path.size()));
		QString error;
		if (!ProjectTemplates::prepareCopy(path, copy, &error)) {
			qWarning() << error;
			return nullptr;
		}
		if (copy_path) *copy_path = copy;
		return new QETProject(copy, QString());
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
	}

	void listsOnlyQetFilesFolderByFolder()
	{
		QDir().mkpath(m_dir.filePath(QStringLiteral("a")));
		QDir().mkpath(m_dir.filePath(QStringLiteral("b")));
		write(m_dir.filePath(QStringLiteral("a/Zeta.qet")), "<project/>");
		write(m_dir.filePath(QStringLiteral("a/alpha.qet")), "<project/>");
		write(m_dir.filePath(QStringLiteral("a/notes.txt")), "x");
		write(m_dir.filePath(QStringLiteral("b/Mine.qet")), "<project/>");
		const auto found = ProjectTemplates::list({
			{QStringLiteral("A"), m_dir.filePath(QStringLiteral("a"))},
			{QStringLiteral("B"), m_dir.filePath(QStringLiteral("b"))},
			{QStringLiteral("none"), m_dir.filePath(QStringLiteral("missing"))}});
		QCOMPARE(found.size(), 3);
		QCOMPARE(found.at(0).name, QStringLiteral("alpha"));
		QCOMPARE(found.at(1).name, QStringLiteral("Zeta"));
		QCOMPARE(found.at(2).origin, QStringLiteral("B"));
	}

	void refusesWhatIsNotAProject()
	{
		const QString path = m_dir.filePath(QStringLiteral("bad.qet"));
		write(path, "not xml at all");
		QString error;
		QVERIFY(!ProjectTemplates::prepareCopy(path, m_dir.filePath(QStringLiteral("out.qet")), &error));
		QVERIFY(!error.isEmpty());
		write(path, "<other/>");
		QVERIFY(!ProjectTemplates::prepareCopy(path, m_dir.filePath(QStringLiteral("out.qet")), &error));
	}

		//Same template, two projects: two identities, and no title
	void everyNewProjectHasItsOwnUuid()
	{
		const QString path = m_dir.filePath(QStringLiteral("t.qet"));
		write(path, "<project title=\"Template\" uuid=\"{11111111-1111-1111-1111-111111111111}\"/>");
		const QString first = m_dir.filePath(QStringLiteral("one.qet")), second = m_dir.filePath(QStringLiteral("two.qet"));
		QVERIFY(ProjectTemplates::prepareCopy(path, first, nullptr));
		QVERIFY(ProjectTemplates::prepareCopy(path, second, nullptr));
		QETProject one(first, QString()), two(second, QString());
		QVERIFY(one.uuid() != two.uuid());
		QVERIFY(one.uuid() != QUuid(QStringLiteral("{11111111-1111-1111-1111-111111111111}")));
		QVERIFY(one.title().isEmpty());
		QVERIFY(one.filePath().isEmpty());
	}

		//The template QElectroTech ships
	void theIecTemplate()
	{
		const QString path = QStringLiteral(QET_TEMPLATES_DIR "/IEC-A3.qet");
		QVERIFY(QFile::exists(path));
		QString copy;
		QScopedPointer<QETProject> project(fromTemplate(path, &copy));
		QVERIFY(project);
			//The editor's copy is a temporary file, gone once it is open
		QVERIFY(QFile::remove(copy));
		QCOMPARE(int(project->state()), int(QETProject::Ok));
		QVERIFY(project->filePath().isEmpty());
		QCOMPARE(project->drawingStandard(), QETProject::DrawingStandard::Iec);

			//Paper size: A3 landscape, 420 x 297 mm at 96 dpi
		QCOMPARE(project->diagrams().size(), 1);
		Diagram *folio = project->diagrams().first();
		const QRectF page = folio->border_and_titleblock.borderAndTitleBlockRect();
			//(the PDF export rounds a page within 3 pt of a paper size to it)
		QVERIFY(qAbs(page.width() - (1587 - 1)) <= 3);
		QVERIFY(qAbs(page.height() - (1122 - 1)) <= 3);

			//Title block
		QCOMPARE(folio->border_and_titleblock.titleBlockTemplateName(), QStringLiteral("ISO7200 A3"));
		QVERIFY(project->embeddedTitleBlockTemplatesCollection()->templates().contains(QStringLiteral("ISO7200 A3")));

			//Numbering, for the folios a new project adds as well
		QCOMPARE(project->conductorCurrentAutoNum(), QStringLiteral("IEC wires"));
		QCOMPARE(project->elementCurrentAutoNum(), QStringLiteral("IEC symbols"));
		QCOMPARE(project->conductorAutoNumFormula(QStringLiteral("IEC wires")), QStringLiteral("%id-%sequf_1"));
		QCOMPARE(project->elementAutoNumFormula(QStringLiteral("IEC symbols")), QStringLiteral("-%prefix%sequ_1"));
		QVERIFY(project->folioAutoNum().contains(QStringLiteral("Folio")));
		QCOMPARE(project->defaultBorderProperties().columns_count, 27);
		QCOMPARE(project->defaultBorderProperties().rows_count, 13);
		QCOMPARE(project->defaultTitleBlockProperties().template_name, QStringLiteral("ISO7200 A3"));

			//Saved where the user says, it opens again as it was
		project->setFilePath(m_dir.filePath(QStringLiteral("saved.qet")));
		const QETResult written = project->write();
		QVERIFY2(written.isOk(), qPrintable(written.errorMessage()));
		QETProject again(m_dir.filePath(QStringLiteral("saved.qet")));
		QCOMPARE(again.uuid(), project->uuid());
		QCOMPARE(again.drawingStandard(), QETProject::DrawingStandard::Iec);
		QCOMPARE(again.diagrams().size(), 1);
		QCOMPARE(again.diagrams().first()->border_and_titleblock.titleBlockTemplateName(), QStringLiteral("ISO7200 A3"));

	}
};

QTEST_MAIN(tst_projecttemplates)
#include "tst_projecttemplates.moc"
