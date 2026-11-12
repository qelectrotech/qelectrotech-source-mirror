/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-2.0-or-later
/*
	A junction is a connection point on a wire rather than a piece of it,
	so what a wire at a junction is called, and where a cable lands, are
	worked out from the chain the junction's definition gives: the source,
	then target 1, then target 2, then target 3 (#1500).

	The scene is built here rather than read from a fixture: one junction
	with three connections, a strip of terminals and two devices hung on
	those connections, with the cables W1 and W2 drawn over them.

	Four things this pins down, each of which has been wrong once:
	- the chain is read at the junction end of a wire, never at the end
	  of the terminal the strip draws (a stub said nothing at all then);
	- a cable which starts at a target lands where the chain leads, at the
	  connection before it -- not at every connection the junction has
	  (a cable list then started at the source's terminal);
	- both lines of one wire say what that wire is made of (only the
	  first of them did).
	- the CSV export, the database list and the dialog over it all name
	  the same ends, which is what the code asks of them.
*/
#include "qetapptest.h"

#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "cable/cable.h"
#include "cablelist/cablelistrows.h"
#include "conductorproperties.h"
#include "diagram.h"
#include "diagramcontext.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetgraphicsitem/terminalelement.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "ui/wiringlistdialog.h"
#include "wiringlistdb.h"
#include "wiringlistexport.h"

#include <QTableView>
#include <QtTest>

namespace {

	/// The three connections of the junction. Its definition says the
	/// wire comes in at the source and goes on to the targets in their
	/// order, and every copy of that definition answers alike, because
	/// the roles belong to the symbol rather than to one copy of it
	/// (#1500).
const QUuid &sourceUuid()
{
	static const QUuid id = QUuid::createUuid();
	return id;
}

const QUuid &firstTargetUuid()
{
	static const QUuid id = QUuid::createUuid();
	return id;
}

const QUuid &secondTargetUuid()
{
	static const QUuid id = QUuid::createUuid();
	return id;
}

	///The connection of the one-connection symbols this test draws.
	///A terminal without one of its own is written the older way, with
	///no element uuid at all, which is not what a symbol in the cabinet
	///looks like (#1500)
const QUuid &clampConnectionUuid()
{
	static const QUuid id = QUuid::createUuid();
	return id;
}

const QUuid &deviceConnectionUuid()
{
	static const QUuid id = QUuid::createUuid();
	return id;
}

	/// The lines of @a wires, one name each: what the strip writes beside
	/// the stubs it draws
QStringList labelsOf(const QVector<QPair<QString, QStringList>> &wires)
{
	QStringList labels;
	for (const auto &wire : wires) {
		labels << wire.first;
	}
	return labels;
}

} // namespace

class tst_junctionchain : public QObject
{
	Q_OBJECT

	QETProject *m_project = nullptr;
	Diagram *m_diagram = nullptr;
	Element *m_junction = nullptr;

		/**
			@brief junctionDefinition
			@param with_roles whether the definition says which of its
			connections leads where; without it nothing is known about
			the junction, the way a symbol written before the roles
			were ever set opens
			@return the definition of the junction this test draws
		*/
	static QString junctionDefinition(const bool with_roles)
	{
		const QString roles = with_roles
			? QStringLiteral(
				  "<kindInformations>"
				  "<kindInformation name=\"junction_source\">%1</kindInformation>"
				  "<kindInformation name=\"junction_target_1\">%2</kindInformation>"
				  "<kindInformation name=\"junction_target_2\">%3</kindInformation>"
				  "</kindInformations>")
				  .arg(sourceUuid().toString(), firstTargetUuid().toString(),
					   secondTargetUuid().toString())
			: QString();
		return QStringLiteral(
				   "<definition type=\"element\" link_type=\"junction\" "
				   "width=\"60\" height=\"40\" hotspot_x=\"30\" hotspot_y=\"20\">"
				   "<names><name lang=\"en\">junction</name></names>%1"
				   "<description>"
				   "<line x1=\"0\" y1=\"20\" x2=\"60\" y2=\"20\" "
				   "style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/>"
				   "<terminal name=\"1\" orientation=\"w\" uuid=\"%2\" x=\"0\" y=\"20\"/>"
				   "<terminal name=\"2\" orientation=\"n\" uuid=\"%3\" x=\"30\" y=\"0\"/>"
				   "<terminal name=\"3\" orientation=\"e\" uuid=\"%4\" x=\"60\" y=\"20\"/>"
				   "</description></definition>")
			.arg(roles, sourceUuid().toString(), firstTargetUuid().toString(),
				 secondTargetUuid().toString());
	}

