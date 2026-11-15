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
// SPDX-License-Identifier: GPL-2.0-or-later

#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "diagram.h"
#include "diagramcontent.h"
#include "qetproject.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/terminal.h"
#include "ui/dialogwaiting.h"
#include "undocommand/changeelementinformationcommand.h"
#include "undocommand/deleteqgraphicsitemcommand.h"
#include "undocommand/linkelementcommand.h"
#include <QDomDocument>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class tst_reportendpoint : public QObject
{
	Q_OBJECT

	static Element *symbol(Diagram *folio, const QString &kind)
	{
		QDomDocument doc;
		const QString name = QUuid::createUuid().toString(QUuid::Id128);
		doc.setContent(QString(
			R"(<definition type="element" link_type="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test symbol</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal name="A1" x="0" y="-20" orientation="n" uuid="%2"/>
<terminal name="A2" x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)").arg(kind, QUuid::createUuid().toString(),
				QUuid::createUuid().toString()));
		if (!folio->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), name, doc.documentElement()))
			return nullptr;
		int state = 0;
		auto *element = ElementFactory::Instance()->createElement(
				ElementsLocation("embed://import/" + name + ".elmt",
					folio->project()), nullptr, &state);
		if (state || !element) return nullptr;
		folio->addItem(element);
		return element;
	}

	static DynamicElementTextItem *field(Element *element, bool composite)
	{
		auto *text = new DynamicElementTextItem(element);
		element->addDynamicTextItem(text);
		text->setTextFrom(composite ? DynamicElementTextItem::CompositeText
								   : DynamicElementTextItem::ElementInfo);
		if (composite)
			text->setCompositeText(QStringLiteral("To %{remote_endpoint}"));
		else
			text->setInfoName(QStringLiteral("remote_endpoint"));
		return text;
	}

	static Terminal *pin(Element *element, const QString &name)
	{
		for (Terminal *terminal : element->terminals())
			if (terminal->name() == name) return terminal;
		return nullptr;
	}

