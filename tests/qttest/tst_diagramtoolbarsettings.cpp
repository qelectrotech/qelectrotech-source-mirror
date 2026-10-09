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
#include "diagramtoolbarsettings.h"
#include "shortcutmanager.h"
#include "ui/configpage/toolbarcommandsconfigpage.h"

#include <QAction>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTest>
#include <QToolBar>
#include <QWidget>

/**
	A top-level window standing in for the diagram editor: counts the
	calls applyToAll() makes.
*/
class RebuildCounter : public QWidget
{
	Q_OBJECT
	public:
		int rebuilds = 0;
	public slots:
		void rebuildToolBars() { ++rebuilds; }
};

/**
	DiagramToolbarSettings and its configuration page: an unchanged
	toolbar stores nothing and is built as before, unknown ids and stray
	separators are skipped, the user's own toolbars are kept and removed
	with their contents, and the page saves what it shows.
*/
class tst_diagramtoolbarsettings : public QObject
{
	Q_OBJECT

	QObject m_owner;
	QHash<QString, QAction *> m_actions;

		/// The action for @a id, made on first use, as the editor's resolver
	QAction *resolve(const QString &id)
	{
		if (id.startsWith(QLatin1String("missing."))) return nullptr;
		if (!m_actions.contains(id)) {
			auto *action = new QAction(id, &m_owner);
			m_actions.insert(id, action);
		}
		return m_actions.value(id);
	}

	static QStringList shape(QToolBar *toolbar)
	{
		QStringList list;
		for (QAction *action : toolbar->actions()) {
			list << (action->isSeparator() ? QStringLiteral("|") : action->text());
		}
		return list;
	}

