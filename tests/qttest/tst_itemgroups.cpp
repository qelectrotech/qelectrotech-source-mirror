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

	// A second click on a member of a group selected whole picks that member
	// out; a click on a member of a group not selected whole does not.
	void aMemberOfAWholeGroupCanBePicked()
	{
		select({a, b});
		QCOMPARE(ItemGroups::memberToPick(a), a);
		QCOMPARE(ItemGroups::memberToPick(b), b);
	}

	void aMemberOfAPartlySelectedGroupIsNotPicked()
	{
		select({a});       // after one member was picked
		QCOMPARE(ItemGroups::memberToPick(a), nullptr);
		select({});
		QCOMPARE(ItemGroups::memberToPick(a), nullptr);
	}

	void anUngroupedItemIsNotPicked()
	{
		select({c});
		QCOMPARE(ItemGroups::memberToPick(c), nullptr);
		QCOMPARE(ItemGroups::memberToPick(nullptr), nullptr);
	}

	void aGroupOfOneIsNotPicked()
	{
		ItemGroups::setGroup(e, QUuid());   // g2 is now d alone
		select({d});
		QCOMPARE(ItemGroups::memberToPick(d), nullptr);
	}

	// A click lands on a symbol's own text, not on the symbol: the member is
	// the nearest grouped ancestor.
	void aClickOnAMembersChildPicksTheMember()
	{
		auto child = new QGraphicsRectItem(0, 0, 2, 2, a);
		QCOMPARE(ItemGroups::groupedItem(child), a);
		select({a, b});
		QCOMPARE(ItemGroups::memberToPick(child), a);
		QCOMPARE(ItemGroups::groupedItem(c), nullptr);
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

	// Rotate turns a selection that is exactly one whole group as one piece.
	void aWholeGroupAloneIsASoleWholeGroup()
	{
		select({a, b});
		QCOMPARE(ItemGroups::soleWholeGroup(selection()), g1);
	}

	void aPickedMemberIsNotAWholeGroup()
	{
		select({a});
		QVERIFY(ItemGroups::soleWholeGroup(selection()).isNull());
	}

	void aGroupWithOtherItemsIsNotASoleGroup()
	{
		select({a, b, c});
		QVERIFY(ItemGroups::soleWholeGroup(selection()).isNull());
		select({a, b, d, e});
		QVERIFY(ItemGroups::soleWholeGroup(selection()).isNull());
	}

	void ungroupedItemsAreNotAGroup()
	{
		select({c});
		QVERIFY(ItemGroups::soleWholeGroup(selection()).isNull());
		QVERIFY(ItemGroups::soleWholeGroup({}).isNull());
	}
};

QTEST_MAIN(tst_itemgroups)
#include "tst_itemgroups.moc"
