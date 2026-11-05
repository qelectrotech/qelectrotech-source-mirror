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
	A copy of a conductor definition keeps the point its right-aligned
	and centred texts are anchored on, the way a copy of a folio report
	has since Diagram::correctTextPos() (PR #399, "Correct compositeText
	alignment on copying").

	Both symbols below carry the same two fields: an information field
	and a composite text, both showing the colour of the wire they sit
	on -- one right-aligned, one centred. The folio report is the
	control: it is the case the fix was written for, so whatever the
	expectation is, it is the one the fixed code already meets.
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "conductorproperties.h"
#include "diagram.h"
#include "diagramcontent.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetmessagebox.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QHash>
#include <QUuid>

#include <optional>
#include <utility>

class TstConductorDefCopy : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;
		Element *m_definition = nullptr;
		Element *m_report = nullptr;
		Conductor *m_wire = nullptr;

		/*
			One symbol of @p link_type with a single terminal on the side
			@p side names, and two fields showing the colour of the wire
			it sits on: an information field, right-aligned, and a
			composite text, centred. @p uuid_tail tells the two
			definitions apart, @p terminal_x puts the terminal on the
			matching edge.
		*/
		static QString symbolXml(const QString &link_type,
								const QString &uuid_tail,
								const QString &side,
								const QString &terminal_x)
		{
			return QStringLiteral(
				R"(<definition version="0.100.0" type="element" link_type="%1" width="40" height="20" hotspot_x="0" hotspot_y="0">
<names><name lang="en">wire colour</name></names>
<description>
<line x1="0" y1="10" x2="40" y2="10" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal uuid="{a5c10000-0000-4000-8000-0000000000%2}" name="1" x="%4" y="10" orientation="%3" type="Generic"/>
<dynamic_text x="40" y="-10" z="1" text_width="-1" Halignment="AlignRight" Valignment="AlignTop" frame="false" rotation="0" keep_visual_rotation="true" text_from="ElementInfo" uuid="{a5c10000-0000-4000-8000-0000000001%2}" font="Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular">
<text></text>
<info_name>conductor_color</info_name>
</dynamic_text>
<dynamic_text x="40" y="25" z="2" text_width="-1" Halignment="AlignHCenter" Valignment="AlignTop" frame="false" rotation="0" keep_visual_rotation="true" text_from="CompositeText" uuid="{a5c10000-0000-4000-8000-0000000002%2}" font="Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular">
<text></text>
<composite_text>%{conductor_color}</composite_text>
</dynamic_text>
</description>
</definition>)")
				.arg(link_type)
				.arg(uuid_tail)
				.arg(side)
				.arg(terminal_x);
		}

		/// The two fields of a symbol, in the order they are in the file
		static QList<DynamicElementTextItem *> fields(Element *element)
		{
			return element->dynamicTextItems();
		}

		static DynamicElementTextItem *field(Element *element, int index)
		{
			const QList<DynamicElementTextItem *> texts = fields(element);
			return index < texts.size() ? texts.at(index) : nullptr;
		}

		/// What the fields of a symbol are called, for a failure message
		static QString fieldName(int index)
		{
			return index == 0 ? QStringLiteral("the right-aligned field")
							  : QStringLiteral("the centred field");
		}

		/*
			@return the x the copy holds for the @p index field of
			@p element, or nothing when the copy holds no such symbol or
			no such field. An x can be negative -- a field the copy did
			not correct sits left of its symbol -- so absence needs a
			value of its own.
		*/
		static std::optional<qreal> copiedTextX(const QDomDocument &copy,
												const Element *element, int index)
		{
			const QDomElement elements = copy.documentElement().firstChildElement(
				QStringLiteral("elements"));
			for (QDomElement e = elements.firstChildElement(QStringLiteral("element"));
				 !e.isNull(); e = e.nextSiblingElement(QStringLiteral("element"))) {
				if (e.attribute(QStringLiteral("uuid")) != element->uuid().toString())
					continue;
				QDomElement text = e.firstChildElement(QStringLiteral("dynamic_texts"))
									   .firstChildElement(QStringLiteral("dynamic_elmt_text"));
				for (int i = 0; i < index && !text.isNull(); ++i)
					text = text.nextSiblingElement(QStringLiteral("dynamic_elmt_text"));
				if (text.isNull())
					return std::nullopt;
				return text.attribute(QStringLiteral("x")).toDouble();
			}
			return std::nullopt;
		}

		/*
			A pasted copy must not answer to the identity of what it was
			made of: the project's rows are keyed by uuid, and a wire
			names its two ends by the uuid of the symbol they belong to.
		*/
		static void renewUuids(QDomDocument &copy)
		{
			QHash<QString, QString> fresh;
			const QDomElement elements = copy.documentElement().firstChildElement(
				QStringLiteral("elements"));
			for (QDomElement e = elements.firstChildElement(QStringLiteral("element"));
				 !e.isNull(); e = e.nextSiblingElement(QStringLiteral("element"))) {
				const QString was = e.attribute(QStringLiteral("uuid"));
				const QString now = QUuid::createUuid().toString();
				fresh.insert(was, now);
				e.setAttribute(QStringLiteral("uuid"), now);
				const QDomElement texts = e.firstChildElement(QStringLiteral("dynamic_texts"));
				for (QDomElement t = texts.firstChildElement();
					 !t.isNull(); t = t.nextSiblingElement()) {
					t.setAttribute(QStringLiteral("uuid"), QUuid::createUuid().toString());
				}
			}
			const QDomElement conductors = copy.documentElement().firstChildElement(
				QStringLiteral("conductors"));
			for (QDomElement c = conductors.firstChildElement(QStringLiteral("conductor"));
				 !c.isNull(); c = c.nextSiblingElement(QStringLiteral("conductor"))) {
				c.setAttribute(QStringLiteral("uuid"), QUuid::createUuid().toString());
				for (const QString &attribute : {QStringLiteral("element1"),
												QStringLiteral("element2")}) {
					const QString was = c.attribute(attribute);
					if (fresh.contains(was))
						c.setAttribute(attribute, fresh.value(was));
				}
			}
		}

		static bool samePoint(const QPointF &a, const QPointF &b)
		{
			return qAbs(a.x() - b.x()) < 1e-6 && qAbs(a.y() - b.y()) < 1e-6;
		}

		static QString pointText(const QPointF &p)
		{
			return QStringLiteral("(%1, %2)").arg(p.x()).arg(p.y());
		}

	private slots:
		void initTestCase()
		{
			QET::QetMessageBox::setNonInteractive(true);
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			QETProject *project = m_diagram->project();
			QVERIFY(project);

			QDomDocument definition;
			definition.setContent(symbolXml(QStringLiteral("conductor_definition"),
											QStringLiteral("01"),
											QStringLiteral("w"),
											QStringLiteral("0")));
			QVERIFY(project->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("leitdef"),
				definition.documentElement()));

			definition.setContent(symbolXml(QStringLiteral("next_report"),
											QStringLiteral("02"),
											QStringLiteral("e"),
											QStringLiteral("40")));
			QVERIFY(project->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("verweis"),
				definition.documentElement()));

			int state = 0;
			m_definition = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/leitdef.elmt"), project),
				nullptr, &state);
			QVERIFY2(!state && m_definition, qPrintable(QString::number(state)));
			m_report = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/verweis.elmt"), project),
				nullptr, &state);
			QVERIFY2(!state && m_report, qPrintable(QString::number(state)));

			m_diagram->addItem(m_definition);
			m_diagram->addItem(m_report);
			m_definition->setPos(300, 200);
			m_report->setPos(200, 200);

			QVERIFY(m_definition->dynamicTextItems().size() == 2);
			QVERIFY(m_report->dynamicTextItems().size() == 2);
			QVERIFY(m_definition->linkType() == Element::ConductorDefinition);
			QVERIFY(m_report->linkType() == Element::NextReport);

			//The wire both symbols sit on: their fields read its colour,
			//which is what fills them and gives the texts their width.
			m_wire = new Conductor(m_report->terminals().first(),
								   m_definition->terminals().first());
			m_diagram->addItem(m_wire);
			QVERIFY(m_wire->isValid());
			ConductorProperties properties = m_wire->properties();
			properties.m_wire_color = QStringLiteral("BK");
			m_wire->setProperties(properties);

			for (int i = 0; i < 2; ++i) {
				QVERIFY(field(m_definition, i));
				QVERIFY(field(m_report, i));
				QCOMPARE(field(m_definition, i)->toPlainText(), QStringLiteral("BK"));
				QCOMPARE(field(m_report, i)->toPlainText(), QStringLiteral("BK"));
			}
		}

		/*
			The copy stores the point each text is anchored on, not the
			top-left corner it happens to have after finishAlignment()
			moved it aside for the width of its text. The two symbols
			carry the same fields, so their copies must agree.
		*/
		void copyStoresTheAnchor()
		{
			m_diagram->clearSelection();
			m_definition->setSelected(true);
			m_report->setSelected(true);
			const QDomDocument copy = m_diagram->toXml(false, true);

			QStringList problems;
			for (int i = 0; i < 2; ++i) {
				const std::optional<qreal> definition_x =
					copiedTextX(copy, m_definition, i);
				const std::optional<qreal> report_x = copiedTextX(copy, m_report, i);
				if (!definition_x)
					problems << QStringLiteral("%1: the conductor definition's copy "
											  "holds no such field")
								.arg(fieldName(i));
				if (!report_x)
					problems << QStringLiteral("%1: the folio report's copy holds "
											  "no such field")
								.arg(fieldName(i));
				if (definition_x && report_x
					&& qAbs(*definition_x - *report_x) >= 1e-6)
					problems << QStringLiteral(
								 "%1: a conductor definition is copied at x=%2, "
								 "a folio report at x=%3")
								 .arg(fieldName(i)).arg(*definition_x).arg(*report_x);
			}
			QVERIFY2(problems.isEmpty(), qPrintable(problems.join(QLatin1Char('\n'))));
		}

		/*
			The folio gives its own texts back where they were: writing
			the copy out must not move anything.
		*/
		void copyingLeavesTheFolioAlone()
		{
			QList<QPointF> definition_anchors;
			QList<QPointF> report_anchors;
			for (int i = 0; i < 2; ++i) {
				QVERIFY(field(m_definition, i));
				QVERIFY(field(m_report, i));
				definition_anchors << field(m_definition, i)->anchorPos();
				report_anchors << field(m_report, i)->anchorPos();
			}

			m_diagram->clearSelection();
			m_definition->setSelected(true);
			m_report->setSelected(true);
			m_diagram->toXml(false, true);

			for (int i = 0; i < 2; ++i) {
				QVERIFY2(samePoint(field(m_definition, i)->anchorPos(),
								   definition_anchors.at(i)),
						 qPrintable(QStringLiteral("a conductor definition: %1 moved from %2 to %3")
									.arg(fieldName(i),
										 pointText(definition_anchors.at(i)),
										 pointText(field(m_definition, i)->anchorPos()))));
				QVERIFY2(samePoint(field(m_report, i)->anchorPos(), report_anchors.at(i)),
						 qPrintable(QStringLiteral("a folio report: %1 moved from %2 to %3")
									.arg(fieldName(i),
										 pointText(report_anchors.at(i)),
										 pointText(field(m_report, i)->anchorPos()))));
			}
		}

		/*
			What the user sees: a paste puts each field back on the point
			it was anchored on. A field is empty while the copy is read
			and filled again once the wire it stands on is there, which
			is when finishAlignment() runs -- and where a copy that
			stored the top-left corner instead of the anchor loses one
			text width.
		*/
		void pasteKeepsTheAnchor()
		{
			QList<QPointF> definition_anchors;
			QList<QPointF> report_anchors;
			for (int i = 0; i < 2; ++i) {
				QVERIFY(field(m_definition, i));
				QVERIFY(field(m_report, i));
				definition_anchors << field(m_definition, i)->anchorPos();
				report_anchors << field(m_report, i)->anchorPos();
			}

			m_diagram->clearSelection();
			m_definition->setSelected(true);
			m_report->setSelected(true);
			QDomDocument copy = m_diagram->toXml(false, true);
			renewUuids(copy);

			DiagramContent content;
			QDomElement root = copy.documentElement();
			QVERIFY(m_diagram->fromXml(root, QPointF(0, 600), false, &content));
			QVERIFY2(!content.m_conductors_to_move.isEmpty(),
					 "the wire came back with the symbols it joins");

			Element *pasted_definition = nullptr;
			Element *pasted_report = nullptr;
			for (Element *e : std::as_const(content.m_elements)) {
				if (e->linkType() == Element::ConductorDefinition)
					pasted_definition = e;
				else if (e->linkType() & Element::AllReport)
					pasted_report = e;
			}
			QVERIFY2(pasted_definition, "the conductor definition came back");
			QVERIFY2(pasted_report, "the folio report came back");

			//The folio report first: it is the case the existing fix was
			//written for, so it says whether the expectation is the right
			//one before the conductor definition is held to it.
			QStringList problems;
			auto check = [&problems](Element *element, const QList<QPointF> &was,
									 const QString &who) {
				for (int i = 0; i < 2; ++i) {
					DynamicElementTextItem *text = field(element, i);
					if (!text) {
						problems << QStringLiteral("%1 of the pasted %2 is missing")
										.arg(fieldName(i), who);
						continue;
					}
					if (text->toPlainText() != QLatin1String("BK"))
						problems << QStringLiteral("%1 of the pasted %2 shows \"%3\"")
										.arg(fieldName(i), who, text->toPlainText());
					if (!samePoint(text->anchorPos(), was.at(i)))
						problems << QStringLiteral("%1 of the pasted %2 sits at %3, was at %4")
										.arg(fieldName(i), who,
											 pointText(text->anchorPos()),
											 pointText(was.at(i)));
				}
			};
			check(pasted_report, report_anchors, QStringLiteral("folio report"));
			check(pasted_definition, definition_anchors,
				  QStringLiteral("conductor definition"));

			QVERIFY2(problems.isEmpty(), qPrintable(problems.join(QLatin1Char('\n'))));
		}
};

QET_APP_TEST_MAIN(TstConductorDefCopy)
#include "tst_conductordefcopy.moc"
