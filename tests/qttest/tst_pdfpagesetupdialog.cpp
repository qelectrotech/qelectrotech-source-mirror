// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QPrinter>
#include <QRadioButton>

#include "print/pdfpagesetupdialog.h"

// The page layout dialog of the PDF export on Windows and macOS: it shows
// the layout it is given, and gives back the paper size, orientation and
// margins chosen in it, leaving the rest of the layout alone.
class tst_pdfpagesetupdialog : public QObject
{
	Q_OBJECT

	static QPageLayout a4Landscape()
	{
		return QPageLayout(QPageSize(QPageSize::A4),
				   QPageLayout::Landscape,
				   QMarginsF(10, 15, 20, 25),
				   QPageLayout::Millimeter);
	}

	template <typename T>
	static T *child(PdfPageSetupDialog &d, const char *name)
	{
		auto w = d.findChild<T *>(QString::fromLatin1(name));
		if (!w)
			qFatal("no child named %s", name);
		return w;
	}

	static QMarginsF marginsMM(const QPageLayout &layout)
	{
		return layout.margins(QPageLayout::Millimeter);
	}

	private slots:
		void unchangedDialogGivesBackTheSameLayout()
		{
			PdfPageSetupDialog d(a4Landscape());
			const QPageLayout l = d.pageLayout();
			QCOMPARE(l.pageSize().id(), QPageSize::A4);
			QCOMPARE(l.orientation(), QPageLayout::Landscape);
			QCOMPARE(marginsMM(l), QMarginsF(10, 15, 20, 25));
			QCOMPARE(l.mode(), QPageLayout::StandardMode);
		}

		void chosenValuesAreGivenBack()
		{
			PdfPageSetupDialog d(a4Landscape());
			auto size = child<QComboBox>(d, "size");
			size->setCurrentIndex(size->findData(int(QPageSize::A3)));
			child<QRadioButton>(d, "portrait")->setChecked(true);
			child<QDoubleSpinBox>(d, "left")->setValue(30);
			child<QDoubleSpinBox>(d, "top")->setValue(5);
			child<QDoubleSpinBox>(d, "right")->setValue(7.5);
			child<QDoubleSpinBox>(d, "bottom")->setValue(0);

			const QPageLayout l = d.pageLayout();
			QCOMPARE(l.pageSize().id(), QPageSize::A3);
			QCOMPARE(l.orientation(), QPageLayout::Portrait);
			QCOMPARE(marginsMM(l), QMarginsF(30, 5, 7.5, 0));
				// The margins really are taken, not refused
			QCOMPARE(l.paintRect(QPageLayout::Millimeter).width(),
				 297.0 - 30 - 7.5);
		}

		void marginsInPointsAreShownInMillimetres()
		{
			const QPageLayout points(QPageSize(QPageSize::A4),
						 QPageLayout::Portrait,
						 QMarginsF(72, 0, 0, 0),
						 QPageLayout::Point);
			PdfPageSetupDialog d(points);
			QCOMPARE(qRound(child<QDoubleSpinBox>(d, "left")->value() * 10),
				 254); // one inch
		}

		void customSize()
		{
			PdfPageSetupDialog d(a4Landscape());
			auto size = child<QComboBox>(d, "size");
			auto width = child<QDoubleSpinBox>(d, "width");
			QVERIFY(!width->isEnabled());
			size->setCurrentIndex(size->findData(int(QPageSize::Custom)));
			QVERIFY(width->isEnabled());
			width->setValue(300);
			child<QDoubleSpinBox>(d, "height")->setValue(200);
			child<QRadioButton>(d, "portrait")->setChecked(true);

			const QPageLayout l = d.pageLayout();
			QCOMPARE(l.pageSize().id(), QPageSize::Custom);
			const QSizeF mm = l.pageSize().size(QPageSize::Millimeter);
			QCOMPARE(qRound(mm.width()), 300);
			QCOMPARE(qRound(mm.height()), 200);
		}

		void sizeOutsideTheListIsKept()
		{
			const QPageLayout b0(QPageSize(QPageSize::B0),
					     QPageLayout::Portrait, QMarginsF());
			PdfPageSetupDialog d(b0);
			QCOMPARE(d.pageLayout().pageSize().id(), QPageSize::B0);
		}

		void marginsLeaveRoomOnThePage()
		{
			PdfPageSetupDialog d(a4Landscape()); // 297 mm wide
			auto left = child<QDoubleSpinBox>(d, "left");
			auto right = child<QDoubleSpinBox>(d, "right");
			left->setValue(200);
			QCOMPARE(right->maximum(), 297.0 - 200 - 10);
			right->setValue(500);
			QCOMPARE(right->value(), 87.0);

				// A smaller page brings the margins down with it
			auto size = child<QComboBox>(d, "size");
			size->setCurrentIndex(size->findData(int(QPageSize::A6)));
			child<QRadioButton>(d, "portrait")->setChecked(true);
			const QPageLayout l = d.pageLayout();
			QVERIFY(l.paintRect(QPageLayout::Millimeter).width() >= 10);
		}

		void pdfPrinterTakesTheLayout()
		{
			QPrinter printer(QPrinter::HighResolution);
			printer.setOutputFormat(QPrinter::PdfFormat);
			PdfPageSetupDialog d(printer.pageLayout());
			auto size = child<QComboBox>(d, "size");
			size->setCurrentIndex(size->findData(int(QPageSize::A3)));
			child<QRadioButton>(d, "landscape")->setChecked(true);
			child<QDoubleSpinBox>(d, "left")->setValue(25);
			child<QDoubleSpinBox>(d, "top")->setValue(12);
			child<QDoubleSpinBox>(d, "right")->setValue(8);
			child<QDoubleSpinBox>(d, "bottom")->setValue(16);

			QVERIFY(printer.setPageLayout(d.pageLayout()));
			const QPageLayout l = printer.pageLayout();
			QCOMPARE(l.pageSize().id(), QPageSize::A3);
			QCOMPARE(l.orientation(), QPageLayout::Landscape);
			QCOMPARE(marginsMM(l), QMarginsF(25, 12, 8, 16));
		}

		void fullPageModeIsKept()
		{
			QPageLayout full = a4Landscape();
			full.setMode(QPageLayout::FullPageMode);
			PdfPageSetupDialog d(full);
			QVERIFY(!child<QGroupBox>(d, "margins")->isEnabled());
			QCOMPARE(d.pageLayout().mode(), QPageLayout::FullPageMode);
		}
};

QTEST_MAIN(tst_pdfpagesetupdialog)
#include "tst_pdfpagesetupdialog.moc"
