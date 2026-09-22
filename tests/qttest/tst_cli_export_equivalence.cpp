#include <QtTest>

#include "cli_test_utils.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QTemporaryDir>

namespace {

QSet<QString> sortedWireSet()
{
	return {
		QStringLiteral("W005"),
		QStringLiteral("W006"),
		QStringLiteral("W007"),
		QStringLiteral("W008"),
		QStringLiteral("W009"),
		QStringLiteral("W010"),
		QStringLiteral("W011"),
	};
}

} // namespace

class tst_cli_export_equivalence : public QObject
{
	Q_OBJECT

private slots:
	void wiringNetsCablesWiresBomAndInfoShareCoreFacts()
	{
		const QString binary = QStringLiteral(QET_TEST_BINARY_PATH);
		QVERIFY2(QFile::exists(binary),
				 qPrintable(QStringLiteral("qelectrotech binary not found at '%1'").arg(binary)));

		const QString fixture = QFINDTESTDATA("fixtures/workflow_exports_minimal.qet");
		QVERIFY2(!fixture.isEmpty(), "export equivalence fixture project not found");

		QTemporaryDir out_dir;
		QVERIFY(out_dir.isValid());

		const QString nets_path = out_dir.filePath(QStringLiteral("nets.json"));
		const QString wiring_path = out_dir.filePath(QStringLiteral("wiring.csv"));
		const QString cables_path = out_dir.filePath(QStringLiteral("cables.csv"));
		const QString wires_path = out_dir.filePath(QStringLiteral("wires.csv"));
		const QString bom_path = out_dir.filePath(QStringLiteral("bom.csv"));
		const QString info_path = out_dir.filePath(QStringLiteral("info.json"));

		const QList<QPair<QString, QString>> commands {
			{QStringLiteral("--export-nets"), nets_path},
			{QStringLiteral("--export-wiring"), wiring_path},
			{QStringLiteral("--export-cables"), cables_path},
			{QStringLiteral("--export-wires"), wires_path},
			{QStringLiteral("--export-bom"), bom_path},
			{QStringLiteral("--info"), info_path},
		};

		for (const auto &command : commands) {
			const CliTestUtils::CliResult result =
				CliTestUtils::runQetCli({command.first, fixture, command.second});
			QVERIFY2(result.exit_code == 0,
					 qPrintable(QStringLiteral("%1 failed with exit %2\nstdout: %3\nstderr: %4")
									.arg(command.first)
									.arg(result.exit_code)
									.arg(result.stdout_text.left(500))
									.arg(result.stderr_text.left(500))));
			QVERIFY2(QFile::exists(command.second),
					 qPrintable(QStringLiteral("%1 did not create output").arg(command.first)));
			QVERIFY2(QFileInfo(command.second).size() > 0,
					 qPrintable(QStringLiteral("%1 created an empty output").arg(command.first)));
		}

		const QSet<QString> expected_wires = sortedWireSet();

		const QJsonObject info = CliTestUtils::readJsonObject(info_path);
		QCOMPARE(info.value(QStringLiteral("diagrams")).toInt(), 1);
		QCOMPARE(info.value(QStringLiteral("elements")).toInt(), 9);
		QCOMPARE(info.value(QStringLiteral("conductors")).toInt(), expected_wires.size());

		const QJsonArray nets = CliTestUtils::readJsonObject(nets_path).value(QStringLiteral("list")).toArray();
		QCOMPARE(nets.size(), expected_wires.size());
		QSet<QString> net_wires;
		for (const QJsonValue &value : nets) {
			const QJsonObject net = value.toObject();
			net_wires.insert(net.value(QStringLiteral("wire_no")).toString());
			QCOMPARE(net.value(QStringLiteral("terminals")).toArray().size(), 2);
		}
		QCOMPARE(net_wires, expected_wires);

		const QList<QStringList> wiring_rows =
			CliTestUtils::parseSemicolonCsv(QString::fromUtf8(CliTestUtils::readFile(wiring_path)));
		QCOMPARE(wiring_rows.size() - 1, expected_wires.size());
		QCOMPARE(CliTestUtils::columnSet(wiring_rows, QStringLiteral("wire_number")), expected_wires);

		const QList<QStringList> cables_rows =
			CliTestUtils::parseSemicolonCsv(QString::fromUtf8(CliTestUtils::readFile(cables_path)));
		QCOMPARE(cables_rows.size() - 1, expected_wires.size());
		QCOMPARE(CliTestUtils::columnSet(cables_rows, QStringLiteral("Couleur du fil")), QSet<QString>{QStringLiteral("BK")});
		QCOMPARE(CliTestUtils::columnSet(cables_rows, QStringLiteral("Section du fil")), QSet<QString>{QStringLiteral("1.5")});
		QCOMPARE(CliTestUtils::columnSet(cables_rows, QStringLiteral("Fonction")), QSet<QString>{QStringLiteral("CTRL")});

		const QStringList wire_lines =
			QString::fromUtf8(CliTestUtils::readFile(wires_path)).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
		QCOMPARE(QSet<QString>(wire_lines.cbegin(), wire_lines.cend()), expected_wires);

		const QList<QStringList> bom_rows =
			CliTestUtils::parseSemicolonCsv(QString::fromUtf8(CliTestUtils::readFile(bom_path)));
		QCOMPARE(bom_rows.size() - 1, info.value(QStringLiteral("elements")).toInt());
		QCOMPARE(CliTestUtils::columnSet(bom_rows, QStringLiteral("folio")), QSet<QString>{QStringLiteral("1 OF 1")});
	}
};

QTEST_APPLESS_MAIN(tst_cli_export_equivalence)

#include "tst_cli_export_equivalence.moc"
