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
// Element numbering schemes have an id, and elements follow a scheme by it
// (elementInformation "formula_id"). Runs the real binary on
// examples/industrial.qet, written before the ids existed: its schemes
// "Equipment" and "XV" are followed by elements, and it also holds
// elements whose formula no scheme defines any more (%prefixV1:%sequ_1...,
// left behind by earlier edits of a scheme), which must stay unlinked.

#include <QtTest>

#include <QDomDocument>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QUuid>

class tst_elementautonumids : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	QString m_preferences;   // contents of the settings file of the next run

	QProcessEnvironment environment()
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		// QET_SETTINGS_DIR, not a file under XDG_CONFIG_HOME: macOS and
		// Windows keep the settings elsewhere and would never read it.
		const QString settings_dir = home + QStringLiteral("/settings");
		if (!m_preferences.isEmpty()) {
			QDir().mkpath(settings_dir + QStringLiteral("/QElectroTech"));
			QFile settings(settings_dir + QStringLiteral("/QElectroTech/QElectroTech.ini"));
			if (settings.open(QIODevice::WriteOnly)) {
				settings.write(m_preferences.toUtf8());
			}
		}
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QET_SETTINGS_DIR"), settings_dir);
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		return env;
	}

	QString resave(const QString &in)
	{
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run));
		QProcess proc;
		proc.setProcessEnvironment(environment());
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), in, out});
		if (!proc.waitForFinished(180000) || proc.exitCode() != 0) return {};
		return out;
	}

	QJsonObject run(const QString &script, const QString &project)
	{
		const QString path = m_dir.filePath(QStringLiteral("probe%1.js").arg(m_run));
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(script.toUtf8());
		f.close();

		QProcess proc;
		proc.setProcessEnvironment(environment());
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), path, project});
		if (!proc.waitForFinished(180000)) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput()
											  + proc.readAllStandardError());
		const QString mark = QStringLiteral("PROBE ");
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(mark);
			if (i >= 0)
				return QJsonDocument::fromJson(line.mid(i + mark.size()).toUtf8()).object();
		}
		qWarning().noquote() << out;
		return {};
	}

	static QByteArray read(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

	struct Saved
	{
		QHash<QString, QUuid> scheme_id;          // title -> id
		QHash<QString, QString> scheme_formula;   // title -> formula
		QString current, current_id;
		// per element: formula and formula_id
		QList<QPair<QString, QString>> elements;
	};

	static Saved parse(const QByteArray &xml)
	{
		Saved s;
		QDomDocument doc;
		if (!doc.setContent(xml)) return s;
		const QDomNodeList groups = doc.elementsByTagName(QStringLiteral("element_autonums"));
		if (!groups.isEmpty()) {
			const QDomElement g = groups.at(0).toElement();
			s.current = g.attribute(QStringLiteral("current_autonum"));
			s.current_id = g.attribute(QStringLiteral("current_autonum_id"));
			const QDomNodeList schemes = g.elementsByTagName(QStringLiteral("element_autonum"));
			for (int i = 0 ; i < schemes.count() ; ++i) {
				const QDomElement e = schemes.at(i).toElement();
				s.scheme_id.insert(e.attribute(QStringLiteral("title")),
								   QUuid(e.attribute(QStringLiteral("id"))));
				s.scheme_formula.insert(e.attribute(QStringLiteral("title")),
										e.attribute(QStringLiteral("formula")));
			}
		}
		const QDomNodeList elements = doc.elementsByTagName(QStringLiteral("element"));
		for (int i = 0 ; i < elements.count() ; ++i) {
			const QDomElement infos = elements.at(i).toElement()
					.firstChildElement(QStringLiteral("elementInformations"));
			if (infos.isNull()) continue;
			QString formula, id;
			for (QDomElement info = infos.firstChildElement(QStringLiteral("elementInformation"));
				 !info.isNull();
				 info = info.nextSiblingElement(QStringLiteral("elementInformation"))) {
				if (info.attribute(QStringLiteral("name")) == QLatin1String("formula"))
					formula = info.text();
				else if (info.attribute(QStringLiteral("name")) == QLatin1String("formula_id"))
					id = info.text();
			}
			s.elements << qMakePair(formula, id);
		}
		return s;
	}

	// industrial.qet with the element labelled @p label frozen
	QString withFrozen(const QString &label)
	{
		QFile in(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		if (!in.open(QIODevice::ReadOnly)) return {};
		QDomDocument doc;
		if (!doc.setContent(&in)) return {};
		int frozen = 0;
		const QDomNodeList elements = doc.elementsByTagName(QStringLiteral("element"));
		for (int i = 0 ; i < elements.count() ; ++i) {
			QDomElement e = elements.at(i).toElement();
			const QDomElement infos = e.firstChildElement(QStringLiteral("elementInformations"));
			for (QDomElement info = infos.firstChildElement(QStringLiteral("elementInformation"));
				 !info.isNull();
				 info = info.nextSiblingElement(QStringLiteral("elementInformation"))) {
				if (info.attribute(QStringLiteral("name")) == QLatin1String("label")
						&& info.text() == label) {
					e.setAttribute(QStringLiteral("freezeLabel"), QStringLiteral("true"));
					++frozen;
				}
			}
		}
		if (frozen != 1) return {};
		const QString out = m_dir.filePath(QStringLiteral("frozen%1.qet").arg(m_run));
		QFile f(out);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(doc.toByteArray());
		return out;
	}

	static QStringList diagramAttributes(const QByteArray &xml, const QString &attribute)
	{
		QStringList values;
		QDomDocument doc;
		if (!doc.setContent(xml)) return values;
		const QDomNodeList diagrams = doc.elementsByTagName(QStringLiteral("diagram"));
		for (int i = 0 ; i < diagrams.count() ; ++i)
			values << diagrams.at(i).toElement().attribute(attribute);
		return values;
	}

	// qet.autoNums() lists the numberings as "name: formula='...'"
	static QStringList titlesOf(const QJsonValue &list)
	{
		QStringList titles;
		for (const QJsonValue &entry : list.toArray()) {
			const QString text = entry.toString();
			const int cut = text.indexOf(QLatin1String(": formula="));
			titles << (cut >= 0 ? text.left(cut) : text);
		}
		titles.sort();
		return titles;
	}

	static int linkedTo(const Saved &s, const QString &title)
	{
		int n = 0;
		for (const auto &e : s.elements)
			if (QUuid(e.second) == s.scheme_id.value(title)) ++n;
		return n;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// A file without ids: every scheme gets one, the same on every load;
	// an element is linked to the one scheme with its formula, and only then.
	void legacyFileGetsIds()
	{
		const QString project = QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet");
		const QString first = resave(project);
		QVERIFY2(!first.isEmpty(), "--resave failed");
		const QString again = resave(project);
		QVERIFY2(!again.isEmpty(), "second --resave of the original failed");
			//Two separate loads write attributes in their own order, so
			//compare what they saved, not the bytes
		const Saved s = parse(read(first));
		const Saved s_again = parse(read(again));
		QVERIFY2(s.scheme_id == s_again.scheme_id,
				 "the ids derived for a legacy file differ between loads");
		QVERIFY2(s.elements == s_again.elements,
				 "the elements were linked differently on another load");
		QVERIFY(!s.scheme_id.isEmpty());
		QSet<QUuid> ids;
		for (auto it = s.scheme_id.constBegin() ; it != s.scheme_id.constEnd() ; ++it) {
			QVERIFY2(!it.value().isNull(), qPrintable(it.key() + QStringLiteral(" has no id")));
			ids << it.value();
		}
		QCOMPARE(ids.size(), s.scheme_id.size());

		QMultiHash<QString, QString> titles_by_formula;
		for (auto it = s.scheme_formula.constBegin() ; it != s.scheme_formula.constEnd() ; ++it)
			titles_by_formula.insert(it.value(), it.key());

		for (const auto &e : s.elements) {
			const QStringList titles = titles_by_formula.values(e.first);
			if (!e.first.isEmpty() && titles.size() == 1) {
				QCOMPARE(QUuid(e.second), s.scheme_id.value(titles.first()));
			} else {
				QVERIFY2(e.second.isEmpty(),
						 qPrintable(QStringLiteral("element with formula '%1' is linked")
									.arg(e.first)));
			}
		}
		QVERIFY(linkedTo(s, QStringLiteral("Equipment")) > 0);
		QVERIFY(linkedTo(s, QStringLiteral("XV")) > 0);

		const QString second = resave(first);
		QVERIFY2(!second.isEmpty(), "--resave of the resaved file failed");
		QVERIFY2(read(first) == read(second), "the second save changed the file");
	}

	// The id given to a numbering of a file written before the ids is the
	// file's own: two projects which both have a numbering of the same name
	// do not get the same id, or a paste from one into the other would
	// take the other's numbering for the same one.
	void derivedIdsBelongToTheirProject()
	{
		QFile in(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY(in.open(QIODevice::ReadOnly));
		QDomDocument doc;
		QVERIFY(doc.setContent(&in));
		doc.documentElement().setAttribute(QStringLiteral("title"), QStringLiteral("Another project"));
		const QString other = m_dir.filePath(QStringLiteral("another-project.qet"));
		QFile out(other);
		QVERIFY(out.open(QIODevice::WriteOnly));
		out.write(doc.toByteArray());
		out.close();

		const QString first = resave(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		const QString second = resave(other);
		QVERIFY2(!first.isEmpty() && !second.isEmpty(), "--resave failed");
		const Saved a = parse(read(first));
		const Saved b = parse(read(second));
		QVERIFY(a.scheme_id.contains(QStringLiteral("Equipment")));
		QVERIFY(b.scheme_id.contains(QStringLiteral("Equipment")));
		QVERIFY(!a.scheme_id.value(QStringLiteral("Equipment")).isNull());
		QVERIFY2(a.scheme_id.value(QStringLiteral("Equipment")) != b.scheme_id.value(QStringLiteral("Equipment")),
				 "two projects gave the same id to their numbering of the same name");
		// each keeps its own ids when saved again
		const QString again = resave(second);
		QVERIFY(!again.isEmpty());
		QCOMPARE(parse(read(again)).scheme_id, b.scheme_id);
	}

	// Rename, refused rename and removal, an edit that keeps the numbers,
	// an edit that renumbers, and undo/redo: through the scripting API,
	// which goes through the same ElementAutoNumSchemeCommand as the UI.
	void renameEditRemoveUndo()
	{
		const QString saved = m_dir.filePath(QStringLiteral("scripted.qet"));
		const QString script = QStringLiteral(R"JS(
function linked(formula) {
  var r = [];
  for (var f = 0; f < qet.folioCount(); ++f) {
    var u = qet.elementUuids(f);
    for (var i = 0; i < u.length; ++i)
      if (qet.elementInfo(f, u[i], 'formula') === formula) r.push([f, u[i]]);
  }
  return r;
}
function label(e) { return qet.elementInfo(e[0], e[1], 'label'); }
function fid(e) { return qet.elementInfo(e[0], e[1], 'formula_id'); }
var xv = linked('%prefix2:%sequ_1');
var eq = linked('%id%prefix%sequ_1');
var r = {xv: xv.length, eq: eq.length};
var e0 = xv[0];
r.id0 = fid(e0);
r.label0 = label(e0);
r.renamed = qet.renameAutoNum('element', 'XV', 'Terminals XV');
r.idAfterRename = fid(e0);
r.clash = qet.renameAutoNum('element', 'Terminals XV', ' equipment ');
r.empty = qet.renameAutoNum('element', 'Terminals XV', '   ');
r.removeUsed = qet.removeAutoNum('element', 'Terminals XV');
r.removeUnused = qet.removeAutoNum('element', 'TB "XPE"');
r.edited = qet.addAutoNum('element', 'Terminals XV', ['elementprefix', 'string:X', 'unit:1']);
r.formulaAfterEdit = qet.elementInfo(e0[0], e0[1], 'formula');
r.labelAfterEdit = label(e0);
r.idAfterEdit = fid(e0);
qet.undo();
r.formulaAfterUndo = qet.elementInfo(e0[0], e0[1], 'formula');
r.labelAfterUndo = label(e0);
qet.redo();
r.labelAfterRedo = label(e0);
r.renumbered = qet.addAutoNum('element', 'Equipment', ['idfolio', 'elementprefix', 'string:-', 'ten:1']);
r.eqStillLinked = linked('%id%prefix-%seqt_1').length;
r.typed = qet.setElementInfo(eq[0][0], eq[0][1], 'formula', '%id%prefix%sequ_1');
r.typedId = fid(eq[0]);
r.internal = qet.setElementInfo(eq[1][0], eq[1][1], 'formula_id', '{00000000-0000-0000-0000-000000000001}');
r.names = qet.autoNums('element');
r.save = qet.save(%1);
qet.log('PROBE ' + JSON.stringify(r));
)JS").arg(QStringLiteral("'") + saved + QStringLiteral("'"));

		const QJsonObject r = run(script, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QVERIFY(r.value(QStringLiteral("xv")).toInt() > 0);
		QVERIFY(r.value(QStringLiteral("eq")).toInt() > 1);

		const QString id0 = r.value(QStringLiteral("id0")).toString();
		QVERIFY(!QUuid(id0).isNull());

		// a rename touches no element
		QVERIFY(r.value(QStringLiteral("renamed")).toBool());
		QCOMPARE(r.value(QStringLiteral("idAfterRename")).toString(), id0);
		// names are unique ignoring case and surrounding spaces, never empty
		QVERIFY(!r.value(QStringLiteral("clash")).toBool());
		QVERIFY(!r.value(QStringLiteral("empty")).toBool());
		// a scheme elements follow cannot be removed, an unused one can
		QVERIFY(!r.value(QStringLiteral("removeUsed")).toBool());
		QVERIFY(r.value(QStringLiteral("removeUnused")).toBool());

		// same sequential parts: new formula, same number, still linked
		QVERIFY(r.value(QStringLiteral("edited")).toBool());
		QCOMPARE(r.value(QStringLiteral("formulaAfterEdit")).toString(),
				 QStringLiteral("%prefixX%sequ_1"));
		QCOMPARE(r.value(QStringLiteral("idAfterEdit")).toString(), id0);
		QString expected = r.value(QStringLiteral("label0")).toString();
		const int colon = expected.lastIndexOf(QStringLiteral("2:"));
		QVERIFY(colon >= 0);
		expected.replace(colon, 2, QStringLiteral("X"));
		QCOMPARE(r.value(QStringLiteral("labelAfterEdit")).toString(), expected);

		// one undo step, both ways
		QCOMPARE(r.value(QStringLiteral("formulaAfterUndo")).toString(),
				 QStringLiteral("%prefix2:%sequ_1"));
		QCOMPARE(r.value(QStringLiteral("labelAfterUndo")).toString(),
				 r.value(QStringLiteral("label0")).toString());
		QCOMPARE(r.value(QStringLiteral("labelAfterRedo")).toString(), expected);

		// other sequential parts: renumbered, every element still linked
		QVERIFY(r.value(QStringLiteral("renumbered")).toBool());
		QCOMPARE(r.value(QStringLiteral("eqStillLinked")).toInt(),
				 r.value(QStringLiteral("eq")).toInt());

		// a formula written by hand follows no scheme, formula_id is internal
		QVERIFY(r.value(QStringLiteral("typed")).toBool());
		QVERIFY(r.value(QStringLiteral("typedId")).toString().isEmpty());
		QVERIFY(!r.value(QStringLiteral("internal")).toBool());

		QVERIFY(r.value(QStringLiteral("save")).toBool());
		const Saved s = parse(read(saved));
		QVERIFY(s.scheme_id.contains(QStringLiteral("Terminals XV")));
		QVERIFY(!s.scheme_id.contains(QStringLiteral("XV")));
		QVERIFY(!s.scheme_id.contains(QStringLiteral("TB \"XPE\"")));
		QCOMPARE(s.scheme_id.value(QStringLiteral("Terminals XV")), QUuid(id0));
		QCOMPARE(linkedTo(s, QStringLiteral("Terminals XV")), r.value(QStringLiteral("xv")).toInt());
		QCOMPARE(linkedTo(s, QStringLiteral("Equipment")), r.value(QStringLiteral("eq")).toInt() - 1);
	}

	// Giving a numbering to elements that have none (what picking it in the
	// element's information window does): formula, id and the next number,
	// the counter moving on, refused where something would be lost, and one
	// undo step that gives the number back.
	void assignScheme()
	{
		const QString script = QStringLiteral(R"JS(
function find(pred, max) {
  var r = [];
  for (var f = 0; f < qet.folioCount() && r.length < max; ++f) {
    var u = qet.elementUuids(f);
    for (var i = 0; i < u.length && r.length < max; ++i)
      if (pred(f, u[i])) r.push([f, u[i]]);
  }
  return r;
}
function info(e, k) { return qet.elementInfo(e[0], e[1], k); }
var r = {};
var candidates = find(function (f, u) { return qet.elementInfo(f, u, 'formula') === ''; }, 40);
var done = [];
for (var i = 0; i < candidates.length && done.length < 3; ++i)
  if (qet.assignElementAutoNum('Equipment', candidates[i][0], candidates[i][1], false)) done.push(candidates[i]);
r.assigned = done.length;
var a = done[0], b = done[1], c = done[2];
r.formulaA = info(a, 'formula');
r.idA = info(a, 'formula_id');
r.labelA = info(a, 'label');
r.labelB = info(b, 'label');
r.labelC = info(c, 'label');
r.again = qet.assignElementAutoNum('Equipment', a[0], a[1], false);
r.other = qet.assignElementAutoNum('XV', a[0], a[1], false);
r.otherOverwrite = qet.assignElementAutoNum('XV', a[0], a[1], true);
r.formulaAfterOverwrite = info(a, 'formula');
r.idAfterOverwrite = info(a, 'formula_id');
r.unknown = qet.assignElementAutoNum('Nope', a[0], a[1], true);
qet.undo();
r.formulaAfterUndo = info(a, 'formula');
r.idAfterUndo = info(a, 'formula_id');
r.save = qet.save(%1);
qet.log('PROBE ' + JSON.stringify(r));
)JS").arg(QStringLiteral("'") + m_dir.filePath(QStringLiteral("assigned.qet")) + QStringLiteral("'"));

		const QJsonObject r = run(script, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QCOMPARE(r.value(QStringLiteral("assigned")).toInt(), 3);

		QCOMPARE(r.value(QStringLiteral("formulaA")).toString(), QStringLiteral("%id%prefix%sequ_1"));
		QVERIFY(!QUuid(r.value(QStringLiteral("idA")).toString()).isNull());
		// the saved counter of "Equipment" is 9: the next numbers, in order
		// (the label is the folio id, the prefix, then the number)
		QVERIFY2(r.value(QStringLiteral("labelA")).toString().endsWith(QLatin1Char('9')),
				 qPrintable(r.value(QStringLiteral("labelA")).toString()));
		QVERIFY2(r.value(QStringLiteral("labelB")).toString().endsWith(QStringLiteral("10")),
				 qPrintable(r.value(QStringLiteral("labelB")).toString()));
		QVERIFY2(r.value(QStringLiteral("labelC")).toString().endsWith(QStringLiteral("11")),
				 qPrintable(r.value(QStringLiteral("labelC")).toString()));

		QVERIFY(!r.value(QStringLiteral("again")).toBool());
		QVERIFY(!r.value(QStringLiteral("other")).toBool());
		QVERIFY(r.value(QStringLiteral("otherOverwrite")).toBool());
		QCOMPARE(r.value(QStringLiteral("formulaAfterOverwrite")).toString(),
				 QStringLiteral("%prefix2:%sequ_1"));
		QVERIFY(r.value(QStringLiteral("idAfterOverwrite")).toString()
				!= r.value(QStringLiteral("idA")).toString());
		QVERIFY(!r.value(QStringLiteral("unknown")).toBool());

		// one undo gives the element its previous numbering back
		QCOMPARE(r.value(QStringLiteral("formulaAfterUndo")).toString(),
				 QStringLiteral("%id%prefix%sequ_1"));
		QCOMPARE(r.value(QStringLiteral("idAfterUndo")).toString(),
				 r.value(QStringLiteral("idA")).toString());

		QVERIFY(r.value(QStringLiteral("save")).toBool());
		const Saved saved = parse(read(m_dir.filePath(QStringLiteral("assigned.qet"))));
		// three elements gained the numbering, whose counter moved past them
		QVERIFY(linkedTo(saved, QStringLiteral("Equipment")) >= 3);
	}

	// An element whose label is frozen is left as it is by a renumbering, and
	// the numbering goes past its number instead of giving it to another
	// element. On industrial.qet the element labelled 5F3 is frozen: without
	// that, the numbering would give 5F3 to another element of folio 5.
	void frozenElementsAreSkipped()
	{
		const QString body = QStringLiteral(R"JS(
function labels() {
  var r = {};
  for (var f = 0; f < qet.folioCount(); ++f) {
    var u = qet.elementUuids(f);
    for (var i = 0; i < u.length; ++i)
      if (qet.elementInfo(f, u[i], 'formula') === '%id%prefix%sequ_1')
        r[f + '/' + u[i]] = qet.elementInfo(f, u[i], 'label');
  }
  return r;
}
var before = labels();
var r = {before: before};
r.frozen = qet.renumberElementAutoNum('Equipment');
r.after = labels();
r.missing = qet.renumberElementAutoNum('Nope');
qet.undo();
r.undone = labels();
qet.log('PROBE ' + JSON.stringify(r));
)JS");

		// control: nothing frozen, 5F3 goes to another element
		const QJsonObject c = run(body, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!c.isEmpty(), "the control script logged nothing");
		QCOMPARE(c.value(QStringLiteral("frozen")).toInt(), 0);
		const QJsonObject cb = c.value(QStringLiteral("before")).toObject();
		const QJsonObject ca = c.value(QStringLiteral("after")).toObject();
		QString holder;
		int given = 0;
		for (auto it = cb.constBegin() ; it != cb.constEnd() ; ++it) {
			if (it.value().toString() == QLatin1String("5F3")) holder = it.key();
		}
		QVERIFY2(!holder.isEmpty(), "no element is labelled 5F3 in industrial.qet");
		for (auto it = ca.constBegin() ; it != ca.constEnd() ; ++it) {
			if (it.value().toString() == QLatin1String("5F3") && it.key() != holder) ++given;
		}
		QVERIFY2(given == 1, "the control renumbering does not give 5F3 to another element: "
							 "this test would prove nothing");

		// with the element frozen
		const QString project = withFrozen(QStringLiteral("5F3"));
		QVERIFY2(!project.isEmpty(), "cannot freeze the element");
		const QJsonObject r = run(body, project);
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QCOMPARE(r.value(QStringLiteral("frozen")).toInt(), 1);
		QCOMPARE(r.value(QStringLiteral("missing")).toInt(), -1);

		const QJsonObject before = r.value(QStringLiteral("before")).toObject();
		const QJsonObject after = r.value(QStringLiteral("after")).toObject();
		QCOMPARE(after.size(), before.size());
		QCOMPARE(after.value(holder).toString(), QStringLiteral("5F3"));

		QSet<QString> seen;
		int changed = 0;
		for (auto it = after.constBegin() ; it != after.constEnd() ; ++it) {
			const QString label = it.value().toString();
			QVERIFY2(!seen.contains(label), qPrintable(QStringLiteral("two elements are labelled %1").arg(label)));
			seen << label;
			if (before.value(it.key()).toString() != label) ++changed;
		}
		QVERIFY(changed > 0);   // the others were numbered again

		// one undo gives every label back
		QCOMPARE(r.value(QStringLiteral("undone")).toObject(), before);
	}

	// A scheme whose formula would change cannot be edited while an element
	// with a frozen label follows it. A rename, and a change of the counter
	// (same formula), still can: they touch no label.
	void frozenElementsBlockAnEdit()
	{
		const QString project = withFrozen(QStringLiteral("5F3"));
		QVERIFY2(!project.isEmpty(), "cannot freeze the element");
		const QString script = QStringLiteral(R"JS(
function find(label) {
  for (var f = 0; f < qet.folioCount(); ++f) {
    var u = qet.elementUuids(f);
    for (var i = 0; i < u.length; ++i)
      if (qet.elementInfo(f, u[i], 'label') === label) return [f, u[i]];
  }
  return null;
}
var frozen = find('5F3'), other = find('5F4');
var r = {oldFormula: qet.elementInfo(frozen[0], frozen[1], 'formula')};
r.refused = qet.addAutoNum('element', 'Equipment', ['idfolio', 'elementprefix', 'unit:1', 'string:x']);
r.frozenFormula = qet.elementInfo(frozen[0], frozen[1], 'formula');
r.frozenLabel = qet.elementInfo(frozen[0], frozen[1], 'label');
r.otherFormula = qet.elementInfo(other[0], other[1], 'formula');
r.otherLabel = qet.elementInfo(other[0], other[1], 'label');
r.counter = qet.addAutoNum('element', 'Equipment', ['idfolio', 'elementprefix', 'unit:40']);
r.otherAfterCounter = qet.elementInfo(other[0], other[1], 'label');
r.renamed = qet.renameAutoNum('element', 'Equipment', 'Equipment 2');
r.frozenId = qet.elementInfo(frozen[0], frozen[1], 'formula_id');
r.names = qet.autoNums('element');
qet.log('PROBE ' + JSON.stringify(r));
)JS");
		const QJsonObject r = run(script, project);
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QVERIFY(!r.value(QStringLiteral("refused")).toBool());
		QCOMPARE(r.value(QStringLiteral("frozenFormula")).toString(),
				 r.value(QStringLiteral("oldFormula")).toString());
		QCOMPARE(r.value(QStringLiteral("frozenLabel")).toString(), QStringLiteral("5F3"));
		QCOMPARE(r.value(QStringLiteral("otherFormula")).toString(), r.value(QStringLiteral("oldFormula")).toString());
		QCOMPARE(r.value(QStringLiteral("otherLabel")).toString(), QStringLiteral("5F4"));

		// same formula: allowed, and no label moves
		QVERIFY(r.value(QStringLiteral("counter")).toBool());
		QCOMPARE(r.value(QStringLiteral("otherAfterCounter")).toString(), QStringLiteral("5F4"));
		// a rename is allowed, the frozen element still follows the numbering
		QVERIFY(r.value(QStringLiteral("renamed")).toBool());
		QVERIFY(!QUuid(r.value(QStringLiteral("frozenId")).toString()).isNull());
		QVERIFY(QJsonDocument(r.value(QStringLiteral("names")).toArray()).toJson().contains("Equipment 2"));
	}

	// Duplicating elements which follow a numbering gives the copies the next
	// numbers of it instead of the labels of the originals, and the numbering
	// moves on; one undo takes the copies away and gives the numbers back, one
	// redo brings the same numbers again.
	void duplicatesAreNumbered()
	{
		const QString script = QStringLiteral(R"JS(
function labelsOf(f) {
  var r = [], u = qet.elementUuids(f);
  for (var i = 0; i < u.length; ++i)
    if (qet.elementInfo(f, u[i], 'formula') === '%id%prefix%sequ_1') r.push(qet.elementInfo(f, u[i], 'label'));
  return r;
}
function find(label) {
  for (var f = 0; f < qet.folioCount(); ++f) {
    var u = qet.elementUuids(f);
    for (var i = 0; i < u.length; ++i)
      if (qet.elementInfo(f, u[i], 'label') === label) return [f, u[i]];
  }
  return null;
}
var a = find('8F1'), b = find('8F2');
var r = {before: labelsOf(a[0])};
var created = qet.duplicateElements(a[0], [a[1], b[1]], a[0], 0, 60);
r.count = created.length;
r.labels = created.map(function (u) { return qet.elementInfo(a[0], u, 'label'); });
r.formulas = created.map(function (u) { return qet.elementInfo(a[0], u, 'formula'); });
r.ids = created.map(function (u) { return qet.elementInfo(a[0], u, 'formula_id'); });
r.originalId = qet.elementInfo(a[0], a[1], 'formula_id');
r.afterAll = labelsOf(a[0]);
qet.undo();
r.afterUndo = labelsOf(a[0]);
qet.redo();
r.afterRedo = labelsOf(a[0]);
qet.undo();
// the counter went back: copying again gives the same numbers
var again = qet.duplicateElements(a[0], [a[1], b[1]], a[0], 0, 60);
r.again = again.map(function (u) { return qet.elementInfo(a[0], u, 'label'); });
qet.log('PROBE ' + JSON.stringify(r));
)JS");
		m_preferences.clear();
		const QJsonObject r = run(script, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QCOMPARE(r.value(QStringLiteral("count")).toInt(), 2);

		const QJsonArray labels = r.value(QStringLiteral("labels")).toArray();
		const QJsonArray before = r.value(QStringLiteral("before")).toArray();
		QCOMPARE(labels.size(), 2);
		for (const QJsonValue &label : labels) {
			QVERIFY2(!label.toString().isEmpty(), "a copy has no label");
			QVERIFY2(!before.contains(label), qPrintable(QStringLiteral("a copy has the label %1 of another element").arg(label.toString())));
		}
		QVERIFY(labels.at(0) != labels.at(1));
		QVERIFY(labels.at(0).toString().startsWith(QLatin1String("8F")));

		const QJsonArray formulas = r.value(QStringLiteral("formulas")).toArray();
		const QJsonArray ids = r.value(QStringLiteral("ids")).toArray();
		for (int i = 0 ; i < 2 ; ++i) {
			QCOMPARE(formulas.at(i).toString(), QStringLiteral("%id%prefix%sequ_1"));
			QCOMPARE(ids.at(i).toString(), r.value(QStringLiteral("originalId")).toString());
		}

		// the originals are untouched, and every label is unique
		const QJsonArray all = r.value(QStringLiteral("afterAll")).toArray();
		QCOMPARE(all.size(), before.size() + 2);
		QSet<QString> unique;
		for (const QJsonValue &label : all) unique << label.toString();
		QCOMPARE(unique.size(), all.size());
		for (const QJsonValue &label : before) QVERIFY(all.contains(label));

		// undo, redo, undo and copy again: the same numbers
		QCOMPARE(r.value(QStringLiteral("afterUndo")).toArray(), before);
		QCOMPARE(r.value(QStringLiteral("afterRedo")).toArray().size(), all.size());
		QCOMPARE(r.value(QStringLiteral("again")).toArray(), labels);
	}

	// The preference "number pasted elements" is off: a copy is what a copy
	// was, whatever the label preference says. Erasing (the default) leaves
	// it with no label and no numbering; keeping leaves it with the label of
	// the original, still tied to the same numbering.
	void duplicatesAreNotNumberedWhenSwitchedOff()
	{
		const QString script = QStringLiteral(R"JS(
function find(label) {
  for (var f = 0; f < qet.folioCount(); ++f) {
    var u = qet.elementUuids(f);
    for (var i = 0; i < u.length; ++i)
      if (qet.elementInfo(f, u[i], 'label') === label) return [f, u[i]];
  }
  return null;
}
var a = find('8F1');
var created = qet.duplicateElements(a[0], [a[1]], a[0], 0, 60);
var r = {label: qet.elementInfo(a[0], created[0], 'label'),
         formula: qet.elementInfo(a[0], created[0], 'formula'),
         id: qet.elementInfo(a[0], created[0], 'formula_id')};
qet.log('PROBE ' + JSON.stringify(r));
)JS");
		m_preferences = QStringLiteral("[diagramcommands]\nautonumber-pasted-elements=false\n");
		const QJsonObject erased = run(script, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!erased.isEmpty(), "the script logged nothing");
		QVERIFY(erased.value(QStringLiteral("label")).toString().isEmpty());
		QVERIFY(erased.value(QStringLiteral("formula")).toString().isEmpty());
		QVERIFY(erased.value(QStringLiteral("id")).toString().isEmpty());

		m_preferences = QStringLiteral("[diagramcommands]\nautonumber-pasted-elements=false\nerase-label-on-copy=false\n");
		const QJsonObject kept = run(script, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!kept.isEmpty(), "the script logged nothing");
		QCOMPARE(kept.value(QStringLiteral("label")).toString(), QStringLiteral("8F1"));
		QCOMPARE(kept.value(QStringLiteral("formula")).toString(), QStringLiteral("%id%prefix%sequ_1"));
		QVERIFY(!QUuid(kept.value(QStringLiteral("id")).toString()).isNull());
		m_preferences.clear();
	}

	// Conductor numberings: names are unique and never empty, a rename moves
	// the folios which read the numbering with it (26 folios of
	// industrial.qet read "Wire"), a numbering which folios read cannot be
	// removed, an unused one can, and every step is one undo.
	void conductorNumberings()
	{
		const QString saved = m_dir.filePath(QStringLiteral("conductors.qet"));
		const QString script = QStringLiteral(R"JS(
var r = {};
r.names = qet.autoNums('conductor');
r.clash = qet.addAutoNum('conductor', ' wire ', ['unit:1']);
r.empty = qet.addAutoNum('conductor', '   ', ['unit:1']);
r.added = qet.addAutoNum('conductor', 'Spare', ['string:S', 'unit:1']);
r.renameClash = qet.renameAutoNum('conductor', 'Spare', 'WIRE');
r.removeUsed = qet.removeAutoNum('conductor', 'Wire');
r.renamed = qet.renameAutoNum('conductor', 'Wire', 'Cable');
r.afterRename = qet.autoNums('conductor');
r.missing = qet.renameAutoNum('conductor', 'Nope', 'X');
r.removeSpare = qet.removeAutoNum('conductor', 'Spare');
r.afterRemove = qet.autoNums('conductor');
qet.undo();                      // the removal
r.afterUndoRemove = qet.autoNums('conductor');
qet.undo(); qet.undo();          // the rename, then the creation
qet.redo(); qet.redo();          // the creation, then the rename
r.afterRedo = qet.autoNums('conductor');
r.save = qet.save(%1);
qet.log('PROBE ' + JSON.stringify(r));
)JS").arg(QStringLiteral("'") + saved + QStringLiteral("'"));

		const QJsonObject r = run(script, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		const auto names = [&](const char *key) {
			return titlesOf(r.value(QLatin1String(key)));
		};
		QCOMPARE(names("names"), QStringList{QStringLiteral("Wire")});
		QVERIFY(!r.value(QStringLiteral("clash")).toBool());          // " wire " is "Wire"
		QVERIFY(!r.value(QStringLiteral("empty")).toBool());
		QVERIFY(r.value(QStringLiteral("added")).toBool());
		QVERIFY(!r.value(QStringLiteral("renameClash")).toBool());    // "WIRE" is "Wire"
		QVERIFY(!r.value(QStringLiteral("removeUsed")).toBool());     // 26 folios read it
		QVERIFY(r.value(QStringLiteral("renamed")).toBool());
		QCOMPARE(names("afterRename"), (QStringList{QStringLiteral("Cable"), QStringLiteral("Spare")}));
		QVERIFY(!r.value(QStringLiteral("missing")).toBool());
		QVERIFY(r.value(QStringLiteral("removeSpare")).toBool());     // no folio reads it
		QCOMPARE(names("afterRemove"), QStringList{QStringLiteral("Cable")});
		QCOMPARE(names("afterUndoRemove"), (QStringList{QStringLiteral("Cable"), QStringLiteral("Spare")}));
		QCOMPARE(names("afterRedo"), (QStringList{QStringLiteral("Cable"), QStringLiteral("Spare")}));

		// the folios read the numbering under its new name
		QVERIFY(r.value(QStringLiteral("save")).toBool());
		const QByteArray xml = read(saved);
		const QStringList followed = diagramAttributes(xml, QStringLiteral("conductorAutonum"));
		QCOMPARE(followed.count(QStringLiteral("Cable")), 26);
		QCOMPARE(followed.count(QStringLiteral("Wire")), 0);
	}

	// Folio numberings. A folio numbering is applied once: the number is
	// written into the folio field and the title block keeps the name (49 of
	// the 50 folios of industrial.qet name "Name of the new numbering", none
	// has a folio field which still holds %autonum). A rename moves every name,
	// removing a numbering which folios only name is allowed, and undo and
	// redo give back the same names.
	void folioNumberings()
	{
		const QString renamed_file = m_dir.filePath(QStringLiteral("folio-renamed.qet"));
		const QString undone_file = m_dir.filePath(QStringLiteral("folio-undone.qet"));
		const QString script = QStringLiteral(R"JS(
var r = {};
r.names = qet.autoNums('folio');
r.clash = qet.addAutoNum('folio', 'name of the new numbering', ['unit:1']);
r.empty = qet.addAutoNum('folio', ' ', ['unit:1']);
r.renamed = qet.renameAutoNum('folio', 'Name of the new numbering', 'Folios');
r.afterRename = qet.autoNums('folio');
r.saveRenamed = qet.save(%1);
r.removed = qet.removeAutoNum('folio', 'Folios');
r.afterRemove = qet.autoNums('folio');
qet.undo();                         // the removal
r.afterUndoRemove = qet.autoNums('folio');
qet.undo();                         // the rename
r.afterUndoRename = qet.autoNums('folio');
r.saveUndone = qet.save(%2);
qet.redo();
r.afterRedo = qet.autoNums('folio');
qet.log('PROBE ' + JSON.stringify(r));
)JS").arg(QStringLiteral("'") + renamed_file + QStringLiteral("'"),
		  QStringLiteral("'") + undone_file + QStringLiteral("'"));

		const QJsonObject r = run(script, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		const auto names = [&](const char *key) {
			return titlesOf(r.value(QLatin1String(key)));
		};
		const QString old_name = QStringLiteral("Name of the new numbering");
		QCOMPARE(names("names"), QStringList{old_name});
		QVERIFY(!r.value(QStringLiteral("clash")).toBool());
		QVERIFY(!r.value(QStringLiteral("empty")).toBool());
		QVERIFY(r.value(QStringLiteral("renamed")).toBool());
		QCOMPARE(names("afterRename"), QStringList{QStringLiteral("Folios")});
		QVERIFY(r.value(QStringLiteral("removed")).toBool());
		QCOMPARE(names("afterRemove"), QStringList());
		QCOMPARE(names("afterUndoRemove"), QStringList{QStringLiteral("Folios")});
		QCOMPARE(names("afterUndoRename"), QStringList{old_name});
		QCOMPARE(names("afterRedo"), QStringList{QStringLiteral("Folios")});

		QVERIFY(r.value(QStringLiteral("saveRenamed")).toBool());
		QStringList named = diagramAttributes(read(renamed_file), QStringLiteral("auto_page_num"));
		QCOMPARE(named.size(), 50);
		QCOMPARE(named.count(QStringLiteral("Folios")), 49);
		QCOMPARE(named.count(old_name), 0);

		QVERIFY(r.value(QStringLiteral("saveUndone")).toBool());
		named = diagramAttributes(read(undone_file), QStringLiteral("auto_page_num"));
		QCOMPARE(named.count(old_name), 49);
		QCOMPARE(named.count(QStringLiteral("Folios")), 0);
	}

	// Free numbers. An element which follows a numbering of one sequence may be
	// given a number nobody has, and keeps following the numbering: the case of
	// an element which, after a reshuffle, must have its real-life number back.
	// Only free numbers are offered and accepted; one undo gives the number back;
	// a number past the counter moves the counter, so that the next element is
	// not given the same label.
	void freeNumbers()
	{
		const QString script = QStringLiteral(R"JS(
function find(label) {
  for (var f = 0; f < qet.folioCount(); ++f) {
    var u = qet.elementUuids(f);
    for (var i = 0; i < u.length; ++i)
      if (qet.elementInfo(f, u[i], 'label') === label) return [f, u[i]];
  }
  return null;
}
function number(label) { var m = /(\d+)$/.exec(label); return m ? parseInt(m[1]) : -1; }
function followers() {
  var r = [];
  for (var f = 0; f < qet.folioCount(); ++f) {
    var u = qet.elementUuids(f);
    for (var i = 0; i < u.length; ++i)
      if (qet.elementInfo(f, u[i], 'formula') === '%id%prefix%sequ_1') r.push(number(qet.elementInfo(f, u[i], 'label')));
  }
  return r.sort(function (a, b) { return a - b; });
}
var e = find('8F2'), other = find('8F1');
var r = {used: followers()};
r.before = qet.elementInfo(e[0], e[1], 'label');
r.own = number(r.before);
r.free = qet.freeElementNumbers(e[0], e[1]);
r.numberOfOther = number(qet.elementInfo(other[0], other[1], 'label'));
r.idBefore = qet.elementInfo(e[0], e[1], 'formula_id');

r.assignUsed = qet.assignElementNumber(e[0], e[1], r.numberOfOther);     // taken: refused
r.assignZero = qet.assignElementNumber(e[0], e[1], 0);
r.assignFree = qet.assignElementNumber(e[0], e[1], r.free[0]);
r.after = qet.elementInfo(e[0], e[1], 'label');
r.formulaAfter = qet.elementInfo(e[0], e[1], 'formula');
r.idAfter = qet.elementInfo(e[0], e[1], 'formula_id');
r.freeAfter = qet.freeElementNumbers(e[0], e[1]);                       // its old number is free now
qet.undo();
r.undone = qet.elementInfo(e[0], e[1], 'label');

// a number past the counter: the counter moves past it
var high = r.free[r.free.length - 1];
r.high = high;
r.assignHigh = qet.assignElementNumber(e[0], e[1], high);
r.highLabel = qet.elementInfo(e[0], e[1], 'label');
var plain = null;
for (var f = 0; f < qet.folioCount() && !plain; ++f) {
  var u = qet.elementUuids(f);
  for (var i = 0; i < u.length; ++i)
    if (qet.elementInfo(f, u[i], 'formula') === '') { plain = [f, u[i]]; break; }
}
// an element which follows nothing has no numbers to choose from, nor can it be given one
r.noFree = qet.freeElementNumbers(plain[0], plain[1]).length;
r.assignNothing = qet.assignElementNumber(plain[0], plain[1], 5);
// the next element of the numbering is past the number given by hand
r.assignNext = qet.assignElementAutoNum('Equipment', plain[0], plain[1], false);
r.nextNumber = number(qet.elementInfo(plain[0], plain[1], 'label'));
qet.log('PROBE ' + JSON.stringify(r));
)JS");
		const QJsonObject r = run(script, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");

		const QJsonArray used = r.value(QStringLiteral("used")).toArray();
		const QJsonArray free = r.value(QStringLiteral("free")).toArray();
		QVERIFY(!free.isEmpty());
		const int own = r.value(QStringLiteral("own")).toInt();
		for (const QJsonValue &n : free)
			QVERIFY2(!used.contains(n) || n.toInt() == own, "a number in use is offered");
		QVERIFY(!free.contains(r.value(QStringLiteral("numberOfOther"))));

		QVERIFY(!r.value(QStringLiteral("assignUsed")).toBool());
		QVERIFY(!r.value(QStringLiteral("assignZero")).toBool());
		QVERIFY(r.value(QStringLiteral("assignFree")).toBool());
		const QString after = r.value(QStringLiteral("after")).toString();
		QVERIFY(after != r.value(QStringLiteral("before")).toString());
		QVERIFY2(after.endsWith(QString::number(free.at(0).toInt())), qPrintable(after));
		QVERIFY(after.startsWith(QLatin1String("8F")));
		QCOMPARE(r.value(QStringLiteral("formulaAfter")).toString(), QStringLiteral("%id%prefix%sequ_1"));
		QCOMPARE(r.value(QStringLiteral("idAfter")).toString(), r.value(QStringLiteral("idBefore")).toString());
		// the number it had is free for another element now
		QVERIFY(r.value(QStringLiteral("freeAfter")).toArray().size() >= free.size());
		QCOMPARE(r.value(QStringLiteral("undone")).toString(), r.value(QStringLiteral("before")).toString());

		// a number past the counter moves the counter on
		QVERIFY(r.value(QStringLiteral("assignHigh")).toBool());
		QVERIFY(r.value(QStringLiteral("assignNext")).toBool());
		QVERIFY2(r.value(QStringLiteral("nextNumber")).toInt() > r.value(QStringLiteral("high")).toInt(),
				 "the next element was given a number which is not past the one given by hand");
		QCOMPARE(r.value(QStringLiteral("noFree")).toInt(), 0);
		QVERIFY(!r.value(QStringLiteral("assignNothing")).toBool());
	}

	// A numbering whose counter is set back over numbers in use does not hand
	// them out again: placing an element takes the first number whose label
	// nobody carries, and the counter goes on from there; one undo gives the
	// counter back. A label held by an element which does not follow the
	// numbering counts too.
	void placementSkipsLabelsInUse()
	{
		const QString script = QStringLiteral(R"JS(
function labels() {
  var r = [];
  for (var f = 0; f < qet.folioCount(); ++f) {
    var u = qet.elementUuids(f);
    for (var i = 0; i < u.length; ++i) {
      var l = qet.elementInfo(f, u[i], 'label');
      if (l !== '') r.push(l);
    }
  }
  return r;
}
// number the first element without a formula which accepts one (a slave or a report does not)
function numberOne(skip) {
  for (var f = 0; f < qet.folioCount(); ++f) {
    var u = qet.elementUuids(f);
    for (var i = 0; i < u.length; ++i) {
      if (qet.elementInfo(f, u[i], 'formula') !== '' || skip.indexOf(u[i]) >= 0) continue;
      if (qet.numberElement(f, u[i])) return [f, u[i]];
    }
  }
  return null;
}
var r = {};
// the counter of "Equipment" back at 1, over the 29 elements which have numbers: allowed
r.back = qet.addAutoNum('element', 'Equipment', ['idfolio', 'elementprefix', 'unit:1']);
r.use = qet.useElementAutoNum('Equipment');
var held = labels();
var a = numberOne([]);
r.placedA = a !== null;
r.labelA = qet.elementInfo(a[0], a[1], 'label');
r.heldA = held.indexOf(r.labelA) >= 0;
var b = numberOne([a[1]]);
r.placedB = b !== null;
r.labelB = qet.elementInfo(b[0], b[1], 'label');
r.heldB = held.indexOf(r.labelB) >= 0 || r.labelB === r.labelA;
r.formulaA = qet.elementInfo(a[0], a[1], 'formula');
// one undo takes B's number back, the next undo A's
qet.undo();
r.afterUndoB = qet.elementInfo(b[0], b[1], 'label');
qet.undo();
r.afterUndoA = qet.elementInfo(a[0], a[1], 'label');
r.again = qet.numberElement(a[0], a[1]);
r.labelAgain = qet.elementInfo(a[0], a[1], 'label');
qet.log('PROBE ' + JSON.stringify(r));
)JS");
		const QJsonObject r = run(script, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QVERIFY(r.value(QStringLiteral("back")).toBool());
		QVERIFY(r.value(QStringLiteral("use")).toBool());
		QVERIFY(r.value(QStringLiteral("placedA")).toBool());
		QVERIFY(r.value(QStringLiteral("placedB")).toBool());
		const QString a = r.value(QStringLiteral("labelA")).toString();
		const QString b = r.value(QStringLiteral("labelB")).toString();
		QVERIFY2(!a.isEmpty() && !b.isEmpty(), "an element was not given a label");
		QVERIFY2(!r.value(QStringLiteral("heldA")).toBool(), qPrintable(QStringLiteral("%1 is held by another element").arg(a)));
		QVERIFY2(!r.value(QStringLiteral("heldB")).toBool(), qPrintable(QStringLiteral("%1 is held by another element").arg(b)));
		QVERIFY(a != b);
		QCOMPARE(r.value(QStringLiteral("formulaA")).toString(), QStringLiteral("%id%prefix%sequ_1"));
		// undoing gives the numbers back, and numbering again gives the same label
		QVERIFY(r.value(QStringLiteral("afterUndoB")).toString() != b);
		QVERIFY(r.value(QStringLiteral("afterUndoA")).toString() != a);
		QVERIFY(r.value(QStringLiteral("again")).toBool());
		QCOMPARE(r.value(QStringLiteral("labelAgain")).toString(), a);
	}
};

QTEST_APPLESS_MAIN(tst_elementautonumids)

#include "tst_elementautonumids.moc"
