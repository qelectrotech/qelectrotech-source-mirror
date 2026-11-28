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
#include "ui/borderpropertieswidget.h"
#include "qetapp.h"
#include "titleblocktemplate.h"
#include "qetunits.h"

#include <QtTest>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>

TitleBlockTemplate *QETApp::defaultTitleBlockTemplate() { return nullptr; }
int TitleBlockTemplate::height() const { return 50; }

/**
	Issue 1662: choosing A4, A3 and A4 again in the folio properties gave
	different columns and rows the second time, because each paper size
	was fitted from the cells the previous one left.
*/
class tst_borderpropertiespaper : public QObject
{
	Q_OBJECT

	private slots:
	void a4_a3_a4_gives_a4_back()
	{
		BorderProperties bp;
		BorderPropertiesWidget w(bp);
		w.setTitleBlockSize(50, Qt::BottomEdge, Qt::AlignTop);
		auto *paper = w.findChild<QComboBox *>("m_paper_cb");
		auto *orient = w.findChild<QComboBox *>("m_orientation_cb");
		auto *cw = w.findChild<QSpinBox *>("m_columns_width_sp");
		auto *rh = w.findChild<QSpinBox *>("m_rows_height_sp");
		auto *cc = w.findChild<QSpinBox *>("m_colums_count_sp");
		auto *rc = w.findChild<QSpinBox *>("m_rows_count_sp");
		QVERIFY(paper && orient && cw && rh && cc && rc);

		auto pick = [&](const char *name, int o) {
			paper->setCurrentIndex(paper->findText(name));
			orient->setCurrentIndex(o);
			emit paper->activated(paper->currentIndex());
		};
		int bad = 0;
		for (int width = 20 ; width <= 80 ; width += 3)
			for (int height = 20 ; height <= 90 ; height += 3)
			{
				cw->setValue(width);
				rh->setValue(height);
				pick("A4", 1);
				const QList<int> first{cc->value(), cw->value(), rc->value(), rh->value()};
				pick("A3", 1);
				pick("A4", 1);
				const QList<int> again{cc->value(), cw->value(), rc->value(), rh->value()};
				if (again != first) ++bad;
			}
		QCOMPARE(bad, 0);
	}

	/// Issue 1662: a folio with no paper size says which paper holds it
	/// and how much is left, so the last row can end at the title block
	void custom_folio_reports_free_space()
	{
		BorderProperties bp;
		BorderPropertiesWidget w(bp);
		w.setTitleBlockSize(50, Qt::BottomEdge, Qt::AlignTop);
		auto *label = w.findChildren<QLabel *>().last();
		auto *cw = w.findChild<QSpinBox *>("m_columns_width_sp");
		auto *rh = w.findChild<QSpinBox *>("m_rows_height_sp");
		cw->setValue(41);
		rh->setValue(47);
		QVERIFY2(label->text().contains("Smallest paper"), qPrintable(label->text()));
		QVERIFY2(label->text().contains("Folio size:"), qPrintable(label->text()));
	}

	/// Step 2 of docs/mm-scale.md: in a project drawn to scale the paper
	/// is 4 px/mm (A3 landscape 1680 x 1188), otherwise 96 dpi as before,
	/// and the folio is recognised as that paper in either
	void paper_follows_pixels_per_mm()
	{
		struct Case { qreal px_per_mm; int width; int height; const char *size; };
		for (const Case &c : {Case{0, 1587, 1123, "Folio size: 4"}, Case{QetUnits::defaultPxPerMm, 1680, 1188, "Folio size: 4"}})
		{
			BorderProperties bp;
			BorderPropertiesWidget w(bp);
			w.setPxPerMm(c.px_per_mm);
			w.setTitleBlockSize(50, Qt::BottomEdge, Qt::AlignTop);
			auto *paper = w.findChild<QComboBox *>("m_paper_cb");
			auto *orient = w.findChild<QComboBox *>("m_orientation_cb");
			auto *cw = w.findChild<QSpinBox *>("m_columns_width_sp");
			auto *rh = w.findChild<QSpinBox *>("m_rows_height_sp");
			auto *cc = w.findChild<QSpinBox *>("m_colums_count_sp");
			auto *rc = w.findChild<QSpinBox *>("m_rows_count_sp");
			paper->setCurrentIndex(paper->findText("A3"));
			orient->setCurrentIndex(0);
			emit paper->activated(paper->currentIndex());

				//Header 20 x 20 px, the line, the title block below
			const BorderProperties &p = w.properties();
			QVERIFY(c.width - (qRound(p.rows_header_width) + 1 + cc->value() * cw->value()) <= 3);
			QVERIFY(c.height - 50 - (qRound(p.columns_header_height) + 1 + rc->value() * rh->value()) <= 3);

				//Changing nothing, the paper is found again
			w.setProperties(w.properties());
			QCOMPARE(paper->currentText(), QString("A3"));
			QCOMPARE(orient->currentIndex(), 0);
			auto *label = w.findChildren<QLabel *>().last();
			QVERIFY2(label->text().contains(QString::fromUtf8(c.size)), qPrintable(label->text()));
		}
	}

