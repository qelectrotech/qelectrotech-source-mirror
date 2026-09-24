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
	SpaceMouseMotion::map() -- how one 3D mouse sample pans and zooms a
	view, under the user's settings.

	Nobody working on this owns a device, so this is where the behaviour is
	pinned down: speed must not depend on how often the driver sends
	samples, zoom must stay a positive factor however hard the cap is
	pushed, and each setting must do what its label says. The defaults must
	also keep the behaviour from before the settings existed.

	Uses its own QSettings scope, so it never touches the configuration of
	whoever runs it.
*/

#include "spacemouse/spacemousemotion.h"

#include <QSettings>
#include <QtMath>
#include <QtTest>

namespace {
	SpaceMouseSample translate(int x, int y, int z)
	{
		SpaceMouseSample s;
		s.x = x;
		s.y = y;
		s.z = z;
		return s;
	}

	constexpr qint64 NOMINAL = 16;
}

class TstSpaceMouseMotion : public QObject
{
		Q_OBJECT

	private slots:
		void initTestCase();

		void stepFollowsElapsedTime_data();
		void stepFollowsElapsedTime();
		void deadZone_data();
		void deadZone();
		void defaultsKeepThePreviousBehaviour();
		void speedDoesNotDependOnSampleRate();
		void zoomIsAlwaysAPositiveFactor();
		void equalPushAndPullCancelOut();
		void speedSettingsScale();
		void invertFlipsOnlyItsOwnAxis();
		void twistZoomsAndPushPullIsIgnored();
		void deadZoneAppliesToEveryAxisUsed();
		void settingsDefaultWhenNothingIsSaved();
		void settingsRoundTrip();
};

void TstSpaceMouseMotion::initTestCase()
{
	QCoreApplication::setOrganizationName(
				QStringLiteral("QElectroTech-tst_spacemousemotion"));
	QCoreApplication::setApplicationName(
				QStringLiteral("tst_spacemousemotion"));
	QSettings().clear();
}

void TstSpaceMouseMotion::stepFollowsElapsedTime_data()
{
	QTest::addColumn<qint64>("elapsed");
	QTest::addColumn<qreal>("step");
	QTest::newRow("first sample") << qint64(-1) << 1.0;
	QTest::newRow("same millisecond: covers no time") << qint64(0) << 0.0;
	QTest::newRow("twice as fast") << qint64(8) << 0.5;
	QTest::newRow("nominal") << NOMINAL << 1.0;
	QTest::newRow("twice as slow") << qint64(32) << 2.0;
	QTest::newRow("longest gap still counted")
			<< SpaceMouseMotion::MAX_PERIOD_MS
			<< SpaceMouseMotion::MAX_PERIOD_MS / SpaceMouseMotion::NOMINAL_PERIOD_MS;
	QTest::newRow("after a rest") << SpaceMouseMotion::MAX_PERIOD_MS + 1 << 1.0;
}

void TstSpaceMouseMotion::stepFollowsElapsedTime()
{
	QFETCH(qint64, elapsed);
	QFETCH(qreal, step);
	QCOMPARE(SpaceMouseMotion::stepFor(elapsed), step);
}

void TstSpaceMouseMotion::deadZone_data()
{
	QTest::addColumn<int>("value");
	QTest::addColumn<int>("dead_zone");
	QTest::addColumn<int>("result");
	QTest::newRow("no dead zone") << 1 << 0 << 1;
	QTest::newRow("inside") << 2 << 3 << 0;
	QTest::newRow("on the edge") << -3 << 3 << 0;
	QTest::newRow("outside, positive") << 5 << 3 << 2;
	QTest::newRow("outside, negative") << -5 << 3 << -2;
}

void TstSpaceMouseMotion::deadZone()
{
	QFETCH(int, value);
	QFETCH(int, dead_zone);
	QFETCH(int, result);
	QCOMPARE(SpaceMouseMotion::applyDeadZone(value, dead_zone), result);
}

