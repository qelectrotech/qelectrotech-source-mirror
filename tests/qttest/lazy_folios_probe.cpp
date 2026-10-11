// SPDX-License-Identifier: GPL-2.0-or-later
// Lazy folios (QET_LAZY_FOLIOS),
// linked against the application's objects: a folio shown in a project
// opened without building its folios draws and links as in the project
// opened as usual, building only it and the folios its links and tables
// reach, and the project saves the same.
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QImage>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QRegularExpression>
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
#include "../../sources/autoNum/elementautonumschemecommand.h"
#include "../../sources/elementprovider.h"
#include "../../sources/TerminalStrip/terminalstrip.h"
#include "../../sources/ui/elementpropertieswidget.h"
#include "../../sources/conductorautonumerotation.h"
#include "../../sources/diagramcontent.h"
#include "../../sources/diagramview.h"
#include "../../sources/diagramevent/diagrameventaddpaste.h"
#include <QKeyEvent>
#include "../../sources/qetgraphicsitem/conductor.h"
#include "../../sources/qetgraphicsitem/terminal.h"
#include "../../sources/undocommand/changeelementinformationcommand.h"
#include "../../sources/undocommand/changetitleblockcommand.h"
#include "../../sources/undocommand/deleteqgraphicsitemcommand.h"
#include "../../sources/factory/elementfactory.h"
#include "../../sources/undocommand/addgraphicsobjectcommand.h"

static void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

	//Every symbol's numbering, in a stable order
static QStringList numbering(const QETProject &project)
{
	QStringList list;
	for (const auto &symbol : project.symbolNumbering())
		list << QStringList{symbol.label, symbol.formula, symbol.scheme.toString(),
							symbol.sequence.unit.join(';'), symbol.sequence.ten.join(';'),
							symbol.sequence.hundred.join(';'),
							QString::number(symbol.numbered)}.join('|');
	list.sort();
	return list;
}

	//What the scheme @p title tells of the numbers its symbols carry
static QString gaps(const QETProject &project, const QString &title)
{
	QStringList list;
	for (const auto &gap : ElementAutoNumSchemeCommand::gapRanges(&project, title))
		list << QString("%1-%2").arg(gap.from).arg(gap.to);
	return list.join(',');
}

	//A symbol of @p location placed on folio @p i as the folio's editor
	//places one (DiagramEventAddElement::addElement()), numbered with the
	//scheme @p title: its label, and the numbers then free for it
static QString place(QETProject &project, int i, const QString &location, const QString &title)
{
	Diagram *folio = project.folios().at(i);
	project.buildFolioToShow(folio);
	project.setCurrrentElementAutonum(title);
	int state = 0;
	Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(location, &project), nullptr, &state);
	check(!state, "symbol made");
	element->setPos(QPointF(-500, -500));
	folio->addItem(element);
	auto *undo = new QUndoCommand("insert");
	new AddGraphicsObjectCommand(element, folio, element->pos(), undo);
	element->setUpFormula(true, undo);
	folio->undoStack().push(undo);
	element->freezeNewAddedElement();
	QStringList free;
	for (int n : ElementAutoNumSchemeCommand::freeNumbers(&project, title, element))
		free << QString::number(n);
	return element->actualLabel() + " free " + free.join(',');
}

	//The symbols a search of a project finds, by uuid
static QStringList found(const QVector<QPointer<Element>> &elements)
{
	QStringList list;
	for (const auto &element : elements)
		list << element->uuid().toString();
	list.sort();
	return list;
}

	//Everyday edits on folio @p i, as its editor makes them: a symbol's
	//information, a wire, the folio's title block, a linked symbol
	//deleted, then one undo and one redo
