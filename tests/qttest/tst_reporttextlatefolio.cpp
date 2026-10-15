// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>

// A report symbol on folio B shows the function of the wire its partner has
// on folio A, right-aligned. Deleting the coil on folio B empties that text
// while folio A, not built yet (lazy folios), is not known to the report, and
// builds it again when folio A is built. The text keeps its right edge when
// emptied outside the opening of the project, and was not moved back when set
// again while the late folios are built (the project is then in the state of
// being opened): it was saved at another x than with every folio built (F119).
// The real binary's --run on fixtures/report_text_late_folio.qet, lazily and
// with QET_EAGER_FOLIOS=1.
class tst_reporttextlatefolio : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QString savedX(bool eager)
	{
		const QString tag = eager ? QStringLiteral("eager") : QStringLiteral("lazy");
		const QString out = m_dir.filePath(tag + QStringLiteral(".qet"));
		const QString script = m_dir.filePath(tag + QStringLiteral(".js"));
		const QString home = m_dir.filePath(tag + QStringLiteral("-home"));
		QDir().mkpath(home);
		QFile f(script);
		if (!f.open(QIODevice::WriteOnly))
			return QString();
		f.write("qet.deleteElement(1, '{794d8944-035d-4bcd-8ee6-c653d384fd4f}');\n"
				"qet.save('" + out.toUtf8() + "');\n");
		f.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		if (eager)
			env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		else
			env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), script,
					QStringLiteral(QET_FIXTURES_DIR "/report_text_late_folio.qet")});
		if (!proc.waitForFinished(120000))
			return QString();
		QFile saved(out);
		if (!saved.open(QIODevice::ReadOnly))
			return QString();
		const QString xml = QString::fromUtf8(saved.readAll());
		const QRegularExpression re(QStringLiteral(
			"uuid=\"\\{cbaf051e-0ce0-40f1-ada9-b8f637a79f92\\}\" x=\"([^\"]*)\""));
		const auto m = re.match(xml);
		return m.hasMatch() ? m.captured(1) : QString();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void rightAlignedTextKeepsItsPlace_data()
	{
		QTest::addColumn<bool>("eager");
		QTest::newRow("lazy folios") << false;
		QTest::newRow("every folio built") << true;
	}

	void rightAlignedTextKeepsItsPlace()
	{
		QFETCH(bool, eager);
		QCOMPARE(savedX(eager), QStringLiteral("60"));
	}
};

QTEST_APPLESS_MAIN(tst_reporttextlatefolio)

#include "tst_reporttextlatefolio.moc"
