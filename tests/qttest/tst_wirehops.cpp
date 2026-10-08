// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

#include "wirehops.h"

// Hops where two wires cross (issue #436). The geometry is tested on its
// own; the project setting is tested through the real binary: --resave
// must keep a <wire_crossings> element, and must not add one to a project
// that never had it, so existing projects save exactly as before.
class tst_wirehops : public QObject
{
	Q_OBJECT

	using Mode = WireHops::Mode;
	QTemporaryDir m_dir;
	int m_run = 0;

	static QVector<QPointF> line(QPointF a, QPointF b) { return {a, b}; }

		// Runs --resave on @p in, returns the saved file's text (empty on failure)
	QString resave(const QString &in)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run));
		const QString tmp = m_dir.filePath(QStringLiteral("tmp%1").arg(m_run));
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run++));
		QDir().mkpath(home);
		QDir().mkpath(tmp);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/.config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/.local/share"));
		env.insert(QStringLiteral("TMPDIR"), tmp);

		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), in, out});
		if (!proc.waitForFinished(120000) || proc.exitCode() != 0)
			return {};
		QFile file(out);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return {};
		return QString::fromUtf8(file.readAll());
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
	}

	void modeNames()
	{
		QCOMPARE(int(WireHops::fromString(WireHops::toString(Mode::Horizontal))), int(Mode::Horizontal));
		QCOMPARE(int(WireHops::fromString(WireHops::toString(Mode::Vertical))), int(Mode::Vertical));
		QCOMPARE(int(WireHops::fromString(WireHops::toString(Mode::None))), int(Mode::None));
		QCOMPARE(int(WireHops::fromString(QStringLiteral("diagonal"))), int(Mode::None));
		QCOMPARE(int(WireHops::fromString(QString())), int(Mode::None));
	}

	void crossing_data()
	{
		QTest::addColumn<QVector<QPointF>>("wire");
		QTest::addColumn<QVector<QPointF>>("other");
		QTest::addColumn<int>("mode");
		QTest::addColumn<QList<QPointF>>("expected");

		const auto horizontal = line({0, 0}, {100, 0});
		const auto vertical = line({50, -20}, {50, 20});
		const int h = int(Mode::Horizontal), v = int(Mode::Vertical), none = int(Mode::None);

		QTest::newRow("off") << horizontal << vertical << none << QList<QPointF>();
		QTest::newRow("horizontal hops") << horizontal << vertical << h << QList<QPointF>{{50, 0}};
		QTest::newRow("horizontal mode, vertical wire") << vertical << horizontal << h << QList<QPointF>();
		QTest::newRow("vertical hops") << vertical << horizontal << v << QList<QPointF>{{50, 0}};
		QTest::newRow("vertical mode, horizontal wire") << horizontal << vertical << v << QList<QPointF>();
		QTest::newRow("reversed wire") << line({100, 0}, {0, 0}) << vertical << h << QList<QPointF>{{50, 0}};
		QTest::newRow("T: other ends on the wire") << horizontal << line({50, 0}, {50, 20}) << h << QList<QPointF>();
		QTest::newRow("T: other starts on the wire") << horizontal << line({50, -20}, {50, 0}) << h << QList<QPointF>();
		QTest::newRow("too near the wire's end") << horizontal << line({2, -20}, {2, 20}) << h << QList<QPointF>();
		QTest::newRow("parallel") << horizontal << line({0, 10}, {100, 10}) << h << QList<QPointF>();
		QTest::newRow("misses") << horizontal << line({150, -20}, {150, 20}) << h << QList<QPointF>();
		QTest::newRow("bent wire, second segment")
				<< QVector<QPointF>{{0, -40}, {0, 0}, {100, 0}} << vertical << h << QList<QPointF>{{50, 0}};
	}

	void crossing()
	{
		QFETCH(QVector<QPointF>, wire);
		QFETCH(QVector<QPointF>, other);
		QFETCH(int, mode);
		QFETCH(QList<QPointF>, expected);
		QCOMPARE(WireHops::crossings(wire, {other}, Mode(mode)), expected);
	}

	void closeCrossings()
	{
		const auto wire = line({0, 0}, {100, 0});
			// 6 apart: the arcs would overlap, so only the first hops
		QCOMPARE(WireHops::crossings(wire, {line({40, -20}, {40, 20}), line({46, -20}, {46, 20})}, Mode::Horizontal),
				 (QList<QPointF>{{40, 0}}));
			// 10 apart, one grid step: both hop, in the wire's order
		QCOMPARE(WireHops::crossings(wire, {line({60, -20}, {60, 20}), line({50, -20}, {50, 20})}, Mode::Horizontal),
				 (QList<QPointF>{{50, 0}, {60, 0}}));
	}

	void pathWithoutHopsIsThePlainWire()
	{
		const QVector<QPointF> wire{{0, -40}, {0, 0}, {100, 0}};
		QPainterPath plain;
		plain.moveTo(wire.at(0));
		plain.lineTo(wire.at(1));
		plain.lineTo(wire.at(2));
		QCOMPARE(WireHops::path(wire, {}, Mode::Horizontal), plain);
		QCOMPARE(WireHops::path(wire, {{50, 0}}, Mode::None), plain);
	}

	void pathArcs_data()
	{
		QTest::addColumn<QVector<QPointF>>("wire");
		QTest::addColumn<int>("mode");
		QTest::addColumn<QRectF>("bounds");

		const qreal r = WireHops::radius;
			// The arc goes above a horizontal wire, right of a vertical one,
			// whichever way the wire runs
		QTest::newRow("left to right") << line({0, 0}, {100, 0}) << int(Mode::Horizontal)
									   << QRectF(0, -r, 100, r);
		QTest::newRow("right to left") << line({100, 0}, {0, 0}) << int(Mode::Horizontal)
									   << QRectF(0, -r, 100, r);
		QTest::newRow("top to bottom") << line({50, -20}, {50, 20}) << int(Mode::Vertical)
									   << QRectF(50, -20, r, 40);
		QTest::newRow("bottom to top") << line({50, 20}, {50, -20}) << int(Mode::Vertical)
									   << QRectF(50, -20, r, 40);
	}

	void pathArcs()
	{
		QFETCH(QVector<QPointF>, wire);
		QFETCH(int, mode);
		QFETCH(QRectF, bounds);

		const QPointF hop(50, 0);
		const QPainterPath path = WireHops::path(wire, {hop}, Mode(mode));
		const QRectF actual = path.boundingRect();
		QVERIFY2(qAbs(actual.left() - bounds.left()) < 0.01 && qAbs(actual.top() - bounds.top()) < 0.01
				 && qAbs(actual.right() - bounds.right()) < 0.01 && qAbs(actual.bottom() - bounds.bottom()) < 0.01,
				 qPrintable(QStringLiteral("bounds %1,%2 %3x%4").arg(actual.x()).arg(actual.y())
							.arg(actual.width()).arg(actual.height())));
		QCOMPARE(path.pointAtPercent(0), wire.first());
		QCOMPARE(path.pointAtPercent(1), wire.last());
	}

	void savedSettingSurvivesResave()
	{
		const QString fixture = QFINDTESTDATA("fixtures/wiring_list_arrows.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");

			// The fixture has no setting: a resave must not add one
		const QString plain = resave(fixture);
		QVERIFY2(!plain.isEmpty(), "--resave failed");
		QVERIFY(!plain.contains(QLatin1String("wire_crossings")));

			// With the setting, a resave keeps it
		QFile source(fixture);
		QVERIFY(source.open(QIODevice::ReadOnly | QIODevice::Text));
		QString text = QString::fromUtf8(source.readAll());
		const int newdiagrams = text.indexOf(QLatin1String("<newdiagrams"));
		QVERIFY(newdiagrams > 0);
		text.insert(newdiagrams, QStringLiteral("<wire_crossings hop=\"vertical\"/>\n    "));
		const QString with_setting = m_dir.filePath(QStringLiteral("with_setting.qet"));
		QFile out(with_setting);
		QVERIFY(out.open(QIODevice::WriteOnly | QIODevice::Text));
		out.write(text.toUtf8());
		out.close();

		const QString saved = resave(with_setting);
		QVERIFY2(!saved.isEmpty(), "--resave failed");
		QVERIFY2(saved.contains(QLatin1String("<wire_crossings hop=\"vertical\"/>")),
				 "the setting was lost on save");
	}
};

QTEST_GUILESS_MAIN(tst_wirehops)

#include "tst_wirehops.moc"