	/// The paper follows the module M of the project: A3 landscape is
	/// 420 * 10 / M px wide
	void paper_follows_the_module_data()
	{
		QTest::addColumn<qreal>("module");
		QTest::newRow("M 1.8") << 1.8;
		QTest::newRow("M 2.5") << 2.5;
		QTest::newRow("M 5.0") << 5.0;
	}

	void paper_follows_the_module()
	{
		QFETCH(qreal, module);
		BorderProperties bp;
		BorderPropertiesWidget w(bp);
		w.setPxPerMm(QetUnits::pxPerMmFor(module));
		w.setTitleBlockSize(50, Qt::BottomEdge, Qt::AlignTop);
		auto *paper = w.findChild<QComboBox *>("m_paper_cb");
		auto *orient = w.findChild<QComboBox *>("m_orientation_cb");
		auto *cw = w.findChild<QSpinBox *>("m_columns_width_sp");
		auto *cc = w.findChild<QSpinBox *>("m_colums_count_sp");
		paper->setCurrentIndex(paper->findText("A3"));
		orient->setCurrentIndex(0);
		emit paper->activated(paper->currentIndex());
		const int width = qRound(420 * 10 / module);
		const BorderProperties &p = w.properties();
		const int used = qRound(p.rows_header_width) + 1 + cc->value() * cw->value();
		QVERIFY2(width - used >= 0 && width - used <= 3,
			 qPrintable(QStringLiteral("%1 of %2").arg(used).arg(width)));
	}

	/// The PDF export snaps a page within 3 pt of a paper to that paper, so
	/// what a folio leaves of the paper has to stay under 3 pt, however
	/// many pixels that is: at M = 5 one px is 1.4 pt, 3 px are 4.3 pt
	void leftover_stays_under_the_pdf_snap_data()
	{
		QTest::addColumn<qreal>("module");
		QTest::newRow("M 1.8") << 1.8;
		QTest::newRow("M 2.5") << 2.5;
		QTest::newRow("M 5.0") << 5.0;
	}

	void leftover_stays_under_the_pdf_snap()
	{
		QFETCH(qreal, module);
		const qreal px_per_mm = QetUnits::pxPerMmFor(module);
		const qreal pt_per_px = 72.0 / (px_per_mm * 25.4);
		BorderProperties bp;
		BorderPropertiesWidget w(bp);
		w.setPxPerMm(px_per_mm);
		w.setTitleBlockSize(50, Qt::BottomEdge, Qt::AlignTop);
		auto *paper = w.findChild<QComboBox *>("m_paper_cb");
		auto *orient = w.findChild<QComboBox *>("m_orientation_cb");
		auto *cw = w.findChild<QSpinBox *>("m_columns_width_sp");
		auto *rh = w.findChild<QSpinBox *>("m_rows_height_sp");
		auto *cc = w.findChild<QSpinBox *>("m_colums_count_sp");
		auto *rc = w.findChild<QSpinBox *>("m_rows_count_sp");
		int bad = 0;
		for (const char *name : {"A3"})
			for (const QSize &start : {QSize(24, 24), QSize(60, 80), QSize(50, 50), QSize(30, 40)})
				{
					const int width = start.width();
					const int height = start.height();
					cw->setValue(width);
					rh->setValue(height);
					paper->setCurrentIndex(paper->findText(name));
					orient->setCurrentIndex(0);
					emit paper->activated(paper->currentIndex());
					const BorderProperties &p = w.properties();
					const bool a3 = QLatin1String(name) == QLatin1String("A3");
					const int page_w = qRound((a3 ? 420 : 297) * px_per_mm);
					const int page_h = qRound((a3 ? 297 : 210) * px_per_mm);
					const int left_w = page_w - (qRound(p.rows_header_width) + 1 + cc->value() * cw->value());
					const int left_h = page_h - 50 - (qRound(p.columns_header_height) + 1 + rc->value() * rh->value());
					if (left_w < 0 || left_h < 0
						|| left_w * pt_per_px >= 3 || left_h * pt_per_px >= 3)
						++bad;
				}
		QCOMPARE(bad, 0);
	}

	/// A folio laid out for the other scale is a custom size
	void paper_of_other_scale_is_custom()
	{
		BorderProperties bp;
		BorderPropertiesWidget w(bp);
		w.setTitleBlockSize(50, Qt::BottomEdge, Qt::AlignTop);
		auto *paper = w.findChild<QComboBox *>("m_paper_cb");
		paper->setCurrentIndex(paper->findText("A3"));
		emit paper->activated(paper->currentIndex());
		w.setProperties(w.properties());
		QCOMPARE(paper->currentText(), QString("A3"));
		w.setPxPerMm(QetUnits::defaultPxPerMm);
		QCOMPARE(paper->currentText(), QString("Custom"));
		w.setPxPerMm(0);
		QCOMPARE(paper->currentText(), QString("A3"));
	}
};

QTEST_MAIN(tst_borderpropertiespaper)
#include "tst_borderpropertiespaper.moc"
