// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QUndoStack>

#include "conductormultiedit.h"
#include "qetapp.h"

	// qet.cpp needs it; the application is not linked
QString QETApp::m_interface_language;

// A wire as ConductorMultiEdit sees one: properties through the same
// Q_PROPERTY the undo command writes, a potential, and a position.
class FakeWire : public QObject
{
	Q_OBJECT
	Q_PROPERTY(ConductorProperties properties READ properties WRITE setProperties)

	public:
		FakeWire(const QString &num, const QString &function,
			 const QString &cable, QPointF pos) :
			m_pos(pos)
		{
			m_properties.text = num;
			m_properties.m_function = function;
			m_properties.m_cable = cable;
		}

		ConductorProperties properties() const { return m_properties; }
		void setProperties(const ConductorProperties &p) { m_properties = p; }
		QSet<FakeWire *> relatedPotentialConductors() const { return m_potential; }
			// The wires of the other side of a function separated terminal
			// element: same electrical node, but only the voltage /
			// protocol crosses.
		QSet<FakeWire *> functionSeparatedConductors() const { return m_separated; }
		QPointF pos() const { return m_pos; }

			// Joins wires into one potential; like the real one, a wire's
			// potential lists the others, not itself.
		static void join(const QList<FakeWire *> &wires)
		{
			for (FakeWire *w : wires)
				for (FakeWire *o : wires)
					if (o != w)
						w->m_potential.insert(o);
		}

			// Marks two groups as separated by a function separated
			// terminal element, without joining their potentials.
		static void separate(const QList<FakeWire *> &one,
				     const QList<FakeWire *> &other)
		{
			for (FakeWire *w : one)
				for (FakeWire *o : other)
				{
					w->m_separated.insert(o);
					o->m_separated.insert(w);
				}
		}

		ConductorProperties m_properties;

	private:
		QSet<FakeWire *> m_potential;
		QSet<FakeWire *> m_separated;
		QPointF m_pos;
};

// The rules for editing several wires at once in the Selection properties
// panel (#500): which wire is shown first, which wires an edit reaches,
// what one undo step does to all of them, and the fields shown blank
// because the wires do not agree on them.
class tst_conductormultiedit : public QObject
{
	Q_OBJECT

	using Wires = QList<FakeWire *>;

	static ConductorProperties edited(ConductorProperties p,
					  QString ConductorProperties::*field,
					  const QString &value)
	{
		p.*field = value;
		return p;
	}

	private slots:
			// Same wires, any selection order: same first wire
		void firstWireIsStable()
		{
			FakeWire a("1", "", "", {50, 10}), b("2", "", "", {10, 10}),
				 c("3", "", "", {0, 90});
			const auto pos = [](FakeWire *w) { return w->pos(); };

			Wires one {&c, &a, &b}, two {&a, &b, &c};
			ConductorMultiEdit::sortByPosition(one, pos);
			ConductorMultiEdit::sortByPosition(two, pos);
			QCOMPARE(one, (Wires {&b, &a, &c}));
			QCOMPARE(two, one);
		}

		void targetsWithoutPotential()
		{
			FakeWire a("1", "", "", {}), b("2", "", "", {}), c("3", "", "", {});
			FakeWire::join({&a, &c});
			QCOMPARE(ConductorMultiEdit::targets(Wires {&a, &b, &a}, false),
				 (Wires {&a, &b}));
		}

			// Each wire once: a selected wire already reached through
			// another one's potential is not added again
		void targetsWithPotential()
		{
			FakeWire a("1", "", "", {}), b("1", "", "", {}), c("1", "", "", {}),
				 d("2", "", "", {});
			FakeWire::join({&a, &b, &c});
			const Wires t = ConductorMultiEdit::targets(Wires {&a, &d, &c}, true);
			QCOMPARE(t.size(), 4);
			QCOMPARE(QSet<FakeWire *>(t.begin(), t.end()),
				 (QSet<FakeWire *> {&a, &b, &c, &d}));
		}

