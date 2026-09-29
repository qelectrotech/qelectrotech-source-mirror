/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "ui/aiassistantsetup.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

using AiAssistantSetup::Client;

class TestAiAssistantSetup : public QObject
{
	Q_OBJECT

	static void touch(const QString &path)
	{
		QDir().mkpath(QFileInfo(path).absolutePath());
		QFile f(path);
		QVERIFY(f.open(QIODevice::WriteOnly));
	}

	static AiAssistantSetup::Paths windowsPaths()
	{
		AiAssistantSetup::Paths p;
		p.server = QStringLiteral("C:\\Program Files\\QElectroTech\\mcp\\qet_mcp.py");
		p.python = QStringLiteral("C:\\Program Files\\QElectroTech\\mcp\\python\\python.exe");
		p.qet_binary = QStringLiteral("C:\\Program Files\\QElectroTech\\bin\\QElectroTech.exe");
		return p;
	}

	static QJsonObject entry(Client client, const AiAssistantSetup::Paths &p, bool edit)
	{
		const QByteArray text = AiAssistantSetup::configuration(
			client, p, QStringLiteral("D:\\Drawings"), edit).toUtf8();
		QJsonParseError error;
		const QJsonDocument doc = QJsonDocument::fromJson(text, &error);
		if (error.error != QJsonParseError::NoError)
			qWarning() << error.errorString() << text;
		const QString key = client == Client::VsCode ? QStringLiteral("servers")
							      : QStringLiteral("mcpServers");
		return doc.object().value(key).toObject().value(QStringLiteral("qet")).toObject();
	}

	private slots:
		void windowsInstallIsFound()
		{
			QTemporaryDir root;
			touch(root.filePath(QStringLiteral("bin/QElectroTech.exe")));
			touch(root.filePath(QStringLiteral("mcp/qet_mcp.py")));
			touch(root.filePath(QStringLiteral("mcp/python/python.exe")));
			const auto p = AiAssistantSetup::detect(root.filePath(QStringLiteral("bin")),
				root.filePath(QStringLiteral("bin/QElectroTech.exe")), true);
			QCOMPARE(p.server, QDir::cleanPath(root.filePath(QStringLiteral("mcp/qet_mcp.py"))));
			QCOMPARE(p.python, QDir::cleanPath(root.filePath(QStringLiteral("mcp/python/python.exe"))));
			QCOMPARE(p.qet_binary, QDir::cleanPath(root.filePath(QStringLiteral("bin/QElectroTech.exe"))));
		}

		void windowsWithoutBundledPythonUsesPath()
		{
			QTemporaryDir root;
			touch(root.filePath(QStringLiteral("mcp/qet_mcp.py")));
			QDir().mkpath(root.filePath(QStringLiteral("bin")));
			const auto p = AiAssistantSetup::detect(root.filePath(QStringLiteral("bin")),
				root.filePath(QStringLiteral("bin/QElectroTech.exe")), true);
			QCOMPARE(p.python, QStringLiteral("python"));
		}

		void unixInstallIsFound()
		{
			QTemporaryDir prefix;
			touch(prefix.filePath(QStringLiteral("share/qelectrotech/mcp/qet_mcp.py")));
			// A bundled-Python layout is only a Windows thing.
			touch(prefix.filePath(QStringLiteral("mcp/python/python.exe")));
			QDir().mkpath(prefix.filePath(QStringLiteral("bin")));
			const auto p = AiAssistantSetup::detect(prefix.filePath(QStringLiteral("bin")),
				prefix.filePath(QStringLiteral("bin/qelectrotech")), false);
			QCOMPARE(p.server, QDir::cleanPath(prefix.filePath(QStringLiteral("share/qelectrotech/mcp/qet_mcp.py"))));
			QCOMPARE(p.python, QStringLiteral("python3"));
		}

		void bundledPythonIsFoundWithoutLookingOnPath()
		{
			QTemporaryDir root;
			touch(root.filePath(QStringLiteral("mcp/qet_mcp.py")));
			touch(root.filePath(QStringLiteral("mcp/python/python.exe")));
			QDir().mkpath(root.filePath(QStringLiteral("bin")));
			bool looked = false;
			const auto p = AiAssistantSetup::detect(root.filePath(QStringLiteral("bin")),
				root.filePath(QStringLiteral("bin/QElectroTech.exe")), true,
				[&looked](const QString &) { looked = true; return QString(); });
			QCOMPARE(p.python_status, AiAssistantSetup::PythonStatus::Found);
			QVERIFY(!looked);
		}

