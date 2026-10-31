// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include "qettesttimeout.h"

// A folio's pictures are in the project database: the drawing_item rows of
// each (kind image) with a "#blob" row, the SHA-256 of the picture, in
// place of the base64 text, and the bytes once in the blob table, whether
// the folio was built (QET_EAGER_FOLIOS=1) or read from the document (lazy
// folios), from a .qet or a .qetz. QET-SQL-FORMAT S3j.
//
// Runs the real binary (--run). The queries do not name the picture tables
// the drawing tables are built for (a read of those builds the folios): the
// kind is picked here.
namespace {

const QString folio_uuid = QStringLiteral("{99999999-9999-4999-8999-999999999999}");
const QString second_folio_uuid = QStringLiteral("{99999999-9999-4999-8999-999999999998}");
const QString first_uuid = QStringLiteral("{dddddddd-dddd-4ddd-8ddd-ddddddddddd1}");
const QString second_uuid = QStringLiteral("{dddddddd-dddd-4ddd-8ddd-ddddddddddd2}");
const QString third_uuid = QStringLiteral("{dddddddd-dddd-4ddd-8ddd-ddddddddddd3}");

QByteArray png(QRgb color, int width)
{
	QImage image(width, 4, QImage::Format_ARGB32);
	image.fill(color);
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	image.save(&buffer, "PNG");
	return bytes;
}

QString picture(const QString &uuid, const QByteArray &bytes, const QString &x)
{
	return QStringLiteral("<image uuid=\"%1\" x=\"%2\" y=\"10\" z=\"1\" rotation=\"0\" size=\"1\""
						  " is_movable=\"1\">%3</image>")
			.arg(uuid, x, QString::fromLatin1(bytes.toBase64()));
}

QString project(const QByteArray &one, const QByteArray &other)
{
	return QStringLiteral(
				"<project title=\"pictures\" version=\"0.100.1\">"
				"<diagram uuid=\"%1\" title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<images>%3%4</images></diagram>"
				"<diagram uuid=\"%2\" title=\"g\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<images>%5</images></diagram></project>")
			.arg(folio_uuid, second_folio_uuid,
				 picture(first_uuid, one, QStringLiteral("20")),
				 picture(second_uuid, other, QStringLiteral("80")),
				 picture(third_uuid, one, QStringLiteral("30")));
}

} // namespace

