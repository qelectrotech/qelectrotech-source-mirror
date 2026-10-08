// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <functional>

#include "conductorproperties.h"
#include "qetapp.h"

	// qet.cpp needs it; the application is not linked
QString QETApp::m_interface_language;

// ConductorProperties::applyChanges(): the Selection properties panel
// applies to each wire only the fields the user changed. Every field is
// checked on its own: change that one field, and a wire whose every field
// differs must take that field and keep all the others.
class tst_conductorapplychanges : public QObject
{
	Q_OBJECT

	using Setter = std::function<void(ConductorProperties &, int)>;

		// One setter per field. Variant 1 always differs from 0 and 2, so
		// a field that is not copied leaves the result visibly wrong;
		// two-valued fields reuse variant 0's value for 2.
	static QList<QPair<const char *, Setter>> fields()
	{
		return {
			{"type", [](ConductorProperties &p, int v) {
				p.type = v == 1 ? ConductorProperties::Single : ConductorProperties::Multi; }},
			{"color", [](ConductorProperties &p, int v) {
				p.color = QColor::fromRgb(10 + v, 0, 0); }},
			{"bicolor", [](ConductorProperties &p, int v) {
				p.m_bicolor = v == 1; }},
			{"color_2", [](ConductorProperties &p, int v) {
				p.m_color_2 = QColor::fromRgb(0, 10 + v, 0); }},
			{"dash_size", [](ConductorProperties &p, int v) {
				p.m_dash_size = 2 + v; }},
			{"style", [](ConductorProperties &p, int v) {
				p.style = Qt::PenStyle(Qt::SolidLine + v); }},
			{"text", [](ConductorProperties &p, int v) {
				p.text = QStringLiteral("text%1").arg(v); }},
			{"text_color", [](ConductorProperties &p, int v) {
				p.text_color = QColor::fromRgb(0, 0, 10 + v); }},
			{"formula", [](ConductorProperties &p, int v) {
				p.m_formula = QStringLiteral("formula%1").arg(v); }},
			{"cable", [](ConductorProperties &p, int v) {
				p.m_cable = QStringLiteral("cable%1").arg(v); }},
			{"bus", [](ConductorProperties &p, int v) {
				p.m_bus = QStringLiteral("bus%1").arg(v); }},
			{"function", [](ConductorProperties &p, int v) {
				p.m_function = QStringLiteral("function%1").arg(v); }},
			{"tension_protocol", [](ConductorProperties &p, int v) {
				p.m_tension_protocol = QStringLiteral("24V%1").arg(v); }},
			{"wire_color", [](ConductorProperties &p, int v) {
				p.m_wire_color = QStringLiteral("BU%1").arg(v); }},
			{"wire_section", [](ConductorProperties &p, int v) {
				p.m_wire_section = QStringLiteral("1.%1").arg(v); }},
			{"show_text", [](ConductorProperties &p, int v) {
				p.m_show_text = v == 1; }},
			{"text_size", [](ConductorProperties &p, int v) {
				p.text_size = 7 + v; }},
			{"cond_size", [](ConductorProperties &p, int v) {
				p.cond_size = 1.5 + v; }},
			{"verti_rotate_text", [](ConductorProperties &p, int v) {
				p.verti_rotate_text = 90.0 * v; }},
			{"horiz_rotate_text", [](ConductorProperties &p, int v) {
				p.horiz_rotate_text = 45.0 * v; }},
			{"one_text_per_folio", [](ConductorProperties &p, int v) {
				p.m_one_text_per_folio = v == 1; }},
			{"horizontal_alignment", [](ConductorProperties &p, int v) {
				p.m_horizontal_alignment = v == 0 ? Qt::AlignTop
					: v == 1 ? Qt::AlignBottom : Qt::AlignVCenter; }},
			{"vertical_alignment", [](ConductorProperties &p, int v) {
				p.m_vertical_alignment = v == 0 ? Qt::AlignLeft
					: v == 1 ? Qt::AlignRight : Qt::AlignHCenter; }},
			{"single_line_ground", [](ConductorProperties &p, int v) {
				p.singleLineProperties.hasGround = v == 1; }},
			{"single_line_neutral", [](ConductorProperties &p, int v) {
				p.singleLineProperties.hasNeutral = v == 1; }},
			{"single_line_pen", [](ConductorProperties &p, int v) {
				p.singleLineProperties.is_pen = v == 1; }},
			{"single_line_phases", [](ConductorProperties &p, int v) {
				p.singleLineProperties.setPhasesCount(v + 1); }},
		};
	}

		// Every field set to the given variant
	static ConductorProperties all(int variant)
	{
		ConductorProperties p;
		for (const auto &f : fields())
			f.second(p, variant);
		return p;
	}

	private slots:
		void eachFieldAlone_data()
		{
			QTest::addColumn<int>("index");
			const auto list = fields();
			for (int i = 0; i < list.size(); ++i)
				QTest::newRow(list.at(i).first) << i;
		}

			// The shown wire is all variant 0 and the user changes one
			// field to variant 1. The other wire holds variant 1 in every
			// other field, so copying one of them by mistake (they are
			// variant 0 in the edit) shows, booleans included; and variant
			// 2 in the edited field, so not copying it shows too.
		void eachFieldAlone()
		{
			QFETCH(int, index);
			const Setter set = fields().at(index).second;

			const ConductorProperties before = all(0);
			ConductorProperties after = before;
			set(after, 1);
			QVERIFY(after != before);

			ConductorProperties other = all(1);
			set(other, 2);
			ConductorProperties expected = other;
			set(expected, 1);
			QVERIFY(expected != other);

			other.applyChanges(before, after);
			QVERIFY(other == expected);
		}

			// Nothing edited: nothing copied, whatever the wire holds
		void noEditChangesNothing()
		{
			const ConductorProperties shown = all(0);
			ConductorProperties other = all(1);
			const ConductorProperties kept = other;
			other.applyChanges(shown, shown);
			QVERIFY(other == kept);
		}

			// Every field edited: the wire ends up exactly as edited, so
			// no field compared by operator== is left out of applyChanges
		void everyFieldEdited()
		{
			ConductorProperties other = all(2);
			other.applyChanges(all(0), all(1));
			QVERIFY(other == all(1));
		}

			// Set the function on a wire with its own number and cable
		void functionOnTwoWires()
		{
			ConductorProperties shown;
			shown.text = QStringLiteral("101");
			shown.m_cable = QStringLiteral("W1");
			ConductorProperties edited = shown;
			edited.m_function = QStringLiteral("24V DC");

			ConductorProperties second;
			second.text = QStringLiteral("102");
			second.m_cable = QStringLiteral("W2");
			second.m_function = QStringLiteral("old");
			second.applyChanges(shown, edited);

			QCOMPARE(second.m_function, QStringLiteral("24V DC"));
			QCOMPARE(second.text, QStringLiteral("102"));
			QCOMPARE(second.m_cable, QStringLiteral("W2"));
		}
};

QTEST_GUILESS_MAIN(tst_conductorapplychanges)
#include "tst_conductorapplychanges.moc"
