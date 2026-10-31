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
		w.setTitleBlockSize(50, Qt::BottomEdge);
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
		w.setTitleBlockSize(50, Qt::BottomEdge);
		auto *label = w.findChildren<QLabel *>().last();
		auto *cw = w.findChild<QSpinBox *>("m_columns_width_sp");
		auto *rh = w.findChild<QSpinBox *>("m_rows_height_sp");
		cw->setValue(41);
		rh->setValue(47);
		QVERIFY2(label->text().contains("Smallest paper"), qPrintable(label->text()));
		QVERIFY2(label->text().contains("Folio size:"), qPrintable(label->text()));
	}
};

QTEST_MAIN(tst_borderpropertiespaper)
#include "tst_borderpropertiespaper.moc"
