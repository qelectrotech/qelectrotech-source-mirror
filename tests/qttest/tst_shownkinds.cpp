#include <QtTest>
#include <QGraphicsRectItem>
#include <QGraphicsScene>

#include "shownkinds.h"

class tst_shownkinds : public QObject
{
	Q_OBJECT

	QGraphicsScene *scene = nullptr;

	QGraphicsRectItem *add(QGraphicsItem *parent = nullptr)
	{
		auto item = new QGraphicsRectItem(0, 0, 10, 10, parent);
		item->setFlag(QGraphicsItem::ItemIsSelectable);
		if (!parent) {
			scene->addItem(item);
		}
		return item;
	}

private slots:
	void init()
	{
		scene = new QGraphicsScene;
		for (int k = 0; k < ShownKinds::KindCount; ++k) {
			ShownKinds::setShown(ShownKinds::Kind(k), true);
		}
	}

	void cleanup()
	{
		delete scene;
		scene = nullptr;
	}

	void hideAndShowAgain()
	{
		auto shape = add();
		auto text = add();
		ShownKinds::tag(shape, ShownKinds::Shapes);
		ShownKinds::tag(text, ShownKinds::FreeTexts);

		ShownKinds::setShown(ShownKinds::Shapes, false);
		ShownKinds::apply(scene, ShownKinds::Shapes);
		QVERIFY(!shape->isVisible());
		QVERIFY(text->isVisible());
		QCOMPARE(ShownKinds::hiddenCount(), 1);

		ShownKinds::setShown(ShownKinds::Shapes, true);
		ShownKinds::apply(scene, ShownKinds::Shapes);
		QVERIFY(shape->isVisible());
		QCOMPARE(ShownKinds::hiddenCount(), 0);
	}

	// Created while its kind is hidden: starts hidden, whatever created it.
	void createdWhileHidden()
	{
		ShownKinds::setShown(ShownKinds::Pictures, false);
		auto picture = add();
		ShownKinds::tag(picture, ShownKinds::Pictures);
		QVERIFY(!picture->isVisible());
	}

	// Untagged items (symbols, wires) are never touched.
	void untaggedUntouched()
	{
		auto symbol = add();
		auto hidden_symbol = add();
		hidden_symbol->setVisible(false);
		for (int k = 0; k < ShownKinds::KindCount; ++k) {
			ShownKinds::setShown(ShownKinds::Kind(k), false);
			ShownKinds::apply(scene, ShownKinds::Kind(k));
		}
		QVERIFY(symbol->isVisible());
		for (int k = 0; k < ShownKinds::KindCount; ++k) {
			ShownKinds::setShown(ShownKinds::Kind(k), true);
			ShownKinds::apply(scene, ShownKinds::Kind(k));
		}
		QVERIFY(!hidden_symbol->isVisible());
	}

	// apply() touches only the kind it is given.
	void applyOnlyThatKind()
	{
		auto shape = add();
		ShownKinds::tag(shape, ShownKinds::Shapes);
		ShownKinds::setShown(ShownKinds::Shapes, false);
		ShownKinds::apply(scene, ShownKinds::Tables);
		QVERIFY(shape->isVisible());
	}

	// An item that hid itself (a wire number switched off) stays hidden
	// when its kind is hidden and shown again.
	void ownHiddenStaysHidden()
	{
		auto number = add();
		ShownKinds::tag(number, ShownKinds::WireNumbers);
		ShownKinds::setVisible(number, false);
		ShownKinds::setShown(ShownKinds::WireNumbers, false);
		ShownKinds::apply(scene, ShownKinds::WireNumbers);
		ShownKinds::setShown(ShownKinds::WireNumbers, true);
		ShownKinds::apply(scene, ShownKinds::WireNumbers);
		QVERIFY(!number->isVisible());
	}

	// An item asking to be shown while its kind is hidden stays hidden,
	// and appears when the kind is shown (one text per potential moving
	// to another wire, a cross-reference changing its snap).
	void showVetoedThenRestored()
	{
		auto number = add();
		ShownKinds::tag(number, ShownKinds::WireNumbers);
		ShownKinds::setShown(ShownKinds::WireNumbers, false);
		ShownKinds::apply(scene, ShownKinds::WireNumbers);
		ShownKinds::setVisible(number, true);
		QVERIFY(!number->isVisible());
		ShownKinds::setShown(ShownKinds::WireNumbers, true);
		ShownKinds::apply(scene, ShownKinds::WireNumbers);
		QVERIFY(number->isVisible());
	}

