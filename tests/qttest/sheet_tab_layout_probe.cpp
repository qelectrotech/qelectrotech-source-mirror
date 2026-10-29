// SPDX-License-Identifier: GPL-2.0-or-later
/*
	The Sheet tab gives its free height to the title block widget and
	the pages of its tab take it -- the Template row keeps its own
	height however tall the tab is. Measured as a delta between two tab
	heights, the way "Settings > New project > Sheet" maximised on a
	large screen shows it: who grows says the stretch factor of the
	title block widget, not whatever size policies the .ui files happen
	to hold -- and a widget added after it later, the way the terminal
	labelling group was added at the end of the tab, cannot take the
	height away again and leave an empty band behind.

	Both pages are measured: the application's defaults (labelling
	group at the end) and a project's own (nothing after the title
	block widget).
*/
#include <QApplication>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QSettings>
#include <cstdio>
#include <stdexcept>

#include "../../sources/qetapp.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetresult.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/ui/configpage/configpages.h"
#include "../../sources/ui/titleblockpropertieswidget.h"
#include "../../sources/ui/borderpropertieswidget.h"

static void check(bool value, const char *why)
{
	if (!value) throw std::runtime_error(why);
}

	///What one page shows at one height
struct Heights
{
	int templ;
	int display_at;
	int edit;
	int tabs;
	int title_block;
	int border;
	int labelling;
};

/**
	Measure one Sheet page at two tall sizes and check who grows.
	@param page
	@param which a name for the report
	@param has_labelling whether the page ends with the labelling group
*/
static void measure(NewDiagramPage *page, const char *which, bool has_labelling)
{
	QComboBox *templ = page->findChild<QComboBox *>(QStringLiteral("m_tbt_cb"));
	QComboBox *display_at = page->findChild<QComboBox *>(QStringLiteral("m_display_at_cb"));
	QPushButton *edit = page->findChild<QPushButton *>(QStringLiteral("m_tbt_pb"));
	TitleBlockPropertiesWidget *title_block = page->findChild<TitleBlockPropertiesWidget *>();
	BorderPropertiesWidget *border = page->findChild<BorderPropertiesWidget *>();
	check(templ && display_at && edit && title_block && border,
		  "the Sheet page holds its widgets");
	QTabWidget *tabs = title_block->findChild<QTabWidget *>(QStringLiteral("tabWidget"));
	check(tabs, "the title block's own tab found");

		//The labelling group of the application's defaults: the group
		//box standing directly on the Sheet page, beside the two named
		//widgets -- found by where it sits, not by a name it has not
	QGroupBox *labelling = nullptr;
	for (QTabWidget *t : page->findChildren<QTabWidget *>()) {
		if (t->parentWidget() != page) {
			continue;
		}
		QWidget *sheet = t->widget(0);
		for (QGroupBox *gb : sheet->findChildren<QGroupBox *>()) {
			if (gb->parentWidget() == sheet) {
				labelling = gb;
				break;
			}
		}
		break;
	}
	check(bool(labelling) == has_labelling,
		  "the labelling group is on the application's defaults only");

	const auto snap = [&](int height) -> Heights {
		page->show();
		page->resize(1000, height);
		for (int pass = 0; pass < 3; ++pass) {
			QApplication::processEvents();
			page->layout()->activate();
		}
		QApplication::processEvents();
		const Heights at{templ->height(), display_at->height(), edit->height(),
						 tabs->height(), title_block->height(), border->height(),
						 labelling ? labelling->height() : 0};
		printf("%s page %dx%d: template %d, display-at %d, edit %d, tab pages %d, "
			   "title block %d, borders %d, labelling %s\n",
			   which, page->width(), page->height(),
			   at.templ, at.display_at, at.edit, at.tabs, at.title_block, at.border,
			   labelling ? std::to_string(at.labelling).c_str() : "none");
		return at;
	};

	const Heights small = snap(1000);
	const Heights tall = snap(1400);

		//The Template row keeps its own height however tall the tab is:
		//the free height is the title block widget's to take, not the
		//combos' to stretch into
	check(tall.templ == small.templ, "the template combo keeps its height");
	check(tall.display_at == small.display_at, "the display-at combo keeps its height");
	check(tall.edit == small.edit, "the edit button keeps its height");

		//The free height goes to the title block widget ...
	check(tall.title_block > small.title_block + 300,
		  "the title block widget takes the free height");
		//... and inside it to the pages of its tab, which is what a
		//taller tab is for
	check(tall.tabs > small.tabs + 300, "the tab pages take the free height");

		//Nothing beside it grows -- the borders and the labelling group
		//sit at their natural height and cannot spread apart
	check(tall.border == small.border, "the border widget keeps its height");
	if (labelling) {
		check(tall.labelling == small.labelling,
			  "the labelling group keeps its height");
	}
}

int main(int argc, char **argv)
{
	const QString fixture = QString::fromLocal8Bit(argv[1]);
	const QString report = QString::fromLocal8Bit(argv[2]);
	QTemporaryDir isolated;
	QByteArray config = "--config-dir=" + isolated.path().toUtf8();
	QByteArray data = "--data-dir=" + isolated.path().toUtf8();
	QByteArray collection = "--common-elements-dir=" + isolated.path().toUtf8();
	char *args[] = {argv[0], config.data(), data.data(), collection.data()};
	int count = 4;
	QApplication app(count, args);
	std::freopen(report.toLocal8Bit().constData(), "w", stdout);
	std::setbuf(stdout, nullptr);
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, isolated.path());
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	try {
		auto *qet = new QETApp;
		Q_UNUSED(qet)

			//The application's defaults: the labelling group at the end
			//of the tab, the very case the stretch was removed for
		measure(new NewDiagramPage(nullptr), "defaults", true);

			//A project's own page: nothing at all after the title block
			//widget, so its height is the only thing keeping a widget
			//added later from taking the free height
		QETProject *project = new QETProject(fixture);
		check(project->state() == QETProject::Ok, "fixture loaded");
		measure(new NewDiagramPage(project), "project", false);

		printf("OK\n");
		return 0;
	} catch (const std::exception &e) {
		printf("FAIL: %s\n", e.what());
		return 1;
	}
}
