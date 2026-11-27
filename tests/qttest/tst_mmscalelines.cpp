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

#include "ElementsCollection/xmlelementcollection.h"
#include "conductorproperties.h"
#include "factory/elementfactory.h"
#include "factory/elementpicturefactory.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "qetunits.h"
#include "ui/conductorpropertieswidget.h"

#include <QComboBox>
#include <QDomDocument>
#include <QDoubleSpinBox>
#include <QImage>
#include <QPainter>
#include <QTranslator>

/**
	Step 5 of docs/mm-scale.md: line widths in mm in a project drawn to
	scale. The conductor width is shown in mm and stored in px; the thin
	weight of a symbol is drawn 0.18 mm. Projects in px are as before.
*/
class tst_mmscalelines : public QObject
{
	Q_OBJECT

	static QDoubleSpinBox *sizeBox(ConductorPropertiesWidget &widget)
	{
		return widget.findChild<QDoubleSpinBox *>(QStringLiteral("m_cond_size_sb"));
	}

	/// Ink of the element drawn 8 times larger: the sum of the alpha
	static qint64 ink(Element *element)
	{
		QImage image(800, 800, QImage::Format_ARGB32_Premultiplied);
		image.fill(Qt::transparent);
		QPainter painter(&image);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.translate(400, 400);
		painter.scale(8, 8);
		element->paint(&painter, nullptr, nullptr);
		painter.end();
		qint64 sum = 0;
		for (int y = 0; y < image.height(); ++y)
			for (int x = 0; x < image.width(); ++x)
				sum += qAlpha(image.pixel(x, y));
		return sum;
	}

	private slots:
		void thinWidth()
		{
			QCOMPARE(ElementPictureFactory::thinWidth(0), 0.5);
			QCOMPARE(ElementPictureFactory::thinWidth(ElementPictureFactory::thinPx(QetUnits::defaultPxPerMm)), 0.72);
		}

		void widgetInMm()
		{
			QETProject project;
			project.addNewDiagram();
			project.setScale(QETProject::Scale::Mm);

			ConductorProperties properties;
			properties.cond_size = 1; //px
			ConductorPropertiesWidget widget(properties);
			widget.setProject(&project);
			QDoubleSpinBox *box = sizeBox(widget);
			QVERIFY(box);

				//1 px is 0.25 mm
			QCOMPARE(box->value(), 0.25);
			QVERIFY(box->suffix().contains(QStringLiteral("mm")));
			QCOMPARE(box->decimals(), 2);
				//Untouched: what was stored is what comes back
			QCOMPARE(widget.properties().cond_size, 1.0);

				//0.5 mm is stored as 2 px
			box->setValue(0.5);
			QCOMPARE(widget.properties().cond_size, 2.0);
			box->setValue(0.18);
			QCOMPARE(widget.properties().cond_size, 0.72);

				//A width that two decimals of mm do not hold is kept
			properties.cond_size = 1.3;
			widget.setProperties(properties);
			QCOMPARE(box->value(), 0.33);
			QCOMPARE(widget.properties().cond_size, 1.3);
			box->setValue(0.35);
			QCOMPARE(widget.properties().cond_size, 1.4);

				//The presets of ISO 128-20
			QComboBox *preset_box = nullptr;
			for (QComboBox *c : widget.findChildren<QComboBox *>())
				if (c->count() == 7 && c->itemData(1).toDouble() == 0.18)
					preset_box = c;
			QVERIFY(preset_box);
			const QList<double> expected {0.18, 0.25, 0.35, 0.5, 0.7, 1.0};
			for (int i = 0; i < expected.size(); ++i)
				QCOMPARE(preset_box->itemData(i + 1).toDouble(), expected.at(i));
			emit preset_box->activated(4);
			QCOMPARE(box->value(), 0.5);
			QCOMPARE(widget.properties().cond_size, 2.0);
		}

		void widthFollowsTheModule_data()
		{
			QTest::addColumn<qreal>("module");
			QTest::newRow("M 1.8") << 1.8;
			QTest::newRow("M 2.5") << 2.5;
			QTest::newRow("M 5.0") << 5.0;
		}

