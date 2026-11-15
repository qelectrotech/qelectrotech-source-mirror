// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/xmlelementcollection.h"
#include "devicetype/devicetype.h"
#include "qetapp.h"
#include "qetapptest.h"
#include "qetproject.h"
#include "ui/devicetypeeditor.h"
#include "ui/devicetypesdialog.h"

#include <QDomDocument>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest>

/**
	Project > Device types: the list of the project's and the library's
	device types (DeviceTypesDialog) and the editor of one type
	(DeviceTypeEditor).
*/
class tst_devicetypeeditor : public QObject
{
	Q_OBJECT

		//An element import/<file>.elmt of the project, with terminals @p pins
	static bool define(QETProject *project, const QString &file, const QStringList &pins)
	{
		QString terminals;
		for (int i = 0 ; i < pins.size() ; ++i)
			terminals += QStringLiteral("<terminal x=\"%1\" y=\"20\" orientation=\"s\" name=\"%2\" uuid=\"%3\"/>")
					.arg(10 * i).arg(pins.at(i), QUuid::createUuid().toString());
		QDomDocument definition;
		if (!definition.setContent(QString(
				R"(<definition type="element" link_type="master" uuid="%1" width="60" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">%2</name></names>
<kindInformations><kindInformation name="type">coil</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>%3</description></definition>)")
				.arg(QUuid::createUuid().toString(), file, terminals)))
			return false;
		return project->embeddedElementCollection()->addElementDefinition(
					QStringLiteral("import"), file, definition.documentElement());
	}

		//A relay: a coil, a contact drawn as NO, NC or changeover, and a
		//part with no symbol yet
	static DeviceType relay()
	{
		DeviceType type;
		type.name = QStringLiteral("RELAY02");
		type.names.insert(QETApp::langFromSetting(), QStringLiteral("RELAY, 1 CONTACT"));
		type.informations << qMakePair(QStringLiteral("description"), QStringLiteral("RELAY, 1 CONTACT"));
		DeviceType::Part coil{QStringLiteral("master"), QStringLiteral("coil"), QString(),
			{{QStringLiteral("embed://import/coil.elmt"), {QStringLiteral("A1"), QStringLiteral("A2")}, {}}}};
		DeviceType::Part contact{QStringLiteral("slave"), QStringLiteral("simple"), QString(),
			{{QStringLiteral("embed://import/no.elmt"), {QStringLiteral("13"), QStringLiteral("14")}, QStringLiteral("NO")},
			 {QStringLiteral("embed://import/nc.elmt"), {QStringLiteral("11"), QStringLiteral("12")}, QStringLiteral("NC")},
			 {QStringLiteral("embed://import/sw.elmt"), {QStringLiteral("11"), QStringLiteral("12"), QStringLiteral("14")}, QStringLiteral("SW")}}};
		DeviceType::Part spare{QStringLiteral("slave"), QStringLiteral("simple"), QStringLiteral("NO"), {}};
		type.parts = {coil, contact, spare};
		return type;
	}

  private slots:
	void initTestCase() {QETProject::setBackupEnabled(false);}

	void editorShowsAndGivesBackTheType()
	{
		const DeviceType type = relay();
		DeviceTypeEditor editor(type, nullptr, QStringList());
		QCOMPARE(editor.type(), type);
			//An existing type keeps its name
		QVERIFY(editor.findChild<QLineEdit *>(QStringLiteral("deviceTypeName"))->isReadOnly());

		DeviceTypeEditor fresh(DeviceType(), nullptr, QStringList());
		QVERIFY(!fresh.findChild<QLineEdit *>(QStringLiteral("deviceTypeName"))->isReadOnly());
		QVERIFY(fresh.type().parts.isEmpty());
	}

