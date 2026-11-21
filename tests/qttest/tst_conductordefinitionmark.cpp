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
#include "autoNum/assignvariables.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "properties/definitionmarkstyle.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductordefinitionmark.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetmessagebox.h"
#include "qetproject.h"
#include "shownkinds.h"

#include <QGraphicsSceneMouseEvent>

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
			p.m_definition_mark_at = pos;
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
			setMark(true, 0.25);
			QCOMPARE(mark()->distance(), 0.25 * m_conductor->length());
			const QPointF on = mark()->pos();
			const QPainterPath path = m_conductor->path();
			QVERIFY2(path.boundingRect().adjusted(-1, -1, 1, 1).contains(on),
				 "the stroke left the conductor");
		}

			//The conductor text is often the wire number: it stays
		void theTextStaysVisible()
		{
			QVERIFY(m_conductor->textItem()->isVisible());
		}

			//Asked to, the stroke stands in for the text, and gives it back
		void replacesTheTextWhenAsked()
		{
			ConductorProperties p = m_conductor->properties();
			p.m_definition_mark_hides_text = true;
			m_conductor->setProperties(p);
			QVERIFY(!m_conductor->textItem()->isVisible());
			p.m_definition_mark_hides_text = false;
			m_conductor->setProperties(p);
			QVERIFY(m_conductor->textItem()->isVisible());
		}

			//A stroke only follows the wire numbers of View > Show
		void hiddenWithTheWireNumbers()
		{
			ShownKinds::setShown(ShownKinds::WireNumbers, false);
			ShownKinds::apply(m_conductor->scene(), ShownKinds::WireNumbers);
			QVERIFY(!mark()->isVisible());
			ShownKinds::setShown(ShownKinds::WireNumbers, true);
			ShownKinds::apply(m_conductor->scene(), ShownKinds::WireNumbers);
			QVERIFY(mark()->isVisible());
		}

			//On a read-only folio the stroke is not dragged
		void notDraggedReadOnly()
		{
			Diagram *diagram = QetAppTest::diagram();
			diagram->project()->setReadOnly(true);
			QGraphicsSceneMouseEvent own(QEvent::GraphicsSceneMousePress);
			own.setButton(Qt::LeftButton);
			own.setAccepted(false);
			diagram->sendEvent(mark(), &own);
			QVERIFY(!own.isAccepted());
			diagram->project()->setReadOnly(false);
		}

		void oneStrokeForARunTurnedByCorners()
		{
			setMark(m_second, true, -1);
			QCOMPARE(int(bool(markOf(m_conductor))) + int(bool(markOf(m_second))), 1);
				//Another section after the corner: a new run
			ConductorProperties p = m_second->properties();
			p.m_wire_section = QStringLiteral("6");
			m_second->setProperties(p);
			QVERIFY(markOf(m_conductor));
			QVERIFY(markOf(m_second));
			setMark(m_second, false, -1);
			QVERIFY(markOf(m_conductor));
			QVERIFY(!markOf(m_second));
		}

			//The conductor carrying the stroke taken off the folio: the
			//rest of the run shows it
		void runKeepsItsStrokeWhenOneGoes()
		{
			setMark(m_second, true, -1);
			Conductor *shown = markOf(m_conductor) ? m_conductor : m_second;
			Conductor *other = shown == m_conductor ? m_second : m_conductor;
			QVERIFY(!markOf(other));
			Diagram *diagram = QetAppTest::diagram();
			diagram->removeItem(shown);
			QVERIFY(markOf(other));
			diagram->addItem(shown);
			setMark(m_second, false, -1);
			QVERIFY(markOf(m_conductor));
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
			QCOMPARE(read.m_definition_mark_at, 0.25);
		}

		void switchedOffItGoes()
		{
			setMark(false, 0.25);
			QVERIFY(!mark());
			QDomDocument document;
			QDomElement element = document.createElement(QStringLiteral("conductor"));
			m_conductor->properties().toXml(element);
			QVERIFY(!element.hasAttribute(QStringLiteral("definition_mark")));
		}

			//The customized drawing of the stroke goes out and comes
			//back whole
		void styleXmlRoundTrip()
		{
			DefinitionMarkStyle style;
			style.m_enabled = true;
			style.m_vertical_turn = 90;
			DefinitionMarkStyle::ShapeItem shape;
			shape.m_kind = DefinitionMarkStyle::ShapeKind::Circle;
			shape.m_pos = QPointF(3, -4);
			shape.m_rotation = 15;
			shape.m_width = 12;
			shape.m_height = 9;
			shape.m_pen_width = 2.5;
			shape.m_color = QColor(200, 10, 10);
			shape.m_filled = true;
			style.m_shapes << shape;
			DefinitionMarkStyle::TextItem text;
			text.m_formula = QStringLiteral("%wt / %wb");
			text.m_pos = QPointF(-7, 8);
			text.m_rotation = -90;
			text.m_size = 6.5;
			text.m_upright = true;
			text.m_alignment = Qt::AlignLeft | Qt::AlignBottom;
			style.m_texts << text;

			QDomDocument document;
			QDomElement element = document.createElement(QStringLiteral("definition_mark_style"));
			style.toXml(element);
			DefinitionMarkStyle read;
			read.fromXml(element);
			QVERIFY(read == style);

				//What the settings keep for the new projects
			QSettings settings;
			style.toSettings(settings, QStringLiteral("test/"));
			DefinitionMarkStyle from_settings;
			from_settings.fromSettings(settings, QStringLiteral("test/"));
			QVERIFY(from_settings == style);
		}

			//A drawing that was never customized is the standard one
		void standardStyleIsTheOldDrawing()
		{
			QVERIFY(DefinitionMarkStyle().isStandard());
			const DefinitionMarkStyle standard = DefinitionMarkStyle::standardStyle();
			QVERIFY(standard.isStandard());
			QCOMPARE(standard.m_texts.size(), 2);
			QCOMPARE(standard.m_texts.at(0).m_formula, QStringLiteral("%wc"));
			QCOMPARE(standard.m_texts.at(1).m_formula, QStringLiteral("%ws"));
			QVERIFY(standard.m_shapes.size() >= 1);
		}

			//The project draws its marks its own way from now on
		void customStyleChangesTheMark()
		{
			setMark(true, -1);
			QVERIFY(mark());
			QETProject *project = m_conductor->diagram()->project();
			const QRectF before = mark()->boundingRect();

			DefinitionMarkStyle style = DefinitionMarkStyle::standardStyle();
			style.m_enabled = true;
			DefinitionMarkStyle::TextItem text;
			text.m_formula = QStringLiteral("%wc and %ws");
			text.m_pos = QPointF(40, -30);
			style.m_texts << text;
			project->setDefinitionMarkStyle(style);
				//The mark is drawn again on its own as the style
				//changes; its texts are resolved for this conductor
			const QRectF after = mark()->boundingRect();
			QVERIFY(after.width() > before.width()
				|| after.height() > before.height());

			project->setDefinitionMarkStyle(DefinitionMarkStyle());
			QCOMPARE(mark()->boundingRect(), before);
		}

			//A formula speaks of the wire it is drawn on
		void wireFormulaVariables()
		{
			const ConductorProperties old_properties = m_conductor->properties();
			ConductorProperties p = old_properties;
			p.m_function = QStringLiteral("PE");
			p.m_tension_protocol = QStringLiteral("24V=");
			p.m_conductor_type = QStringLiteral("H07V-K");
			p.m_cable = QStringLiteral("W1");
			m_conductor->setProperties(p);
			autonum::sequentialNumbers numbers;
			const QString label = autonum::AssignVariables::formulaToLabel(
					QStringLiteral("%wf|%wv|%wc|%ws|%wt|%wb"),
					numbers, m_conductor->diagram(), nullptr, m_conductor);
			QCOMPARE(label, QStringLiteral("PE|24V=|BK|10|H07V-K|W1"));
			m_conductor->setProperties(old_properties);
		}

			//The words of a customized drawing follow the wire they
			//are drawn on: editing its section redraws the stroke
		void theMarkFollowsAnEditedWire()
		{
			QETProject *project = m_conductor->diagram()->project();
			DefinitionMarkStyle style = DefinitionMarkStyle::standardStyle();
			style.m_enabled = true;
			project->setDefinitionMarkStyle(style);
			setMark(true, -1);
			QVERIFY(mark());

			ConductorProperties p = m_conductor->properties();
			p.m_wire_section = QStringLiteral("16mm²");
			m_conductor->setProperties(p);
			const qreal wide = mark()->boundingRect().width();

			p.m_wire_section = QStringLiteral("16");
			m_conductor->setProperties(p);
			const qreal narrow = mark()->boundingRect().width();
			QVERIFY2(narrow < wide, "the stroke kept the section it had before");
			project->setDefinitionMarkStyle(DefinitionMarkStyle());
		}

			//A project saved before the drawing existed is drawn
			//the standard way, whatever the application's default
			//for new projects says
		void anOldProjectKeepsTheStandardDrawing()
		{
			DefinitionMarkStyle custom = DefinitionMarkStyle::standardStyle();
			custom.m_enabled = true;
			QSettings settings;
			custom.toSettings(settings, QStringLiteral("diagrameditor/default"));

				//The default really is the customized one...
			QETProject *fresh = new QETProject();
			QVERIFY(fresh->definitionMarkStyle().m_enabled);
			delete fresh;

				//...and a project without the element is not
				//touched by it
			QETProject *old = new QETProject(
					QFINDTESTDATA("fixtures/ten_sequential_numbers.qet"));
			QVERIFY(old);
			QVERIFY(old->definitionMarkStyle().isStandard());
			QVERIFY(!old->definitionMarkStyle().m_enabled);
			delete old;

			settings.remove(QStringLiteral("diagrameditor/defaultdefinitionmarkstyle"));
		}

			//A stroke made before its wire reaches the folio -- a
			//paste or an undone delete makes wires so -- follows
			//the project's drawing all the same
		void aMarkMadeBeforeItsWireJoinsTheFolio()
		{
			Diagram *diagram = QetAppTest::diagram();
			const ElementsLocation location(
				QStringLiteral("common://10_electric/10_allpole/114_connections/corner.elmt"));
			int state = 0;
			Element *a = ElementFactory::Instance()->createElement(location, nullptr, &state);
			Element *b = ElementFactory::Instance()->createElement(location, nullptr, &state);
			QVERIFY(a && b);
			diagram->addItem(a);
			diagram->addItem(b);
			a->setPos(100, 500);
			b->setPos(300, 500);

				//The properties ask for a stroke before the wire
				//is anywhere
			Conductor *conductor = new Conductor(a->terminals().first(), b->terminals().first());
			setMark(conductor, true, -1);
			QVERIFY(!markOf(conductor));
			diagram->addItem(conductor);
			QVERIFY(conductor->isValid());
			ConductorDefinitionMark *mark = markOf(conductor);
			QVERIFY(mark);

			const QRectF before = mark->boundingRect();
			DefinitionMarkStyle style = DefinitionMarkStyle::standardStyle();
			style.m_enabled = true;
			style.m_texts[0].m_pos = QPointF(60, -30);
			QETProject *project = diagram->project();
			project->setDefinitionMarkStyle(style);
			QVERIFY2(mark->boundingRect() != before,
				 "the stroke did not follow the project's drawing");
			project->setDefinitionMarkStyle(DefinitionMarkStyle());
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
		//QETApp keeps its standard folders unless the ones named exist
	QDir().mkpath(home.filePath("config"));
	QDir().mkpath(home.filePath("data"));
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