			//0.5 mm is 0.5 * 10 / M px, which may be fractional; the secondary
			//line size is whole px; the presets stay in mm
		void widthFollowsTheModule()
		{
			QFETCH(qreal, module);
			QETProject project;
			project.addNewDiagram();
			project.setScale(QETProject::Scale::Mm);
			project.setModuleMm(module);
			ConductorProperties properties;
			properties.m_bicolor = true;
			properties.cond_size = 1;
			properties.m_dash_size = 4;
			ConductorPropertiesWidget widget(properties);
			widget.setProject(&project);
			QDoubleSpinBox *box = sizeBox(widget);
			QVERIFY(box);
			QVERIFY(qAbs(box->value() - module / 10) < 0.006); //1 px
			QCOMPARE(widget.properties().cond_size, 1.0); //untouched
			box->setValue(0.5);
			QVERIFY(qAbs(widget.properties().cond_size - 0.5 * 10 / module) < 1e-9);
				//a preset is in mm too
			for (QComboBox *c : widget.findChildren<QComboBox *>())
				if (c->count() == 7 && c->itemData(1).toDouble() == 0.18) {
					emit c->activated(6); //1.0 mm
					QVERIFY(qAbs(widget.properties().cond_size - 10 / module) < 1e-9);
				}
			auto *dash = widget.findChild<QDoubleSpinBox *>(QStringLiteral("m_dash_size_sb"));
			QVERIFY(dash);
			dash->setValue(1.5);
			QCOMPARE(widget.properties().m_dash_size, qRound(1.5 * 10 / module));
		}

			//At M = 5 the pixel minimum 0.4 px is 0.20 mm: the 0.18 mm preset
			//of ISO 128-20 has to fit all the same
		void everyPresetFitsAtLargeModule()
		{
			QETProject project;
			project.addNewDiagram();
			project.setScale(QETProject::Scale::Mm);
			project.setModuleMm(5.0);
			ConductorProperties properties;
			ConductorPropertiesWidget widget(properties);
			widget.setProject(&project);
			bool found = false;
			for (QComboBox *c : widget.findChildren<QComboBox *>())
				if (c->count() == 7 && c->itemData(1).toDouble() == 0.18) {
					found = true;
					emit c->activated(1); //0.18 mm
					QVERIFY(qAbs(widget.properties().cond_size - 0.18 * 2) < 1e-9);
				}
			QVERIFY(found);
		}

		void widgetInPx()
		{
			QETProject project; //Px
			project.addNewDiagram();
			ConductorProperties properties;
			properties.cond_size = 1.4;
			ConductorPropertiesWidget widget(properties);
			widget.setProject(&project);
			QDoubleSpinBox *box = sizeBox(widget);
			QVERIFY(box);
			QCOMPARE(box->value(), 1.4);
			QVERIFY(box->suffix().isEmpty());
			QCOMPARE(widget.properties().cond_size, 1.4);
			box->setValue(2.0);
			QCOMPARE(widget.properties().cond_size, 2.0);

				//No project: px too
			ConductorPropertiesWidget defaults(properties);
			QCOMPARE(sizeBox(defaults)->value(), 1.4);
		}

			//The line size of the secondary color follows the width: mm in a
			//project drawn to scale, stored in whole px
		void secondaryLineSizeInMm()
		{
			QETProject project;
			project.addNewDiagram();
			project.setScale(QETProject::Scale::Mm);
			ConductorProperties properties;
			properties.m_bicolor = true;
			properties.m_dash_size = 4; //px
			ConductorPropertiesWidget widget(properties);
			widget.setProject(&project);
			auto *box = widget.findChild<QDoubleSpinBox *>(QStringLiteral("m_dash_size_sb"));
			QVERIFY(box);
			QVERIFY(box->suffix().contains(QStringLiteral("mm")));
			QCOMPARE(box->value(), 1.0);
				//Untouched: what was stored comes back
			QCOMPARE(widget.properties().m_dash_size, 4);
			box->setValue(0.75);
			QCOMPARE(widget.properties().m_dash_size, 3);

				//Back to px: the same size, as before
			project.setScale(QETProject::Scale::Px);
			widget.setProject(&project);
			QCOMPARE(box->suffix(), QStringLiteral("px"));
			QCOMPARE(box->value(), 3.0);
			QCOMPARE(widget.properties().m_dash_size, 3);

				//No project: px
			ConductorPropertiesWidget defaults(properties);
			auto *plain = defaults.findChild<QDoubleSpinBox *>(QStringLiteral("m_dash_size_sb"));
			QCOMPARE(plain->suffix(), QStringLiteral("px"));
			QCOMPARE(plain->value(), 4.0);
			QCOMPARE(defaults.properties().m_dash_size, 4);
		}

