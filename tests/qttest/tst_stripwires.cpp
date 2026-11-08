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
	A terminal strip draws the wires which end at it above itself, one
	stub per terminal which has a wire of its own, the way it draws the
	cables below itself. What such a wire says -- where it lands and what
	it is made of -- is worked out here, together with how long the stub
	has to be for it.

	The fixture has the strip X1 with the elements 891b70f2 and
	4861e268. The wire fc9cc920 -> 891b70f2 and the wire 891b70f2 ->
	4861e268 are both cores of the cable W1, so neither of them is a
	wire of its own; the wire 4861e268 -> de9ccb25 is one.
*/
#include "qetapptest.h"

#include "TerminalStrip/GraphicsItem/terminalstripdrawerhelpers.h"
#include "TerminalStrip/GraphicsItem/trueterminalstrip.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "conductorproperties.h"
#include "diagram.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetgraphicsitem/terminalelement.h"
#include "qetproject.h"
#include "utils/qetsettings.h"

#include <QImage>
#include <QPainter>
#include <QPen>

class tst_stripwires : public QObject
{
	Q_OBJECT

	QETProject *m_project = nullptr;

	static QString uuidOf(Element *element)
	{
		return element ? element->uuid().toString() : QString();
	}

		///The first wire of @p terminal: what the other end is called
		///and what the wire is made of
	static QPair<QString, QStringList> firstWire(
		const QSharedPointer<RealTerminal> &terminal)
	{
		const auto wires = terminal->wires();
		return wires.isEmpty() ? QPair<QString, QStringList>()
				       : wires.first();
	}

		///The first terminal of a strip whose element carries @p uuid
	QSharedPointer<RealTerminal> terminalOf(const QString &uuid) const
	{
		for (TerminalStrip *strip : m_project->terminalStrip())
		{
			for (const auto &real : strip->realTerminals())
			{
				if (real && uuidOf(real->element()).contains(uuid)) {
					return real;
				}
			}
		}
		return {};
	}

		///The wire between the elements whose uuids start with @p a and @p b
	Conductor *wireBetween(const QString &a, const QString &b) const
	{
		for (Diagram *diagram : m_project->diagrams())
		{
			for (Conductor *conductor : diagram->conductors())
			{
				const QString ends[2] = {
					uuidOf(conductor->terminal1
						? conductor->terminal1->parentElement() : nullptr),
					uuidOf(conductor->terminal2
						? conductor->terminal2->parentElement() : nullptr)};
				if ((ends[0].contains(a) && ends[1].contains(b))
					|| (ends[0].contains(b) && ends[1].contains(a))) {
					return conductor;
				}
			}
		}
		return nullptr;
	}

	static void setLabel(Element *element, const QString &label)
	{
		auto informations = element->elementInformations();
		informations.addValue(QStringLiteral("label"), label);
		informations.remove(QStringLiteral("formula"));
		element->setElementInformations(informations);
	}

		///The terminal of @p conductor which does not belong to @p uuid
	static Terminal *otherEndOf(Conductor *conductor, const QString &uuid)
	{
		for (Terminal *terminal : {conductor->terminal1,
					   conductor->terminal2})
		{
			Element *element = terminal ? terminal->parentElement() : nullptr;
			if (element && !uuidOf(element).contains(uuid)) {
				return terminal;
			}
		}
		return nullptr;
	}

private slots:
	void initTestCase();
	void init();
	void cleanup();

	void theSettingIsOffByDefault();
	void theSettingIsKept();
	void onlyTheTerminalWithAWireHasOne();
	void whatTheFarEndIsCalled();
	void aTerminalIsNamedByItsOwnLabel();
	void whatTheWireIsMadeOf();
	void theEmptyValuesAreLeftOut();
	void theStubGrowsWithWhatTheWireIsMadeOf();
	void aTwinSleeveGetsAWireForEachOfItsAdern();
	void allTheTargetsStandAtOneHeight();
	void theAdapterSaysWhatTheTerminalSays();
};

