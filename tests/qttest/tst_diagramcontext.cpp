// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include "diagramcontext.h"
#include "qetapp.h"

QString QETApp::m_interface_language;

/**
	DiagramContext::fromXml() has two readers: QDom, for projects, and
	pugixml, for element definitions in the collection. Both must give the
	values the next save writes -- stray spaces around real content trimmed,
	accented characters kept.
*/
class tst_diagramcontext : public QObject
{
	Q_OBJECT

	static QByteArray xml(const QString &value)
	{
		return QStringLiteral(
				   "<elementInformations>"
				   "<elementInformation name=\"v\" show=\"1\">%1</elementInformation>"
				   "</elementInformations>")
				.arg(value)
				.toUtf8();
	}

	static QString fromDom(const QByteArray &data)
	{
		QDomDocument doc;
		if (!doc.setContent(data)) return QStringLiteral("<parse error>");
		DiagramContext dc;
		dc.fromXml(doc.documentElement(), QStringLiteral("elementInformation"));
		return dc.value(QStringLiteral("v")).toString();
	}

	static QString fromPugi(const QByteArray &data)
	{
		pugi::xml_document doc;
		if (!doc.load_buffer(data.constData(), size_t(data.size())))
			return QStringLiteral("<parse error>");
		DiagramContext dc;
		dc.fromXml(doc.document_element(), QStringLiteral("elementInformation"));
		return dc.value(QStringLiteral("v")).toString();
	}

private slots:
	void bothReadersAgree_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<QString>("expected");

		QTest::newRow("plain")       << "PRISE" << "PRISE";
		QTest::newRow("stray spaces") << " PRISE  " << "PRISE";
		QTest::newRow("accents")     << "Armoire façade été" << "Armoire façade été";
		QTest::newRow("accents and stray spaces") << "  Moteur à cage " << "Moteur à cage";
		QTest::newRow("non-Latin")   << "Двигатель 電機" << "Двигатель 電機";
	}

	void bothReadersAgree()
	{
		QFETCH(QString, value);
		QFETCH(QString, expected);
		QCOMPARE(fromDom(xml(value)), expected);
		QCOMPARE(fromPugi(xml(value)), expected);
	}
};

QTEST_APPLESS_MAIN(tst_diagramcontext)

#include "tst_diagramcontext.moc"
