/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include <QtTest>

#include "contactcrossrefprojectionservice.h"
#include "qetmessagebox.h"
#include "qetproject.h"

#include <QApplication>
#include <QDomImplementation>
#include <QFile>
#include <QTemporaryDir>

namespace {

const QUuid kMasterUuid(QStringLiteral("{11111111-1111-4111-8111-111111111111}"));
const QUuid kNoSlaveUuid(QStringLiteral("{22222222-2222-4222-8222-222222222222}"));
const QUuid kNcSlaveUuid(QStringLiteral("{33333333-3333-4333-8333-333333333333}"));

QString fixturePath()
{
	return QFINDTESTDATA("fixtures/master_slave_links_group_index_minimal.qet");
}

QString readTextFile(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning().noquote() << QStringLiteral("cannot read %1").arg(path);
		return {};
	}
	return QString::fromUtf8(file.readAll());
}

QString writeVariant(QTemporaryDir &dir, const QString &file_name, const QString &content)
{
	const QString path = dir.filePath(file_name);
	QFile file(path);
	if (!dir.isValid() || !file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
		qWarning().noquote() << QStringLiteral("cannot write %1").arg(path);
		return {};
	}
	if (file.write(content.toUtf8()) <= 0) {
		qWarning().noquote() << QStringLiteral("empty write for %1").arg(path);
		return {};
	}
	file.close();
	return path;
}

const ContactAssignmentProjection *assignmentFor(
	const QList<ContactAssignmentProjection> &assignments,
	const QUuid &slave_uuid)
{
	for (const ContactAssignmentProjection &assignment : assignments) {
		if (assignment.slave_uuid == slave_uuid) {
			return &assignment;
		}
	}
	return nullptr;
}

void initHeadlessProjectLoad()
{
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QDomImplementation::setInvalidDataPolicy(QDomImplementation::ReturnNullNode);
}

} // namespace

class tst_contactcrossrefprojectionservice : public QObject
{
	Q_OBJECT

private slots:
	void fixtureProjectProjectsContactGroupsAndAssignments()
	{
		initHeadlessProjectLoad();

		const QString fixture = fixturePath();
		QVERIFY2(!fixture.isEmpty(), "master/slave projection fixture project not found");
		QVERIFY2(QFile::exists(fixture), qPrintable(QStringLiteral("fixture not found: %1").arg(fixture)));

		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		ContactCrossRefProjectionService service;
		const QList<ContactMasterProjection> masters = service.masters(project);
		QCOMPARE(masters.size(), 1);

		const ContactMasterProjection master = masters.first();
		QCOMPARE(master.uuid, kMasterUuid);
		QCOMPARE(master.label, QStringLiteral("KMS"));
		QCOMPARE(master.folio, 0);
		QCOMPARE(master.groups.size(), 2);
		QCOMPARE(master.linked_slave_count, 2);
		QVERIFY2(master.validation_messages.isEmpty(), qPrintable(master.validation_messages.join(QStringLiteral("; "))));

		QCOMPARE(master.groups.at(0).index, 0);
		QCOMPARE(master.groups.at(0).type, ElementData::NO);
		QCOMPARE(master.groups.at(0).contact_count, 1);
		QCOMPARE(master.groups.at(0).terminal_count, 2);
		QCOMPARE(master.groups.at(0).terminal_labels, QStringList({QStringLiteral("13"), QStringLiteral("14")}));

		QCOMPARE(master.groups.at(1).index, 1);
		QCOMPARE(master.groups.at(1).type, ElementData::NC);
		QCOMPARE(master.groups.at(1).contact_count, 1);
		QCOMPARE(master.groups.at(1).terminal_count, 2);
		QCOMPARE(master.groups.at(1).terminal_labels, QStringList({QStringLiteral("21"), QStringLiteral("22")}));

		QCOMPARE(master.usage.no, 1);
		QCOMPARE(master.usage.nc, 1);
		QCOMPARE(master.usage.total(), 2);
		QCOMPARE(master.capacity.no, 1);
		QCOMPARE(master.capacity.nc, 1);
		QCOMPARE(master.capacity.total(), 2);

		const QList<ContactAssignmentProjection> assignments = service.assignments(project);
		QCOMPARE(assignments.size(), 2);

		const ContactAssignmentProjection *no_assignment = assignmentFor(assignments, kNoSlaveUuid);
		QVERIFY(no_assignment);
		QCOMPARE(no_assignment->master_uuid, kMasterUuid);
		QCOMPARE(no_assignment->master_label, QStringLiteral("KMS"));
		QCOMPARE(no_assignment->slave_label, QStringLiteral("KMS-NO"));
		QCOMPARE(no_assignment->group_index, 0);
		QVERIFY(no_assignment->group_index_resolves);
		QCOMPARE(no_assignment->slave_contact_type, ElementData::NO);
		QCOMPARE(no_assignment->slave_contact_count, 1);
		QCOMPARE(no_assignment->group.terminal_labels, QStringList({QStringLiteral("13"), QStringLiteral("14")}));
		QVERIFY2(no_assignment->validation_messages.isEmpty(),
				 qPrintable(no_assignment->validation_messages.join(QStringLiteral("; "))));

		const ContactAssignmentProjection *nc_assignment = assignmentFor(assignments, kNcSlaveUuid);
		QVERIFY(nc_assignment);
		QCOMPARE(nc_assignment->master_uuid, kMasterUuid);
		QCOMPARE(nc_assignment->master_label, QStringLiteral("KMS"));
		QCOMPARE(nc_assignment->slave_label, QStringLiteral("KMS-NC"));
		QCOMPARE(nc_assignment->group_index, 1);
		QVERIFY(nc_assignment->group_index_resolves);
		QCOMPARE(nc_assignment->slave_contact_type, ElementData::NC);
		QCOMPARE(nc_assignment->slave_contact_count, 1);
		QCOMPARE(nc_assignment->group.terminal_labels, QStringList({QStringLiteral("21"), QStringLiteral("22")}));
		QVERIFY2(nc_assignment->validation_messages.isEmpty(),
				 qPrintable(nc_assignment->validation_messages.join(QStringLiteral("; "))));
	}