void tst_stripwires::initTestCase()
{
		//A fresh settings folder of its own: nothing has turned it on
	QVERIFY(!QetSettings::stripWiresShown());
}

void tst_stripwires::init()
{
	const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
	QVERIFY(!fixture.isEmpty());
	m_project = new QETProject(fixture);
	QCOMPARE(m_project->state(), QETProject::Ok);
	QetSettings::setStripWiresShown(true);
}

void tst_stripwires::cleanup()
{
	QetSettings::setStripWiresShown(false);
	delete m_project;
	m_project = nullptr;
}

void tst_stripwires::theSettingIsOffByDefault()
{
	QetSettings::setStripWiresShown(false);
	QVERIFY(!QetSettings::stripWiresShown());
}

void tst_stripwires::theSettingIsKept()
{
	QetSettings::setStripWiresShown(true);
	QVERIFY(QetSettings::stripWiresShown());
	QetSettings::setStripWiresShown(false);
	QVERIFY(!QetSettings::stripWiresShown());
}

void tst_stripwires::onlyTheTerminalWithAWireHasOne()
{
	const auto cable_core = terminalOf(QStringLiteral("891b70f2"));
	QVERIFY(cable_core);
	const auto wired = terminalOf(QStringLiteral("4861e268"));
	QVERIFY(wired);

		//Both of them are cores of the cable W1 but one, which carries
		//the wire 4861e268 -> de9ccb25: only that one has a wire of its
		//own to draw a stub for
	QVERIFY(!cable_core->hasWire());
	QVERIFY(wired->hasWire());
}

void tst_stripwires::whatTheFarEndIsCalled()
{
	Conductor *conductor = wireBetween(QStringLiteral("4861e268"),
					   QStringLiteral("de9ccb25"));
	QVERIFY(conductor);
	Element *far_element = otherEndOf(conductor, QStringLiteral("4861e268"))
		->parentElement();
	QVERIFY(far_element);
	setLabel(far_element, QStringLiteral("K1"));

	Terminal *far_terminal = otherEndOf(conductor, QStringLiteral("4861e268"));
	const QString expected = far_terminal->name().isEmpty()
		? QStringLiteral("K1")
		: QStringLiteral("K1:") + far_terminal->name();

	const auto wired = terminalOf(QStringLiteral("4861e268"));
	QVERIFY(wired);
	QCOMPARE(firstWire(wired).first, expected);
}

/**
 * The number a terminal strip draws on a terminal comes from the label of
 * that terminal, and the labels run out of step with the positions the
 * terminals stand at as soon as one of them is reordered. The label is
 * therefore what says which terminal is meant -- never the position.
 */
void tst_stripwires::aTerminalIsNamedByItsOwnLabel()
{
	const auto wired = terminalOf(QStringLiteral("891b70f2"));
	QVERIFY(wired);
	const auto other = terminalOf(QStringLiteral("4861e268"));
	QVERIFY(other);
		//Without this the terminal would be named by its position, which
		//is exactly what this test must not see happen
	QVERIFY(other->parentStrip());
	setLabel(other->element(), QStringLiteral("X1:99"));
	QCOMPARE(other->element()->actualLabel(), QStringLiteral("X1:99"));

		//Both of them carry cores of the cable W1 and nothing else so
		//far, so this wire is the first one of its own the first of them
		//has -- the far end of it is the other terminal
	Diagram *diagram = m_project->diagrams().first();
	QVERIFY(diagram);
	auto *conductor = new Conductor(wired->element()->terminals().first(),
					other->element()->terminals().first());
	diagram->addItem(conductor);

	QVERIFY(wired->hasWire());
	QCOMPARE(firstWire(wired).first, QStringLiteral("X1:99"));
}

