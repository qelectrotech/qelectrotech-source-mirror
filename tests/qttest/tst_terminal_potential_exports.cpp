#include <QtTest>

#include "cli_test_utils.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QTemporaryDir>

namespace {

QString terminalKey(const QJsonObject &terminal)
{
	return QStringLiteral("%1:%2:%3")
		.arg(terminal.value(QStringLiteral("folio")).toInt())
		.arg(terminal.value(QStringLiteral("element")).toString())
		.arg(terminal.value(QStringLiteral("terminal")).toString());
}

QSet<QString> terminalSet(const QJsonArray &terminals)
{
	QSet<QString> set;
	for (const QJsonValue &value : terminals)
		set.insert(terminalKey(value.toObject()));
	return set;
}

QMap<QString, QSet<QString>> expectedNets()
{
	return {
		{QStringLiteral("W005"), {
			QStringLiteral("1:v2_transformer_2w_primary-delta_secondary-wye_g:bottom"),
			QStringLiteral("1:v2_circuit_breaker:top"),
		}},
		{QStringLiteral("W006"), {
			QStringLiteral("1:v2_circuit_breaker:bottom"),
			QStringLiteral("1:v2_busstub_n:tap"),
		}},
		{QStringLiteral("W007"), {
			QStringLiteral("1:v2_fuse:bottom"),
			QStringLiteral("1:v2_tee_l:t"),
		}},
		{QStringLiteral("W008"), {
			QStringLiteral("1:v2_tee_l:l"),
			QStringLiteral("1:v2_box_h-8_text-UV_w-10_tape:top"),
		}},
		{QStringLiteral("W009"), {
			QStringLiteral("1:v2_tee_l:b"),
			QStringLiteral("1:v2_overload:top"),
		}},
		{QStringLiteral("W010"), {
			QStringLiteral("1:v2_overload:bottom"),
			QStringLiteral("1:v2_no_contact:top"),
		}},
		{QStringLiteral("W011"), {
			QStringLiteral("1:v2_no_contact:bottom"),
			QStringLiteral("1:v2_busstub_n:tap"),
		}},
	};
}

QMap<QString, QSet<QString>> expectedWiringTerminals()
{
	return {
		{QStringLiteral("W005"), {QStringLiteral("bottom"), QStringLiteral("top")}},
		{QStringLiteral("W006"), {QStringLiteral("bottom"), QStringLiteral("tap")}},
		{QStringLiteral("W007"), {QStringLiteral("bottom"), QStringLiteral("t")}},
		{QStringLiteral("W008"), {QStringLiteral("l"), QStringLiteral("top")}},
		{QStringLiteral("W009"), {QStringLiteral("b"), QStringLiteral("top")}},
		{QStringLiteral("W010"), {QStringLiteral("bottom"), QStringLiteral("top")}},
		{QStringLiteral("W011"), {QStringLiteral("bottom"), QStringLiteral("tap")}},
	};
}

} // namespace

class tst_terminal_potential_exports : public QObject
{
	Q_OBJECT

private slots:
	void terminalReportsIsolatingAndWireExportsMatch()
	{
		const QString binary = QStringLiteral(QET_TEST_BINARY_PATH);
		QVERIFY2(QFile::exists(binary),
				 qPrintable(QStringLiteral("qelectrotech binary not found at '%1'").arg(binary)));

		const QString fixture = QFINDTESTDATA("fixtures/workflow_exports_minimal.qet");
		QVERIFY2(!fixture.isEmpty(), "terminal/potential fixture project not found");

		QTemporaryDir out_dir;
		QVERIFY(out_dir.isValid());

		const QString nets_path = out_dir.filePath(QStringLiteral("nets.json"));
		const QString wiring_path = out_dir.filePath(QStringLiteral("wiring.csv"));
		const QString wires_path = out_dir.filePath(QStringLiteral("wires.csv"));
		const QString info_path = out_dir.filePath(QStringLiteral("info.json"));

		const QList<QPair<QString, QString>> commands {
			{QStringLiteral("--export-nets"), nets_path},
			{QStringLiteral("--export-wiring"), wiring_path},
			{QStringLiteral("--export-wires"), wires_path},
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

		const QMap<QString, QSet<QString>> expected_nets = expectedNets();
		const QSet<QString> expected_wires(expected_nets.keyBegin(), expected_nets.keyEnd());

		const QJsonObject info = CliTestUtils::readJsonObject(info_path);
		QCOMPARE(info.value(QStringLiteral("diagrams")).toInt(), 1);
		QCOMPARE(info.value(QStringLiteral("elements")).toInt(), 9);
		QCOMPARE(info.value(QStringLiteral("conductors")).toInt(), expected_wires.size());

		const QJsonArray nets = CliTestUtils::readJsonObject(nets_path).value(QStringLiteral("list")).toArray();
		QCOMPARE(nets.size(), expected_wires.size());
		for (const QJsonValue &value : nets) {
			const QJsonObject net = value.toObject();
			const QString wire = net.value(QStringLiteral("wire_no")).toString();
			QVERIFY2(expected_nets.contains(wire),
					 qPrintable(QStringLiteral("unexpected net wire number '%1'").arg(wire)));
			QCOMPARE(terminalSet(net.value(QStringLiteral("terminals")).toArray()), expected_nets.value(wire));
		}

		const QList<QStringList> wiring_rows =
			CliTestUtils::parseSemicolonCsv(QString::fromUtf8(CliTestUtils::readFile(wiring_path)));
		QCOMPARE(wiring_rows.size() - 1, expected_wires.size());
		QCOMPARE(CliTestUtils::columnSet(wiring_rows, QStringLiteral("wire_number")), expected_wires);

		const int wire_index = wiring_rows.first().indexOf(QStringLiteral("wire_number"));
		const int from_terminal_index = wiring_rows.first().indexOf(QStringLiteral("from_terminal"));
		const int to_terminal_index = wiring_rows.first().indexOf(QStringLiteral("to_terminal"));
		QVERIFY(wire_index >= 0);
		QVERIFY(from_terminal_index >= 0);
		QVERIFY(to_terminal_index >= 0);

		const QMap<QString, QSet<QString>> expected_wiring = expectedWiringTerminals();
		for (int i = 1; i < wiring_rows.size(); ++i) {
			const QStringList row = wiring_rows.at(i);
			const QString wire = row.at(wire_index);
			QVERIFY2(expected_wiring.contains(wire),
					 qPrintable(QStringLiteral("unexpected wiring wire number '%1'").arg(wire)));
			const QSet<QString> actual_terminals {
				row.at(from_terminal_index),
				row.at(to_terminal_index),
			};
			QCOMPARE(actual_terminals, expected_wiring.value(wire));
		}

		const QStringList wire_lines =
			QString::fromUtf8(CliTestUtils::readFile(wires_path)).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
		QCOMPARE(QSet<QString>(wire_lines.cbegin(), wire_lines.cend()), expected_wires);
	}
};

QTEST_APPLESS_MAIN(tst_terminal_potential_exports)

#include "tst_terminal_potential_exports.moc"