			//The suffix a box gets back in px is the one of the .ui, which
			//has a translation: a translator for "px" of the widget
			//changes both
		void secondaryLineSuffixIsTranslated()
		{
			struct Translator : QTranslator
			{
				QString translate(const char *context, const char *source,
								  const char *disambiguation, int n) const override
				{
					Q_UNUSED(disambiguation)
					Q_UNUSED(n)
					return QLatin1String(context) == QLatin1String("ConductorPropertiesWidget")
							&& QLatin1String(source) == QLatin1String("px")
						? QStringLiteral("pkt") : QString();
				}
				bool isEmpty() const override { return false; }
			} translator;
			QCoreApplication::installTranslator(&translator);

			QETProject project;
			project.addNewDiagram();
			project.setScale(QETProject::Scale::Mm);
			ConductorProperties properties;
			properties.m_bicolor = true;
			ConductorPropertiesWidget widget(properties);
			auto *box = widget.findChild<QDoubleSpinBox *>(QStringLiteral("m_dash_size_sb"));
			QVERIFY(box);
			const QString from_ui = box->suffix();
			QCOMPARE(from_ui, QStringLiteral("pkt"));
			widget.setProject(&project);
			QVERIFY(box->suffix().contains(QStringLiteral("mm")));
			project.setScale(QETProject::Scale::Px);
			widget.setProject(&project);
			QCOMPARE(box->suffix(), from_ui);
			QCoreApplication::removeTranslator(&translator);
		}

		void widgetFollowsTheProject()
		{
			QETProject project;
			project.addNewDiagram();
			ConductorProperties properties;
			properties.cond_size = 2;
			ConductorPropertiesWidget widget(properties);
			widget.setProject(&project);
			QCOMPARE(sizeBox(widget)->value(), 2.0);
			project.setScale(QETProject::Scale::Mm);
			widget.setProject(&project);
			QCOMPARE(sizeBox(widget)->value(), 0.5);
			QCOMPARE(widget.properties().cond_size, 2.0);
		}

