// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include "genericdevice/genericdevice.h"

using namespace GenericDevice;

namespace {

	/// QElectroTech's default diagram font, at the sizes the wizard uses
Fonts fonts()
{
	QFont names(QStringLiteral("Liberation Sans"));
	names.setPointSizeF(7);
	QFont label(QStringLiteral("Liberation Sans"));
	label.setPointSizeF(9);
	return {names, label};
}

QStringList names(const Slots &slot_list)
{
	QStringList list;
	for (const Slot &s : slot_list)
		list << (s.kind == Slot::Terminal ? s.name : s.kind == Slot::Gap ? QStringLiteral("_")
										 : QStringLiteral("~"));
	return list;
}

Slots parse(const QString &pattern)
{
	const PatternResult r = expandPattern(pattern);
	Q_ASSERT(r.ok());
	return r.list;
}

	/// The VSD of the scope document, §3
Spec vsd()
{
	Spec spec;
	spec.name = QStringLiteral("VSD");
	spec.label = QStringLiteral("-U1");
	spec.manufacturer = QStringLiteral("ACME");
	spec.reference = QStringLiteral("VF-400");
	spec.sides[Left] = parse(QStringLiteral("L1,L2,L3,,PE"));
	spec.sides[Right] = parse(QStringLiteral("U,V,W,,PE"));
	spec.sides[Bottom] = parse(QStringLiteral("DI{4},COM,+24V"));
	return spec;
}

QString toText(const QDomElement &e)
{
	QString out;
	QTextStream stream(&out);
	e.save(stream, 1);
	return out;
}

} // namespace

class tst_genericdevice : public QObject
{
	Q_OBJECT

private slots:
	void expandPattern_data()
	{
		QTest::addColumn<QString>("pattern");
		QTest::addColumn<QStringList>("expected");
		QTest::newRow("plain") << "L1" << QStringList{"L1"};
		QTest::newRow("gap") << "L3,,PE" << QStringList{"L3", "_", "PE"};
		QTest::newRow("count") << "In{5}" << QStringList{"In1", "In2", "In3", "In4", "In5"};
		QTest::newRow("from zero") << "Q{z5}" << QStringList{"Q0", "Q1", "Q2", "Q3", "Q4"};
		QTest::newRow("range") << "X{3-8}" << QStringList{"X3", "X4", "X5", "X6", "X7", "X8"};
		QTest::newRow("prefix") << "X1:{4}" << QStringList{"X1:1", "X1:2", "X1:3", "X1:4"};
		QTest::newRow("suffix") << "{2}.1" << QStringList{"1.1", "2.1"};
		QTest::newRow("spaces") << " L1 , L2 " << QStringList{"L1", "L2"};
		QTest::newRow("empty") << "" << QStringList{};
		QTest::newRow("vsd bottom") << "DI{4},COM,+24V"
					    << QStringList{"DI1", "DI2", "DI3", "DI4", "COM", "+24V"};
	}
	void expandPattern()
	{
		QFETCH(QString, pattern);
		QFETCH(QStringList, expected);
		const PatternResult r = GenericDevice::expandPattern(pattern);
		QVERIFY2(r.ok(), qPrintable(r.error));
		QCOMPARE(names(r.list), expected);
	}

	void patternErrors_data()
	{
		QTest::addColumn<QString>("pattern");
		QTest::newRow("two braces") << "X{2}{3}";
		QTest::newRow("unclosed") << "X{2";
		QTest::newRow("letters") << "X{a}";
		QTest::newRow("z with range") << "X{z1-3}";
		QTest::newRow("backwards range") << "X{5-3}";
		QTest::newRow("zero count") << "X{0}";
		QTest::newRow("over the cap") << "X{201}";
	}
	void patternErrors()
	{
		QFETCH(QString, pattern);
		const PatternResult r = GenericDevice::expandPattern(pattern);
		QVERIFY(!r.ok());
		QVERIFY(r.list.isEmpty());
	}

	void capIsInclusive()
	{
		QCOMPARE(GenericDevice::expandPattern(QStringLiteral("X{200}")).list.size(), 200);
	}

