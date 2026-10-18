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
#include <QtTest>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>

/**
	Deleting a symbol whose terminal held several wires joins the wires
	left behind with a new one. That wire got a random uuid, so the same
	deletion saved a different file on every run (F115). Its uuid now
	comes from its two ends, as a wire saved without a uuid gets on load.

	Runs the real binary's --run on examples/affuteuse_250h.qet (wires
	saved without uuids): the coil KA1 on folio 3 deleted, twice, in two
	separate runs. Both saves must be identical, and no wire may carry a
	random (version 4) uuid.
*/
class tst_joinedwireuuid : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QByteArray deleteAndSave(const QString &name)
	{
		const QString out = m_dir.filePath(name + QStringLiteral(".qet"));
		const QString js = m_dir.filePath(name + QStringLiteral(".js"));
		QFile script(js);
		if (!script.open(QIODevice::WriteOnly)) return {};
		script.write(QStringLiteral(
			"qet.log('DELETED ' + qet.deleteElement(3, '{d8ba5ae4-c76f-4936-bb05-f719eec2be81}'));\n"
			"qet.save('%1');\n").arg(out).toUtf8());
		script.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
			//Its own settings and single-instance socket: another test's
			//QElectroTech running at the same time would otherwise be
			//handed this run (SingleApplication)
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.setProcessChannelMode(QProcess::MergedChannels);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), js,
					QStringLiteral(QET_EXAMPLES_DIR "/affuteuse_250h.qet")});
		if (!proc.waitForFinished(180000) || proc.exitCode() != 0
				|| !proc.readAll().contains("DELETED true"))
			return {};
		QFile saved(out);
		return saved.open(QIODevice::ReadOnly) ? saved.readAll() : QByteArray();
	}

private slots:
	void initTestCase()
	{
		QVERIFY2(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)), "qelectrotech binary not found");
		QVERIFY(m_dir.isValid());
	}

	void sameDeletionSavesTheSameFile()
	{
		const QByteArray first = deleteAndSave(QStringLiteral("first"));
		const QByteArray second = deleteAndSave(QStringLiteral("second"));
		QVERIFY2(!first.isEmpty() && !second.isEmpty(), "the coil was not deleted and saved");

			//No wire with a random uuid: every one is derived
		const QRegularExpression wire(QStringLiteral(
			"<conductor [^>]*\\buuid=\"\\{[0-9a-f]{8}-[0-9a-f]{4}-([0-9a-f])"));
		int wires = 0, random = 0;
		for (auto it = wire.globalMatch(QString::fromUtf8(first)); it.hasNext(); ++wires)
			if (it.next().captured(1) == QLatin1String("4")) ++random;
		QVERIFY(wires > 200);
		QCOMPARE(random, 0);

		QVERIFY(first == second);
	}
};

QTEST_GUILESS_MAIN(tst_joinedwireuuid)
#include "tst_joinedwireuuid.moc"