			// A function separated terminal element: the other side of the
			// terminal is reached too, but only the voltage / protocol
			// crosses it -- not the function, not the number.
		void functionSeparatedGetsVoltageProtocolOnly()
		{
			FakeWire a("1", "fnA", "", {}), b("2", "fnB", "", {}),
				 c("3", "fnC", "", {});
			FakeWire::join({&a, &b});
			FakeWire::separate({&a}, {&c});

			QSet<FakeWire *> separated;
			const Wires t = ConductorMultiEdit::targets(Wires {&a}, true,
								      &separated);
			QCOMPARE(QSet<FakeWire *>(t.begin(), t.end()),
				 (QSet<FakeWire *> {&a, &b, &c}));
			QCOMPARE(separated, (QSet<FakeWire *> {&c}));
				// A caller that does not know about the rule gets the
				// separated side left out, as before
			QCOMPARE(ConductorMultiEdit::targets(Wires {&a}, true),
				 (Wires {&a, &b}));

			const ConductorProperties shown = a.properties();
			ConductorProperties after = edited(shown,
				&ConductorProperties::m_function, "PWR");
			after.m_tension_protocol = "24V";

			QUndoStack stack;
			stack.push(ConductorMultiEdit::undo(t, shown, after, separated));
			QCOMPARE(stack.count(), 1);

			for (FakeWire *w : {&a, &b})
			{
				QCOMPARE(w->m_properties.m_function, QStringLiteral("PWR"));
				QCOMPARE(w->m_properties.m_tension_protocol,
					 QStringLiteral("24V"));
			}
			QCOMPARE(c.m_properties.m_function, QStringLiteral("fnC"));
			QCOMPARE(c.m_properties.text, QStringLiteral("3"));
			QCOMPARE(c.m_properties.m_tension_protocol, QStringLiteral("24V"));

			stack.undo();
			QVERIFY(c.properties().m_tension_protocol.isEmpty());
			QCOMPARE(c.m_properties.m_function, QStringLiteral("fnC"));
		}

			// The explicitly picked side of a separation: selecting one
			// wire on each side and editing them edits them both, in
			// full -- however the selection happens to be walked (#review)
		void selectedWireIsNeverSeparated()
		{
			FakeWire a("1", "fnA", "", {}), c("3", "fnC", "", {});
			FakeWire::separate({&a}, {&c});

			const ConductorProperties original_a = a.properties();
			const ConductorProperties original_c = c.properties();
			const ConductorProperties after = edited(original_a,
				&ConductorProperties::m_function, "PWR");

			for (const Wires &selection : {Wires {&a, &c}, Wires {&c, &a}})
			{
					//Each order starts from the same, unedited state
				a.setProperties(original_a);
				c.setProperties(original_c);

				QSet<FakeWire *> separated;
				const Wires t = ConductorMultiEdit::targets(selection, true,
								      &separated);
					//c is picked, so it is not "the far side" of anything
				QCOMPARE(separated, (QSet<FakeWire *> {}));

				QUndoStack stack;
				QUndoCommand *undo = ConductorMultiEdit::undo(t, original_a,
									     after, separated);
				QVERIFY(undo);
				stack.push(undo);
				QCOMPARE(a.m_properties.m_function, QStringLiteral("PWR"));
				QCOMPARE(c.m_properties.m_function, QStringLiteral("PWR"));
			}
		}

			// A wire that belongs to the potential as well -- on any path,
			// however far that path runs -- takes the full edit, even when a
			// separation also reaches it. The fake's potential is one hop
			// deep, unlike the real one, so the shared potential is stated
			// in full here.
		void wireReachableBothWaysIsNotSeparated()
		{
			FakeWire a("1", "fnA", "", {}), b("2", "fnB", "", {}),
				 c("3", "fnC", "", {});
			FakeWire::join({&a, &b, &c});
			FakeWire::separate({&a}, {&c});

			QSet<FakeWire *> separated;
			const Wires t = ConductorMultiEdit::targets(Wires {&a}, true,
								      &separated);
			QCOMPARE(QSet<FakeWire *>(t.begin(), t.end()),
				 (QSet<FakeWire *> {&a, &b, &c}));
			QCOMPARE(separated, (QSet<FakeWire *> {}));

			const ConductorProperties shown = a.properties();
			QUndoStack stack;
			stack.push(ConductorMultiEdit::undo(t, shown, edited(shown,
				&ConductorProperties::m_function, "PWR"), separated));
			QCOMPARE(c.m_properties.m_function, QStringLiteral("PWR"));
		}

			// The far side only takes the voltage when the edit changed it:
			// editing the function of a 24 V wire must leave the 0 V one
			// at 0 V, not drag it along (#review)
		void separatedWireKeepsVoltageWhenNotEdited()
		{
			FakeWire a("1", "fnA", "", {}), c("3", "fnC", "", {});
			FakeWire::separate({&a}, {&c});
			a.m_properties.m_tension_protocol = "24V";
			c.m_properties.m_tension_protocol = "0V";

			QSet<FakeWire *> separated;
			const Wires t = ConductorMultiEdit::targets(Wires {&a}, true,
								      &separated);
			const ConductorProperties shown = a.properties();
			QUndoStack stack;

				//Only the function changes: the far side changes nothing
			stack.push(ConductorMultiEdit::undo(t, shown, edited(shown,
				&ConductorProperties::m_function, "PWR"), separated));
			QCOMPARE(c.m_properties.m_function, QStringLiteral("fnC"));
			QCOMPARE(c.m_properties.m_tension_protocol, QStringLiteral("0V"));

				//Now the voltage changes too: it crosses, and only it
			const ConductorProperties shown_now = a.properties();
			stack.push(ConductorMultiEdit::undo(t, shown_now, edited(shown_now,
				&ConductorProperties::m_tension_protocol, "30V"), separated));
			QCOMPARE(c.m_properties.m_tension_protocol, QStringLiteral("30V"));
			QCOMPARE(c.m_properties.m_function, QStringLiteral("fnC"));
			QCOMPARE(a.m_properties.m_tension_protocol, QStringLiteral("30V"));
		}