	void toPatternFoldsRuns()
	{
		bool ok = false;
		QCOMPARE(toPattern(parse(QStringLiteral("In{5},COM")), &ok), QStringLiteral("In{5},COM"));
		QVERIFY(ok);
		QCOMPARE(toPattern(parse(QStringLiteral("Q{z4}"))), QStringLiteral("Q{z4}"));
		QCOMPARE(toPattern(parse(QStringLiteral("X{3-8}"))), QStringLiteral("X{3-8}"));
		QCOMPARE(toPattern(parse(QStringLiteral("L1,L2,L3,,PE"))), QStringLiteral("L{3},,PE"));
			//Two in a row stay as written
		QCOMPARE(toPattern(parse(QStringLiteral("A1,A2"))), QStringLiteral("A1,A2"));
			//Leading zeros are not numbers to fold
		QCOMPARE(toPattern(parse(QStringLiteral("01,02,03"))), QStringLiteral("01,02,03"));
	}

	void toPatternRoundTrips_data()
	{
		QTest::addColumn<QString>("pattern");
		for (const char *p : {"L1,L2,L3,,PE", "DI{4},COM,+24V", "Q{z5}", "X1:{4}", ",A,,B,",
				      "1,2,3,4,5", "X9,X10,X11", "A,A,A"})
			QTest::newRow(p) << QString::fromLatin1(p);
	}
	void toPatternRoundTrips()
	{
		QFETCH(QString, pattern);
		const Slots original = parse(pattern);
		QCOMPARE(names(parse(toPattern(original))), names(original));
	}

	void toPatternFlagsWhatItCannotSay()
	{
		bool ok = true;
		Slots s{Slot::terminal(QStringLiteral("A,B"))};
		toPattern(s, &ok);
		QVERIFY(!ok);
		ok = true;
		toPattern({Slot::terminal(QStringLiteral("{x}"))}, &ok);
		QVERIFY(!ok);
	}

	void applyPatternKeepsWhatTheTableHolds()
	{
		Slots current = parse(QStringLiteral("1,2"));
		current[0].function = QStringLiteral("OUT1");
		current[0].uuid = QUuid::createUuid();
		current[1].type = QStringLiteral("Inner");
		const Slots result = applyPattern(current, parse(QStringLiteral("A,,B,C")));
		QCOMPARE(names(result), (QStringList{"A", "_", "B", "C"}));
		QCOMPARE(result[0].function, QStringLiteral("OUT1"));
		QCOMPARE(result[0].uuid, current[0].uuid);
		QCOMPARE(result[2].type, QStringLiteral("Inner"));
		QVERIFY(result[3].uuid.isNull());
	}

	void groupEvery_data()
	{
		QTest::addColumn<int>("count");
		QTest::addColumn<int>("n");
		QTest::addColumn<QString>("expected");
		QTest::newRow("9 by 4") << 9 << 4 << "1 2 3 4 _ 5 6 7 8 _ 9";
		QTest::newRow("8 by 4") << 8 << 4 << "1 2 3 4 _ 5 6 7 8";
		QTest::newRow("none") << 3 << 0 << "1 2 3";
		QTest::newRow("by 1") << 3 << 1 << "1 _ 2 _ 3";
		QTest::newRow("n = count") << 4 << 4 << "1 2 3 4";
		QTest::newRow("n > count") << 3 << 8 << "1 2 3";
	}
	void groupEvery()
	{
		QFETCH(int, count);
		QFETCH(int, n);
		QFETCH(QString, expected);
		const Slots s = parse(QStringLiteral("{%1}").arg(count));
		QCOMPARE(names(GenericDevice::groupEvery(s, n)).join(' '), expected);
	}

	void groupEveryReplacesHandGaps()
	{
		QCOMPARE(names(GenericDevice::groupEvery(parse(QStringLiteral("1,,2,3,,,4")), 2)).join(' '),
			 QStringLiteral("1 2 _ 3 4"));
	}

	void resizeKeepsTypedNames()
	{
		const Slots s = parse(QStringLiteral("L1,L2,,PE"));
		QCOMPARE(names(resize(s, 5, 0)).join(' '), QStringLiteral("L1 L2 _ PE  "));
		QCOMPARE(names(resize(s, 2, 0)).join(' '), QStringLiteral("L1 L2"));
		QCOMPARE(names(resize(s, 0, 0)), QStringList());
		QCOMPARE(names(resize(s, 5, 2)).join(' '), QStringLiteral("L1 L2 _ PE  _ "));
	}

	void lineUpGroups()
	{
		Slots a = parse(QStringLiteral("L1,L2,L3,N,,PE"));
		Slots b = parse(QStringLiteral("U,V,W,,PE"));
		GenericDevice::lineUpGroups(a, b);
		QCOMPARE(names(a).join(' '), QStringLiteral("L1 L2 L3 N _ PE"));
		QCOMPARE(names(b).join(' '), QStringLiteral("U V W ~ _ PE"));
		QCOMPARE(a.size(), b.size());
	}