void tst_stripwires::whatTheWireIsMadeOf()
{
	Conductor *conductor = wireBetween(QStringLiteral("4861e268"),
					   QStringLiteral("de9ccb25"));
	QVERIFY(conductor);
	ConductorProperties properties = conductor->ownProperties();
	properties.m_conductor_type = QStringLiteral("H05V-K");
	properties.m_wire_color = QStringLiteral("BN");
	properties.m_wire_section = QStringLiteral("1.5");
	conductor->setProperties(properties);

	const auto wired = terminalOf(QStringLiteral("4861e268"));
	QVERIFY(wired);
	QCOMPARE(firstWire(wired).second,
		 QStringList({QStringLiteral("H05V-K"),
				     QStringLiteral("BN"),
				     QStringLiteral("1.5")}));
}

void tst_stripwires::theEmptyValuesAreLeftOut()
{
	Conductor *conductor = wireBetween(QStringLiteral("4861e268"),
					   QStringLiteral("de9ccb25"));
	QVERIFY(conductor);
	ConductorProperties properties = conductor->ownProperties();
	properties.m_conductor_type.clear();
	properties.m_wire_section.clear();
	properties.m_wire_color = QStringLiteral("BN");
	conductor->setProperties(properties);

	const auto wired = terminalOf(QStringLiteral("4861e268"));
	QVERIFY(wired);
	QCOMPARE(firstWire(wired).second,
		 QStringList({QStringLiteral("BN")}));

		//A cable says where its cores go and what they are made of
		//itself, and is drawn below the strip with the rest of it
	const auto cable_core = terminalOf(QStringLiteral("891b70f2"));
	QVERIFY(cable_core);
	QVERIFY(cable_core->wires().isEmpty());
}

/**
 * A terminal with a twin sleeve carries two wires, and both of them have
 * somewhere to go: the strip draws a stub for each of them, spread over
 * the width of the terminal.
 */
void tst_stripwires::aTwinSleeveGetsAWireForEachOfItsAdern()
{
	const auto twin = terminalOf(QStringLiteral("891b70f2"));
	QVERIFY(twin);
	const auto other = terminalOf(QStringLiteral("4861e268"));
	QVERIFY(other);
	QVERIFY(twin->wires().isEmpty());

	Diagram *diagram = m_project->diagrams().first();
	QVERIFY(diagram);
	auto *first = new Conductor(twin->element()->terminals().first(),
				     other->element()->terminals().first());
	diagram->addItem(first);
	QCOMPARE(twin->wires().size(), 1);

	auto *second = new Conductor(twin->element()->terminals().last(),
				      other->element()->terminals().first());
	diagram->addItem(second);
	QCOMPARE(twin->wires().size(), 2);
	QVERIFY(twin->hasWire());
}

/**
 * The strokes of a strip all stop at one line, worked out from the longest
 * target among them, and every target stands with its foot on that line:
 * a wire with a short target gets as long a stroke as one with a long
 * target, instead of the strokes ending where their own writing does.
 */
