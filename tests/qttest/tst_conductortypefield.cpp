// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDomDocument>

#include "conductorproperties.h"
#include "qetinformation.h"
#include "qetapp.h"

	// qet.cpp needs it; the application is not linked
QString QETApp::m_interface_language;

// The conductor type of a wire (H05V-K, H07V-K ...): a free text field
// in the conductor's properties, written only when it is set, read back
// the way it was written, and offered wherever the other conductor
// information is offered -- the dynamic text fields of the element
// editor above all.
class tst_conductortypefield : public QObject
{
	Q_OBJECT

	private slots:
		void nothingIsWrittenWithoutAType()
		{
			ConductorProperties properties;
			QDomDocument document;
			QDomElement element = document.createElement(QStringLiteral("conductor"));
			document.appendChild(element);
			properties.toXml(element);
			QVERIFY2(!element.hasAttribute(QStringLiteral("conductor_type")),
					 "a conductor without a type writes no attribute");
		}

		void theTypeComesBack()
		{
			ConductorProperties properties;
			properties.m_conductor_type = QStringLiteral("H05V-K");
			QDomDocument document;
			QDomElement element = document.createElement(QStringLiteral("conductor"));
			document.appendChild(element);
			properties.toXml(element);
			QCOMPARE(element.attribute(QStringLiteral("conductor_type")),
					 QStringLiteral("H05V-K"));

			ConductorProperties read_back;
			read_back.fromXml(element);
			QCOMPARE(read_back.m_conductor_type, QStringLiteral("H05V-K"));
		}

		void aFileWithoutTheFieldReadsAsNoType()
		{
			//A file written before this field existed: no attribute,
			//and everything else still read as it was
			QDomDocument document;
			QDomElement element = document.createElement(QStringLiteral("conductor"));
			element.setAttribute(QStringLiteral("conductor_section"),
								QStringLiteral("1.5"));
			document.appendChild(element);
			ConductorProperties read_back;
			read_back.fromXml(element);
			QVERIFY(read_back.m_conductor_type.isEmpty());
			QCOMPARE(read_back.m_wire_section, QStringLiteral("1.5"));
		}

		void theChoiceIsInTheDynamicTextFieldList()
		{
			//The element editor offers the conductor information in its
			//dynamic text fields: the type has to be among them
			QVERIFY(QETInformation::folioReportInfoKeys().contains(
						QETInformation::COND_TYPE));
			QVERIFY(QETInformation::conductorInfoKeys().contains(
						QETInformation::COND_TYPE));
		}
};

QTEST_GUILESS_MAIN(tst_conductortypefield)
#include "tst_conductortypefield.moc"