	void lineUpLeavesEqualAndUngroupedSidesAlone()
	{
		Slots a = parse(QStringLiteral("L1,L2,L3,,PE"));
		Slots b = parse(QStringLiteral("U,V,W,,PE"));
		GenericDevice::lineUpGroups(a, b);
		QCOMPARE(names(b).join(' '), QStringLiteral("U V W _ PE"));

			//No gap on one side: nothing faces the other's groups
		Slots c = parse(QStringLiteral("1,2,3,4,5"));
		Slots d = parse(QStringLiteral("A,,B"));
		GenericDevice::lineUpGroups(c, d);
		QCOMPARE(names(c).join(' '), QStringLiteral("1 2 3 4 5"));
		QCOMPARE(names(d).join(' '), QStringLiteral("A _ B"));

			//More groups on one side: the extra ones simply follow
		Slots e = parse(QStringLiteral("A,,B,B2,,C"));
		Slots f = parse(QStringLiteral("X,X2,,Y"));
		GenericDevice::lineUpGroups(e, f);
		QCOMPARE(names(e).join(' '), QStringLiteral("A ~ _ B B2 _ C"));
		QCOMPARE(names(f).join(' '), QStringLiteral("X X2 _ Y"));
	}

	void numberPinsCounterClockwise()
	{
		Spec spec;
		spec.sides[Left] = resize({}, 3, 0);
		spec.sides[Bottom] = {Slot::terminal({}), Slot::gap(), Slot::terminal({})};
		spec.sides[Right] = resize({}, 2, 0);
		spec.sides[Top] = resize({}, 2, 0);
		numberPins(spec);
		QCOMPARE(names(spec.sides[Left]).join(' '), QStringLiteral("1 2 3"));
		QCOMPARE(names(spec.sides[Bottom]).join(' '), QStringLiteral("4 _ 5"));
		QCOMPARE(names(spec.sides[Right]).join(' '), QStringLiteral("7 6"));
		QCOMPARE(names(spec.sides[Top]).join(' '), QStringLiteral("9 8"));
		QVERIFY(hasNumberedPins(spec));
		spec.sides[Top][0].name = QStringLiteral("PE");
		QVERIFY(!hasNumberedPins(spec));
	}

	void nameUnnamedLeavesTypedNames()
	{
		Spec spec;
		spec.sides[Left] = {Slot::terminal(QStringLiteral("L1")), Slot::terminal({}), Slot::gap(),
				    Slot::terminal({})};
		nameUnnamed(spec);
		QCOMPARE(names(spec.sides[Left]).join(' '), QStringLiteral("L1 2 _ 3"));
	}

	void pasteTabsAndCommasReadAlike()
	{
		const QString tabs = QStringLiteral(
			"side\tname\tfunction\n"
			"L\tL1\n" "L\tL2\n" "L\tL3\n" "L\t\n" "L\tPE\n"
			"right\tU\tmotor\n" "R\tV\n" "R\tW\n" "R\t\n" "R\tPE\n"
			"bottom\tDI1\t\tInner\n");
		QString commas = tabs;
		commas.replace('\t', ',');
		for (const QString &text : {tabs, commas})
		{
			const PasteResult r = parsePastedTable(text);
			QVERIFY2(r.bad_rows.isEmpty(), qPrintable(r.bad_rows.join('|')));
			QCOMPARE(r.rows, 11);
			QCOMPARE(names(r.sides[Left]).join(' '), QStringLiteral("L1 L2 L3 _ PE"));
			QCOMPARE(r.sides[Right][0].function, QStringLiteral("motor"));
			QCOMPARE(r.sides[Bottom][0].type, QStringLiteral("Inner"));
			QVERIFY(r.has_side[Left] && r.has_side[Right] && r.has_side[Bottom]);
			QVERIFY(!r.has_side[Top]);
		}
	}

	void pasteListsBadRows()
	{
		const PasteResult r = parsePastedTable(QStringLiteral(
			"left,A\r\nmiddle,B\r\ntop,C,,Sideways\r\n\r\nbottom,D,f,Generic,extra\r\nT,E"));
		QCOMPARE(r.rows, 2);
		QCOMPARE(r.bad_rows, (QStringList{"middle,B", "top,C,,Sideways", "bottom,D,f,Generic,extra"}));
		QCOMPARE(names(r.sides[Top]), QStringList{"E"});
	}

		/// Scope §3: the VSD body is 160 × 140 with the terminals packed
		/// from the top and clear of the corners
	void vsdSize()
	{
		const Layout l = layout(vsd(), fonts());
		QCOMPARE(l.body_width, 160);
		QCOMPARE(l.body_height, 140);
		QCOMPARE(l.terminals.size(), 14);
		QCOMPARE(l.dividers.size(), 2);
		QVERIFY(l.warnings.join(' ').contains(QStringLiteral("PE")));
	}