		/// The definition of a one-connection terminal, the kind of
		/// symbol a terminal strip holds
	static QString terminalDefinition()
	{
		return QStringLiteral(
			"<definition type=\"element\" link_type=\"terminal\" "
			"width=\"20\" height=\"20\" hotspot_x=\"0\" hotspot_y=\"10\">"
			"<names><name lang=\"en\">clamp</name></names>"
			"<kindInformations><kindInformation name=\"type\">terminal</kindInformation></kindInformations>"
			"<description>"
			"<rect x=\"0\" y=\"0\" width=\"20\" height=\"20\" "
			"style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/>"
			"<terminal uuid=\"%1\" x=\"0\" y=\"10\" orientation=\"w\"/>"
			"</description></definition>")
			.arg(clampConnectionUuid().toString());
	}

		/// The definition of a device with one connection, drawn in the
		/// cabinet and numbered like any other piece
	static QString deviceDefinition()
	{
		return QStringLiteral(
			"<definition type=\"element\" link_type=\"simple\" "
			"width=\"20\" height=\"20\" hotspot_x=\"0\" hotspot_y=\"10\">"
			"<names><name lang=\"en\">device</name></names>"
			"<description>"
			"<rect x=\"0\" y=\"0\" width=\"20\" height=\"20\" "
			"style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/>"
			"<terminal uuid=\"%1\" x=\"0\" y=\"10\" orientation=\"w\"/>"
			"</description></definition>")
			.arg(deviceConnectionUuid().toString());
	}

		/**
			@brief define
			@param name the name of the symbol in the collection
			@param xml its definition
			@return true when the project's own collection holds that
			symbol, either already or now
		*/
	bool define(const QString &name, const QString &xml)
	{
		auto *collection = m_project->embeddedElementCollection();
		if (!collection) {
			return false;
		}
		const QString path = QStringLiteral("import/%1.elmt").arg(name);
		if (collection->exist(path)) {
			return true;
		}
		QDomDocument document;
		document.setContent(xml);
		if (document.documentElement().isNull()) {
			return false;
		}
		return collection->addElementDefinition(QStringLiteral("import"), name,
						      document.documentElement());
	}

