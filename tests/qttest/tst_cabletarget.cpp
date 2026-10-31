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

#include "TerminalStrip/GraphicsItem/realisticterminalstripdrawer.h"
#include "TerminalStrip/GraphicsItem/trueterminalstrip.h"
#include "TerminalStrip/physicalterminal.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "cable/cable.h"
#include "conductorproperties.h"
#include "diagram.h"
#include "properties/elementdata.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetinformation.h"
#include "qetproject.h"

#include <QImage>
#include <QPainter>
#include <QStandardPaths>

/**
	The terminal strip graphic names where a cable goes: Installation,
	Location and Label (BMK) of the first piece the wires of the cable
	land on, past the strip's own terminals, in plan order -- the same
	three fields the cable list computes for a target
	(cablelistrows.cpp), so the strip and the list can never disagree.
	The graphic draws them in a small box under the cable, the bundle
	staying together (RealTerminal::cableTarget()).

	Also here: the number of a wire must stay in the room left between
	its mark and the terminal -- it shrinks, and what does not fit is
	clipped, so it never runs into the drawing of the terminal.

	Also here: the box at the end of a cable is written at the font of
	the drawing -- the end zone the settings leave is what gives it the
	room, so three lines never end up squeezed into a tiny text. A
	target typed in by hand is written one line per field, Installation,
	Location, Label, the way the column shows them, and only a line
	break breaks it into fields: a "/" written inside a field is part of
	it, the box draws one line holding it. Two cables of one strip end
	close to each other, and their boxes are wider than the room between
	them: the box standing in the way of the other is set lower, its
	stub drawn longer, and the strip reserves the room for it too.

	Both drawing styles are covered: the schematic one and the
	realistic one (the strip seen from above), which paint the same
	cable through Helpers::drawCables().

	fixtures/every_feature.qet has the strip X1 (elements 891b70f2 and
	4861e268) and the contact fc9cc920 wired to 891b70f2 by the
	conductor 8bd16d7a; the test gives the contact the Installation,
	Location and Label it reads back. The conductor 584b452e joins the
	two strip terminals to each other. No conductor of the fixture
	belongs to a cable yet.
*/
class tst_cabletarget : public QObject
{
	Q_OBJECT

	QETProject *m_project = nullptr;

	Element *element(const QString &uuid) const
	{
		for (Diagram *diagram_ : m_project->diagrams()) {
			for (Element *element_ : diagram_->elements()) {
				if (element_->uuid().toString() == uuid) {
					return element_;
				}
			}
		}
		return nullptr;
	}

	QSharedPointer<RealTerminal> terminalInStrip(const QString &element_uuid) const
	{
		const auto strips_ = m_project->terminalStrip();
		if (strips_.isEmpty()) {
			return {};
		}
		for (const auto &real_ : strips_.first()->realTerminals()) {
			if (real_ && real_->element()
				&& real_->element()->uuid().toString() == element_uuid) {
				return real_;
			}
		}
		return {};
	}

	Conductor *conductor(const QString &uuid) const
	{
		for (Diagram *diagram_ : m_project->diagrams()) {
			for (Conductor *conductor_ : diagram_->conductors()) {
				if (conductor_ && conductor_->uuid().toString() == uuid) {
					return conductor_;
				}
			}
		}
		return nullptr;
	}

	/// Let the drawn wires of @a conductor_ belong to the cable @a cable_uuid.
	void claimCable(Conductor *conductor_, const QUuid &cable_uuid) const
	{
		auto properties_ = conductor_->properties();
		properties_.m_cable_uuid = cable_uuid;
		conductor_->setProperties(properties_);
	}

