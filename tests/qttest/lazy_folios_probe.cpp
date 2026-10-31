// SPDX-License-Identifier: GPL-2.0-or-later
// Lazy folios (on by default, QET_EAGER_FOLIOS=1 for every folio built),
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
#include "../../sources/container/qetzip.h"
#include <QSqlDatabase>
#include <QMap>
#include "../../sources/TerminalStrip/terminalstrip.h"
#include "../../sources/ui/elementpropertieswidget.h"
#include "../../sources/conductorautonumerotation.h"
#include "../../sources/conductornumexport.h"
#include "../../sources/scripting/qetscriptapi.h"
#include "../../sources/wiringlistexport.h"
#include "../../sources/ui/terminalnumberingdialog.h"
#include "../../sources/diagramcontent.h"
#include "../../sources/diagramview.h"
#include "../../sources/diagramevent/diagrameventaddpaste.h"
#include <QKeyEvent>
#include "../../sources/qetgraphicsitem/conductor.h"
#include "../../sources/qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "../../sources/qetgraphicsitem/terminal.h"
#include "../../sources/undocommand/changeelementinformationcommand.h"
#include "../../sources/undocommand/changetitleblockcommand.h"
#include "../../sources/undocommand/deleteqgraphicsitemcommand.h"
#include "../../sources/factory/elementfactory.h"
#include "../../sources/undocommand/addgraphicsobjectcommand.h"
#include "../../sources/dataBase/projectdatabase.h"
#include <QDomDocument>

static void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
static QStringList sortedLines(QString xml, const QString &path);

	//A new revision on every folio (its title block's revision index and
	//author), saved lazily and with every folio built: what saving builds,
	//and whether the two saves hold the same. A change to folio
	//@p label_folio's label (what formulas read) when it is not -1
static void revisionEveryFolio(const QString &source, const QString &output, const QString &name,
							   const QString &path, int label_folio = -1)
{
	QString error;
	qunsetenv("QET_EAGER_FOLIOS");
	QETProject revised(source);
	const int unbuilt = revised.unloadedFolioCount();
	QetScriptApi api(&revised);
	for (int i = 0 ; i < api.folioCount() ; ++i) {
		check(api.setFolioProperty(i, "indexrev", "B"), "a folio's revision set");
		check(api.setFolioProperty(i, "author", "Revised"), "a folio's author set");
	}
	if (label_folio >= 0)
		check(api.setFolioProperty(label_folio, "folio", "X%id"), "a folio's label set");
	check(revised.unloadedFolioCount() == unbuilt, "setting the folios' title blocks builds none");
	int with_tables = 0;
	for (Diagram *folio : revised.folios())
		if (!folio->isLoaded() && revised.unbuiltFolioHasTables(folio)) ++with_tables;
	const bool qetz = QETProject::isContainerPath(source);
	const QString lazy_path = output + "/" + name + (qetz ? ".qetz" : ".qet");
	if (qetz) check(revised.writeContainerFile(lazy_path, &error), "revised project saved");
	else { revised.setFilePath(lazy_path); check(revised.write().isOk(), "revised project saved"); }
	const int built = unbuilt - revised.unloadedFolioCount();
	std::printf("%s: new revision on every folio, %d folios built by saving, %d hold a table%s\n",
				qPrintable(name), built, with_tables,
				label_folio >= 0 ? ", one label changed" : "");
	if (label_folio < 0)
		check(built <= with_tables, "saving a new revision builds only the folios holding a table");
	else
		check(revised.folios().at(label_folio)->isLoaded(), "a folio whose label changed is built to save");
	qputenv("QET_EAGER_FOLIOS", "1");
	const QString usual_path = output + "/" + name + "-usual" + (qetz ? ".qetz" : ".qet");
	{
		QETProject usual(source);
		QetScriptApi usual_api(&usual);
		for (int i = 0 ; i < usual_api.folioCount() ; ++i) {
			usual_api.setFolioProperty(i, "indexrev", "B");
			usual_api.setFolioProperty(i, "author", "Revised");
		}
		if (label_folio >= 0) usual_api.setFolioProperty(label_folio, "folio", "X%id");
		if (qetz) check(usual.writeContainerFile(usual_path, &error), "revision saved when built");
		else { usual.setFilePath(usual_path); check(usual.write().isOk(), "revision saved when built"); }
	}
	QETProject lazy_back(lazy_path), usual_back(usual_path);
	qunsetenv("QET_EAGER_FOLIOS");
		//What a save says of itself (file name, path, time) left out
	auto lines = [&path](QETProject &project) {
		QStringList list = sortedLines(project.toXml().toString(), path);
		list.removeIf([](const QString &line) { return line.contains(QLatin1String("name=\"saved")); });
		return list;
	};
	const QStringList lazy_lines = lines(lazy_back), usual_lines = lines(usual_back);
	if (lazy_lines != usual_lines) {
		int shown = 0;
		for (const QString &line : usual_lines)
			if (!lazy_lines.contains(line) && shown++ < 8) std::printf("  when built: %s\n", qPrintable(line.left(300)));
		for (const QString &line : lazy_lines)
			if (!usual_lines.contains(line) && shown++ < 16) std::printf("  lazily:     %s\n", qPrintable(line.left(300)));
	}
	check(lazy_lines == usual_lines, "a new revision saved as when built");
}

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

	//The project as saving writes it: folios not built written as read
	//when building them would write the same (QETProject::toXml())
