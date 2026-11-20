// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetapp.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QScopeGuard>
#include <QtTest>

/**
	A symbol embedded in a project is also written as text,
	"project<id>+embed://...", and read back when the symbol is edited from
	the collection panel or placed on a folio. Only a one-digit id was read
	back, so from the eleventh project opened in a session on, editing an
	embedded symbol did nothing and placing a symbol failed, with no message
	(#1958).
*/
class tst_projectidlocation : public QObject
{
	Q_OBJECT

		//Registers @p project, again and again until it gets @p id
	static bool registerAs(QETProject *project, int id)
	{
		while (QETApp::registerProject(project)) {
			const int got = QETApp::projectId(project);
			if (got >= id) return got == id;
			QETApp::unregisterProject(project);
		}
		return false;
	}

	private slots:
		void roundTrip_data()
		{
			QTest::addColumn<int>("id");
			QTest::newRow("9") << 9;
			QTest::newRow("10") << 10;
			QTest::newRow("100") << 100;
		}

		void roundTrip()
		{
			QFETCH(int, id);
			QETProject project;
			const bool registered_as_id = registerAs(&project, id);
			const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
			QVERIFY2(registered_as_id, "could not register the project with this id");

			QDomDocument definition;
			QVERIFY(definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">Probe</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/></description></definition>)")));
			QVERIFY(project.embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("probe"), definition.documentElement()));

			const ElementsLocation embedded(QStringLiteral("embed://import/probe.elmt"), &project);
			QVERIFY(embedded.exist());
			const QString text = embedded.projectCollectionPath();
			QCOMPARE(text, QStringLiteral("project%1+embed://import/probe.elmt").arg(id));

				//What the collection panel's Edit and a drop on a folio do
			const ElementsLocation read_back(text);
			QCOMPARE(read_back.project(), &project);
			QVERIFY(read_back.exist());
			QCOMPARE(read_back.collectionPath(), embedded.collectionPath());

				//What DiagramEventAddElement::buildElement() does next
			int state = -1;
			Element *element = ElementFactory::Instance()->createElement(read_back, nullptr, &state);
			const auto built = qScopeGuard([element]() {delete element;});
			QCOMPARE(state, 0);
			QVERIFY(element);
		}
};

QET_APP_TEST_MAIN(tst_projectidlocation)

#include "tst_projectidlocation.moc"
