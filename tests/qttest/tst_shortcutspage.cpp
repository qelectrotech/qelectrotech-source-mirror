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
#include "shortcutmanager.h"
#include "ui/configpage/shortcutsconfigpage.h"

#include <QAction>
#include <QComboBox>
#include <QKeySequenceEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QPixmap>
#include <QSettings>
#include <QStandardPaths>
#include <QTest>
#include <QTreeWidget>

/**
	The Shortcuts configuration page: the icon and menu columns, the
	category filter, finding a command by pressing its key, and the
	copied list.
*/
class tst_shortcutspage : public QObject
{
	Q_OBJECT

	QMainWindow m_window;
	QAction *m_save = nullptr;
	QAction *m_run = nullptr;
	QAction *m_tool = nullptr;

	QTreeWidgetItem *row(QTreeWidget *tree, const QString &text)
	{
		const auto found = tree->findItems(text, Qt::MatchExactly | Qt::MatchRecursive, 0);
		return found.isEmpty() ? nullptr : found.first();
	}

	QStringList visible(QTreeWidget *tree)
	{
		QStringList texts;
		for (const QString &text : {QStringLiteral("Enregistrer"), QStringLiteral("Exécuter"), QStringLiteral("Outil")}) {
			if (QTreeWidgetItem *item = row(tree, text); item && !item->isHidden()) {
				texts << text;
			}
		}
		return texts;
	}

private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
		QSettings().remove(QStringLiteral("shortcuts"));

		QPixmap pixmap(16, 16);
		pixmap.fill(Qt::red);

		QMenu *file = m_window.menuBar()->addMenu(QStringLiteral("&Fichier"));
		QMenu *scripts = file->addMenu(QStringLiteral("&Scripts"));
		m_save = file->addAction(QIcon(pixmap), QStringLiteral("&Enregistrer"));
		m_run = scripts->addAction(QStringLiteral("Exécuter"));
			//On a toolbar only, in no menu
		m_tool = new QAction(QStringLiteral("Outil"), &m_window);

		ShortcutManager &manager = ShortcutManager::instance();
		manager.registerAction(m_save, QStringLiteral("t.save"), QStringLiteral("Catégorie A"), QKeySequence(Qt::CTRL | Qt::Key_S));
		manager.registerAction(m_run, QStringLiteral("t.run"), QStringLiteral("Catégorie A"), QKeySequence(Qt::CTRL | Qt::Key_R));
		manager.registerAction(m_tool, QStringLiteral("t.tool"), QStringLiteral("Catégorie B"), QKeySequence());
	}

	void menuPath()
	{
		QCOMPARE(ShortcutsConfigPage::menuPath(m_save), QStringLiteral("Fichier"));
		QCOMPARE(ShortcutsConfigPage::menuPath(m_run), QStringLiteral("Fichier › Scripts"));
		QCOMPARE(ShortcutsConfigPage::menuPath(m_tool), QString());
		QCOMPARE(ShortcutsConfigPage::menuPath(nullptr), QString());
	}

	void iconAndMenuColumns()
	{
		ShortcutsConfigPage page(nullptr);
		auto *tree = page.findChild<QTreeWidget *>();
		QVERIFY(row(tree, QStringLiteral("Enregistrer")));
		QVERIFY(!row(tree, QStringLiteral("Enregistrer"))->icon(0).isNull());
		QVERIFY(row(tree, QStringLiteral("Outil"))->icon(0).isNull());
		QCOMPARE(row(tree, QStringLiteral("Exécuter"))->text(1), QStringLiteral("Fichier › Scripts"));
		QCOMPARE(row(tree, QStringLiteral("Outil"))->text(1), QString());
	}

	void categoryFilter()
	{
		ShortcutsConfigPage page(nullptr);
		auto *tree = page.findChild<QTreeWidget *>();
		auto *combo = page.findChild<QComboBox *>(QStringLiteral("categoryFilterCombo"));
		QCOMPARE(visible(tree).size(), 3);
		combo->setCurrentIndex(combo->findText(QStringLiteral("Catégorie B")));
		QCOMPARE(visible(tree), QStringList{QStringLiteral("Outil")});
		combo->setCurrentIndex(0);
		QCOMPARE(visible(tree).size(), 3);
	}

	void findByKey()
	{
		ShortcutsConfigPage page(nullptr);
		auto *tree = page.findChild<QTreeWidget *>();
		auto *key = page.findChild<QKeySequenceEdit *>(QStringLiteral("keySearchEdit"));
		key->setKeySequence(QKeySequence(Qt::CTRL | Qt::Key_S));
		QCOMPARE(visible(tree), QStringList{QStringLiteral("Enregistrer")});
		key->setKeySequence(QKeySequence(Qt::CTRL | Qt::Key_J));
		QVERIFY(visible(tree).isEmpty());
		key->clear();
		QCOMPARE(visible(tree).size(), 3);
	}

	void copiedList()
	{
		ShortcutsConfigPage page(nullptr);
		auto *combo = page.findChild<QComboBox *>(QStringLiteral("categoryFilterCombo"));
		combo->setCurrentIndex(combo->findText(QStringLiteral("Catégorie A")));
		const QStringList lines = page.listAsText().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
		QCOMPARE(lines.size(), 3);
		QCOMPARE(lines.at(0).count(QLatin1Char('\t')), 3);
		QVERIFY(lines.contains(QStringLiteral("Catégorie A\tFichier\tEnregistrer\t")
				       + QKeySequence(Qt::CTRL | Qt::Key_S).toString(QKeySequence::NativeText)));
		QVERIFY(lines.contains(QStringLiteral("Catégorie A\tFichier › Scripts\tExécuter\t")
				       + QKeySequence(Qt::CTRL | Qt::Key_R).toString(QKeySequence::NativeText)));
	}
};

QTEST_MAIN(tst_shortcutspage)
#include "tst_shortcutspage.moc"
