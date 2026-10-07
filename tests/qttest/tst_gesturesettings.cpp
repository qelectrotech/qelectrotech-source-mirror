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
#include "diagramgestureoverlay.h"
#include "gesturesettings.h"
#include "shortcutmanager.h"
#include "ui/configpage/gesturesconfigpage.h"

#include <QAction>
#include <QComboBox>
#include <QDropEvent>
#include <QListWidget>
#include <QMainWindow>
#include <QMimeData>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTest>

/**
	The mouse gesture ring's own command list: until the user picks one it
	follows the shortcut bar as before; a picked list keeps its directions;
	4 or 8 directions; and the configuration page that edits it.
*/
class tst_gesturesettings : public QObject
{
	Q_OBJECT

	QMainWindow m_window;

	QAction *addCommand(const QString &id, const QString &text)
	{
		auto *action = new QAction(text, &m_window);
		ShortcutManager::instance().registerAction(
			action, id, QStringLiteral("Éditeur de schémas"), QKeySequence());
		return action;
	}

	QListWidgetItem *listItem(QListWidget *list, const QString &id)
	{
		for (int i = 0 ; i < list->count() ; ++i) {
			if (list->item(i)->data(Qt::UserRole).toString() == id) {
				return list->item(i);
			}
		}
		return nullptr;
	}

private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
		QSettings().remove(QStringLiteral("diagrameditor"));
		addCommand(QStringLiteral("diagrameditor.copy"), QStringLiteral("Copier"));
		addCommand(QStringLiteral("diagrameditor.cut"), QStringLiteral("Couper"));
		addCommand(QStringLiteral("diagrameditor.paste"), QStringLiteral("Coller"));
	}

	void cleanup()
	{
		QSettings().remove(QStringLiteral("diagrameditor"));
	}

	void defaultsFollowTheBar()
	{
		QCOMPARE(GestureSettings::directions(), 8);
		for (const auto c : ShortcutBarSettings::contexts()) {
			QVERIFY(!GestureSettings::isCustom(c));
			QCOMPARE(GestureSettings::ids(c), ShortcutBarSettings::ids(c));
		}
			//A changed bar still reaches the ring, without its elements
		ShortcutBarSettings::setIds(ShortcutBarSettings::Selection,
			{QStringLiteral("diagrameditor.copy"),
			 QStringLiteral("common://10_electric/relay.elmt"),
			 QStringLiteral("diagrameditor.cut")});
		QCOMPARE(GestureSettings::ids(ShortcutBarSettings::Selection),
			 QStringList({QStringLiteral("diagrameditor.copy"),
				      QStringLiteral("diagrameditor.cut")}));
	}

	void pickedListKeepsDirections()
	{
		GestureSettings::setIds(ShortcutBarSettings::Canvas,
			{QStringLiteral("diagrameditor.copy"), QString(),
			 QStringLiteral("diagrameditor.paste"), QString(), QString()});
		QVERIFY(GestureSettings::isCustom(ShortcutBarSettings::Canvas));
		QCOMPARE(GestureSettings::ids(ShortcutBarSettings::Canvas),
			 QStringList({QStringLiteral("diagrameditor.copy"), QString(),
				      QStringLiteral("diagrameditor.paste")}));
			//The bar no longer drives it
		ShortcutBarSettings::setIds(ShortcutBarSettings::Canvas,
					    {QStringLiteral("diagrameditor.cut")});
		QCOMPARE(GestureSettings::ids(ShortcutBarSettings::Canvas).first(),
			 QStringLiteral("diagrameditor.copy"));
			//Other contexts still follow the bar
		QVERIFY(!GestureSettings::isCustom(ShortcutBarSettings::Selection));
	}

	void savingTheDefaultsFollowsTheBarAgain()
	{
		GestureSettings::setIds(ShortcutBarSettings::Conductor, {QStringLiteral("diagrameditor.cut")});
		QVERIFY(GestureSettings::isCustom(ShortcutBarSettings::Conductor));
		GestureSettings::setIds(ShortcutBarSettings::Conductor,
					GestureSettings::defaultIds(ShortcutBarSettings::Conductor));
		QVERIFY(!GestureSettings::isCustom(ShortcutBarSettings::Conductor));
	}

	void directions()
	{
		GestureSettings::setDirections(4);
		QCOMPARE(GestureSettings::directions(), 4);
		GestureSettings::setDirections(8);
		QVERIFY(!QSettings().contains(QStringLiteral("diagrameditor/gestures/directions")));
		QSettings().setValue(QStringLiteral("diagrameditor/gestures/directions"), 5);
		QCOMPARE(GestureSettings::directions(), 8);
	}

	void overlayDirectionsAndEmptySlots()
	{
		QWidget viewport;
		viewport.resize(400, 400);
		DiagramGestureOverlay overlay(&viewport);
		QAction *copy = ShortcutManager::instance().action(QStringLiteral("diagrameditor.copy"), &m_window);
		QAction *paste = ShortcutManager::instance().action(QStringLiteral("diagrameditor.paste"), &m_window);
		const QPoint c(200, 200);
		const QPoint right(260, 200), down(200, 260);

		overlay.showAt(c, {copy, nullptr, paste}, 4);
		QCOMPARE(overlay.sectors(), 4);
		QCOMPARE(overlay.sectorAt(right), 1);
		QCOMPARE(overlay.actionAt(right), nullptr);
		QCOMPARE(overlay.actionAt(down), paste);
		overlay.setPointer(right);
		QVERIFY(!overlay.grab().isNull());

			//Today's ring: eight directions
		overlay.showAt(c, {copy, nullptr, paste});
		QCOMPARE(overlay.sectors(), 8);
		QCOMPARE(overlay.sectorAt(right), 2);
		QCOMPARE(overlay.actionAt(right), paste);
	}

	void pagePlacesAndSaves()
	{
		{
			GesturesConfigPage page;
			auto *directions = page.findChild<QComboBox *>(QStringLiteral("directionsCombo"));
			auto *list = page.findChild<QListWidget *>(QStringLiteral("availableList"));
			auto *ring = page.findChild<GestureRingEditor *>(QStringLiteral("ringEditor"));
			QCOMPARE(directions->currentData().toInt(), 8);
			QCOMPARE(ring->ids(), GestureSettings::defaultIds(ShortcutBarSettings::Canvas));

			directions->setCurrentIndex(directions->findData(4));
			ring->setCurrentSlot(1);
			list->setCurrentItem(listItem(list, QStringLiteral("diagrameditor.cut")));
			page.findChild<QPushButton *>(QStringLiteral("placeButton"))->click();
			page.applyConf();
		}
		QCOMPARE(GestureSettings::directions(), 4);
		QVERIFY(GestureSettings::isCustom(ShortcutBarSettings::Canvas));
		QCOMPARE(GestureSettings::ids(ShortcutBarSettings::Canvas).at(1),
			 QStringLiteral("diagrameditor.cut"));
			//Contexts left alone still follow the bar
		QVERIFY(!GestureSettings::isCustom(ShortcutBarSettings::Selection));

		{
			GesturesConfigPage page;
			page.findChild<QPushButton *>(QStringLiteral("followBarButton"))->click();
			page.applyConf();
		}
		QVERIFY(!GestureSettings::isCustom(ShortcutBarSettings::Canvas));
	}

	void dropOnASlot()
	{
		GesturesConfigPage page;
		auto *list = page.findChild<QListWidget *>(QStringLiteral("availableList"));
		auto *ring = page.findChild<GestureRingEditor *>(QStringLiteral("ringEditor"));
		ring->resize(ring->sizeHint());
		const QModelIndex index = list->model()->index(
			list->row(listItem(list, QStringLiteral("diagrameditor.paste"))), 0);
		QMimeData *mime = list->model()->mimeData({index});
			//The right-hand slot of eight
		const QPointF target = QRectF(ring->rect()).center() + QPointF(68, 0);
		QCOMPARE(ring->slotAt(target.toPoint()), 2);
		QDropEvent drop(target, Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
			//Straight to the widget: QApplication drops drag events when
			//no real drag is running
		QVERIFY(static_cast<QObject *>(ring)->event(&drop));
		QVERIFY(drop.isAccepted());
		QCOMPARE(ring->currentSlot(), 2);
		QCOMPARE(ring->ids().value(2), QStringLiteral("diagrameditor.paste"));
			//Moved, not copied: paste was already on the default canvas ring
		QCOMPARE(ring->ids().count(QStringLiteral("diagrameditor.paste")), 1);
		delete mime;
	}
};

QTEST_MAIN(tst_gesturesettings)
#include "tst_gesturesettings.moc"
