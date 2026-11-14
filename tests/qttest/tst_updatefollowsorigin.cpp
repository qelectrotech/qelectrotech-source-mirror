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
	A symbol redrawn with its insertion point elsewhere -- the drawing moved
	as a whole -- reaches a project with "Update elements from the
	collections": with the setting on, each terminal keeps the uuid of the
	one it was moved from, and each placed symbol is moved back by the
	offset, turned as it is, so its terminals, conductors and texts stay
	where they are.
*/
#include "qetapptest.h"
#include "ElementsCollection/terminaluuids.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetapp.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QSettings>
#include <QTemporaryDir>

class tst_updatefollowsorigin : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;
		QTemporaryDir m_collection;

			/// A vertical line with a terminal at each end, at @p top and
			/// @p top + 20, the frame @p frame
		static QString definition(const QString &frame, int top,
								  const QString &uuid_top, const QString &uuid_bottom)
		{
			return QStringLiteral(
				R"(<definition type="element" link_type="simple" %1>)"
				R"(<uuid uuid="{7d1f4c2a-5b3e-4a8f-9c61-2e4b6a8d0f13}"/>)"
				R"(<names><name lang="en">v</name></names>)"
				R"(<description><line x1="0" y1="%2" x2="0" y2="%3" style="line-style:normal;line-weight:normal;filling:none;color:black"/>)"
				R"(<terminal x="0" y="%2" orientation="n" uuid="%4"/>)"
				R"(<terminal x="0" y="%3" orientation="s" uuid="%5"/></description></definition>)")
					.arg(frame).arg(top).arg(top + 20).arg(uuid_top, uuid_bottom);
		}

			/// The origin in the middle, as most library symbols have it
		static QString middle()
		{
			return definition(QStringLiteral(R"(width="20" height="40" hotspot_x="10" hotspot_y="20")"), -10,
							  QStringLiteral("{1a000000-0000-4000-8000-000000000001}"),
							  QStringLiteral("{1a000000-0000-4000-8000-000000000002}"));
		}

			/// The same drawing with the origin on its top terminal, and
			/// terminal uuids of its own
		static QString onTerminal()
		{
			return definition(QStringLiteral(R"(width="20" height="30" hotspot_x="10" hotspot_y="0")"), 0,
							  QStringLiteral("{2b000000-0000-4000-8000-000000000001}"),
							  QStringLiteral("{2b000000-0000-4000-8000-000000000002}"));
		}

		void writeCollection(const QString &xml)
		{
			QFile file(m_collection.filePath(QStringLiteral("v.elmt")));
			QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
			file.write(xml.toUtf8());
		}

		void embed(const QString &xml)
		{
			auto *collection = m_diagram->project()->embeddedElementCollection();
			collection->removeElement(QStringLiteral("import/v.elmt"));
			QDomDocument document;
			document.setContent(xml);
			QVERIFY(collection->addElementDefinition(QStringLiteral("import"), QStringLiteral("v"),
													 document.documentElement()));
		}

		Element *place(const QPointF &pos, qreal rotation)
		{
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/v.elmt"), m_diagram->project()),
				nullptr, &state);
			if (state || !element)
				return nullptr;
			m_diagram->addItem(element);
			element->setPos(pos);
			element->setRotation(rotation);
			return element;
		}

		static QList<QPointF> terminalsOnFolio(const Element *element)
		{
			QList<QPointF> points;
			for (const Terminal *terminal : element->terminals())
				points << terminal->dockConductor();
			return points;
		}

		static QDomElement parsed(const QString &xml, QDomDocument &document)
		{
			document.setContent(xml);
			return document.documentElement();
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

		void offFromTheStart()
		{
			QSettings().remove(QStringLiteral("diagrameditor/update_follows_moved_origin"));
			QVERIFY(!Element::updateFollowsMovedOrigin());
			Element::setUpdateFollowsMovedOrigin(true);
			QVERIFY(Element::updateFollowsMovedOrigin());
			Element::setUpdateFollowsMovedOrigin(false);
		}

			//One offset fits, or none
		void theCommonOffset()
		{
			QDomDocument a, b, c;
			QPointF offset;
			QVERIFY(TerminalUuids::commonOffset(parsed(middle(), a), parsed(onTerminal(), b), &offset));
			QCOMPARE(offset, QPointF(0, 10));

				//A terminal moved on its own: not a moved drawing
			QString other = onTerminal();
			other.replace(QStringLiteral(R"(<terminal x="0" y="20")"), QStringLiteral(R"(<terminal x="10" y="20")"));
			QVERIFY(!TerminalUuids::commonOffset(parsed(middle(), a), parsed(other, c), &offset));

				//Same places, another orientation: no fit either
			other = onTerminal();
			other.replace(QStringLiteral(R"(y="0" orientation="n")"), QStringLiteral(R"(y="0" orientation="w")"));
			QVERIFY(!TerminalUuids::commonOffset(parsed(middle(), a), parsed(other, c), &offset));
		}

		void theUuidsGoWithTheTerminals()
		{
			QDomDocument a, b;
			const QDomElement old_definition = parsed(middle(), a);
			QDomElement new_definition = parsed(onTerminal(), b);
			TerminalUuids::keepShifted(old_definition, new_definition, QPointF(0, 10));
			const QDomNodeList terminals = new_definition.elementsByTagName(QStringLiteral("terminal"));
			QCOMPARE(terminals.at(0).toElement().attribute(QStringLiteral("uuid")),
					 QStringLiteral("{1a000000-0000-4000-8000-000000000001}"));
			QCOMPARE(terminals.at(1).toElement().attribute(QStringLiteral("uuid")),
					 QStringLiteral("{1a000000-0000-4000-8000-000000000002}"));
		}

			//Two turned symbols and the conductor between them stay put
		void placedSymbolsStayPut()
		{
			embed(middle());
			Element *first = place(QPointF(200, 200), 90);
			Element *second = place(QPointF(300, 200), 270);
			QVERIFY(first && second);
			auto *text = new DynamicElementTextItem(first);
			first->addDynamicTextItem(text);
			text->setPos(30, 0);
			auto *conductor = new Conductor(first->terminals().at(1), second->terminals().at(1));
			m_diagram->addItem(conductor);

			const QList<QPointF> first_terminals = terminalsOnFolio(first);
			const QList<QPointF> second_terminals = terminalsOnFolio(second);
			const QPointF text_on_folio = text->scenePos();
			const QPainterPath wire = conductor->mapToScene(conductor->path());

			writeCollection(onTerminal());
			QETProject *project = m_diagram->project();
			const auto outdated = project->outdatedElements();
			QCOMPARE(outdated.size(), 1);
			project->updateElement(outdated.first(), true);

				//The embedded terminals carry the uuids of the ones they were moved from
			const QDomNodeList terminals = outdated.first().embedded.xml().elementsByTagName(QStringLiteral("terminal"));
			QCOMPARE(terminals.at(0).toElement().attribute(QStringLiteral("uuid")),
					 QStringLiteral("{1a000000-0000-4000-8000-000000000001}"));

			for (Element *element : {first, second}) {
				QCOMPARE(element->reloadPicture(true), Element::ReloadPictureResult::GeometryChanged);
				QVERIFY(element->followMovedOrigin());
				QCOMPARE(element->reloadPicture(true), Element::ReloadPictureResult::Reloaded);
			}

				//Turned by 90 degrees, the offset (0, 10) is (-10, 0) on the folio
			QCOMPARE(first->pos(), QPointF(210, 200));
			QCOMPARE(second->pos(), QPointF(290, 200));
			QCOMPARE(first->hotspot(), QPoint(10, 0));
			QCOMPARE(terminalsOnFolio(first), first_terminals);
			QCOMPARE(terminalsOnFolio(second), second_terminals);
			QCOMPARE(text->scenePos(), text_on_folio);
			QCOMPARE(conductor->mapToScene(conductor->path()), wire);

				//Nothing left to move
			QVERIFY(!first->followMovedOrigin());

				//Saved and opened again, the conductor finds its terminals
			QTemporaryDir saved;
			const QString path = saved.filePath(QStringLiteral("moved.qet"));
			project->setFilePath(path);
			QVERIFY(project->write().isOk());
			{
				QETProject again(path);
				QList<Conductor *> conductors;
				for (Diagram *diagram : again.diagrams())
					conductors << diagram->conductors();
				QCOMPARE(conductors.size(), 1);
				const QList<QPointF> ends {conductors.first()->terminal1->dockConductor(),
										   conductors.first()->terminal2->dockConductor()};
				QVERIFY(ends.contains(first_terminals.at(1)));
				QVERIFY(ends.contains(second_terminals.at(1)));
			}

			delete conductor;
			delete first;
			delete second;
		}

			//Without the setting the terminals keep what keep() gives them
		void offLeavesTheUuidsAlone()
		{
			embed(middle());
			writeCollection(onTerminal());
			QETProject *project = m_diagram->project();
			const auto outdated = project->outdatedElements();
			QCOMPARE(outdated.size(), 1);
			project->updateElement(outdated.first());
			const QDomNodeList terminals = outdated.first().embedded.xml().elementsByTagName(QStringLiteral("terminal"));
			QVERIFY(terminals.at(0).toElement().attribute(QStringLiteral("uuid"))
					!= QStringLiteral("{1a000000-0000-4000-8000-000000000001}"));
		}
};

QET_APP_TEST_MAIN(tst_updatefollowsorigin)
#include "tst_updatefollowsorigin.moc"
