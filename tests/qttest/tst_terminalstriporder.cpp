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

#include "TerminalStrip/UndoCommand/sortterminalstripcommand.h"
#include "TerminalStrip/physicalterminal.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "diagram.h"
#include "properties/elementdata.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "utils/qetutils.h"

#include <QStandardPaths>

/**
	The terminal strip sorts into the plan's own order -- folio by folio,
	then left to right -- instead of by label, so "X1:N" and "X1:PE"
	stand where they are drawn rather than at the end of the strip. The
	auto import and SortTerminalStripCommand share the one comparator
	(SortTerminalStripCommand::planOrderLessThan), so the order the
	import works out cannot be rewritten by the sort it then pushes for
	the strip it just created.

	fixtures/every_feature.qet has the strip X1 with element 891b70f2
	(x=300) left of 4861e268 (x=360) on the first folio, and 4861e268
	holds an empty label where 891b70f2 has a non-empty one: a label
	sort would put 4861e268 first, which is the very swap this test
	pins down.
*/
class tst_terminalstriporder : public QObject
{
	Q_OBJECT

	QETProject *m_project = nullptr;

	Element *element(const QString &uuid) const
	{
		for (Diagram *d_ : m_project->diagrams()) {
			for (Element *e_ : d_->elements()) {
				if (e_->uuid().toString() == uuid) {
					return e_;
				}
			}
		}
		return nullptr;
	}

	TerminalStrip *strip() const
	{
		const auto strips_ = m_project->terminalStrip();
		return strips_.isEmpty() ? nullptr : strips_.first();
	}

	QSharedPointer<RealTerminal> terminalInStrip(const QString &element_uuid) const
	{
		auto *strip_ = strip();
		if (!strip_) {
			return {};
		}
		for (const auto &real_ : strip_->realTerminals()) {
			if (real_ && real_->element()
				&& real_->element()->uuid().toString() == element_uuid) {
				return real_;
			}
		}
		return {};
	}

	/// The element uuids of the strip's physical positions, in strip order.
	QStringList physicalOrder() const
	{
		QStringList order_;
		auto *strip_ = strip();
		if (!strip_) {
			return order_;
		}
		for (const auto &physical_ : strip_->physicalTerminal()) {
			if (physical_ && physical_->realTerminalCount()
				&& physical_->realTerminals().first()->element()) {
				order_ << physical_->realTerminals().first()->element()->uuid().toString();
			}
		}
		return order_;
	}

private slots:
	void init();
	void cleanup();

	void positionBeatsLabel();
	void folioComesBeforePosition();
	void spareWithoutFolioGoesLast();
	void sortKeepsThePlanOrder();
	void sortFollowsMovedElements();
};

void tst_terminalstriporder::init()
{
	const QString fixture_ = QFINDTESTDATA("fixtures/every_feature.qet");
	QVERIFY(!fixture_.isEmpty());
	m_project = new QETProject(fixture_);
	QCOMPARE(m_project->state(), QETProject::Ok);
}

void tst_terminalstriporder::cleanup()
{
	delete m_project;
	m_project = nullptr;
}