class tst_picturerows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// The result of @p sql after opening @p source, a row per line, the
	// columns @p columns joined by "|". @p log: what the binary printed.
	QStringList query(bool eager, const QString &source, const QString &sql,
					  const QStringList &columns, QString *log = nullptr)
	{
		const QString js = m_dir.filePath(QStringLiteral("script%1.js").arg(m_run));
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run));
		++m_run;
		QDir().mkpath(home);
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write(QStringLiteral("qet.log('ROWS ' + JSON.stringify(qet.query('%1')));").arg(sql).toUtf8());
		s.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		if (eager) env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		else env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), js, source});
		if (!proc.waitForFinished(QetTest::timeout(120000))) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		if (log) *log = out;
		QStringList result;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("ROWS "));
			if (i < 0) continue;
			for (const QJsonValue &v : QJsonDocument::fromJson(line.mid(i + 5).toUtf8()).array()) {
				const QJsonObject o = v.toObject();
				QStringList cells;
				for (const QString &name : columns) {
					const QJsonValue c = o.value(name);
					cells << (c.isNull() || c.isUndefined() ? QStringLiteral("NULL")
							: c.isString() ? c.toString() : QString::number(c.toDouble()));
				}
				result << cells.join(QLatin1Char('|'));
			}
		}
		return result;
	}

	// The picture rows (kind image) of drawing_item, item by item
	QStringList pictureRows(bool eager, const QString &source, QString *log = nullptr)
	{
		const QStringList all = query(eager, source,
				QStringLiteral("SELECT kind, item_uuid, folio_uuid, section, ord, name, value "
							   "FROM drawing_item ORDER BY item_uuid, section, ord, name"),
				{QStringLiteral("kind"), QStringLiteral("item_uuid"), QStringLiteral("folio_uuid"),
				 QStringLiteral("section"), QStringLiteral("ord"), QStringLiteral("name"),
				 QStringLiteral("value")}, log);
		QStringList result;
		for (const QString &row : all)
			if (row.startsWith(QStringLiteral("image|"))) result << row;
		return result;
	}

	// The blob table: hash, media type, byte length (and the bytes in hex
	// when @p with_bytes)
	QStringList blobs(bool eager, const QString &source, bool with_bytes = false)
	{
		return query(eager, source,
				QStringLiteral("SELECT hash, media_type, length(bytes) AS n, ")
				+ (with_bytes ? QStringLiteral("hex(bytes)") : QStringLiteral("0"))
				+ QStringLiteral(" AS h FROM blob ORDER BY hash"),
				{QStringLiteral("hash"), QStringLiteral("media_type"), QStringLiteral("n"),
				 QStringLiteral("h")});
	}

	static QString sha(const QByteArray &bytes)
	{
		return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
	}

	QString resave(const QString &example, const QString &name)
	{
		const QString saved = m_dir.filePath(name);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), m_dir.filePath(QStringLiteral("home-resave")));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--resave"),
					QFile::exists(example) ? example : QStringLiteral(QET_EXAMPLES_DIR "/") + example, saved});
		if (!proc.waitForFinished(QetTest::timeout(180000)) || !QFile::exists(saved)) return QString();
		return saved;
	}

	// Opened lazily and with every folio built, the pictures are the same
	// rows and blobs
	void lazyAndBuiltAgree(const QString &example, const QString &suffix, bool filled_from_document)
	{
		const QString first = resave(example, example + QStringLiteral(".first.qet"));
		QVERIFY(!first.isEmpty());
		const QString saved = suffix == QStringLiteral("qet") ? first
				: resave(first, example + QStringLiteral(".") + suffix);
		QVERIFY(!saved.isEmpty());

		QString lazy_log;
		const QStringList lazy = pictureRows(false, saved, &lazy_log);
		const QStringList built = pictureRows(true, saved);
		const QStringList lazy_blobs = blobs(false, saved);
		const QStringList built_blobs = blobs(true, saved);
		if (filled_from_document)
			QVERIFY2(lazy_log.contains(QStringLiteral("Project database filled from the document")),
					 qPrintable(lazy_log.right(2000)));
		qInfo() << example << suffix << "picture rows:" << built.size() << "lazy:" << lazy.size()
				<< "blobs:" << built_blobs.size() << "lazy:" << lazy_blobs.size();
		QVERIFY(!built.isEmpty());
		QVERIFY(!built_blobs.isEmpty());
		QCOMPARE(lazy, built);
		QCOMPARE(lazy_blobs, built_blobs);
		for (const QString &row : built)
			QVERIFY2(!row.contains(QStringLiteral("|#text|")) || !row.contains(QStringLiteral("iVBOR")),
					 qPrintable(row.left(200)));
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// A picture is the rows of what it saves, with a "#blob" row for the
	// SHA-256 of its bytes instead of the base64 text; the bytes are in the
	// blob table, once for the two pictures that are the same
	void pictureIsRowsAndOneBlob()
	{
		const QByteArray one = png(qRgb(200, 30, 30), 6);
		const QByteArray other = png(qRgb(30, 30, 200), 9);
		const QString in = m_dir.filePath(QStringLiteral("pictures.qet"));
		QFile f(in);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(project(one, other).toUtf8());
		f.close();

		for (bool eager : {true, false}) {
			const QStringList r = pictureRows(eager, in);
			QVERIFY2(!r.isEmpty(), qPrintable(QString::number(eager)));
			auto row = [](const QString &uuid, const QString &folio, const QString &name, const QString &value) {
				return QStringLiteral("image|%1|%2|image|0|%3|%4").arg(uuid, folio, name, value);
			};
			QVERIFY(r.contains(row(first_uuid, folio_uuid, QStringLiteral("#blob"), sha(one))));
			QVERIFY(r.contains(row(second_uuid, folio_uuid, QStringLiteral("#blob"), sha(other))));
			QVERIFY(r.contains(row(third_uuid, second_folio_uuid, QStringLiteral("#blob"), sha(one))));
			QVERIFY(r.contains(row(first_uuid, folio_uuid, QStringLiteral("x"), QStringLiteral("20"))));
			QVERIFY(r.contains(row(third_uuid, second_folio_uuid, QStringLiteral("x"), QStringLiteral("30"))));
			for (const QString &line : r)
				QVERIFY2(!line.contains(QStringLiteral("|#text|")), qPrintable(line));
			QCOMPARE(r.size(), 3 * 8);

			const QStringList b = blobs(eager, in, true);
			QStringList expected;
			for (const QByteArray &bytes : {one, other})
				expected << QStringList{sha(bytes), QStringLiteral("image/png"),
										QString::number(bytes.size()),
										QString::fromLatin1(bytes.toHex().toUpper())}.join(QLatin1Char('|'));
			expected.sort();
			QCOMPARE(b, expected);
		}
	}

	void perceuseQetAgrees() { lazyAndBuiltAgree(QStringLiteral("perceuse.qet"), QStringLiteral("qet"), true); }
	void perceuseQetzAgrees() { lazyAndBuiltAgree(QStringLiteral("perceuse.qet"), QStringLiteral("qetz"), false); }
	void polonezQetAgrees() { lazyAndBuiltAgree(QStringLiteral("weneedpolonez-Polonez_MR89_wiring_diagram.qet"), QStringLiteral("qet"), true); }
	void polonezQetzAgrees() { lazyAndBuiltAgree(QStringLiteral("weneedpolonez-Polonez_MR89_wiring_diagram.qet"), QStringLiteral("qetz"), false); }
	void m000QetAgrees() { lazyAndBuiltAgree(QStringLiteral("m_000.qet"), QStringLiteral("qet"), true); }
	void m000QetzAgrees() { lazyAndBuiltAgree(QStringLiteral("m_000.qet"), QStringLiteral("qetz"), false); }
};

QTEST_GUILESS_MAIN(tst_picturerows)
#include "tst_picturerows.moc"