	void invalidGroupIndexVariantsProduceValidationMessages()
	{
		initHeadlessProjectLoad();

		const QString fixture = fixturePath();
		QVERIFY2(!fixture.isEmpty(), "master/slave projection fixture project not found");
		QVERIFY2(QFile::exists(fixture), qPrintable(QStringLiteral("fixture not found: %1").arg(fixture)));
		const QString original = readTextFile(fixture);
		QVERIFY2(!original.isEmpty(), "fixture content not readable");
		const QString nc_link = QStringLiteral(
			"<link_uuid uuid=\"{33333333-3333-4333-8333-333333333333}\" group_index=\"1\"/>");

		QTemporaryDir dir;
		QVERIFY(dir.isValid());

		struct Variant {
			QString name;
			QString replacement;
			QUuid slave_uuid;
			int expected_group_index;
			bool expected_resolves;
			QStringList expected_messages;
		};

		const QList<Variant> variants {
			{
				QStringLiteral("missing_group_index.qet"),
				QStringLiteral("<link_uuid uuid=\"{33333333-3333-4333-8333-333333333333}\"/>"),
				kNcSlaveUuid,
				-1,
				false,
				{
					QStringLiteral("missing group_index"),
					QStringLiteral("linked slave {33333333-3333-4333-8333-333333333333} has no group_index")
				}
			},
			{
				QStringLiteral("out_of_range_group_index.qet"),
				QStringLiteral("<link_uuid uuid=\"{33333333-3333-4333-8333-333333333333}\" group_index=\"99\"/>"),
				kNcSlaveUuid,
				99,
				false,
				{
					QStringLiteral("out-of-range group_index 99"),
					QStringLiteral("linked slave {33333333-3333-4333-8333-333333333333} references out-of-range group_index 99")
				}
			},
			{
				QStringLiteral("type_mismatch_group_index.qet"),
				QStringLiteral("<link_uuid uuid=\"{33333333-3333-4333-8333-333333333333}\" group_index=\"0\"/>"),
				kNcSlaveUuid,
				0,
				true,
				{
					QStringLiteral("slave contact type NC differs from group type NO")
				}
			}
		};

		for (const Variant &variant : variants) {
			QString xml = original;
			QVERIFY2(xml.contains(nc_link), "expected NC link XML not found in fixture");
			xml.replace(nc_link, variant.replacement);
			const QString path = writeVariant(dir, variant.name, xml);
			QVERIFY2(!path.isEmpty(), qPrintable(QStringLiteral("variant write failed for %1").arg(variant.name)));

			QETProject project(path);
			QCOMPARE(project.state(), QETProject::Ok);

			ContactCrossRefProjectionService service;
			const QList<ContactAssignmentProjection> assignments = service.assignments(project);
			QCOMPARE(assignments.size(), 2);

			const ContactAssignmentProjection *assignment = assignmentFor(assignments, variant.slave_uuid);
			QVERIFY2(assignment, qPrintable(QStringLiteral("assignment missing for %1").arg(variant.slave_uuid.toString())));
			QCOMPARE(assignment->group_index, variant.expected_group_index);
			QCOMPARE(assignment->group_index_resolves, variant.expected_resolves);

			for (const QString &message : variant.expected_messages) {
				const bool assignment_has_message = assignment->validation_messages.contains(message);
				const bool master_has_message = !service.masters(project).isEmpty()
					&& service.masters(project).first().validation_messages.contains(message);
				QVERIFY2(assignment_has_message || master_has_message,
						 qPrintable(QStringLiteral("expected validation message '%1' for %2").arg(message, variant.name)));
			}
		}
	}