		/**
			@brief place
			@param name the symbol to take from the collection
			@param label its BMK, empty for a symbol without one
			@param pos where to draw it
			@return the symbol on the folio, or null when there is
			none by that name
		*/
	Element *place(const QString &name, const QString &label, const QPointF &pos)
	{
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name),
					 m_project),
			nullptr, &state);
		if (state || !element) {
			return nullptr;
		}
		m_diagram->addItem(element);
		element->setPos(pos);
		if (!label.isEmpty()) {
			DiagramContext info = element->elementInformations();
			info.addValue(QETInformation::ELMT_LABEL, label);
			info.remove(QStringLiteral("formula"));
			element->setElementInformations(info);
		}
		return element;
	}

		/// The connection of @a element which carries @a uuid
	static Terminal *connection(Element *element, const QUuid &uuid)
	{
		if (!element) {
			return nullptr;
		}
		for (Terminal *terminal : element->terminals())
		{
			if (terminal->uuid() == uuid) {
				return terminal;
			}
		}
		return nullptr;
	}

		/// The one connection of a one-connection symbol
	static Terminal *connection(Element *element)
	{
		if (!element || element->terminals().isEmpty()) {
			return nullptr;
		}
		return element->terminals().first();
	}

		/**
			@brief wire
			Draw a wire between two connections, hung on @a cable
			when there is one.
			@return the wire, or null when an end was missing
		*/
	Conductor *wire(Terminal *first, Terminal *second, Cable *cable = nullptr)
	{
		if (!first || !second) {
			return nullptr;
		}
		auto *conductor = new Conductor(first, second);
		m_diagram->addItem(conductor);
		if (cable) {
			ConductorProperties properties = conductor->properties();
			properties.m_cable_uuid = cable->uuid();
			conductor->setProperties(properties);
		}
		return conductor;
	}

		/// A cable of this project, by the designation the lists write
	Cable *cable(const QString &designation)
	{
		auto *result = new Cable(m_project);
		result->setDesignation(designation);
		m_project->addCable(result);
		return result;
	}

		/// The strip terminal the folio holds the element @a to
	QSharedPointer<RealTerminal> realOf(Element *element) const
	{
		for (TerminalStrip *strip : m_project->terminalStrip())
		{
			for (const auto &real : strip->realTerminals())
			{
				if (real && real->element() == element) {
					return real;
				}
			}
		}
		return {};
	}

		/// A strip holding the terminals this test wires up
	TerminalStrip *newStrip()
	{
		auto *strip = new TerminalStrip(QStringLiteral("="), QStringLiteral("UV1"),
						QStringLiteral("X1"), m_project);
		if (!m_project->addTerminalStrip(strip)) {
			delete strip;
			return nullptr;
		}
		return strip;
	}

		/**
			@brief drawThroughAJunction
			The scene both wiring lists are read from: the cable W1 comes
			in at the source of the junction, the device 7 hangs on target
			1 on wires of its own which no cable claims, and the cable W2
			starts at target 2 and is drawn to the device 8.
			@return true when everything was drawn
		*/
	bool drawThroughAJunction()
	{
		Element *terminal = place(QStringLiteral("clamp"), QStringLiteral("-X1:1"),
					   QPointF(100, 100));
		Element *before = place(QStringLiteral("device"), QStringLiteral("7"),
					QPointF(200, 300));
		Element *after = place(QStringLiteral("device"), QStringLiteral("8"),
				       QPointF(300, 500));
		if (!terminal || !before || !after) {
			return false;
		}
		Cable *w1 = cable(QStringLiteral("W1"));
		Cable *w2 = cable(QStringLiteral("W2"));
		return w1 && w2
			&& wire(connection(terminal), connection(m_junction, sourceUuid()), w1)
			&& wire(connection(before), connection(m_junction, firstTargetUuid()))
			&& wire(connection(m_junction, secondTargetUuid()), connection(after), w2);
	}

private slots:
	void initTestCase();
	void init();
	void cleanup();

	void aCableStartsWhereTheChainLeads();
	void aTerminalAtAJunctionIsNamedByTheChain();
	void aJunctionWithoutASourceIsLeftUnsaid();
	void bothListsAgree();
	void aJunctionWithNoSourceIsCountedInBothLists();
	void theDialogShowsTheResolvedRowsToo();
};

void tst_junctionchain::initTestCase()
{
	QETProject::setBackupEnabled(false);
}

void tst_junctionchain::init()
{
	m_project = new QETProject;
	QVERIFY(m_project);
	m_diagram = m_project->addNewDiagram();
	QVERIFY(m_diagram);
	QVERIFY(define(QStringLiteral("chain_junction"), junctionDefinition(true)));
	QVERIFY(define(QStringLiteral("clamp"), terminalDefinition()));
	QVERIFY(define(QStringLiteral("device"), deviceDefinition()));
	m_junction = place(QStringLiteral("chain_junction"), QString(),
			   QPointF(400, 200));
	QVERIFY(m_junction);
	QVERIFY(connection(m_junction, sourceUuid()));
	QVERIFY(connection(m_junction, firstTargetUuid()));
	QVERIFY(connection(m_junction, secondTargetUuid()));
}