	/**
		Same setup for the value and the drawing test: the contact the
		strip's wire lands on carries the three fields the test reads
		back (the fixture's own block has none), the wire belongs to a
		new cable, and the terminal carries that cable's name the way
		the strip editor stores it -- without a name the strip draws no
		cable at all. @return false when the fixture is not the one this
		test was written for.
	*/
	bool wireCableToTheContact()
	{
		Element *contact_ = element(
			QStringLiteral("{fc9cc920-5822-421a-916e-a17100acdcf6}"));
		if (!contact_) {
			return false;
		}
		ElementData contact_data_{contact_->elementData()};
		contact_data_.m_informations.addValue(QETInformation::ELMT_PLANT,
											  QStringLiteral("Plant 7"));
		contact_data_.m_informations.addValue(QETInformation::ELMT_LOCATION,
											  QStringLiteral("Cabinet 7"));
		contact_data_.m_informations.addValue(QETInformation::ELMT_LABEL,
											  QStringLiteral("-S7"));
		contact_->setElementData(contact_data_);

		Cable *cable_ = m_project->newCable();
		Conductor *wired_ = conductor(
			QStringLiteral("{8bd16d7a-18c4-4fa0-9495-8dab65fd6408}"));
		if (!cable_ || !wired_) {
			return false;
		}
		claimCable(wired_, cable_->uuid());

			//The name of the cable on the terminal, the way the strip
			//editor stores it: without it the strip draws no cable at all
		Element *terminal_ = element(
			QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
		if (!terminal_) {
			return false;
		}
		ElementData terminal_data_{terminal_->elementData()};
		terminal_data_.m_informations.addValue(RealTerminal::cableInfoKey(),
											   QStringLiteral("X99"));
		terminal_->setElementData(terminal_data_);
		return true;
	}

private slots:
	void init();
	void cleanup();

	void noCableNoTarget();
	void targetIsTheOtherPiece();
	void ownStripIsNoTarget();
	void spareHasNoTarget();
	void paintsWithTargetBox();
	void wireLabelStaysOutOfTheTerminal();
	void targetBoxKeepsTheDrawingFont();
	void typedTargetIsDrawnInItsOwnLines();
	void boxesOfTwoCablesAreSetApart();
	void realisticStyleDrawsTheCable();
};

void tst_cabletarget::init()
{
	const QString fixture_ = QFINDTESTDATA("fixtures/every_feature.qet");
	QVERIFY(!fixture_.isEmpty());
	m_project = new QETProject(fixture_);
	QCOMPARE(m_project->state(), QETProject::Ok);
}

void tst_cabletarget::cleanup()
{
	delete m_project;
	m_project = nullptr;
}

/**
	Nothing to name when no wire of the terminal belongs to a cable and
	no cable of the project is drawn under a name the terminal carries.
*/
void tst_cabletarget::noCableNoTarget()
{
	const auto a_ = terminalInStrip(QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
	const auto b_ = terminalInStrip(QStringLiteral("{4861e268-d344-413e-a86f-034ea5ada16c}"));
	QVERIFY(a_);
	QVERIFY(b_);
	QVERIFY(a_->cable().isEmpty());
	QVERIFY(a_->cableTarget().isEmpty());
	QVERIFY(b_->cableTarget().isEmpty());
}

/**
	The cable of the strip lands on the contact past it: the three
	fields of that piece, in the order the cable list shows them
	(Installation, Location, Label), and nothing of the strip itself.
*/
void tst_cabletarget::targetIsTheOtherPiece()
{
	QVERIFY(wireCableToTheContact());

	const auto a_ = terminalInStrip(QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
	QVERIFY(a_);
	QCOMPARE(a_->cableTarget(),
			 QVector<QString>({
				 QStringLiteral("Plant 7"),
				 QStringLiteral("Cabinet 7"),
				 QStringLiteral("-S7"),
			 }));

		//The other strip terminal carries no claimed wire of its own
		//(584b452e joins it to a_ and is not claimed) and no cable name,
		//so it has no target to name
	const auto b_ = terminalInStrip(QStringLiteral("{4861e268-d344-413e-a86f-034ea5ada16c}"));
	QVERIFY(b_);
	QVERIFY(b_->cableTarget().isEmpty());
}

/**
	A cable whose wires only ever join the terminals of the strip
	itself lands on nothing past the strip: no target, no box.
*/
void tst_cabletarget::ownStripIsNoTarget()
{
	Cable *cable_ = m_project->newCable();
	QVERIFY(cable_);
	Conductor *inside_ = conductor(
		QStringLiteral("{584b452e-a885-491a-98f1-fd348610b5e0}"));
	QVERIFY(inside_);
	claimCable(inside_, cable_->uuid());

	const auto a_ = terminalInStrip(QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
	const auto b_ = terminalInStrip(QStringLiteral("{4861e268-d344-413e-a86f-034ea5ada16c}"));
	QVERIFY(a_);
	QVERIFY(b_);
	QVERIFY(a_->cableTarget().isEmpty());
	QVERIFY(b_->cableTarget().isEmpty());
}

/**
	A spare terminal stands on no folio and is wired to nothing: it has
	no target to name either.
*/
void tst_cabletarget::spareHasNoTarget()
{
	ElementData data_;
	data_.m_informations.addValue(QStringLiteral("label"),
								  QStringLiteral("AAA"));
	const auto spare_ = RealTerminal::createSpare(data_);
	QVERIFY(spare_);
	QVERIFY(spare_->isSpare());
	QVERIFY(spare_->cableTarget().isEmpty());
}

/**
	The strip graphic draws the box: the same strip painted with
	something to name at the end of the cable and painted with nothing
	to name come out as two different pictures -- the box is the
	difference. Painting it also runs the whole drawCables() path.
*/
void tst_cabletarget::paintsWithTargetBox()
{
	QVERIFY(wireCableToTheContact());

	const auto strips_ = m_project->terminalStrip();
	QVERIFY(!strips_.isEmpty());
	TerminalStripDrawer::TerminalStripDrawer drawer{
		QSharedPointer<TerminalStripDrawer::TrueTerminalStrip>{
			new TerminalStripDrawer::TrueTerminalStrip{strips_.first()}},
		QSharedPointer<TerminalStripLayoutPattern>{new TerminalStripLayoutPattern}};

	const QRectF bounds{drawer.boundingRect()};
	QVERIFY(!bounds.isEmpty());

	const auto render = [&drawer, &bounds]() {
		QImage image{bounds.size().toSize() + QSize{4, 4},
					 QImage::Format_ARGB32_Premultiplied};
		image.fill(Qt::transparent);
		QPainter painter{&image};
		painter.translate(-bounds.topLeft() + QPointF{2, 2});
		drawer.paint(&painter);
		return image;
	};

	const QImage with_box{render()};

		//Nothing left to name at the far end of the cable
	Element *contact_ = element(
		QStringLiteral("{fc9cc920-5822-421a-916e-a17100acdcf6}"));
	QVERIFY(contact_);
	ElementData contact_data_{contact_->elementData()};
	contact_data_.m_informations.addValue(QETInformation::ELMT_PLANT, QString());
	contact_data_.m_informations.addValue(QETInformation::ELMT_LOCATION, QString());
	contact_data_.m_informations.addValue(QETInformation::ELMT_LABEL, QString());
	contact_->setElementData(contact_data_);

	const QImage without_box{render()};
	QVERIFY(with_box != without_box);
}

/**
	The number of a wire is written next to its mark, in the room left
	between the mark and the terminal: it shrinks to fit there, and what
	still does not fit is clipped away, so a long number never runs into
	the drawing of the terminal. The two pictures -- one with a number
	long enough to reach into the terminal, one without any -- must be
	the same above the bottom of the terminal, and differ below it (the
	number is drawn somewhere).
*/
void tst_cabletarget::wireLabelStaysOutOfTheTerminal()
{
	QVERIFY(wireCableToTheContact());

	auto pattern_ = QSharedPointer<TerminalStripLayoutPattern>{new TerminalStripLayoutPattern};
	const auto strips_ = m_project->terminalStrip();
	QVERIFY(!strips_.isEmpty());
	TerminalStripDrawer::TerminalStripDrawer drawer{
		QSharedPointer<TerminalStripDrawer::TrueTerminalStrip>{
			new TerminalStripDrawer::TrueTerminalStrip{strips_.first()}},
		pattern_};

	const QRectF bounds{drawer.boundingRect()};
	QVERIFY(!bounds.isEmpty());

	const auto render = [&drawer, &bounds]() {
		QImage image{bounds.size().toSize() + QSize{4, 4},
					 QImage::Format_ARGB32_Premultiplied};
		image.fill(Qt::transparent);
		QPainter painter{&image};
		painter.translate(-bounds.topLeft() + QPointF{2, 2});
		drawer.paint(&painter);
		return image;
	};

		//A number much longer than the room left over the mark
	Element *terminal_ = element(
		QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
	QVERIFY(terminal_);
	ElementData terminal_data_{terminal_->elementData()};
	terminal_data_.m_informations.addValue(
		RealTerminal::cableWireInfoKey(),
		QStringLiteral("gnge-gnge-gnge-gnge-gnge"));
	terminal_->setElementData(terminal_data_);
	const QImage with_label{render()};

		//The same strip, the wire carries no number at all
	terminal_data_.m_informations.addValue(RealTerminal::cableWireInfoKey(),
										   QString());
	terminal_->setElementData(terminal_data_);
	const QImage without_label{render()};

		//The number is drawn: the two pictures differ
	QVERIFY(with_label != without_label);

		//The terminals of this strip all stand on the last level of the
		//pattern, so their bottom is the same rectangle
	const qreal terminal_bottom{pattern_->m_terminal_rect.at(3).bottom()};
	const auto rows_to_check{qMin(qRound(terminal_bottom - bounds.topLeft().y() + 2),
								  with_label.height())};
	for (auto y = 0 ; y < rows_to_check ; ++y)
	{
		for (auto x = 0 ; x < with_label.width() ; ++x)
		{
			QCOMPARE(with_label.pixel(x, y), without_label.pixel(x, y));
		}
	}
}

/**
	The box at the end of a cable keeps the font of the drawing: with
	its three lines (Installation, Location and Label) and the end zone
	the settings leave, the text is still written at the font of the
	strip instead of being squeezed into a tiny one. The box is the
	lowest thing the strip draws, so its bottom line is the lowest
	painted row, and it must land where three lines of that font need
	it to.
*/
void tst_cabletarget::targetBoxKeepsTheDrawingFont()
{
	QVERIFY(wireCableToTheContact());

	auto pattern_ = QSharedPointer<TerminalStripLayoutPattern>{new TerminalStripLayoutPattern};
	const auto strips_ = m_project->terminalStrip();
	QVERIFY(!strips_.isEmpty());
	TerminalStripDrawer::TerminalStripDrawer drawer{
		QSharedPointer<TerminalStripDrawer::TrueTerminalStrip>{
			new TerminalStripDrawer::TrueTerminalStrip{strips_.first()}},
		pattern_};

	const QRectF bounds{drawer.boundingRect()};
	QVERIFY(!bounds.isEmpty());
	QImage image{bounds.size().toSize() + QSize{4, 4},
				 QImage::Format_ARGB32_Premultiplied};
	image.fill(Qt::transparent);
	QPainter painter{&image};
	painter.translate(-bounds.topLeft() + QPointF{2, 2});
	drawer.paint(&painter);
	painter.end();

	qreal lowest{-1};
	for (auto y = 0 ; y < image.height() ; ++y)
	{
		for (auto x = 0 ; x < image.width() ; ++x)
		{
			if (image.pixel(x, y) != qRgba(0, 0, 0, 0)) {
				lowest = y;
				break;
			}
		}
	}
	QVERIFY(lowest >= 0);

		//The end zone the default layout leaves is what gives the box its
		//room: three lines of the default font must fit in it, or the text
		//of a target is squeezed down again by the drawing itself
	const QFontMetricsF metrics{pattern_->font()};
	const qreal room{pattern_->m_cable_end_length
					 - qMin<qreal>(6, pattern_->m_cable_end_length)};
	QVERIFY2(metrics.height() * 3 + 4 <= room,
			 "the default end zone leaves no room for three lines of the "
			 "default font, a target would be squeezed again");

		//Where the bottom line of the box lands when it holds the three
		//lines of its text at the font of the drawing
	const qreal terminals_bottom{pattern_->m_terminal_rect.at(3).bottom()};
	const qreal bar_2{terminals_bottom + pattern_->m_cable_wire_length
					  + pattern_->m_cable_length};
	const qreal box_bottom{bar_2 + qMin<qreal>(6, pattern_->m_cable_end_length)
						   + metrics.height() * 3 + 4};

		//The strip draws nothing under the box, so the lowest painted row
		//is its bottom line -- within a pixel, the outline of a rectangle
		//is painted on its edge
	const qreal row{box_bottom - bounds.top() + 2};
	QVERIFY(lowest + 2 >= row);
}

/**
	The realistic style draws the cable too. The strip seen from above
	reserves the zone under it when one of its terminals carries a
	cable, and the wires, the cable and the box naming where it goes
	are painted in it -- exactly what the schematic style draws, since
	both styles now share the same Helpers::drawCables().
*/
void tst_cabletarget::realisticStyleDrawsTheCable()
{
	QVERIFY(wireCableToTheContact());

	const auto strips_ = m_project->terminalStrip();
	QVERIFY(!strips_.isEmpty());
	const auto strip_ = QSharedPointer<TerminalStripDrawer::TrueTerminalStrip>{
		new TerminalStripDrawer::TrueTerminalStrip{strips_.first()}};
	TerminalStripLayoutPattern pattern_;

	const auto bounds_of = [&strip_, &pattern_]() {
		return TerminalStripDrawer::Realistic::boundingRect(strip_.data(),
															pattern_);
	};
	const auto render = [&strip_, &pattern_](const QRectF &bounds) {
		QImage image{bounds.size().toSize() + QSize{4, 4},
					 QImage::Format_ARGB32_Premultiplied};
		image.fill(Qt::transparent);
		QPainter painter{&image};
		painter.translate(-bounds.topLeft() + QPointF{2, 2});
		TerminalStripDrawer::Realistic::paint(
			&painter, *strip_, pattern_,
			TerminalStripLayoutPattern::DrawStyle::RealisticPushIn);
		return image;
	};

	const QRectF with_bounds{bounds_of()};
	const QImage with_cable{render(with_bounds)};

		//The body of the strip ends there : everything painted under it
		//belongs to the cable
	const qreal body_bottom{
		pattern_.m_realistic_length_mm * pattern_.m_realistic_px_per_mm};
	QVERIFY(with_bounds.height() > body_bottom);

	int painted_{0};
	for (auto y = qRound(body_bottom) + 4 ; y + 4 < with_cable.height() ; ++y)
	{
		for (auto x = 0 ; x + 4 < with_cable.width() ; ++x)
		{
			if (with_cable.pixel(x, y) != qRgba(0, 0, 0, 0)) {
				++painted_;
			}
		}
	}
		//The wires and the cable are drawn in that zone
	QVERIFY(painted_ > 0);

		//The same strip, its terminal carries no cable name any more
	Element *terminal_ = element(
		QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
	QVERIFY(terminal_);
	ElementData terminal_data_{terminal_->elementData()};
	terminal_data_.m_informations.addValue(RealTerminal::cableInfoKey(),
										   QString());
	terminal_->setElementData(terminal_data_);

		//No cable to let out : the strip does not reserve the zone
	const QRectF without_bounds{bounds_of()};
	QVERIFY(without_bounds.height() < with_bounds.height());
}

/**
	A target typed in by hand is written one line per field, and only a
	line break breaks it into fields: the box under the cable draws one
	line per field, with no separator in it. The picture of a strip
	whose cable ends on three fields is therefore that much deeper than
	the picture of the same strip with one single field -- two lines of
	the font of the drawing. A "/" written inside a field, an
	installation named "Betrieb 1/2", keeps its line: the box is no
	deeper than with one plain field.
*/
void tst_cabletarget::typedTargetIsDrawnInItsOwnLines()
{
		//The cable lands on a piece which names nothing, so there is no
		//target of the drawing and the box takes what is typed in
	QVERIFY(wireCableToTheContact());
	Element *contact_ = element(
		QStringLiteral("{fc9cc920-5822-421a-916e-a17100acdcf6}"));
	QVERIFY(contact_);
	ElementData contact_data_{contact_->elementData()};
	contact_data_.m_informations.addValue(QETInformation::ELMT_PLANT, QString());
	contact_data_.m_informations.addValue(QETInformation::ELMT_LOCATION, QString());
	contact_data_.m_informations.addValue(QETInformation::ELMT_LABEL, QString());
	contact_->setElementData(contact_data_);

	const auto a_ = terminalInStrip(
		QStringLiteral("{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
	QVERIFY(a_);
	QVERIFY(a_->cableTarget().isEmpty());
	Element *terminal_ = a_->element();
	QVERIFY(terminal_);

	auto pattern_ = QSharedPointer<TerminalStripLayoutPattern>{new TerminalStripLayoutPattern};
	const auto strips_ = m_project->terminalStrip();
	QVERIFY(!strips_.isEmpty());
	TerminalStripDrawer::TerminalStripDrawer drawer{
		QSharedPointer<TerminalStripDrawer::TrueTerminalStrip>{
			new TerminalStripDrawer::TrueTerminalStrip{strips_.first()}},
		pattern_};

	const QRectF bounds{drawer.boundingRect()};
	QVERIFY(!bounds.isEmpty());

	const auto render = [&drawer, &bounds]() {
		QImage image{bounds.size().toSize() + QSize{4, 4},
					 QImage::Format_ARGB32_Premultiplied};
		image.fill(Qt::transparent);
		QPainter painter{&image};
		painter.translate(-bounds.topLeft() + QPointF{2, 2});
		drawer.paint(&painter);
		painter.end();
		return image;
	};
	const auto lowest_row = [](const QImage &image_) {
		qreal lowest{-1};
		for (auto y = 0 ; y < image_.height() ; ++y)
		{
			for (auto x = 0 ; x < image_.width() ; ++x)
			{
				if (image_.pixel(x, y) != qRgba(0, 0, 0, 0)) {
					lowest = y;
					break;
				}
			}
		}
		return lowest;
	};
	const auto type_in = [terminal_](const QString &text_) {
		ElementData data_{terminal_->elementData()};
		data_.m_informations.addValue(RealTerminal::cableTargetInfoKey(), text_);
		terminal_->setElementData(data_);
	};

		//One field: the box holds one single line
	type_in(QStringLiteral("Boiler room"));
	const qreal one_line{lowest_row(render())};
	QVERIFY(one_line >= 0);

		//Three fields: the box is two lines deeper, at the font of the
		//drawing -- the fields are written one under the other, the cell
		//holding them only tells them apart by their line breaks
	type_in(QStringLiteral("Boiler room\nCabinet 3\n-K3"));
	QCOMPARE(a_->cableTarget(), (QVector<QString>{QStringLiteral("Boiler room"),
												  QStringLiteral("Cabinet 3"),
												  QStringLiteral("-K3")}));
	const qreal three_lines{lowest_row(render())};
	const QFontMetricsF metrics{pattern_->font()};
	QVERIFY2(three_lines - one_line > 1.5 * metrics.height(),
			 "three fields must give a box two lines deeper than one field");

		//A slash written inside a field is part of it: the text gives
		//one single line, the box is back to the depth of one field and
		//holds the slash where it was typed
	type_in(QStringLiteral("Betrieb 1/2 / Keller"));
	QCOMPARE(a_->cableTarget(),
			 QVector<QString>{QStringLiteral("Betrieb 1/2 / Keller")});
	const qreal one_slashed_field{lowest_row(render())};
	QVERIFY2(qAbs(one_slashed_field - one_line) < metrics.height() / 2,
			 "a slash inside a field must not split it into lines");
}

/**
	Two cables of one strip end close to each other, and their boxes are
	wider than the room between the two cables: the box standing in the
	way of the other is set lower than it, its stub drawn longer -- the
	two boxes are never written the one over the other, and the text is
	never cut down to fit. The strip reserves the room for the lower box
	too, or the drawing would be cut off there.
*/
void tst_cabletarget::boxesOfTwoCablesAreSetApart()
{
	QVERIFY(wireCableToTheContact());

		//The second cable of the strip : its wires only join the strip's
		//own terminals, so it names nothing and takes what is typed in
	Cable *cable_ = m_project->newCable();
	QVERIFY(cable_);
	Conductor *inside_ = conductor(
		QStringLiteral("{584b452e-a885-491a-98f1-fd348610b5e0}"));
	QVERIFY(inside_);
	claimCable(inside_, cable_->uuid());

	const auto b_ = terminalInStrip(
		QStringLiteral("{4861e268-d344-413e-a86f-034ea5ada16c}"));
	QVERIFY(b_);
	Element *second_ = b_->element();
	QVERIFY(second_);

	auto pattern_ = QSharedPointer<TerminalStripLayoutPattern>{new TerminalStripLayoutPattern};
	const auto strips_ = m_project->terminalStrip();
	QVERIFY(!strips_.isEmpty());
	TerminalStripDrawer::TerminalStripDrawer drawer{
		QSharedPointer<TerminalStripDrawer::TrueTerminalStrip>{
			new TerminalStripDrawer::TrueTerminalStrip{strips_.first()}},
		pattern_};

	const auto render = [&drawer](const QRectF &bounds) {
		QImage image{bounds.size().toSize() + QSize{4, 4},
					 QImage::Format_ARGB32_Premultiplied};
		image.fill(Qt::transparent);
		QPainter painter{&image};
		painter.translate(-bounds.topLeft() + QPointF{2, 2});
		drawer.paint(&painter);
		painter.end();
		return image;
	};
	const auto lowest_row = [](const QImage &image_) {
		qreal lowest{-1};
		for (auto y = 0 ; y < image_.height() ; ++y)
		{
			for (auto x = 0 ; x < image_.width() ; ++x)
			{
				if (image_.pixel(x, y) != qRgba(0, 0, 0, 0)) {
					lowest = y;
					break;
				}
			}
		}
		return lowest;
	};

		//One cable with a target : one box, in the end zone
	const QRectF one_bounds{drawer.boundingRect()};
	const qreal one_row{lowest_row(render(one_bounds))};
	QVERIFY(one_row >= 0);

		//The second cable gets a target of its own : its box is wider
		//than the distance between the two cables and would be written
		//over the first one
	ElementData second_data_{second_->elementData()};
	second_data_.m_informations.addValue(RealTerminal::cableInfoKey(),
										 QStringLiteral("X98"));
	second_data_.m_informations.addValue(RealTerminal::cableTargetInfoKey(),
										 QStringLiteral("Plant 9\nCabinet 9\n-S9"));
	second_->setElementData(second_data_);
	QCOMPARE(b_->cableTarget(),
			 (QVector<QString>{QStringLiteral("Plant 9"),
							   QStringLiteral("Cabinet 9"),
							   QStringLiteral("-S9")}));

	const QRectF two_bounds{drawer.boundingRect()};
	const qreal two_row{lowest_row(render(two_bounds))};

		//The box in the way is set lower : the strip draws a whole box
		//deeper than it did with one single box -- the second box stands
		//under the first one, one box height plus the gap between them
	const QFontMetricsF metrics{pattern_->font()};
	const qreal box_height{metrics.height() * 3 + 4};
	QVERIFY2(two_row - one_row > box_height,
			 "a box reaching into another one must be set lower, not "
			 "written over it");

		//And the strip is deep enough for it, or the lower box would be
		//cut off by the drawing area
	QVERIFY2(two_bounds.height() > one_bounds.height(),
			 "the strip reserves no room for the box it sets lower");
	QVERIFY2(two_bounds.bottom() - one_bounds.bottom() > 0.5 * box_height,
			 "the room reserved for the lower box is too small");
}

int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	return QetAppTest::run<tst_cabletarget>(argc, argv);
}

#include "tst_cabletarget.moc"