/**
	Two terminals of one folio, 891b70f2 left of 4861e268: the position
	decides, and the label says the opposite (4861e268's is empty), so a
	comparator by label would order them the wrong way round.
*/
void tst_terminalstriporder::positionBeatsLabel()
{
	const auto a_ = terminalInStrip(QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
	const auto b_ = terminalInStrip(QStringLiteral("{4861e268-d344-413e-a86f-034ea5ada16c}"));
	QVERIFY(a_);
	QVERIFY(b_);
	QCOMPARE(a_->element()->pos().x() < b_->element()->pos().x(), true);

	QVERIFY(SortTerminalStripCommand::planOrderLessThan(a_, b_));
	QVERIFY(!SortTerminalStripCommand::planOrderLessThan(b_, a_));

		//The labels would sort the other way round (b's is empty), which
		//is exactly the order the plan must NOT follow
	QVERIFY(QETUtils::naturalLessThan(b_->label(), a_->label()));
}

/**
	The page comes before the position ("page by page, then left to
	right"): a terminal moved to the second folio sorts after every
	terminal of the first one, even when it stands further left than
	them. The element is put back on its own folio before the test
	ends, so the project is left as it was.
*/
void tst_terminalstriporder::folioComesBeforePosition()
{
	const auto a_ = terminalInStrip(QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
	const auto b_ = terminalInStrip(QStringLiteral("{4861e268-d344-413e-a86f-034ea5ada16c}"));
	QVERIFY(a_);
	QVERIFY(b_);
	Element *moved_ = a_->element();
	QVERIFY(moved_);
	const QList<Diagram *> folios_ = m_project->diagrams();
	QVERIFY(folios_.size() > 1);
	Diagram *own_folio_ = moved_->diagram();
	Diagram *later_folio_ = folios_.at(1);
	QVERIFY(later_folio_ != own_folio_);
	const QPointF original_pos_ = moved_->pos();

		//Put a on the second folio, left of where b stands on the first
	own_folio_->removeItem(moved_);
	later_folio_->addItem(moved_);
	moved_->setPos(QPointF(50, 600));
	QVERIFY(moved_->diagram() == later_folio_);

	const bool folio_later_ = later_folio_->folioIndex() > own_folio_->folioIndex();
	const bool a_left_of_b_ = moved_->pos().x() < b_->element()->pos().x();
	const bool b_first_ = SortTerminalStripCommand::planOrderLessThan(b_, a_);
	const bool a_first_ = SortTerminalStripCommand::planOrderLessThan(a_, b_);

		//Back to where it was, so the conductors drawn to it and the
		//project's cleanup see the folio they were loaded from
	later_folio_->removeItem(moved_);
	own_folio_->addItem(moved_);
	moved_->setPos(original_pos_);
	QVERIFY(moved_->diagram() == own_folio_);

	QVERIFY(folio_later_);
	QVERIFY(a_left_of_b_);		//the position alone would put a first
	QVERIFY(b_first_);			//the page decides: b (first folio) first
	QVERIFY(!a_first_);
}

/**
	A spare terminal stands on no folio: it has no place in the plan
	order and goes last, whatever its label would say.
*/
void tst_terminalstriporder::spareWithoutFolioGoesLast()
{
	ElementData data_;
	data_.m_informations.addValue(QStringLiteral("label"),
								  QStringLiteral("AAA"));
	const auto spare_ = RealTerminal::createSpare(data_);
	QVERIFY(spare_);
	QVERIFY(spare_->isSpare());

	const auto a_ = terminalInStrip(QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
	QVERIFY(a_);
		//"AAA" would sort first by label alone
	QVERIFY(QETUtils::naturalLessThan(spare_->label(), a_->label()));

	QVERIFY(SortTerminalStripCommand::planOrderLessThan(a_, spare_));
	QVERIFY(!SortTerminalStripCommand::planOrderLessThan(spare_, a_));
}

/**
	The sort command, run on the fixture as it stands, keeps the strip
	the way the plan is drawn: 891b70f2 (x=300) before 4861e268
	(x=360). The old label sort swapped them, since 4861e268's label is
	the empty one.
*/
void tst_terminalstriporder::sortKeepsThePlanOrder()
{
	auto *strip_ = strip();
	QVERIFY(strip_);
	QCOMPARE(strip_->physicalTerminalCount(), 2);

	SortTerminalStripCommand command_(strip_);
	command_.redo();

	QCOMPARE(physicalOrder(),
			 QStringList({
				 QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"),
				 QStringLiteral("{4861e268-d344-413e-a86f-034ea5ada16c}"),
			 }));
}

/**
	The order follows the plan, not the strip's history: once the two
	elements are swapped on the folio, sorting puts the strip the other
	way round too.
*/
void tst_terminalstriporder::sortFollowsMovedElements()
{
	auto *strip_ = strip();
	QVERIFY(strip_);
	Element *a_ = element(QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
	Element *b_ = element(QStringLiteral("{4861e268-d344-413e-a86f-034ea5ada16c}"));
	QVERIFY(a_);
	QVERIFY(b_);

	b_->setPos(100, 400);
	a_->setPos(700, 400);

	SortTerminalStripCommand command_(strip_);
	command_.redo();

	QCOMPARE(physicalOrder(),
			 QStringList({
				 QStringLiteral("{4861e268-d344-413e-a86f-034ea5ada16c}"),
				 QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"),
			 }));
}

int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	return QetAppTest::run<tst_terminalstriporder>(argc, argv);
}

#include "tst_terminalstriporder.moc"
