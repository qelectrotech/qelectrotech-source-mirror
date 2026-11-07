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
/*
	An element edited in its collection after it was placed reaches the
	project: placing it again reuses the embedded copy, which carries the
	same uuid, so the project lists the embedded elements that differ and
	replaces them. A placed element then takes over a new frame, as long as
	its terminals stay where they are.
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetapp.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QSettings>
#include <QTemporaryDir>

class tst_updateelements : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;
		QTemporaryDir m_collection;

			/// A definition with the frame @p frame and one terminal at (0, -10)
		static QString definition(const QString &frame, const QString &terminal_uuid)
		{
			return QStringLiteral(
				R"(<definition type="element" link_type="simple" %1>)"
				R"(<uuid uuid="{4b5c1b3e-2f7a-4b8e-9b55-0d8a5e2f1a10}"/>)"
				R"(<names><name lang="en">t</name></names>)"
				R"(<description><line x1="0" y1="-10" x2="0" y2="10" style="line-style:normal;line-weight:normal;filling:none;color:black"/>)"
				R"(<terminal x="0" y="-10" orientation="n" uuid="%2"/></description></definition>)")
					.arg(frame, terminal_uuid);
		}

			/// Write @p xml as the collection's element "t.elmt"
		void writeCollection(const QString &xml)
		{
			QFile file(m_collection.filePath(QStringLiteral("t.elmt")));
			QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
			file.write(xml.toUtf8());
		}

			/// Embed @p xml as "import/t.elmt" and place it
		Element *place(const QString &xml)
		{
			auto *collection = m_diagram->project()->embeddedElementCollection();
			collection->removeElement(QStringLiteral("import/t.elmt"));
			QDomDocument document;
			document.setContent(xml);
			if (!collection->addElementDefinition(QStringLiteral("import"), QStringLiteral("t"), document.documentElement()))
				return nullptr;
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/t.elmt"), m_diagram->project()),
				nullptr, &state);
			if (state || !element)
				return nullptr;
			m_diagram->addItem(element);
			element->setPos(200, 200);
			return element;
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			QVERIFY(m_collection.isValid());
			QSettings().setValue(QStringLiteral("elements-collections/custom-collection-path"), m_collection.path());
			QETApp::resetCollectionsPath();
		}

			//The same element, terminal uuid aside: nothing to update
		void unchanged()
		{
			const QString frame = QStringLiteral(R"(width="20" height="20" hotspot_x="5" hotspot_y="10")");
			writeCollection(definition(frame, QStringLiteral("{0e7c5a43-8d2e-4d0f-a6a1-3c3f6b0b7a01}")));
			Element *element = place(definition(frame, QStringLiteral("{0e7c5a43-8d2e-4d0f-a6a1-3c3f6b0b7a02}")));
			QVERIFY(element);
			QVERIFY(m_diagram->project()->outdatedElements().isEmpty());
			delete element;
		}

			//A tighter frame in the collection: listed, replaced, taken over
		void tighterFrame()
		{
			const QString uuid = QStringLiteral("{0e7c5a43-8d2e-4d0f-a6a1-3c3f6b0b7a01}");
			Element *element = place(definition(
				QStringLiteral(R"(width="20" height="20" hotspot_x="5" hotspot_y="10")"), uuid));
			QVERIFY(element);
			writeCollection(definition(
				QStringLiteral(R"(width="10" height="20" hotspot_x="0" hotspot_y="10")"), uuid));

			QETProject *project = m_diagram->project();
			const auto outdated = project->outdatedElements();
			QCOMPARE(outdated.size(), 1);
			QCOMPARE(outdated.first().embedded.collectionPath(false), QStringLiteral("import/t.elmt"));
			project->updateElement(outdated.first());
			QVERIFY(project->outdatedElements().isEmpty());

				//The plain reload keeps the old frame, the update takes the new one
			QCOMPARE(element->reloadPicture(), Element::ReloadPictureResult::GeometryChanged);
			QCOMPARE(element->reloadPicture(true), Element::ReloadPictureResult::Reloaded);
			QCOMPARE(element->size(), QSize(10, 20));
			QCOMPARE(element->hotspot(), QPoint(0, 10));
			QCOMPARE(element->pos(), QPointF(200, 200));
			QVERIFY(!element->boundingRect().contains(QPointF(-4, 0)));
			delete element;
		}

			//A moved terminal: the frame is not taken over
		void movedTerminal()
		{
			Element *element = place(definition(
				QStringLiteral(R"(width="20" height="20" hotspot_x="5" hotspot_y="10")"),
				QStringLiteral("{0e7c5a43-8d2e-4d0f-a6a1-3c3f6b0b7a01}")));
			QVERIFY(element);
			QString moved = definition(
				QStringLiteral(R"(width="10" height="20" hotspot_x="0" hotspot_y="10")"),
				QStringLiteral("{0e7c5a43-8d2e-4d0f-a6a1-3c3f6b0b7a01}"));
			moved.replace(QStringLiteral(R"(<terminal x="0" y="-10")"), QStringLiteral(R"(<terminal x="0" y="10")"));
			writeCollection(moved);

			QETProject *project = m_diagram->project();
			const auto outdated = project->outdatedElements();
			QCOMPARE(outdated.size(), 1);
			project->updateElement(outdated.first());
			QCOMPARE(element->reloadPicture(true), Element::ReloadPictureResult::GeometryChanged);
			QCOMPARE(element->size(), QSize(20, 20));
			delete element;
		}
};

QET_APP_TEST_MAIN(tst_updateelements)
#include "tst_updateelements.moc"