void TstSpaceMouseMotion::defaultsKeepThePreviousBehaviour()
{
	const SpaceMouseSettings defaults;

		// Before the settings existed a sample moved the scrollbar by
		// minus its deflection, and zoomed by 1 + z/1000.
	const SpaceMouseViewMotion pan =
			SpaceMouseMotion::map(translate(20, -7, 0), NOMINAL, defaults);
	QCOMPARE(pan.scroll_x, -20.0);
	QCOMPARE(pan.scroll_y, 7.0);
	QCOMPARE(pan.zoom_factor, 1.0);

	const SpaceMouseViewMotion zoom =
			SpaceMouseMotion::map(translate(0, 0, 50), NOMINAL, defaults);
	QCOMPARE(zoom.scroll_x, 0.0);
	QCOMPARE(zoom.scroll_y, 0.0);
	QVERIFY2(qAbs(zoom.zoom_factor - 1.05) < 0.002,
		 qPrintable(QString::number(zoom.zoom_factor)));
}

void TstSpaceMouseMotion::speedDoesNotDependOnSampleRate()
{
	const SpaceMouseSettings defaults;
	const SpaceMouseSample push = translate(30, 12, 80);

		// One driver sending every 16 ms, another every 8 ms, cap held the
		// same for 16 ms: the view must end up in the same place.
	const SpaceMouseViewMotion slow = SpaceMouseMotion::map(push, 16, defaults);
	const SpaceMouseViewMotion fast = SpaceMouseMotion::map(push, 8, defaults);

	QCOMPARE(2 * fast.scroll_x, slow.scroll_x);
	QCOMPARE(2 * fast.scroll_y, slow.scroll_y);
	QVERIFY(qFuzzyCompare(fast.zoom_factor * fast.zoom_factor, slow.zoom_factor));
}

void TstSpaceMouseMotion::zoomIsAlwaysAPositiveFactor()
{
	SpaceMouseSettings fastest;
	fastest.zoom_speed = 400;

		// 1 + z/1000 went to zero and below for a hard pull; a zoom of
		// zero or less would flip or collapse the view.
	for (int z : {-100000, -5000, -1000, -350, 350, 5000}) {
		const qreal factor =
				SpaceMouseMotion::map(translate(0, 0, z), NOMINAL, fastest).zoom_factor;
		QVERIFY2(factor > 0 && qIsFinite(factor),
			 qPrintable(QStringLiteral("z=%1 gives %2").arg(z).arg(factor)));
	}
}

void TstSpaceMouseMotion::equalPushAndPullCancelOut()
{
	const SpaceMouseSettings defaults;
	const qreal in = SpaceMouseMotion::map(translate(0, 0, 120), NOMINAL, defaults).zoom_factor;
	const qreal out = SpaceMouseMotion::map(translate(0, 0, -120), NOMINAL, defaults).zoom_factor;
	QVERIFY(qFuzzyCompare(in * out, 1.0));
}

void TstSpaceMouseMotion::speedSettingsScale()
{
	const SpaceMouseSettings defaults;
	SpaceMouseSettings doubled;
	doubled.pan_speed = 200;
	doubled.zoom_speed = 200;
	const SpaceMouseSample push = translate(15, 25, 60);

	const SpaceMouseViewMotion base = SpaceMouseMotion::map(push, NOMINAL, defaults);
	const SpaceMouseViewMotion twice = SpaceMouseMotion::map(push, NOMINAL, doubled);

	QCOMPARE(twice.scroll_x, 2 * base.scroll_x);
	QCOMPARE(twice.scroll_y, 2 * base.scroll_y);
	QVERIFY(qFuzzyCompare(twice.zoom_factor, base.zoom_factor * base.zoom_factor));
}