		void pythonStatusFromPath_data()
		{
			QTest::addColumn<bool>("windows");
			QTest::addColumn<QString>("found");
			QTest::addColumn<int>("status");
			using S = AiAssistantSetup::PythonStatus;
			QTest::newRow("windows, none") << true << QString() << int(S::Missing);
			QTest::newRow("windows, python.org") << true
				<< QStringLiteral("C:/Users/me/AppData/Local/Programs/Python/Python314/python.exe") << int(S::Found);
			QTest::newRow("windows, Store shortcut") << true
				<< QStringLiteral("C:/Users/me/AppData/Local/Microsoft/WindowsApps/python.exe") << int(S::StoreShortcut);
			QTest::newRow("windows, Store shortcut, backslashes") << true
				<< QStringLiteral("C:\\Users\\me\\AppData\\Local\\Microsoft\\WINDOWSAPPS\\python.exe") << int(S::StoreShortcut);
			QTest::newRow("linux, none") << false << QString() << int(S::Missing);
			QTest::newRow("linux, python3") << false << QStringLiteral("/usr/bin/python3") << int(S::Found);
			QTest::newRow("linux, a folder named WindowsApps") << false
				<< QStringLiteral("/opt/WindowsApps/python3") << int(S::Found);
		}

		void pythonStatusFromPath()
		{
			QFETCH(bool, windows);
			QFETCH(QString, found);
			QFETCH(int, status);
			QTemporaryDir build;
			QString asked;
			const auto p = AiAssistantSetup::detect(build.path(),
				build.filePath(QStringLiteral("qelectrotech")), windows,
				[&](const QString &name) { asked = name; return found; });
			QCOMPARE(int(p.python_status), status);
			QCOMPARE(asked, windows ? QStringLiteral("python") : QStringLiteral("python3"));
		}

		void noServerIsReportedAsEmpty()
		{
			QTemporaryDir build;
			const auto p = AiAssistantSetup::detect(build.path(),
				build.filePath(QStringLiteral("qelectrotech")), false);
			QVERIFY(p.server.isEmpty());
		}

		void everyJsonClientGetsValidJson()
		{
			for (Client c : AiAssistantSetup::clients()) {
				if (c == Client::CodexCli)
					continue;
				const QJsonObject e = entry(c, windowsPaths(), false);
				QVERIFY2(!e.isEmpty(), qPrintable(QString::number(int(c))));
				// Backslashes survive: parsing the text gives the path back.
				QCOMPARE(e.value(QStringLiteral("args")).toArray().at(0).toString(), windowsPaths().server);
				QCOMPARE(e.value(QStringLiteral("command")).toString(), windowsPaths().python);
				const QJsonObject env = e.value(QStringLiteral("env")).toObject();
				QCOMPARE(env.value(QStringLiteral("QET_MCP_WORKSPACE")).toString(), QStringLiteral("D:\\Drawings"));
				QCOMPARE(env.value(QStringLiteral("QET_BINARY")).toString(), windowsPaths().qet_binary);
				const bool typed = c == Client::VsCode || c == Client::Cursor;
				QCOMPARE(e.contains(QStringLiteral("type")), typed);
			}
		}

		void editingIsOffUnlessAllowed()
		{
			for (Client c : AiAssistantSetup::clients()) {
				const QString off = AiAssistantSetup::configuration(c, windowsPaths(), QStringLiteral("D:\\x"), false);
				const QString on = AiAssistantSetup::configuration(c, windowsPaths(), QStringLiteral("D:\\x"), true);
				QVERIFY(!off.contains(QStringLiteral("QET_ENABLE_SCRIPTING")));
				QVERIFY(on.contains(QStringLiteral("QET_ENABLE_SCRIPTING")));
			}
		}

		void codexGetsEscapedToml()
		{
			const QString t = AiAssistantSetup::configuration(
				Client::CodexCli, windowsPaths(), QStringLiteral("D:\\Drawings"), true);
			QVERIFY(t.startsWith(QStringLiteral("[mcp_servers.qet]\n")));
			QVERIFY(t.contains(QStringLiteral("args = [\"C:\\\\Program Files\\\\QElectroTech\\\\mcp\\\\qet_mcp.py\"]")));
			QVERIFY(t.contains(QStringLiteral("[mcp_servers.qet.env]\n")));
			QVERIFY(t.contains(QStringLiteral("QET_MCP_WORKSPACE = \"D:\\\\Drawings\"")));
			QVERIFY(t.contains(QStringLiteral("QET_ENABLE_SCRIPTING = \"1\"")));
		}
};

QTEST_GUILESS_MAIN(TestAiAssistantSetup)
#include "tst_aiassistantsetup.moc"
