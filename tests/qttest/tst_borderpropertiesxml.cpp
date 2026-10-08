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
#include "borderproperties.h"

#include <QtTest>
#include <QDomDocument>

/**
	BorderProperties::fromXml() -- the <border> element a project keeps for
	its new folios. Its column width and row height are decimals, written
	by toXml() as "%1"; they used to be read with toInt(), which is 0 for
	"60.5", and 0 is then clamped to the 5 px minimum by the folio.
*/
class tst_borderpropertiesxml : public QObject
{
	Q_OBJECT

	static QDomElement border(const QString &attributes)
	{
		static QDomDocument doc;
		doc.setContent(QStringLiteral("<border %1/>").arg(attributes));
		return doc.documentElement();
	}

	static BorderProperties base()
	{
		BorderProperties p;
		p.columns_count = 17;
		p.columns_width = 60;
		p.rows_count = 8;
		p.rows_height = 80;
		p.display_columns = true;
		p.display_rows = true;
		return p;
	}

private slots:
	void sizesAreReadAsDecimals_data()
	{
		QTest::addColumn<QString>("colsize");
		QTest::addColumn<QString>("rowsize");
		QTest::addColumn<qreal>("width");
		QTest::addColumn<qreal>("height");
		QTest::newRow("whole numbers") << "50" << "100" << 50.0 << 100.0;
		QTest::newRow("decimals")      << "60.5" << "80.25" << 60.5 << 80.25;
		QTest::newRow("rounds down")   << "61.3" << "61.3" << 61.3 << 61.3;
		QTest::newRow("rounds up")     << "61.7" << "61.7" << 61.7 << 61.7;
	}

	void sizesAreReadAsDecimals()
	{
		QFETCH(QString, colsize);
		QFETCH(QString, rowsize);
		QFETCH(qreal, width);
		QFETCH(qreal, height);

		BorderProperties p = base();
		QDomElement e = border(QStringLiteral("cols=\"17\" colsize=\"%1\" rows=\"8\" rowsize=\"%2\"")
				       .arg(colsize, rowsize));
		p.fromXml(e);
		QCOMPARE(p.columns_width, width);
		QCOMPARE(p.rows_height, height);
		QCOMPARE(p.columns_count, 17);
		QCOMPARE(p.rows_count, 8);
	}

	void savedSizesComeBack()
	{
		BorderProperties saved = base();
		saved.columns_width = 60.5;
		saved.rows_height = 80.25;
		QDomDocument doc;
		QDomElement e = doc.createElement(QStringLiteral("border"));
		saved.toXml(e);

		BorderProperties loaded = base();
		loaded.fromXml(e);
		QCOMPARE(loaded.columns_width, 60.5);
		QCOMPARE(loaded.rows_height, 80.25);
	}

	// An unreadable size leaves the previous value, as a missing one does.
	void unreadableSizeIsLeftAlone_data()
	{
		QTest::addColumn<QString>("attributes");
		QTest::newRow("missing")  << "cols=\"17\" rows=\"8\"";
		QTest::newRow("text")     << "colsize=\"wide\" rowsize=\"tall\"";
		QTest::newRow("nan")      << "colsize=\"nan\" rowsize=\"nan\"";
		QTest::newRow("inf")      << "colsize=\"inf\" rowsize=\"-inf\"";
	}

	void unreadableSizeIsLeftAlone()
	{
		QFETCH(QString, attributes);
		BorderProperties p = base();
		QDomElement e = border(attributes);
		p.fromXml(e);
		QCOMPARE(p.columns_width, 60.0);
		QCOMPARE(p.rows_height, 80.0);
	}
};

QTEST_GUILESS_MAIN(tst_borderpropertiesxml)
#include "tst_borderpropertiesxml.moc"
