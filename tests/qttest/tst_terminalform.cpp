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
#include "qetapptest.h"

#include <QDomDocument>
#include <QtTest>

#include "properties/elementdata.h"

/**
	The terminal form (what a terminal is built as: a through terminal,
	a multi-level one...) is a second choice beside the function type,
	kept apart so that symbols written before the list existed load and
	look as they did.
*/
class tst_terminalform : public QObject
{
	Q_OBJECT

	private slots:
		void initTestCase();
		void aFormIsUnsetUntilOneIsChosen();
		void theFormSurvivesASaveAndAReload();
		void anOldFileWithoutAFormStaysUnset();
		void aFormOtherThanTheSymbolsBecomesAnOverride();
		void theFormTakesPartInTheEqualityCheck();
		void cleanupTestCase();
};

void tst_terminalform::initTestCase()
{
}

/**
	A terminal which was never given a form carries none, and choosing
	one is remembered.
*/
void tst_terminalform::aFormIsUnsetUntilOneIsChosen()
{
	ElementData data;
	data.m_type = ElementData::Terminal;
	QCOMPARE(data.terminalForm(), ElementData::TFormNone);
	QVERIFY(!data.terminalFormIsOverrided());

	data.setTerminalForm(ElementData::TFormMultiLevel);
	QCOMPARE(data.terminalForm(), ElementData::TFormMultiLevel);
}

/**
	The form of the symbol survives a save and a reload through the
	definition of a terminal symbol, like the type beside it.
*/
void tst_terminalform::theFormSurvivesASaveAndAReload()
{
	ElementData data;
	data.m_type = ElementData::Terminal;
	data.m_terminal_form = ElementData::TFormFeedThrough3;

	QDomDocument document;
	QDomElement root = document.createElement(QStringLiteral("definition"));
	root.setAttribute(QStringLiteral("type"), QStringLiteral("element"));
	root.setAttribute(QStringLiteral("link_type"), QStringLiteral("terminal"));
	root.appendChild(data.kindInfoToXml(document));
	document.appendChild(root);

	ElementData reloaded;
	QVERIFY(reloaded.fromXml(root));
	QCOMPARE(reloaded.m_type, ElementData::Terminal);
	QCOMPARE(reloaded.terminalForm(), ElementData::TFormFeedThrough3);
}

/**
	A file written before the form existed has no form in it: it loads
	without one, and the name does not land among the kind informations
	the class keeps for names it does not know.
*/
void tst_terminalform::anOldFileWithoutAFormStaysUnset()
{
	ElementData data;
	data.m_type = ElementData::Terminal;

	QDomDocument document;
	QDomElement root = document.createElement(QStringLiteral("definition"));
	root.setAttribute(QStringLiteral("type"), QStringLiteral("element"));
	root.setAttribute(QStringLiteral("link_type"), QStringLiteral("terminal"));
	root.appendChild(data.kindInfoToXml(document));
	document.appendChild(root);

	QDomNodeList nodes = document.elementsByTagName(QStringLiteral("kindInformation"));
	for (int i = 0 ; i < nodes.count() ; ++i) {
		QDomElement node = nodes.at(i).toElement();
		if (node.attribute(QStringLiteral("name")) == QLatin1String("form")) {
			node.parentNode().removeChild(node);
			break;
		}
	}

	ElementData reloaded;
	QVERIFY(reloaded.fromXml(root));
	QCOMPARE(reloaded.terminalForm(), ElementData::TFormNone);
	QVERIFY(reloaded.m_other_kind_informations.isEmpty());
}

/**
	A form given to a placed terminal is an override of the one of the
	symbol it comes from, and giving it back the form of the symbol
	brings the override away.
*/
void tst_terminalform::aFormOtherThanTheSymbolsBecomesAnOverride()
{
	ElementData data;
	data.m_type = ElementData::Terminal;
	data.m_terminal_form = ElementData::TFormFeedThrough2;

	data.setTerminalForm(ElementData::TFormMultiLevel);
	QCOMPARE(data.terminalForm(), ElementData::TFormMultiLevel);
	QVERIFY(data.terminalFormIsOverrided());

	data.setTerminalForm(ElementData::TFormFeedThrough2);
	QCOMPARE(data.terminalForm(), ElementData::TFormFeedThrough2);
	QVERIFY(!data.terminalFormIsOverrided());
}

/**
	Two terminal data which differ only in their form are not the same.
*/
void tst_terminalform::theFormTakesPartInTheEqualityCheck()
{
	ElementData first, second;
	first.m_type = ElementData::Terminal;
	second.m_type = ElementData::Terminal;
	QCOMPARE(first, second);

	first.m_terminal_form = ElementData::TFormFuse;
	QVERIFY(first != second);

	second.m_terminal_form = ElementData::TFormFuse;
	QCOMPARE(first, second);
}

void tst_terminalform::cleanupTestCase()
{
}

QET_APP_TEST_MAIN(tst_terminalform)
#include "tst_terminalform.moc"
