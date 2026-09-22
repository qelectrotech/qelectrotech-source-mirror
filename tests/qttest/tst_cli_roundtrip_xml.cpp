#include <QtTest>

#include "cli_test_utils.h"

#include <QDomDocument>
#include <QFile>
#include <QTemporaryDir>

namespace {

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

QString canonicalNode(const QDomNode &node)
{
	if (node.isText())
		return node.nodeValue().simplified();

	if (!node.isElement())
		return {};

	const QDomElement element = node.toElement();
	QString out = QStringLiteral("<") + element.tagName();

	QStringList attributes;
	const QDomNamedNodeMap map = element.attributes();
	for (int i = 0; i < map.count(); ++i) {
		const QDomNode attr = map.item(i);
		attributes << attr.nodeName() + QStringLiteral("=") + attr.nodeValue();
	}
	attributes.sort();
	for (const QString &attribute : attributes)
		out += QStringLiteral(" ") + attribute;
	out += QStringLiteral(">");

	for (QDomNode child = element.firstChild(); !child.isNull(); child = child.nextSibling()) {
		const QString canonical_child = canonicalNode(child);
		if (!canonical_child.isEmpty())
			out += canonical_child;
	}

	out += QStringLiteral("</") + element.tagName() + QStringLiteral(">");
	return out;
}

QString canonicalProjectXml(const QString &path)
{
	const QDomDocument doc = loadXml(path);
	if (doc.isNull())
		return {};
	return canonicalNode(doc.documentElement());
}

} // namespace

class tst_cli_roundtrip_xml : public QObject
{
	Q_OBJECT

private slots:
	void resaveIsStableAfterFirstRewrite()
	{
		const QString binary = QStringLiteral(QET_TEST_BINARY_PATH);
		QVERIFY2(QFile::exists(binary),
				 qPrintable(QStringLiteral("qelectrotech binary not found at '%1'").arg(binary)));

		const QString fixture = QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");

		QTemporaryDir out_dir;
		QVERIFY(out_dir.isValid());

		const QString first = out_dir.filePath(QStringLiteral("first.qet"));
		const QString second = out_dir.filePath(QStringLiteral("second.qet"));

		const CliTestUtils::CliResult first_run =
			CliTestUtils::runQetCli({QStringLiteral("--resave"), fixture, first});
		QVERIFY2(first_run.exit_code == 0,
				 qPrintable(QStringLiteral("first --resave failed with exit %1\nstdout: %2\nstderr: %3")
								.arg(first_run.exit_code)
								.arg(first_run.stdout_text.left(500))
								.arg(first_run.stderr_text.left(500))));
		QVERIFY2(QFile::exists(first), "first resave output was not created");

		const CliTestUtils::CliResult second_run =
			CliTestUtils::runQetCli({QStringLiteral("--resave"), first, second});
		QVERIFY2(second_run.exit_code == 0,
				 qPrintable(QStringLiteral("second --resave failed with exit %1\nstdout: %2\nstderr: %3")
								.arg(second_run.exit_code)
								.arg(second_run.stdout_text.left(500))
								.arg(second_run.stderr_text.left(500))));
		QVERIFY2(QFile::exists(second), "second resave output was not created");

		const QDomDocument first_doc = loadXml(first);
		const QDomDocument second_doc = loadXml(second);
		QVERIFY2(!first_doc.isNull(), "first resave output is not parseable XML");
		QVERIFY2(!second_doc.isNull(), "second resave output is not parseable XML");
		QCOMPARE(first_doc.documentElement().tagName(), QStringLiteral("project"));
		QCOMPARE(second_doc.documentElement().tagName(), QStringLiteral("project"));
		QVERIFY2(first_doc.elementsByTagName(QStringLiteral("diagram")).count() > 0,
				 "fixture should contain at least one diagram after resave");

		QCOMPARE(canonicalProjectXml(second), canonicalProjectXml(first));
	}
};

QTEST_APPLESS_MAIN(tst_cli_roundtrip_xml)

#include "tst_cli_roundtrip_xml.moc"
