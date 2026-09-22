#include <QtTest>

#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>

namespace {

struct Point {
	double x = 0.0;
	double y = 0.0;
	bool operator==(const Point &other) const { return x == other.x && y == other.y; }
};

// Mirrors count_retraces.py (qelectrotech-docker), which this project's own
// regression checks for bugtracker #734 already use: restricted to the
// conductor path shape (4-6 points, every segment axis-aligned, not closed)
// so legitimate closed element-graphics shapes (rectangles, polygons) are
// never mistaken for a self-retracing conductor.
QList<Point> parsePoints(const QString &d)
{
	QList<Point> points;
	static const QRegularExpression re(QStringLiteral("[ML]\\s*(-?\\d+\\.?\\d*)[ ,](-?\\d+\\.?\\d*)"));
	auto it = re.globalMatch(d);
	while (it.hasNext()) {
		const QRegularExpressionMatch m = it.next();
		points << Point{m.captured(1).toDouble(), m.captured(2).toDouble()};
	}
	return points;
}

bool isConductorShaped(const QList<Point> &points)
{
	if (points.size() < 4 || points.size() > 6) return false;
	if (points.first() == points.last()) return false; // closed shape, not a conductor
	for (int i = 0; i + 1 < points.size(); ++i) {
		if (points.at(i).x != points.at(i + 1).x && points.at(i).y != points.at(i + 1).y)
			return false; // not axis-aligned
	}
	return true;
}

bool isSelfRetracing(const QList<Point> &points)
{
	// (a) skip-one out-and-back: ... A B A ...
	for (int i = 0; i + 2 < points.size(); ++i) {
		if (points.at(i) == points.at(i + 2)) return true;
	}
	// (b) immediate duplicate vertex: ... A A ...
	for (int i = 0; i + 1 < points.size(); ++i) {
		if (points.at(i) == points.at(i + 1)) return true;
	}
	// (c) 6-point crossed-stub loop: depart0 == arrivee AND depart == arrivee0
	if (points.size() == 6 && points.at(0) == points.at(4) && points.at(1) == points.at(5)) return true;
	return false;
}

} // namespace

// Bugtracker #734: Conductor::generateConductorPath()'s cas "3"/"4" grid-
// snap bridge used to route an unnecessary out-and-back excursion, or a
// small looping detour, when depart and arrivee already shared the axis the
// bridge would run along. This fixture (the report's own canonical
// reproduction, "qet_bug_repro_resaved.qet") is known to trigger it.
class tst_conductorselfretrace : public QObject
{
	Q_OBJECT

private slots:
	void noSelfRetracingPaths()
	{
		const QString binary = QStringLiteral(QET_TEST_BINARY_PATH);
		QVERIFY2(QFile::exists(binary), qPrintable(QStringLiteral("qelectrotech binary not found at '%1'").arg(binary)));

		const QString fixture = QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");

		QTemporaryDir out_dir;
		QVERIFY(out_dir.isValid());

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));

		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(binary, {QStringLiteral("--export-svg"), fixture, out_dir.path()});
		QVERIFY2(proc.waitForFinished(30000), "export --export-svg timed out or failed to start");
		QCOMPARE(proc.exitCode(), 0);

		QDir dir(out_dir.path());
		const QStringList svgs = dir.entryList({QStringLiteral("*.svg")}, QDir::Files);
		QVERIFY2(!svgs.isEmpty(), "no SVG exported");

		static const QRegularExpression path_re(QStringLiteral("<path[^>]*\\bd=\"([^\"]+)\""));

		int retraces = 0;
		for (const QString &name : svgs) {
			QFile file(dir.filePath(name));
			QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
			const QString content = QString::fromUtf8(file.readAll());
			auto it = path_re.globalMatch(content);
			while (it.hasNext()) {
				const QRegularExpressionMatch m = it.next();
				const QList<Point> points = parsePoints(m.captured(1));
				if (isConductorShaped(points) && isSelfRetracing(points)) {
					++retraces;
				}
			}
		}
		QCOMPARE(retraces, 0);
	}
};

QTEST_APPLESS_MAIN(tst_conductorselfretrace)

#include "tst_conductorselfretrace.moc"
