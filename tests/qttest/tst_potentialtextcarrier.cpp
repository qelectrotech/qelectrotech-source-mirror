// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>

// With "one text per potential" on, the potential's number is drawn on its
// longest conductor. fixtures/one_text_per_potential_tie.qet has two
// conductors of the same, greatest length in one potential: which of them
// carried the number used to follow pointer order, so the same file
// exported twice could put it on either. Runs the real binary, in separate
// processes, since that order only changes from one run to the next.
class tst_potentialtextcarrier : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	// Export the fixture to SVG and return where the number is drawn: the
	// transform of the group holding it. Empty if it is not drawn exactly
	// once.
	QString carrier(int run)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(run));
		const QString out = m_dir.filePath(QStringLiteral("svg%1").arg(run));
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
					QFINDTESTDATA("fixtures/one_text_per_potential_tie.qet"), out});
		if (!proc.waitForFinished(60000) || proc.exitCode() != 0)
			return {};

		const QStringList svgs = QDir(out).entryList({QStringLiteral("*.svg")});
		if (svgs.size() != 1)
			return {};
		QFile f(QDir(out).filePath(svgs.first()));
		if (!f.open(QIODevice::ReadOnly))
			return {};
		const QString svg = QString::fromUtf8(f.readAll());

		static const QRegularExpression re(
			QStringLiteral("transform=\"([^\"]*)\"[^<]*>\\s*<text[^>]*>42</text>"));
		QStringList found;
		auto it = re.globalMatch(svg);
		while (it.hasNext())
			found << it.next().captured(1);
		return found.size() == 1 ? found.first() : QString();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// Before the fix about half the runs differed from the first, so
	// eight agreeing runs leave a 1 in 128 chance of a false pass.
	void sameCarrierEveryRun()
	{
		const QString first = carrier(0);
		QVERIFY2(!first.isEmpty(), "the number is not drawn exactly once");
		for (int run = 1; run < 8; ++run)
			QCOMPARE(carrier(run), first);
	}
};

QTEST_GUILESS_MAIN(tst_potentialtextcarrier)
#include "tst_potentialtextcarrier.moc"
