// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>

#include "container/qetzip.h"

#include <sqlite3.h>

// With lazy folios (the default) a folio that was not built is saved as it
// was read, unless something marked it stale: a list kept by hand in
// QETProject (markKeptFoliosStale() and its callers, setProjectProperties(),
// the conditions in toXml()). An edit that list misses loses data without a
// word (discussion #1440). So each edit is made twice on the same project,
// once lazily and once with QET_EAGER_FOLIOS=1 (every folio built, nothing
// kept), and the two saved files must be the same but for what a save says of
// itself (saved date and path).
// The real binary's --run on two examples, resaved with every folio built as
// a .qet and as a .qetz first, so both kept paths are tried: a .qet's folio
// text and a .qetz's folio parts.
//
// Edits left out, each a difference between the two saves that is not the
// lazy one losing data:
//  - folio-remove, folio-remove-0: the save with every folio built is the
//    wrong one, it keeps links and labels that point to the folio (F113);
//  - folio-label: the label follows the new folio name, and right-aligned
//    text moves (F120);
//  - folio-insert-0, folio-insert-mid: labels, wire numbers and report texts
//    made from the folio number or the total are made again when every folio
//    is built and not when it is kept; the lazy save is the right one (F122);
//  - a table whose chain head sits on a removed folio (F121).
class tst_lazysaveparity : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;
	// Each example saved with every folio built, by name and suffix
	QHash<QString, QString> m_sources;

	// What the --run scripts have in common: the middle folio, and finding
	// the same item on it whichever folios are built (lists are sorted:
	// the order of a folio's items differs with its neighbours built)
	static QByteArray prelude()
	{
		return R"JS(
var n = qet.folioCount();
var f = Math.floor(n / 2);
function sorted(list) { return list.slice().sort(); }
function symbol(kinds) {
	for (var k = 0 ; k < n ; ++k) {
		var g = (Math.floor(n / 2) + k) % n;
		var u = sorted(qet.elementUuids(g));
		for (var i = 0 ; i < u.length ; ++i)
			if (kinds.indexOf(qet.elementLinkType(g, u[i])) >= 0 && qet.linkedElements(g, u[i]).length) {
				f = g;
				return u[i];
			}
	}
	return '';
}
function autoNumName(entry) { return entry.substring(0, entry.lastIndexOf(": formula=")); }
function first() { var u = sorted(qet.elementUuids(f)); return u.length ? u[0] : ''; }
var added = -1;
function wire() { var u = sorted(qet.conductorUuids(f)); return u.length ? u[0] : ''; }
)JS";
	}

	// QElectroTech run in a sandbox of its own, with @p args: its own HOME
	// and TMPDIR, as SingleApplication takes two launches of the same binary
	// for one application and hands the second to the first. Returns the
	// output; @p ok tells it finished.
	QString runQet(const QStringList &args, bool eager, bool *ok = nullptr)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run));
		const QString tmp = m_dir.filePath(QStringLiteral("t%1").arg(m_run++));
		QDir().mkpath(home);
		QDir().mkpath(tmp);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("XDG_CACHE_HOME"), home + QStringLiteral("/cache"));
		env.insert(QStringLiteral("TMPDIR"), tmp);
		env.remove(QStringLiteral("QET_LAZY_FOLIOS"));
		if (eager)
			env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		else
			env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), args);
		const bool done = proc.waitForFinished(180000);
		if (ok) *ok = done && proc.exitStatus() == QProcess::NormalExit;
		return QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
	}

	static QByteArray read(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

	static const QString zero()
	{
		return QStringLiteral("00000000-0000-0000-0000-000000000000");
	}

	// The lines of @p text without what a save says of itself, and with
	// <x></x> as <x/> (F114: the same document written two ways); @p mask,
	// a folio's random uuid, as zeros
	static QStringList lines(QString text, const QString &mask)
	{
		if (!mask.isEmpty())
			text.replace(mask, zero());
		static const QRegularExpression empty(QStringLiteral("<([\\w-]+)([^<>]*)>\\s*</\\1>"));
		text.replace(empty, QStringLiteral("<\\1\\2/>"));
		QStringList out;
		const QStringList all = text.split(QLatin1Char('\n'));
		for (const QString &l : all)
			if (!l.contains(QLatin1String("name=\"saved")))
				out << l;
		return out;
	}

	// Every row of every table of the SQLite file @p data, one line each,
	// but the rows that say when the file was saved
	static QStringList databaseRows(const QByteArray &data, const QString &mask)
	{
		QStringList out;
		sqlite3 *db = nullptr;
		sqlite3_open(":memory:", &db);
		unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(data.size())));
		memcpy(copy, data.constData(), size_t(data.size()));
		sqlite3_deserialize(db, "main", copy, data.size(), data.size(), SQLITE_DESERIALIZE_FREEONCLOSE);
		auto text = [](sqlite3_stmt *st, int col) {
			const unsigned char *t = sqlite3_column_text(st, col);
			return t ? QString::fromUtf8(reinterpret_cast<const char *>(t)) : QStringLiteral("NULL");
		};
		QStringList tables;
		sqlite3_stmt *list = nullptr;
		sqlite3_prepare_v2(db, "SELECT name, sql FROM sqlite_master WHERE type = 'table' ORDER BY name", -1, &list, nullptr);
		while (list && sqlite3_step(list) == SQLITE_ROW) {
			tables << text(list, 0);
			out << text(list, 1);
		}
		sqlite3_finalize(list);
		for (const QString &table : std::as_const(tables)) {
			sqlite3_stmt *st = nullptr;
			const QByteArray sql = "SELECT * FROM \"" + table.toUtf8() + "\" ORDER BY rowid";
			if (sqlite3_prepare_v2(db, sql.constData(), -1, &st, nullptr) != SQLITE_OK) {
				sqlite3_finalize(st);
				continue;
			}
			while (sqlite3_step(st) == SQLITE_ROW) {
				QStringList row{table};
				bool saved = false;
				for (int c = 0 ; c < sqlite3_column_count(st) ; ++c) {
					row << text(st, c);
					saved = saved || row.last().startsWith(QLatin1String("saved"));
				}
				if (!saved) {
					QString line = row.join(QLatin1Char('|'));
					if (!mask.isEmpty())
						line.replace(mask, zero());
					out << line;
				}
			}
			sqlite3_finalize(st);
		}
		sqlite3_close(db);
		return out;
	}

	// What the saved file @p path holds but what a save says of itself, as
	// lines: a .qet's text; a .qetz's entries, XML as text, the database as
	// its rows, anything else (pictures) as a hash
	static QStringList contents(const QString &path, const QString &mask, bool *ok)
	{
		const QByteArray bytes = read(path);
		*ok = !bytes.isEmpty();
		if (!path.endsWith(QLatin1String(".qetz")))
			return lines(QString::fromUtf8(bytes), mask);
		QList<QetZip::Entry> entries;
		if (!QetZip::fromBytes(bytes, &entries)) {
			*ok = false;
			return {};
		}
			// By name: the order of the entries is not part of the project
		QMap<QString, QStringList> byName;
		for (const QetZip::Entry &e : std::as_const(entries)) {
			QString name = e.name;
			if (!mask.isEmpty())
				name.replace(mask, zero());
			if (e.name.endsWith(QLatin1String(".sqlite")))
				byName[name] = databaseRows(e.data, mask);
			else if (e.name.endsWith(QLatin1String(".xml")))
				byName[name] = lines(QString::fromUtf8(e.data), mask);
			else
				byName[name] = {QString::fromLatin1(QCryptographicHash::hash(e.data, QCryptographicHash::Sha1).toHex())};
		}
		QStringList out;
		for (auto it = byName.cbegin() ; it != byName.cend() ; ++it)
			out << QStringLiteral("#### ") + it.key() << it.value();
		return out;
	}

	// The first line where @p a and @p b differ, to say in a failure
	static QString firstDifference(const QStringList &a, const QStringList &b)
	{
		for (int i = 0 ; i < qMax(a.size(), b.size()) ; ++i) {
			const QString x = i < a.size() ? a.at(i) : QStringLiteral("<end>");
			const QString y = i < b.size() ? b.at(i) : QStringLiteral("<end>");
			if (x != y) {
				QString where;
				for (int k = i ; k >= 0 && k < a.size() ; --k)
					if (a.at(k).startsWith(QLatin1String("#### "))) {
						where = a.at(k);
						break;
					}
				return QStringLiteral("line %1 %2: lazy [%3] eager [%4]")
					.arg(i).arg(where, x.left(300), y.left(300));
			}
		}
		return QString();
	}

	// The edit @p body (a function body returning true when it applied) made
	// on @p source and saved as @p out; false when it did not apply or
	// nothing was saved. @p mask gets the uuid of a folio the edit adds.
	bool edit(const QString &source, const QString &body, const QString &out, bool eager, QString *mask)
	{
		const QString script = out + QStringLiteral(".js");
		QFile f(script);
		if (!f.open(QIODevice::WriteOnly))
			return false;
		f.write(prelude());
		f.write("var ok = (function() { " + body.toUtf8() + " })();\n"
				"qet.log('EDITDIFF ' + ok);\n"
				"if (ok && added >= 0) qet.log('EDITDIFF-UUID ' + qet.folioUuid(added));\n"
				"if (ok) qet.save('" + out.toUtf8() + "');\n");
		f.close();
		bool ran = false;
		const QString log = runQet({QStringLiteral("--run"), script, source}, eager, &ran);
		const auto m = QRegularExpression(QStringLiteral("EDITDIFF-UUID \\{([0-9a-f-]+)\\}")).match(log);
		*mask = m.hasMatch() ? m.captured(1) : QString();
		return ran && log.contains(QLatin1String("EDITDIFF true")) && QFileInfo(out).size() > 0;
	}

	struct Op { const char *name; const char *body; };
	// Each runs on the middle folio (stages 1a and 1b of
	// LAZY-SAVE-PARITY-SCOPE.md tried more; these are ones that save the same
	// on both examples, one of each kind, to keep the run short: every row
	// opens the project twice, 3 to 4 s each on industrial.qet)
	static QList<Op> ops(bool industrial)
	{
		QList<Op> list{
			{"folio-title", "return n > 1 && qet.setFolioProperty(f, 'title', 'Edited title');"},
			{"master-move", "var u = symbol(['master']); return !!u && qet.moveElement(f, u, 200, 0);"},
			{"master-unlink", "var u = symbol(['master']); return !!u && qet.unlinkElement(f, u);"},
			{"slave-delete", "var u = symbol(['slave']); return !!u && qet.deleteElement(f, u);"},
			{"report-delete", "var u = symbol(['next_report', 'previous_report']); return !!u && qet.deleteElement(f, u);"},
			{"symbol-comment", "var u = first(); return !!u && qet.setElementInfo(f, u, 'comment', 'edited');"},
			{"wire-function", "var c = wire(); return !!c && qet.setConductorPropertyByUuid(f, c, 'function', 'edited');"},
			{"folio-add", "added = qet.addFolio(); return added >= 0;"},
			{"project-title", "return qet.setProjectTitle('Edited project title');"},
		};
		if (industrial) {
			// master-delete is unstable on affuteuse_250h, and it has no
			// automatic numbering to renumber
			list << Op{"master-delete", "var u = symbol(['master']); return !!u && qet.deleteElement(f, u);"};
			list << Op{"renumber", "var a = qet.autoNums('element'); return a.length > 0 && qet.renumberElementAutoNum(autoNumName(a[0])) >= 0;"};
		} else {
			// The labels and wire numbers of industrial.qet come from
			// formulas, which write over a label or number set by hand: the
			// edit changes nothing there (the guard in saveIsTheSame())
			list << Op{"master-label", "var u = symbol(['master']); return !!u && qet.setElementLabel(f, u, 'KX99');"};
			list << Op{"symbol-label", "var u = first(); return !!u && qet.setElementLabel(f, u, 'QX1');"};
			list << Op{"wire-num", "var c = wire(); return !!c && qet.setConductorPropertyByUuid(f, c, 'num', 'W99');"};
		}
		return list;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		for (const char *name : {"industrial", "affuteuse_250h"})
			for (const char *suffix : {".qet", ".qetz"}) {
				const QString key = QLatin1String(name) + QLatin1String(suffix);
				const QString dir = m_dir.filePath(QStringLiteral("src-") + key);
				QDir().mkpath(dir);
				const QString out = dir + QStringLiteral("/") + key;
				runQet({QStringLiteral("--resave"),
						QStringLiteral(QET_EXAMPLES_DIR "/") + QLatin1String(name) + QStringLiteral(".qet"), out},
					   true);
				QVERIFY2(QFileInfo(out).size() > 0, qPrintable(QStringLiteral("cannot resave ") + key));
				m_sources.insert(key, out);
			}
	}

	void saveIsTheSame_data()
	{
		QTest::addColumn<QString>("source");
		QTest::addColumn<QString>("op");
		QTest::addColumn<QString>("body");
		for (const char *name : {"industrial", "affuteuse_250h"})
			for (const char *suffix : {".qet", ".qetz"})
				for (const Op &op : ops(QLatin1String(name) == QLatin1String("industrial")))
					QTest::newRow(qPrintable(QLatin1String(name) + QLatin1String(suffix) + QLatin1Char(' ') + QLatin1String(op.name)))
						<< QLatin1String(name) + QLatin1String(suffix)
						<< QString::fromLatin1(op.name) << QString::fromLatin1(op.body);
	}

	void saveIsTheSame()
	{
		QFETCH(QString, source);
		QFETCH(QString, op);
		QFETCH(QString, body);
		const QString suffix = source.mid(source.indexOf(QLatin1Char('.')));
		const QString in = m_sources.value(source);
		const QString lazy = m_dir.filePath(QStringLiteral("out-lazy-%1-%2").arg(source, op)) + suffix;
		const QString eager = m_dir.filePath(QStringLiteral("out-eager-%1-%2").arg(source, op)) + suffix;
		QString lazyMask, eagerMask;
		QVERIFY2(edit(in, body, lazy, false, &lazyMask), "the edit did not apply or save with lazy folios");
		QVERIFY2(edit(in, body, eager, true, &eagerMask), "the edit did not apply or save with every folio built");

		bool ok1 = false, ok2 = false, ok3 = false;
		const QStringList a = contents(lazy, lazyMask, &ok1);
		const QStringList b = contents(eager, eagerMask, &ok2);
		const QStringList original = contents(in, QString(), &ok3);
		QVERIFY2(ok1 && ok2 && ok3, "a saved file cannot be read");
		// The edit must be seen in the file: two saves of an edit that did
		// nothing would be the same whatever lazy folios do
		QVERIFY2(b != original, "the edit changed nothing in the save with every folio built");
		QVERIFY2(a == b, qPrintable(firstDifference(a, b)));
	}
};

QTEST_APPLESS_MAIN(tst_lazysaveparity)

#include "tst_lazysaveparity.moc"
