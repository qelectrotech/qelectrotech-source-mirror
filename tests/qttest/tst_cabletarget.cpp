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

int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	return QetAppTest::run<tst_cabletarget>(argc, argv);
}

#include "tst_cabletarget.moc"
