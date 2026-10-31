// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDomDocument>
#include <QSettings>

#include "conductordashpattern.h"
#include "conductorproperties.h"
#include "qetapp.h"

	// qet.cpp needs it; the application is not linked
QString QETApp::m_interface_language;

// The custom dash pattern of a conductor: how it draws, and that it is
// saved, loaded and merged like the other conductor properties.
class tst_conductordashpattern : public QObject
{
	Q_OBJECT

	static ConductorDashPattern custom()
	{
		ConductorDashPattern p;
		p.enabled = true;
		p.dash = 8;
		p.gap = 3;
		p.dot = 0.5;
		p.dot_gap = 2;
		return p;
	}

	private slots:
		void disabledPatternChangesNothing()
		{
			QPen pen(Qt::black, 2);
			QVERIFY(!ConductorDashPattern().apply(pen, Qt::DashLine));
			QCOMPARE(pen.style(), Qt::DashLine);
		}

		void lengthsAreInDrawingUnits()
		{
			QPen pen(Qt::black, 2);
			QVERIFY(custom().apply(pen, Qt::DashDotLine));
				// Qt counts the pattern in pen widths
			QCOMPARE(pen.dashPattern(), (QVector<qreal>{4, 1.5, 0.25, 1}));
			QCOMPARE(pen.capStyle(), Qt::FlatCap);

			QPen dashed(Qt::black, 4);
			QVERIFY(custom().apply(dashed, Qt::DashLine));
			QCOMPARE(dashed.dashPattern(), (QVector<qreal>{2, 0.75}));
		}

		void solidLineIsNeverPatterned()
		{
			QPen pen(Qt::black, 2);
			QVERIFY(!custom().apply(pen, Qt::SolidLine));
			QCOMPARE(pen.style(), Qt::SolidLine);
		}

		void invalidLengthsAreRefused()
		{
			ConductorDashPattern p = custom();
			p.gap = 0;
			QVERIFY(!p.valid());
			QPen pen(Qt::black, 2);
			QVERIFY(!p.apply(pen, Qt::DashLine));
			p.gap = std::nan("");
			QVERIFY(!p.valid());
		}

		void xmlRoundTrip()
		{
			ConductorProperties cp;
			cp.m_dash_pattern = custom();
			QDomDocument doc;
			QDomElement e = doc.createElement("conductor");
			cp.toXml(e);
			ConductorProperties back;
			back.fromXml(e);
			QCOMPARE(back.m_dash_pattern, custom());
		}

		void defaultPatternSavesNothing()
		{
			ConductorProperties cp;
			QDomDocument doc;
			QDomElement e = doc.createElement("conductor");
			cp.toXml(e);
			QVERIFY(!e.hasAttribute("custom-dash-pattern"));
			QVERIFY(!e.hasAttribute("pattern-dash"));
		}

		void invalidXmlFallsBackToDefault()
		{
			QDomDocument doc;
			QDomElement e = doc.createElement("conductor");
			e.setAttribute("custom-dash-pattern", "true");
			e.setAttribute("pattern-dash", "-3");
			ConductorProperties cp;
			cp.fromXml(e);
			QCOMPARE(cp.m_dash_pattern, ConductorDashPattern());
		}

		void settingsRoundTrip()
		{
			QTemporaryDir dir;
			QSettings settings(dir.filePath("t.ini"), QSettings::IniFormat);
			ConductorProperties cp;
			cp.m_dash_pattern = custom();
			cp.toSettings(settings, "c/");
			ConductorProperties back;
			back.fromSettings(settings, "c/");
			QCOMPARE(back.m_dash_pattern, custom());
		}

		void editTakesOnlyChangedPatternFields()
		{
			ConductorProperties before, after, wire;
			after.m_dash_pattern.dash = 9;
			wire.m_dash_pattern = custom();
			wire.applyChanges(before, after);
			QCOMPARE(wire.m_dash_pattern.dash, 9.0);
			QCOMPARE(wire.m_dash_pattern.gap, 3.0);
			QVERIFY(wire.m_dash_pattern.enabled);
		}

		void severalWiresKeepOnlyWhatTheyShare()
		{
			ConductorProperties a, b;
			a.m_dash_pattern = custom();
			b.m_dash_pattern = custom();
			b.m_dash_pattern.gap = 5;
			ConductorProperties merged;
			merged.applyForEqualAttributes({a, b});
			QCOMPARE(merged.m_dash_pattern.dash, 8.0);
			QCOMPARE(merged.m_dash_pattern.gap, ConductorDashPattern().gap);
		}
};

QTEST_GUILESS_MAIN(tst_conductordashpattern)
#include "tst_conductordashpattern.moc"
