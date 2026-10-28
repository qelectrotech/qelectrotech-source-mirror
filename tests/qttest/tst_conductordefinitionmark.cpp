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
	A conductor's colour and section shown at a slanted stroke across it:
	the stroke stands on the conductor where its properties say, comes
	and goes with them, and is saved with the conductor.
*/
#include "qetapptest.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductordefinitionmark.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetmessagebox.h"

#include <QDomDocument>

class TstConductorDefinitionMark : public QObject
{
	Q_OBJECT

	private:
		Conductor *m_conductor = nullptr;

		Conductor *m_second = nullptr;

		ConductorDefinitionMark *mark() const { return markOf(m_conductor); }

		static ConductorDefinitionMark *markOf(const Conductor *conductor)
		{
			for (QGraphicsItem *child : conductor->childItems())
				if (auto *found = qgraphicsitem_cast<ConductorDefinitionMark *>(child))
					return found;
			return nullptr;
		}

		void setMark(bool on, qreal pos) { setMark(m_conductor, on, pos); }

		static void setMark(Conductor *conductor, bool on, qreal pos)
		{
			ConductorProperties p = conductor->properties();
			p.m_definition_mark = on;
			p.m_definition_mark_pos = pos;
			p.m_wire_color = QStringLiteral("BK");
			p.m_wire_section = QStringLiteral("10");
			conductor->setProperties(p);
		}

	private slots:
		void initTestCase()
		{
			QET::QetMessageBox::setNonInteractive(true);
			Diagram *diagram = QetAppTest::diagram();
			QVERIFY(diagram);
			const ElementsLocation location(
				QStringLiteral("common://10_electric/10_allpole/114_connections/corner.elmt"));
			int state = 0;
			Element *a = ElementFactory::Instance()->createElement(location, nullptr, &state);
			Element *b = ElementFactory::Instance()->createElement(location, nullptr, &state);
			QVERIFY(a && b);
			diagram->addItem(a);
			diagram->addItem(b);
			a->setPos(100, 100);
			b->setPos(300, 300);
			m_conductor = new Conductor(a->terminals().first(), b->terminals().first());
			diagram->addItem(m_conductor);
			QVERIFY(m_conductor->isValid());

				//b is a corner: a second conductor goes on from it
			Element *c = ElementFactory::Instance()->createElement(location, nullptr, &state);
			QVERIFY(c);
			diagram->addItem(c);
			c->setPos(500, 300);
			m_second = new Conductor(b->terminals().last(), c->terminals().first());
			diagram->addItem(m_second);
			QVERIFY(m_second->isValid());
		}

		void pointsAlongAPolyline()
		{
			const QList<QPointF> l {{0, 0}, {0, 40}, {30, 40}};
			bool vertical = false;
			QCOMPARE(ConductorDefinitionMark::pointAt(l, 10, &vertical), QPointF(0, 10));
			QVERIFY(vertical);
			QCOMPARE(ConductorDefinitionMark::pointAt(l, 50, &vertical), QPointF(10, 40));
			QVERIFY(!vertical);
			QCOMPARE(ConductorDefinitionMark::pointAt(l, 500), QPointF(30, 40));
			QCOMPARE(ConductorDefinitionMark::distanceOf(l, QPointF(12, 47)), 52.0);
			QCOMPARE(ConductorDefinitionMark::defaultDistance(l), 20.0);
		}

		void noMarkUnlessAsked()
		{
			QVERIFY(!mark());
		}

		void markStandsOnTheConductor()
		{
			setMark(true, -1);
			QVERIFY(mark());
			setMark(true, 20);
			QCOMPARE(mark()->distance(), 20.0);
			const QPointF on = mark()->pos();
			const QPainterPath path = m_conductor->path();
			QVERIFY2(path.boundingRect().adjusted(-1, -1, 1, 1).contains(on),
				 "the stroke left the conductor");
		}

		void theTextMakesWayForTheStroke()
		{
			QVERIFY(!m_conductor->textItem()->isVisible());
		}

		void oneStrokeForARunTurnedByCorners()
		{
			setMark(m_second, true, -1);
			QCOMPARE(int(bool(markOf(m_conductor))) + int(bool(markOf(m_second))), 1);
			setMark(m_second, false, -1);
			QVERIFY(markOf(m_conductor));
			QVERIFY(!markOf(m_second));
		}

		void savedWithTheConductor()
		{
			ConductorProperties p = m_conductor->properties();
			QDomDocument document;
			QDomElement element = document.createElement(QStringLiteral("conductor"));
			p.toXml(element);
			QCOMPARE(element.attribute(QStringLiteral("definition_mark")), QStringLiteral("1"));
			ConductorProperties read;
			read.fromXml(element);
			QVERIFY(read.m_definition_mark);
			QCOMPARE(read.m_definition_mark_pos, 20.0);
		}

		void switchedOffItGoes()
		{
			setMark(false, 20);
			QVERIFY(!mark());
			QDomDocument document;
			QDomElement element = document.createElement(QStringLiteral("conductor"));
			m_conductor->properties().toXml(element);
			QVERIFY(!element.hasAttribute(QStringLiteral("definition_mark")));
		}
};

int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("QElectroTech");
	QCoreApplication::setApplicationName("QElectroTech");
	QTemporaryDir home;
	if (!home.isValid())
		return 1;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.filePath("settings"));
	QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.filePath("settings"));
	QETApp::applyDirectoryArguments(QETArguments(QList<QString>{
		"--config-dir=" + home.filePath("config"),
		"--data-dir=" + home.filePath("data"),
		QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/")}));
	QETApp qetapp;
	QCoreApplication::processEvents();
	TstConductorDefinitionMark test;
	return QTest::qExec(&test, argc, argv);
}

#include "tst_conductordefinitionmark.moc"