void tst_junctionchain::cleanup()
{
	delete m_project;
	m_project = nullptr;
	m_diagram = nullptr;
	m_junction = nullptr;
}

/**
 * The cable W1 hangs on the source of the junction and the cable W2 on its
 * second target, while a device of its own hangs on the target between
 * them. Neither cable lands on the junction: the chain says where each of
 * them goes on to, and W2 -- which starts at a target -- lands where the
 * chain leads, at the connection before it rather than at every
 * connection the junction has (#1500).
 */
void tst_junctionchain::aCableStartsWhereTheChainLeads()
{
	QVERIFY(drawThroughAJunction());

	const auto rows = CableList::rows(m_project);
	QCOMPARE(rows.size(), 2);

		//W1 starts at the terminal and goes on where the chain leads
	QCOMPARE(rows.at(0).value(QStringLiteral("bmk_cable")), QStringLiteral("W1"));
	QCOMPARE(rows.at(0).value(QStringLiteral("bmk_start")), QStringLiteral("-X1"));
	QCOMPARE(rows.at(0).value(QStringLiteral("bmk_end")), QStringLiteral("7"));

		//W2 starts where the chain leads -- at target 1, which is
		//the device 7 -- and ends at the device 8 it is drawn to.
		//A junction which simply offered every connection it has
		//would put the terminal -X1 here instead
	QCOMPARE(rows.at(1).value(QStringLiteral("bmk_cable")), QStringLiteral("W2"));
	QCOMPARE(rows.at(1).value(QStringLiteral("bmk_start")), QStringLiteral("7"));
	QCOMPARE(rows.at(1).value(QStringLiteral("bmk_end")), QStringLiteral("8"));
}

/**
 * A terminal whose wire ends on a junction is named by the chain the
 * junction's definition gives: what the wire comes from first, then where
 * it goes on to. A terminal in the middle of the chain has a wire on both
 * sides of it and gets one line each, the way a terminal with two wires of
 * its own already has.
 */
void tst_junctionchain::aTerminalAtAJunctionIsNamedByTheChain()
{
	Element *at_source = place(QStringLiteral("clamp"), QStringLiteral("-X1:3"),
				    QPointF(100, 100));
	Element *at_first = place(QStringLiteral("clamp"), QStringLiteral("-X1:1"),
				  QPointF(200, 100));
	Element *at_second = place(QStringLiteral("clamp"), QStringLiteral("-X1:2"),
				   QPointF(300, 100));
	QVERIFY(at_source && at_first && at_second);

	TerminalStrip *strip = newStrip();
	QVERIFY(strip);
	QVERIFY(strip->addTerminal(at_source));
	QVERIFY(strip->addTerminal(at_first));
	QVERIFY(strip->addTerminal(at_second));

	Conductor *from_source = wire(connection(at_source),
				      connection(m_junction, sourceUuid()));
	QVERIFY(from_source);
	Conductor *from_first = wire(connection(at_first),
				     connection(m_junction, firstTargetUuid()));
	QVERIFY(from_first);
	QVERIFY(wire(connection(at_second),
		     connection(m_junction, secondTargetUuid())));

		//The wire at target 1 says what it is made of...
	ConductorProperties properties = from_first->properties();
	properties.m_conductor_type = QStringLiteral("H05V-K");
	properties.m_wire_color = QStringLiteral("BN");
	properties.m_wire_section = QStringLiteral("1.5");
	from_first->setProperties(properties);

	const auto from_source_stub = realOf(at_source);
	const auto from_first_stub = realOf(at_first);
	const auto from_second_stub = realOf(at_second);
	QVERIFY(from_source_stub && from_first_stub && from_second_stub);

		//The source leads on to target 1, so its stub says where
		//the wire goes on to...
	QCOMPARE(labelsOf(from_source_stub->wires()),
		 QStringList({QStringLiteral("-X1:1")}));

		//...target 1 has a wire on both sides of the chain and
		//gets one line for each, what it comes from first...
	QCOMPARE(labelsOf(from_first_stub->wires()),
		 QStringList({QStringLiteral("-X1:3"),
				     QStringLiteral("-X1:2")}));

		//...and target 2 only has what its wire comes from
	QCOMPARE(labelsOf(from_second_stub->wires()),
		 QStringList({QStringLiteral("-X1:1")}));

		//Both lines of the wire at target 1 say what that wire is
		//made of: they stand at different places, the way the two
		//wires of a twin sleeve each say what they are made of
	const auto both_lines = from_first_stub->wires();
	QCOMPARE(both_lines.size(), 2);
	QCOMPARE(both_lines.at(0).second,
		 QStringList({QStringLiteral("H05V-K"),
				     QStringLiteral("BN"),
				     QStringLiteral("1.5")}));
	QCOMPARE(both_lines.at(1).second, both_lines.at(0).second);
}

