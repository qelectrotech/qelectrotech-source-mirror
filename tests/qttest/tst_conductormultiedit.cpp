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

		ConductorProperties m_properties;

	private:
		QSet<FakeWire *> m_potential;
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