void tst_stripwires::allTheTargetsStandAtOneHeight()
{
	using TerminalStripDrawer::Helpers::WireCell;
	using TerminalStripDrawer::Helpers::drawWires;

	TerminalStripLayoutPattern pattern;
	const qreal strip_top{150};
	QVector<WireCell> cells;
	WireCell plain;
	plain.x = 20;
	plain.top = strip_top;
	WireCell long_target = plain;
	long_target.x = 60;
	long_target.target = QStringLiteral("12K1:D13");
	WireCell short_target = plain;
	short_target.x = 100;
	short_target.target = QStringLiteral("X1:2");
	cells << plain << long_target << short_target;

	QImage image(140, 200, QImage::Format_ARGB32);
	image.fill(Qt::white);
	QPainter painter(&image);
	painter.setPen(QPen(Qt::black, 1));
	drawWires(&painter, cells, pattern);
	painter.end();

	const auto dark = [&image](int x, int y) {
		return qGray(image.pixel(x, y)) < 200;
	};
		//The stroke of a stub: the dark run which reaches the strip,
		//from the strip up to where its writing begins
	const auto stroke_top = [&](int x) {
		int top = int(strip_top);
		for (int y = int(strip_top) - 1 ; y >= 0 ; --y)
		{
			if (!dark(x, y)) {
				break;
			}
			top = y;
		}
		return top;
	};
	const int plain_top = stroke_top(20);
	const int long_top = stroke_top(60);
	const int short_top = stroke_top(100);
	QVERIFY(plain_top > 0);
	QVERIFY(plain_top < int(strip_top));
	QVERIFY2(qAbs(plain_top - long_top) <= 2
		 && qAbs(plain_top - short_top) <= 2,
		 qPrintable(QStringLiteral("the strokes end at %1, %2 and %3")
				.arg(plain_top).arg(long_top).arg(short_top)));

		//The foot of a target: the first ink above the stroke, the
		//same line for every target of the strip
	const auto target_foot = [&](int x) {
		for (int y = stroke_top(x) - 1 ; y >= 0 ; --y)
		{
			for (int dx = -9 ; dx <= 9 ; ++dx)
			{
				if (dark(x + dx, y)) {
					return y;
				}
			}
		}
		return -1;
	};
	const int long_foot = target_foot(60);
	const int short_foot = target_foot(100);
	QVERIFY(long_foot > 0);
	QVERIFY(short_foot > 0);
	QVERIFY2(qAbs(long_foot - short_foot) <= 3,
		 qPrintable(QStringLiteral("the targets stand at %1 and %2")
				.arg(long_foot).arg(short_foot)));
}

void tst_stripwires::theStubGrowsWithWhatTheWireIsMadeOf()
{
	using TerminalStripDrawer::Helpers::WireCell;
	using TerminalStripDrawer::Helpers::wiresTop;

	TerminalStripLayoutPattern pattern;
	QCOMPARE(pattern.m_cable_wire_length, 40.0);

		//A wire with nothing to say about it gets the stub a cable gets
	WireCell bare;
	bare.x = 10;
	bare.top = 0;
	QCOMPARE(wiresTop({bare}, pattern), -40.0);

		//Where it lands needs its room above the strip...
	WireCell named = bare;
	named.target = QStringLiteral("K1:A1");
	const qreal named_top = wiresTop({named}, pattern);
	QVERIFY(named_top < -40.0);

		//...and what it is made of is written in a gap of the stub, so
		//the stub grows with every value which has something to say
	WireCell full = named;
	full.definition = {QStringLiteral("H05V-K"),
			   QStringLiteral("BN"),
			   QStringLiteral("1.5")};
	QVERIFY(wiresTop({full}, pattern) < named_top);
}

void tst_stripwires::theAdapterSaysWhatTheTerminalSays()
{
	const auto wired = terminalOf(QStringLiteral("4861e268"));
	QVERIFY(wired);
	TerminalStripDrawer::TrueRealTerminal adapted(wired);

	QCOMPARE(adapted.hasWire(), wired->hasWire());
	const auto from_terminal = wired->wires();
	const auto from_adapter = adapted.wires();
	QCOMPARE(from_adapter.size(), from_terminal.size());
	for (auto i = 0 ; i < from_terminal.size() ; ++i)
	{
		QCOMPARE(from_adapter.at(i).target, from_terminal.at(i).first);
		QCOMPARE(from_adapter.at(i).definition,
			 from_terminal.at(i).second);
	}

	const auto cable_core = terminalOf(QStringLiteral("891b70f2"));
	QVERIFY(cable_core);
	TerminalStripDrawer::TrueRealTerminal adapted_core(cable_core);
	QVERIFY(!adapted_core.hasWire());
}

QET_APP_TEST_MAIN(tst_stripwires)

#include "tst_stripwires.moc"