			// One edit of three wires: each takes the function, keeps its
			// own number and cable; one undo restores all three exactly
		void undoRedoThreeWires()
		{
			FakeWire a("101", "fnA", "W1", {}), b("102", "fnB", "W2", {}),
				 c("103", "fnC", "W3", {});
			const Wires wires {&a, &b, &c};
			const ConductorProperties before_a = a.properties(),
				before_b = b.properties(), before_c = c.properties();

			const ConductorProperties shown = a.properties();
			QUndoStack stack;
			stack.push(ConductorMultiEdit::undo(wires, shown,
				edited(shown, &ConductorProperties::m_function, "PWR")));
			QCOMPARE(stack.count(), 1);

			for (FakeWire *w : wires)
				QCOMPARE(w->m_properties.m_function, QStringLiteral("PWR"));
			QCOMPARE(b.m_properties.text, QStringLiteral("102"));
			QCOMPARE(c.m_properties.m_cable, QStringLiteral("W3"));

			stack.undo();
			QVERIFY(a.properties() == before_a);
			QVERIFY(b.properties() == before_b);
			QVERIFY(c.properties() == before_c);

			stack.redo();
			QCOMPARE(c.m_properties.m_function, QStringLiteral("PWR"));
			QCOMPARE(c.m_properties.text, QStringLiteral("103"));
		}

		void noUndoWhenNothingChanges()
		{
			FakeWire a("1", "PWR", "", {}), b("2", "PWR", "", {});
			const ConductorProperties shown = a.properties();
			QCOMPARE(ConductorMultiEdit::undo(Wires {&a, &b}, shown, shown),
				 static_cast<QUndoCommand *>(nullptr));
				// An edit every wire already has
			QCOMPARE(ConductorMultiEdit::undo(Wires {&a, &b}, edited(shown,
				&ConductorProperties::m_function, "X"), shown),
				 static_cast<QUndoCommand *>(nullptr));
		}

			// Wires that disagree on a text field show it blank; typing the
			// first wire's own value then reaches every wire (1.5 mm² on
			// a 1.5 and a 2.5 wire)
		void mixedFieldTakesFirstWiresValue()
		{
			FakeWire a("1", "fn", "", {}), b("2", "fn", "", {});
			a.m_properties.m_wire_section = "1.5";
			b.m_properties.m_wire_section = "2.5";
			const QList<ConductorProperties> list {a.properties(), b.properties()};

			const auto mixed = ConductorMultiEdit::mixedTextFields(list);
			QCOMPARE(mixed, (QList<ConductorMultiEdit::TextField> {
				ConductorMultiEdit::Text, ConductorMultiEdit::WireSection}));

			const ConductorProperties shown = ConductorMultiEdit::shown(list, mixed);
			QVERIFY(shown.m_wire_section.isEmpty());
			QCOMPARE(shown.m_function, QStringLiteral("fn"));

			QUndoStack stack;
			stack.push(ConductorMultiEdit::undo(Wires {&a, &b}, shown,
				edited(shown, &ConductorProperties::m_wire_section, "1.5")));
			QCOMPARE(a.m_properties.m_wire_section, QStringLiteral("1.5"));
			QCOMPARE(b.m_properties.m_wire_section, QStringLiteral("1.5"));
				// Another field edited while the section is mixed leaves
				// each wire's section alone
			a.m_properties.m_wire_section = "1.5";
			b.m_properties.m_wire_section = "2.5";
			stack.push(ConductorMultiEdit::undo(Wires {&a, &b}, shown,
				edited(shown, &ConductorProperties::m_function, "PWR")));
			QCOMPARE(a.m_properties.m_wire_section, QStringLiteral("1.5"));
			QCOMPARE(b.m_properties.m_wire_section, QStringLiteral("2.5"));
		}

		void oneWireIsNeverMixed()
		{
			FakeWire a("1", "fn", "", {});
			QVERIFY(ConductorMultiEdit::mixedTextFields({a.properties()}).isEmpty());
		}
};

QTEST_GUILESS_MAIN(tst_conductormultiedit)
#include "tst_conductormultiedit.moc"