static QString saved(QETProject &project)
{
	QHash<int, QByteArray> kept;
	const QDomDocument document = project.toXml(&kept);
	return QETProject::spliceKeptFolios(document.toString(4), kept);
}

	//@p xml's lines in order, any uuid not in the file @p path "new": a
	//pasted copy's and a new wire's are random, and a folio's symbols are
	//saved in uuid order
static QStringList sortedLines(QString xml, const QString &path)
{
	QFile file(path); file.open(QIODevice::ReadOnly);
	const QString before = QString::fromUtf8(file.readAll());
	static const QRegularExpression uuid_re(QStringLiteral("\\{[0-9a-f-]{36}\\}"));
	for (auto it = uuid_re.globalMatch(QString(xml)) ; it.hasNext() ; ) {
		const QString uuid = it.next().captured(0);
		if (!before.contains(uuid)) xml.replace(uuid, QStringLiteral("new"));
	}
	QStringList list = xml.split(QLatin1Char('\n'));
	list.sort();
	return list;
}

	//The saved links of a .qetz, symbol by symbol in their order: what a
	//symbol saves of its partners, as its file holds it
static QStringList savedLinks(const QString &qetz, const QString &path)
{
	QFile file(qetz); file.open(QIODevice::ReadOnly);
	QList<QetZip::Entry> entries;
	QString error;
	check(QetZip::fromBytes(file.readAll(), &entries, &error), "a .qetz read");
	QTemporaryDir dir;
	const QString db_path = dir.filePath(QStringLiteral("project.sqlite"));
	for (const QetZip::Entry &entry : entries)
		if (entry.name == QLatin1String("project.sqlite")) {
			QFile db(db_path); db.open(QIODevice::WriteOnly); db.write(entry.data);
		}
	QMap<QString, QStringList> links;   //symbol -> its partners in order
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("saved_links"));
		db.setDatabaseName(db_path);
		check(db.open(), "a .qetz's database opened");
		QSqlQuery query(db);
		check(query.exec(QStringLiteral("SELECT element_uuid, linked_uuid FROM link ORDER BY element_uuid, ord")),
			  "saved links read");
		while (query.next())
			links[query.value(0).toString()] << query.value(1).toString();
	}
	QSqlDatabase::removeDatabase(QStringLiteral("saved_links"));
	QStringList rows;
	for (auto it = links.cbegin() ; it != links.cend() ; ++it)
		rows << it.key() + QStringLiteral(": ") + it.value().join(QLatin1Char(','));
	return sortedLines(rows.join(QLatin1Char('\n')), path);
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
		//Folios not built saved as read (QETProject::toXml()): this
		//fixture is written by this version's own save
	qputenv("QET_LAZY_SAVE_KEPT", "1");
	try {
		const QString output = QString::fromLocal8Bit(argv[3]);
		qputenv("QET_EAGER_FOLIOS", "1");
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
			qunsetenv("QET_EAGER_FOLIOS");
			QETProject lazy_strips(path);
			qputenv("QET_EAGER_FOLIOS", "1");
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

		qunsetenv("QET_EAGER_FOLIOS");
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
		revisionEveryFolio(path, output, "revised-qet", path);
		{
				//Wire numbers exported from the database filled from the
				//document, as when built (D5)
			QETProject exported(path);
			check(ConductorNumExport(&exported).wiresNum() == ConductorNumExport(&eager).wiresNum(),
				  "wire numbers of a .qet exported as when built");
			const QString wiring = WiringListExport(&exported).toCsvString(), usual_wiring = WiringListExport(&eager).toCsvString();
			if (wiring != usual_wiring) {
				const QStringList a = wiring.split('\n'), b = usual_wiring.split('\n');
				for (const QString &line : b) if (!a.contains(line)) std::printf("  wiring when built: %s\n", qPrintable(line));
				for (const QString &line : a) if (!b.contains(line)) std::printf("  wiring lazily:     %s\n", qPrintable(line));
				std::printf("  %d lines lazily, %d when built\n", int(a.size()), int(b.size()));
			}
			check(wiring == usual_wiring, "wiring list of a .qet exported as when built");
			check(exported.unloadedFolioCount() == folios.size() - strip_folios.size(),
				  "exporting a .qet's wire numbers and wiring list builds no folio");
		}
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
			qputenv("QET_EAGER_FOLIOS", "1");
			QETProject usual_edited(path);
			qunsetenv("QET_EAGER_FOLIOS");
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
			//Saving writes the folios not built as they were read, the same
			//bytes as building them would write
		const QString usual_text = saved(eager);
		{
			QETProject untouched(path);
			const int unbuilt = untouched.unloadedFolioCount();
			const QString untouched_text = saved(untouched);
			if (untouched_text != usual_text) {
				QFile fa(output + "/untouched-lazy.xml"); fa.open(QIODevice::WriteOnly); fa.write(untouched_text.toUtf8());
				QFile fb(output + "/untouched-usual.xml"); fb.open(QIODevice::WriteOnly); fb.write(usual_text.toUtf8());
			}
			check(untouched_text == usual_text, "a project saved without building, as when built");
				//Only the partners of the folios the terminal strip built
			std::printf("saved untouched: %d of %d folios not built built to save\n",
						unbuilt - untouched.unloadedFolioCount(), unbuilt);
			check(untouched.unloadedFolioCount() > 0, "saving does not build every folio");
		}
			//After everyday edits on a folio: as when built, others not built
		{
			QETProject edited(path), usual(path);
			qputenv("QET_EAGER_FOLIOS", "1");
			QETProject usual_edited(path);
			qunsetenv("QET_EAGER_FOLIOS");
			edit(edited, busiest);
			edit(usual_edited, busiest);
			QFile file(path); file.open(QIODevice::ReadOnly);
			const QString before = QString::fromUtf8(file.readAll());
			static const QRegularExpression uuid_re(QStringLiteral("\\{[0-9a-f-]{36}\\}"));
			auto lines = [&before](QString text) {
				for (auto it = uuid_re.globalMatch(QString(text)) ; it.hasNext() ; ) {
					const QString uuid = it.next().captured(0);
					if (!before.contains(uuid)) text.replace(uuid, QStringLiteral("new"));
				}
				QStringList list = text.split(QLatin1Char('\n'));
				list.sort();
				return list;
			};
			const QStringList la = lines(saved(edited)), lb = lines(saved(usual_edited));
			if (la != lb) {
				int shown = 0;
				for (const QString &l : lb) if (!la.contains(l) && shown++ < 6) std::printf("  only usual: %s\n", qPrintable(l.trimmed().left(150)));
				shown = 0;
				for (const QString &l : la) if (!lb.contains(l) && shown++ < 6) std::printf("  only lazy:  %s\n", qPrintable(l.trimmed().left(150)));
			}
			check(la == lb, "edits saved without building, as when built");
			std::printf("saved after the edits: %d folios not built\n", edited.unloadedFolioCount());
			check(edited.unloadedFolioCount() > 0, "saving after edits does not build every folio");
		}
			//A folio not built given a new title: it and the folios its
			//links reach are built to be saved, the others not
		{
			QETProject renamed(path);
			qputenv("QET_EAGER_FOLIOS", "1");
			QETProject usual_renamed(path);
			qunsetenv("QET_EAGER_FOLIOS");
			for (QETProject *project : {&renamed, &usual_renamed}) {
				Diagram *folio = project->folios().at(busiest);
				TitleBlockProperties block = folio->border_and_titleblock.exportTitleBlock();
				block.title += QStringLiteral(" (renamed)");
				folio->border_and_titleblock.importTitleBlock(block);
			}
			check(saved(renamed) == saved(usual_renamed), "a renamed folio saved as when built");
			std::printf("saved after a folio not built was renamed: %d folios not built\n", renamed.unloadedFolioCount());
			check(renamed.unloadedFolioCount() > 0, "renaming a folio does not build every folio to save");
		}
			//Project properties, which any folio's texts can use: every
			//folio is built to be saved
		{
			QETProject changed(path);
			qputenv("QET_EAGER_FOLIOS", "1");
			QETProject usual_changed(path);
			qunsetenv("QET_EAGER_FOLIOS");
			for (QETProject *project : {&changed, &usual_changed}) {
				DiagramContext properties = project->projectProperties();
				properties.addValue(QStringLiteral("lazy_test"), QStringLiteral("1"));
				project->setProjectProperties(properties);
			}
			check(saved(changed) == saved(usual_changed), "changed project properties saved as when built");
			check(changed.unloadedFolioCount() == 0, "changed project properties build every folio to save");
		}
			//A potential through folio reports is followed as far as when
			//every folio is built
		{
			Conductor *usual_wire = nullptr;
			for (Diagram *folio : eager.folios())
				for (Element *element : folio->elements())
					if ((element->linkType() & Element::AllReport) && !element->conductors().isEmpty()
							&& !usual_wire)
						usual_wire = element->conductors().first();
			check(usual_wire, "a wire on a folio report");
			auto potential = [](Conductor *wire) {
				QStringList list;
				for (Conductor *c : wire->relatedPotentialConductors(true))
					list << c->uuid().toString();
				list.sort();
				return list;
			};
			QETProject followed(path);
			Diagram *folio = followed.folios().at(eager.folioIndex(usual_wire->diagram()));
			followed.buildFolioToShow(folio);
			Conductor *wire = nullptr;
			for (Conductor *c : folio->conductors())
				if (c->uuid() == usual_wire->uuid()) wire = c;
			check(wire, "the same wire");
			check(potential(wire) == potential(usual_wire), "a potential followed as when built");
			std::printf("potential through reports: %d wires, %d folios not built\n",
						int(potential(wire).size()), followed.unloadedFolioCount());
		}
			//Two symbols sharing a uuid: the database is filled from the
			//folios, which builds every one of them, so the project opens
			//with every folio built rather than building them all later
		{
			QFile file(path);
			check(file.open(QIODevice::ReadOnly), "fixture read");
			QDomDocument document;
			check(bool(document.setContent(&file)), "fixture parsed");
			file.close();
			check(projectDataBase::documentRefusal(document).isEmpty(), "the fixture fills the database itself");
			QDomNodeList elements = document.elementsByTagName(QStringLiteral("element"));
			QList<QDomElement> placed;
			for (int i = 0; i < elements.size(); ++i) {
				QDomElement e = elements.at(i).toElement();
				if (Element::valideXml(e)) placed << e;
			}
			check(placed.size() > 1, "two placed symbols");
			placed.last().setAttribute(QStringLiteral("uuid"), placed.first().attribute(QStringLiteral("uuid")));
			check(!projectDataBase::documentRefusal(document).isEmpty(), "a shared uuid is seen before the folios are read");
			const QString shared = output + "/lazy_folios_shared_uuid.qet";
			QFile out(shared);
			check(out.open(QIODevice::WriteOnly), "copy written");
			out.write(document.toByteArray(4));
			out.close();
			qunsetenv("QET_EAGER_FOLIOS");
			qputenv("QET_LAZY_FOLIOS_TRACE", "1");
			static int built_late; built_late = 0;
			const QtMessageHandler previous = qInstallMessageHandler(
					[](QtMsgType, const QMessageLogContext &, const QString &message) {
				if (message.contains(QStringLiteral("built for a caller of diagrams()"))) ++built_late;
			});
			QETProject opened(shared);
			qInstallMessageHandler(previous);
			qunsetenv("QET_LAZY_FOLIOS_TRACE");
			check(opened.state() == QETProject::Ok, "a project with a shared uuid opens");
			check(opened.unloadedFolioCount() == 0, "a project with a shared uuid opens with every folio built");
			check(built_late == 0, "a project with a shared uuid builds its folios as it reads them, not later");
		}
			//A .qetz opened lazily: no folio built but those
			//the terminal strip needs, each built when shown, drawn and
			//linked as when built, saved as the .qet is
		{
			const QString zipped = output + "/lazy_folios.qetz";
			QString error;
			check(eager.writeContainerFile(zipped, &error), "saved as a .qetz");
			QETProject indexed(zipped);
			check(indexed.state() == QETProject::Ok, "the .qetz opens");
			std::printf(".qetz opened: %d of %d folios not built (strip folios %d)\n",
						indexed.unloadedFolioCount(), int(folios.size()), int(strip_folios.size()));
			check(indexed.unloadedFolioCount() == folios.size() - strip_folios.size(),
				  "the .qetz opens building only the folios of the terminal strip's terminals");
			check(numbering(indexed) == numbering(eager), "symbols' numbering of a .qetz as when built");
				//The drawn items, through the view made of their tables: the
				//folios built for it, as for the tables (D6: none were)
			{
				QETProject drawn(zipped);
				auto count = [](QETProject &project) {
					QSqlQuery query = project.dataBase()->newQuery(QStringLiteral("SELECT COUNT(*) FROM drawing_item_view"));
					return query.exec() && query.next() ? query.value(0).toInt() : -1;
				};
				const int lazily = count(drawn), built = count(eager);
				std::printf(".qetz drawn items through their view: %d lazily, %d when built\n", lazily, built);
				check(built > 0, "the project has drawn items");
				check(lazily == built, "the drawn items' view as when built");
			}
				//Every wire's potential from the database as QElectroTech
				//follows it with every folio built
			{
				QETProject indexed_potentials(zipped);
				int same = 0, differ = 0, unknown = 0;
				for (Diagram *folio : eager.folios())
					for (Conductor *wire : folio->conductors()) {
						QSet<QUuid> live{wire->uuid()};
						for (Conductor *c : wire->relatedPotentialConductors(true)) live.insert(c->uuid());
						for (QETProject *project : {&eager, &indexed_potentials}) {
							const auto stored = project->dataBase()->potentialWires(wire->uuid());
							if (!stored) { ++unknown; continue; }
							if (*stored == live) { ++same; continue; }
							if (differ++ < 3)
								std::printf("  potential of %s: %d wires followed, %d from the database\n",
											qPrintable(wire->uuid().toString()), int(live.size()), int(stored->size()));
						}
					}
				std::printf("potentials from the database: %d same, %d differ, %d not answered\n", same, differ, unknown);
				check(differ == 0 && same > 0, "every wire's potential from the database as followed");
				check(indexed_potentials.unloadedFolioCount() > 0, "potentials read without building");
			}
				//The folios' rows as saved, not as reading their headers one
				//by one numbers them ("1/1" for the first of three, D7)
			{
				auto folioRows = [](QETProject &project) {
					QStringList rows;
					for (const QString &row : tables(project))
						if (row.startsWith("diagram")) rows << row;
					return rows;
				};
				check(folioRows(indexed) == folioRows(eager), "the folios' rows of a .qetz as when built");
					//Folio numbers that count the folios ("%id/%total")
				const QString totals = output + "/totals.qetz";
				qputenv("QET_EAGER_FOLIOS", "1");
				{
					QETProject numbered(zipped);
					QetScriptApi api(&numbered);
					for (int i = 0 ; i < api.folioCount() ; ++i)
						api.setFolioProperty(i, "folio", "%id/%total");
					check(numbered.writeContainerFile(totals, &error), "folio numbers with totals saved");
				}
				QETProject usual_totals(totals);
				qunsetenv("QET_EAGER_FOLIOS");
				QETProject lazy_totals(totals);
				check(lazy_totals.unloadedFolioCount() > 0, "opened lazily");
				const QStringList lazy_rows = folioRows(lazy_totals), usual_rows = folioRows(usual_totals);
				if (lazy_rows != usual_rows)
					for (int i = 0 ; i < lazy_rows.size() && i < 3 ; ++i)
						std::printf("  lazily %s\n  built  %s\n", qPrintable(lazy_rows.at(i).left(140)),
									qPrintable(usual_rows.value(i).left(140)));
				check(lazy_rows == usual_rows, "folio numbers with totals of a .qetz as when built");
			}
			for (int i = 0; i < folios.size(); ++i)
				same(indexed, i, "shown from the .qetz");
			check(indexed.toXml().toString() == eager.toXml().toString(), "the .qetz saves as when built");
				//A folio whose table continues one on another folio, shown
				//first: the folios its table is built with (D5)
			int continued = 0;
			for (int i = 0; i < folios.size(); ++i) {
				bool continues = false;
				for (QGraphicsItem *item : folios.at(i)->items())
					if (item->type() == QetGraphicsTableItem::Type
							&& !static_cast<QetGraphicsTableItem *>(item)->previousTableUuid().isNull())
						continues = true;
				if (!continues) continue;
				QETProject fresh(zipped);
				same(fresh, i, "a continued table shown first from the .qetz");
				++continued;
			}
			check(continued > 0, "a folio whose table continues another");

				//Numbering, the searches for symbols and the unused symbols
				//are answered from the database (D5)
			{
				QETProject asked(zipped);
				check(numbering(asked) == numbering(eager), "numbering of a .qetz as when built");
				for (const QString &title : eager.elementAutoNum().keys())
					check(gaps(asked, title) == gaps(eager, title), "a .qetz scheme's gaps as when built");
				auto locations = [](const QList<ElementsLocation> &list) {
					QStringList strings;
					for (const ElementsLocation &location : list) strings << location.toString();
					strings.sort();
					return strings;
				};
				check(locations(asked.unusedElements()) == locations(eager.unusedElements()),
					  "a .qetz's unused symbols as when built");
				const ElementProvider usual_search(&eager), search(&asked);
				for (const ElementData::Types kinds : {ElementData::Types(ElementData::Slave),
													   ElementData::Types(ElementData::Master),
													   ElementData::Types(ElementData::AllReport)})
					check(found(search.freeElement(kinds)) == found(usual_search.freeElement(kinds)),
						  "free symbols of a .qetz found as when built");
				const int built = int(folios.size()) - asked.unloadedFolioCount();
				std::printf(".qetz asked numbering, searches, unused symbols: %d folios built\n", built);
				check(asked.unloadedFolioCount() > 0, "asking a .qetz does not build every folio");
			}

				//A folio added, moved and removed: no other folio built, the
				//folios' rows as when every folio is built (D5)
			{
				auto shuffle = [](QETProject &project) {
					Diagram *added = project.addNewDiagram();
					project.diagramOrderChanged(1, 0);
					project.removeDiagram(added);
				};
				auto folioRows = [](QETProject &project) {
					QStringList rows;
					for (const QString &row : tables(project))
						if (row.startsWith("diagram")) rows << row;
					return rows;
				};
				QETProject shuffled(zipped);
				const int unbuilt = shuffled.unloadedFolioCount();
				shuffle(shuffled);
				std::printf(".qetz folio added, moved, removed: %d folios built\n",
							unbuilt - shuffled.unloadedFolioCount());
				check(shuffled.unloadedFolioCount() == unbuilt, "adding, moving and removing a folio builds no other");
				qputenv("QET_EAGER_FOLIOS", "1");
				QETProject usual_shuffled(zipped);
				qunsetenv("QET_EAGER_FOLIOS");
				shuffle(usual_shuffled);
				check(folioRows(shuffled) == folioRows(usual_shuffled), "folios' rows after a move as when built");
			}

				//The tables a table can continue (its properties editor) and
				//the wiring list's count of wires left out: only the folios
				//holding a table built (D5)
			{
				QETProject listed(zipped);
				const int unbuilt = listed.unloadedFolioCount();
				auto tableUuids = [](const QVector<QetGraphicsTableItem *> &list) {
					QStringList uuids;
					for (QetGraphicsTableItem *table : list) uuids << table->uuid().toString();
					uuids.sort();
					return uuids;
				};
				const QStringList lazy_tables = tableUuids(ElementProvider(&listed).table(nullptr, nullptr));
				check(!lazy_tables.isEmpty(), "the project has tables");
				check(lazy_tables == tableUuids(ElementProvider(&eager).table(nullptr, nullptr)),
					  "the project's tables listed as when built");
				check(listed.dataBase()->excludedConductorCount() == eager.dataBase()->excludedConductorCount(),
					  "wires left out of the wiring list counted as when built");
				int with_tables = 0;
				for (Diagram *folio : folios) {
					for (QGraphicsItem *item : folio->items())
						if (item->type() == QetGraphicsTableItem::Type) { ++with_tables; break; }
				}
				std::printf(".qetz tables listed: %d folios built, %d hold a table\n",
							unbuilt - listed.unloadedFolioCount(), with_tables);
				check(listed.unloadedFolioCount() > 0, "listing the tables does not build every folio");
			}

				//A script (and the assistant's live mode) asking about the
				//folios and one folio's items: that folio built, no other (D5)
			{
				QETProject scripted(zipped);
				const int unbuilt = scripted.unloadedFolioCount();
				QetScriptApi api(&scripted), usual_api(&eager);
				check(api.folioCount() == usual_api.folioCount(), "a script counts the folios as when built");
				for (int i = 0 ; i < folios.size() ; ++i) {
					check(api.folioTitle(i) == usual_api.folioTitle(i) && api.folioUuid(i) == usual_api.folioUuid(i)
						  && api.folioBorder(i, "columns") == usual_api.folioBorder(i, "columns"),
						  "a script reads a folio's own data as when built");
				}
				check(scripted.unloadedFolioCount() == unbuilt, "reading the folios' own data builds none");
					//The folio with the most links to other folios
				const int target = busiest;
				check(!scripted.folios().at(target)->isLoaded(), "a folio not built");
				QStringList lazy_uuids = api.elementUuids(target), usual_uuids = usual_api.elementUuids(target);
				lazy_uuids.sort(); usual_uuids.sort();
				check(lazy_uuids == usual_uuids, "a script lists a folio's symbols as when built");
				QStringList lazy_wires = api.conductors(target), usual_wires = usual_api.conductors(target);
				lazy_wires.sort(); usual_wires.sort();
				check(lazy_wires == usual_wires, "a script lists a folio's wires as when built");
				check(scripted.folios().at(target)->isLoaded(), "the folio a script reads is built");
				std::printf(".qetz script on folio %d: %d folios built\n", target + 1,
							unbuilt - scripted.unloadedFolioCount());
				check(unbuilt - scripted.unloadedFolioCount() == 1, "a script reading one folio builds that folio only");
			}

				//A script changes the information of a symbol on a folio not
				//built, which nothing else shows: through the project
				//database, no folio built; saved as when built; undone (D5)
			{
				QETProject changed(zipped);
				const int unbuilt = changed.unloadedFolioCount();
				int folio_index = -1;
				QString uuid;
				for (int i = 0 ; i < folios.size() && uuid.isEmpty() ; ++i) {
					if (changed.folios().at(i)->isLoaded()) continue;
					for (Element *element : folios.at(i)->elements())
						if (changed.dataBase()->canEditUnbuilt(element->uuid())) {
							folio_index = i; uuid = element->uuid().toString(); break;
						}
				}
				check(!uuid.isEmpty(), "a symbol to change through the database");
				QetScriptApi api(&changed);
				const QString before = api.elementInfo(folio_index, uuid, "comment");
				const QString value = QStringLiteral("changed through the database");
				check(api.setElementInfo(folio_index, uuid, "comment", value), "a script changes a symbol not built");
				check(api.elementInfo(folio_index, uuid, "comment") == value, "the change read back");
				check(changed.unloadedFolioCount() == unbuilt, "changing a symbol not built builds no folio");
				const QString changed_path = output + "/changed.qetz";
				check(changed.writeContainerFile(changed_path, &error), "saved after the change");
				std::printf(".qetz symbol on folio %d changed by a script: %d folios built, %d by saving\n",
							folio_index + 1, 0, unbuilt - changed.unloadedFolioCount());
				qputenv("QET_EAGER_FOLIOS", "1");
				const QString usual_path = output + "/changed-usual.qetz";
				{
					QETProject usual_changed(zipped);
					QetScriptApi usual_api(&usual_changed);
					check(usual_api.setElementInfo(folio_index, uuid, "comment", value), "the same change when built");
					check(usual_changed.writeContainerFile(usual_path, &error), "saved after the change when built");
				}
				QETProject changed_back(changed_path), usual_back(usual_path);
				qunsetenv("QET_EAGER_FOLIOS");
				check(sortedLines(changed_back.toXml().toString(), path) == sortedLines(usual_back.toXml().toString(), path),
					  "a symbol changed through the database saved as when built");
				check(QetScriptApi(&changed_back).elementInfo(folio_index, uuid, "comment") == value, "the change saved");
					//Undone, then redone and the folio built: the symbol shows it
				changed.undoStack()->undo();
				check(api.elementInfo(folio_index, uuid, "comment") == before, "the change undone");
				changed.undoStack()->redo();
				changed.buildFolioToShow(changed.folios().at(folio_index));
				Element *built_symbol = nullptr;
				for (Element *element : changed.folios().at(folio_index)->elements())
					if (element->uuid().toString() == uuid) built_symbol = element;
				check(built_symbol && built_symbol->elementInformations().value("comment").toString() == value,
					  "a symbol changed before its folio was built shows the change");
			}

				//A new revision on every folio: saved from the folios as read,
				//not built; a label change builds its folio (D5)
			revisionEveryFolio(zipped, output, "revised", path);
			revisionEveryFolio(zipped, output, "relabelled", path, busiest);

				//A script changes a wire's cable by its uuid, on folios not
				//built, the wire and its potential numbered by hand: through
				//the project database, no folio built; saved as when built;
				//undone; the folio built afterwards shows it (D5)
			{
				QETProject cabled(zipped);
				const int unbuilt = cabled.unloadedFolioCount();
				int folio_index = -1;
				QString uuid;
				QSet<QUuid> potential;
				for (int i = 0 ; i < folios.size() && uuid.isEmpty() ; ++i) {
					if (cabled.folios().at(i)->isLoaded()) continue;
					for (Conductor *wire : folios.at(i)->conductors()) {
						QSet<Conductor *> wires = wire->relatedPotentialConductors(true);
						wires << wire;
						bool usable = true;
						for (Conductor *c : std::as_const(wires))
							if (!c->properties().m_formula.isEmpty()
									|| cabled.folios().at(eager.folioIndex(c->diagram()))->isLoaded())
								usable = false;
						if (!usable || wires.size() < 2) continue;
						folio_index = i;
						uuid = wire->uuid().toString();
						for (Conductor *c : std::as_const(wires)) potential.insert(c->uuid());
						break;
					}
				}
				check(!uuid.isEmpty(), "a wire numbered by hand, its potential on folios not built");
				QetScriptApi api(&cabled);
				const QString before = api.conductorPropertyByUuid(folio_index, uuid, "cable");
				const QString value = QStringLiteral("W-42");
				check(api.setConductorPropertyByUuid(folio_index, uuid, "cable", value), "a script changes a wire not built");
				check(api.conductorPropertyByUuid(folio_index, uuid, "cable") == value, "the wire's change read back");
				for (const QUuid &wire : std::as_const(potential))
					check(cabled.dataBase()->conductorProperties(wire).m_cable == value, "every wire of the potential changed");
				check(cabled.unloadedFolioCount() == unbuilt, "changing wires not built builds no folio");
				const QString cabled_path = output + "/cabled.qetz";
				check(cabled.writeContainerFile(cabled_path, &error), "saved after the wire change");
				std::printf(".qetz wire on folio %d, potential of %d wires, cable changed by uuid: 0 folios built, %d by saving\n",
							folio_index + 1, int(potential.size()), unbuilt - cabled.unloadedFolioCount());
				qputenv("QET_EAGER_FOLIOS", "1");
				const QString usual_path = output + "/cabled-usual.qetz";
				{
					QETProject usual_cabled(zipped);
					check(QetScriptApi(&usual_cabled).setConductorPropertyByUuid(folio_index, uuid, "cable", value),
						  "the same wire change when built");
					check(usual_cabled.writeContainerFile(usual_path, &error), "saved after the wire change when built");
				}
				QETProject cabled_back(cabled_path), usual_back(usual_path);
				qunsetenv("QET_EAGER_FOLIOS");
				auto lines = [&path](QETProject &project) {
					QStringList list = sortedLines(project.toXml().toString(), path);
					list.removeIf([](const QString &line) { return line.contains(QLatin1String("name=\"saved")); });
					return list;
				};
				check(lines(cabled_back) == lines(usual_back), "a wire changed through the database saved as when built");
				cabled.undoStack()->undo();
				check(api.conductorPropertyByUuid(folio_index, uuid, "cable") == before, "the wire change undone");
				cabled.undoStack()->redo();
				Conductor *built_wire = nullptr;
				for (Conductor *wire : cabled.folios().at(folio_index)->conductors())
					if (wire->uuid().toString() == uuid) built_wire = wire;
				check(!built_wire, "the folio not built yet");
				cabled.buildFolioToShow(cabled.folios().at(folio_index));
				for (Conductor *wire : cabled.folios().at(folio_index)->conductors())
					if (wire->uuid().toString() == uuid) built_wire = wire;
				check(built_wire && built_wire->properties().m_cable == value,
					  "a wire changed before its folio was built shows the change");
			}

				//The wire numbers and wiring list exports, and the terminal
				//numbering dialog: as when built, building only the folios
				//holding a terminal (D5)
			{
				QETProject exported(zipped);
				const int unbuilt = exported.unloadedFolioCount();
				check(ConductorNumExport(&exported).wiresNum() == ConductorNumExport(&eager).wiresNum(),
					  "wire numbers exported as when built");
				const QString wiring = WiringListExport(&exported).toCsvString(), usual_wiring = WiringListExport(&eager).toCsvString();
				if (wiring != usual_wiring) {
					const QStringList a = wiring.split('\n'), b = usual_wiring.split('\n');
					for (const QString &line : b) if (!a.contains(line)) std::printf("  wiring when built: %s\n", qPrintable(line));
					for (const QString &line : a) if (!b.contains(line)) std::printf("  wiring lazily:     %s\n", qPrintable(line));
				}
				check(wiring == usual_wiring, "wiring list exported as when built");
				check(exported.unloadedFolioCount() == unbuilt, "exporting wire numbers and the wiring list builds no folio");
				{ TerminalNumberingDialog dialog(nullptr, &exported); }
				int with_terminals = 0;
				for (Diagram *folio : folios)
					for (Element *element : folio->elements())
						if (element->elementData().m_type == ElementData::Terminal) { ++with_terminals; break; }
				std::printf(".qetz terminal numbering: %d folios built, %d hold a terminal\n",
							unbuilt - exported.unloadedFolioCount(), with_terminals);
				check(exported.folios().size() - exported.unloadedFolioCount() <= with_terminals + (folios.size() - unbuilt),
					  "terminal numbering builds only the folios holding a terminal");
			}

				//Saved as a .qetz without building the folios not touched
				//(D4): their parts and rows copied as they were
			QETProject untouched(zipped);
			const int unbuilt = untouched.unloadedFolioCount();
			const QString kept_path = output + "/kept.qetz";
			check(untouched.writeContainerFile(kept_path, &error), "saved as a .qetz without building");
			std::printf(".qetz saved untouched: %d of %d folios not built before, %d after\n",
						unbuilt, int(folios.size()), untouched.unloadedFolioCount());
			check(untouched.unloadedFolioCount() > 0, "saving an untouched .qetz does not build every folio");
			qputenv("QET_EAGER_FOLIOS", "1");
			QETProject kept_back(kept_path);
			qunsetenv("QET_EAGER_FOLIOS");
			check(kept_back.toXml().toString() == eager.toXml().toString(),
				  "a .qetz saved without building holds the project");

				//After everyday edits on a folio
			QETProject edited_z(zipped);
			edit(edited_z, busiest);
			const QString edited_path = output + "/edited.qetz";
			check(edited_z.writeContainerFile(edited_path, &error), "edited .qetz saved");
			std::printf(".qetz saved after the edits: %d folios not built\n", edited_z.unloadedFolioCount());
			check(edited_z.unloadedFolioCount() > 0, "saving an edited .qetz does not build every folio");
				//Against the same edits made with every folio built, saved and
				//read back too: what a save and a reading do to a project is
				//not what is checked here
			const QString usual_path = output + "/edited-usual.qetz";
			qputenv("QET_EAGER_FOLIOS", "1");
			{
				QETProject edited_usual(path);
				edit(edited_usual, busiest);
				check(edited_usual.writeContainerFile(usual_path, &error), "edits saved with every folio built");
			}
			QETProject edited_back(edited_path), usual_back(usual_path);
			qunsetenv("QET_EAGER_FOLIOS");
			check(sortedLines(edited_back.toXml().toString(), path) == sortedLines(usual_back.toXml().toString(), path),
				  "an edited .qetz saved without building holds what the edits made");
				//Each symbol's links as saved, partners on folios not built
				//included, in their order (D5a)
			check(savedLinks(edited_path, path) == savedLinks(usual_path, path),
				  "an edited .qetz saved without building keeps every symbol's links");
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