	void everythingOnTheGrid()
	{
		Spec spec = vsd();
		spec.sides[Top] = parse(QStringLiteral("X{3}"));
		spec.extra_width = 3;
		QDomDocument doc;
		const QDomElement def = toDefinition(spec, fonts(), doc);
		for (const char *a : {"width", "height", "hotspot_x", "hotspot_y"})
			QCOMPARE(def.attribute(a).toInt() % Grid, 0);
		const QDomNodeList terminals = def.elementsByTagName(QStringLiteral("terminal"));
		QCOMPARE(terminals.size(), 17);
		for (int i = 0 ; i < terminals.size() ; ++i) {
			const QDomElement t = terminals.at(i).toElement();
			QCOMPARE(qRound(t.attribute("x").toDouble()) % Grid, 0);
			QCOMPARE(qRound(t.attribute("y").toDouble()) % Grid, 0);
			QVERIFY(!QUuid(t.attribute("uuid")).isNull());
		}
	}

	void lineUpPutsPeOnOneRow()
	{
		Spec spec;
		spec.sides[Left] = parse(QStringLiteral("L1,L2,L3,N,,PE"));
		spec.sides[Right] = parse(QStringLiteral("U,V,W,,PE"));
		Layout l = layout(spec, fonts());
		QList<qreal> pe;
		for (const auto &t : l.terminals)
			if (t.slot.name == QLatin1String("PE")) pe << t.point.y();
		QCOMPARE(pe.size(), 2);
		QCOMPARE(pe[0], pe[1]);
			//Both gaps marked, the padding not
		QCOMPARE(l.dividers.size(), 2);
		QCOMPARE(l.dividers[0].y1(), l.dividers[1].y1());

		spec.line_up = false;
		l = layout(spec, fonts());
		pe.clear();
		for (const auto &t : l.terminals)
			if (t.slot.name == QLatin1String("PE")) pe << t.point.y();
		QVERIFY(pe[0] != pe[1]);

		spec.mark_gaps = false;
		QVERIFY(layout(spec, fonts()).dividers.isEmpty());
	}

	void edgeCases()
	{
		Spec empty;
		Layout l = layout(empty, fonts());
			//No terminals: the minimum, or as wide as the label needs
		QVERIFY(l.body_width >= MinWidth);
		QCOMPARE(l.body_width % Grid, 0);
		QCOMPARE(l.body_height, MinHeight);
		QVERIFY(l.terminals.isEmpty());

		Spec one;
		one.sides[Top] = parse(QStringLiteral("A"));
		l = layout(one, fonts());
		QCOMPARE(l.terminals.size(), 1);
		QCOMPARE(l.terminals[0].point.y(), qreal(-Stub));

		Spec many;
		many.sides[Left] = parse(QStringLiteral("{50}"));
		l = layout(many, fonts());
		QCOMPARE(l.terminals.size(), 50);
		QVERIFY(l.body_height >= 50 * many.pitch);
	}

	void extraSizeOnlyAdds()
	{
		Spec spec = vsd();
		const Layout base = layout(spec, fonts());
		spec.extra_width = 2;
		spec.extra_height = 1;
		const Layout grown = layout(spec, fonts());
		QCOMPARE(grown.body_width, base.body_width + 20);
		QCOMPARE(grown.body_height, base.body_height + 10);
		spec.extra_width = -5;
		QCOMPARE(layout(spec, fonts()).body_width, base.body_width);
	}