static void edit(QETProject &project, int i)
{
	Diagram *folio = project.folios().at(i);
	project.buildFolioToShow(folio);
	QList<Element *> elements = folio->elements();
	std::sort(elements.begin(), elements.end(), [](Element *a, Element *b) {
		return a->uuid() < b->uuid(); });
	check(elements.size() > 2, "a folio with symbols to edit");

	const DiagramContext old_info = elements.first()->elementInformations();
	DiagramContext new_info = old_info;
	new_info.addValue(QStringLiteral("comment"), QStringLiteral("edited lazily"));
	folio->undoStack().push(new ChangeElementInformationCommand(elements.first(), old_info, new_info));

	Terminal *ends[2] = {nullptr, nullptr};
	for (Element *element : elements) {
		for (Terminal *terminal : element->terminals()) {
			if (terminal->conductors().isEmpty()
					&& (!ends[0] || ends[0]->parentElement() != element)) {
				ends[ends[0] ? 1 : 0] = terminal;
				break;
			}
		}
		if (ends[1]) break;
	}
	if (ends[1]) {
		auto *undo = new QUndoCommand(QStringLiteral("wire"));
		auto *wire = new Conductor(ends[0], ends[1]);
		new AddGraphicsObjectCommand(wire, folio, QPointF(), undo);
		ConductorAutoNumerotation numbering(wire, folio, undo);
		numbering.numerate();
		folio->undoStack().push(undo);
	}

	const TitleBlockProperties old_block = folio->border_and_titleblock.exportTitleBlock();
	TitleBlockProperties new_block = old_block;
	new_block.title += QStringLiteral(" (edited)");
	folio->undoStack().push(new ChangeTitleBlockCommand(folio, old_block, new_block));

		//Copy three symbols and paste them, as Ctrl+C then Ctrl+Shift+V
		//and Return do
	{
		DiagramView view(folio);
		folio->clearSelection();
		for (int n = 0 ; n < 3 ; ++n)
			elements.at(n)->setSelected(true);
		view.copy();
		auto *paste = new DiagramEventAddPaste(folio, QPointF(), DiagramEventAddPaste::AtOrigin);
		folio->setEventInterface(paste);
		QKeyEvent drop(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
		paste->keyPressEvent(&drop);
		folio->clearEventInterface();
	}

	Element *linked = nullptr;
	for (Element *element : elements)
		if (!element->linkedElements().isEmpty()) { linked = element; break; }
	folio->clearSelection();
	(linked ? linked : elements.last())->setSelected(true);
	folio->undoStack().push(new DeleteQGraphicsItemCommand(folio, DiagramContent(folio, true)));

	project.undoStack()->undo();
	project.undoStack()->redo();
}

	//The rows of the project database's tables, in their order, read as
	//every caller reads them (newQuery(): brought up to date first)
static QStringList tables(QETProject &project)
{
	QStringList rows;
	for (const char *table : {"diagram", "diagram_info", "element", "element_info",
							  "terminal", "conductor", "link"}) {
		QSqlQuery query = project.dataBase()->newQuery(
					QStringLiteral("SELECT * FROM %1 ORDER BY rowid").arg(QLatin1String(table)));
		check(query.exec(), "a table read");
		QStringList table_rows;
		while (query.next()) {
			QStringList row;
			for (int i = 0 ; i < query.record().count() ; ++i)
				row << query.value(i).toString();
			table_rows << QLatin1String(table) + ": " + row.join('|');
		}
			//Rows are written in the order the scene gives a folio's items,
			//which is not the same from one opening to the next: two usual
			//opens given the same edits differ in it (637 rows of 3 394 on
			//industrial.qet). What a table holds is compared, not its order
		table_rows.sort();
		rows << table_rows;
	}
	return rows;
}

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
		QSet<int> strip_folios;
		{
			QETProject source(QString::fromLocal8Bit(argv[1]));
			check(source.state() == QETProject::Ok, "fixture opens");
				//A terminal strip of terminals on several folios: it is
				//read with the project, before any folio is built
			TerminalStrip *strip = source.newTerminalStrip(QStringLiteral("=A1"), QStringLiteral("+B1"),
														  QStringLiteral("X99"));
			int added = 0;
			for (Diagram *folio : source.folios())
				for (Element *element : folio->elements())
					if (element->elementData().m_type == ElementData::Terminal && added < 12
							&& strip->addTerminal(element)) {
						++added;
						strip_folios.insert(source.folioIndex(folio));
					}
			check(added > 3, "a terminal strip with terminals");
			source.setFilePath(path);
			check(source.write().isOk(), "fixture saved by this version");
		}
		QETProject eager(path);
		check(eager.state() == QETProject::Ok, "project opens");
		{
			qputenv("QET_LAZY_FOLIOS", "1");
			QETProject lazy_strips(path);
			qunsetenv("QET_LAZY_FOLIOS");
			auto strip_of = [](const QETProject &project) {
				for (TerminalStrip *strip : project.terminalStrip())
					if (strip->name() == QStringLiteral("X99")) return strip->physicalTerminalCount();
				return -1;
			};
			check(strip_of(lazy_strips) > 3 && strip_of(lazy_strips) == strip_of(eager),
				  "a terminal strip keeps its terminals when the folios are not built");
		}
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
			check(lazy.unloadedFolioCount() == folios.size() - strip_folios.size(),
				  "opened building only the folios of the terminal strip's terminals");
			same(lazy, i, "shown first");
			check(lazy.unloadedFolioCount() > 0, "showing a folio does not build them all");
		}
			//Every folio, one after the other, then the save
		QETProject lazy(path);
		for (int i = 0; i < folios.size(); ++i)
			same(lazy, i, "shown in turn");
		check(lazy.toXml().toString() == eager.toXml().toString(), "saves as opened as usual");
		check(lazy.undoStack()->isClean(), "showing folios changes nothing");

			//Numbering asks every symbol of the project: it is answered for
			//the folios not built without building them, as when built
		QETProject numbered(path);
		check(numbering(numbered) == numbering(eager), "symbols' numbering as when built");
		const QStringList titles = eager.elementAutoNum().keys();
		check(!titles.isEmpty(), "the project has element numbering schemes");
		for (const QString &title : titles)
			check(gaps(numbered, title) == gaps(eager, title), "a scheme's gaps as when built");
		check(numbered.unloadedFolioCount() == folios.size() - strip_folios.size(), "numbering read without building");
			//A symbol placed on a folio, numbered by a scheme others follow
		QString location, title;
		int folio = -1;
		for (const auto &symbol : eager.symbolNumbering()) {
			if (symbol.numbered && symbol.element && !eager.elementAutoNumTitle(symbol.scheme).isEmpty()) {
				location = symbol.element->location().toString();
				title = eager.elementAutoNumTitle(symbol.scheme);
				folio = eager.folios().indexOf(symbol.element->diagram());
				break;
			}
		}
		check(folio >= 0, "a symbol following a scheme");
		const QString placed = place(numbered, folio, location, title);
		std::printf("placed on folio %d with %s: %s\n", folio + 1, qPrintable(title), qPrintable(placed));
		check(numbered.unloadedFolioCount() > 0, "numbering a placed symbol does not build every folio");
		QETProject usual(path);
		check(placed == place(usual, folio, location, title), "a placed symbol numbered as when built");
			//The candidates a link tab lists (ElementProvider): found as
			//when built, building only the folios which hold some
		const ElementProvider usual_search(&eager);
		for (const ElementData::Types kinds : {ElementData::Types(ElementData::Slave),
											   ElementData::Types(ElementData::Master),
											   ElementData::Types(ElementData::AllReport),
											   ElementData::Types(ElementData::Terminal)}) {
			QETProject searched(path);
			const ElementProvider search(&searched);
			const QStringList lazy_free = found(search.freeElement(kinds)), usual_free = found(usual_search.freeElement(kinds));
			if (lazy_free != usual_free) {
				for (const QString &u : usual_free) if (!lazy_free.contains(u)) std::printf("  only when built: %s\n", qPrintable(u));
				for (const QString &u : lazy_free) if (!usual_free.contains(u)) std::printf("  only lazily: %s\n", qPrintable(u));
			}
			check(lazy_free == usual_free, "free symbols found as when built");
			std::printf("free symbols of kinds %d: %d folios built\n", int(kinds),
						int(folios.size() - searched.unloadedFolioCount()));
			check(found(search.find(kinds)) == found(usual_search.find(kinds)),
				  "symbols found as when built");
		}
		{
			QETProject searched(path);
			ElementProvider(&searched).freeElement(ElementData::Slave);
			check(searched.unloadedFolioCount() > 0, "a search for free contacts does not build every folio");
		}
			//Selecting a symbol shows its properties, a link tab for a
			//coil, a contact or a report: the folio and few others built
		for (const Element::kind kind : {Element::Master, Element::Slave, Element::NextReport,
										 Element::PreviousReport}) {
			QETProject selected(path);
			Element *element = nullptr;
			for (int i = 0 ; !element && i < folios.size() ; ++i) {
				for (Element *candidate : eager.folios().at(i)->elements()) {
					if (candidate->linkType() == kind) {
						selected.buildFolioToShow(selected.folios().at(i));
						for (Element *e : selected.folios().at(i)->elements())
							if (e->uuid() == candidate->uuid()) element = e;
						break;
					}
				}
			}
			check(element, "a symbol of each kind with a link tab");
			const int before = selected.unloadedFolioCount();
			delete new ElementPropertiesWidget(element);
			std::printf("properties of a symbol of kind %d: %d more folios built, %d not built\n",
						int(kind), before - selected.unloadedFolioCount(), selected.unloadedFolioCount());
			check(selected.unloadedFolioCount() > 0, "selecting a symbol does not build every folio");
		}
			//Everyday edits on a folio linked to others: the others stay
			//not built, and the project saves as when they are
		int busiest = 0, most = -1;
		for (int i = 0 ; i < folios.size() ; ++i) {
			int partners = 0;
			for (Element *element : folios.at(i)->elements())
				for (Element *partner : element->linkedElements())
					partners += partner->diagram() != folios.at(i);
			if (partners > most) { most = partners; busiest = i; }
		}
		{
			QETProject edited(path), usual(path);
			qunsetenv("QET_LAZY_FOLIOS");
			QETProject usual_edited(path);
			qputenv("QET_LAZY_FOLIOS", "1");
			edit(edited, busiest);
			std::printf("edits on folio %d (%d links to other folios): %d folios not built\n",
						busiest + 1, most, edited.unloadedFolioCount());
			check(edited.unloadedFolioCount() > 0, "editing a folio does not build every folio");
			edit(usual_edited, busiest);
				//Made by the edits, a new wire's uuid is random: any uuid the
				//file had not is "new" before comparing
			QFile saved(path); saved.open(QIODevice::ReadOnly);
			const QString before = QString::fromUtf8(saved.readAll());
			static const QRegularExpression uuid_re(QStringLiteral("uuid=\"(\\{[0-9a-f-]{36}\\})\""));
			auto known = [&before](QString xml) {
				QString out;
				qsizetype at = 0;
				for (auto it = uuid_re.globalMatch(xml) ; it.hasNext() ; ) {
					const auto match = it.next();
					out += xml.mid(at, match.capturedStart() - at);
					out += before.contains(match.captured(1)) ? match.captured(0)
															  : QStringLiteral("uuid=\"new\"");
					at = match.capturedEnd();
				}
				return out + xml.mid(at);
			};
			const QStringList lazy_rows = tables(edited), usual_rows = tables(usual_edited);
			static const QRegularExpression bare_uuid_re(QStringLiteral("\\{[0-9a-f-]{36}\\}"));
			auto known_row = [&before](QString row) {
				for (auto it = bare_uuid_re.globalMatch(QString(row)) ; it.hasNext() ; ) {
					const QString uuid = it.next().captured(0);
					if (!before.contains(uuid)) row.replace(uuid, QStringLiteral("new"));
				}
				return row;
			};
			check(edited.unloadedFolioCount() > 0, "reading the database does not build every folio");
			QStringList lazy_known, usual_known;
			for (const QString &row : lazy_rows) lazy_known << known_row(row);
			for (const QString &row : usual_rows) usual_known << known_row(row);
			lazy_known.sort();
			usual_known.sort();
			int differing = 0;
			for (int i = 0 ; i < std::max(lazy_known.size(), usual_known.size()) ; ++i)
				if (lazy_known.value(i) != usual_known.value(i) && differing++ < 5)
					std::printf("  row %d: lazy  %s\n         usual %s\n", i, qPrintable(lazy_known.value(i).left(160)),
								qPrintable(usual_known.value(i).left(160)));
			std::printf("database after the edits: %d rows, %d differ\n", int(usual_rows.size()), differing);
			check(!differing, "the database after the edits as when every folio is built");
				//A pasted copy has a random uuid, and a folio's symbols are
				//saved in uuid order: the lines are compared, not their order
			auto lines = [&known](const QString &xml) {
				QStringList list = known(xml).split(QLatin1Char('\n'));
				list.sort();
				return list.join(QLatin1Char('\n'));
			};
			const QString a = lines(edited.toXml().toString()), b = lines(usual_edited.toXml().toString());
			if (a != b) {
				QFile fa(output + "/edited-lazy.xml"); fa.open(QIODevice::WriteOnly); fa.write(a.toUtf8());
				QFile fb(output + "/edited-usual.xml"); fb.open(QIODevice::WriteOnly); fb.write(b.toUtf8());
			}
			check(a == b,
				  "edits save as when every folio is built");
		}
		std::printf("PASS: %d folios\n", int(folios.size()));
	} catch (const std::exception &error) {
		std::printf("FAIL: %s\n", error.what());
		std::fflush(stdout);
		return 1;
	}
	std::fflush(stdout);
	return 0;
}
