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
#include "createdxf.h"
#include "dxfexport.h"
#include "exportproperties.h"
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

			/// Where the DXF TEXT of m_text starts (DXF units, y up) and
			/// its angle (degrees, anticlockwise)
		QPair<QPointF, double> dxfText()
		{
			QTemporaryDir dir;
			const QString path = dir.filePath(QStringLiteral("folio.dxf"));
			Diagram *diagram = m_element->diagram();
			ExportProperties properties;
			properties.dxf_blocks = false;
			properties.dxf_attributes = false;
			DxfExport::write(diagram, 1000, 1000, path, properties);
			QFile file(path);
			if (!file.open(QIODevice::ReadOnly))
				return {};
			QStringList v = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
			for (QString &s : v) s = s.trimmed();
			const int i = int(v.indexOf(m_text->toPlainText()));
			if (i < 1 || v.at(i - 1) != QLatin1String("1"))
				return {};
			int start = i;
			while (start > 0 && v.at(start) != QLatin1String("TEXT")) --start;
			QPointF at;
			double angle = 0;
			for (int j = start; j + 1 < v.size() && j < i + 12; j += 1) {
				if (v.at(j) == QLatin1String("10")) at.setX(v.at(j + 1).toDouble());
				if (v.at(j) == QLatin1String("20")) at.setY(v.at(j + 1).toDouble());
				if (j > i && v.at(j) == QLatin1String("50")) { angle = v.at(j + 1).toDouble(); break; }
			}
			return {at, angle};
		}

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
			QVERIFY(flipped());
			const QPointF c = m_text->boundingRect().center();
			QCOMPARE(m_text->transform().map(c), c);
			DynamicElementTextItem::setVerticalFromBottom(false);
		}

			//A text given to a symbol already turned, as when a folio is
			//opened: turned as well
		void addedToATurnedSymbol()
		{
			DynamicElementTextItem::setVerticalFromBottom(true);
			m_element->setRotation(90);
			auto *text = new DynamicElementTextItem(m_element);
			text->setText(QStringLiteral("-X2:3"));
			text->setKeepVisualRotation(false);
			m_element->addDynamicTextItem(text);
			QVERIFY(!text->transform().isIdentity());
			text->setText(QStringLiteral("-X2:3 +UV1/7"));
			QVERIFY(!text->transform().isIdentity());
			m_element->removeDynamicTextItem(text);
			delete text;
			DynamicElementTextItem::setVerticalFromBottom(false);
		}

			//The DXF export writes a turned text as it is drawn: from its
			//other end, reading the other way
		void dxfReadsTheSameWay()
		{
			m_text->setText(QStringLiteral("-X1:12"));
			m_element->setRotation(90);
			const auto down = dxfText();
			DynamicElementTextItem::setVerticalFromBottom(true);
			QVERIFY(flipped());
			const auto up = dxfText();
			DynamicElementTextItem::setVerticalFromBottom(false);
			QCOMPARE(std::fmod(up.second - down.second + 360, 360), 180.0);
			QCOMPARE(up.second, 90.0);
				//Half a turn about the middle of the box, as drawn
			const QRectF box = m_text->mapRectToScene(m_text->boundingRect());
			const QPointF centre(box.center().x() * Createdxf::xScale,
								 Createdxf::sheetHeight - box.center().y() * Createdxf::yScale);
			const QPointF mirrored = 2 * centre - down.first;
			QVERIFY2(QLineF(up.first, mirrored).length() < 0.5,
					 qPrintable(QStringLiteral("%1,%2 / %3,%4").arg(up.first.x()).arg(up.first.y())
								.arg(mirrored.x()).arg(mirrored.y())));
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
