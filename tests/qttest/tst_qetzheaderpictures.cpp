// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// A folio of a .qetz whose title only changed is saved from its items as
// read, not built (Diagram::deferredXmlWithHeader()). Those items name
// their pictures' files in the .qetz, which the new .qetz left out: the
// project saved then refused to open, 'picture "images/..." is missing'
// (FINDINGS F118). Runs scripts through the real binary's --run on a copy
// of examples/grafcet.qet given a picture on its second folio.
class tst_qetzheaderpictures : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

		// Run @p script on @p project; its output
	QString run(const QString &script, const QString &project, bool every_folio_built)
	{
		const QString path = m_dir.filePath(QStringLiteral("script.js"));
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return QString();
		f.write(script.toUtf8());
		f.close();
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		if (every_folio_built)
			env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		else
			env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), path, project});
		proc.waitForFinished(120000);
		return QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void titleChangeKeepsPictures()
	{
			// A 1x1 PNG
		const QString png = m_dir.filePath(QStringLiteral("dot.png"));
		QFile p(png);
		QVERIFY(p.open(QIODevice::WriteOnly));
		p.write(QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg=="));
		p.close();
		const QString with_picture = m_dir.filePath(QStringLiteral("picture.qetz"));
		const QString titled = m_dir.filePath(QStringLiteral("titled.qetz"));
		QString out = run(QStringLiteral("qet.log('ADDED ' + qet.addImage(1, '%1', 100, 100));"
										 "qet.save('%2');").arg(png, with_picture),
						  QStringLiteral(QET_EXAMPLES_DIR "/grafcet.qet"), true);
		QVERIFY2(out.contains(QStringLiteral("ADDED 0")), qPrintable(out));
		QVERIFY(QFile::exists(with_picture));

			// Only the title of the folio with the picture, folios built
			// when needed
		out = run(QStringLiteral("qet.log('SET ' + qet.setFolioProperty(1, 'title', 'Edited'));"
								 "qet.save('%1');").arg(titled), with_picture, false);
		QVERIFY2(out.contains(QStringLiteral("SET true")), qPrintable(out));
		QVERIFY(QFile::exists(titled));

			// Opens, with its picture and the new title
		out = run(QStringLiteral("qet.log('PICTURES ' + qet.images(1).length);"
								 "qet.log('TITLE ' + qet.folioTitle(1));"), titled, true);
		QVERIFY2(out.contains(QStringLiteral("PICTURES 1")), qPrintable(out));
		QVERIFY2(out.contains(QStringLiteral("TITLE Edited")), qPrintable(out));
	}
};

QTEST_APPLESS_MAIN(tst_qetzheaderpictures)

#include "tst_qetzheaderpictures.moc"
