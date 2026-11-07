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
#include "diagram.h"
#include "properties/potentialtypes.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetresult.h"
#include "qetproject.h"
#include "ui/conductorpropertieswidget.h"
#include "ui/potentialtypeswidget.h"

#include "dataBase/projectdatabase.h"
#include "dataBase/qetdbfile.h"

#include <QDir>
#include <QImage>
#include <QPainter>
#include <QSqlQuery>
#include <QStyleOptionGraphicsItem>

/**
	Potential types (#1539): a project lists types (L, N, PE...) with a
	colour, line style and width, and, when it asks to, draws a conductor
	whose type is one of them in that format, whatever style the conductor
	stores. Off by default, so a project that never turns it on is drawn and
	saved as before.
*/
class tst_potentialtypes : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	static PotentialTypes blueTypes(bool enabled, Qt::PenStyle style, qreal width)
	{
		PotentialTypes types;
		types.setEnabled(enabled);
		types.setTypes({{QStringLiteral("N"), QColor(0, 0, 255), style, width}});
		return types;
	}

	/// The longest conductor of the fixture, whose own style is thin black
	Conductor *wire(QETProject &project)
	{
		Conductor *best = nullptr;
		for (Diagram *diagram : project.diagrams())
			for (Conductor *c : diagram->conductors())
				if (!best || c->boundingRect().width() + c->boundingRect().height()
						> best->boundingRect().width() + best->boundingRect().height())
					best = c;
		return best;
	}

	/// Number of pixels of one colour in the conductor as it is painted
	int pixels(Conductor *c, const QColor &color)
	{
		const QRectF area = c->sceneBoundingRect().adjusted(-10, -10, 10, 10);
		QImage image(area.size().toSize(), QImage::Format_RGB32);
		image.fill(Qt::white);
		QPainter painter(&image);
		painter.translate(-area.topLeft());
		QStyleOptionGraphicsItem option;
		c->paint(&painter, &option, nullptr);
		painter.end();
		int count = 0;
		for (int y = 0; y < image.height(); ++y)
			for (int x = 0; x < image.width(); ++x)
				if (QColor(image.pixel(x, y)) == color)
					++count;
		return count;
	}

	/// A wire with no type of its own whose potential is known and
	/// holds other wires, and the terminal it starts from
	Conductor *sourcedWire(QETProject &project, Terminal **source)
	{
		const auto groups = project.dataBase()->potentialGroups();
		for (Diagram *diagram : project.diagrams())
			for (Conductor *c : diagram->conductors()) {
				if (!c->properties().m_potential_type.isEmpty() || groups.isUnknown(c->uuid())
						|| !groups.group_of.contains(c->uuid()) || c->relatedPotentialConductors().isEmpty()
						|| !c->terminal1 || !c->terminal1->parentElement())
					continue;
				*source = c->terminal1;
				return c;
			}
		return nullptr;
	}

	static PotentialTypes sourceTypes(PotentialTypes::Mode mode)
	{
		PotentialTypes types;
		types.setMode(mode);
		types.setTypes({{QStringLiteral("N"), QColor(0, 0, 255), Qt::SolidLine, 3},
						{QStringLiteral("PE"), QColor(0, 160, 0), Qt::SolidLine, 3}});
		return types;
	}

	static void setOwnType(Conductor *c, const QString &type)
	{
		ConductorProperties p = c->properties();
		p.color = Qt::black;
		p.style = Qt::SolidLine;
		p.cond_size = 1;
		p.m_bicolor = false;
		p.m_potential_type = type;
		c->setProperties(p);
	}

	private slots:
		void xmlRoundTrip()
		{
			PotentialTypes types = blueTypes(true, Qt::DashDotLine, 2.5);
			QDomDocument doc;
			doc.appendChild(types.toXml(doc));
			QCOMPARE(PotentialTypes::fromXml(doc.documentElement()), types);
		}

		void projectWithoutItIsOffAndWritesNothing()
		{
			QCOMPARE(PotentialTypes::fromXml(QDomElement()), PotentialTypes());
			QVERIFY(!PotentialTypes().isEnabled());
			QVERIFY(PotentialTypes().isDefault());
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			QVERIFY(!project.potentialTypes().isEnabled());
			QVERIFY(project.toXml().documentElement()
					.firstChildElement(QStringLiteral("potentialtypes")).isNull());
		}

		void defaultTypes()
		{
			PotentialType found;
			QVERIFY(PotentialTypes().find(QStringLiteral("PE"), &found));
			QCOMPARE(found.style, Qt::DashDotLine);
			QVERIFY(PotentialTypes().find(QStringLiteral("N")));
			QVERIFY(!PotentialTypes().find(QStringLiteral("nope")));
			QVERIFY(!PotentialTypes().find(QString()));
		}

		void keptWhenSavedAndOpenedAgain()
		{
			const QString path = m_dir.filePath(QStringLiteral("types.qet"));
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), path));
			{
				QETProject project(path);
				QVERIFY(!project.isReadOnly());
				project.setPotentialTypes(blueTypes(true, Qt::DashLine, 2));
				QVERIFY(project.projectWasModified());
				Conductor *c = wire(project);
				QVERIFY(c);
				ConductorProperties p = c->properties();
				p.m_potential_type = QStringLiteral("N");
				c->setProperties(p);
				QVERIFY(project.write().isOk());
			}
			QETProject reopened(path);
			QCOMPARE(reopened.potentialTypes(), blueTypes(true, Qt::DashLine, 2));
			QCOMPARE(wire(reopened)->properties().m_potential_type, QStringLiteral("N"));
		}

		/// The project database is what the data direction builds on: a
		/// typed wire must be readable from it, before and after a reopen
		void typedWireIsInTheDatabase()
		{
			const QString path = m_dir.filePath(QStringLiteral("db.qet"));
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), path));
			const auto stored = [](QETProject &project, const QUuid &wire) {
					//newQuery() runs the text as it is: no binding
				QSqlQuery query = project.dataBase()->newQuery(
							QStringLiteral("SELECT value FROM conductor_properties"
										   " WHERE name = 'potential_type' AND conductor_uuid = '")
							+ wire.toString() + QStringLiteral("'"));
				return query.next() ? query.value(0).toString() : QString();
			};
			QUuid uuid;
			{
				QETProject project(path);
				project.setPotentialTypes(blueTypes(true, Qt::DashLine, 2));
				Conductor *c = wire(project);
				QVERIFY(c);
				uuid = c->uuid();
				QVERIFY(stored(project, uuid).isEmpty());
				ConductorProperties p = c->properties();
				p.m_potential_type = QStringLiteral("N");
				c->setProperties(p);
				QCOMPARE(stored(project, uuid), QStringLiteral("N"));
				QVERIFY(project.write().isOk());
			}
			QETProject reopened(path);
			QCOMPARE(stored(reopened, uuid), QStringLiteral("N"));
		}

		/// The list and the switch are rows of project_setting, and the
		/// database alone gives back the element the project writes: what a
		/// file made from the database needs (POTENTIAL-TYPES-PLAN.md step 2)
		void typeListIsInTheDatabase()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			PotentialTypes types;
			types.setEnabled(true);
			types.setTypes({{QStringLiteral("PE"), QColor(0, 128, 0), Qt::DashDotLine, 1.5},
							{QStringLiteral("N"), QColor(0, 0, 255), Qt::DashLine, 1.0},
							{QStringLiteral("L"), QColor(Qt::black), Qt::SolidLine, 2.0}});
			project.setPotentialTypes(types);

			QSqlQuery query = project.dataBase()->newQuery(
						QStringLiteral("SELECT value FROM project_setting WHERE name = 'name'"
									   " AND section = 'potentialtypes/potentialtype' ORDER BY ord"));
			QStringList names;
			while (query.next())
				names << query.value(0).toString();
			QCOMPARE(names, (QStringList{"PE", "N", "L"}));

			const QDomDocument rebuilt = projectDataBase::toDocument(project.dataBase()->database());
			const QDomElement element = rebuilt.documentElement()
					.firstChildElement(QStringLiteral("potentialtypes"));
			QVERIFY2(!element.isNull(), "the database gives back <potentialtypes>");
			QCOMPARE(PotentialTypes::fromXml(element), types);
		}

		/// potentialGroups() is potentialWires() for every wire at once, and
		/// both are what the scene finds (POTENTIAL-TYPES-PLAN.md step 5b).
		/// QET_GROUPS_EXTRA_DIR adds the .qet files of one more directory.
		void groupsAgreeWithTheOneWireAnswerAndTheScene()
		{
			QStringList files;
			const QDir fixtures(QFileInfo(QFINDTESTDATA("fixtures/every_feature.qet")).absolutePath());
			for (const QString &name : fixtures.entryList({QStringLiteral("*.qet")}, QDir::Files))
				files << fixtures.filePath(name);
			const QString extra = qEnvironmentVariable("QET_GROUPS_EXTRA_DIR");
			if (!extra.isEmpty())
				for (const QString &name : QDir(extra).entryList({QStringLiteral("*.qet")}, QDir::Files))
					files << QDir(extra).filePath(name);

			int wires = 0, unknown = 0, projects = 0;
			for (const QString &file : files) {
				QETProject project(file);
				projectDataBase *db = project.dataBase();
				if (!db)
					continue;
				++projects;
				const projectDataBase::PotentialGroups groups = db->potentialGroups();
				QHash<int, QSet<QUuid>> members;
				for (auto it = groups.group_of.constBegin(); it != groups.group_of.constEnd(); ++it)
					members[it.value()].insert(it.key());

				for (Diagram *diagram : project.diagrams()) {
					for (Conductor *c : diagram->conductors()) {
						const QUuid id = c->uuid();
						const QString where = file + QStringLiteral(" ") + id.toString();
						++wires;
						const auto one = db->potentialWires(id);
						if (!groups.group_of.contains(id)) {
							QVERIFY2(!one.has_value(), qPrintable(where));
							continue;
						}
						if (!one.has_value()) {
							++unknown;
							QVERIFY2(groups.isUnknown(id), qPrintable(where));
							continue;
						}
						QVERIFY2(!groups.isUnknown(id), qPrintable(where));
						const QSet<QUuid> group = members.value(groups.group_of.value(id));
						QVERIFY2(group == *one, qPrintable(where));
						QSet<QUuid> scene{id};
						for (Conductor *other : c->relatedPotentialConductors())
							scene.insert(other->uuid());
						QVERIFY2(group == scene, qPrintable(where));
					}
				}
			}
			fprintf(stderr, "GROUPSRESULT %d projects, %d wires, %d unknown\n", projects, wires, unknown);
			QVERIFY(projects > 5 && wires > 50);
		}

		/// A terminal block that isolates its terminals makes the potential
		/// unknown, in the bulk answer as in the one-wire one
		void isolatingTerminalBlockMakesThePotentialUnknown()
		{
			int checked = 0;
			const QDir examples(qEnvironmentVariable("QET_GROUPS_EXTRA_DIR"));
			const QDir fixtures(QFileInfo(QFINDTESTDATA("fixtures/every_feature.qet")).absolutePath());
			QStringList files;
			for (const QDir &dir : {fixtures, examples})
				for (const QString &name : dir.entryList({QStringLiteral("*.qet")}, QDir::Files))
					files << dir.filePath(name);
			for (const QString &file : files) {
				QETProject project(file);
				if (!project.dataBase())
					continue;
				Element *block = nullptr;
				for (Diagram *diagram : project.diagrams())
					for (Element *e : diagram->elements())
						if (!block && (e->elementData().m_type & ElementData::Terminal)
								&& !e->conductors().isEmpty())
							block = e;
				if (!block)
					continue;
				DiagramContext info = block->elementInformations();
				info.addValue(QStringLiteral("potential_isolating"), true);
				block->setElementInformations(info);
				const Conductor *wire = block->conductors().first();
				const auto groups = project.dataBase()->potentialGroups();
				QVERIFY2(groups.isUnknown(wire->uuid()), qPrintable(file));
				QVERIFY2(!project.dataBase()->potentialWires(wire->uuid()).has_value(), qPrintable(file));
				++checked;
			}
			fprintf(stderr, "ISOLATING checked %d projects\n", checked);
			if (checked == 0)
				QSKIP("no project with a wired terminal block");
		}

		/// Two types on one potential are reported; the same type twice, one
		/// typed wire, and untyped wires are not; the TEMP table is not saved
		void mismatchIsReportedFromTheDatabase()
		{
			const QString path = m_dir.filePath(QStringLiteral("mismatch.qet"));
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), path));
			QETProject project(path);
			projectDataBase *db = project.dataBase();
			Conductor *c = wire(project);
			QVERIFY(c);
			const QSet<Conductor *> others = c->relatedPotentialConductors();
			QVERIFY(!others.isEmpty());
			Conductor *other = *others.constBegin();
			const auto typeOf = [](Conductor *w, const QString &type) {
				ConductorProperties p = w->properties();
				p.m_potential_type = type;
				w->setProperties(p);
			};

			QVERIFY(db->potentialTypeMismatches().isEmpty());          // untyped
			typeOf(c, QStringLiteral("N"));
			QVERIFY(db->potentialTypeMismatches().isEmpty());          // one typed wire
			typeOf(other, QStringLiteral("N"));
			QVERIFY(db->potentialTypeMismatches().isEmpty());          // same type twice
			typeOf(other, QStringLiteral("PE"));
			const auto found = db->potentialTypeMismatches();
			QCOMPARE(found.size(), 1);
			QCOMPARE(found.first().types, (QStringList{"N", "PE"}));
			QVERIFY(found.first().wires.contains(c->uuid()));
			QVERIFY(found.first().wires.contains(other->uuid()));
			typeOf(other, QStringLiteral("N"));
			QVERIFY(db->potentialTypeMismatches().isEmpty());          // fixed again

			typeOf(other, QStringLiteral("PE"));
			QVERIFY(project.write().isOk());
			QETProject reopened(path);
			QSqlQuery temp = reopened.dataBase()->newQuery(
						QStringLiteral("SELECT COUNT(*) FROM main.sqlite_master WHERE name = 'potential_member'"));
			QVERIFY(temp.next());
			QCOMPARE(temp.value(0).toInt(), 0);                         // not carried by the file
			QCOMPARE(reopened.dataBase()->potentialTypeMismatches().size(), 1);
		}

		/// wire_potential_view: every wire, its potential and its effective
		/// type with where it comes from; TEMP, so never in the saved file
		void wirePotentialViewListsEveryWire()
		{
			const QString path = m_dir.filePath(QStringLiteral("view.qet"));
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), path));
			QETProject project(path);
			projectDataBase *db = project.dataBase();
			Conductor *c = wire(project);
			QVERIFY(c);
			ConductorProperties p = c->properties();
			p.m_potential_type = QStringLiteral("PE");
			c->setProperties(p);
			db->refreshPotentialMembers();

			const auto count = [db](const QString &sql) {
				QSqlQuery q = db->newQuery(sql);
				return q.next() ? q.value(0).toInt() : -1;
			};
			QCOMPARE(count(QStringLiteral("SELECT COUNT(*) FROM wire_potential_view")),
					 count(QStringLiteral("SELECT COUNT(*) FROM conductor")));
			QCOMPARE(count(QStringLiteral("SELECT COUNT(*) FROM wire_potential_view WHERE potential_id IS NULL")), 0);
			QCOMPARE(count(QStringLiteral("SELECT COUNT(*) FROM wire_potential_view WHERE potential_type IS NOT NULL")), 1);
			QSqlQuery q = db->newQuery(QStringLiteral("SELECT potential_type, source FROM wire_potential_view WHERE wire_uuid = '")
									   + c->uuid().toString() + QStringLiteral("'"));
			QVERIFY(q.next());
			QCOMPARE(q.value(0).toString(), QStringLiteral("PE"));
			QCOMPARE(q.value(1).toString(), QStringLiteral("wire"));

			QVERIFY(project.write().isOk());
			QETProject reopened(path);
			QSqlQuery stored = reopened.dataBase()->newQuery(QStringLiteral(
				"SELECT COUNT(*) FROM sqlite_master WHERE name IN ('wire_potential_view', 'potential_member')"));
			QVERIFY(stored.next());
			QCOMPARE(stored.value(0).toInt(), 0);
		}

		/// A .qetdb read by itself, without opening the project, gives the
		/// project's potentials, its unknown ones and its mismatches
		void qetdbGivesTheSameAnswersAsTheProject()
		{
			QStringList files{QFINDTESTDATA("fixtures/every_feature.qet"),
							  QFINDTESTDATA("fixtures/linked_reports.qet"),
							  QFINDTESTDATA("fixtures/junction_dot_shared_potential.qet")};
			const QString extra = qEnvironmentVariable("QET_GROUPS_EXTRA_DIR");
			if (!extra.isEmpty())
				for (const QString &name : QDir(extra).entryList({QStringLiteral("*.qet")}, QDir::Files))
					files << QDir(extra).filePath(name);

				//The partition, as sorted lines of sorted wires, and the
				//unknown wires: comparable whatever the group numbers are
			const auto shape = [](const projectDataBase::PotentialGroups &g) {
				QHash<int, QStringList> members;
				QStringList unknown;
				for (auto it = g.group_of.constBegin(); it != g.group_of.constEnd(); ++it) {
					members[it.value()] << it.key().toString();
					if (g.isUnknown(it.key())) unknown << it.key().toString();
				}
				QStringList lines;
				for (QStringList wires : std::as_const(members)) {
					wires.sort();
					lines << wires.join(QLatin1Char(' '));
				}
				lines.sort();
				unknown.sort();
				return QPair<QStringList, QStringList>(lines, unknown);
			};
			const auto mismatchShape = [](const QList<projectDataBase::PotentialMismatch> &list) {
				QStringList lines;
				for (const auto &m : list) {
					QStringList wires;
					for (const QUuid &w : m.wires) wires << w.toString();
					wires.sort();
					lines << m.types.join(QLatin1Char('+')) + QLatin1Char(':') + wires.join(QLatin1Char(' '));
				}
				lines.sort();
				return lines;
			};

			int compared = 0, mismatching = 0, unknown_total = 0, mismatch_total = 0;
			for (const QString &file : files) {
				QETProject project(file);
				if (!project.dataBase())
					continue;
					//Two different types on one potential, and an isolating
					//terminal block somewhere, so both kinds of answer exist
				for (Diagram *diagram : project.diagrams())
					for (Conductor *c : diagram->conductors()) {
						const QSet<Conductor *> related = c->relatedPotentialConductors();
						if (related.isEmpty() || mismatching > 0)
							continue;
						ConductorProperties p = c->properties();
						p.m_potential_type = QStringLiteral("N");
						c->setProperties(p);
						Conductor *other = *related.constBegin();
						p = other->properties();
						p.m_potential_type = QStringLiteral("PE");
						other->setProperties(p);
						++mismatching;
					}
				Element *block = nullptr;
				for (Diagram *diagram : project.diagrams())
					for (Element *e : diagram->elements())
						if (!block && (e->elementData().m_type & ElementData::Terminal)
								&& !e->conductors().isEmpty())
							block = e;
				if (block) {
					DiagramContext info = block->elementInformations();
					info.addValue(QStringLiteral("potential_isolating"), true);
					block->setElementInformations(info);
				}
				mismatching = 0;

				projectDataBase *db = project.dataBase();
				const auto from_project = shape(db->potentialGroups());
				const auto mismatches_project = mismatchShape(db->potentialTypeMismatches());

				const QString path = m_dir.filePath(QStringLiteral("same%1.qetdb").arg(compared));
				QString error;
				QVERIFY2(QetDbFile::save(*db, path, &error), qPrintable(file + QStringLiteral(": ") + error));
				const QString name = QStringLiteral("potential_qetdb_%1").arg(compared);
				{
					QSqlDatabase file_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
					file_db.setDatabaseName(path);
					file_db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
					QVERIFY2(file_db.open(), qPrintable(file));
					const auto from_file = shape(projectDataBase::potentialGroupsOf(file_db));
					QVERIFY2(from_file == from_project, qPrintable(file));
					QCOMPARE(mismatchShape(projectDataBase::potentialTypeMismatchesOf(file_db)), mismatches_project);
					unknown_total += from_file.second.size();
					mismatch_total += mismatches_project.size();
					file_db.close();
				}
				QSqlDatabase::removeDatabase(name);
				++compared;
			}
			fprintf(stderr, "QETDBSAME compared %d projects, %d unknown wires, %d mismatching potentials\n",
					compared, unknown_total, mismatch_total);
			QVERIFY(compared >= 3);
		}

		/// The three modes: off by default, the name written is the one read,
		/// an unknown name reads as off, and the database holds the mode
		void modeIsKeptAndUnknownReadsAsOff()
		{
			QCOMPARE(PotentialTypes().mode(), PotentialTypes::Mode::Off);
			for (auto mode : {PotentialTypes::Mode::Off, PotentialTypes::Mode::PerWire,
							  PotentialTypes::Mode::AtSource}) {
				PotentialTypes types;
				types.setMode(mode);
				QDomDocument doc;
				doc.appendChild(types.toXml(doc));
				const PotentialTypes back = PotentialTypes::fromXml(doc.documentElement());
				QCOMPARE(back.mode(), mode);
				QCOMPARE(back, types);
			}
			QDomDocument doc;
			QDomElement root = doc.createElement(QStringLiteral("potentialtypes"));
			root.setAttribute(QStringLiteral("mode"), QStringLiteral("from-a-newer-qet"));
			doc.appendChild(root);
			const PotentialTypes newer = PotentialTypes::fromXml(doc.documentElement());
			QCOMPARE(newer.mode(), PotentialTypes::Mode::Off);
				//Written back as it was, so a re-save does not lose it; a
				//mode the user then picks replaces it
			QDomDocument again;
			again.appendChild(newer.toXml(again));
			QCOMPARE(again.documentElement().attribute(QStringLiteral("mode")),
					 QStringLiteral("from-a-newer-qet"));
			PotentialTypes chosen = newer;
			chosen.setMode(PotentialTypes::Mode::PerWire);
			QDomDocument picked;
			picked.appendChild(chosen.toXml(picked));
			QCOMPARE(picked.documentElement().attribute(QStringLiteral("mode")), QStringLiteral("wire"));
			bool known = true;
			PotentialTypes::modeFromName(QStringLiteral("from-a-newer-qet"), &known);
			QVERIFY(!known);
			PotentialTypes::modeFromName(QStringLiteral("off"), &known);
			QVERIFY(known);

			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			PotentialTypes types;
			types.setMode(PotentialTypes::Mode::AtSource);
			project.setPotentialTypes(types);
			QSqlQuery query = project.dataBase()->newQuery(
						QStringLiteral("SELECT value FROM project_setting WHERE section = 'potentialtypes' AND name = 'mode'"));
			QVERIFY(query.next());
			QCOMPARE(query.value(0).toString(), QStringLiteral("source"));
		}

		/// A wire's own type is drawn in the source mode as in the
		/// per-wire mode
		void sourceModeDrawsAWiresOwnType()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Conductor *c = wire(project);
			QVERIFY(c);
			ConductorProperties p = c->properties();
			p.color = Qt::black;
			p.style = Qt::SolidLine;
			p.cond_size = 1;
			p.m_bicolor = false;
			p.m_potential_type = QStringLiteral("N");
			c->setProperties(p);
			PotentialTypes types = blueTypes(true, Qt::SolidLine, 3);
			types.setMode(PotentialTypes::Mode::AtSource);
			project.setPotentialTypes(types);
			QVERIFY(pixels(c, QColor(0, 0, 255)) > 0);
			QCOMPARE(pixels(c, Qt::black), 0);
		}

		/// The settings page offers the three modes and gives back the one
		/// it was shown
		void settingsPageOffersEveryMode()
		{
			PotentialTypesWidget widget;
			for (auto mode : {PotentialTypes::Mode::Off, PotentialTypes::Mode::PerWire,
							  PotentialTypes::Mode::AtSource}) {
				PotentialTypes types;
				types.setMode(mode);
				widget.setPotentialTypes(types);
				QCOMPARE(widget.potentialTypes().mode(), mode);
			}
		}

		/// A project saved before the modes holds only enabled="0|1": 1 is
		/// the per-wire mode; and a file written now still carries enabled,
		/// for a QET that does not know the modes
		void fileBeforeTheModesStillReads()
		{
			for (const QString &enabled : {QStringLiteral("1"), QStringLiteral("0")}) {
				QDomDocument doc;
				QDomElement root = doc.createElement(QStringLiteral("potentialtypes"));
				root.setAttribute(QStringLiteral("enabled"), enabled);
				QDomElement type = doc.createElement(QStringLiteral("potentialtype"));
				type.setAttribute(QStringLiteral("name"), QStringLiteral("N"));
				root.appendChild(type);
				doc.appendChild(root);
				const PotentialTypes read = PotentialTypes::fromXml(doc.documentElement());
				QCOMPARE(read.mode(), enabled == QLatin1String("1")
						 ? PotentialTypes::Mode::PerWire : PotentialTypes::Mode::Off);
				QCOMPARE(read.types().size(), 1);
			}
			for (auto mode : {PotentialTypes::Mode::Off, PotentialTypes::Mode::PerWire,
							  PotentialTypes::Mode::AtSource}) {
				PotentialTypes types;
				types.setMode(mode);
				QDomDocument doc;
				doc.appendChild(types.toXml(doc));
				QCOMPARE(doc.documentElement().attribute(QStringLiteral("enabled")).toInt() != 0,
						 mode != PotentialTypes::Mode::Off);
			}
		}

		/// A pair of types joined on purpose is kept in the file and the
		/// database, and a potential whose types are all so joined is not a
		/// mismatch; one more type not joined to them is
		/// Saving the settings page must not drop the declared bonds
		void widgetKeepsDeclaredBonds()
		{
			PotentialTypes types;
			types.setTypes(PotentialTypes::defaultTypes());
			types.setBonds({{"0V", "PE"}});
			PotentialTypesWidget widget;
			widget.setPotentialTypes(types);
			QCOMPARE(widget.potentialTypes().bonds().size(), 1);
			QVERIFY(widget.potentialTypes().isBond("PE", "0V"));
		}

		void declaredBondIsNotAMismatch()
		{
			PotentialTypes types;
			types.setBonds({{QStringLiteral("0V"), QStringLiteral("PE")}});
			QVERIFY(!types.isDefault());
			QVERIFY(types.isBond(QStringLiteral("PE"), QStringLiteral("0V")));
			QVERIFY(types.isBond(QStringLiteral("N"), QStringLiteral("N")));
			QVERIFY(!types.isBond(QStringLiteral("N"), QStringLiteral("PE")));
			QDomDocument doc;
			doc.appendChild(types.toXml(doc));
			QCOMPARE(PotentialTypes::fromXml(doc.documentElement()), types);
			QCOMPARE(PotentialTypes::fromXml(doc.documentElement()).bonds().size(), 1);

			const QString path = m_dir.filePath(QStringLiteral("bond.qet"));
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), path));
			QETProject project(path);
			projectDataBase *db = project.dataBase();
			Conductor *c = wire(project);
			QVERIFY(c);
			const QSet<Conductor *> others = c->relatedPotentialConductors();
			QVERIFY(others.size() >= 1);
			const auto typeOf = [](Conductor *w, const QString &type) {
				ConductorProperties p = w->properties();
				p.m_potential_type = type;
				w->setProperties(p);
			};
			typeOf(c, QStringLiteral("0V"));
			typeOf(*others.constBegin(), QStringLiteral("PE"));
			QCOMPARE(db->potentialTypeMismatches().size(), 1);          // not declared yet

			project.setPotentialTypes(types);
			QCOMPARE(projectDataBase::potentialBondsOf(db->database()).size(), 1);
			QVERIFY(db->potentialTypeMismatches().isEmpty());            // declared: meant

			typeOf(*others.constBegin(), QStringLiteral("N"));           // 0V with N: not joined
			QCOMPARE(db->potentialTypeMismatches().size(), 1);
		}

		/// A query that names the check's objects gets them, through the
		/// project and through a .qetdb read by itself; both give the same rows
		void mismatchRowsAreQueryable()
		{
			const QString path = m_dir.filePath(QStringLiteral("rows.qet"));
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), path));
			QETProject project(path);
			projectDataBase *db = project.dataBase();
			Conductor *c = wire(project);
			QVERIFY(c);
			Conductor *other = *c->relatedPotentialConductors().constBegin();
			ConductorProperties p = c->properties();
			p.m_potential_type = QStringLiteral("N");
			c->setProperties(p);
			p = other->properties();
			p.m_potential_type = QStringLiteral("PE");
			other->setProperties(p);

			const QString sql = QStringLiteral("SELECT types, potential_type, wire_uuid FROM potential_mismatch "
											   "ORDER BY wire_uuid");
			const auto rows = [](QSqlQuery q) {
				QStringList list;
				while (q.next())
					list << q.value(0).toString() + QLatin1Char('|') + q.value(1).toString()
							+ QLatin1Char('|') + q.value(2).toString();
				return list;
			};
			QString error;
			const QStringList from_project = rows(db->newQuery(sql, &error));
			QVERIFY2(error.isEmpty(), qPrintable(error));
			QCOMPARE(from_project.size(), 2);
			QVERIFY(from_project.first().startsWith(QStringLiteral("N+PE|")));

			const QString file = m_dir.filePath(QStringLiteral("rows.qetdb"));
			QVERIFY(QetDbFile::save(*db, file, &error));
			const QString name = QStringLiteral("potential_rows");
			QStringList from_file;
			{
				QSqlDatabase f = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
				f.setDatabaseName(file);
				f.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
				QVERIFY(f.open());
				projectDataBase::potentialTypeMismatchesOf(f);
				from_file = rows(QSqlQuery(sql, f));
				f.close();
			}
			QSqlDatabase::removeDatabase(name);
			QCOMPARE(from_file, from_project);
		}

		void drawnFromTheTypeOnlyWhenOn()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Conductor *c = wire(project);
			QVERIFY(c);
			ConductorProperties p = c->properties();
			p.color = Qt::black;
			p.style = Qt::SolidLine;
			p.cond_size = 1;
			p.m_bicolor = false;
			p.m_potential_type = QStringLiteral("N");
			c->setProperties(p);
			const QColor blue(0, 0, 255);

				//Off: the conductor's own style, whatever type it names
			QVERIFY(pixels(c, Qt::black) > 0);
			QCOMPARE(pixels(c, blue), 0);

			project.setPotentialTypes(blueTypes(true, Qt::SolidLine, 3));
			const int solid = pixels(c, blue);
			QVERIFY(solid > 0);
			QCOMPARE(pixels(c, Qt::black), 0);

				//The line style and the width come from the type too
			project.setPotentialTypes(blueTypes(true, Qt::DashLine, 3));
			const int dashed = pixels(c, blue);
			QVERIFY(dashed > 0 && dashed < solid);
			project.setPotentialTypes(blueTypes(true, Qt::SolidLine, 6));
			QVERIFY(pixels(c, blue) > solid);

				//A type the project does not list, or none: own style
			p.m_potential_type = QStringLiteral("unknown");
			c->setProperties(p);
			QCOMPARE(pixels(c, blue), 0);
			p.m_potential_type.clear();
			c->setProperties(p);
			QCOMPARE(pixels(c, blue), 0);

				//Nothing stored on the conductor was touched
			QCOMPARE(c->properties().color, QColor(Qt::black));
			QCOMPARE(c->properties().cond_size, 1.0);
		}

		void typeSpreadsOverThePotential()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Conductor *c = wire(project);
			QVERIFY(c);
			const QSet<Conductor *> others = c->relatedPotentialConductors();
			QVERIFY2(!others.isEmpty(), "the fixture wire shares its potential with others");
			ConductorProperties p = c->properties();
			p.m_potential_type = QStringLiteral("PE");
			c->setPropertyToPotential(p, true);
			for (Conductor *other : others)
				QCOMPARE(other->properties().m_potential_type, QStringLiteral("PE"));
		}

		void choiceInTheDialogKeepsAnUnlistedType()
		{
			ConductorProperties p;
			p.m_potential_type = QStringLiteral("gone");
			ConductorPropertiesWidget widget(p);
				//Hidden: nothing was offered, the type is kept as it is
			QCOMPARE(widget.properties().m_potential_type, QStringLiteral("gone"));
			widget.setPotentialTypes(blueTypes(true, Qt::SolidLine, 1));
			QCOMPARE(widget.properties().m_potential_type, QStringLiteral("gone"));
			p.m_potential_type = QStringLiteral("N");
			widget.setProperties(p);
			QCOMPARE(widget.properties().m_potential_type, QStringLiteral("N"));
		}

		void severalWiresKeepOnlyWhatTheyShare()
		{
			ConductorProperties a, b;
			a.m_potential_type = b.m_potential_type = QStringLiteral("N");
			ConductorProperties merged;
			merged.applyForEqualAttributes({a, b});
			QCOMPARE(merged.m_potential_type, QStringLiteral("N"));
			b.m_potential_type = QStringLiteral("PE");
			ConductorProperties mixed;
			mixed.applyForEqualAttributes({a, b});
			QVERIFY(mixed.m_potential_type.isEmpty());
		}
		/// A type set on a terminal of a placed symbol reaches every wire
		/// of its potential that has no type of its own, as the scene's
		/// own walk finds that potential, and only in the source mode
		void sourceTerminalTypesItsPotential()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Terminal *source = nullptr;
			Conductor *c = sourcedWire(project, &source);
			QVERIFY(c);
			QSet<Conductor *> potential = c->relatedPotentialConductors();
			potential << c;
			for (Conductor *w : std::as_const(potential))
				setOwnType(w, QString());

			project.setPotentialTypes(sourceTypes(PotentialTypes::Mode::AtSource));
			QVERIFY(project.derivedPotentialType(c->uuid()).isEmpty());
			source->parentElement()->setPotentialSources({{source->stableUuid(), QStringLiteral("N")}});
			for (Conductor *w : std::as_const(potential))
				QCOMPARE(project.derivedPotentialType(w->uuid()), QStringLiteral("N"));
			int outside = 0;
			for (Diagram *diagram : project.diagrams())
				for (Conductor *w : diagram->conductors())
					if (!potential.contains(w)) {
						QVERIFY(project.derivedPotentialType(w->uuid()).isEmpty());
						++outside;
					}
			QVERIFY(outside > 0);
			QVERIFY(pixels(c, QColor(0, 0, 255)) > 0);
			QCOMPARE(pixels(c, Qt::black), 0);

				//The check's view says where the type comes from
			projectDataBase *db = project.dataBase();
			db->refreshPotentialMembers();
			QSqlQuery q = db->newQuery(QStringLiteral("SELECT potential_type, source FROM wire_potential_view WHERE wire_uuid = '")
									   + c->uuid().toString() + QStringLiteral("'"));
			QVERIFY(q.next());
			QCOMPARE(q.value(0).toString(), QStringLiteral("N"));
			QCOMPARE(q.value(1).toString(), QStringLiteral("source"));

				//A wire's own type wins, and is then a mismatch on that potential
			setOwnType(c, QStringLiteral("PE"));
			QVERIFY(pixels(c, QColor(0, 160, 0)) > 0);
			QCOMPARE(pixels(c, QColor(0, 0, 255)), 0);
			const auto mismatches = db->potentialTypeMismatches();
			QCOMPARE(mismatches.size(), 1);
			QCOMPARE(mismatches.first().types, QStringList({QStringLiteral("N"), QStringLiteral("PE")}));
			setOwnType(c, QString());
			QVERIFY(db->potentialTypeMismatches().isEmpty());

				//The other modes keep the source and do not apply it
			for (auto mode : {PotentialTypes::Mode::PerWire, PotentialTypes::Mode::Off}) {
				project.setPotentialTypes(sourceTypes(mode));
				QVERIFY(project.derivedPotentialType(c->uuid()).isEmpty());
				QCOMPARE(pixels(c, QColor(0, 0, 255)), 0);
				QVERIFY(db->potentialTypeMismatches().isEmpty());
				QCOMPARE(source->parentElement()->potentialSources().size(), 1);
			}
			project.setPotentialTypes(sourceTypes(PotentialTypes::Mode::AtSource));
			QCOMPARE(project.derivedPotentialType(c->uuid()), QStringLiteral("N"));

				//Removing it removes the type
			source->parentElement()->setPotentialSources({});
			QVERIFY(project.derivedPotentialType(c->uuid()).isEmpty());
		}

		/// Two sources of different types on one potential: no type is
		/// drawn and the check reports the potential, with every wire on it;
		/// a declared bond between the two makes it meant
		void conflictingSourcesAreReported()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Terminal *source = nullptr;
			Conductor *c = sourcedWire(project, &source);
			QVERIFY(c);
			QSet<Conductor *> potential = c->relatedPotentialConductors();
			potential << c;
			for (Conductor *w : std::as_const(potential))
				setOwnType(w, QString());
			Terminal *other = c->terminal2;
			QVERIFY(other && other->parentElement());
			project.setPotentialTypes(sourceTypes(PotentialTypes::Mode::AtSource));
			source->parentElement()->setPotentialSources({{source->stableUuid(), QStringLiteral("N")}});
			if (other->parentElement() == source->parentElement())
				source->parentElement()->setPotentialSources({{source->stableUuid(), QStringLiteral("N")},
															   {other->stableUuid(), QStringLiteral("PE")}});
			else
				other->parentElement()->setPotentialSources({{other->stableUuid(), QStringLiteral("PE")}});

			QVERIFY(project.derivedPotentialType(c->uuid()).isEmpty());
			const auto mismatches = project.dataBase()->potentialTypeMismatches();
			QCOMPARE(mismatches.size(), 1);
			QCOMPARE(mismatches.first().types, QStringList({QStringLiteral("N"), QStringLiteral("PE")}));
			QCOMPARE(mismatches.first().wires.size(), potential.size());
			QVERIFY(mismatches.first().wires.contains(c->uuid()));

			PotentialTypes bonded = sourceTypes(PotentialTypes::Mode::AtSource);
			bonded.setBonds({{QStringLiteral("N"), QStringLiteral("PE")}});
			project.setPotentialTypes(bonded);
			QVERIFY(project.dataBase()->potentialTypeMismatches().isEmpty());
		}

		/// The source is saved with the placed symbol, comes back when the
		/// project is opened again, and a .qetdb read by itself gives the
		/// same derived types and writes it back in the document
		void sourceIsKeptInTheProjectAndTheQetdb()
		{
			const QString path = m_dir.filePath(QStringLiteral("source.qet"));
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), path));
			QUuid element_uuid, terminal_uuid, wire_uuid;
			QHash<QUuid, QString> derived;
			{
				QETProject project(path);
				Terminal *source = nullptr;
				Conductor *c = sourcedWire(project, &source);
				QVERIFY(c);
				setOwnType(c, QString());
				for (Conductor *w : c->relatedPotentialConductors())
					setOwnType(w, QString());
				project.setPotentialTypes(sourceTypes(PotentialTypes::Mode::AtSource));
				source->parentElement()->setPotentialSources({{source->stableUuid(), QStringLiteral("N")}});
				element_uuid = source->parentElement()->uuid();
				terminal_uuid = source->stableUuid();
				wire_uuid = c->uuid();
				derived = project.dataBase()->derivedPotentialTypes().type_of;
				QVERIFY(derived.size() >= 2);
				QCOMPARE(derived.value(wire_uuid), QStringLiteral("N"));
				QVERIFY(project.write().isOk());

				const QString file = m_dir.filePath(QStringLiteral("source.qetdb"));
				QString error;
				QVERIFY2(QetDbFile::save(*project.dataBase(), file, &error), qPrintable(error));
				const QString name = QStringLiteral("potential_source_qetdb");
				{
					QSqlDatabase file_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
					file_db.setDatabaseName(file);
					file_db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
					QVERIFY(file_db.open());
					QCOMPARE(projectDataBase::potentialModeOf(file_db), PotentialTypes::Mode::AtSource);
					QCOMPARE(projectDataBase::derivedPotentialTypesOf(
								 file_db, projectDataBase::potentialGroupsOf(file_db)).type_of, derived);
					const QDomDocument rebuilt = projectDataBase::toDocument(file_db);
					bool found = false;
					const QDomNodeList sources = rebuilt.elementsByTagName(QStringLiteral("potentialSources"));
					for (int i = 0 ; i < sources.size() ; ++i) {
						const QDomElement s = sources.at(i).toElement().firstChildElement(QStringLiteral("source"));
						if (QUuid(s.parentNode().parentNode().toElement().attribute(QStringLiteral("uuid"))) == element_uuid
								&& QUuid(s.attribute(QStringLiteral("terminal"))) == terminal_uuid
								&& s.attribute(QStringLiteral("type")) == QLatin1String("N"))
							found = true;
					}
					QVERIFY(found);
					file_db.close();
				}
				QSqlDatabase::removeDatabase(name);
			}
			QETProject reopened(path);
			QCOMPARE(reopened.potentialTypes().mode(), PotentialTypes::Mode::AtSource);
			Element *element = nullptr;
			for (Diagram *diagram : reopened.diagrams())
				for (Element *e : diagram->elements())
					if (e->uuid() == element_uuid) element = e;
			QVERIFY(element);
			QCOMPARE(element->potentialSources().value(terminal_uuid), QStringLiteral("N"));
			QCOMPARE(reopened.dataBase()->derivedPotentialTypes().type_of, derived);
			QCOMPARE(reopened.derivedPotentialType(wire_uuid), QStringLiteral("N"));
		}

		/// A symbol without a source saves exactly as before
		void symbolWithoutSourceWritesNothing()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			QVERIFY(project.toXml().documentElement()
					.elementsByTagName(QStringLiteral("potentialSources")).isEmpty());
		}

};

QET_APP_TEST_MAIN(tst_potentialtypes)
#include "tst_potentialtypes.moc"