/**
 * A junction nothing was ever set on leads nowhere, so there is no order
 * to follow and no name to guess: the stub is still drawn, and left empty
 * (#1500).
 */
void tst_junctionchain::aJunctionWithoutASourceIsLeftUnsaid()
{
	QVERIFY(define(QStringLiteral("open_junction"), junctionDefinition(false)));
	Element *open = place(QStringLiteral("open_junction"), QString(),
			     QPointF(400, 300));
	Element *terminal = place(QStringLiteral("clamp"), QStringLiteral("-X1:1"),
				   QPointF(100, 300));
	QVERIFY(open && terminal);
	QVERIFY(!connection(open, QUuid()));

	TerminalStrip *strip = newStrip();
	QVERIFY(strip);
	QVERIFY(strip->addTerminal(terminal));
	QVERIFY(wire(connection(terminal), connection(open, sourceUuid())));

		//Nothing leads anywhere...
	const auto neighbours =
		junctionChainNeighbours(connection(open, sourceUuid()));
	QVERIFY(!neighbours.first);
	QVERIFY(!neighbours.second);

		//...so the stub is drawn but says nothing, rather than
		//being named by a guess
	const auto stub = realOf(terminal);
	QVERIFY(stub);
	QCOMPARE(stub->wires().size(), 1);
	QVERIFY(stub->wires().first().first.isEmpty());
}

/**
 * The wiring list has two readers: the CSV export, which walks the project
 * document, and the database list behind the dialog and --export-wiring.
 * The code says the two are meant to agree, so both are run over the same
 * scene here and the ends they name compared (#1500).
 */
void tst_junctionchain::bothListsAgree()
{
	QVERIFY(drawThroughAJunction());

	int database_unassigned = 0;
	QString error;
	const QVector<WiringListRow> database =
		wiringListRows(m_project, &database_unassigned, &error);
	QVERIFY2(error.isEmpty(), qPrintable(error));

	int csv_unassigned = 0;
	const WiringListExport exporter(m_project);
	const QString csv = exporter.toCsvString(&csv_unassigned);
	QVERIFY(!csv.isEmpty());

		//Nothing here was left without a source, so neither list has
		//an unresolved junction to report
	QCOMPARE(database_unassigned, 0);
	QCOMPARE(csv_unassigned, 0);

		//The ends each list names, as unordered pairs: the CSV writes
		//its two components in label order and the database list in the
		//order the chain gives, so only which pair it is can be
		//compared. The terminals are part of it, so a row which names
		//the wrong connection of a piece still fails here
	const auto pairs = [](const QStringList &rows) {
		QStringList found;
		for (const QString &row : rows)
		{
			const QStringList cells = row.split(QLatin1Char(';'));
			const QString first = cells.value(1) + QLatin1Char(':') + cells.value(2);
			const QString second = cells.value(3) + QLatin1Char(':') + cells.value(4);
			found << (first < second ? first + QLatin1Char('|') + second
						 : second + QLatin1Char('|') + first);
		}
		found.sort();
		return found;
	};

	QStringList from_csv;
	const QStringList lines = csv.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
	for (int i = 1 ; i < lines.size() ; ++i) {
		from_csv << lines.at(i);
	}
	QStringList from_database;
	for (const WiringListRow &row : database) {
		from_database << row.values().join(QLatin1Char(';'));
	}

	QCOMPARE(pairs(from_database), pairs(from_csv));

		//Two rows: the run from the terminal through the junction to the
		//device 7, and on from there to the device 8. The junction itself
		//is named by neither of them, and neither list has an empty end
		//to show for it
	QCOMPARE(database.size(), 2);
	QCOMPARE(from_csv.size(), 2);
	for (const WiringListRow &row : database)
	{
		QVERIFY2(!row.from_element_label.isEmpty(),
			 qPrintable(row.values().join(QLatin1Char('|'))));
		QVERIFY2(!row.to_element_label.isEmpty(),
			 qPrintable(row.values().join(QLatin1Char('|'))));
	}
}

