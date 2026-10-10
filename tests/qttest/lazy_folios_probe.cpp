// SPDX-License-Identifier: GPL-2.0-or-later
// Lazy folios (QET_LAZY_FOLIOS),
// linked against the application's objects: a folio shown in a project
// opened without building its folios draws and links as in the project
// opened as usual, building only it and the folios its links and tables
// reach, and the project saves the same.
#include <QApplication>
#include <QFontDatabase>
#include <QImage>
#include <QPainter>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <stdexcept>
#include "../../sources/diagram.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetresult.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetgraphicsitem/element.h"

static void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

static QImage render(Diagram *folio)
{
	const QRectF rect = folio->border_and_titleblock.borderAndTitleBlockRect().adjusted(-5, -5, 5, 5);
	QImage image(rect.size().toSize(), QImage::Format_RGB32);
	image.fill(Qt::white);
	QPainter painter(&image);
	folio->render(&painter, QRectF(QPointF(0, 0), rect.size()), rect);
	return image;
}

	//Each symbol's partners, in their order
static QStringList links(Diagram *folio)
{
	QStringList list;
	for (Element *element : folio->elements()) {
		QStringList partners;
		for (Element *partner : element->linkedElements())
			partners << partner->uuid().toString();
		list << element->uuid().toString() + ' ' + partners.join(',');
	}
	list.sort();
	return list;
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv); std::freopen(argv[2], "w", stdout);
	QTemporaryDir settings; QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETLazyFoliosRegression");
	QETProject::setBackupEnabled(false); QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");
	try {
		const QString output = QString::fromLocal8Bit(argv[3]);
		qunsetenv("QET_LAZY_FOLIOS");
			//A file saved by this version: its folios carry their uuids,
			//which opening without building them needs
		const QString path = output + "/lazy_folios.qet";
		{
			QETProject source(QString::fromLocal8Bit(argv[1]));
			check(source.state() == QETProject::Ok, "fixture opens");
			source.setFilePath(path);
			check(source.write().isOk(), "fixture saved by this version");
		}
		QETProject eager(path);
		check(eager.state() == QETProject::Ok, "project opens");
		const QList<Diagram *> folios = eager.folios();
		check(folios.size() > 3, "several folios");
		QList<QImage> images; QList<QStringList> partners;
		int linked = 0;
		for (Diagram *folio : folios) {
			images << render(folio);
			partners << links(folio);
			for (Element *element : folio->elements())
				linked += !element->linkedElements().isEmpty();
		}
		check(linked > 0, "the project has linked symbols");

		qputenv("QET_LAZY_FOLIOS", "1");
		auto same = [&](QETProject &lazy, int i, const char *what) {
			Diagram *folio = lazy.folios().at(i);
			lazy.buildFolioToShow(folio);
			if (render(folio) != images.at(i) || links(folio) != partners.at(i)) {
				std::printf("folio %d, %s: differs\n", i + 1, what);
				throw std::runtime_error("a folio shown lazily draws or links as opened as usual");
			}
		};
			//Each folio first after opening
		for (int i = 0; i < folios.size(); ++i) {
			QETProject lazy(path);
			check(lazy.unloadedFolioCount() == folios.size(), "opened without building any folio");
			same(lazy, i, "shown first");
			check(lazy.unloadedFolioCount() > 0, "showing a folio does not build them all");
		}
			//Every folio, one after the other, then the save
		QETProject lazy(path);
		for (int i = 0; i < folios.size(); ++i)
			same(lazy, i, "shown in turn");
		check(lazy.toXml().toString() == eager.toXml().toString(), "saves as opened as usual");
		check(lazy.undoStack()->isClean(), "showing folios changes nothing");
		std::printf("PASS: %d folios\n", int(folios.size()));
	} catch (const std::exception &error) {
		std::printf("FAIL: %s\n", error.what());
		std::fflush(stdout);
		return 1;
	}
	std::fflush(stdout);
	return 0;
}
