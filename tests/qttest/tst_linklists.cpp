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
#include "ui/linksingleelementwidget.h"
#include "ui/masterpropertieswidget.h"
#include <QDomDocument>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <memory>
#include <QtTest>

/**
	The lists of a symbol's link tab (#1842): a slave's masters, a master's
	free slaves, a folio report's free reports. With lazy folios they are
	filled from the project database, so filling one builds no folio, and
	choosing a symbol builds its folio only. They list what they list when
	every folio is built.
*/
class tst_linklists : public QObject
{
	Q_OBJECT

		//industrial.qet with every link removed: every slave and folio
		//report free, so that each list has rows to compare
	QTemporaryDir m_dir;
	QString m_path;

	enum class Tab { Masters, FreeSlaves, FreeReports };

		/// A symbol whose link tab lists what @p tab is about, and its folio
	static Element *subject(QETProject &project, Tab tab, int *folio)
	{
		const QList<Diagram *> folios = project.folios();
		for (int i = 0 ; i < folios.size() ; ++i) {
			for (Element *element : folios.at(i)->elements()) {
				const bool fits = tab == Tab::Masters
						? element->linkType() == Element::Slave && element->isFree()
						: tab == Tab::FreeSlaves
						? element->linkType() == Element::Master
						: (element->linkType() & Element::AllReport) && element->isFree();
				if (fits) {
					*folio = i;
					return element;
				}
			}
		}
		return nullptr;
	}

	static QTreeWidget *list(QWidget *widget, Tab tab)
	{
		return widget->findChild<QTreeWidget *>(tab == Tab::FreeSlaves
												? QStringLiteral("m_free_tree_widget")
												: QStringLiteral("m_tree_widget"));
	}

		/// Every row as its texts, sorted: the lists may order them apart
	static QStringList rows(QTreeWidget *tree)
	{
		QStringList all;
		for (int i = 0 ; i < tree->topLevelItemCount() ; ++i) {
			QStringList texts;
			for (int c = 0 ; c < tree->columnCount() ; ++c)
				texts << tree->topLevelItem(i)->text(c);
			all << texts.join(QLatin1Char('|'));
		}
		all.sort();
		return all;
	}

	static QWidget *tabFor(Element *element, Tab tab)
	{
		if (tab == Tab::FreeSlaves)
			return new MasterPropertiesWidget(element);
		return new LinkSingleElementWidget(element);
	}

private slots:
	void initTestCase()
	{
		QETProject::setBackupEnabled(false);
		QFile source(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY(source.open(QIODevice::ReadOnly));
		QDomDocument document;
		QVERIFY(bool(document.setContent(&source)));
		const QDomNodeList links = document.elementsByTagName(QStringLiteral("links_uuids"));
		QVERIFY(links.size() > 0);
		for (int i = links.size() - 1 ; i >= 0 ; --i)
			links.at(i).parentNode().removeChild(links.at(i));
		QVERIFY(m_dir.isValid());
		m_path = m_dir.filePath(QStringLiteral("unlinked.qet"));
		QFile unlinked(m_path);
		QVERIFY(unlinked.open(QIODevice::WriteOnly));
		unlinked.write(document.toByteArray());
	}

	void theListsBuildNoFolio_data()
	{
		QTest::addColumn<int>("kind");
		QTest::newRow("a slave's masters") << int(Tab::Masters);
		QTest::newRow("a master's free slaves") << int(Tab::FreeSlaves);
		QTest::newRow("a report's free reports") << int(Tab::FreeReports);
	}

	void theListsBuildNoFolio()
	{
		QFETCH(int, kind);
		const Tab tab = Tab(kind);

			//Every folio built: what the list shows
		qputenv("QET_EAGER_FOLIOS", "1");
		QETProject eager(m_path);
		qunsetenv("QET_EAGER_FOLIOS");
		int folio = -1;
		Element *eager_subject = subject(eager, tab, &folio);
		QVERIFY(eager_subject);
		std::unique_ptr<QWidget> eager_tab(tabFor(eager_subject, tab));
		const QStringList expected = rows(list(eager_tab.get(), tab));
		QVERIFY(!expected.isEmpty());

			//Lazy folios: the subject's folio shown, as when selected
		QETProject lazy(m_path);
		lazy.buildFolioToShow(lazy.folios().at(folio));
		Element *lazy_subject = nullptr;
		for (Element *element : lazy.folios().at(folio)->elements())
			if (element->uuid() == eager_subject->uuid()) lazy_subject = element;
		QVERIFY(lazy_subject);
		const int unbuilt = lazy.unloadedFolioCount();
		QVERIFY(unbuilt > 0);

		std::unique_ptr<QWidget> lazy_tab(tabFor(lazy_subject, tab));
		QTreeWidget *tree = list(lazy_tab.get(), tab);
		QCOMPARE(rows(tree), expected);
		QCOMPARE(lazy.unloadedFolioCount(), unbuilt);

			//Showing a symbol of a folio not built builds that folio only
		QTreeWidgetItem *far = nullptr;
		for (int i = 0 ; !far && i < tree->topLevelItemCount() ; ++i) {
			const int column = tab == Tab::FreeSlaves ? 1 : tree->columnCount() - 3;
			const int number = tree->topLevelItem(i)->text(column).toInt();
			if (number > 0 && !lazy.folios().at(number - 1)->isLoaded())
				far = tree->topLevelItem(i);
		}
		QVERIFY(far);
		emit tree->itemDoubleClicked(far, 0);
		QCOMPARE(lazy.unloadedFolioCount(), unbuilt - 1);
	}
};

QET_APP_TEST_MAIN(tst_linklists)
#include "tst_linklists.moc"