/**
 * A junction nothing was ever set on leads nowhere, so its rows stay as
 * they are -- there is no chain to follow -- and both lists count it
 * rather than listing it as if the way through were known (#1500).
 */
void tst_junctionchain::aJunctionWithNoSourceIsCountedInBothLists()
{
	QVERIFY(define(QStringLiteral("open_junction"), junctionDefinition(false)));
	Element *open = place(QStringLiteral("open_junction"), QString(),
			     QPointF(400, 300));
	Element *terminal = place(QStringLiteral("clamp"), QStringLiteral("-X1:1"),
				   QPointF(100, 300));
	QVERIFY(open && terminal);
	QVERIFY(wire(connection(terminal), connection(open, sourceUuid())));

		//A junction nothing was ever wired to has no row to get wrong,
		//and is not counted among the ones which do
	QVERIFY(place(QStringLiteral("open_junction"), QString(), QPointF(400, 400)));

	int database_unassigned = 0;
	QString error;
	const QVector<WiringListRow> database =
		wiringListRows(m_project, &database_unassigned, &error);
	QVERIFY2(error.isEmpty(), qPrintable(error));

		//The row is still there, still ending at the junction, and the
		//list says so
	QCOMPARE(database.size(), 1);
	QCOMPARE(database_unassigned, 1);
	QCOMPARE(database.first().to_element_uuid, open->uuid().toString());
	QVERIFY(database.first().to_element_label.isEmpty());

	int csv_unassigned = 0;
	const WiringListExport exporter(m_project);
	const QString csv = exporter.toCsvString(&csv_unassigned);
	QCOMPARE(csv_unassigned, 1);
	QCOMPARE(csv.split(QLatin1Char('\n'), Qt::SkipEmptyParts).size(), 2);
}


/**
 * The dialog is the third reader of the same list: it reads the database,
 * so it has to show the resolved rows too rather than the junctions the
 * wires end on (#1500).
 */
void tst_junctionchain::theDialogShowsTheResolvedRowsToo()
{
	QVERIFY(drawThroughAJunction());

	WiringListDialog dialog(m_project);
	auto *view = dialog.findChild<QTableView *>();
	QVERIFY(view);
	QVERIFY(view->model());
	QCOMPARE(view->model()->rowCount(), 2);
	QCOMPARE(view->model()->headerData(1, Qt::Horizontal).toString(),
		 QStringLiteral("Component 1"));

		//Which pair each row names, in whichever order the list gives
		//them: the run from the terminal through the junction to the
		//device 7, and on from there to the device 8
	QStringList found;
	for (int r = 0 ; r < view->model()->rowCount() ; ++r)
	{
		const QModelIndex at = view->model()->index(r, 1);
		const QModelIndex to = view->model()->index(r, 3);
		found << at.data().toString() + QLatin1Char('|') + to.data().toString();
	}
	found.sort();
	QCOMPARE(found, QStringList({QStringLiteral("-X1:1|7"),
				     QStringLiteral("7|8")}));
}

QET_APP_TEST_MAIN(tst_junctionchain)

#include "tst_junctionchain.moc"