void TstSpaceMouseMotion::invertFlipsOnlyItsOwnAxis()
{
	const SpaceMouseSettings defaults;
	const SpaceMouseSample push = translate(15, 25, 60);
	const SpaceMouseViewMotion base = SpaceMouseMotion::map(push, NOMINAL, defaults);

	SpaceMouseSettings s;
	s.invert_pan_x = true;
	SpaceMouseViewMotion m = SpaceMouseMotion::map(push, NOMINAL, s);
	QCOMPARE(m.scroll_x, -base.scroll_x);
	QCOMPARE(m.scroll_y, base.scroll_y);
	QCOMPARE(m.zoom_factor, base.zoom_factor);

	s = SpaceMouseSettings();
	s.invert_pan_y = true;
	m = SpaceMouseMotion::map(push, NOMINAL, s);
	QCOMPARE(m.scroll_x, base.scroll_x);
	QCOMPARE(m.scroll_y, -base.scroll_y);
	QCOMPARE(m.zoom_factor, base.zoom_factor);

	s = SpaceMouseSettings();
	s.invert_zoom = true;
	m = SpaceMouseMotion::map(push, NOMINAL, s);
	QCOMPARE(m.scroll_x, base.scroll_x);
	QCOMPARE(m.scroll_y, base.scroll_y);
	QVERIFY(qFuzzyCompare(m.zoom_factor * base.zoom_factor, 1.0));
}

void TstSpaceMouseMotion::twistZoomsAndPushPullIsIgnored()
{
	SpaceMouseSettings twist;
	twist.zoom_axis = SpaceMouseSettings::ZoomAxis::Twist;

	SpaceMouseSample pushed = translate(0, 0, 200);
	QCOMPARE(SpaceMouseMotion::map(pushed, NOMINAL, twist).zoom_factor, 1.0);

	SpaceMouseSample twisted;
	twisted.rz = 50;
	const qreal by_twist = SpaceMouseMotion::map(twisted, NOMINAL, twist).zoom_factor;
	const qreal by_push = SpaceMouseMotion::map(translate(0, 0, 50), NOMINAL,
						    SpaceMouseSettings()).zoom_factor;
	QCOMPARE(by_twist, by_push);

		// the other rotations never zoom
	SpaceMouseSample tilted;
	tilted.rx = 300;
	tilted.ry = -300;
	QCOMPARE(SpaceMouseMotion::map(tilted, NOMINAL, twist).zoom_factor, 1.0);
}

void TstSpaceMouseMotion::deadZoneAppliesToEveryAxisUsed()
{
	SpaceMouseSettings s;
	s.dead_zone = 10;
	const SpaceMouseViewMotion drift =
			SpaceMouseMotion::map(translate(9, -10, 8), NOMINAL, s);
	QCOMPARE(drift.scroll_x, 0.0);
	QCOMPARE(drift.scroll_y, 0.0);
	QCOMPARE(drift.zoom_factor, 1.0);

	s.zoom_axis = SpaceMouseSettings::ZoomAxis::Twist;
	SpaceMouseSample twisted;
	twisted.rz = 10;
	QCOMPARE(SpaceMouseMotion::map(twisted, NOMINAL, s).zoom_factor, 1.0);
}

void TstSpaceMouseMotion::settingsDefaultWhenNothingIsSaved()
{
	QSettings().clear();
	QVERIFY(SpaceMouseSettings::load() == SpaceMouseSettings());
}

void TstSpaceMouseMotion::settingsRoundTrip()
{
	SpaceMouseSettings s;
	s.pan_speed = 150;
	s.zoom_speed = 40;
	s.dead_zone = 7;
	s.invert_pan_x = true;
	s.invert_pan_y = true;
	s.invert_zoom = true;
	s.zoom_axis = SpaceMouseSettings::ZoomAxis::Twist;
	s.save();

	QVERIFY(SpaceMouseSettings::load() == s);
	QVERIFY(!(SpaceMouseSettings::load() == SpaceMouseSettings()));
	QSettings().clear();
}

QTEST_GUILESS_MAIN(TstSpaceMouseMotion)
#include "tst_spacemousemotion.moc"