		void thinSymbolInMmProject()
		{
			Diagram *diagram = QetAppTest::diagram();
			QVERIFY(diagram);
			QETProject *project = diagram->project();
			QVERIFY(project);
			QDomDocument doc;
			doc.setContent(QStringLiteral(
				"<definition type=\"element\" link_type=\"simple\" version=\"0.80\""
				" width=\"100\" height=\"20\" hotspot_x=\"50\" hotspot_y=\"10\">"
				"<uuid uuid=\"{3a1c0a4e-6f0b-4b8e-9d52-7a1d0c2f9e11}\"/>"
				"<names><name lang=\"en\">thin</name></names><description>"
				"<line x1=\"-40\" y1=\"0\" x2=\"40\" y2=\"0\" end1=\"none\" end2=\"none\""
				" length1=\"1.5\" length2=\"1.5\""
				" style=\"line-style:normal;line-weight:thin;filling:none;color:black\"/>"
				"<terminal x=\"0\" y=\"0\" orientation=\"n\"/>"
				"</description></definition>"));
			QVERIFY(project->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("thinline"), doc.documentElement()));

			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/thinline.elmt"), project),
				nullptr, &state);
			QVERIFY(element && !state);
			diagram->addItem(element);

			QCOMPARE(project->scale(), QETProject::Scale::Px);
			QVERIFY(!element->thinWeightInMm());
			const qint64 px_ink = ink(element);
			QVERIFY(px_ink > 0);

				//The same folio drawn to scale: 0.72 px instead of 0.5 px
			project->setScale(QETProject::Scale::Mm);
			QVERIFY(element->thinWeightInMm());
			const qint64 mm_ink = ink(element);
			QVERIFY2(mm_ink > px_ink * 1.3 && mm_ink < px_ink * 1.6,
					 qPrintable(QStringLiteral("px ink %1, mm ink %2").arg(px_ink).arg(mm_ink)));

				//and back
			project->setScale(QETProject::Scale::Px);
			QVERIFY(!element->thinWeightInMm());
			QCOMPARE(ink(element), px_ink);

			diagram->removeItem(element);
			delete element;
		}

		void thinPenFollowsTheModule_data()
		{
			QTest::addColumn<qreal>("module");
			QTest::newRow("M 1.8") << 1.8;
			QTest::newRow("M 2.5") << 2.5;
			QTest::newRow("M 5.0") << 5.0;
		}

			//0.18 mm is 0.18 * 10 / M px; projects with another M can be
			//open together (one picture cache), and changing M in an open
			//project redraws its symbols
		void thinPenFollowsTheModule()
		{
			QFETCH(qreal, module);
			QETProject first;
			Diagram *diagram = first.addNewDiagram();
			first.setScale(QETProject::Scale::Mm);
			QDomDocument doc;
			doc.setContent(QStringLiteral(
				"<definition type=\"element\" link_type=\"simple\" version=\"0.80\""
				" width=\"100\" height=\"20\" hotspot_x=\"50\" hotspot_y=\"10\">"
				"<uuid uuid=\"{3a1c0a4e-6f0b-4b8e-9d52-7a1d0c2f9e12}\"/>"
				"<names><name lang=\"en\">thin</name></names><description>"
				"<line x1=\"-40\" y1=\"0\" x2=\"40\" y2=\"0\" end1=\"none\" end2=\"none\""
				" length1=\"1.5\" length2=\"1.5\""
				" style=\"line-style:normal;line-weight:thin;filling:none;color:black\"/>"
				"<terminal x=\"0\" y=\"0\" orientation=\"n\"/>"
				"</description></definition>"));
			QVERIFY(first.embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("thinline"), doc.documentElement()));
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/thinline.elmt"), &first),
				nullptr, &state);
			QVERIFY(element && !state);
			diagram->addItem(element);
			QVERIFY(qAbs(element->thinWeightPx() - 0.18 * 4) < 1e-9);
			const qint64 default_ink = ink(element);

			first.setModuleMm(module);
			QVERIFY(qAbs(element->thinWeightPx() - 0.18 * 10 / module) < 1e-9);
			QVERIFY(qAbs(ElementPictureFactory::thinWidth(element->thinWeightPx())
				     - 0.18 * 10 / module) < 1e-9);
			const qint64 ink_now = ink(element);
				//the ink of a line follows its width
			QVERIFY2(qAbs(double(ink_now) / default_ink - 2.5 / module) < 0.12,
				 qPrintable(QStringLiteral("%1 / %2").arg(ink_now).arg(default_ink)));

				//Two projects with other modules, one drawing each
			QETProject second;
			Diagram *other_diagram = second.addNewDiagram();
			second.setScale(QETProject::Scale::Mm);
			second.setModuleMm(5.0 == module ? 1.8 : 5.0);
			QVERIFY(second.embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("thinline"), doc.documentElement()));
			Element *other = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/thinline.elmt"), &second),
				nullptr, &state);
			QVERIFY(other && !state);
			other_diagram->addItem(other);
			QVERIFY(qAbs(other->thinWeightPx() - 0.18 * 10 / second.moduleMm()) < 1e-9);
			QVERIFY(qAbs(element->thinWeightPx() - 0.18 * 10 / module) < 1e-9);
			QCOMPARE(ink(element), ink_now);

			diagram->removeItem(element);
			delete element;
			other_diagram->removeItem(other);
			delete other;
		}
};

QET_APP_TEST_MAIN(tst_mmscalelines)

#include "tst_mmscalelines.moc"
