// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementpicturefactory.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QPicture>

// A symbol made by copying another one and redrawing it keeps the original's
// uuid. The drawings of symbols were cached by that uuid alone, so whichever
// of the two was drawn first was shown for both, on the folio and in the
// collection panel (#1789).
class tst_symbolpicturecache : public QObject
{
	Q_OBJECT

		/// A symbol with one terminal and the shape given, always with the same uuid.
	static QDomElement definition(QDomDocument &doc, const QString &shape)
	{
		doc.setContent(QStringLiteral(
			"<definition type=\"element\" link_type=\"simple\" version=\"0.80\""
			" width=\"100\" height=\"100\" hotspot_x=\"50\" hotspot_y=\"50\">"
			"<uuid uuid=\"{52169153-2558-21ab-63ec-96ce53bfc7c1}\"/>"
			"<names><name lang=\"en\">shared</name></names>"
			"<description>%1"
			"<terminal x=\"0\" y=\"0\" orientation=\"n\"/>"
			"</description></definition>").arg(shape));
		return doc.documentElement();
	}

	private slots:
		void sameUuidKeepsEachDrawing()
		{
			Diagram *diagram = QetAppTest::diagram();
			QVERIFY(diagram);
			QETProject *project = diagram->project();
			QVERIFY(project);

				// A small square, and a copy of it redrawn as a large circle.
			QDomDocument small_doc, large_doc;
			QVERIFY(project->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("arrow"),
				definition(small_doc, QStringLiteral(
					"<rect x=\"-5\" y=\"-5\" width=\"10\" height=\"10\""
					" style=\"line-style:normal;line-weight:normal;filling:none;color:black\""
					" antialias=\"false\"/>"))));
			QVERIFY(project->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("company_arrow"),
				definition(large_doc, QStringLiteral(
					"<ellipse x=\"-40\" y=\"-40\" width=\"80\" height=\"80\""
					" style=\"line-style:normal;line-weight:normal;filling:none;color:black\""
					" antialias=\"false\"/>"))));

			const ElementsLocation arrow(QStringLiteral("embed://import/arrow.elmt"), project);
			const ElementsLocation company(QStringLiteral("embed://import/company_arrow.elmt"), project);
			QVERIFY(!arrow.uuid().isNull());
			QCOMPARE(arrow.uuid(), company.uuid());

				// The circle is drawn first, as the company symbol was in #1789.
			ElementPictureFactory *factory = ElementPictureFactory::instance();
			QPicture picture, low;
			factory->getPictures(company, picture, low);
			const int circle_width = picture.boundingRect().width();
			factory->getPictures(arrow, picture, low);
			const int square_width = picture.boundingRect().width();

			QVERIFY2(circle_width >= 80,
				qPrintable(QStringLiteral("circle drawn %1 wide").arg(circle_width)));
			QVERIFY2(square_width <= 20,
				qPrintable(QStringLiteral("square drawn %1 wide: it got the other symbol's drawing")
					.arg(square_width)));
			QVERIFY2(factory->pixmap(arrow).toImage() != factory->pixmap(company).toImage(),
				"the collection panel shows one icon for both symbols");
		}
};

QET_APP_TEST_MAIN(tst_symbolpicturecache)

#include "tst_symbolpicturecache.moc"
