// SPDX-License-Identifier: GPL-2.0-or-later
// The outline and the click area of a text hug the lines written,
// linked against the application's objects.
//
// A symbol's text kept at a fixed width and aligned to the right, "-XS6"
// in a box 120 wide, used to be outlined, hovered and picked over the
// whole box: a double click on the empty left part of it hit this text
// instead of whatever lay beneath. Now the outline and the click area
// are the written line, at the right of the box; while editing, the
// whole box still takes the clicks.
#include <QApplication>
#include <QFontDatabase>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <stdexcept>

#include "../../sources/diagram.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetgraphicsitem/element.h"
#include "../../sources/qetgraphicsitem/dynamicelementtextitem.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
	std::printf("ok: %s\n", message);
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETTextOutlineRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		project.buildFolios(project.diagrams());
		Diagram *scene = project.diagrams().first();

		DynamicElementTextItem *text = nullptr;
		for (Element *e : scene->elements())
			for (DynamicElementTextItem *t : e->dynamicTextItems())
				if (!text) text = t;
		check(text, "the fixture has a symbol with a text");

		text->setTextFrom(DynamicElementTextItem::UserText);
		text->setText(QStringLiteral("-XS6"));
		text->setTextWidth(120);
		text->setAlignment(Qt::AlignRight | Qt::AlignTop);

		const QRectF box = text->boundingRect();
		const QRectF written = text->writtenRect();
		check(box.width() >= 120, "the box keeps its fixed width");
		check(written.width() < 40, "the outline is as wide as the text written");
		check(qAbs(written.right() - box.right()) < 8, "and sits at the right, as the text is aligned");

		const QPointF empty(box.left() + 10, written.center().y());
		check(!text->shape().contains(empty), "the empty part of the box is not this text's to click");
		check(text->shape().contains(written.center()), "the written line is");
		check(text->contains(written.center()) && !text->contains(empty), "hover and click follow the shape");

		text->setText(QStringLiteral("Sockets bathroom first floor, 5"));
		text->setTextWidth(50);
		check(text->writtenRect().height() > written.height() + 5, "a text on several lines is outlined whole");

		text->setText(QString());
		check(text->writtenRect() == text->boundingRect(), "a text with nothing written can still be picked");

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
