#include <QtTest>
#include <QDomDocument>
#include <QGraphicsRectItem>
#include <QGraphicsScene>

#include "itemgroups.h"

class tst_itemgroups : public QObject
{
	Q_OBJECT

	QGraphicsScene *scene = nullptr;
	QGraphicsRectItem *a = nullptr, *b = nullptr, *c = nullptr, *d = nullptr, *e = nullptr;
	QUuid g1, g2;

	QGraphicsRectItem *add()
	{
		auto item = scene->addRect(0, 0, 10, 10);
		item->setFlag(QGraphicsItem::ItemIsSelectable);
		return item;
	}

	QList<QGraphicsItem *> selection() const { return scene->selectedItems(); }

	// Select exactly @a items, as Qt leaves the scene after a click.
	void select(const QList<QGraphicsItem *> &items)
	{
		scene->clearSelection();
		for (auto item : items) {
			item->setSelected(true);
		}
	}

	bool selected(std::initializer_list<QGraphicsItem *> expected) const
	{
		QList<QGraphicsItem *> got = selection();
		if (got.size() != int(expected.size())) {
			return false;
		}
		for (auto item : expected) {
			if (!got.contains(item)) {
				return false;
			}
		}
		return true;
	}

private slots:
	// a, b in g1; c alone; d, e in g2
	void init()
	{
		scene = new QGraphicsScene;
		a = add(); b = add(); c = add(); d = add(); e = add();
		g1 = QUuid::createUuid();
		g2 = QUuid::createUuid();
		ItemGroups::setGroup(a, g1);
		ItemGroups::setGroup(b, g1);
		ItemGroups::setGroup(d, g2);
		ItemGroups::setGroup(e, g2);
	}

	void cleanup()
	{
		delete scene;
		scene = nullptr;
	}

	void clickingAMemberSelectsTheGroup()
	{
		select({a});
		QVERIFY(ItemGroups::completeSelection(scene, {}, false));
		QVERIFY(selected({a, b}));
	}

	// Releasing a click on one member of the selected group leaves only it
	// selected in Qt; the group must stay whole.
	void reclickingAMemberKeepsTheGroup()
	{
		select({b});
		ItemGroups::completeSelection(scene, {a, b}, false);
		QVERIFY(selected({a, b}));
	}

	void ctrlClickingAMemberOffDeselectsTheGroup()
	{
		select({b});
		ItemGroups::completeSelection(scene, {a, b}, true);
		QVERIFY(selected({}));
	}

	void ctrlClickingAMemberOnAddsTheGroup()
	{
		select({c, a});
		ItemGroups::completeSelection(scene, {c}, true);
		QVERIFY(selected({a, b, c}));
	}

	void otherGroupsAreUntouched()
	{
		select({b, d, e});
		ItemGroups::completeSelection(scene, {a, b, d, e}, true);
		QVERIFY(selected({d, e}));
	}

	void ungroupedItemsAreUntouched()
	{
		select({c});
		QVERIFY(!ItemGroups::completeSelection(scene, {}, false));
		QVERIFY(selected({c}));
	}

	// A member taken off the scene (deleted, kept by the undo stack) must
	// not count as "deselected".
	void itemsLeavingTheSceneAreIgnored()
	{
		select({a, b});
		scene->removeItem(b);
		ItemGroups::completeSelection(scene, {a, b}, true);
		QVERIFY(selected({a}));
		delete b;
		b = nullptr;
	}

	void xmlRoundTrip()
	{
		QDomDocument doc;
		QDomElement grouped = doc.createElement(QStringLiteral("element"));
		ItemGroups::write(grouped, a);
		QCOMPARE(ItemGroups::read(grouped), g1);

		QDomElement alone = doc.createElement(QStringLiteral("element"));
		ItemGroups::write(alone, c);
		QVERIFY(!alone.hasAttribute(QStringLiteral("group")));
		QVERIFY(ItemGroups::read(alone).isNull());

		ItemGroups::setGroup(a, QUuid());
		QVERIFY(ItemGroups::groupOf(a).isNull());
	}
};

QTEST_MAIN(tst_itemgroups)
#include "tst_itemgroups.moc"
