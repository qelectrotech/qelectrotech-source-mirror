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
	Editing a placed element reaches the folio. Saved into the project's
	copy, the placed elements are redrawn at once, taking over the frame
	the element editor recomputes. With "find and edit in the collection",
	finding and editing go to the collection element it was taken from,
	and saving that one updates the copy of the open project.
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "editor/ui/qetelementeditor.h"
#include "factory/elementfactory.h"
#include "qetapp.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "undocommand/resetsymboltextscommand.h"

#include <QSettings>
#include <QTemporaryDir>

class tst_editcollectionsource : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;
		QTemporaryDir m_collection;
		const QString m_uuid = QStringLiteral("{0e7c5a43-8d2e-4d0f-a6a1-3c3f6b0b7a01}");

		static QString definition(const QString &frame, const QString &terminal_uuid, int text_y)
		{
			return QStringLiteral(
				R"(<definition type="element" link_type="simple" %1>)"
				R"(<uuid uuid="{4b5c1b3e-2f7a-4b8e-9b55-0d8a5e2f1a10}"/>)"
				R"(<names><name lang="en">t</name></names>)"
				R"(<description><line x1="0" y1="-10" x2="0" y2="10" style="line-style:normal;line-weight:normal;filling:none;color:black"/>)"
				R"(<text text="PE" x="2" y="%3" size="4"/>)"
				R"(<terminal x="0" y="-10" orientation="n" uuid="%2"/></description></definition>)")
					.arg(frame, terminal_uuid).arg(text_y);
		}

		static QDomDocument document(const QString &xml)
		{
			QDomDocument d;
			d.setContent(xml);
			return d;
		}

		void writeCollection(const QString &xml)
		{
			QFile file(m_collection.filePath(QStringLiteral("t.elmt")));
			QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
			file.write(xml.toUtf8());
		}

		ElementsLocation embedded() const
		{
			return ElementsLocation(QStringLiteral("embed://import/t.elmt"), m_diagram->project());
		}

		Element *place(const QString &xml)
		{
			auto *collection = m_diagram->project()->embeddedElementCollection();
			collection->removeElement(QStringLiteral("import/t.elmt"));
			if (!collection->addElementDefinition(QStringLiteral("import"), QStringLiteral("t"),
							      document(xml).documentElement()))
				return nullptr;
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(embedded(), nullptr, &state);
			if (state || !element)
				return nullptr;
			m_diagram->addItem(element);
			element->setPos(200, 200);
			return element;
		}

		static QString frame(int w, int h, int hx, int hy)
		{
			return QStringLiteral(R"(width="%1" height="%2" hotspot_x="%3" hotspot_y="%4")")
					.arg(w).arg(h).arg(hx).arg(hy);
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

		void cleanup()
		{
			ElementsLocation::setFindsAndEditsCollectionSource(false);
		}

			//The imported copy knows the collection element it came from;
			//find and edit go there only with the setting
		void source()
		{
			writeCollection(definition(frame(20, 20, 5, 10), m_uuid, -4));
			Element *element = place(definition(frame(20, 20, 5, 10), m_uuid, -4));
			QVERIFY(element);
			const ElementsLocation copy = element->location();
			QCOMPARE(copy.collectionSource().fileSystemPath(),
				 QFileInfo(m_collection.filePath(QStringLiteral("t.elmt"))).absoluteFilePath());
			QCOMPARE(copy.forFindAndEdit(), copy);
			ElementsLocation::setFindsAndEditsCollectionSource(true);
			QCOMPARE(copy.forFindAndEdit(), copy.collectionSource());
				//Not an imported copy: no source
			QVERIFY(copy.collectionSource().collectionSource().isNull());
			delete element;
		}

			//Saved into the project's copy with a new frame, as the element
			//editor writes it: the placed element shows it at once
		void savedIntoProjectCopy()
		{
			Element *element = place(definition(frame(20, 20, 5, 10), m_uuid, -4));
			QVERIFY(element);
			QVERIFY(embedded().setXml(document(definition(frame(30, 40, 15, 20), m_uuid, -2))));
			QETElementEditor::redrawPlacedElements(embedded());
			QCOMPARE(element->size(), QSize(30, 40));
			QCOMPARE(element->hotspot(), QPoint(15, 20));
			QCOMPARE(element->pos(), QPointF(200, 200));
			delete element;
		}

			//Saved into the collection: the open project takes it over
			//only with the setting
		void savedIntoCollection()
		{
			writeCollection(definition(frame(20, 20, 5, 10), m_uuid, -4));
			Element *element = place(definition(frame(20, 20, 5, 10), m_uuid, -4));
			QVERIFY(element);
			const ElementsLocation source = element->location().collectionSource();
			QVERIFY(!source.isNull());

			writeCollection(definition(frame(30, 40, 15, 20), m_uuid, -2));
			QETElementEditor::redrawPlacedElements(source);
			QCOMPARE(m_diagram->project()->outdatedElements().size(), 1);
			QCOMPARE(element->size(), QSize(20, 20));

			ElementsLocation::setFindsAndEditsCollectionSource(true);
			QETElementEditor::redrawPlacedElements(source);
			QVERIFY(m_diagram->project()->outdatedElements().isEmpty());
			QCOMPARE(embedded().xml().attribute(QStringLiteral("width")), QStringLiteral("30"));
			QCOMPARE(element->size(), QSize(30, 40));
			QCOMPARE(element->hotspot(), QPoint(15, 20));
			delete element;
		}

			//A placed symbol keeps its own copy of the symbol's texts; the
			//command lays it out again as the symbol does, and undo puts
			//it back
		void resetSymbolTexts()
		{
			const QString xml = definition(frame(20, 20, 5, 10), m_uuid, -4).replace(
				QStringLiteral("<terminal "),
				QStringLiteral(R"(<dynamic_text x="-40" y="8" text_from="ElementInfo" Halignment="AlignRight" Valignment="AlignTop" text_width="30" font="Liberation Sans,8,-1,5,50,0,0,0,0,0,Regular" uuid="{5ef9ecf5-8bdb-5177-b974-84399b5c28e4}"><text/><info_name>label</info_name></dynamic_text><terminal )"));
			Element *element = place(xml);
			QVERIFY(element);
			QCOMPARE(element->dynamicTextItems().size(), 1);
			DynamicElementTextItem *text = element->dynamicTextItems().first();
			text->setPos(-40, -2);

			ResetSymbolTextsCommand command({element});
			QCOMPARE(command.count(), 1);
			command.redo();
			QCOMPARE(text->pos(), QPointF(-40, 8));
			command.undo();
			QCOMPARE(text->pos(), QPointF(-40, -2));
				//Already where the symbol places it: nothing to do
			command.redo();
			QCOMPARE(ResetSymbolTextsCommand({element}).count(), 0);
			delete element;
		}
};

QET_APP_TEST_MAIN(tst_editcollectionsource)
#include "tst_editcollectionsource.moc"
