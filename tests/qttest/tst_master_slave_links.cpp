#include <QtTest>

#include "cli_test_utils.h"

#include <QDomDocument>
#include <QFile>
#include <QTemporaryDir>

namespace {

const QString kMasterUuid = QStringLiteral("{11111111-1111-4111-8111-111111111111}");
const QMap<QString, QString> kExpectedGroupIndices {
	{QStringLiteral("{22222222-2222-4222-8222-222222222222}"), QStringLiteral("0")},
	{QStringLiteral("{33333333-3333-4333-8333-333333333333}"), QStringLiteral("1")},
};

QDomDocument loadXml(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return {};

	QDomDocument doc;
	const QDomDocument::ParseResult result = doc.setContent(&file);
	if (!result) {
		qWarning().noquote()
			<< QStringLiteral("XML parse error in %1:%2:%3: %4")
				  .arg(path)
				  .arg(result.errorLine)
				  .arg(result.errorColumn)
				  .arg(result.errorMessage);
		return {};
	}
	return doc;
}

QMap<QString, QString> masterLinkGroups(const QString &path)
{
	QMap<QString, QString> groups;
	const QDomDocument doc = loadXml(path);
	if (doc.isNull())
		return groups;

	const QDomNodeList elements = doc.elementsByTagName(QStringLiteral("element"));
	for (int i = 0; i < elements.count(); ++i) {
		const QDomElement element = elements.at(i).toElement();
		if (element.attribute(QStringLiteral("uuid")) != kMasterUuid)
			continue;

		const QDomNodeList links = element.elementsByTagName(QStringLiteral("link_uuid"));
		for (int j = 0; j < links.count(); ++j) {
			const QDomElement link = links.at(j).toElement();
			groups.insert(link.attribute(QStringLiteral("uuid")),
						  link.attribute(QStringLiteral("group_index")));
		}
		return groups;
	}

	return groups;
}

bool hasLinkedMasterRow(const QList<QStringList> &rows)
{
	if (rows.isEmpty())
		return false;

	const QStringList header = rows.first();
	const int element_col = header.indexOf(QStringLiteral("element"));
	const int type_col = header.indexOf(QStringLiteral("link_type"));
	const int linked_col = header.indexOf(QStringLiteral("linked_to"));
	const int status_col = header.indexOf(QStringLiteral("status"));
	if (element_col < 0 || type_col < 0 || linked_col < 0 || status_col < 0)
		return false;

	for (int i = 1; i < rows.size(); ++i) {
		const QStringList row = rows.at(i);
		if (row.size() <= qMax(qMax(element_col, type_col), qMax(linked_col, status_col)))
			continue;
		if (row.at(element_col) == QStringLiteral("KMS")
			&& row.at(type_col) == QStringLiteral("Master")
			&& row.at(status_col) == QStringLiteral("linked")
			&& row.at(linked_col).contains(QStringLiteral("KMS-NO(f1)"))
			&& row.at(linked_col).contains(QStringLiteral("KMS-NC(f1)")))
			return true;
	}

	return false;
}

} // namespace

class tst_master_slave_links : public QObject
{
	Q_OBJECT

private slots:
	void resavePreservesGroupIndexedMasterSlaveLinks()
	{
		const QString binary = QStringLiteral(QET_TEST_BINARY_PATH);
		QVERIFY2(QFile::exists(binary),
				 qPrintable(QStringLiteral("qelectrotech binary not found at '%1'").arg(binary)));

		const QString fixture = QFINDTESTDATA("fixtures/master_slave_links_group_index_minimal.qet");
		QVERIFY2(!fixture.isEmpty(), "master/slave link fixture project not found");
		QCOMPARE(masterLinkGroups(fixture), kExpectedGroupIndices);

		QTemporaryDir out_dir;
		QVERIFY(out_dir.isValid());

		const QString resaved_path = out_dir.filePath(QStringLiteral("resaved.qet"));
		const CliTestUtils::CliResult resave =
			CliTestUtils::runQetCli({QStringLiteral("--resave"), fixture, resaved_path});
		QVERIFY2(resave.exit_code == 0,
				 qPrintable(QStringLiteral("--resave failed with exit %1\nstdout: %2\nstderr: %3")
								.arg(resave.exit_code)
								.arg(resave.stdout_text.left(500))
								.arg(resave.stderr_text.left(500))));
		QVERIFY2(QFile::exists(resaved_path), "--resave did not create output");
		QCOMPARE(masterLinkGroups(resaved_path), kExpectedGroupIndices);

		const QString links_path = out_dir.filePath(QStringLiteral("links.csv"));
		const CliTestUtils::CliResult links =
			CliTestUtils::runQetCli({QStringLiteral("--export-links"), resaved_path, links_path});
		QVERIFY2(links.exit_code == 0,
				 qPrintable(QStringLiteral("--export-links failed with exit %1\nstdout: %2\nstderr: %3")
								.arg(links.exit_code)
								.arg(links.stdout_text.left(500))
								.arg(links.stderr_text.left(500))));
		QVERIFY2(QFile::exists(links_path), "--export-links did not create output");

		const QList<QStringList> rows =
			CliTestUtils::parseSemicolonCsv(QString::fromUtf8(CliTestUtils::readFile(links_path)));
		QVERIFY2(hasLinkedMasterRow(rows), "expected linked KMS master row was not found");
		QVERIFY2(!CliTestUtils::columnSet(rows, QStringLiteral("status")).contains(QStringLiteral("UNRESOLVED")),
				 "fixture should not export unresolved master/slave links");
	}
};

QTEST_APPLESS_MAIN(tst_master_slave_links)

#include "tst_master_slave_links.moc"
