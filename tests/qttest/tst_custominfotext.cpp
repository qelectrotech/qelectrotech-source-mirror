// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "diagram.h"
#include "diagramcontext.h"
#include "properties/elementdata.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "autoNum/assignvariables.h"
#include "ui/compositetexteditdialog.h"
#include "ui/dynamicelementtextmodel.h"

#include <QComboBox>
#include <QStyleOptionViewItem>
#include <QWidget>

#include <memory>

/**
	A symbol's user-defined custom property (#642, "Add a custom
	property") can be shown on the drawing (discussion #561): as the
	information of a text and as %{key} in a composite text, and both
	follow when the value is edited.
*/
class tst_custominfotext : public QObject
{
	Q_OBJECT

		//A symbol that shows its own information: not linked to another
		//(a contact shows its coil's), not a report, not a terminal
	static Element *firstSymbol(QETProject &project)
	{
		for (Diagram *d : project.diagrams())
			for (Element *e : d->elements())
				if (e->linkType() == Element::Simple
					&& e->elementData().m_type != ElementData::Terminal)
					return e;
		return nullptr;
	}

	static void setInfo(Element *e, const QString &key, const QString &value)
	{
		DiagramContext dc = e->elementInformations();
		dc.addValue(key, value);
		e->setElementInformations(dc);
	}

private slots:
	void customKeysAreTheUnknownOnes()
	{
		const QStringList keys = { QStringLiteral("label"),
								   QStringLiteral("plc_address"),
								   QStringLiteral("formula_id"),
								   QStringLiteral("auto_num_locked"),
								   QStringLiteral("label_shown"),
								   QStringLiteral("exclude_from_bom"),
								   QStringLiteral("address_b"),
								   QStringLiteral("address_a") };
		QCOMPARE(QETInformation::elementCustomInfoKeys(keys),
				 QStringList({ QStringLiteral("address_a"),
							   QStringLiteral("address_b") }));
	}

	void compositeTextReplacesACustomKey()
	{
		DiagramContext dc;
		dc.addValue(QStringLiteral("label"), QStringLiteral("-A1"));
		dc.addValue(QStringLiteral("address_a"), QStringLiteral("I0.0"));
		dc.addValue(QStringLiteral("io-card_2"), QStringLiteral("Q4.7"));

		QCOMPARE(autonum::AssignVariables::replaceVariable(
					 QStringLiteral("%{label} %{address_a} %{io-card_2}"), dc),
				 QStringLiteral("-A1 I0.0 Q4.7"));
			//A key the symbol does not have stays as it was typed
		QCOMPARE(autonum::AssignVariables::replaceVariable(
					 QStringLiteral("%{address_z}"), dc),
				 QStringLiteral("%{address_z}"));
	}

	void textsOnTheSymbolFollowTheValue()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		Element *e = firstSymbol(project);
		QVERIFY(e);
		setInfo(e, QStringLiteral("address_a"), QStringLiteral("I0.0"));

		auto *info_text = new DynamicElementTextItem(e);
		e->addDynamicTextItem(info_text);
		info_text->setTextFrom(DynamicElementTextItem::ElementInfo);
		info_text->setInfoName(QStringLiteral("address_a"));

		auto *composite = new DynamicElementTextItem(e);
		e->addDynamicTextItem(composite);
		composite->setCompositeText(QStringLiteral("In: %{address_a}"));
		composite->setTextFrom(DynamicElementTextItem::CompositeText);

		QCOMPARE(info_text->toPlainText(), QStringLiteral("I0.0"));
		QCOMPARE(composite->toPlainText(), QStringLiteral("In: I0.0"));

		setInfo(e, QStringLiteral("address_a"), QStringLiteral("I1.3"));
		QCOMPARE(info_text->toPlainText(), QStringLiteral("I1.3"));
		QCOMPARE(composite->toPlainText(), QStringLiteral("In: I1.3"));
	}

	void compositeDialogOffersTheCustomKey()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		Element *e = firstSymbol(project);
		QVERIFY(e);
		setInfo(e, QStringLiteral("address_a"), QStringLiteral("I0.0"));

		auto *deti = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);
		deti->setTextFrom(DynamicElementTextItem::CompositeText);

		CompositeTextEditDialog dialog(deti);
		auto *combo = dialog.findChild<QComboBox *>(QStringLiteral("m_info_cb"));
		QVERIFY(combo);
		const int index = combo->findData(QStringLiteral("%{address_a}"));
		QVERIFY(index >= 0);
		QCOMPARE(combo->itemText(index), QStringLiteral("address_a"));
	}
	void textTableOffersTheCustomKey()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		Element *e = firstSymbol(project);
		QVERIFY(e);
		setInfo(e, QStringLiteral("address_a"), QStringLiteral("I0.0"));

		auto *deti = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);
		deti->setTextFrom(DynamicElementTextItem::ElementInfo);
		deti->setInfoName(QStringLiteral("address_a"));

		DynamicElementTextModel model(e);
		QStandardItem *row = model.itemFromIndex(model.indexFromText(deti));
		QVERIFY(row);
		QStandardItem *info_cell = nullptr;
		for (int r = 0; r < row->rowCount(); ++r)
			if (QStandardItem *c = row->child(r, 1))
				if (c->data(Qt::UserRole + 1).toInt() == DynamicElementTextModel::infoText)
					info_cell = c;
		QVERIFY(info_cell);
			//A custom key has no translation: the table shows the key
		QCOMPARE(info_cell->data(Qt::DisplayRole).toString(), QStringLiteral("address_a"));

		QWidget parent;
		DynamicTextItemDelegate delegate;
		std::unique_ptr<QWidget> editor(delegate.createEditor(
					&parent, QStyleOptionViewItem(), info_cell->index()));
		auto *combo = qobject_cast<QComboBox *>(editor.get());
		QVERIFY(combo);
		const int index = combo->findData(QStringLiteral("address_a"));
		QVERIFY(index >= 0);
		QCOMPARE(combo->itemText(index), QStringLiteral("address_a"));
	}
};

QET_APP_TEST_MAIN(tst_custominfotext)
#include "tst_custominfotext.moc"