private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }

	void opensWithProgressDialog_data()
	{
		QTest::addColumn<QString>("file");
		QTest::addColumn<QByteArray>("eager");
		QTest::newRow("fixture") << QFINDTESTDATA("fixtures/every_feature.qet")
								<< QByteArray("0");
		QTest::newRow("industrial") << QFINDTESTDATA("../../examples/industrial.qet")
								   << QByteArray("1");
		QTest::newRow("m_000") << QFINDTESTDATA("../../examples/m_000.qet")
							  << QByteArray("1");
	}

	void opensWithProgressDialog()
	{
		QFETCH(QString, file);
		QFETCH(QByteArray, eager);
		QVERIFY(!file.isEmpty());
		const QByteArray previous = qgetenv("QET_EAGER_FOLIOS");
		qputenv("QET_EAGER_FOLIOS", eager);
		DialogWaiting::instance()->show();
		// The progress dialog processes events inside the constructor.
		QETProject project(file);
		DialogWaiting::dropInstance();
		if (previous.isNull()) {
			qunsetenv("QET_EAGER_FOLIOS");
		} else {
			qputenv("QET_EAGER_FOLIOS", previous);
		}
		QCOMPARE(project.state(), QETProject::Ok);
		QVERIFY(project.undoStack());
	}

	void readsDestinationWithoutBuildingItsFolio()
	{
		QETProject project;
		auto *first = project.addNewDiagram();
		auto *second = project.addNewDiagram();
		auto *source = symbol(first, "simple");
		auto *target = symbol(second, "simple");
		auto *out = symbol(first, "next_report");
		auto *in = symbol(second, "previous_report");
		QVERIFY(source && target && out && in);
		auto info = target->elementInformations();
		info.addValue("label", "KM2");
		target->setElementInformations(info);
		first->addItem(new Conductor(pin(source, "A1"),
									 out->terminals().first()));
		second->addItem(new Conductor(in->terminals().first(),
									  pin(target, "A1")));
		out->linkToElement(in);
		auto *text = field(out, false);
		text->setPlainText(QString());

		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		QFile file(directory.filePath("endpoint.qet"));
		QVERIFY(file.open(QIODevice::WriteOnly));
		const auto xml = project.toXml().toByteArray();
		QCOMPARE(file.write(xml), xml.size());
		file.close();

		const QByteArray previous = qgetenv("QET_EAGER_FOLIOS");
		qputenv("QET_EAGER_FOLIOS", "0");
		QETProject reopened(file.fileName());
		if (previous.isNull()) {
			qunsetenv("QET_EAGER_FOLIOS");
		} else {
			qputenv("QET_EAGER_FOLIOS", previous);
		}
		QCOMPARE(reopened.state(), QETProject::Ok);
		QCOMPARE(reopened.folios().size(), 2);
		auto *loaded = reopened.folios().first();
		auto *remote = reopened.folios().last();
		reopened.buildFolios({loaded});
		QVERIFY(!remote->isLoaded());
		for (auto *element : loaded->elements()) {
			if (element->uuid() != out->uuid()) continue;
			QCOMPARE(element->dynamicTextItems().size(), 1);
			QTRY_COMPARE(element->dynamicTextItems().first()->toPlainText(),
						 QStringLiteral("KM2:A1"));
			QVERIFY(!remote->isLoaded());
			return;
		}
		QFAIL("The source report was not loaded");
	}

	void followsEditsAndUndo()
	{
		QETProject project;
		auto *first = project.addNewDiagram();
		auto *second = project.addNewDiagram();
		auto *source = symbol(first, "simple");
		auto *target = symbol(second, "simple");
		auto *out = symbol(first, "next_report");
		auto *in = symbol(second, "previous_report");
		QVERIFY(source && target && out && in);
		auto info = target->elementInformations();
		info.addValue("label", "KM2");
		target->setElementInformations(info);
		first->addItem(new Conductor(pin(source, "A1"), out->terminals().first()));
		auto *wire = new Conductor(in->terminals().first(), pin(target, "A1"));
		second->addItem(wire);
		out->linkToElement(in);
		auto *text = field(out, false);
		auto *composite = field(out, true);
		QTRY_COMPARE(text->toPlainText(), QString("KM2:A1"));
		QTRY_COMPARE(composite->toPlainText(), QString("To KM2:A1"));

		auto renamed = info;
		renamed.addValue("label", "KM20");
		project.undoStack()->push(new ChangeElementInformationCommand(target, info, renamed));
		QTRY_COMPARE(text->toPlainText(), QString("KM20:A1"));
		QTRY_COMPARE(composite->toPlainText(), QString("To KM20:A1"));
		project.undoStack()->undo();
		QTRY_COMPARE(text->toPlainText(), QString("KM2:A1"));
		project.undoStack()->redo();
		QTRY_COMPARE(text->toPlainText(), QString("KM20:A1"));

		DiagramContent content;
		content.m_conductors_to_move.append(wire);
		project.undoStack()->push(new DeleteQGraphicsItemCommand(second, content));
		QTRY_COMPARE(text->toPlainText(), QString());
		project.undoStack()->undo();
		QTRY_COMPARE(text->toPlainText(), QString("KM20:A1"));

		auto *unlink = new LinkElementCommand(out);
		unlink->unlinkAll();
		project.undoStack()->push(unlink);
		QTRY_COMPARE(text->toPlainText(), QString());
		project.undoStack()->undo();
		QTRY_COMPARE(text->toPlainText(), QString("KM20:A1"));

		// A second wire on the remote arrow makes its endpoint ambiguous.
		auto *extra = new Conductor(in->terminals().first(), pin(target, "A2"));
		second->addItem(extra);
		QTRY_COMPARE(text->toPlainText(), QString());
		DiagramContent extra_content;
		extra_content.m_conductors_to_move.append(extra);
		project.undoStack()->push(new DeleteQGraphicsItemCommand(second, extra_content));
		QTRY_COMPARE(text->toPlainText(), QString("KM20:A1"));

		// Reconnect to the other pin.
		project.undoStack()->push(new DeleteQGraphicsItemCommand(second, content));
		auto *replacement = new Conductor(in->terminals().first(), pin(target, "A2"));
		second->addItem(replacement);
		QTRY_COMPARE(text->toPlainText(), QString("KM20:A2"));

		// The endpoint also follows a composite that includes the folio.
		composite->setCompositeText("%{label} / %{remote_endpoint}");
		QVERIFY(composite->toPlainText().endsWith(" / KM20:A2"));
		in->setPos(400, 300);
		QTRY_VERIFY(composite->toPlainText().endsWith(" / KM20:A2"));
		QVERIFY(!composite->toPlainText().contains("%{"));
		text->setTextFrom(DynamicElementTextItem::UserText);
		text->setTextFrom(DynamicElementTextItem::ElementInfo);
		QCOMPARE(text->toPlainText(), QString("KM20:A2"));
	}
};

int main(int argc, char **argv)
{
	return QetAppTest::run<tst_reportendpoint>(argc, argv);
}
#include "tst_reportendpoint.moc"