	// Hidden while its kind is hidden: not shown again with the kind.
	void hiddenWhileKindHidden()
	{
		auto number = add();
		ShownKinds::tag(number, ShownKinds::WireNumbers);
		ShownKinds::setShown(ShownKinds::WireNumbers, false);
		ShownKinds::apply(scene, ShownKinds::WireNumbers);
		ShownKinds::setVisible(number, false);
		ShownKinds::setShown(ShownKinds::WireNumbers, true);
		ShownKinds::apply(scene, ShownKinds::WireNumbers);
		QVERIFY(!number->isVisible());
	}

	// wantsVisible() tells the wire carrying a potential's text from the
	// others while wire texts are hidden, so deleting it moves the text.
	void wantsVisibleWhileKindHidden()
	{
		auto carrier = add();
		auto other = add();
		ShownKinds::tag(carrier, ShownKinds::WireNumbers);
		ShownKinds::tag(other, ShownKinds::WireNumbers);
		ShownKinds::setVisible(other, false);
		ShownKinds::setShown(ShownKinds::WireNumbers, false);
		ShownKinds::apply(scene, ShownKinds::WireNumbers);
		QVERIFY(!carrier->isVisible());
		QVERIFY(ShownKinds::wantsVisible(carrier));
		QVERIFY(!ShownKinds::wantsVisible(other));
		ShownKinds::setShown(ShownKinds::WireNumbers, true);
		ShownKinds::apply(scene, ShownKinds::WireNumbers);
		QVERIFY(ShownKinds::wantsVisible(carrier));
		QVERIFY(!ShownKinds::wantsVisible(other));
	}

	// A text under a hidden parent still counts as wanting to be visible.
	void childOfHiddenParent()
	{
		auto group = add();
		auto text = add(group);
		ShownKinds::tag(group, ShownKinds::SymbolTexts);
		ShownKinds::tag(text, ShownKinds::CrossReferences);
		ShownKinds::setShown(ShownKinds::SymbolTexts, false);
		ShownKinds::apply(scene, ShownKinds::SymbolTexts);
		ShownKinds::setShown(ShownKinds::CrossReferences, false);
		ShownKinds::apply(scene, ShownKinds::CrossReferences);
		ShownKinds::setShown(ShownKinds::SymbolTexts, true);
		ShownKinds::apply(scene, ShownKinds::SymbolTexts);
		QVERIFY(!text->isVisible());
		ShownKinds::setShown(ShownKinds::CrossReferences, true);
		ShownKinds::apply(scene, ShownKinds::CrossReferences);
		QVERIFY(text->isVisible());
	}

	// isHidden() follows the parents: a cross-reference under a symbol text.
	void hiddenThroughParent()
	{
		auto symbol = add();
		auto label = add(symbol);
		auto xref = add(label);
		ShownKinds::tag(label, ShownKinds::SymbolTexts);
		ShownKinds::tag(xref, ShownKinds::CrossReferences);
		QVERIFY(!ShownKinds::isHidden(xref));

		ShownKinds::setShown(ShownKinds::SymbolTexts, false);
		QVERIFY(ShownKinds::isHidden(xref));
		QVERIFY(!ShownKinds::isHidden(symbol));
	}

	// Hidden means safe from Select All, copy and delete: Qt will not
	// select it, and drops a selection it had.
	void hiddenIsNotSelectable()
	{
		auto text = add();
		ShownKinds::tag(text, ShownKinds::FreeTexts);
		text->setSelected(true);
		ShownKinds::setShown(ShownKinds::FreeTexts, false);
		ShownKinds::apply(scene, ShownKinds::FreeTexts);
		QVERIFY(!text->isSelected());
		text->setSelected(true);
		QVERIFY(scene->selectedItems().isEmpty());
		QVERIFY(scene->items(QRectF(-5, -5, 20, 20)).isEmpty());
	}
};

QTEST_MAIN(tst_shownkinds)
#include "tst_shownkinds.moc"
