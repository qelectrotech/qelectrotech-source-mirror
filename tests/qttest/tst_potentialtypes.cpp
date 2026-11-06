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
#include "qetresult.h"
#include "qetproject.h"
#include "ui/conductorpropertieswidget.h"

#include "dataBase/projectdatabase.h"

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
						QStringLiteral("SELECT COUNT(*) FROM sqlite_temp_master WHERE name = 'potential_member'"));
			QVERIFY(temp.next());
			QCOMPARE(temp.value(0).toInt(), 0);                         // not carried by the file
			QCOMPARE(reopened.dataBase()->potentialTypeMismatches().size(), 1);
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
};

QET_APP_TEST_MAIN(tst_potentialtypes)
#include "tst_potentialtypes.moc"
