// SPDX-License-Identifier: GPL-2.0-or-later
// The label of a terminal strip at its first terminal, linked against the
// application's objects.
//
// Three terminals A, B, C in a row form strip XN1, two more, D and E,
// strip -X2. With the project option on:
// - the label "-XN1" stands at A only, the first in reading order, 3
//   before its frame and with its middle on the row; "-X2" at D;
// - it moves with A; C moved before A takes it, and gives it back when it
//   leaves the strip;
// - renaming the strip renames the label;
// - on a terminal turned upright the label stands above it, upright;
// - View > Show hides it with the symbol texts;
// - it is not saved: only the option is.
#include <QApplication>
#include <QFontDatabase>
#include <QGraphicsSimpleTextItem>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <stdexcept>

#include "../../sources/diagram.h"
#include "../../sources/elementsmover.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/shownkinds.h"
#include "../../sources/TerminalStrip/terminalstrip.h"
#include "../../sources/qetgraphicsitem/terminalelement.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
	std::printf("ok: %s\n", message);
}

static bool near(qreal a, qreal b) { return qAbs(a - b) < 0.6; }

static void settle() { QApplication::processEvents(); QApplication::processEvents(); }

static void move(Diagram *diagram, Element *e, const QPointF &by)
{
	diagram->clearSelection();
	e->setSelected(true);
	ElementsMover &mover = diagram->elementsMover();
	mover.beginMovement(diagram);
	mover.continueMovement(by);
	mover.endMovement();
	diagram->clearSelection();
	settle();
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETStripLabelRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		project.buildFolios(project.diagrams());
		Diagram *folio = project.diagrams().first();

		QList<TerminalElement *> row, pair;
		for (Element *e : folio->elements())
			if (auto *te = qobject_cast<TerminalElement *>(e))
				(te->pos().y() < 300 ? row : pair) << te;
		auto byX = [](TerminalElement *a, TerminalElement *b) { return a->pos().x() < b->pos().x(); };
		std::sort(row.begin(), row.end(), byX);
		std::sort(pair.begin(), pair.end(), byX);
		check(row.size() == 3 && pair.size() == 2, "fixture: a row of three terminals and a pair");
		TerminalElement *A = row[0], *B = row[1], *C = row[2], *D = pair[0], *E = pair[1];

		auto *xn1 = new TerminalStrip(QStringLiteral("="), QStringLiteral("UV1"), QStringLiteral("XN1"), &project);
		auto *x2 = new TerminalStrip(QStringLiteral("="), QStringLiteral("UV1"), QStringLiteral("-X2"), &project);
		project.addTerminalStrip(xn1);
		project.addTerminalStrip(x2);
		for (Element *e : {static_cast<Element *>(C), static_cast<Element *>(A), static_cast<Element *>(B)})
			xn1->addTerminal(e);
		x2->addTerminal(E);
		x2->addTerminal(D);
		settle();
		check(A->stripLabel().isEmpty() && D->stripLabel().isEmpty(), "off by default: no label");

		project.setTerminalStripLabelsShown(true);
		settle();
		check(A->stripLabel() == "-XN1", "on: the label at the first terminal, with the - of a designation");
		check(B->stripLabel().isEmpty() && C->stripLabel().isEmpty(), "and at none of the others");
		check(D->stripLabel() == "-X2", "a name that has its - keeps it");

		QRectF label = A->stripLabelItem()->sceneBoundingRect();
		check(near(label.right(), A->sceneBoundingRect().left() - 3), "3 before the terminal's frame");
		check(near(label.center().y(), A->scenePos().y()), "its middle on the row");

		move(folio, A, QPointF(0, 40));
		label = A->stripLabelItem()->sceneBoundingRect();
		check(A->stripLabel() == "-XN1" && near(label.center().y(), A->scenePos().y()), "it moves with the terminal");
		move(folio, A, QPointF(0, -40));

		move(folio, C, QPointF(-100, 0));
		check(C->stripLabel() == "-XN1" && A->stripLabel().isEmpty(), "a terminal moved before the first takes the label");
		xn1->removeTerminal(C);
		settle();
		check(A->stripLabel() == "-XN1" && C->stripLabel().isEmpty(), "and gives it back when it leaves the strip");

		xn1->setName(QStringLiteral("XN9"));
		settle();
		check(A->stripLabel() == "-XN9", "renaming the strip renames the label");

		A->setRotation(90);
		settle();
		label = A->stripLabelItem()->sceneBoundingRect();
		check(label.width() > label.height(), "on a terminal turned upright the label stays upright");
		check(near(label.bottom(), A->sceneBoundingRect().top() - 3) && near(label.center().x(), A->scenePos().x()),
			  "and stands above it, centred");
		A->setRotation(0);
		settle();

		ShownKinds::setShown(ShownKinds::SymbolTexts, false);
		ShownKinds::apply(folio, ShownKinds::SymbolTexts);
		check(!A->stripLabelItem()->isVisible(), "View > Show hides it with the symbol texts");
		ShownKinds::setShown(ShownKinds::SymbolTexts, true);
		ShownKinds::apply(folio, ShownKinds::SymbolTexts);
		check(A->stripLabelItem()->isVisible(), "and shows it again");

		const QString xml = project.toXml().toString();
		check(!xml.contains(QStringLiteral("-XN9")), "the label is not saved");
		check(xml.contains(QStringLiteral("at_first_terminal=\"true\"")), "the option is");

		project.setTerminalStripLabelsShown(false);
		check(A->stripLabel().isEmpty() && D->stripLabel().isEmpty(), "off again: the labels go");
		std::puts("PASS");
	}
	catch (const std::exception &error)
	{
		std::printf("FAIL: %s\n", error.what());
		std::fflush(stdout);
		return 1;
	}
	std::fflush(stdout);
	return 0;
}
