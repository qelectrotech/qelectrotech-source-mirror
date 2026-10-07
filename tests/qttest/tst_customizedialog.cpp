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
#include "ui/customizedialog.h"
#include "ui/configpage/configpage.h"

#include <QDialogButtonBox>
#include <QIcon>
#include <QPushButton>
#include <QTabWidget>
#include <QTest>

/// A configuration page that counts how often it was applied
class CountingPage : public ConfigPage
{
	public:
		CountingPage(const QString &title) : ConfigPage(nullptr), m_title(title) {}
		void applyConf() override { ++applied; }
		QString title() const override { return m_title; }
		QIcon icon() const override { return QIcon(); }
		int applied = 0;
	private:
		QString m_title;
};

/**
	CustomizeDialog: one tab per page in the order added, OK applies every
	page once, Cancel applies none.
*/
class tst_customizedialog : public QObject
{
	Q_OBJECT

private slots:
	void tabsInOrder()
	{
		CustomizeDialog dialog;
		auto *a = new CountingPage(QStringLiteral("Barres d'outils"));
		auto *b = new CountingPage(QStringLiteral("Clavier"));
		dialog.addPage(a);
		dialog.addPage(b);
		dialog.addPage(a);      // twice: ignored
		dialog.addPage(nullptr);
		auto *tabs = dialog.findChild<QTabWidget *>(QStringLiteral("customizeTabs"));
		QVERIFY(tabs);
		QCOMPARE(tabs->count(), 2);
		QCOMPARE(tabs->tabText(0), QStringLiteral("Barres d'outils"));
		QCOMPARE(tabs->tabText(1), QStringLiteral("Clavier"));
		QCOMPARE(tabs->widget(1), b);
	}

	void okAppliesEveryPage()
	{
		CustomizeDialog dialog;
		auto *a = new CountingPage(QStringLiteral("a"));
		auto *b = new CountingPage(QStringLiteral("b"));
		dialog.addPage(a);
		dialog.addPage(b);
		dialog.show();
		dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
		QCOMPARE(a->applied, 1);
		QCOMPARE(b->applied, 1);
		QCOMPARE(dialog.result(), int(QDialog::Accepted));
	}

	void cancelAppliesNothing()
	{
		CustomizeDialog dialog;
		auto *a = new CountingPage(QStringLiteral("a"));
		dialog.addPage(a);
		dialog.show();
		dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel)->click();
		QCOMPARE(a->applied, 0);
		QCOMPARE(dialog.result(), int(QDialog::Rejected));
	}
};

QTEST_MAIN(tst_customizedialog)
#include "tst_customizedialog.moc"
