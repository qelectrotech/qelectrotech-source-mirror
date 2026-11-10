// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "diagram.h"
#include "editor/elementscene.h"
#include "editor/ui/elementpropertieseditorwidget.h"
#include "diagramcontext.h"
#include "properties/elementdata.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "ui/customelementinfopartwidget.h"
#include "ui/elementinfowidget.h"

#include <QDomDocument>
#include <QFile>
#include <QInputDialog>
#include <QPushButton>
#include <QTimer>
#include <QTreeWidget>
#include <QLineEdit>
#include <QTemporaryDir>

/**
	A symbol can define its own custom properties (discussion #561):
	<customInformations> in its definition. Every placed copy then has
	them, filled in or not, and an empty one adds nothing to the saved
	project. A typed property name is turned into a key every version
	can open.
*/
class tst_symbolcustominfo : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

		//every_feature.qet with every symbol of its collection
		//defining address_a
	QString fixtureDefiningAddressA()
	{
		QFile in(QFINDTESTDATA("fixtures/every_feature.qet"));
		if (!in.open(QIODevice::ReadOnly)) return QString();
		QString xml = QString::fromUtf8(in.readAll());
		xml.replace(QStringLiteral("</definition>"),
					QStringLiteral("<customInformations>"
								   "<customInformation name=\"address_a\"/>"
								   "</customInformations></definition>"));
		const QString path = m_dir.filePath(QStringLiteral("custom.qet"));
		QFile out(path);
		if (!out.open(QIODevice::WriteOnly)) return QString();
		out.write(xml.toUtf8());
		return path;
	}

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

	static QString savedInformation(const QDomDocument &xml, const QUuid &uuid)
	{
		const auto elements = xml.elementsByTagName(QStringLiteral("element"));
		for (int i = 0; i < elements.size(); ++i) {
			const QDomElement e = elements.at(i).toElement();
			if (QUuid(e.attribute(QStringLiteral("uuid"))) != uuid) continue;
			QString text;
			QTextStream stream(&text);
			e.firstChildElement(QStringLiteral("elementInformations")).save(stream, 0);
			return text;
		}
		return QStringLiteral("<not found>");
	}

