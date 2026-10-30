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
/*
	Upright symbol texts read from bottom to top (a setting, off by
	default): a symbol's text that its symbol's turn makes read from top
	to bottom is turned half a turn about the middle of its box, which
	keeps its place; nothing else changes, and nothing is saved.
*/
#include "qetapptest.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetmessagebox.h"

class TstUprightTexts : public QObject
{
	Q_OBJECT

	private:
		Element *m_element = nullptr;
		DynamicElementTextItem *m_text = nullptr;

		bool flipped() const { return !m_text->transform().isIdentity(); }

	private slots:
		void initTestCase()
		{
			QET::QetMessageBox::setNonInteractive(true);
			Diagram *diagram = QetAppTest::diagram();
			QVERIFY(diagram);
			const ElementsLocation location(
				QStringLiteral("common://10_electric/10_allpole/114_connections/corner.elmt"));
			int state = 0;
			m_element = ElementFactory::Instance()->createElement(location, nullptr, &state);
			QVERIFY(m_element);
			m_element->setPos(200, 200);
			m_text = new DynamicElementTextItem(m_element);
			m_text->setText(QStringLiteral("-X1:12"));
				//As a symbol's own texts usually are: turned with it
			m_text->setKeepVisualRotation(false);
			m_element->addDynamicTextItem(m_text);
			diagram->addItem(m_element);
		}

			//Off by default: the text turns with its symbol, as before
		void offByDefault()
		{
			QVERIFY(!DynamicElementTextItem::verticalFromBottom());
			m_element->setRotation(90);
			QVERIFY(!flipped());
		}

		void readingDownIsTurned()
		{
			DynamicElementTextItem::setVerticalFromBottom(true);
			m_element->setRotation(90);
			QVERIFY(flipped());
				//In the same place: half a turn about the middle of its box
			const QRectF box = m_text->mapRectToScene(m_text->boundingRect());
			m_element->setRotation(0);
			m_element->setRotation(90);
			QCOMPARE(m_text->mapRectToScene(m_text->boundingRect()), box);
		}

			//Read from bottom to top already, or level: left as it is
		void otherTurnsLeftAlone()
		{
			for (int angle : {0, 180, 270}) {
				m_element->setRotation(angle);
				QVERIFY2(!flipped(), qPrintable(QString::number(angle)));
			}
		}

			//Switched off, every open folio's texts turn back at once
		void switchingOffTurnsBack()
		{
			m_element->setRotation(90);
			QVERIFY(flipped());
			DynamicElementTextItem::setVerticalFromBottom(false);
			QVERIFY(!flipped());
		}

			//A longer text: still turned about its new middle
		void followsTheText()
		{
			DynamicElementTextItem::setVerticalFromBottom(true);
			m_element->setRotation(90);
			m_text->setText(QStringLiteral("-X1:12 and more"));
			const QPointF c = m_text->boundingRect().center();
			QCOMPARE(m_text->transform().map(c), c);
			DynamicElementTextItem::setVerticalFromBottom(false);
		}
};

	//As QET_APP_TEST_MAIN, with the source tree's common collection
int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("QElectroTech");
	QCoreApplication::setApplicationName("QElectroTech");
	QTemporaryDir home;
	if (!home.isValid())
		return 1;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.filePath("settings"));
	QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.filePath("settings"));
	QETApp::applyDirectoryArguments(QETArguments(QList<QString>{
		"--config-dir=" + home.filePath("config"),
		"--data-dir=" + home.filePath("data"),
		QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/")}));
	QETApp qetapp;
	QCoreApplication::processEvents();
	TstUprightTexts test;
	return QTest::qExec(&test, argc, argv);
}

#include "tst_uprighttexts.moc"