	void definitionRoundTrip()
	{
		Spec spec = vsd();
		spec.sides[Right][0].function = QStringLiteral("motor");
		spec.sides[Left][1].type = QStringLiteral("Inner");
		spec.sides[Top] = parse(QStringLiteral("A,B"));
		spec.extra_width = 2;
		spec.extra_height = 1;
		spec.pitch = 30;

		QDomDocument doc;
		const QDomElement def = toDefinition(spec, fonts(), doc, QStringLiteral("0.100.0"));
		Spec back;
		QVERIFY(fromDefinition(def, fonts(), &back));

		QCOMPARE(back.name, spec.name);
		QCOMPARE(back.label, spec.label);
		QCOMPARE(back.manufacturer, spec.manufacturer);
		QCOMPARE(back.reference, spec.reference);
		QCOMPARE(back.show_reference, true);
		QCOMPARE(back.pitch, 30);
		QCOMPARE(back.extra_width, 2);
		QCOMPARE(back.extra_height, 1);
		QVERIFY(back.mark_gaps);
		QVERIFY(back.line_up);
		for (int s = 0 ; s < SideCount ; ++s)
			QCOMPARE(names(back.sides[s]), names(spec.sides[s]));
		QCOMPARE(back.sides[Right][0].function, QStringLiteral("motor"));
		QCOMPARE(back.sides[Left][1].type, QStringLiteral("Inner"));

			//Written again, the terminals keep their uuids and positions
		QDomDocument doc2;
		const QDomElement def2 = toDefinition(back, fonts(), doc2, QStringLiteral("0.100.0"));
		const QDomNodeList t1 = def.elementsByTagName(QStringLiteral("terminal"));
		const QDomNodeList t2 = def2.elementsByTagName(QStringLiteral("terminal"));
		QCOMPARE(t2.size(), t1.size());
		for (int i = 0 ; i < t1.size() ; ++i)
			for (const char *a : {"x", "y", "uuid", "name", "orientation", "type"})
				QCOMPARE(t2.at(i).toElement().attribute(a), t1.at(i).toElement().attribute(a));
		for (const char *a : {"width", "height", "hotspot_x", "hotspot_y"})
			QCOMPARE(def2.attribute(a), def.attribute(a));
	}

	void roundTripWithPaddingAndNoMarks()
	{
		Spec spec;
		spec.name = QStringLiteral("X");
		spec.sides[Left] = parse(QStringLiteral("L1,L2,L3,N,,PE"));
		spec.sides[Right] = parse(QStringLiteral("U,V,W,,PE"));
		for (bool marks : {true, false})
		{
			spec.mark_gaps = marks;
			QDomDocument doc;
			const QDomElement def = toDefinition(spec, fonts(), doc);
			Spec back;
			QVERIFY(fromDefinition(def, fonts(), &back));
			QDomDocument doc2;
			const QDomElement def2 = toDefinition(back, fonts(), doc2);
			const QDomNodeList t1 = def.elementsByTagName(QStringLiteral("terminal"));
			const QDomNodeList t2 = def2.elementsByTagName(QStringLiteral("terminal"));
			QCOMPARE(t2.size(), t1.size());
			for (int i = 0 ; i < t1.size() ; ++i)
				QCOMPARE(t2.at(i).toElement().attribute("y"), t1.at(i).toElement().attribute("y"));
			QCOMPARE(def2.elementsByTagName("line").size(), def.elementsByTagName("line").size());
		}
	}

	void foreignDefinitionsAreRefused()
	{
		QDomDocument doc;
		QDomElement def = toDefinition(vsd(), fonts(), doc);
		Spec back;
		QVERIFY(fromDefinition(def, fonts(), &back));

			//A circle drawn on it in the symbol editor
		QDomElement description = def.firstChildElement(QStringLiteral("description"));
		QDomElement extra = doc.createElement(QStringLiteral("ellipse"));
		description.appendChild(extra);
		QVERIFY(!fromDefinition(def, fonts(), &back));
		description.removeChild(extra);

			//A terminal moved off its edge
		QDomElement t = description.firstChildElement(QStringLiteral("terminal"));
		t.setAttribute(QStringLiteral("x"), 5);
		QVERIFY(!fromDefinition(def, fonts(), &back));
	}

		/// The output is stable apart from the uuids
	void definitionIsDeterministic()
	{
		Spec spec = vsd();
		for (Slots &side : spec.sides)
			for (Slot &s : side)
				s.uuid = QUuid::createUuid();
		QDomDocument a, b;
		QString one = toText(toDefinition(spec, fonts(), a));
		QString two = toText(toDefinition(spec, fonts(), b));
		static const QRegularExpression uuid(QStringLiteral("uuid=\"\\{[^}]+\\}\""));
		static const QRegularExpression part_uuid(QStringLiteral("<uuid uuid=\"[^\"]+\"/>"));
			//Only parts and the definition get fresh uuids; the terminals keep theirs
		QCOMPARE(one.count(QStringLiteral("<terminal ")), 14);
		for (QString *s : {&one, &two}) {
			const QStringList lines = s->split('\n');
			QStringList kept;
			for (QString line : lines) {
				if (!line.contains(QLatin1String("<terminal")))
					line.replace(uuid, QStringLiteral("uuid=\"\""));
				kept << line;
			}
			*s = kept.join('\n');
		}
		QCOMPARE(one, two);
	}
};

QTEST_MAIN(tst_genericdevice)

#include "tst_genericdevice.moc"
