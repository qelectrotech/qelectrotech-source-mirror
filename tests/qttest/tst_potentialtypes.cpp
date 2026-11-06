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
#include "qetresult.h"
#include "qetproject.h"
#include "ui/conductorpropertieswidget.h"

#include <QImage>
#include <QPainter>
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
