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
#include "diagram.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

/**
	A read-only project (a file this user cannot write, or one another
	QElectroTech has open) still builds its folios when they are first
	needed: shown, exported, saved elsewhere. Building one is reading it,
	not editing it, so it must hold everything its file holds. Diagram::
	addItem() refused every item of a read-only project, so such a folio
	came out empty, and so did every export or copy of it.
	Set read-only directly: a file made read-only is still writable to
	root, so permissions would not test this under root.
*/
class tst_readonlyfolios : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	static int elementsIn(const QDomElement &xml)
	{
		return xml.elementsByTagName(QStringLiteral("element")).count();
	}

	//The symbols on the folios, not the definitions of the embedded
	//collection, which are <element> tags too
	static int elementsInFolios(const QDomDocument &document)
	{
		int n = 0;
		const QDomNodeList folios = document.elementsByTagName(QStringLiteral("diagram"));
		for (int i = 0 ; i < folios.count() ; ++i)
			n += elementsIn(folios.at(i).toElement());
		return n;
	}

	static int elementsIn(Diagram *folio)
	{
		int n = 0;
		for (QGraphicsItem *item : folio->items())
			if (item->type() == Element::Type) ++n;
		return n;
	}

	//A copy of @p example saved by this QElectroTech: opened again, its
	//folios are built only when needed
	QString saved(const QString &example)
	{
		const QString path = m_dir.filePath(example);
		QETProject project(QStringLiteral(QET_EXAMPLES_DIR "/") + example);
		if (project.state() != QETProject::Ok) return {};
		project.setFilePath(path);
		if (!project.write().isOk()) return {};
		return path;
	}

  private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
	}

	void unbuiltFolioOfAReadOnlyProjectKeepsItsContent()
	{
		const QString path = saved(QStringLiteral("industrial.qet"));
		QVERIFY(!path.isEmpty());
		QETProject project(path);
		QCOMPARE(project.state(), QETProject::Ok);
		project.setReadOnly(true);
		QVERIFY(project.isReadOnly());

		int checked = 0;
			//folios(): diagrams() would build them all first
		for (Diagram *folio : project.folios()) {
			if (folio->isLoaded()) continue;
			const int expected = elementsIn(folio->deferredXml());
			if (!expected) continue;
			folio->ensureLoaded();
			QVERIFY(folio->isLoaded());
			QCOMPARE(elementsIn(folio), expected);
			++checked;
		}
		QVERIFY2(checked > 0, "no folio was left to build: nothing tested");
	}

	//What --resave and the exports write (QETProject::toXml()) while
	//the project stays read-only: every folio as its file has it
	void readOnlyProjectWrittenOutKeepsItsContent()
	{
		const QString path = saved(QStringLiteral("industrial.qet"));
		QVERIFY(!path.isEmpty());
		QFile file(path);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QDomDocument original;
		QVERIFY(bool(original.setContent(&file)));
		const int expected = elementsInFolios(original);
		QVERIFY(expected > 0);

		QETProject project(path);
		project.setReadOnly(true);
		QCOMPARE(elementsInFolios(project.toXml()), expected);
		QVERIFY(project.isReadOnly());
	}
};

QET_APP_TEST_MAIN(tst_readonlyfolios)
#include "tst_readonlyfolios.moc"