	void problemsAreNamed()
	{
		QETProject project;
		QVERIFY(define(&project, QStringLiteral("coil"), {QStringLiteral("A1"), QStringLiteral("A2")}));

		DeviceType type;
		QStringList found = DeviceTypeEditor::problems(type, &project, {});
		QVERIFY(found.join(QLatin1Char('|')).contains(QStringLiteral("name")));
		QVERIFY(found.join(QLatin1Char('|')).contains(QStringLiteral("at least one part")));

		type.name = QStringLiteral("CONTR10");
		type.parts << DeviceType::Part{QStringLiteral("master"), QStringLiteral("coil"), QString(),
			{{QStringLiteral("embed://import/coil.elmt"), {QStringLiteral("A1"), QStringLiteral("A2")}, {}}}};
		QVERIFY(DeviceTypeEditor::problems(type, &project, {}).isEmpty());
		QCOMPARE(DeviceTypeEditor::problems(type, &project, {QStringLiteral("CONTR10")}).size(), 1);

		type.parts[0].symbols[0].pins << QStringLiteral("A3");
		found = DeviceTypeEditor::problems(type, &project, {});
		QCOMPARE(found.size(), 1);
		QVERIFY2(found.first().contains(QStringLiteral("3 pins")), qPrintable(found.first()));

		type.parts[0].symbols[0].location = QStringLiteral("embed://import/nothing.elmt");
		found = DeviceTypeEditor::problems(type, &project, {});
		QVERIFY2(found.first().contains(QStringLiteral("cannot be found")), qPrintable(found.first()));

		int count = 0;
		QCOMPARE(DeviceTypeEditor::terminalNames(QStringLiteral("embed://import/coil.elmt"), &project, &count),
				 QStringList({QStringLiteral("A1"), QStringLiteral("A2")}));
		QCOMPARE(count, 2);
	}

	void projectTypesAreUndoable()
	{
		QETProject project;
		QTemporaryDir library;
		QVERIFY(library.isValid());
		DeviceTypesDialog dialog(&project, nullptr, library.path());
		QUndoStack *undo = project.undoStack();
		const int steps = undo->count();

		DeviceType type = relay();
		QVERIFY(dialog.keep(type, false));
		QCOMPARE(project.deviceType(type.name), type);
		QCOMPARE(undo->count(), steps + 1);

		DeviceType changed = type;
		changed.parts.removeLast();
		QVERIFY(dialog.keep(changed, false));
		QCOMPARE(project.deviceType(type.name), changed);
		undo->undo();
		QCOMPARE(project.deviceType(type.name), type);

		QVERIFY(dialog.remove(type.name, false));
		QVERIFY(project.deviceType(type.name).isNull());
		undo->undo();
		QCOMPARE(project.deviceType(type.name), type);
		undo->undo();
		QVERIFY(project.deviceType(type.name).isNull());
	}

	void copiedBetweenProjectAndLibrary()
	{
		QETProject project;
		QTemporaryDir library;
		QVERIFY(library.isValid());
		DeviceTypesDialog dialog(&project, nullptr, library.path());
		const DeviceType type = relay();
		QVERIFY(dialog.keep(type, false));

		QVERIFY(dialog.copy(type.name, true));
		QCOMPARE(DeviceTypeLibrary::load(type.name, library.path()), type);

		QVERIFY(dialog.remove(type.name, false));
		QVERIFY(project.deviceType(type.name).isNull());
		QVERIFY(dialog.copy(type.name, false));
		QCOMPARE(project.deviceType(type.name), type);

		QVERIFY(dialog.remove(type.name, true));
		QVERIFY(!DeviceTypeLibrary::names(library.path()).contains(type.name));
	}

	void duplicateTakesAFreeName()
	{
		QCOMPARE(DeviceTypesDialog::freeName(QStringLiteral("CONTR10"), {}), QStringLiteral("CONTR10"));
		QCOMPARE(DeviceTypesDialog::freeName(QStringLiteral("CONTR10"),
											 {QStringLiteral("CONTR10"), QStringLiteral("CONTR10 2")}),
				 QStringLiteral("CONTR10 3"));
		DeviceType copy = relay();
		copy.name = QStringLiteral("RELAY02 2");
		DeviceTypeEditor editor(copy, nullptr, {QStringLiteral("RELAY02")});
		editor.setNameEditable(true);
		auto *name = editor.findChild<QLineEdit *>(QStringLiteral("deviceTypeName"));
		QVERIFY(!name->isReadOnly());
		name->setText(QStringLiteral("RELAY03"));
		QCOMPARE(editor.type().name, QStringLiteral("RELAY03"));
		QCOMPARE(editor.type().parts, relay().parts);
	}
};

QET_APP_TEST_MAIN(tst_devicetypeeditor)
#include "tst_devicetypeeditor.moc"