private slots:
	void typedNamesBecomeKeys()
	{
		QCOMPARE(QETInformation::toInfoKey(QStringLiteral("Address A")), QStringLiteral("address_a"));
		QCOMPARE(QETInformation::toInfoKey(QStringLiteral("AddressA")), QStringLiteral("addressa"));
		QCOMPARE(QETInformation::toInfoKey(QStringLiteral("Adresse É!")), QStringLiteral("adresse_e"));
		QCOMPARE(QETInformation::toInfoKey(QStringLiteral("I/O-2")), QStringLiteral("io-2"));
		QVERIFY(QETInformation::toInfoKey(QStringLiteral("§%")).isEmpty());
	}

	void definitionKeepsItsCustomProperties()
	{
		QDomDocument doc;
		QVERIFY(doc.setContent(QStringLiteral(
			"<definition type=\"element\" link_type=\"simple\">"
			"<elementInformations>"
			"<elementInformation name=\"address_b\" show=\"1\">I0.1</elementInformation>"
			"</elementInformations>"
			"<customInformations>"
			"<customInformation name=\"address_a\"/>"
			"<customInformation name=\"address_b\"/>"
			"<customInformation name=\"Not A Key\"/>"
			"</customInformations>"
			"</definition>")));
		ElementData data;
		QVERIFY(data.fromXml(doc.documentElement()));
		QCOMPARE(data.m_custom_info_keys,
				 QStringList({ QStringLiteral("address_a"), QStringLiteral("address_b") }));

		const DiagramContext info = data.withCustomInfo(data.m_informations);
		QVERIFY(info.contains(QStringLiteral("address_a")));
		QCOMPARE(info.value(QStringLiteral("address_a")).toString(), QString());
		QCOMPARE(info.value(QStringLiteral("address_b")).toString(), QStringLiteral("I0.1"));

		QDomDocument out;
		const QDomElement xml = data.customInfoToXml(out);
		QCOMPARE(xml.elementsByTagName(QStringLiteral("customInformation")).size(), 2);
	}

	void everyCopyHasThePropertyAndNothingIsSaved()
	{
		const QString path = fixtureDefiningAddressA();
		QVERIFY(!path.isEmpty());
		QETProject project(path);
		QCOMPARE(project.state(), QETProject::Ok);
		Element *e = firstSymbol(project);
		QVERIFY(e);
		QVERIFY(e->elementData().m_custom_info_keys.contains(QStringLiteral("address_a")));

		const DiagramContext info = e->elementInformations();
		QVERIFY(info.contains(QStringLiteral("address_a")));
		QCOMPARE(info.value(QStringLiteral("address_a")).toString(), QString());
		QVERIFY(QETInformation::elementCustomInfoKeys(info.keys())
				.contains(QStringLiteral("address_a")));

			//An empty property shows as nothing, not as %{address_a}
		auto *composite = new DynamicElementTextItem(e);
		e->addDynamicTextItem(composite);
		composite->setCompositeText(QStringLiteral("[%{address_a}]"));
		composite->setTextFrom(DynamicElementTextItem::CompositeText);
		QCOMPARE(composite->toPlainText(), QStringLiteral("[]"));

			//The property panel shows the row and, untouched, gives it back
			//as the element holds it: no change
		{
			ElementInfoWidget panel(e);
			const DiagramContext shown = panel.currentInfo();
			QVERIFY(shown.contains(QStringLiteral("address_a")));
			QCOMPARE(shown.value(QStringLiteral("address_a")), info.value(QStringLiteral("address_a")));

				//Defined by the symbol: its name cannot be changed here
			CustomElementInfoPartWidget *row = nullptr;
			for (auto *w : panel.findChildren<CustomElementInfoPartWidget *>())
				if (w->key() == QLatin1String("address_a")) row = w;
			QVERIFY(row);
			QVERIFY(row->findChildren<QLineEdit *>().first()->isReadOnly());
		}

			//Nothing reaches the file while it is empty
		QVERIFY(!savedInformation(project.toXml(), e->uuid())
				.contains(QStringLiteral("address_a")));

		DiagramContext filled = e->elementInformations();
		filled.addValue(QStringLiteral("address_a"), QStringLiteral("I0.0"));
		e->setElementInformations(filled);
		QCOMPARE(composite->toPlainText(), QStringLiteral("[I0.0]"));
		QVERIFY(savedInformation(project.toXml(), e->uuid())
				.contains(QStringLiteral("I0.0")));
	}

	void symbolEditorAddsAndRemovesThem()
	{
		ElementData data;
		data.m_type = ElementData::Simple;
		data.m_custom_info_keys = { QStringLiteral("address_a") };
		data.m_informations.addValue(QStringLiteral("address_a"), QStringLiteral("I0.0"));

		ElementPropertiesEditorWidget dialog(data);
		auto *tree = dialog.findChild<QTreeWidget *>(QStringLiteral("m_tree"));
		QVERIFY(tree);
		auto row = [tree](const QString &key) -> QTreeWidgetItem * {
			for (int i = 0; i < tree->topLevelItemCount(); ++i)
				if (tree->topLevelItem(i)->data(0, Qt::UserRole).toString() == key)
					return tree->topLevelItem(i);
			return nullptr;
		};
		auto button = [&dialog](const QString &text) -> QPushButton * {
			for (auto *b : dialog.findChildren<QPushButton *>())
				if (b->text() == text) return b;
			return nullptr;
		};
		QVERIFY(row(QStringLiteral("address_a")));
		QCOMPARE(row(QStringLiteral("address_a"))->text(1), QStringLiteral("I0.0"));

			//Add "Address B": the name is turned into a key
		QPushButton *add = button(QStringLiteral("Add a custom property"));
		QVERIFY(add);
		QTimer::singleShot(0, [] {
			for (QWidget *w : QApplication::topLevelWidgets())
				if (auto *input = qobject_cast<QInputDialog *>(w)) {
					input->setTextValue(QStringLiteral("Address B"));
					input->accept();
				}
		});
		add->click();
		QVERIFY(row(QStringLiteral("address_b")));

			//Remove address_a
		QPushButton *remove = button(QStringLiteral("Remove the custom property"));
		QVERIFY(remove);
		tree->setCurrentItem(row(QStringLiteral("address_a")));
		QVERIFY(remove->isEnabled());
		remove->click();
		QVERIFY(!row(QStringLiteral("address_a")));

		QVERIFY(QMetaObject::invokeMethod(&dialog, "on_m_buttonBox_accepted"));
		const ElementData edited = dialog.editedData();
		QCOMPARE(edited.m_custom_info_keys, QStringList({ QStringLiteral("address_b") }));
		QVERIFY2(!edited.m_informations.contains(QStringLiteral("address_a")),
				 "a removed property takes its default with it");

			//The symbol file carries the list
		ElementScene scene(nullptr, nullptr);
		scene.setElementData(edited);
		const QDomElement custom = scene.toXml().documentElement()
				.firstChildElement(QStringLiteral("customInformations"));
		QCOMPARE(custom.firstChildElement(QStringLiteral("customInformation"))
				 .attribute(QStringLiteral("name")), QStringLiteral("address_b"));
	}

	void projectWithoutCustomPropertiesIsUnchanged()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		Element *e = firstSymbol(project);
		QVERIFY(e);
		QVERIFY(e->elementData().m_custom_info_keys.isEmpty());
		QVERIFY(!e->elementInformations().contains(QStringLiteral("address_a")));
	}
};

QET_APP_TEST_MAIN(tst_symbolcustominfo)
#include "tst_symbolcustominfo.moc"
