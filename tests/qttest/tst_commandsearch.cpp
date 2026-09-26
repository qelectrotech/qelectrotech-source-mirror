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
#include "commandsearchpopup.h"
#include "shortcutmanager.h"

#include <QAction>
#include <QLineEdit>
#include <QListWidget>
#include <QSignalSpy>
#include <QTest>

/**
	CommandSearchPopup: accent- and case-blind matching, ranking, and that
	Enter runs the highlighted command of the owning window only.
*/
class tst_commandsearch : public QObject
{
	Q_OBJECT

private:
	QAction *add(QWidget *owner, const QString &id, const QString &text)
	{
		auto *action = new QAction(text, owner);
		ShortcutManager::instance().registerAction(action, id, QStringLiteral("test"), QKeySequence());
		return action;
	}

	QStringList shown(CommandSearchPopup &popup)
	{
		QStringList texts;
		auto *list = popup.findChild<QListWidget *>();
		for (int i = 0 ; i < list->count() ; ++i) {
			texts << list->item(i)->text();
		}
		return texts;
	}

private slots:
	void fold_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("expected");
		QTest::newRow("accents")  << QStringLiteral("Éditer l'élément") << QStringLiteral("editer l'element");
		QTest::newRow("mnemonic") << QStringLiteral("&Fichier")         << QStringLiteral("fichier");
		QTest::newRow("cedilla")  << QStringLiteral("Façade")           << QStringLiteral("facade");
		QTest::newRow("plain")    << QStringLiteral("zoom")             << QStringLiteral("zoom");
	}

	void fold()
	{
		QFETCH(QString, input);
		QFETCH(QString, expected);
		QCOMPARE(CommandSearchPopup::fold(input), expected);
	}

	void ranksAndRunsTheBestMatch()
	{
		QWidget owner;
		QWidget other;
		QAction *rotate = add(&owner, QStringLiteral("t.rotate"), QStringLiteral("Pivoter"));
		add(&owner, QStringLiteral("t.rotate_texts"), QStringLiteral("Orienter les textes"));
		QAction *edit = add(&owner, QStringLiteral("t.edit"), QStringLiteral("Éditer l'item sélectionné"));
		add(&owner, QStringLiteral("t.text"), QStringLiteral("Ajouter un champ de texte"));
			//Same id family, another window: must not be listed
		add(&other, QStringLiteral("t.other"), QStringLiteral("Texte d'une autre fenêtre"));

		CommandSearchPopup popup(&owner);
		popup.popUpAt(QPoint(0, 0));
		auto *search = popup.findChild<QLineEdit *>();

		search->setText(QStringLiteral("texte"));
			//word start ("…textes") before a contained match; other window absent
		QCOMPARE(shown(popup), (QStringList{QStringLiteral("Ajouter un champ de texte"),
						    QStringLiteral("Orienter les textes")}));

		search->setText(QStringLiteral("editer"));
		QCOMPARE(shown(popup).value(0), QStringLiteral("Éditer l'item sélectionné"));
		QSignalSpy edited(edit, &QAction::triggered);
		QSignalSpy rotated(rotate, &QAction::triggered);
		QTest::keyClick(&popup, Qt::Key_Return);
		QCOMPARE(edited.count(), 1);
		QCOMPARE(rotated.count(), 0);
		QVERIFY(!popup.isVisible());
	}

	void disabledCommandsCannotRun()
	{
		QWidget owner;
		QAction *paste = add(&owner, QStringLiteral("u.paste"), QStringLiteral("Coller"));
		paste->setEnabled(false);
		CommandSearchPopup popup(&owner);
		popup.popUpAt(QPoint(0, 0));
		popup.findChild<QLineEdit *>()->setText(QStringLiteral("coller"));
		QCOMPARE(shown(popup), QStringList{QStringLiteral("Coller")});
		QSignalSpy pasted(paste, &QAction::triggered);
		QTest::keyClick(&popup, Qt::Key_Return);
		QCOMPARE(pasted.count(), 0);
	}
};

QTEST_MAIN(tst_commandsearch)
#include "tst_commandsearch.moc"
