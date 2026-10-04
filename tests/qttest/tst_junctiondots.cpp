// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>

// fixtures/junction_dot_shared_potential.qet: four contacts K1..K4 in a row,
// wired K1-K2, K1-K4 and K2-K3. All three wires run along one line above the
// contacts, so there is a T above K2 and another above K3. The corner above
// K3 belongs to K2-K3 and lies on K1-K4, which shares no terminal with it:
// that dot used to be missing (issue #1280).
class tst_junctiondots : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	// Export the fixture to SVG and return the distinct positions of the
	// junction dots drawn on it.
	QSet<QString> dots()
	{
		const QString home = m_dir.filePath(QStringLiteral("home"));
		const QString out = m_dir.filePath(QStringLiteral("svg"));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), home);
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--export-svg"),
					QFINDTESTDATA("fixtures/junction_dot_shared_potential.qet"), out});
		if (!proc.waitForFinished(60000) || proc.exitCode() != 0)
			return {};

		const QStringList svgs = QDir(out).entryList({QStringLiteral("*.svg")});
		if (svgs.size() != 1)
			return {};
		QFile f(QDir(out).filePath(svgs.first()));
		if (!f.open(QIODevice::ReadOnly))
			return {};
		const QString svg = QString::fromUtf8(f.readAll());

			// A junction dot on a wire of the default width is a circle
			// 3.0 across; the symbols in the fixture draw none that size.
		static const QRegularExpression re(
			QStringLiteral("<circle cx=\"([^\"]*)\" cy=\"([^\"]*)\" r=\"1.5\"/>"));
		QSet<QString> found;
		auto it = re.globalMatch(svg);
		while (it.hasNext()) {
			const auto m = it.next();
			found << m.captured(1) + QLatin1Char(',') + m.captured(2);
		}
		return found;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void dotAtEveryT()
	{
		const QSet<QString> expected{QStringLiteral("200,260"),
									 QStringLiteral("300,260")};
		QCOMPARE(dots(), expected);
	}
};

QTEST_GUILESS_MAIN(tst_junctiondots)
#include "tst_junctiondots.moc"
