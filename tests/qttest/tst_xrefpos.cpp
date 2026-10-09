// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include "properties/xrefproperties.h"

/**
	XRefProperties reads the cross-reference position from a project's
	<xref xrefpos="..."> and from the settings file. Older versions saved an
	empty value; read as-is it drew the cross-reference over the element's
	label (issue #1238). An empty or unknown value must fall back to the
	default, AlignBottom, and a valid one must be kept.
*/
class tst_xrefpos : public QObject
{
	Q_OBJECT

	static Qt::AlignmentFlag fromXml(const QString &attribute, bool present = true)
	{
		QDomDocument doc;
		QDomElement e = doc.createElement("xref");
		e.setAttribute("type", "protection");
		if (present)
			e.setAttribute("xrefpos", attribute);
		XRefProperties xrp;
		xrp.fromXml(e);
		return xrp.getXrefPos();
	}

	private slots:
	void xml_data()
	{
		QTest::addColumn<QString>("stored");
		QTest::addColumn<int>("expected");
		QTest::newRow("empty")    << ""              << int(Qt::AlignBottom);
		QTest::newRow("garbage")  << "Sideways"      << int(Qt::AlignBottom);
		QTest::newRow("not offered") << "AlignJustify" << int(Qt::AlignBottom);
		QTest::newRow("bottom")   << "AlignBottom"   << int(Qt::AlignBottom);
		QTest::newRow("top")      << "AlignTop"      << int(Qt::AlignTop);
		QTest::newRow("left")     << "AlignLeft"     << int(Qt::AlignLeft);
		QTest::newRow("right")    << "AlignRight"    << int(Qt::AlignRight);
		QTest::newRow("baseline") << "AlignBaseline" << int(Qt::AlignBaseline);
		QTest::newRow("hcenter")  << "AlignHCenter"  << int(Qt::AlignHCenter);
	}

	void xml()
	{
		QFETCH(QString, stored);
		QFETCH(int, expected);
		QCOMPARE(int(fromXml(stored)), expected);
	}

	void xmlMissing()
	{
		QCOMPARE(fromXml(QString(), false), Qt::AlignBottom);
	}

	/**
		An empty value is written back as "AlignBottom" on the next save,
		so the file heals instead of carrying the empty value forward.
	*/
	void emptyIsRewritten()
	{
		QDomDocument doc;
		QDomElement e = doc.createElement("xref");
		e.setAttribute("xrefpos", "");
		XRefProperties xrp;
		xrp.fromXml(e);
		QCOMPARE(xrp.toXml(doc).attribute("xrefpos"), QStringLiteral("AlignBottom"));
	}

	void settings()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		QSettings s(dir.filePath("qet.ini"), QSettings::IniFormat);
		XRefProperties xrp;

		s.setValue("xrefprotectionxrefpos", "");
		xrp.fromSettings(s, "xrefprotection");
		QCOMPARE(xrp.getXrefPos(), Qt::AlignBottom);

		s.remove("xrefprotectionxrefpos");
		xrp.setXrefPos(Qt::AlignTop);
		xrp.fromSettings(s, "xrefprotection");
		QCOMPARE(xrp.getXrefPos(), Qt::AlignBottom);

		s.setValue("xrefprotectionxrefpos", "AlignRight");
		xrp.fromSettings(s, "xrefprotection");
		QCOMPARE(xrp.getXrefPos(), Qt::AlignRight);
	}
};

QTEST_GUILESS_MAIN(tst_xrefpos)
#include "tst_xrefpos.moc"