	static void clearSettings()
	{
		QSettings settings;
		settings.remove(QStringLiteral("diagrameditor/toolbars"));
		settings.remove(QStringLiteral("diagrameditor/custom_toolbars"));
	}

private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
		clearSettings();
			//A few of the diagram editor's commands, as it registers them
		for (const QString &id : {QStringLiteral("diagrameditor.copy"),
					  QStringLiteral("diagrameditor.paste"),
					  QStringLiteral("diagrameditor.zoom_fit"),
					  QStringLiteral("depth.raise"),
					  QStringLiteral("elementeditor.copy")}) {
			ShortcutManager::instance().registerAction(
				new QAction(id, &m_owner), id, QStringLiteral("test"), QKeySequence());
		}
	}

	void init() { clearSettings(); }

	void defaultsStoreNothing()
	{
		for (const QString &name : DiagramToolbarSettings::builtInNames()) {
			QVERIFY(!DiagramToolbarSettings::defaultIds(name).isEmpty());
			QCOMPARE(DiagramToolbarSettings::ids(name), DiagramToolbarSettings::defaultIds(name));
			DiagramToolbarSettings::setIds(name, DiagramToolbarSettings::defaultIds(name));
		}
		QVERIFY(!QSettings().contains(QStringLiteral("diagrameditor/toolbars/toolbar")));

		DiagramToolbarSettings::setIds(QStringLiteral("toolbar"), {QStringLiteral("diagrameditor.copy")});
		QCOMPARE(DiagramToolbarSettings::ids(QStringLiteral("toolbar")),
			 QStringList{QStringLiteral("diagrameditor.copy")});
			//Saving an empty toolbar keeps it empty, not back to the defaults
		DiagramToolbarSettings::setIds(QStringLiteral("toolbar"), {});
		QVERIFY(DiagramToolbarSettings::ids(QStringLiteral("toolbar")).isEmpty());
	}

		/// The defaults are built exactly as the toolbars were hand-built
		/// before: the main toolbar's three separators in place
	void defaultsBuildAsBefore()
	{
		QToolBar toolbar;
		DiagramToolbarSettings::fill(&toolbar, DiagramToolbarSettings::defaultIds(QStringLiteral("toolbar")),
					     [this](const QString &id) { return resolve(id); });
		QCOMPARE(toolbar.actions().size(), 18);
		QCOMPARE(shape(&toolbar).mid(6, 2),
			 (QStringList{QStringLiteral("diagrameditor.export_to_pdf"), QStringLiteral("|")}));
		QCOMPARE(shape(&toolbar).count(QStringLiteral("|")), 3);
	}

	void fillSkipsUnknownAndStraySeparators()
	{
		const QString sep = DiagramToolbarSettings::separatorId();
		QToolBar toolbar;
		toolbar.addAction(QStringLiteral("left over"));
		DiagramToolbarSettings::fill(&toolbar,
			{sep, QStringLiteral("a"), sep, sep, QStringLiteral("missing.b"),
			 QStringLiteral("c"), sep, QStringLiteral("missing.d"), sep},
			[this](const QString &id) { return resolve(id); });
		QCOMPARE(shape(&toolbar), (QStringList{QStringLiteral("a"), QStringLiteral("|"), QStringLiteral("c")}));

		DiagramToolbarSettings::fill(&toolbar, {}, [this](const QString &id) { return resolve(id); });
		QVERIFY(toolbar.actions().isEmpty());
	}

	void customToolbarsKeptAndRemoved()
	{
		using T = DiagramToolbarSettings::Toolbar;
		const QString first = DiagramToolbarSettings::newCustomName({});
		QCOMPARE(first, QStringLiteral("custom_1"));
		DiagramToolbarSettings::setCustomToolbars({T{first, QStringLiteral("Mine"), true},
							   T{QStringLiteral("custom_2"), QStringLiteral("Other"), true}});
		DiagramToolbarSettings::setIds(first, {QStringLiteral("diagrameditor.copy")});
		QCOMPARE(DiagramToolbarSettings::customToolbars().size(), 2);
		QCOMPARE(DiagramToolbarSettings::customToolbars().first().title, QStringLiteral("Mine"));
		QCOMPARE(DiagramToolbarSettings::toolbars().size(),
			 DiagramToolbarSettings::builtInNames().size() + 2);
		QCOMPARE(DiagramToolbarSettings::newCustomName({}), QStringLiteral("custom_3"));
			//A new toolbar starts empty
		QVERIFY(DiagramToolbarSettings::ids(QStringLiteral("custom_2")).isEmpty());

		DiagramToolbarSettings::setCustomToolbars({T{QStringLiteral("custom_2"), QStringLiteral("Other"), true}});
		QCOMPARE(DiagramToolbarSettings::customToolbars().size(), 1);
		QVERIFY(!QSettings().contains(QStringLiteral("diagrameditor/toolbars/") + first));

		DiagramToolbarSettings::setCustomToolbars({});
		QVERIFY(!QSettings().contains(QStringLiteral("diagrameditor/custom_toolbars/names")));
	}

	void availableCommandsAreTheDiagramEditors()
	{
		const QStringList ids = DiagramToolbarSettings::availableCommandIds();
		QVERIFY(ids.contains(QStringLiteral("diagrameditor.copy")));
		QVERIFY(ids.contains(QStringLiteral("depth.raise")));
		QVERIFY(!ids.contains(QStringLiteral("elementeditor.copy")));
	}

	void pageSavesAndRebuilds()
	{
		RebuildCounter marked, unmarked;
		DiagramToolbarSettings::markWindow(&marked);

		ToolbarCommandsConfigPage page;
		auto *combo = page.findChild<QComboBox *>(QStringLiteral("toolbarCombo"));
		auto *available = page.findChild<QListWidget *>(QStringLiteral("availableList"));
		auto *chosen = page.findChild<QListWidget *>(QStringLiteral("chosenList"));
		QVERIFY(combo && available && chosen);
		QCOMPARE(combo->count(), DiagramToolbarSettings::builtInNames().size());
		QCOMPARE(chosen->count(), DiagramToolbarSettings::defaultIds(QStringLiteral("toolbar")).size());

			//OK without a change stores nothing
		page.applyConf();
		QVERIFY(!QSettings().contains(QStringLiteral("diagrameditor/toolbars/toolbar")));
		QCOMPARE(marked.rebuilds, 1);
		QCOMPARE(unmarked.rebuilds, 0);

			//Remove everything but the first command, add a separator
		while (chosen->count() > 1) delete chosen->takeItem(1);
		chosen->setCurrentRow(0);
		page.findChild<QPushButton *>(QStringLiteral("separatorButton"))->click();
		page.applyConf();
		QCOMPARE(DiagramToolbarSettings::ids(QStringLiteral("toolbar")),
			 (QStringList{QStringLiteral("diagrameditor.new_file"), DiagramToolbarSettings::separatorId()}));
		QCOMPARE(marked.rebuilds, 2);
	}

		/// A widget button can be in one place only: once moved to another
		/// toolbar it is no longer offered, and resetting its own toolbar
		/// takes it back
	void widgetInOnePlace()
	{
		ToolbarCommandsConfigPage page;
		auto *combo = page.findChild<QComboBox *>(QStringLiteral("toolbarCombo"));
		auto *available = page.findChild<QListWidget *>(QStringLiteral("availableList"));
		auto *chosen = page.findChild<QListWidget *>(QStringLiteral("chosenList"));
		auto offered = [available](const QString &id) {
			for (int i = 0 ; i < available->count() ; ++i)
				if (available->item(i)->data(Qt::UserRole).toString() == id) return true;
			return false;
		};
		const QString colour = QStringLiteral("widget:conductor_color");

			//On "diagram" by default, so not offered for "toolbar"
		QVERIFY(!offered(colour));
			//Taken off "diagram" (switching toolbars keeps the change)
		combo->setCurrentIndex(combo->findData(QStringLiteral("diagram")));
		for (int i = chosen->count() - 1 ; i >= 0 ; --i) {
			if (chosen->item(i)->data(Qt::UserRole).toString() == colour) delete chosen->takeItem(i);
		}
		combo->setCurrentIndex(combo->findData(QStringLiteral("toolbar")));
		QVERIFY(offered(colour));

			//Reset "diagram": the colour button goes back there
		combo->setCurrentIndex(combo->findData(QStringLiteral("diagram")));
		page.findChild<QPushButton *>(QStringLiteral("resetButton"))->click();
		QCOMPARE(chosen->count(), DiagramToolbarSettings::defaultIds(QStringLiteral("diagram")).size());
		combo->setCurrentIndex(combo->findData(QStringLiteral("toolbar")));
		QVERIFY(!offered(colour));
	}
};

QTEST_MAIN(tst_diagramtoolbarsettings)
#include "tst_diagramtoolbarsettings.moc"