	void duplicateGroupAssignmentIsObservableInUsageAndAssignments()
	{
		initHeadlessProjectLoad();

		const QString fixture = fixturePath();
		QVERIFY2(!fixture.isEmpty(), "master/slave projection fixture project not found");
		QVERIFY2(QFile::exists(fixture), qPrintable(QStringLiteral("fixture not found: %1").arg(fixture)));
		QString xml = readTextFile(fixture);
		QVERIFY2(!xml.isEmpty(), "fixture content not readable");

		const QString nc_link = QStringLiteral(
			"<link_uuid uuid=\"{33333333-3333-4333-8333-333333333333}\" group_index=\"1\"/>");
		QVERIFY2(xml.contains(nc_link), "expected NC link XML not found in fixture");
		xml.replace(nc_link, QStringLiteral(
			"<link_uuid uuid=\"{33333333-3333-4333-8333-333333333333}\" group_index=\"0\"/>"));

		const QString nc_state = QStringLiteral("<kindInformation name=\"state\" show=\"1\">NC</kindInformation>");
		QVERIFY2(xml.contains(nc_state), "expected NC slave state XML not found in fixture");
		xml.replace(nc_state, QStringLiteral("<kindInformation name=\"state\" show=\"1\">NO</kindInformation>"));

		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString path = writeVariant(dir, QStringLiteral("duplicate_group_assignment.qet"), xml);
		QVERIFY2(!path.isEmpty(), "duplicate assignment variant write failed");

		QETProject project(path);
		QCOMPARE(project.state(), QETProject::Ok);

		ContactCrossRefProjectionService service;
		const QList<ContactMasterProjection> masters = service.masters(project);
		QCOMPARE(masters.size(), 1);
		const ContactMasterProjection master = masters.first();
		QCOMPARE(master.linked_slave_count, 2);
		QCOMPARE(master.usage.no, 2);
		QCOMPARE(master.usage.nc, 0);
		QCOMPARE(master.usage.total(), 2);
		QCOMPARE(master.capacity.no, 1);
		QCOMPARE(master.capacity.nc, 1);
		QCOMPARE(master.capacity.total(), 2);
		QVERIFY2(master.validation_messages.isEmpty(), qPrintable(master.validation_messages.join(QStringLiteral("; "))));

		const QList<ContactAssignmentProjection> assignments = service.assignments(project);
		QCOMPARE(assignments.size(), 2);

		const ContactAssignmentProjection *no_assignment = assignmentFor(assignments, kNoSlaveUuid);
		QVERIFY(no_assignment);
		QCOMPARE(no_assignment->group_index, 0);
		QVERIFY(no_assignment->group_index_resolves);
		QCOMPARE(no_assignment->slave_contact_type, ElementData::NO);
		QVERIFY2(no_assignment->validation_messages.isEmpty(),
				 qPrintable(no_assignment->validation_messages.join(QStringLiteral("; "))));

		const ContactAssignmentProjection *duplicate_assignment = assignmentFor(assignments, kNcSlaveUuid);
		QVERIFY(duplicate_assignment);
		QCOMPARE(duplicate_assignment->group_index, 0);
		QVERIFY(duplicate_assignment->group_index_resolves);
		QCOMPARE(duplicate_assignment->slave_contact_type, ElementData::NO);
		QCOMPARE(duplicate_assignment->group.type, ElementData::NO);
		QCOMPARE(duplicate_assignment->group.terminal_labels, QStringList({QStringLiteral("13"), QStringLiteral("14")}));
		QVERIFY2(duplicate_assignment->validation_messages.isEmpty(),
				 qPrintable(duplicate_assignment->validation_messages.join(QStringLiteral("; "))));
	}
};

int main(int argc, char **argv)
{
	qputenv("QT_HASH_SEED", "0");
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QCoreApplication::setOrganizationName("QElectroTech");
	QCoreApplication::setOrganizationDomain("qelectrotech.org");
	QCoreApplication::setApplicationName("QElectroTech");
	QApplication app(argc, argv);
	tst_contactcrossrefprojectionservice tc;
	return QTest::qExec(&tc, argc, argv);
}

#include "tst_contactcrossrefprojectionservice.moc"
