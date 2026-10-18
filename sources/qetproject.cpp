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

#include "qetproject.h"
#include "container/qetcontainer.h"
#include "container/qetcontainerdb.h"

#include "ElementsCollection/xmlelementcollection.h"
#include "autoNum/assignvariables.h"
#include "autoNum/numerotationcontext.h"
#include "autoNum/numerotationcontextcommands.h"
#include "autoNum/renumberelementscommand.h"
#include "cable/cable.h"
#include "cable/cablemanager.h"
#include "cable/cablepart.h"
#include "autoNum/elementautonumschemecommand.h"
#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "qetapp.h"
#include "qetmessagebox.h"
#include "qetresult.h"
#include "titleblock/integrationmovetemplateshandler.h"
#include "titleblock/movetemplateshandler.h"
#include "titleblocktemplate.h"
#include "ui/dialogwaiting.h"
#include "ui/importelementdialog.h"
#include "TerminalStrip/terminalstrip.h"
#include "qetxml.h"
#include "qetinformation.h"
#include "qetversion.h"
#include "undocommand/adddiagramcommand.h"
#include "utils/previousversions.h"

#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QHash>
#include <QTimer>
#include <QtConcurrentRun>

#include <QtDebug>
#include <algorithm>
#include <utility>

static int BACKUP_INTERVAL = 1200000; //interval in ms of backup = 20min

bool QETProject::m_backup_enabled = true;

void QETProject::setBackupEnabled(bool enabled)
{
	m_backup_enabled = enabled;
}

/**
	@brief QETProject::QETProject
	Create a empty project
	@param parent
*/
QETProject::QETProject(QObject *parent) :
QObject              (parent),
m_titleblocks_collection(this),
m_data_base(this, this),
m_project_properties_handler{this}
{
	setDefaultTitleBlockProperties(TitleBlockProperties::defaultProperties());

	m_elements_collection = new XmlElementCollection(this);
	init();

		//A new project keeps the texts of its turned symbols horizontal; a
		//project saved without the setting is read with it off, so it looks
		//as it always did (readSymbolTextsXml())
	m_upright_symbol_texts = true;

	QSettings settings;

		//Read auto break conductor default from global settings
	m_auto_break_conductor = settings.value(QStringLiteral("diagrameditor/auto_break_conductor"), false).toBool();

	int size = settings.beginReadArray(QStringLiteral("diagrameditor/defaultguides"));
	for (int i = 0; i < size; ++i) {
		settings.setArrayIndex(i);
		GuideProperties g;
		g.orientation = settings.value(QStringLiteral("orientation"), 0).toInt();
		g.position = settings.value(QStringLiteral("position"), 0.0).toReal();
		g.color = QColor(settings.value(QStringLiteral("color"), QStringLiteral("#ff0000")).toString());
		m_default_guides.append(g);
	}
	settings.endArray();

		//Load global auto-numbering defaults from QSettings
	{
		auto conductorData = NumerotationContext::loadFromSettings(settings, QStringLiteral("autonum/conductor"));
		for (auto it = conductorData.first.constBegin(); it != conductorData.first.constEnd(); ++it) {
			addConductorAutoNum(it.key(), it.value());
		}
		if (!conductorData.second.isEmpty()) {
			setCurrentConductorAutoNum(conductorData.second);
		}

		auto elementData = NumerotationContext::loadFromSettings(settings, QStringLiteral("autonum/element"));
		for (auto it = elementData.first.constBegin(); it != elementData.first.constEnd(); ++it) {
			addElementAutoNum(it.key(), it.value());
		}
		if (!elementData.second.isEmpty()) {
			setCurrrentElementAutonum(elementData.second);
		}

		auto folioData = NumerotationContext::loadFromSettings(settings, QStringLiteral("autonum/folio"));
		for (auto it = folioData.first.constBegin(); it != folioData.first.constEnd(); ++it) {
			addFolioAutoNum(it.key(), it.value());
		}

			//The cable numbering rule a project starts with, so a project
			//made from these defaults numbers its cables the way the
			//global settings say without having to be opened first.
		auto cableData = NumerotationContext::loadFromSettings(settings, QStringLiteral("autonum/cable"));
		for (auto it = cableData.first.constBegin(); it != cableData.first.constEnd(); ++it) {
			addCableAutoNum(it.key(), it.value());
		}
		if (!cableData.second.isEmpty()) {
			setCurrentCableAutoNum(cableData.second);
		}
	}
}

ProjectPropertiesHandler &QETProject::projectPropertiesHandler()
{
	return m_project_properties_handler;
}

/**
	@brief QETProject::QETProject
	Construct a project from a .qet file
	@param path : path of the file
	@param parent : parent QObject
*/
QETProject::QETProject(const QString &path, QObject *parent) :
	QObject              (parent),
	m_titleblocks_collection(this),
	m_data_base(this, this),
	m_project_properties_handler{this}
{
	QFile file(path);
	m_state = openFile(&file);
	if (m_state != ProjectState::Ok) {
		return;
	}

		//The file just read already holds everything a crash could lose, so
		//there is nothing to back up until the project is changed.
	m_backup_needed = false;
	init();
}

/**
	@brief QETProject::QETProject
	@param backup : backup file to open, QETProject take ownership of backup.
	@param parent : parent QObject
*/
QETProject::QETProject(KAutoSaveFile *backup, QObject *parent) :
	QObject              (parent),
	m_titleblocks_collection(this),
	m_data_base(this, this),
	m_project_properties_handler{this}
{
	m_state = openFile(backup);
		//Failed to open from the backup, try to open the crashed
	if (m_state != ProjectState::Ok)
	{
		QFile file(backup->managedFile().path());
		m_state = openFile(&file);
		if(m_state != ProjectState::Ok)
		{
			backup->open(QIODevice::ReadWrite);
			delete backup;
			return;
		}
	}
		//Set the real path, instead of the path of the backup.
	setFilePath(backup->managedFile().path());
		//A project read from a .qet is saved as a .qetz beside it
	m_opened_from_qet = !isContainerPath(m_file_path);
	delete  backup;

		//Set the project to read only mode if the file it.
	QFileInfo fi(m_file_path);
	setReadOnly(!fi.isWritable());

	init();
}

/**
	@brief QETProject::~QETProject
	Destructor
*/
QETProject::~QETProject()
{
		//Wait for any in-flight async crash-recovery backup to finish: the worker
		//writes through m_backup_files, a member that would otherwise be destroyed
		//under it (issue #492).
	m_backup_future.waitForFinished();

		//We block database signal to avoid hundreds of unnecessary emitted signal
		//due to deletion (diagram, item, etc...) and as much update made in the not yet deleted things.
	m_data_base.blockSignals(true);
		//Same reasoning for the rebuild itself : destroying a table relinks
		//the tables that were chained to it, which re-queries the database,
		//which rebuilds it completely -- for a project that is on its way out.
		//Nothing can observe the result : the database is destroyed with it.
	m_data_base.setUpdateBlocked(true);

		//Each time a diagram is deleted we also remove it from m_diagram_list
		//because a lot of thing append during the destructor of a diagram class
		//and one of these thing (not directly in the destructor of the diagram
		//but in another destructor called by the diagram destructor)
		//is to get the diagram list of the project to make some updates @see QList<Diagram *> QETProject::diagrams() const.
		//So we need to remove the freshly deleted diagram from the list
		//in each iteration of the loop to avoid  the use of a dangling pointer.
	const auto diag_list = m_diagrams_list;
	for (const auto &diagram : diag_list)
	{
		delete  diagram;
		m_diagrams_list.removeOne(diagram);
	}

		//A diagram can be detached from this project (detachDiagram(), used by
		//both removeDiagram() and RemoveDiagramCommand::redo()) and scheduled
		//for deferred deletion via deleteLater(), without that deletion having
		//actually run yet -- deleteLater() only fires on the next event-loop
		//iteration, and nothing guarantees one runs before this destructor
		//does. Such a diagram is no longer in m_diagrams_list (so the loop
		//above never touches it) but is still a QObject child of this project
		//(Diagram's constructor passes `project` straight to QGraphicsScene's
		//parent argument). Left alone, it is destroyed later by QObject's own
		//automatic child cleanup in ~QObject(), which runs AFTER m_data_base
		//(a plain value member, destroyed by ordinary C++ member teardown)
		//has already been destroyed -- and Diagram's destructor calls back
		//into dataBase()->removeElement() for each of its elements, so that
		//ordering is a use-after-free (confirmed by crash: SIGSEGV in
		//QSqlResult::exec(), called from Diagram::~Diagram() by way of
		//Diagram::removeItem(), by way of QObjectPrivate::deleteChildren()).
		//Delete any such stragglers now, synchronously, while m_data_base is
		//still alive. The deleteLater() event, if it is ever processed
		//afterward, is a safe no-op on an already-deleted QObject.
	const auto orphaned_diagrams = findChildren<Diagram *>(QString(), Qt::FindDirectChildrenOnly);
	for (Diagram *diagram : orphaned_diagrams) {
		delete diagram;
	}
}

/**
	@brief QETProject::dataBase
	@return The data base of this project
*/
projectDataBase *QETProject::dataBase()
{
	return &m_data_base;
}

/**
	@brief QETProject::uuid
	@return the uuid of this project
*/
QUuid QETProject::uuid() const
{
	return m_uuid;
}

/**
	@brief QETProject::containerPicture
	The folio pictures of a .qetz are kept as bytes beside the document,
	not as base64 text in it (QetContainer::join()), for as long as the
	project is open: a folio built after opening still finds its pictures.
	They cost almost no memory, sharing their bytes with the pictures
	built from them.
	@return the picture in @p file, or an empty array when there is none
*/
QByteArray QETProject::containerPicture(const QString &file) const
{
	return m_container_pictures.value(file);
}

/**
	@brief QETProject::renewedSharedUuid
	While the project opens: the uuid of the @p ordinal -th symbol carrying
	@p uuid on the folio at @p folio_index (in the file's order), when
	other symbols of the project carry the same uuid and this copy is not
	the one keeping it (readDiagramsXml()). Derived from the uuid and the
	folio's uuid, so the same file gives the same uuid on every open.
	@return a null uuid when this symbol keeps its uuid
*/
QUuid QETProject::renewedSharedUuid(int folio_index, int ordinal, const QUuid &uuid,
									const QUuid &folio_uuid)
{
	const auto keeper = m_shared_uuid_keepers.constFind(uuid);
	if (keeper == m_shared_uuid_keepers.constEnd()
			|| (keeper->first == folio_index && keeper->second == ordinal)) {
		return QUuid();
	}
	return derivedItemUuid(QStringLiteral("element-copy"),
						   uuid.toString() + QLatin1Char('\n') + folio_uuid.toString());
}

/**
	@brief QETProject::derivedItemUuid
	A uuid for an item of this project that was saved without one, the same
	on every load of the same file.
	@p key describes the item by what it is, never by its place in the file
	or its folio's index: inserting or moving a folio must not change it.
	Items with the same @p kind and @p key anywhere in the project (a copied
	folio, two identical symbols stacked on one spot) are told apart by a
	counter, in load order among those items alone.

	A derived uuid is never one the file already carries: an item saved with
	a derived uuid and then moved or re-connected keeps it, so a newcomer
	later taking its old place or ends would otherwise derive the same one.
	The file's saved uuids are collected before any folio loads
	(readDiagramsXml()), so the result still depends on the file alone.

	uuids are unique within one project; copies of a project share them, as
	they share every saved uuid. Anything bringing items in from another
	project must renew them, as paste does.
	@return a UUID v5, which cannot collide with the v4 uuids given to new
	items
*/
QUuid QETProject::derivedItemUuid(const QString &kind, const QString &key)
{
	static const QUuid derived_ns(QStringLiteral("{7d1e9c3a-5b2f-4e8a-9c61-2f4b8d0e6a17}"));
	const QString full = kind + QLatin1Char('\n') + key;
	int &n = m_derived_uuid_keys[full];
	QUuid uuid;
	do {
		uuid = QUuid::createUuidV5(derived_ns,
								   n ? full + QLatin1Char('\n') + QString::number(n) : full);
		++n;
	} while (m_saved_item_uuids.contains(uuid));
	return uuid;
}

/**
	@brief QETProject::init
*/
void QETProject::init()
{
	connect(&m_titleblocks_collection, &TitleBlockTemplatesCollection::changed, this, &QETProject::updateDiagramsTitleBlockTemplate);
	connect(&m_titleblocks_collection, &TitleBlockTemplatesCollection::aboutToRemove, this, &QETProject::removeDiagramsTitleBlockTemplate);

		//What changes the saved items of folios not built (lazy folios):
		//a folio's position, number or the folio total in their texts,
		//the report and cross-reference settings, a symbol's definition.
		//A folio not built is then built to be saved (writesKeptFolios())
	connect(this, &QETProject::diagramAdded, this, &QETProject::markKeptFoliosStale);
	connect(this, &QETProject::diagramRemoved, this, &QETProject::markKeptFoliosStale);
	connect(this, &QETProject::projectDiagramsOrderChanged, this, &QETProject::markKeptFoliosStale);
	connect(this, &QETProject::reportPropertiesChanged, this, &QETProject::markKeptFoliosStale);
	connect(this, &QETProject::XRefPropertiesChanged, this, &QETProject::markKeptFoliosStale);

	m_undo_stack = new QUndoStack(this);
		//Folios not built are saved as read until something is edited
		//(m_edited_since_open, toXml())
	connect(m_undo_stack, &QUndoStack::indexChanged, this, [this]() {m_edited_since_open = true;});
	connect(m_undo_stack, &QUndoStack::cleanChanged, this, &QETProject::undoStackChanged);

		//What counts as a change for writeBackup(): the undo stack moving,
		//setModified(true), and the embedded collections, which can change
		//without going through either.
	const auto backup_needed = [this]() { m_backup_needed = true; };
	connect(m_undo_stack, &QUndoStack::indexChanged, this, backup_needed);
	connect(&m_titleblocks_collection, &TitleBlockTemplatesCollection::changed, this, backup_needed);
	connect(&m_titleblocks_collection, &TitleBlockTemplatesCollection::aboutToRemove, this, backup_needed);
	connect(m_elements_collection, &XmlElementCollection::elementAdded, this, backup_needed);
	connect(m_elements_collection, &XmlElementCollection::elementChanged, this, backup_needed);
	connect(m_elements_collection, &XmlElementCollection::elementChanged, this, &QETProject::markKeptFoliosStale);
	connect(m_elements_collection, &XmlElementCollection::elementRemoved, this, backup_needed);
	connect(m_elements_collection, &XmlElementCollection::directorieAdded, this, backup_needed);
	connect(m_elements_collection, &XmlElementCollection::directoryRemoved, this, backup_needed);

	m_save_backup_timer.setInterval(BACKUP_INTERVAL);
	connect(&m_save_backup_timer, &QTimer::timeout, this, &QETProject::writeBackup);
	m_save_backup_timer.start();
	writeBackup();

	QSettings settings;
	int autosave_interval = settings.value(QStringLiteral("diagrameditor/autosave-interval"), 0).toInt();
	if(autosave_interval > 0)
	{
		int ms = autosave_interval*60*1000;
		m_autosave_timer.setInterval(ms);
		connect(&m_autosave_timer, &QTimer::timeout, this, [this]()
		{
				//Not while folios are being built (the event loop can run
				//then, see setFolioBuildObserver()) : the next time
			if (this->m_state == ProjectParsingRunning) {
				return;
			}
			if(!this->m_file_path.isEmpty())
			{
					//No one is there to ask: a file changed behind this
					//QET is left alone
				const QETResult result = this->write();
				if (!result.isOk())
					qWarning() << "autosave:" << result.errorMessage();
			}
		});
		m_autosave_timer.start();
	}

}

/**
	@brief QETProject::derivedUuid
	Name-based (version 5) uuid for a project file that has no uuid yet,
	because it was written before the uuid was persisted.

	A random uuid would do as an identity, but it would make saving an
	unmodified legacy project non-reproducible : every load would invent a
	different one and write it out (see #754). The uuid is therefore derived
	from the content of the file, so the same file always yields the same
	uuid, while two different projects practically never share one.
	Carriage returns are dropped first, so that a checkout with CRLF line
	endings (Windows, git autocrlf) gives the same uuid as one with LF.

	It is computed once, when the file is loaded, and saved from then on :
	editing, renaming or moving the project later does not change it.
	@param content : the raw content of the project file
	@return the derived uuid
*/
QUuid QETProject::derivedUuid(const QByteArray &content)
{
		//Fixed namespace for QElectroTech project uuids, never change it :
		//doing so would change the uuid given to every legacy project.
	static const QUuid project_namespace(
				QStringLiteral("{c8c75719-0fea-4b1c-9f4b-2dd179fb2f0c}"));

		//Same result as QUuid::createUuidV5(project_namespace,
		//"qet-project-legacy\n" + content without '\r'), hashed in place :
		//copying the content twice took 2 s of opening a 263 MB project.
	QCryptographicHash hash(QCryptographicHash::Sha1);
	hash.addData(project_namespace.toRfc4122());
	hash.addData(QByteArrayView("qet-project-legacy\n"));
	const QByteArrayView view(content);
	qsizetype start = 0;
	while (start < view.size()) {
		qsizetype cr = view.indexOf('\r', start);
		if (cr < 0)
			cr = view.size();
		hash.addData(view.sliced(start, cr - start));
		start = cr + 1;
	}
	QByteArray bytes = hash.result().left(16);
	bytes[6] = char((bytes[6] & 0x0F) | 0x50); // version 5
	bytes[8] = char((bytes[8] & 0x3F) | 0x80); // RFC 4122 variant
	return QUuid::fromRfc4122(bytes);
}

/**
	@brief QETProject::openFile
	@param file
	@return
*/
QETProject::ProjectState QETProject::openFile(QFile *file)
{
	QElapsedTimer load_timer;
	load_timer.start();

	bool opened_here = file->isOpen() ? false : true;
	if (opened_here)
	{
			//A .qetz is a zip and is read as bytes: text mode would turn
			//its CR LF pairs into LF on Windows. A .qet is read as text,
			//as it always was.
		if (!file->open(QIODevice::ReadOnly)) {
			return FileOpenFailed;
		}
		if (!QetContainer::isZip(file->peek(4))) {
			file->close();
			if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
				return FileOpenFailed;
			}
		}
	}
	QFileInfo fi(*file);
	setFilePath(fi.absoluteFilePath());
		//Saved again unchanged, the file keeps its saved date and path
	m_stamped_path = m_file_path;

		//Extract the content of the xml. The raw bytes are kept : a project
		//file without a persisted uuid derives its uuid from them.
	const QByteArray content = file->readAll();
	rememberDiskState();
		//Each folio's text is kept while it is not built (readDiagramsXml()):
		//from a .qet only, a .qetz is a zip of parts
	if (!content.startsWith("PK\x03\x04"))
		m_opened_content = content;
		//A project read from a .qet is saved as a .qetz beside it
	m_opened_from_qet = !content.startsWith("PK\x03\x04");
	QDomDocument xml_project;
	// PreserveSpacingOnlyNodes: without it, a text node that is entirely
	// whitespace -- e.g. a title-block custom variable deliberately set to
	// a single space, the only way to give it a value other than blank
	// (bugtracker #973) -- is silently dropped by Qt's default parsing,
	// and QDomElement::text() then returns "" for it exactly as if it had
	// never been set. Confirmed in isolation: <a> </a> parses to text()=="",
	// this option makes it text()==" ". Every place in this codebase that
	// walks a QDomNode's children already filters on isElement() (see
	// QET::findInDomElement()), so the extra whitespace-only text nodes
	// this keeps around are inert everywhere but the two elements that
	// call .text() on themselves -- which is exactly where the bug was.
	// The option exists since Qt 6.5; older Qt always drops such nodes,
	// so there an all-whitespace value still reloads as "".
	// QetContainer::parse() does exactly that, for a .qet and for each part
	// of a .qetz alike.
	QString read_error;
		//A .qetz: its entries are kept, for a save to copy the parts of the
		//folios not built (stage D4)
	m_container_files.clear();
	m_folio_parts.clear();
	bool read = false;
	int container_format = 0;
	if (QetContainer::isZip(content)) {
		QList<QetZip::Entry> entries;
		read = QetZip::fromBytes(content, &entries, &read_error)
				&& QetContainer::join(entries, &xml_project, &read_error, &m_folio_parts,
									  &m_container_pictures, &container_format);
		if (read)
			for (const QetZip::Entry &entry : std::as_const(entries))
				m_container_files.insert(entry.name, entry.data);
	} else {
		read = QetContainer::readDocument(content, &xml_project, &read_error);
	}
	if (!read)
	{
		qWarning().noquote() << "Unable to read" << fi.fileName() << ":" << read_error;
		m_open_error = read_error;
		m_container_pictures.clear();
		m_folio_parts.clear();
		if(opened_here) {
			file->close();
		}
		return XmlParsingFailed;
	}
	const qint64 xml_parse_ms = load_timer.elapsed();

		//Restore the persisted project uuid before anything else is built
		//from the file. The project database already got its connection name
		//from the uuid created at construction, it does not depend on this.
	const QDomElement root_elmt = xml_project.documentElement();
	if (root_elmt.tagName() == QLatin1String("project"))
	{
		const QUuid persisted_uuid(root_elmt.attribute(QStringLiteral("uuid")));
		m_uuid = persisted_uuid.isNull() ? derivedUuid(content)
										 : persisted_uuid;
	}

		//Build the project from the xml
	readProjectXml(xml_project);
	m_opened_content.clear();

	const qint64 total_ms = load_timer.elapsed();
	qInfo().nospace().noquote()
			<< "Project \"" << fi.fileName() << "\" ("
			<< fi.size() / 1024 << " KiB) opened in "
			<< total_ms / 1000.0 << " seconds (xml parsing "
			<< xml_parse_ms / 1000.0 << ", content "
			<< (total_ms - xml_parse_ms) / 1000.0 << ")";

		//Written by a later QElectroTech that lets this one read it: saving
		//here would drop what this version does not know
	m_from_later_version = container_format > QetContainer::Format;
	if (!fi.isWritable() || m_from_later_version) {
		setReadOnly(true);
	}
	if (m_from_later_version) {
		qInfo().noquote() << QStringLiteral("Project %1 is of .qetz format %2 (this version writes %3): opened read-only")
							 .arg(fi.fileName()).arg(container_format).arg(QetContainer::Format);
	}
	if(opened_here) {
		file->close();
	}
	return m_state;
}

/**
 * @brief QETProject::refresh
 * Refresh everything in the project.
 * This is notably use when open a project from file.
 */
void QETProject::refresh()
{
	DialogWaiting *dlgWaiting { nullptr };
	if(DialogWaiting::hasInstance())
	{
	dlgWaiting = DialogWaiting::instance();
		dlgWaiting->setModal(true);
		dlgWaiting->show();
	}

		//A folio not built yet is refreshed when it is (folioLoaded())
	for(const auto &diagram : folios())
	{
		if (!diagram->isLoaded()) {
			continue;
		}
		if(dlgWaiting)
		{
			dlgWaiting->setProgressBar(dlgWaiting->progressBarValue()+1);
			dlgWaiting->setDetail(diagram->title());
		}
		diagram->refreshContents();
	}
}

/**
	Cette methode peut etre utilisee pour tester la bonne ouverture d'un projet
	@return l'etat du projet
	@see ProjectState
*/
QETProject::ProjectState QETProject::state() const
{
	return(m_state);
}

/**
	@brief QETProject::openError
	@return why the file could not be read, when state() is
	XmlParsingFailed: "picture ... is missing", say; empty otherwise
*/
QString QETProject::openError() const
{
	return m_open_error;
}

/**
	@return la liste des schemas de ce projet
*/
QList<Diagram *> QETProject::diagrams() const
{
		//Every caller of diagrams() expects every folio built: those that
		//do not are moved to folios() one by one
	loadFolios();
	return(m_diagrams_list);
}

/**
	@brief QETProject::folios
	@return the folios of this project, whether their items are built yet
	or not (a project opened with lazy folios); see Diagram::isLoaded()
*/
QList<Diagram *> QETProject::folios() const
{
	return(m_diagrams_list);
}

/**
	@brief QETProject::cableLabelFolios
	@return the folios CableManager::refreshLabels() works on, built: every
	folio when the project has cables (a core is followed through folio
	reports to any folio); without cables, the folios built and those not
	built yet holding a wire that still refers to a cable -- the only
	wires the pass changes then, taking the reference away, read from the
	folios' XML.
*/
QList<Diagram *> QETProject::cableLabelFolios()
{
	if (!cables().isEmpty()) {
		return diagrams();
	}
	QList<Diagram *> referring;
	for (Diagram *d : std::as_const(m_diagrams_list)) {
		if (d->isLoaded()) continue;
		for (const QDomElement &wire : QET::findInDomElement(d->deferredXml(), QStringLiteral("conductors"),
															 QStringLiteral("conductor")))
			if (!QUuid(wire.attribute(QStringLiteral("cable_uuid"))).isNull()) {
				referring << d;
				break;
			}
	}
	buildFolios(referring);
	return builtFolios();
}

/**
	@brief QETProject::builtFolios
	@return the folios of this project whose items are built: all of them,
	but for a project opened with lazy folios
*/
QList<Diagram *> QETProject::builtFolios() const
{
	QList<Diagram *> built;
	for (Diagram *d : m_diagrams_list)
		if (d->isLoaded()) built << d;
	return built;
}

/**
	@brief QETProject::unloadedFolioCount
	@return how many folios of this project wait to be built
*/
int QETProject::unloadedFolioCount() const
{
	int n = 0;
	for (const Diagram *d : m_diagrams_list)
		if (!d->isLoaded()) ++n;
	return n;
}

/**
	@brief QETProject::foliosWithWiresNotReconnected
	@return the folios that left out a wire of the file, an end of it not
	being found (Diagram::wiresNotReconnected()). A folio not built yet is
	built only when the database, filled from the document, says it has
	such a wire: the others are not built to find out.
*/
QList<Diagram *> QETProject::foliosWithWiresNotReconnected()
{
	const QSet<QUuid> unjoined = m_data_base.foliosWithUnjoinedWires();
	QList<Diagram *> found;
	for (Diagram *diagram : std::as_const(m_diagrams_list))
	{
		if (!diagram->isLoaded()) {
			if (!unjoined.contains(diagram->uuid())) {
				continue;
			}
			diagram->ensureLoaded();
		}
		if (!diagram->wiresNotReconnected().isEmpty()) {
			found << diagram;
		}
	}
	return found;
}

/**
	@brief QETProject::loadFolios
	Build every folio of this project still waiting to be built.
	QET_LAZY_FOLIOS_TRACE=1 says when it happens, to find the callers that
	still need every folio.
*/
void QETProject::loadFolios() const
{
	const int waiting = unloadedFolioCount();
	if (!waiting) {
		return;
	}
	if (qEnvironmentVariableIntValue("QET_LAZY_FOLIOS_TRACE") == 1) {
		qWarning().noquote() << "lazy folios:" << waiting << "folios built for a caller of diagrams()";
	}
	buildFolios(m_diagrams_list);
}

/**
	@brief QETProject::setFolioBuildObserver
	@p observer is called after each folio built by buildFolios(), with the
	number of folios built so far and the number to build : a long build
	can show its progress, and keep the window system answered by running
	the event loop (without the user input) in @p observer. The autosave
	waits meanwhile. An empty function removes the observer.
*/
void QETProject::setFolioBuildObserver(std::function<void (int, int)> observer) {
	m_folio_build_observer = std::move(observer);
}

/**
	@brief QETProject::buildFolios
	Build those of @p folios still waiting, as opening builds them: all
	first, then what opening does after building (the cross-references of
	each look for symbols on the others), with the project in the state
	opening has, which keeps the texts where the file put them (a text
	aligned right is not moved when its content is set).
*/
void QETProject::buildFolios(const QList<Diagram *> &folios) const
{
	auto *self = const_cast<QETProject *>(this);
	const ProjectState state = m_state;
	self->m_state = ProjectParsingRunning;
		//Batch the database's signals as a paste does
		//(DiagramEventAddPaste): every symbol and wire built would emit
		//dataBaseUpdated(), and every table already built would measure all
		//its rows again each time -- minutes for a save building 40 folios
		//with 10 parts-list folios built. The rows are still written as each
		//item is built: only the signal waits, and is sent once at the end
		//(updateDB() would fill the whole database again, every time).
	const bool signals_were_blocked = self->m_data_base.signalsBlocked();
	self->m_data_base.blockSignals(true);
	QList<Diagram *> built;
	int waiting_ = 0;
	if (m_folio_build_observer) {
		for (Diagram *d : folios)
			if (!d->isLoaded()) ++waiting_;
	}
	for (Diagram *d : folios)
	{
		if (d->buildDeferred())
		{
			built << d;
				//Someone shows the progress (a long build, opening the
				//terminal block manager...)
			if (m_folio_build_observer) {
				m_folio_build_observer(built.size(), waiting_);
			}
		}
	}
	for (Diagram *d : std::as_const(built))
		self->folioLoaded(d);
	self->m_data_base.blockSignals(signals_were_blocked);
	if (!signals_were_blocked && !built.isEmpty()) {
		emit self->m_data_base.dataBaseUpdated();
	}
		//A symbol built before still keeping the links it was saved with
		//(a partner on a folio not built): its initLink() links those just
		//built, puts its partners back in their saved order and drops what
		//is done
	for (Diagram *d : builtFolios())
		if (!built.contains(d))
			for (Element *elmt : d->elements())
				if (elmt->keepsSavedLinks())
					elmt->initLink(self);
		//A linked folio report's texts show a wire's properties: the
		//report's own wire, else its partner's. A report built alone
		//first, as a partner (buildPartnerSymbols()), linked before it had
		//wires, and a report elsewhere may watch that partner: both look
		//again now, as opening has them do once every wire exists (an
		//unlinked report keeps its texts, as opening leaves them)
	for (Diagram *d : std::as_const(built))
		for (Element *elmt : d->elements())
			if ((elmt->linkType() & Element::AllReport) && !elmt->linkedElements().isEmpty())
			{
				emit elmt->linkedElementChanged();
				for (Element *partner : elmt->linkedElements())
					if (!built.contains(partner->diagram()))
						emit partner->linkedElementChanged();
			}
		//Likewise a table whose previous table was on a folio not built
	for (Diagram *d : builtFolios())
		for (QGraphicsItem *item : d->items())
			if (item->type() == QetGraphicsTableItem::Type)
				if (auto table = static_cast<QetGraphicsTableItem *>(item);
						table->hasPendingLink())
					table->initLink();
		//The tables just built were filled with nothing, as opening builds
		//them before the database is filled (ProjectDBModel::fillValue()):
		//fill them now, as opening does then
	QSet<ProjectDBModel *> models;
	for (Diagram *d : std::as_const(built))
		for (QGraphicsItem *item : d->items())
			if (item->type() == QetGraphicsTableItem::Type)
				if (auto model = qobject_cast<ProjectDBModel *>(
						static_cast<QetGraphicsTableItem *>(item)->model()))
					models.insert(model);
	for (ProjectDBModel *model : std::as_const(models))
		model->folioBuilt();
	self->m_state = state;
}

/**
	@brief QETProject::buildFoliosHolding
	Build the folios not built yet (lazy folios) holding a symbol of
	one of the kinds @p kinds -- if @p free_only, one which may be linked
	to nothing once built: none of the partners it was saved linked to is
	in the project. What a search for such symbols must build to find
	every one (ElementProvider), and no more.
*/
void QETProject::buildFoliosHolding(ElementData::Types kinds, bool free_only)
{
	if (!unloadedFolioCount()) {
		return;
	}
	QSet<QUuid> unbuilt;
	QSet<QUuid> placed;
	for (Diagram *d : std::as_const(m_diagrams_list)) {
		if (!d->isLoaded()) {
			unbuilt.insert(d->uuid());
		} else if (free_only) {
			for (Element *element : d->elements())
				placed.insert(element->uuid());
		}
	}
		//A symbol the database has no row for is one building leaves out
	const QHash<QUuid, QString> types = m_data_base.elementTypesOnFolios(unbuilt);
	if (free_only) {
		for (auto it = types.cbegin() ; it != types.cend() ; ++it)
			placed.insert(it.key());
	}
		//The symbols and their links as the database has them
	QList<Diagram *> to_build;
	for (Diagram *d : std::as_const(m_diagrams_list))
	{
		if (d->isLoaded()) {
			continue;
		}
		for (const projectDataBase::FolioSymbol &symbol : m_data_base.symbolsOnFolios({d->uuid()}))
		{
			if (!(kinds & ElementData::typeFromString(symbol.kind))) {
				continue;
			}
			bool linked = false;
			if (free_only) {
				for (const QUuid &link : m_data_base.linkedSymbols(symbol.uuid)) {
					if (placed.contains(link)) {
						linked = true;
						break;
					}
				}
			}
			if (!linked) {
				to_build << d;
				break;
			}
		}
	}
	if (to_build.isEmpty()) {
		return;
	}
	if (qEnvironmentVariableIntValue("QET_LAZY_FOLIOS_TRACE") == 1) {
		qWarning().noquote() << "lazy folios:" << to_build.size()
							 << "folios built for a search of"
							 << (free_only ? "free" : "") << "symbols of kinds" << int(kinds);
	}
	buildFolios(to_build);
}

/**
	@brief QETProject::buildFoliosHoldingSymbols
	Build the folios not built yet (lazy folios) holding one of the
	symbols @p symbols, found in the project database once it holds the
	folios not built, else in the XML kept for them: what needs those
	symbols while the project is being read, before the database knows
	where they are (a terminal strip's terminals)
*/
void QETProject::buildFoliosHoldingSymbols(const QSet<QUuid> &symbols)
{
	if (symbols.isEmpty() || !unloadedFolioCount()) {
		return;
	}
	QList<Diagram *> to_build;
		//Filled from the document, the database knows where every symbol is
	if (m_data_base.rowsFromDocument()) {
		const QSet<QUuid> folios = m_data_base.folioUuidsOfElements(symbols);
		for (Diagram *d : std::as_const(m_diagrams_list))
			if (!d->isLoaded() && folios.contains(d->uuid()))
				to_build << d;
		buildFolios(to_build);
		return;
	}
	for (Diagram *d : std::as_const(m_diagrams_list))
	{
		if (d->isLoaded()) {
			continue;
		}
		for (const QDomElement &element_xml :
			 QET::findInDomElement(d->deferredXml(),
								   QStringLiteral("elements"),
								   QStringLiteral("element"))) {
			if (symbols.contains(QUuid(element_xml.attribute(QStringLiteral("uuid"))))) {
				to_build << d;
				break;
			}
		}
	}
	if (to_build.isEmpty()) {
		return;
	}
	if (qEnvironmentVariableIntValue("QET_LAZY_FOLIOS_TRACE") == 1) {
		qWarning().noquote() << "lazy folios:" << to_build.size()
							 << "folios built for the symbols asked for by uuid";
	}
	buildFolios(to_build);
}

/**
	@brief QETProject::symbolsOnUnbuiltFolios
	@return the uuids of the symbols on the folios not built yet
	(lazy folios) that building them places
*/
QSet<QUuid> QETProject::symbolsOnUnbuiltFolios() const
{
	QSet<QUuid> unbuilt;
	for (Diagram *d : m_diagrams_list)
		if (!d->isLoaded()) unbuilt.insert(d->uuid());
	QSet<QUuid> symbols;
	if (unbuilt.isEmpty()) {
		return symbols;
	}
	const QHash<QUuid, QString> types = m_data_base.elementTypesOnFolios(unbuilt);
	for (auto it = types.cbegin() ; it != types.cend() ; ++it)
		symbols.insert(it.key());
	return symbols;
}

/**
	@brief QETProject::tableChainFolios
	@return the folios holding a table chained to one on @p folio, before
	or after it, read from the folios built and the XML kept for the
	others: a chain shares its model and its columns' widths
*/
QSet<Diagram *> QETProject::tableChainFolios(Diagram *folio) const
{
	QHash<QUuid, Diagram *> folio_of;
	QHash<QUuid, QUuid> previous_of;
	for (Diagram *d : m_diagrams_list)
	{
		if (d->isLoaded()) {
			for (QGraphicsItem *item : d->items()) {
				if (item->type() == QetGraphicsTableItem::Type) {
					auto table = static_cast<QetGraphicsTableItem *>(item);
					folio_of.insert(table->uuid(), d);
					previous_of.insert(table->uuid(), table->previousTableUuid());
				}
			}
			continue;
		}
		const QDomNodeList tables = d->deferredXml().elementsByTagName(
					QetGraphicsTableItem::xmlTagName());
		for (int i = 0 ; i < tables.size() ; ++i) {
			const QDomElement table = tables.at(i).toElement();
			const QUuid uuid(table.attribute(QStringLiteral("uuid")));
			folio_of.insert(uuid, d);
			const QDomElement previous = table.firstChildElement(QStringLiteral("previous_table"));
			previous_of.insert(uuid, QUuid(previous.attribute(QStringLiteral("uuid"))));
		}
	}
		//Every table's chain head, then the folios of the heads @p folio has
	auto head = [&previous_of](QUuid uuid) {
		QSet<QUuid> seen;
		while (!previous_of.value(uuid).isNull() && !seen.contains(uuid)) {
			seen.insert(uuid);
			uuid = previous_of.value(uuid);
		}
		return uuid;
	};
	QSet<QUuid> heads;
	for (auto it = folio_of.cbegin() ; it != folio_of.cend() ; ++it)
		if (it.value() == folio)
			heads.insert(head(it.key()));
	QSet<Diagram *> folios;
	if (heads.isEmpty()) {
		return folios;
	}
	for (auto it = folio_of.cbegin() ; it != folio_of.cend() ; ++it)
		if (heads.contains(head(it.key())))
			folios.insert(it.value());
	return folios;
}

/**
	@brief QETProject::buildFolioToShow
	Build @p folio to show it, and the folios holding the partners its
	symbols were saved linked to: a cross-reference or a folio report
	draws what it knows of its partner, which must be built for that.
	Their own partners on folios further away wait until those are built.
*/
void QETProject::buildFolioToShow(Diagram *folio)
{
		//A folio built earlier as another's partner may still have partners
		//of its own waiting: it is looked at again
	if (!folio || !unloadedFolioCount()) {
		return;
	}
		//A table chain is built at once: its model is in its first table
	QSet<Diagram *> chained = tableChainFolios(folio);
	chained.insert(folio);
	QList<Diagram *> first;
	for (Diagram *d : std::as_const(m_diagrams_list))
		if (chained.contains(d))
			first << d;
	buildFolios(first);
	QSet<QUuid> partners;
	for (Element *elmt : folio->elements())
		partners.unite(elmt->pendingLinks());
	const QSet<QUuid> folio_uuids = partners.isEmpty()
			? QSet<QUuid>()
			: m_data_base.folioUuidsOfElements(partners);
	QList<Diagram *> to_build;
	for (Diagram *d : std::as_const(m_diagrams_list))
		if (!d->isLoaded() && folio_uuids.contains(d->uuid()))
			to_build << d;
	if (to_build.isEmpty()) {
		return;
	}
	if (qEnvironmentVariableIntValue("QET_LAZY_FOLIOS_TRACE") == 1) {
		qWarning().noquote() << "lazy folios: the partners of folio" << folio->folioIndex() + 1
							 << "built alone, on" << to_build.size() << "folios";
	}
		//The partners alone, not their folios (stage D5b)
	buildPartnerSymbols(to_build, partners);
}

/**
	@brief QETProject::buildPartnerSymbols
	Build on the folios @p folios, not built, the symbols @p partners alone
	(Diagram::buildSymbols()), as opening builds them: linked to the symbols
	built already, their texts refreshed, in the state opening has.
*/
void QETProject::buildPartnerSymbols(const QList<Diagram *> &folios, const QSet<QUuid> &partners)
{
	const ProjectState state = m_state;
	m_state = ProjectParsingRunning;
	QList<Element *> built;
	for (Diagram *d : folios) {
		const QList<Element *> symbols = d->buildSymbols(partners);
		if (!symbols.isEmpty()) {
			built += symbols;
			linkElementsToElementAutoNums(d);
		}
	}
	for (Element *element : std::as_const(built)) {
		element->initLink(this);
		for (auto &text : element->dynamicTextItems())
			text->refreshLabelConnection();
	}
		//A built symbol linked to one of them puts its partners back in
		//their saved order
	for (Diagram *d : builtFolios())
		for (Element *element : d->elements())
			if (element->keepsSavedLinks())
				element->initLink(this);
	m_state = state;
}

/**
	@brief QETProject::folioLoaded
	@p folio's items were just built, after the project was opened: do for
	them what opening does for every folio's.
*/
void QETProject::folioLoaded(Diagram *folio)
{
	linkElementsToElementAutoNums(folio);
	folio->refreshContents();
}

/**
	@param diagram Pointer to a Diagram object
	@return the folio number of the given diagram object within the project,
	or -1 if it is not part of this project.
	Note: this returns 0 for the first diagram, not 1
*/
int QETProject::folioIndex(const Diagram *diagram) const
{
	// QList::indexOf returns -1 if no item matched.
	return(m_diagrams_list.indexOf(const_cast<Diagram *>(diagram)));
}

/**
	@brief QETProject::embeddedCollection
	@return The embedded collection
*/
XmlElementCollection *QETProject::embeddedElementCollection() const
{
	return m_elements_collection;
}

/**
	@return the title block templates collection enbeedded within this project
*/
TitleBlockTemplatesProjectCollection *QETProject::embeddedTitleBlockTemplatesCollection()
{
	return(&m_titleblocks_collection);
}

/**
	@return le chemin du fichier dans lequel ce projet est enregistre
*/
QString QETProject::filePath()
{
	return(m_file_path);
}

/**
	@brief QETProject::setFilePath
	Set the filepath of this project file
	Set a file path also create a backup file according to the path.
	If a previous path was set, the previous backup file is deleted and a new one
	is created according to the path.
	@param filepath
*/
void QETProject::setFilePath(const QString &filepath)
{
	if (filepath == m_file_path) {
		return;
	}
		//Don't close/re-point the backup files while a backup is still writing one.
	m_backup_future.waitForFinished();
	const QUrl managed_file = QUrl::fromLocalFile(filepath);
	for (auto &backup_file : m_backup_files) {
		if (backup_file.isOpen()) {
			backup_file.close();
		}
		backup_file.setManagedFile(managed_file);
	}
	m_next_backup_slot = 0;
	m_file_path = filepath;
		//A path chosen is saved to as it is
	m_opened_from_qet = false;

	QFileInfo fi(m_file_path);
	if (fi.isWritable()) {
		setReadOnly(false);
	}

		//The saved date, time and path of the title block are set by
		//write(), when the project is saved (stampSaved())
	emit projectFilePathChanged(this, m_file_path);
	emit projectInformationsChanged(this);
	updateDiagramsFolioData();
}

/**
	@return the folder containing the project file if it has been saved;
	otherwise, this method returns the location of the user's documents.
	en français:
	@return le dossier contenant le fichier du projet s'il a été enregistré ;
	sinon, cette méthode renvoie l'emplacement des documents de l'utilisateur.
*/
QString QETProject::currentDir() const
{
	QString current_directory;
	if (m_file_path.isEmpty()) {
		current_directory = QETApp::documentDir();
	} else {
		current_directory = QFileInfo(m_file_path).absoluteDir().absolutePath();
	}
	return(current_directory);
}

/**

	@return une chaine de caractere du type "Projet titre du projet".
	Si le projet n'a pas de titre, le nom du fichier est utilise.
	Si le projet n'est pas associe a un fichier, cette methode retourne "Projet
	sans titre".
	De plus, si le projet est en lecture seule, le tag "[lecture seule]" est
	ajoute.
*/
QString QETProject::pathNameTitle() const
{
	QString final_title;

	if (!project_title_.isEmpty()) {
		final_title = QString(
			tr(
				"Project « %1 : %2»",
				"displayed title for a ProjectView - %1 is the project title, -%2 is the project path"
			)
		).arg(project_title_, m_file_path);
	} else if (!m_file_path.isEmpty()) {
		final_title = QString(
			tr(
				"Project %1",
				"displayed title for a title-less project - %1 is the file name"
			)
		).arg(QFileInfo(m_file_path).completeBaseName());
	} else {
		final_title = QString(
			tr(
				"Untitled project",
				"displayed title for a project-less, file-less project"
			)
		);
	}

	if (isReadOnly()) {
		final_title = QString(
			tr(
				"%1 [read-only]",
				"displayed title for a read-only project - %1 is a displayable title"
			)
		).arg(final_title);
	}
	// Same condition as projectWasModified(): project-options changeg (m_modified) OR the undo stack sitting away from
	// its clean index. 
	if (m_modified || !m_undo_stack->isClean()) {
		final_title = QString(
			tr(
				"%1 [modified]",
				"displayed title for a modified project - %1 is a displayable title"
			)
		).arg(final_title);
	}

	return(final_title);
}

/**
	@return le titre du projet
*/
QString QETProject::title() const
{
	return(project_title_);
}

/**
	@return la version de QElectroTech declaree dans le fichier projet lorsque
	celui-ci a ete ouvert ; si ce projet n'a jamais ete enregistre / ouvert
	depuis un fichier, cette methode une version nulle.
*/
QVersionNumber QETProject::declaredQElectroTechVersion()
{
	return(m_project_qet_version);
}

/**
	@param title le nouveau titre du projet
*/
void QETProject::setTitle(const QString &title) {
	// ne fait rien si le projet est en lecture seule
	if (isReadOnly()) return;

	// ne fait rien si le titre du projet n'est pas change par l'appel de cette methode
	if (project_title_ == title) return;

	project_title_ = title;
	emit projectTitleChanged(this, project_title_);
	emit projectInformationsChanged(this);
	updateDiagramsFolioData();
}

/**
	@return les dimensions par defaut utilisees lors de la creation d'un
	nouveau schema dans ce projet.
*/
BorderProperties QETProject::defaultBorderProperties() const
{
	return(default_border_properties_);
}

/**
	Permet de specifier les dimensions par defaut utilisees lors de la creation
	d'un nouveau schema dans ce projet.
	@param border dimensions d'un schema
*/
void QETProject::setDefaultBorderProperties(const BorderProperties &border) {
	default_border_properties_ = border;
}

/**
	@return le cartouche par defaut utilise lors de la creation d'un
	nouveau schema dans ce projet.
*/
TitleBlockProperties QETProject::defaultTitleBlockProperties() const
{
	return(default_titleblock_properties_);
}

QList<GuideProperties> QETProject::defaultGuides() const {
	return m_default_guides;
}

void QETProject::setDefaultGuides(const QList<GuideProperties> &guides) {
	if (m_default_guides != guides) {
		m_default_guides = guides;
		setModified(true);

		for (Diagram *diagram : m_diagrams_list) {
			if (diagram) {
				diagram->updateProjectGuides(m_default_guides);
			}
		}
	}
}

/**
	@brief QETProject::setDefaultTitleBlockProperties
	Specify the title block to be used at the creation of a new diagram for this project
	@param titleblock
*/
void QETProject::setDefaultTitleBlockProperties(const TitleBlockProperties &titleblock) {
	default_titleblock_properties_ = titleblock;
		//Integrate the title block in this project
	if (!titleblock.template_name.isEmpty())
	{
		TitleBlockTemplatesFilesCollection *collection = nullptr;
		switch (titleblock.collection)
		{
			case QET::Common :
				collection = QETApp::commonTitleBlockTemplatesCollection();
				break;
			case QET::Company :
				collection = QETApp::companyTitleBlockTemplatesCollection();
				break;
			case QET::Custom :
				collection = QETApp::customTitleBlockTemplatesCollection();
				break;
			case QET::Embedded :
				//Titleblock is already embedded to project
				return;
		}

		IntegrationMoveTitleBlockTemplatesHandler m_;
		integrateTitleBlockTemplate(collection -> location(titleblock.template_name), &m_);
	}
	emit defaultTitleBlockPropertiesChanged();
}

/**
	@return le type de conducteur par defaut utilise lors de la creation d'un
	nouveau schema dans ce projet.
*/
ConductorProperties QETProject::defaultConductorProperties() const
{
	return(default_conductor_properties_);
}

/**
	Permet de specifier e type de conducteur par defaut utilise lors de la
	creation d'un nouveau schema dans ce projet.
*/
void QETProject::setDefaultConductorProperties(const ConductorProperties &conductor) {
	default_conductor_properties_ = conductor;
}

QString QETProject::defaultReportProperties() const
{
	return m_default_report_properties;
}

void QETProject::setDefaultReportProperties(const QString &properties)
{
	QString old = m_default_report_properties;
	m_default_report_properties = properties;

	emit reportPropertiesChanged(old, properties);
}

void QETProject::setDefaultXRefProperties(const QString& type, const XRefProperties &properties) {
	m_default_xref_properties.insert(type, properties);
	emit XRefPropertiesChanged();
}

void QETProject::setDefaultXRefProperties(QHash<QString, XRefProperties> hash)
{
	m_default_xref_properties.swap(hash);
	emit XRefPropertiesChanged();
}

/**
	@brief QETProject::conductorAutoNum
	@return All value of conductor autonum stored in project
*/
QHash <QString, NumerotationContext> QETProject::conductorAutoNum() const
{
	return m_conductor_autonum;
}

/**
	@brief QETProject::elementAutoNum
	@return All value of element autonum stored in project
*/
QHash <QString, NumerotationContext> QETProject::elementAutoNum() const
{
	return m_element_autonum;
}

/**
	@brief QETProject::cableAutoNum
	@return the cable numerotation stored in the project -- one entry at
	most, since cables are numbered by a single rule
*/
QHash <QString, NumerotationContext> QETProject::cableAutoNum() const
{
	return m_cable_autonum;
}

/**
	@brief QETProject::elementAutoNumFormula
	@param key : autonum title
	@return Formula of element autonum stored in element autonum
*/
QString QETProject::elementAutoNumFormula (const QString& key) const
{
	if (m_element_autonum.contains(key)) {
		return autonum::numerotationContextToFormula(m_element_autonum[key]);
	}

	return QString();
}

/**
	@brief QETProject::elementAutoNumCurrentFormula
	@return current formula being used by project
*/
QString QETProject::elementAutoNumCurrentFormula() const
{
	return elementAutoNumFormula(m_current_element_autonum);
}

/**
	@brief QETProject::elementCurrentAutoNum
	@return current element autonum title
*/
QString QETProject::elementCurrentAutoNum () const
{
	return m_current_element_autonum;
}

/**
	@brief QETProject::setCurrrentElementAutonum
	@param autoNum : set the current element autonum to autonum
*/
void QETProject::setCurrrentElementAutonum(QString autoNum) {
	m_current_element_autonum = std::move(autoNum);
}

/**
	@brief QETProject::cableAutoNumFormula
	@param key : autonum title
	@return Formula of the cable numerotation stored with key
*/
QString QETProject::cableAutoNumFormula(const QString& key) const
{
	if (m_cable_autonum.contains(key)) {
		return autonum::numerotationContextToFormula(m_cable_autonum.value(key));
	}

	return QString();
}

/**
	@brief QETProject::hasCableAutoNum
	@return true when a numbering rule for cables is set: a context
	stored under the name which is in use. A project which has never been
	given one answers false, and that is what makes the cable tool ask
	whether one should be defined (see QETDiagramEditor).
*/
bool QETProject::hasCableAutoNum() const
{
	if (m_current_cable_autonum.isEmpty()) return false;
	return !cableAutoNum(m_current_cable_autonum).isEmpty();
}

/**
	@brief QETProject::cableCurrentAutoNum
	@return title of the cable numerotation in use (one rule only)
*/
QString QETProject::cableCurrentAutoNum() const
{
	return m_current_cable_autonum;
}

/**
	@brief QETProject::setCurrentCableAutoNum
	@param autoNum : the rule which from now on numbers the cables
*/
void QETProject::setCurrentCableAutoNum(QString autoNum)
{
	m_current_cable_autonum = std::move(autoNum);
}

/**
	@brief QETProject::cableAutoNumRuleName
	@return the name the one and only cable numbering rule is kept under
*/
QString QETProject::cableAutoNumRuleName()
{
	return QStringLiteral("Cable");
}

/**
	@brief QETProject::cableXAxisFirst
	@return true when the cables are numbered along the X axis (the
	leftmost first), false when they are numbered along the Y axis (the
	topmost first)
*/
bool QETProject::cableXAxisFirst() const
{
	return m_cable_axis_x_first;
}

/**
	@brief QETProject::setCableXAxisFirst
	Which axis the whole project numbers its cables along. Stored with
	the rule rather than on its own, since it only means anything while
	a rule is being applied to every cable at once.
	@param x_axis_first
*/
void QETProject::setCableXAxisFirst(bool x_axis_first)
{
	m_cable_axis_x_first = x_axis_first;
}

/**
	@brief QETProject::cableAskNumbering
	@return true when the cable tool may still ask this project whether
	a numbering rule should be defined. The answer to that question is
	kept with the project rather than in the program settings, so that
	one project being answered with no does not stop every other
	project from asking.
*/
bool QETProject::cableAskNumbering() const
{
	return m_cable_ask_numbering;
}

/**
	@brief QETProject::setCableAskNumbering
	Take the answer he gave to the numbering question down with the
	project. False means he has said no once here and is not to be
	asked again in this project; taking the rule away puts it back to
	true so the question comes again.
	@param ask
*/
void QETProject::setCableAskNumbering(bool ask)
{
	m_cable_ask_numbering = ask;
}

/**
	@brief QETProject::renumberElementsBySchemeTitle
	Renumber existing elements by element autonumbering scheme title.

	Elements follow a scheme by its uuid (elementInformations["formula_id"]),
	see elementsUsingElementAutoNum().

	If scheme_title is empty, all schemes are renumbered. Otherwise only that scheme is renumbered.
	The operation is undoable.
*/
void QETProject::renumberElementsBySchemeTitle(const QString &scheme_title)
{
	if (!m_undo_stack) return;

	auto *cmd = ElementAutoNumSchemeCommand::renumber(
				this, scheme_title, nullptr,
				scheme_title.isEmpty() ? tr("Renumber elements")
									   : tr("Renumber elements (%1)").arg(scheme_title));
	if (cmd) {
		m_undo_stack->push(cmd);
	}
}

/**
	@brief QETProject::conductorAutoNumFormula
	@param key : autonum title
	@return Formula of element autonum stored in conductor autonum
*/
QString QETProject::conductorAutoNumFormula (const QString& key) const
{
	if (m_conductor_autonum.contains(key))
		return autonum::numerotationContextToFormula(m_conductor_autonum.value(key));
	else
		return QString();
}

/**
	@brief QETProject::conductorCurrentAutoNum
	@return current conductor autonum title
*/
QString QETProject::conductorCurrentAutoNum () const
{
	return m_current_conductor_autonum;
}

/**
	@brief QETProject::setCurrentConductorAutoNum
	@param autoNum set the current conductor autonum to autonum
*/
void QETProject::setCurrentConductorAutoNum(QString autoNum) {
	m_current_conductor_autonum = std::move(autoNum);
}

/**
	@brief QETProject::folioAutoNum
	@return All value of folio autonum stored in project
*/
QHash <QString, NumerotationContext> QETProject::folioAutoNum() const
{
	return  m_folio_autonum;
}

/**
	@brief QETProject::addConductorAutoNum
	Add a new conductor numerotation context. If key already exist,
	replace old context with the new context
	@param key
	@param context
*/
void QETProject::addConductorAutoNum(const QString& key, const NumerotationContext& context) {
	m_conductor_autonum.insert(key, context);
	emit autoNumContextUpdated();
}

/**
	@brief QETProject::addElementAutoNum
	Add a new element numerotation context. If key already exist,
	replace old context with the new context
	@param key
	@param context
*/
void QETProject::addElementAutoNum(const QString& key, const NumerotationContext& context)
{
	addElementAutoNum(key, context, QUuid());
}

/**
	@brief QETProject::addElementAutoNum
	Add or replace the element numbering scheme @p key.
	A scheme that already exists keeps its uuid unless @p id is given;
	a new one takes @p id, or a new uuid when @p id is null or already
	used by another scheme.
	@param key : title of the scheme
	@param context : its numerotation context
	@param id : its uuid, null to keep or create one
*/
void QETProject::addElementAutoNum(const QString &key,
								   const NumerotationContext &context,
								   const QUuid &id)
{
	QUuid scheme_id = id;
	if (!scheme_id.isNull()) {
		const QString owner = elementAutoNumTitle(scheme_id);
		if (!owner.isEmpty() && owner != key) {
			scheme_id = QUuid();
		}
	}
	if (scheme_id.isNull()) {
		scheme_id = m_element_autonum_id.value(key);
	}
	if (scheme_id.isNull()) {
		scheme_id = QUuid::createUuid();
	}
	m_element_autonum_id.insert(key, scheme_id);
	m_element_autonum.insert(key, context);
	emit elementAutoNumAdded(key);
	emit autoNumContextUpdated();
}

/**
	@brief QETProject::elementAutoNumId
	@return the uuid of the element numbering scheme @p title, null if
	there is no such scheme
*/
QUuid QETProject::elementAutoNumId(const QString &title) const
{
	return m_element_autonum_id.value(title);
}

/**
	@brief QETProject::elementAutoNumTitle
	@return the title of the element numbering scheme with uuid @p id,
	empty if there is none
*/
QString QETProject::elementAutoNumTitle(const QUuid &id) const
{
	if (id.isNull()) {
		return QString();
	}
	for (auto it = m_element_autonum_id.constBegin();
		 it != m_element_autonum_id.constEnd(); ++it) {
		if (it.value() == id) {
			return it.key();
		}
	}
	return QString();
}

/**
	@brief QETProject::renameElementAutoNum
	Give the element numbering scheme @p old_title the title @p new_title.
	It keeps its uuid, so the elements following it are not touched;
	the folios' per-scheme maxima of folio sequential numbers and the
	project's current scheme follow the new title.
	Not undoable by itself: see ElementAutoNumSchemeCommand.
	@return false if there is no scheme @p old_title or a scheme
	@p new_title already exists
*/
bool QETProject::renameElementAutoNum(const QString &old_title, const QString &new_title)
{
	if (old_title == new_title) {
		return m_element_autonum.contains(old_title);
	}
	if (!m_element_autonum.contains(old_title)
			|| m_element_autonum.contains(new_title)
			|| new_title.isEmpty()) {
		return false;
	}

	m_element_autonum.insert(new_title, m_element_autonum.take(old_title));
	m_element_autonum_id.insert(new_title, m_element_autonum_id.take(old_title));
	if (m_current_element_autonum == old_title) {
		m_current_element_autonum = new_title;
	}

	for (Diagram *d : std::as_const(m_diagrams_list)) {
		if (!d) continue;
		for (auto *hash : {&d->m_elmt_unitfolio_max,
						   &d->m_elmt_tenfolio_max,
						   &d->m_elmt_hundredfolio_max}) {
			if (hash->contains(old_title)) {
				hash->insert(new_title, hash->take(old_title));
			}
		}
	}

	emit elementAutoNumRemoved(old_title);
	emit elementAutoNumAdded(new_title);
	emit autoNumContextUpdated();
	return true;
}

/**
	@brief QETProject::normalizedAutoNumName
	@return @p name as it is stored as the title of a numbering scheme
*/
QString QETProject::normalizedAutoNumName(const QString &name)
{
	return name.simplified();
}

/**
	@brief QETProject::elementAutoNumNameClash
	Two element numbering schemes may not have the same name, compared
	without regard to case or surrounding white space, so that the name
	alone identifies a scheme for the user and for scripts.
	@param name : the name to check
	@param ignored_title : a scheme not to compare with (the one being
	renamed)
	@return the title of the existing scheme @p name clashes with, empty
	if none
*/
QString QETProject::elementAutoNumNameClash(const QString &name,
											const QString &ignored_title) const
{
	const QString wanted = normalizedAutoNumName(name);
	for (auto it = m_element_autonum.constBegin();
		 it != m_element_autonum.constEnd(); ++it) {
		if (it.key() == ignored_title) {
			continue;
		}
		if (QString::compare(normalizedAutoNumName(it.key()), wanted,
							 Qt::CaseInsensitive) == 0) {
			return it.key();
		}
	}
	return QString();
}

/**
	@brief QETProject::elementsUsingElementAutoNum
	@return the elements whose label follows the element numbering
	scheme @p title, in no particular order
*/
QVector<Element *> QETProject::elementsUsingElementAutoNum(const QString &title) const
{
	loadFolios(); //walks the folios' symbols
	QVector<Element *> list;
	const QUuid id = elementAutoNumId(title);
	if (id.isNull()) {
		return list;
	}
	for (Diagram *d : m_diagrams_list) {
		if (!d) continue;
		const auto items = d->items();
		for (QGraphicsItem *it : items) {
			auto *el = qgraphicsitem_cast<Element *>(it);
			if (!el) continue;
			const DiagramContext &info = el->elementInformations();
			if (info.value(QETInformation::ELMT_FORMULA).toString().isEmpty()) {
				continue;
			}
			if (QUuid(info.value(QETInformation::ELMT_FORMULA_ID).toString()) == id) {
				list << el;
			}
		}
	}
	return list;
}

/**
	@brief QETProject::linkElementsToElementAutoNums
	Called once the diagrams of a file are loaded. Makes every element's
	ELMT_FORMULA_ID name an element numbering scheme of this project:
	- an id naming one of the schemes is kept;
	- an id naming none (the scheme was renamed by a version of
	QElectroTech that lost the ids, or the element was pasted from
	another project) and, in a file written before the ids existed,
	an element with no id at all, are linked to the one scheme whose
	formula is the element's formula; with no such scheme, or more than
	one, the element is left unlinked.
	The label is not touched: nothing it is built from changes.
*/
void QETProject::linkElementsToElementAutoNums()
{
	for (Diagram *d : std::as_const(m_diagrams_list)) {
		if (d) linkElementsToElementAutoNums(d);
	}
}

/**
	@brief QETProject::linkElementsToElementAutoNums
	linkElementsToElementAutoNums() for the elements of @p folio alone
*/
void QETProject::linkElementsToElementAutoNums(Diagram *folio)
{
	const QHash<QString, QStringList> titles_by_formula = elementAutoNumTitlesByFormula();
	{
		const auto items = folio->items();
		for (QGraphicsItem *it : items) {
			auto *el = qgraphicsitem_cast<Element *>(it);
			if (!el) continue;
			if (const auto id = elementAutoNumIdToLink(el->elementInformations(),
													   titles_by_formula)) {
				el->setFormulaSchemeId(*id);
			}
		}
	}
}

/**
	@brief QETProject::elementAutoNumTitlesByFormula
	@return the titles of the element numbering schemes, by their formula
*/
QHash<QString, QStringList> QETProject::elementAutoNumTitlesByFormula() const
{
	QHash<QString, QStringList> titles_by_formula;
	for (auto it = m_element_autonum.constBegin();
		 it != m_element_autonum.constEnd(); ++it) {
		titles_by_formula[autonum::numerotationContextToFormula(it.value())] << it.key();
	}
	return titles_by_formula;
}

/**
	@brief QETProject::elementAutoNumIdToLink
	The rule of linkElementsToElementAutoNums() for one element, whose
	information is @p information.
	@return the scheme id the element must be given, or nothing when the
	one it has is kept
*/
std::optional<QUuid> QETProject::elementAutoNumIdToLink(
		const DiagramContext &information,
		const QHash<QString, QStringList> &titles_by_formula) const
{
	const QString formula = information.value(QETInformation::ELMT_FORMULA).toString();
	const bool has_id = information.contains(QETInformation::ELMT_FORMULA_ID);
	const QUuid id(information.value(QETInformation::ELMT_FORMULA_ID).toString());

	if (formula.isEmpty()) {
		if (has_id) return QUuid();
		return std::nullopt;
	}
	if (!elementAutoNumTitle(id).isEmpty()) {
		return std::nullopt;
	}
	if (!has_id && !m_legacy_element_autonums) {
		return std::nullopt; //A formula typed by hand
	}
	const QStringList matches = titles_by_formula.value(formula);
	return matches.size() == 1 ? elementAutoNumId(matches.first()) : QUuid();
}

/**
	@brief QETProject::symbolNumbering
	@return the numbering of every symbol placed on the folios, read from
	the symbol on a folio built and, on a folio not built yet
	(lazy folios), from what building it would give: its information
	in the project database's store, the scheme it would be linked to,
	its sequential numbers and its kind as the database has them.
*/
QList<QETProject::SymbolNumbering> QETProject::symbolNumbering() const
{
	QList<SymbolNumbering> list;
	QSet<QUuid> unbuilt;
	for (Diagram *d : m_diagrams_list)
	{
		if (!d->isLoaded()) {
			unbuilt.insert(d->uuid());
			continue;
		}
		for (QGraphicsItem *item : d->items()) {
			auto *el = qgraphicsitem_cast<Element *>(item);
			if (!el) continue;
			const DiagramContext &info = el->elementInformations();
			SymbolNumbering symbol;
			symbol.element = el;
			symbol.label = info.value(QETInformation::ELMT_LABEL).toString();
			symbol.formula = info.value(QETInformation::ELMT_FORMULA).toString();
			symbol.scheme = QUuid(info.value(QETInformation::ELMT_FORMULA_ID).toString());
			symbol.sequence = el->sequenceStruct();
			symbol.numbered = el->linkType() != Element::Slave
					&& !(el->linkType() & Element::AllReport);
			list << symbol;
		}
	}
	if (unbuilt.isEmpty()) {
		return list;
	}

		//The symbols of the folios not built, from the database. A symbol
		//it has no row for is one building leaves out (its definition is
		//missing)
	QList<QUuid> unbuilt_in_order;
	for (Diagram *d : m_diagrams_list)
		if (!d->isLoaded()) unbuilt_in_order << d->uuid();
	const QHash<QString, QStringList> titles_by_formula = elementAutoNumTitlesByFormula();
	for (const projectDataBase::FolioSymbol &row : m_data_base.symbolsOnFolios(unbuilt_in_order))
	{
		const DiagramContext info = m_data_base.elementInformation(row.uuid);
		SymbolNumbering symbol;
		symbol.label = info.value(QETInformation::ELMT_LABEL).toString();
		symbol.formula = info.value(QETInformation::ELMT_FORMULA).toString();
		const auto linked = elementAutoNumIdToLink(info, titles_by_formula);
		symbol.scheme = linked ? *linked
							   : QUuid(info.value(QETInformation::ELMT_FORMULA_ID).toString());
		symbol.sequence = row.sequence;
		const ElementData::Type kind = ElementData::typeFromString(row.kind);
		symbol.numbered = kind != ElementData::Slave
				&& !(kind & ElementData::AllReport);
		list << symbol;
	}
	return list;
}

/**
	@brief QETProject::addFolioAutoNum
	Add a new folio numerotation context. If key already exist,
	replace old context with the new context
	@param key
	@param context
*/
void QETProject::addFolioAutoNum(const QString& key, const NumerotationContext& context) {
	m_folio_autonum.insert(key, context);
	emit autoNumContextUpdated();
}

/**
	@brief QETProject::addCableAutoNum
	Store the cable numbering rule. There is room for exactly one: saving
	it under a name drops every other entry, so a project can never end up
	with a list of cable rules to choose from.
	@param key
	@param context
*/
void QETProject::addCableAutoNum(const QString& key, const NumerotationContext& context)
{
	if (!key.isEmpty()) {
		const QStringList others = m_cable_autonum.keys();
		for (const QString &other : others) {
			if (other != key) m_cable_autonum.remove(other);
		}
	}
	m_cable_autonum.insert(key, context);
		//The rule in use must always be one which exists: after a save
		//under another name the old entry is gone, so follow the new one.
	if (!key.isEmpty() && (m_current_cable_autonum.isEmpty()
						   || !m_cable_autonum.contains(m_current_cable_autonum))) {
		m_current_cable_autonum = key;
	}
	emit autoNumContextUpdated();
}

/**
	@brief QETProject::removeConductorAutoNum
	Remove Conductor Numerotation Context stored with key
	@param key
*/
void QETProject::removeConductorAutoNum(const QString& key) {
	m_conductor_autonum.remove(key);
}

/**
	@brief QETProject::removeElementAutonum
	Remove Element Numerotation Context stored with key
	@param key
*/
void QETProject::removeElementAutoNum(const QString& key)
{
	m_element_autonum.remove(key);
	m_element_autonum_id.remove(key);
	emit elementAutoNumRemoved(key);
}

/**
	@brief QETProject::removeFolioAutonum
	Remove Folio Numerotation Context stored with key
	@param key
*/
void QETProject::removeFolioAutoNum(const QString& key) {
	m_folio_autonum.remove(key);
}

/**
	@brief QETProject::removeCableAutoNum
	Forget the cable numbering rule stored with key
	@param key
*/
void QETProject::removeCableAutoNum(const QString& key)
{
	m_cable_autonum.remove(key);
	if (m_current_cable_autonum == key) {
		m_current_cable_autonum = m_cable_autonum.isEmpty()
				? QString() : m_cable_autonum.keys().first();
	}
	emit autoNumContextUpdated();
}

/**
	@brief QETProject::conductorAutoNum
	Return conductor numerotation context stored with key.
	If key is not found, return an empty numerotation context
	@param key
*/
NumerotationContext QETProject::conductorAutoNum (const QString &key) const
{
	if (m_conductor_autonum.contains(key)) return m_conductor_autonum[key];
	else return NumerotationContext();
}

/**
	@brief QETProject::elementAutoNum
	Return element numerotation context stored with key.
	If key is not found, return an empty numerotation context
	@param key
*/
NumerotationContext QETProject::elementAutoNum (const QString &key) {
	if (m_element_autonum.contains(key)) return m_element_autonum[key];
	else return NumerotationContext();
}

/**
	@brief QETProject::folioAutoNum
	Return folio numerotation context stored with key.
	If key is not found, return an empty numerotation context
	@param key
*/
NumerotationContext QETProject::folioAutoNum (const QString &key) const
{
	if (m_folio_autonum.contains(key)) return m_folio_autonum[key];
	else return NumerotationContext();
}

/**
	@brief QETProject::cableAutoNum
	Return the cable numerotation context stored with key.
	If key is not found, return an empty numerotation context
	@param key
*/
NumerotationContext QETProject::cableAutoNum(const QString &key) const
{
	if (m_cable_autonum.contains(key)) return m_cable_autonum.value(key);
	else return NumerotationContext();
}

/**
	@brief QETProject::freezeExistentConductorLabel
	Freeze Existent Conductors in the selected folios
	@param freeze
	@param from - first folio index to apply freeze
	@param to - last folio index to apply freeze
*/
void QETProject::freezeExistentConductorLabel(bool freeze, int from, int to) {
	loadFolios(); //walks the folios' wires
	for (int i = from; i <= to; i++) {
		m_diagrams_list.at(i)->freezeConductors(freeze);
	}
}

/**
	@brief QETProject::freezeNewConductorLabel
	Freeze New Conductors in the selected folios
	@param freeze
	@param from - first folio index to apply freeze
	@param to - last folio index to apply freeze
*/
void QETProject::freezeNewConductorLabel(bool freeze, int from, int to) {
	for (int i = from; i <= to; i++) {
		m_diagrams_list.at(i)->setFreezeNewConductors(freeze);
	}
}

/**
	@brief QETProject::isFreezeNewConductors
	@return freeze new conductors Project Wide status
*/
bool QETProject::isFreezeNewConductors()
{
	return m_freeze_new_conductors;
}

/**
	@brief QETProject::setfreezeNewConductors
	Set Project Wide freeze new conductors
*/
void QETProject::setFreezeNewConductors(bool set) {
	m_freeze_new_conductors = set;
}

/**
	@brief QETProject::freezeExistentElementLabel
	Freeze Existent Elements in the selected folios
	@param freeze
	@param from - first folio index to apply freeze
	@param to - last folio index to apply freeze
*/
void QETProject::freezeExistentElementLabel(bool freeze, int from, int to) {
	loadFolios(); //walks the folios' symbols
	for (int i = from; i <= to; i++) {
		m_diagrams_list.at(i)->freezeElements(freeze);
	}
}

/**
	@brief QETProject::freezeNewElementLabel
	Freeze New Elements in the selected folios
	@param freeze
	@param from - first folio index to apply freeze
	@param to - last folio index to apply freeze
*/
void QETProject::freezeNewElementLabel(bool freeze, int from, int to) {
	for (int i = from; i <= to; i++) {
		m_diagrams_list.at(i)->setFreezeNewElements(freeze);
	}
}

/**
	@brief QETProject::freezeNewElements
	@return freeze new elements Project Wide status
*/
bool QETProject::isFreezeNewElements()
{
	return m_freeze_new_elements;
}

/**
	@brief QETProject::setfreezeNewElements
	Set Project Wide freeze new elements
*/
void QETProject::setFreezeNewElements(bool set) {
	m_freeze_new_elements = set;
}

/**
	@brief QETProject::autoConductor
	@return true if use of auto conductor is authorized.
	See also Q_PROPERTY autoConductor
*/
bool QETProject::autoConductor() const
{
	return m_auto_conductor;
}

/**
	@brief QETProject::setAutoConductor
	@param ac
	Enable the use of auto conductor if true
	See also Q_PROPERTY autoConductor
*/
void QETProject::setAutoConductor(bool ac)
{
	if (ac != m_auto_conductor)
		m_auto_conductor = ac;
}

/**
	@brief QETProject::wireHops
	@return which wire of a crossing draws a hop (issue #436),
	WireHops::Mode::None when crossings are drawn as plain lines.
*/
WireHops::Mode QETProject::wireHops() const {
	return m_wire_hops;
}

/**
	@brief QETProject::setWireHops
	Set which wire of a crossing draws a hop, and redraw every folio.
	@param mode
*/
void QETProject::setWireHops(WireHops::Mode mode)
{
	if (mode == m_wire_hops) {
		return;
	}
	m_wire_hops = mode;
		//A folio not built yet reads the mode when its wires are drawn
	for (Diagram *diagram : folios()) {
		diagram->update();
	}
}

/**
	@brief QETProject::uprightSymbolTexts
	@return true if the texts drawn in a symbol stay horizontal, and read
	normally, when the symbol is turned or mirrored on a folio, instead of
	turning with it. A mirrored symbol always keeps its texts readable.
*/
bool QETProject::uprightSymbolTexts() const {
	return m_upright_symbol_texts;
}

/**
	@brief QETProject::setUprightSymbolTexts
	Set whether the texts drawn in a turned symbol stay horizontal, and
	redraw every symbol of the project.
	@param upright
*/
void QETProject::setUprightSymbolTexts(bool upright)
{
	if (upright == m_upright_symbol_texts) {
		return;
	}
	m_upright_symbol_texts = upright;
		//A folio not built yet reads the setting when its symbols are drawn
	for (Diagram *diagram : folios()) {
		for (QGraphicsItem *item : diagram->items()) {
			if (Element *element = qgraphicsitem_cast<Element *>(item)) {
				element->updateSymbolPictures();
			}
		}
	}
}

/**
	@brief QETProject::wiringRules
	@return how many wires a terminal of this project may take
	(discussion #1158): the project's own rules when it sets them,
	otherwise the application's (Settings > General).
*/
WiringRules::Settings QETProject::wiringRules() const {
	return WiringRules::effective(m_wiring_rules, WiringRules::applicationSettings());
}

/**
	@brief QETProject::projectWiringRules
	@return the rules as the project stores them: Settings::own false when
	it follows the application's.
*/
WiringRules::Settings QETProject::projectWiringRules() const {
	return m_wiring_rules;
}

/**
	@brief QETProject::setWiringRules
	Set the project's own wiring rules, or (Settings::own false) make it
	follow the application's. Only wires drawn from now on are affected:
	none already drawn is removed.
	@param rules
*/
void QETProject::setWiringRules(const WiringRules::Settings &rules)
{
	m_wiring_rules = rules;
}

/**
	@brief QETProject::autoBreakConductor
	@return true if use of auto break conductor is authorized.
	See also Q_PROPERTY autoBreakConductor
*/
bool QETProject::autoBreakConductor() const
{
	return m_auto_break_conductor;
}

/**
	@brief QETProject::setAutoBreakConductor
	@param abc
	Enable the use of auto break conductor if true
	See also Q_PROPERTY autoBreakConductor
*/
void QETProject::setAutoBreakConductor(bool abc)
{
	if (abc != m_auto_break_conductor)
		m_auto_break_conductor = abc;
}

/**
	@brief QETProject::autoFolioNumberingNewFolios
	emit Signal to add new Diagram with autonum
	properties
*/
void QETProject::autoFolioNumberingNewFolios()
{
	emit addAutoNumDiagram();
}

/**
	@brief QETProject::autoFolioNumberingNewFolios
	@param from
	@param to
	@param autonum : used, index from selected tabs "from" and "to"
	rename folios with selected autonum
*/
void QETProject::autoFolioNumberingSelectedFolios(int from,
						  int to,
						  const QString& autonum){
	int total_folio = m_diagrams_list.count();
	DiagramContext project_wide_properties = m_project_properties;

	for (int i=from; i<=to; i++)
	{
		NumerotationContext nC = folioAutoNum(autonum);
		NumerotationContextCommands nCC = NumerotationContextCommands(nC);
		m_diagrams_list[i] -> border_and_titleblock.setFolio("%autonum");
		m_diagrams_list[i] -> border_and_titleblock.setFolioData(
					i + 1,
					total_folio,
					nCC.toRepresentedString(),
					project_wide_properties);
		m_diagrams_list[i] -> project() -> addFolioAutoNum(
					autonum,nCC.next());
		m_diagrams_list[i] -> update();
	}
}

/**
	@brief QETProject::toXml
	@return un document XML representant le projet
*/
QDomDocument QETProject::toXml(QHash<int, QByteArray> *kept_folios, QStringList *kept_parts)
{
		//With @p kept_parts, for a .qetz: a
		//folio not built is written as <diagram qetz-kept-part="..."/>, its
		//part and its rows copied by QetContainer::split()
		//(stage D4)
		//With @p kept_folios, a folio not built (lazy folios) is
		//written as a placeholder <diagram qet_kept_folio="n"/> and its text
		//as read from the file goes in kept_folios, for
		//spliceKeptFolios() -- when building it would write that same
		//text: the file is this version's and its database rows came from
		//it (m_kept_folios_trusted), and nothing a folio's saved items are
		//made from changed since: the folios' order and number, the
		//project's properties, report and cross-reference settings, a
		//symbol's definition (m_kept_folios_stale). Otherwise every folio is
		//built and written from its items. A folio whose own data changed
		//is built with the folios its links reach, as when it is shown:
		//their saved texts can show its data (a report, its partner's
		//folio label).
	const bool keep = unloadedFolioCount() && !m_kept_folios_stale
			&& ((kept_folios && m_kept_folios_trusted) || (kept_parts && m_kept_parts_trusted));
	if (keep) {
			//Partners built alone and linked or unlinked since: their folio
			//is not what was read
			//and symbols whose information was changed through the project
			//database while their folio was not built: built together
			//-- a .qetz writes those in its database instead, when it can
		const QSet<QUuid> edited = m_data_base.foliosWithUnbuiltEdits();
		QList<Diagram *> changed;
		for (Diagram *diagram : std::as_const(m_diagrams_list)) {
			if (diagram->isLoaded()) continue;
			bool need_build = !kept_parts;
			if (edited.contains(diagram->uuid()) && kept_parts)
				m_data_base.unbuiltEditsOnFolio(diagram->uuid(), &need_build);
			if (diagram->partialChanged() || (edited.contains(diagram->uuid()) && need_build))
				changed << diagram;
		}
		buildFolios(changed);
			//-- unless it changed only in what nothing is computed from (its
			//title, date, revision...): written from its items as read
			//(Diagram::deferredXmlWithHeader())
		for (Diagram *diagram : std::as_const(m_diagrams_list)) {
			if (!diagram->headerChanged()) continue;
			bool edits_need_build = false;
			if (!diagram->isLoaded())
				m_data_base.unbuiltEditsOnFolio(diagram->uuid(), &edits_need_build);
			if (diagram->isLoaded() || !diagram->headerChangeShowsOnly() || edits_need_build)
				buildFolioToShow(diagram);
		}
			//A table filled from the database saves the size its rows
			//give it, which any edit can change
		if (m_edited_since_open)
			for (Diagram *diagram : std::as_const(m_diagrams_list))
				if (!diagram->isLoaded() && unbuiltFolioHasTables(diagram))
					buildFolioToShow(diagram);
			//A built folio is written from its items, which need their
			//partners: a table saves the table before it, and in a .qet a
			//report the position of its partner. Build the folios of every
			//table chain, and for a .qet of every partner still waiting,
			//until none is. A symbol saves the partners it waits for itself
			//(Element::toXml()), and a .qetz keeps no partner's text (D3c):
			//saving a .qetz builds no partner (stage D5a)
		for (int built = -1 ; built != unloadedFolioCount() ; )
		{
			built = unloadedFolioCount();
			QSet<QUuid> waiting;
			QSet<Diagram *> chained;
			for (Diagram *diagram : builtFolios()) {
				if (kept_folios)
					for (Element *element : diagram->elements())
						waiting.unite(element->pendingLinks());
				for (Diagram *other : tableChainFolios(diagram))
					if (!other->isLoaded()) chained.insert(other);
			}
			buildFoliosHoldingSymbols(waiting);
			QList<Diagram *> chain_list;
			for (Diagram *diagram : std::as_const(m_diagrams_list))
				if (chained.contains(diagram) && !diagram->isLoaded())
					chain_list << diagram;
			buildFolios(chain_list);
		}
	} else {
		loadFolios();
	}
	m_waiting_link_targets = unloadedFolioCount() ? symbolsOnUnbuiltFolios() : QSet<QUuid>();
		//QET_CHECK_ELEMENT_INFO_STORE=1: say whether the stores of symbol
		//information, folio title blocks and wire properties agree with every placed symbol, folio and
		//wire, at each save. A test and debugging aid; it changes nothing.
	if (qEnvironmentVariableIntValue("QET_CHECK_ELEMENT_INFO_STORE") == 1) {
		const QStringList mismatches = m_data_base.elementInformationMismatches();
		qWarning().noquote() << "element information store:" << mismatches.size() << "differ"
							 << mismatches.mid(0, 5).join(QStringLiteral(", "));
		const QStringList folios = m_data_base.folioTitleBlockMismatches();
		qWarning().noquote() << "folio title block store:" << folios.size() << "differ"
							 << folios.mid(0, 5).join(QStringLiteral(", "));
		const QStringList wires = m_data_base.conductorPropertiesMismatches();
		qWarning().noquote() << "wire properties store:" << wires.size() << "differ"
							 << wires.mid(0, 5).join(QStringLiteral(", "));
	}
	// racine du projet
	QDomDocument xml_doc;
	QDomElement project_root = xml_doc.createElement("project");
	QetVersion::toXmlAttribute(project_root);
	if (project_title_.isEmpty())
	{
		// if project_title_is Empty add title from m_file_path
		// is for project name in Collectie
		setTitle(QFileInfo(m_file_path).completeBaseName());
	}
	project_root.setAttribute("title", project_title_);
		//Persist the project identity, so that the project keeps the same
		//uuid across save/load. Without it every load invents a new one, and
		//nothing outside the running instance (version control, a cloud or
		//key-value store, a lock...) can tell which project a file belongs to.
	project_root.setAttribute(QStringLiteral("uuid"), m_uuid.toString());
	xml_doc.appendChild(project_root);

	// titleblock templates, if any
	if (m_titleblocks_collection.templates().count()) {
		QDomElement titleblocktemplates_elmt = xml_doc.createElement("titleblocktemplates");
			//Sorted, because templates() returns QHash::keys() and Qt
			//randomises hash order per process. Writing them unsorted put the
			//<titleblocktemplate> children in a different order on every save,
			//which is what made a project holding more than one template save
			//irreproducibly.
		QStringList template_names = m_titleblocks_collection.templates();
		template_names.sort();
		for (const QString &template_name : std::as_const(template_names)) {
			QDomElement e = m_titleblocks_collection.getTemplateXmlDescription(template_name);
			titleblocktemplates_elmt.appendChild(xml_doc.importNode(e, true));
		}
		project_root.appendChild(titleblocktemplates_elmt);
	}

	// project-wide properties
	QDomElement project_properties = xml_doc.createElement("properties");
	writeProjectPropertiesXml(project_properties);
	project_root.appendChild(project_properties);

	// local, non-transmitted usage tracking (time spent on this project)
	writeUsageXml(project_root);
	writeWireHopsXml(project_root);
	writeSymbolTextsXml(project_root);
	writeWiringRulesXml(project_root);

	// Properties for news diagrams
	QDomElement new_diagrams_properties = xml_doc.createElement("newdiagrams");
	writeDefaultPropertiesXml(new_diagrams_properties);
	project_root.appendChild(new_diagrams_properties);

	// schemas

	qDebug() << "Export XML de" << m_diagrams_list.count() << "schemas";
	int order_num = 1;
	const QList<Diagram *> diagrams_list = m_diagrams_list;
	for(Diagram *diagram : diagrams_list)
	{
		qDebug() << QString("exporting diagram \"%1\""
					).arg(diagram -> title())
			 << "["
			 << diagram
			 << "]";
		if (keep && !diagram->isLoaded() && diagram->headerChanged()) {
			QDomElement xml_diagram = diagram->deferredXmlWithHeader(xml_doc);
				//Its symbols whose information changed while it was not built
			const QHash<QUuid, DiagramContext> edits = m_data_base.unbuiltEditsOnFolio(diagram->uuid());
			for (const QDomElement &list : QET::findInDomElement(xml_diagram, QStringLiteral("elements")))
				for (QDomElement element = list.firstChildElement(QStringLiteral("element")) ; !element.isNull() ;
					 element = element.nextSiblingElement(QStringLiteral("element"))) {
					const auto edit = edits.constFind(QUuid(element.attribute(QStringLiteral("uuid"))));
					if (edit == edits.constEnd()) continue;
					QDomElement information = xml_doc.createElement(QStringLiteral("elementInformations"));
					edit->toXml(information, QStringLiteral("elementInformation"));
					const QDomElement old_information = element.firstChildElement(QStringLiteral("elementInformations"));
					element.replaceChild(information, old_information);   //it has one (unbuiltEditsOnFolio())
				}
				//Its wires whose properties changed
			const QHash<QUuid, ConductorProperties> wire_edits = m_data_base.unbuiltWireEditsOnFolio(diagram->uuid());
			if (!wire_edits.isEmpty())
				for (const QDomElement &list : QET::findInDomElement(xml_diagram, QStringLiteral("conductors")))
					for (QDomElement wire = list.firstChildElement(QStringLiteral("conductor")) ; !wire.isNull() ;
						 wire = wire.nextSiblingElement(QStringLiteral("conductor"))) {
						const auto edit = wire_edits.constFind(QUuid(wire.attribute(QStringLiteral("uuid"))));
						if (edit == wire_edits.constEnd()) continue;
						for (const auto &attribute : edit->attributes())
							wire.setAttribute(attribute.first, attribute.second);
					}
			xml_diagram.setAttribute("order", order_num ++);
			project_root.appendChild(xml_diagram);
			continue;
		}
		if (keep && !diagram->isLoaded() && kept_parts) {
			auto placeholder = xml_doc.createElement(QStringLiteral("diagram"));
			placeholder.setAttribute(QStringLiteral("qetz-kept-part"), diagram->keptPart());
			placeholder.setAttribute(QStringLiteral("uuid"), diagram->uuid().toString());
				//Symbols whose information changed while the folio was
				//not built: the rows QetContainer::split() writes for them
			const QHash<QUuid, DiagramContext> edits = m_data_base.unbuiltEditsOnFolio(diagram->uuid());
			for (auto it = edits.cbegin() ; it != edits.cend() ; ++it) {
				auto symbol = xml_doc.createElement(QStringLiteral("qetz-edited-element"));
				symbol.setAttribute(QStringLiteral("uuid"), it.key().toString());
				auto information = xml_doc.createElement(QStringLiteral("elementInformations"));
				it.value().toXml(information, QStringLiteral("elementInformation"));
				symbol.appendChild(information);
				placeholder.appendChild(symbol);
			}
				//Its wires whose properties changed: what the .qetz database
				//holds of them (only that can change, QetScriptApi)
			const QHash<QUuid, ConductorProperties> wire_edits = m_data_base.unbuiltWireEditsOnFolio(diagram->uuid());
			for (auto it = wire_edits.cbegin() ; it != wire_edits.cend() ; ++it) {
				auto wire = xml_doc.createElement(QStringLiteral("qetz-edited-conductor"));
				wire.setAttribute(QStringLiteral("uuid"), it.key().toString());
				for (const auto &attribute : it.value().attributes())
					wire.setAttribute(attribute.first, attribute.second);
				placeholder.appendChild(wire);
			}
			kept_parts->append(diagram->keptPart());
			project_root.appendChild(placeholder);
			++order_num;
			continue;
		}
		if (keep && !diagram->isLoaded()) {
			auto placeholder = xml_doc.createElement(QStringLiteral("diagram"));
			placeholder.setAttribute(QStringLiteral("qet_kept_folio"), order_num);
			kept_folios->insert(order_num ++, diagram->keptText());
			project_root.appendChild(placeholder);
			continue;
		}
		QDomElement xml_diagram = diagram->toXml().documentElement();
		QDomNode xml_node = xml_doc.importNode(xml_diagram, true);

		QDomNode appended_diagram = project_root.appendChild(xml_node);
		appended_diagram.toElement().setAttribute("order", order_num ++);
	}

		//Write terminal strip to xml
	if (m_terminal_strip_vector.count())
	{
		m_project_properties_handler.terminalStripLayoutHandler().toXml(project_root);

		auto xml_strip = xml_doc.createElement(QStringLiteral("terminal_strips"));
		for (auto &strip : m_terminal_strip_vector) {
			xml_strip.appendChild(strip->toXml(xml_doc));
		}
		project_root.appendChild(xml_strip);
	}

		//Write the cables to xml. Their sections are written here too,
		//beside the cable they belong to, so a cable and everything drawn
		//for it are read back together and can never come apart.
	if (m_cables.count())
	{
		auto xml_cables = xml_doc.createElement(QStringLiteral("cables"));
		for (auto &cable : m_cables) {
			xml_cables.appendChild(cable->toXml(xml_doc));
		}
		project_root.appendChild(xml_cables);
	}

	// Write the elements collection.
	project_root.appendChild(m_elements_collection->root().cloneNode(true));

		//QET_CHECK_LINK_ROWS=1: say whether the project database's link rows
		//are the links each placed symbol saves, in its order, once the
		//folios are written. Builds nothing (lazy folios stay as they are):
		//a test and debugging aid; it changes nothing.
	if (qEnvironmentVariableIntValue("QET_CHECK_LINK_ROWS") == 1) {
		const QStringList links = m_data_base.linkRowMismatches();
		qWarning().noquote() << "link rows:" << links.size() << "differ"
							 << links.mid(0, 5).join(QStringLiteral(", "));
	}

	m_waiting_link_targets.clear();
	return(xml_doc);
}

/**
	@brief QETProject::spliceKeptFolios
	@return @p text, a project written by toXml() with kept folios, with
	each placeholder line replaced by the folio's text as it was read
*/
QString QETProject::spliceKeptFolios(QString text, const QHash<int, QByteArray> &kept_folios)
{
	for (auto it = kept_folios.cbegin() ; it != kept_folios.cend() ; ++it)
	{
		const QString placeholder = QStringLiteral("<diagram qet_kept_folio=\"%1\"/>").arg(it.key());
		const qsizetype at = text.indexOf(placeholder);
		if (at < 0) {
			continue;
		}
		const qsizetype line_start = text.lastIndexOf(QLatin1Char('\n'), at) + 1;
		qsizetype line_end = text.indexOf(QLatin1Char('\n'), at);
		line_end = line_end < 0 ? text.size() : line_end + 1;
		text.replace(line_start, line_end - line_start, QString::fromUtf8(it.value()));
	}
	return text;
}

/**
	Ferme le projet
*/
bool QETProject::close()
{
	return(true);
}

/**
	@brief QETProject::write
	Save the project in a file
	@see filePath()
	@see setFilePath()
	@return true if the project was successfully saved, else false
*/
/**
	@brief QETProject::isContainerPath
	@return true if @p path names a .qetz file: write() saves it zipped
*/
bool QETProject::isContainerPath(const QString &path)
{
	return path.endsWith(QLatin1String(".qetz"), Qt::CaseInsensitive);
}

/**
	@brief QETProject::openFileFilter
	@return the file-dialog filter for a project to open: .qet and .qetz
*/
QString QETProject::openFileFilter()
{
	return tr("QElectroTech project (*.qet *.qetz)");
}

/**
	@brief QETProject::writeContainer
	Write the project document @p project to @p path as a .qetz: the same
	document, in parts, zipped (#1440). The file is replaced only once
	the whole archive is written.
*/
bool QETProject::writeContainer(const QDomDocument &project, const QString &path, QString *error,
								const QHash<QString, QByteArray> &kept_from)
{
	QList<QetZip::Entry> entries = QetContainer::split(
				project,
				QStringLiteral("QElectroTech %1").arg(QetVersion::currentVersion().toString()),
				error, kept_from);
	return !entries.isEmpty() && QetZip::write(path, entries, error);
}

/**
	@brief QETProject::writeContainerFile
	Write this project to @p path as a .qetz
*/
bool QETProject::writeContainerFile(const QString &path, QString *error)
{
		//The folios not built, written as they were read, when that is
		//what building them would write (toXml()); every other folio built
	QStringList kept;
	const QDomDocument document = toXml(nullptr, &kept);
	return writeContainer(document, path, error,
						  kept.isEmpty() ? QHash<QString, QByteArray>() : m_container_files);
}

/**
	@brief QETProject::besideContainerPath
	@return the .qetz a project read from a .qet is saved to: the same
	name, beside it
*/
QString QETProject::besideContainerPath() const
{
	QString path = m_file_path;
	if (path.endsWith(QStringLiteral(".qet"), Qt::CaseInsensitive)) {
		path.chop(4);
	}
	return path + QStringLiteral(".qetz");
}

/**
	@brief QETProject::writeCopyForOlderVersions
	Write this project as a .qet at @p path, which a QElectroTech that
	reads no .qetz opens. The project keeps its own file and state.
*/
QETResult QETProject::writeCopyForOlderVersions(const QString &path)
{
	QHash<int, QByteArray> kept_folios;
	QDomDocument xml_project(toXml(&kept_folios));
	QString error_message;
	if (!QET::writeXmlText(spliceKeptFolios(xml_project.toString(4), kept_folios),
						   path, &error_message))
		return(error_message);
	return(QETResult());
}

/**
	@brief QETProject::rememberDiskState
	Remember the size and modification time of the project file, as this QET
	has just read or written it. changedOnDisk() compares against them.
*/
void QETProject::rememberDiskState()
{
	const QFileInfo info(m_file_path);
	m_disk_path = m_file_path;
	m_disk_size = info.exists() ? info.size() : -1;
	m_disk_mtime = info.exists() ? info.lastModified() : QDateTime();
}

/**
	@brief QETProject::writeTarget
	@return the file write() writes to: the .qetz beside a project read
	from a .qet, else the project's own file
*/
QString QETProject::writeTarget() const
{
	return m_opened_from_qet ? besideContainerPath() : m_file_path;
}

/**
	@brief QETProject::changedOnDisk
	@param when set to the modification time of the file now on disk
	@return true if the file write() would replace is not the one this QET
	last read or wrote: another user, another program or another QET saved
	it since. A file that does not exist yet, or one this QET never read
	or wrote (Save As to another name), is not a change.
*/
bool QETProject::changedOnDisk(QDateTime *when) const
{
	const QString target = writeTarget();
	if (target.isEmpty() || target != m_disk_path || m_disk_size < 0)
		return false;
	const QFileInfo info(target);
	if (!info.exists())
		return false;
	if (info.size() == m_disk_size && info.lastModified() == m_disk_mtime)
		return false;
	if (when)
		*when = info.lastModified();
	return true;
}

/**
	@brief QETProject::stampSaved
	Set the title block variables of the save (saveddate, saveddate-eu,
	saveddate-us, savedtime, savedfilename, savedfilepath) to now and to
	the file being written, when there is something new to save: the
	project changed since it was opened or last saved, or it is written to
	another file. Otherwise they keep what the file holds, so saving an
	unchanged project gives an identical file (F124).
	@return true if they were set
*/
bool QETProject::stampSaved()
{
	const bool changed = m_modified || m_undo_stack->index() != m_stamped_undo_index;
	if (!changed && m_file_path == m_stamped_path) {
		return false;
	}
	m_project_properties.addValue("saveddate",     QLocale::system().toString(QDate::currentDate(), QLocale::ShortFormat));
	m_project_properties.addValue("saveddate-eu",  QDate::currentDate().toString("dd-MM-yyyy"));
	m_project_properties.addValue("saveddate-us",  QDate::currentDate().toString("yyyy-MM-dd"));
	m_project_properties.addValue("savedtime",     QDateTime::currentDateTime().toString("HH:mm"));
	m_project_properties.addValue("savedfilename", QFileInfo(filePath()).completeBaseName());
	m_project_properties.addValue("savedfilepath", filePath());
	emit projectInformationsChanged(this);
	updateDiagramsFolioData();
	return true;
}

/**
	@brief QETProject::write
	Save the project in a file.
	If the file was changed on disk since this QET read or wrote it, nothing
	is written and the result says so: the interactive save asks the user
	first (ProjectView) and calls again with @p overwrite_changed; scripts,
	the command line, autosave and MCP have no one to ask and keep the
	refusal. Setting QET_OVERWRITE_CHANGED=1 in the environment overwrites
	without asking, for a script that means to.
	@param overwrite_changed write even if the file changed on disk
*/
QETResult QETProject::write(bool overwrite_changed)
{
		// this operation requires a filepath
	if (m_file_path.isEmpty())
		return(QString("unable to save project to file: no filepath was specified"));

		//Another session holds this file: it was opened read-only for that
	if (!m_locked_out_path.isEmpty() && m_locked_out_path == m_file_path)
		return(tr("%1 was opened read-only because someone else has it open. Use Save As to keep your changes in another file.").arg(m_file_path));

		//A project read from a .qet is saved as a .qetz beside it; the
		//.qet stays as it was (stage D1). If the .qetz cannot be written
		//(a read-only folder), the project stays the .qet it was.
	const QString opened_qet = m_opened_from_qet ? m_file_path : QString();
	if (m_opened_from_qet) {
		setFilePath(besideContainerPath());
	}
	const auto failed = [this, &opened_qet](const QString &error) {
		const QString message = tr("Cannot save %1: %2").arg(m_file_path, error);
		if (!opened_qet.isEmpty()) {
			setFilePath(opened_qet);
			m_opened_from_qet = true;
		}
		return(QETResult(message));
	};

		// If the project was opened read-only, only refuse when the target
		// really can't be written: an existing file that is not writable, or a
		// new file (e.g. "Save As" to another location) whose directory is not
		// writable.  A non-existent file reports isWritable() == false, so the
		// old check wrongly blocked saving a read-only project elsewhere.
	if (isReadOnly()) {
		const QFileInfo file_info(m_file_path);
		const bool can_write = file_info.exists()
			? file_info.isWritable()
			: QFileInfo(file_info.absolutePath()).isWritable();
		if (!can_write)
			return(QString("the file %1 was opened read-only and thus will not be written").arg(m_file_path));
	}

	if (!overwrite_changed && qEnvironmentVariableIntValue("QET_OVERWRITE_CHANGED") == 0) {
		QDateTime when;
		if (changedOnDisk(&when))
			return(failed(tr("someone else changed the file on disk at %1 since it was opened, "
							 "and saving would overwrite that. Save to another name, or "
							 "set QET_OVERWRITE_CHANGED=1 to overwrite it.")
						  .arg(QLocale::system().toString(when, QLocale::ShortFormat))));
	}

		//The file as it was goes among its previous versions first
		//(Settings > General > Projects); a copy that fails does not stop
		//the save
	QString backup_error;
	if (!PreviousVersions::keep(m_file_path, PreviousVersions::count(), &backup_error))
		qWarning().noquote() << "Previous version of" << m_file_path << "not kept:" << backup_error;

		//The saved date, time and path go into the file being written, and
		//only when there is something new to save: an unchanged project
		//saves to an identical file (F124, a project kept in git). A
		//failed write puts the previous values back.
	const DiagramContext stamped_before = m_project_properties;
	const bool stamped = stampSaved();
	const auto failed_stamped = [&](const QString &error) {
		if (stamped) {
			m_project_properties = stamped_before;
			emit projectInformationsChanged(this);
			updateDiagramsFolioData();
		}
		return failed(error);
	};

	QString error_message;
	if (isContainerPath(m_file_path))
	{
			//A container is written from the whole document: every folio
			//built (stage D4 writes it from the database)
		if (!writeContainerFile(m_file_path, &error_message))
			return(failed_stamped(error_message));
	}
	else
	{
			//Folios not built (lazy folios) written as read when that
			//is what building them would write (toXml())
		QHash<int, QByteArray> kept_folios;
		QDomDocument xml_project(toXml(&kept_folios));
		if (!QET::writeXmlText(spliceKeptFolios(xml_project.toString(4), kept_folios),
						   m_file_path, &error_message))
			return(failed_stamped(error_message));
	}

	rememberDiskState();

		// The project has just been written to a writable file (e.g. saved to
		// a new location with "Save As"), so it is no longer read-only.
	if (isReadOnly())
		setReadOnly(false);

		//The file may be a new one (Save As, a .qet saved as .qetz): the lock
		//follows it. A project that had none (new, or opened read-only for
		//another's lock) takes one now. Never stops the save: it is done.
	if (ProjectLock::mayTake()) {
		if (!m_lock) m_lock = std::make_unique<ProjectLock>();
		if (!m_lock->isHeld() || m_lock->projectPath() != m_file_path) {
			if (m_lock->isHeld()) m_lock->moveTo(m_file_path);
			else m_lock->acquire(m_file_path);
		}
	}

	m_stamped_path = m_file_path;
	m_stamped_undo_index = m_undo_stack->index();
	setModified(false);
	return(QETResult());
}

/**
	@brief QETProject::isReadOnly
	@return true si le projet est en mode readonly, false sinon
*/
bool QETProject::isReadOnly() const
{
	return(m_read_only && read_only_file_path_ == m_file_path);
}

/**
	@brief QETProject::setLock
	Hand the lock sidecar taken for this project's file to the project: it
	is released when the project is closed.
*/
void QETProject::setLock(std::unique_ptr<ProjectLock> lock)
{
	m_lock = std::move(lock);
}

/**
	@brief QETProject::setOpenedReadOnlyByLock
	Open this project read-only because another session has it: write()
	does not write to this file (Save As to another file is allowed).
*/
void QETProject::setOpenedReadOnlyByLock(bool by_lock)
{
	m_locked_out_path = by_lock ? m_file_path : QString();
	if (by_lock) setReadOnly(true);
}

/**
	@brief QETProject::setReadOnly
	Set this project to read only if read_only = true
	@param read_only
*/
void QETProject::setReadOnly(bool read_only)
{
	if (m_read_only != read_only)
	{
			//keep the file to which this project is read-only
		read_only_file_path_ = m_file_path;
		m_read_only = read_only;
		emit readOnlyChanged(this, read_only);
	}
}

/**
	@return true si le projet peut etre considere comme vide, c'est-a-dire :
	  - soit avec une collection embarquee vide
	  - soit avec uniquement des schemas consideres comme vides
	  - soit avec un titre de projet
*/
bool QETProject::isEmpty() const
{
	loadFolios(); //asks the folios whether they hold anything
	// si le projet a un titre, on considere qu'il n'est pas vide
	if (!project_title_.isEmpty()) return(false);

#if TODO_LIST
#pragma message("@TODO check if the embedded element collection is empty")
#endif
	//@TODO check if the embedded element collection is empty

	// compte le nombre de schemas non vides
	int pertinent_diagrams = 0;
	foreach(Diagram *diagram, m_diagrams_list) {
		if (!diagram -> isEmpty()) ++ pertinent_diagrams;
	}

	return(pertinent_diagrams > 0);
}

/**
	@brief QETProject::importElement
	Import the element represented by location
	to the embedded collection of this project
	@param location
	@return the location of the imported element, location can be null.
*/
ElementsLocation QETProject::importElement(ElementsLocation &location)
{
	//Location isn't an element or doesn't exist
	if (! (location.isElement() && location.exist()) ) {
		return ElementsLocation();
	}

	//Get the path where the element must be imported
	QString import_path;
	if (location.isFileSystem()) {
		import_path = "import/" % location.collectionPath(false);
	}
	else if (location.isProject()) {
		if (location.project() == this) {
			return location;
		}

		import_path = location.collectionPath(false);
	}

	//Element already exist in the embedded collection, we ask what to do to user
	if (m_elements_collection->exist(import_path)) {
		ElementsLocation existing_location(import_path, this);

		//existing_location and location have the same uuid, so it is the same element
		if (existing_location.uuid() == location.uuid()) {
			return existing_location;
		}

		ImportElementDialog ied;
		if (ied.exec() == QDialog::Accepted) {
			QET::Action action = ied.action();

			//Use the exisiting element
			if (action == QET::Ignore) {
				return existing_location;
			}
			//Erase the existing element, and use the newer instead
			else if (action == QET::Erase) {
				// Warn if the new element introduces slave contact groups
				QDomElement new_kind = location.xml().firstChildElement("kindInformations");
				if (!new_kind.firstChildElement("slaveContactGroups").isNull()) {
					QMessageBox::StandardButton answer = QET::QetMessageBox::warning(nullptr,
						tr("Modified contact system"),
						tr("The new element defines slave contact groups.\n"
						   "Existing slave elements will not be automatically assigned. "
						   "You will need to link the slaves manually and assign the "
						   "contact groups.\n"
						   "\n"
						   "Do you wish to continue?"),
						QMessageBox::Yes | QMessageBox::No,
						QMessageBox::Yes);
					if (answer == QMessageBox::No) {
						return ElementsLocation();
					}
				}
				ElementsLocation parent_loc = existing_location.parent();
				return m_elements_collection->copy(location, parent_loc);
			}
			//Add the new element with an other name.
			else if (action == QET::Rename) {
				int a = 0;
				QString parent_path = existing_location.parent().projectCollectionPath();
				QString name_ = existing_location.fileName();
				name_.remove(".elmt");

				ElementsLocation loc;
				do
				{
					a++;
					QString new_path = parent_path % "/" % name_ % QString::number(a) % ".elmt";
					loc = ElementsLocation (new_path);
				} while (loc.exist());

				ElementsLocation parent_loc = existing_location.parent();
				return m_elements_collection->copy(location, parent_loc, loc.fileName());
			}
			else {
				return ElementsLocation();
			}
		}
		else {
			return ElementsLocation();
		}
	}
	//Element doesn't exist in the collection, we just import it
	else {
		ElementsLocation loc(m_elements_collection->addElement(
						 location), this);

		if (!loc.exist()) {
			qDebug() << "failed to import location. "
				 << location;
			return ElementsLocation();
		}
		else {
			return loc;
		}
	}
}

/**
	@brief QETProject::integrateTitleBlockTemplate
	Integrate a title block template into this project.
	@param src_tbt The location of the title block template
	to be integrated into this project
	@param handler
	@return the name of the template after integration,
	or an empty QString if a problem occurred.
*/
QString QETProject::integrateTitleBlockTemplate(const TitleBlockTemplateLocation &src_tbt, MoveTitleBlockTemplatesHandler *handler) {
	TitleBlockTemplateLocation dst_tbt(src_tbt.name(), &m_titleblocks_collection);

	// check whether a TBT having the same name already exists within this project
	QString target_name = dst_tbt.name();
	while (m_titleblocks_collection.templates().contains(target_name))
	{
		QET::Action action = handler -> templateAlreadyExists(src_tbt, dst_tbt);
		if (action == QET::Retry) {
			continue;
		} else if (action == QET::Erase) {
			break;
		} else if (action == QET::Abort || action == QET::Ignore) {
			return(QString());
		} else if (action == QET::Rename) {
			target_name = handler -> nameForRenamingOperation();
		} else if (action == QET::Managed) {
			return(target_name);
		}
	}

	if (!m_titleblocks_collection.setTemplateXmlDescription(target_name, src_tbt.getTemplateXmlDescription()))
	{
		handler -> errorWithATemplate(src_tbt, tr("An error occurred during the template integration.", "error message"));
		target_name = QString();
	}
	return(target_name);
}

/**
	Permet de savoir si un element est utilise dans un projet
	@param location Emplacement d'un element
	@return true si l'element location est utilise sur au moins un des schemas
	de ce projet, false sinon
*/
bool QETProject::usesElement(const ElementsLocation &location) const
{
	foreach(Diagram *diagram, builtFolios()) {
		if (diagram -> usesElement(location)) {
			return(true);
		}
	}
	return(unbuiltFolioElements().contains(location));
}

/**
	@brief QETProject::folioXml
	@return the built folio @p folio written as saving writes it: its
	symbols keep the links to partners on folios not built yet
	(lazy folios), which Diagram::toXml() alone leaves out
*/
QDomDocument QETProject::folioXml(Diagram *folio)
{
	m_waiting_link_targets = unloadedFolioCount() ? symbolsOnUnbuiltFolios() : QSet<QUuid>();
	const QDomDocument xml = folio->toXml();
	m_waiting_link_targets.clear();
	return xml;
}

/**
	@brief QETProject::buildFoliosHoldingTables
	Build the folios not built yet (lazy folios) that hold a drawn
	table: what a list of the project's tables needs, and no more
*/
void QETProject::buildFoliosHoldingTables()
{
	QList<Diagram *> to_build;
	for (Diagram *d : std::as_const(m_diagrams_list))
		if (!d->isLoaded() && unbuiltFolioHasTables(d))
			to_build << d;
	if (to_build.isEmpty()) {
		return;
	}
	if (qEnvironmentVariableIntValue("QET_LAZY_FOLIOS_TRACE") == 1) {
		qWarning().noquote() << "lazy folios:" << to_build.size() << "folios built for their tables";
	}
	buildFolios(to_build);
}

/**
	@brief QETProject::folioHoldsTablesOrSymbols
	@return true if @p folio, built or not, has a drawn table or a symbol
*/
bool QETProject::folioHoldsTablesOrSymbols(Diagram *folio) const
{
	if (!folio->isLoaded()) {
		const QDomElement xml = folio->deferredXml();
		return !xml.elementsByTagName(QetGraphicsTableItem::xmlTagName()).isEmpty()
			|| !xml.elementsByTagName(QStringLiteral("element")).isEmpty();
	}
	for (QGraphicsItem *item : folio->items())
		if (item->type() == QetGraphicsTableItem::Type || item->type() == Element::Type)
			return true;
	return false;
}

/**
	@brief QETProject::unbuiltFolioHasTables
	@return true if @p folio, not built yet (lazy folios), has a drawn
	table, read from its XML
*/
bool QETProject::unbuiltFolioHasTables(Diagram *folio) const
{
	return !folio->deferredXml().elementsByTagName(QetGraphicsTableItem::xmlTagName()).isEmpty();
}

/**
	@brief QETProject::unbuiltFolioElements
	@return the locations of the symbols placed on the folios not built yet
	(lazy folios), each once, read from the project database once it
	holds those folios, else from the XML kept for them as building them
	reads it (Diagram::fromXml())
*/
QList<ElementsLocation> QETProject::unbuiltFolioElements() const
{
	QSet<QString> types;
	if (m_data_base.rowsFromDocument()) {
		QList<QUuid> unbuilt;
		for (Diagram *diagram : m_diagrams_list)
			if (!diagram->isLoaded()) unbuilt << diagram->uuid();
		for (const projectDataBase::FolioSymbol &symbol : m_data_base.symbolsOnFolios(unbuilt))
			types.insert(symbol.location);
	} else {
		for (Diagram *diagram : m_diagrams_list) {
			if (diagram->isLoaded()) {
				continue;
			}
			for (const QDomElement &element_xml :
				 QET::findInDomElement(diagram->deferredXml(),
									   QStringLiteral("elements"),
									   QStringLiteral("element"))) {
				types.insert(element_xml.attribute(QStringLiteral("type")));
			}
		}
	}
	QList<ElementsLocation> locations;
	auto *self = const_cast<QETProject *>(this);
	for (const QString &type : std::as_const(types)) {
		locations << (type.startsWith(QStringLiteral("embed://"))
					  ? ElementsLocation(type, self)
					  : ElementsLocation(type));
	}
	return locations;
}

/**
	@brief QETProject::unusedElements
	@return the list of unused element (exactly her location)
	An unused element, is an element present in the embedded collection but not present in a diagram of this project.
	Be aware that an element can be not present in a diagram,
	but managed by an undo command (delete an element), so an unused element can be used after an undo.
*/
QList<ElementsLocation> QETProject::unusedElements() const
{
	QList <ElementsLocation> unused_list;
	const QList<Diagram *> built = builtFolios();
	const QList<ElementsLocation> unbuilt = unbuiltFolioElements();

	foreach(ElementsLocation location, m_elements_collection->elementsLocation())
	{
		if (!location.isElement() || unbuilt.contains(location)) {
			continue;
		}
		bool used = false;
		for (Diagram *diagram : built) {
			if (diagram->usesElement(location)) {
				used = true;
				break;
			}
		}
		if (!used) {
			unused_list << location;
		}
	}

	return unused_list;
}

/**
	@param location Location of a title block template
	@return true if the provided template is used by at least one diagram
	within this project, false otherwise
*/
bool QETProject::usesTitleBlockTemplate(const TitleBlockTemplateLocation &location) {
	// a diagram can only use a title block template embedded within its parent project
	if (location.parentProject() != this) return(false);

		//The template is the folio's own data: folios() builds nothing
	foreach (Diagram *diagram, folios()) {
		if (diagram -> usesTitleBlockTemplate(location.name())) {
			return(true);
		}
	}
	return(false);
}

/**
	@brief QETProject::addNewDiagram
	Add a new diagram in project at position pos.
	This is pushed as an undoable AddDiagramCommand: use undoStack()->undo()
	to remove it again.
	@param pos
	@return the new created diagram
*/
Diagram *QETProject::addNewDiagram(int pos)
{
	if (isReadOnly()) {
		return(nullptr);
	}

	Diagram *diagram = new Diagram(this);

	diagram->border_and_titleblock.importBorder(defaultBorderProperties());
	diagram->border_and_titleblock.importTitleBlock(defaultTitleBlockProperties());
	diagram->defaultConductorProperties = defaultConductorProperties();

	m_undo_stack->push(new AddDiagramCommand(this, diagram, pos));
	return(diagram);
}

/**
	@brief QETProject::removeDiagram
	Remove diagram from project immediately (not undoable). UI code should
	generally go through ProjectView::removeDiagram() instead, which pushes
	an undoable RemoveDiagramCommand.
	@param diagram
*/
void QETProject::removeDiagram(Diagram *diagram)
{
	if (isReadOnly() ||
		!diagram || !m_diagrams_list.contains(diagram)) {
		return;
	}

	detachDiagram(diagram);
	diagram->deleteLater();
}

/**
	Gere le fait que l'ordre des schemas ait change
	@param old_index ancien indice du schema deplace
	@param new_index nouvel indice du schema deplace
	Si l'ancien ou le nouvel index est negatif ou superieur au nombre de schemas
	dans le projet, cette methode ne fait rien.
	Les index vont de 0 a "nombre de schemas - 1"
*/
void QETProject::diagramOrderChanged(int old_index, int new_index) {
	if (old_index < 0 || new_index < 0) return;

	int diagram_max_index = m_diagrams_list.size() - 1;
	if (old_index > diagram_max_index || new_index > diagram_max_index) return;

	m_diagrams_list.move(old_index, new_index);
	updateDiagramsFolioData();
	setModified(true);
	emit projectDiagramsOrderChanged(this, old_index, new_index);
}

/**
	Mark this project as modified and emit the projectModified() signal.
*/
void QETProject::setModified(bool modified) {
	if (modified) {
		m_backup_needed = true;
	}
	if (m_modified != modified) {
		m_modified = modified;
		emit projectModified(this, m_modified);
		emit projectInformationsChanged(this);
	}
}

/**
	@brief QETProject::readProjectXml
	Read and make the project from an xml description
	@param xml_project : the description of the project from an xml
*/
void QETProject::readProjectXml(QDomDocument &xml_project)
{
	QDomElement root_elmt = xml_project.documentElement();
	m_state = ProjectParsingRunning;

	//The roots of the xml document must be a "project" element
	if (root_elmt.tagName() == QLatin1String("project"))
	{
		//Normal opening mode
		m_project_qet_version = QetVersion::fromXmlAttribute(root_elmt);
		if (!m_project_qet_version.isNull())
		{
			if (QetVersion::currentVersion() < m_project_qet_version)
			{
				int ret = QET::QetMessageBox::warning(
							nullptr,
							tr("Warning",
							   "message box title"),
							tr("This document appears to have been saved with version %1\n"
							   " which is later than your version ! \n"
							   "You are currently using QElectroTech in version %2")
							.arg(root_elmt.attribute(QStringLiteral("version")), QetVersion::currentVersion().toString() %
							tr(".\n"
							   " It is then possible that the opening of all or part of this "
							   "document fails.\n"
							   "What do you wish to do ?"),
							   "message box content"),
							  QMessageBox::Open | QMessageBox::Cancel
							  );

				if (ret == QMessageBox::Cancel)
				{
					m_state = FileOpenDiscard;
					return;
				}
			}

				//Since QElectrotech 0.9 the compatibility with project made with
				//Qet 0.6 or lower is break;
			if (m_project_qet_version <= QetVersion::versionZeroDotSix())
			{
				auto ret = QET::QetMessageBox::warning(
							nullptr,
							tr("Warning ", "message box title"),
							tr("The project you are trying to open is partially "
							   "compatible with your version %1 of QElectroTech.\n")
							.arg(QetVersion::currentVersion().toString()) %
							tr("In order to make it fully compatible please open this same project with "
							   "version 0.8, or 0.80 of QElectroTech and save the project and open it "
							   "again with this version.\n"
							   "What do you wish to do ?"),
							   QMessageBox::Open | QMessageBox::Cancel
							  );

				if (ret == QMessageBox::Cancel)
				{
					m_state = FileOpenDiscard;
					return;
				}
			}
		}
		setTitle(root_elmt.attribute(QStringLiteral("title")));
	}
	else
	{
		m_state = ProjectParsingFailed;
		return;
	}

	m_data_base.blockSignals(true);
		//Blocking the signals is not enough : every table model built below
		//re-queries the database, and each of those queries used to trigger a
		//complete rebuild of it. The content being loaded is the same for all
		//of them, so a single rebuild once everything is in place is enough.
	m_data_base.setUpdateBlocked(true);

		//Load the project-wide properties
	readProjectPropertiesXml(xml_project);

		//Load the local, non-transmitted usage tracking
	readUsageXml(xml_project);
	readWireHopsXml(xml_project);
	readSymbolTextsXml(xml_project);
	readWiringRulesXml(xml_project);

		//Load the default properties for the new diagrams
	readDefaultPropertiesXml(xml_project);

		//load the embedded titleblock templates
	m_titleblocks_collection.fromXml(xml_project.documentElement());

		/* Timings of the phases below are logged so the cost of opening a
		 * project can be attributed. Reading the XML and building the objects
		 * is mostly independent of the Qt version, while refreshing the
		 * diagrams is graphics-scene work -- keeping them apart is what makes
		 * the numbers comparable between builds.
		 */
	QElapsedTimer phase_timer;
	phase_timer.start();

		//Load the embedded elements collection
	readElementsCollectionXml(xml_project);
	const qint64 elements_ms = phase_timer.restart();

		//Load the layout of the terminal strip (font, size of cells...),
		//before the diagrams because the terminal strip items use it
	m_project_properties_handler.terminalStripLayoutHandler().fromXml(xml_project.documentElement());

		//The stores of symbol information, folio title blocks and wire
		//properties are filled from the document before the folios are
		//built
	m_data_base.prefillFromDocument(xml_project);

		//Folios are left to be built when needed (lazyFolios()) only
		//when the database can be filled from the document: otherwise
		//filling it from the folios builds every one of them anyway, late
		//and more slowly than reading them now
	m_lazy_open = lazyFolios();
	if (m_lazy_open) {
		const QString why = projectDataBase::documentRefusal(xml_project);
		if (!why.isEmpty()) {
			qInfo().noquote() << "Project opened with every folio built:" << why;
			m_lazy_open = false;
		}
	}

		//Load the diagrams
	readDiagramsXml(xml_project);
	const qint64 diagrams_ms = phase_timer.restart();

		//Tie the elements to the numbering schemes they follow
	linkElementsToElementAutoNums();

		//Load the terminal strip
	readTerminalStripXml(xml_project);
	const qint64 strips_ms = phase_timer.restart();

		//Load the cables, after the diagrams their sections are drawn on
	readCableXml(xml_project);
	const qint64 cables_ms = phase_timer.restart();

		//Now that all are loaded we refresh content of the project.
	refresh();
	const qint64 refresh_ms = phase_timer.restart();

		//refresh() above resolved the folio report links for the first
		//time (refreshContents() -> initLink). The cable label pass at
		//the end of readCableXml() ran before that, while no element
		//knew its partners yet, so a core running through a report could
		//not be carried over there -- it was even cleared off the far
		//wire again. Run the pass once more now that the links exist:
		//it recomputes everything from the project as it now stands.
	CableManager::refreshLabels(this);

	m_data_base.blockSignals(false);
	m_data_base.setUpdateBlocked(false);
	m_data_base.updateDB(xml_project);
	m_data_base.endPrefill(unloadedFolioCount() > 0);
		//Saving may write the folios not built as they were read, when
		//building them would write the same: a file written by this
		//version, whose database rows came from it (toXml()). A .qet does
		//not say which program wrote it, and one with this version's
		//number can hold what a save rewrites (tst_freetextnonfinite,
		//tst_foliofreezeflags): only with QET_LAZY_SAVE_KEPT=1, until a
		//.qetz, whose manifest names its writer, does it
	m_kept_folios_trusted = qEnvironmentVariableIntValue("QET_LAZY_SAVE_KEPT") == 1
			&& unloadedFolioCount() > 0
			&& m_project_qet_version == QetVersion::currentVersion()
			&& m_data_base.rowsFromDocument();
	for (Diagram *diagram : std::as_const(m_diagrams_list))
		if (!diagram->isLoaded() && diagram->keptText().isEmpty())
			m_kept_folios_trusted = false;
	m_kept_parts_trusted = !m_folio_parts.isEmpty() && unloadedFolioCount() > 0
			&& m_project_qet_version == QetVersion::currentVersion()
			&& m_data_base.rowsFromDocument()
			&& QetContainerDb::currentFormat(m_container_files.value(QStringLiteral("project.sqlite")));
	for (Diagram *diagram : std::as_const(m_diagrams_list))
		if (!diagram->isLoaded() && diagram->keptPart().isEmpty())
			m_kept_parts_trusted = false;
	m_kept_folios_stale = false;
	m_edited_since_open = false;
	if (m_kept_folios_trusted || m_kept_parts_trusted)
		for (Diagram *diagram : std::as_const(m_diagrams_list))
			diagram->rememberHeader();
	if (qEnvironmentVariableIntValue("QET_LAZY_FOLIOS_TRACE") == 1)
		qWarning().noquote() << "lazy folios: opened with" << unloadedFolioCount() << "of" << m_diagrams_list.size() << "folios not built";
	if (qEnvironmentVariableIntValue("QET_CHECK_ELEMENT_INFO_STORE") == 1)
		qWarning().noquote() << "filled before building:" << m_data_base.prefillReport();
	const qint64 database_ms = phase_timer.elapsed();

	qInfo().nospace()
			<< "Project content built in "
			<< (elements_ms + diagrams_ms + strips_ms + cables_ms
				+ refresh_ms + database_ms) / 1000.0
			<< " seconds (elements collection " << elements_ms / 1000.0
			<< ", diagrams " << diagrams_ms / 1000.0
			<< ", terminal strips " << strips_ms / 1000.0
			<< ", cables " << cables_ms / 1000.0
			<< ", refresh " << refresh_ms / 1000.0
			<< ", database " << database_ms / 1000.0 << ")";

	m_state = Ok;
}

/**
	@brief QETProject::lazyFolios
	@see the declaration
*/
bool QETProject::lazyFolios()
{
	return qEnvironmentVariableIntValue("QET_EAGER_FOLIOS") != 1;
}

/**
	@brief QETProject::readDiagramsXml
	Load the diagrams from the xml description of the project.
	Note a project can have 0 diagram
	@param xml_project
*/
void QETProject::readDiagramsXml(QDomDocument &xml_project)
{
#if TODO_LIST
#pragma message("@TODO try to solve a weird bug (dialog is black) since port to Qt5 with the DialogWaiting")
#endif
	//@TODO try to solve a weird bug (dialog is black) since port to Qt5 with the DialogWaiting
	//show DialogWaiting
	DialogWaiting *dlgWaiting = nullptr;
	if(DialogWaiting::hasInstance())
	{
		dlgWaiting = DialogWaiting::instance();
		dlgWaiting -> setModal(true);
		dlgWaiting -> show();
		dlgWaiting -> setTitle(tr("<p align=\"center\"><b>Opening the project "
					  "...</b><br/>Creation of sheets</p>"));
	}

	//Search the diagrams in the project
	QDomNodeList diagram_nodes = xml_project.elementsByTagName(QStringLiteral("diagram"));

		//Every symbol and wire uuid the file already carries, on any folio,
		//before a folio derives one for an item saved without: see
		//derivedItemUuid().
	for (const QString &tag : {QStringLiteral("element"), QStringLiteral("conductor")}) {
		const QDomNodeList nodes = xml_project.elementsByTagName(tag);
		for (int i = 0; i < nodes.size(); ++i) {
			const QUuid saved(nodes.at(i).toElement().attribute(QStringLiteral("uuid")));
			if (!saved.isNull())
				m_saved_item_uuids.insert(saved);
		}
	}

		//Symbols sharing a uuid (copies made by older versions, F100): the
		//project database, links and wires name a symbol by its uuid, so
		//each copy but one gets its own (renewedSharedUuid()). The copy
		//that keeps it is the first one linked to another symbol (a
		//partner's link names the uuid), else the first. The folios with
		//a copy to renew are built now, also with lazy folios, so the
		//whole project has one uuid per symbol from the start.
	m_shared_uuid_keepers.clear();
	QSet<int> build_now;
	{
		QHash<QUuid, QList<std::tuple<int, int, bool>>> copies;
		for (int i = 0 ; i < diagram_nodes.length() ; ++i) {
			const QDomElement elements = diagram_nodes.at(i).toElement()
					.firstChildElement(QStringLiteral("elements"));
			QHash<QUuid, int> ordinal;
			for (QDomElement e = elements.firstChildElement(QStringLiteral("element"));
				 !e.isNull() ; e = e.nextSiblingElement(QStringLiteral("element"))) {
				const QUuid uuid(e.attribute(QStringLiteral("uuid")));
				if (uuid.isNull()) continue;
				const bool linked = !e.firstChildElement(QStringLiteral("links_uuids"))
						.firstChildElement(QStringLiteral("link_uuid")).isNull();
				copies[uuid].append({i, ordinal[uuid]++, linked});
			}
		}
		for (auto it = copies.cbegin() ; it != copies.cend() ; ++it) {
			if (it.value().size() < 2) continue;
			auto keeper = it.value().first();
			for (const auto &copy : it.value())
				if (std::get<2>(copy)) { keeper = copy; break; }
			m_shared_uuid_keepers.insert(it.key(), {std::get<0>(keeper), std::get<1>(keeper)});
			for (const auto &copy : it.value())
				if (copy != keeper) build_now.insert(std::get<0>(copy));
		}
	}

	if(dlgWaiting)
		dlgWaiting->setProgressBarRange(0, diagram_nodes.length()*3);

		//Where each line of the file starts, for a folio's text: lines
		//[its start, the next element's start) (QETProject::toXml())
	QList<qsizetype> line_starts;
	if (m_lazy_open && !m_opened_content.isEmpty()) {
		line_starts << 0;
		for (qsizetype i = 0 ; i < m_opened_content.size() ; ++i)
			if (m_opened_content.at(i) == '\n') line_starts << i + 1;
	}
	auto folio_text = [this, &line_starts](const QDomElement &folio) {
		const int first = folio.lineNumber();
		const QDomElement next = folio.nextSiblingElement();
		int last = next.isNull() ? -1 : next.lineNumber();
		if (last < 0) {
				//The last folio: up to the project's closing tag
			const qsizetype end = m_opened_content.lastIndexOf("</project>");
			last = end < 0 ? -1 : int(std::upper_bound(line_starts.cbegin(), line_starts.cend(), end)
									  - line_starts.cbegin());
		}
		if (first < 1 || last <= first || last > line_starts.size()) {
			return QByteArray();
		}
		const QByteArray text = m_opened_content.mid(line_starts.at(first - 1),
													 line_starts.at(last - 1) - line_starts.at(first - 1));
			//One folio, whole: else it is built to be saved
		const QByteArray trimmed = text.trimmed();
		if (!trimmed.startsWith("<diagram ") || text.count("<diagram ") != 1
				|| !(trimmed.endsWith("</diagram>") || (trimmed.endsWith("/>") && !text.contains("</diagram>")))) {
			return QByteArray();
		}
		return text;
	};

	for (int i = 0 ; i < diagram_nodes.length() ; ++ i)
	{
		if(dlgWaiting)
			dlgWaiting->setProgressBar(i+1);

		if (diagram_nodes.at(i).isElement())
		{
			QDomElement diagram_xml_element = diagram_nodes
					.at(i)
					.toElement();
			auto diagram = new Diagram(this);
			m_diagrams_list << diagram;

			connect(&diagram->border_and_titleblock, &BorderTitleBlock::needFolioData,
					this, &QETProject::updateDiagramsFolioData);
			connect(diagram, &Diagram::usedTitleBlockTemplateChanged,
					this, &QETProject::usedTitleBlockTemplateChanged);

			m_deferring_folios = m_lazy_open && !build_now.contains(i);
			diagram->initFromXml(diagram_xml_element);
			m_deferring_folios = false;
			if (!diagram->isLoaded() && !line_starts.isEmpty())
				diagram->setKeptText(folio_text(diagram_xml_element));
			if (!diagram->isLoaded() && m_folio_parts.size() == diagram_nodes.length())
				diagram->setKeptPart(m_folio_parts.at(i));
			m_data_base.placeFolio(diagram);
			if(dlgWaiting)
				dlgWaiting->setDetail(diagram->title());
		}
	}

	m_shared_uuid_keepers.clear();
	updateDiagramsFolioData();

		//Initialise links between elements in this project
		//and refresh the text of conductor
	if(dlgWaiting)
	{
		dlgWaiting->setTitle( tr("<p align=\"center\"><b>Opening the project "
								 "...</b><br/>Setting up cross references</p>"));
	}
}

/**
	@brief QETProject::readElementsCollectionXml
	Load the diagrams from the xml description of the project
	@param xml_project : the xml description of the project
*/
void QETProject::readElementsCollectionXml(QDomDocument &xml_project)
{
		//Get the embedded elements collection of the project
	QDomNodeList collection_roots = xml_project.elementsByTagName(QStringLiteral("collection"));
	QDomElement collection_root;

	if (!collection_roots.isEmpty())
	{
			//Only the first found collection is take
		collection_root = collection_roots.at(0).toElement();
	}
		//Make an empty collection
	if (collection_root.isNull())  {
		m_elements_collection = new XmlElementCollection(this);
	}
		//Read the collection
	else {
		m_elements_collection = new XmlElementCollection(collection_root, this);
	}
}

/**
	@brief QETProject::readProjectPropertiesXml
	Load project properties from the XML description of the project
	@param xml_project : the xml description of the project
*/
void QETProject::readProjectPropertiesXml(QDomDocument &xml_project)
{
	for (const auto &dom_elmt : QET::findInDomElement(xml_project.documentElement(), QStringLiteral("properties")))
		m_project_properties.fromXml(dom_elmt);
}

/**
	@brief QETProject::readUsageXml
	Load the local, non-transmitted usage tracking (time spent on this
	project) from the XML description of the project.
	@param xml_project : the xml description of the project
*/
void QETProject::readUsageXml(QDomDocument &xml_project)
{
	m_project_properties_handler.usageTracker().fromXml(xml_project.documentElement());
}

/**
	@brief QETProject::readWireHopsXml
	Read the <wire_crossings> element of the project, if any.
	A project without it draws no hop.
	@param xml_project : the xml description of the project
*/
void QETProject::readWireHopsXml(QDomDocument &xml_project)
{
	const QDomElement crossings = xml_project.documentElement()
			.firstChildElement(QStringLiteral("wire_crossings"));
	m_wire_hops = crossings.isNull()
			? WireHops::Mode::None
			: WireHops::fromString(crossings.attribute(QStringLiteral("hop")));
}

/**
	@brief QETProject::readSymbolTextsXml
	Read the <symbol_texts> element of the project, if any. A project
	without it was saved before the setting existed, or with it off, and
	keeps the texts of its turned symbols turned.
	@param xml_project : the xml description of the project
*/
void QETProject::readSymbolTextsXml(QDomDocument &xml_project)
{
	const QDomElement texts = xml_project.documentElement()
			.firstChildElement(QStringLiteral("symbol_texts"));
	m_upright_symbol_texts = !texts.isNull()
			&& texts.attribute(QStringLiteral("upright")) == QLatin1String("true");
}

/**
	@brief QETProject::readWiringRulesXml
	Read the <wiring_rules> element of the project, if any.
	A project without it sets no rule.
	@param xml_project : the xml description of the project
*/
void QETProject::readWiringRulesXml(QDomDocument &xml_project)
{
	m_wiring_rules = WiringRules::fromXml(xml_project.documentElement());
}

/**
	@brief QETProject::readDefaultPropertiesXml
	load default properties for new diagram, found in the xml of this project
	or by default find in the QElectroTech global conf
	@param xml_project : the xml description of the project
*/
void QETProject::readDefaultPropertiesXml(QDomDocument &xml_project)
{
		// Find xml element where is stored properties for new diagram
	QDomNodeList newdiagrams_nodes = xml_project.elementsByTagName(QStringLiteral("newdiagrams"));
	if (newdiagrams_nodes.isEmpty()) return;

	QDomElement newdiagrams_elmt = newdiagrams_nodes.at(0).toElement();

		// By default, use value find in the global conf of QElectroTech
	default_border_properties_	   = BorderProperties::    defaultProperties();
	default_titleblock_properties_ = TitleBlockProperties::defaultProperties();
	default_conductor_properties_  = ConductorProperties:: defaultProperties();
	m_default_report_properties	   = ReportProperties::    defaultProperties();
	m_default_xref_properties	   = XRefProperties::      defaultProperties();

		//Read values indicate in project
	QDomElement border_elmt, titleblock_elmt, conductors_elmt, report_elmt, xref_elmt, conds_autonums, folio_autonums, element_autonums, cable_autonums, guides_elmt;

	for (QDomNode child = newdiagrams_elmt.firstChild() ; !child.isNull() ; child = child.nextSibling())
	{
		QDomElement child_elmt = child.toElement();
		if (child_elmt.isNull()) continue;

		if (child_elmt.tagName() == QLatin1String("border"))
			border_elmt = child_elmt;
		else if (child_elmt.tagName() == QLatin1String("inset"))
			titleblock_elmt = child_elmt;
		else if (child_elmt.tagName() == QLatin1String("conductors"))
			conductors_elmt = child_elmt;
		else if (child_elmt.tagName() == QLatin1String("report"))
			report_elmt = child_elmt;
		else if (child_elmt.tagName() == QLatin1String("xrefs"))
			xref_elmt = child_elmt;
		else if (child_elmt.tagName() == QLatin1String("conductors_autonums"))
			conds_autonums = child_elmt;
		else if (child_elmt.tagName()== QLatin1String("folio_autonums"))
			folio_autonums = child_elmt;
		else if (child_elmt.tagName()== QLatin1String("element_autonums"))
			element_autonums = child_elmt;
		else if (child_elmt.tagName()== QLatin1String("cable_autonums"))
			cable_autonums = child_elmt;
		else if (child_elmt.tagName() == QLatin1String("guides"))
			guides_elmt = child_elmt;
	}

		// size, titleblock, conductor, report, conductor autonum, folio autonum, element autonum
	if (!border_elmt.isNull())	   default_border_properties_.fromXml(border_elmt);
	if (!titleblock_elmt.isNull()) default_titleblock_properties_.fromXml(titleblock_elmt);
	if (!conductors_elmt.isNull()) default_conductor_properties_.fromXml(conductors_elmt);
	if (!report_elmt.isNull())	   setDefaultReportProperties(report_elmt.attribute(QStringLiteral("label")));
	if (!xref_elmt.isNull())
	{
		for (const auto &elmt : QET::findInDomElement(xref_elmt, QStringLiteral("xref")))
		{
			XRefProperties xrp;
			xrp.fromXml(elmt);
			m_default_xref_properties.insert(elmt.attribute(QStringLiteral("type")), xrp);
		}
	}
	if (!conds_autonums.isNull())
	{
		m_current_conductor_autonum = conds_autonums.attribute(QStringLiteral("current_autonum"));
		m_freeze_new_conductors = conds_autonums.attribute(QStringLiteral("freeze_new_conductors")) == QLatin1String("true");
		m_auto_break_conductor = conds_autonums.attribute(QStringLiteral("auto_break_conductors")) == QLatin1String("true");
		for (auto elmt : QET::findInDomElement(conds_autonums, QStringLiteral("conductor_autonum")))
		{
			NumerotationContext nc;
			nc.fromXml(elmt);
			m_conductor_autonum.insert(elmt.attribute("title"), nc);
		}
	}
	if (!folio_autonums.isNull())
	{
		for (auto elmt : QET::findInDomElement(folio_autonums, QStringLiteral("folio_autonum")))
		{
			NumerotationContext nc;
			nc.fromXml(elmt);
			m_folio_autonum.insert(elmt.attribute(QStringLiteral("title")), nc);
		}
	}
	if (!element_autonums.isNull())
	{
		m_current_element_autonum = element_autonums.attribute(QStringLiteral("current_autonum"));
		m_freeze_new_elements = element_autonums.attribute(QStringLiteral("freeze_new_elements")) == QLatin1String("true");
		for (auto elmt : QET::findInDomElement(element_autonums, QStringLiteral("element_autonum")))
		{
			NumerotationContext nc;
			nc.fromXml(elmt);
			const QString title = elmt.attribute(QStringLiteral("title"));
			QUuid id(elmt.attribute(QStringLiteral("id")));
			if (id.isNull() || !elementAutoNumTitle(id).isEmpty()) {
					//Saved before schemes had an id: derive one, the
					//same on every load of the file.
				m_legacy_element_autonums = true;
					//Of this project: another project may have a numbering of the
					//same name, which is not the same numbering
				id = derivedItemUuid(QStringLiteral("element_autonum"),
									 m_uuid.toString() + QLatin1Char('\n') + title);
			}
			m_element_autonum.insert(title, nc);
			m_element_autonum_id.insert(title, id);
		}
			//The id is authoritative, the title is kept for older versions
		const QString current_title = elementAutoNumTitle(
					QUuid(element_autonums.attribute(QStringLiteral("current_autonum_id"))));
		if (!current_title.isEmpty()) {
			m_current_element_autonum = current_title;
		}
	}
	if (!cable_autonums.isNull())
	{
		m_current_cable_autonum = cable_autonums.attribute(QStringLiteral("current_autonum"));
			//Which axis the cables are numbered along. A file written
			//before this existed has no say in it and stays with the
			//axis the terminals have always started from.
		m_cable_axis_x_first = cable_autonums.attribute(
			QStringLiteral("axis_priority"), QStringLiteral("x"))
			!= QLatin1String("y");
			//Whether the numbering question was already answered with
			//no in this project. A file which does not hold the answer
			//yet asks again rather than staying silent: it is his own
			//answer per project which is kept, not a default saying no.
		m_cable_ask_numbering = cable_autonums.attribute(
			QStringLiteral("ask_numbering_rule"), QStringLiteral("true"))
			!= QLatin1String("false");
		for (auto elmt : QET::findInDomElement(cable_autonums, QStringLiteral("cable_autonum")))
		{
			NumerotationContext nc;
			nc.fromXml(elmt);
			if (elmt.attribute(QStringLiteral("title")).isEmpty()) continue;
			m_cable_autonum.insert(elmt.attribute(QStringLiteral("title")), nc);
		}
			//A file written by hand, or written before this rule existed,
			//may name a rule which is not in the list: the numbering must
			//never be left pointing at something which is not there.
		if (!m_cable_autonum.contains(m_current_cable_autonum)) {
			m_current_cable_autonum = m_cable_autonum.isEmpty()
					? QString() : m_cable_autonum.keys().first();
		}
	}
	// Read guides from XML (if missing, e.g. in old projects, list stays empty)
	m_default_guides.clear();

	if (!guides_elmt.isNull()) {
		for (auto elmt : QET::findInDomElement(guides_elmt, QStringLiteral("guide"))) {
			GuideProperties g;
			g.orientation = elmt.attribute(QStringLiteral("orientation"), QStringLiteral("0")).toInt();
			g.position = elmt.attribute(QStringLiteral("position"), QStringLiteral("0.0")).toDouble();
			g.color = QColor(elmt.attribute(QStringLiteral("color"), QStringLiteral("#ff0000")));
			m_default_guides.append(g);
		}
	}
}

/**
 * @brief QETProject::readTerminalStripXml
 * Read the terminal strips of this project
 * @param xml_project
 */
void QETProject::readTerminalStripXml(const QDomDocument &xml_project)
{
	auto xml_elmt = xml_project.documentElement();
	auto xml_strips = xml_elmt.firstChildElement(QStringLiteral("terminal_strips"));
	if (!xml_strips.isNull())
	{
		for (auto xml_strip : QETXML::findInDomElement(xml_strips, TerminalStrip::xmlTagName()))
		{
			auto terminal_strip = new TerminalStrip(this);
			terminal_strip->fromXml(xml_strip);
			addTerminalStrip(terminal_strip);
		}
	}
}

/**
 * @brief QETProject::readCableXml
 * Read the cables of the project, and draw a section for each of them on
 * the folio it belongs to.
 *
 * The diagrams have already been read at this point, which is what makes
 * it possible to put the sections back where they were: a folio is
 * identified by its uuid, so reordering or renumbering pages never moves
 * a cable onto the wrong one.
 * @param xml_project
 */
void QETProject::readCableXml(const QDomDocument &xml_project)
{
	auto xml_elmt = xml_project.documentElement();
	auto xml_cables = xml_elmt.firstChildElement(QStringLiteral("cables"));
	if (xml_cables.isNull()) return;

	for (auto xml_cable = xml_cables.firstChildElement(QStringLiteral("cable"));
		 !xml_cable.isNull();
		 xml_cable = xml_cable.nextSiblingElement(QStringLiteral("cable")))
	{
		auto cable = new Cable(this);
		cable->fromXml(xml_cable);
		addCable(cable);

		for (const CablePartData &part_data : cable->parts())
		{
			Diagram *diagram = diagramByUuid(part_data.diagram);
			if (!diagram)
			{
					//The folio this section was drawn on has been removed:
					//the section goes with it, and so do the cores which
					//were only ever shown on it.
				cable->removePart(part_data.uuid);
				continue;
			}

			auto part = new CablePart(part_data);
			part->setCable(cable);
			diagram->addItem(part);
		}
	}

		//The file holds the generated cable field of every conductor as
		//well as the cables themselves; working it out again here makes
		//sure the two agree even if a file was edited by hand.
	CableManager::refreshLabels(this);
}

/**
	Export project properties under the \a xml_element XML element.
*/
void QETProject::writeProjectPropertiesXml(QDomElement &xml_element) {
	m_project_properties.toXml(xml_element);
}

/**
	@brief QETProject::writeUsageXml
	Export the local, non-transmitted usage tracking (time spent on this
	project) as a <usage> child of \a xml_element.
*/
void QETProject::writeUsageXml(QDomElement &xml_element) {
	m_project_properties_handler.usageTracker().toXml(xml_element);
}

/**
	@brief QETProject::writeWireHopsXml
	Export which wire of a crossing hops as a <wire_crossings> child of
	\a xml_element. Written only when hops are on, so a project that never
	used them saves exactly as before.
*/
void QETProject::writeWireHopsXml(QDomElement &xml_element)
{
	if (m_wire_hops == WireHops::Mode::None) {
		return;
	}
	QDomElement crossings = xml_element.ownerDocument()
			.createElement(QStringLiteral("wire_crossings"));
	crossings.setAttribute(QStringLiteral("hop"), WireHops::toString(m_wire_hops));
	xml_element.appendChild(crossings);
}

/**
	@brief QETProject::writeSymbolTextsXml
	Export whether the texts of turned symbols stay horizontal, as a
	<symbol_texts> child of \a xml_element. Written only when on, so a
	project with it off saves exactly as before.
*/
void QETProject::writeSymbolTextsXml(QDomElement &xml_element)
{
	if (!m_upright_symbol_texts) {
		return;
	}
	QDomElement texts = xml_element.ownerDocument()
			.createElement(QStringLiteral("symbol_texts"));
	texts.setAttribute(QStringLiteral("upright"), QStringLiteral("true"));
	xml_element.appendChild(texts);
}

/**
	@brief QETProject::writeWiringRulesXml
	Export the project's wiring rules as a <wiring_rules> child of
	\a xml_element, only when one is set.
*/
void QETProject::writeWiringRulesXml(QDomElement &xml_element)
{
	WiringRules::toXml(m_wiring_rules, xml_element);
}

/**
	@brief QETProject::writeDefaultPropertiesXml
	Export all defaults properties used by a new diagram and his content
	size of border
	content of titleblock
	default conductor
	defaut folio report
	default Xref
	@param xml_element : xml element to use to store default properties.
*/
void QETProject::writeDefaultPropertiesXml(QDomElement &xml_element)
{
	QDomDocument xml_document = xml_element.ownerDocument();

	// export size of border
	QDomElement border_elmt = xml_document.createElement("border");
	default_border_properties_.toXml(border_elmt);
	xml_element.appendChild(border_elmt);

	// export content of titleblock
	QDomElement titleblock_elmt = xml_document.createElement("inset");
	default_titleblock_properties_.toXml(titleblock_elmt);
	xml_element.appendChild(titleblock_elmt);

	// exporte default conductor
	QDomElement conductor_elmt = xml_document.createElement("conductors");
	default_conductor_properties_.toXml(conductor_elmt);
	xml_element.appendChild(conductor_elmt);

	// export default report properties
	QDomElement report_elmt = xml_document.createElement("report");
	report_elmt.setAttribute("label", defaultReportProperties());
	xml_element.appendChild(report_elmt);

		// export default XRef properties
	QDomElement xrefs_elmt = xml_document.createElement("xrefs");
		//Sorted, because defaultXRefProperties() is a QHash and its key order
		//is randomised per process. Writing it unsorted made two saves of an
		//unchanged project differ only in the order of these <xref> children,
		//so a save was not reproducible and diffing two saved files showed
		//spurious changes.
	QStringList xref_keys = defaultXRefProperties().keys();
	xref_keys.sort();
	for (QString &key : xref_keys)
	{
		auto xrp = defaultXRefProperties(key);
		xrp.setKey(key);
		auto xref_dom = xrp.toXml(xml_document);
		xrefs_elmt.appendChild(xref_dom);
	}
	xml_element.appendChild(xrefs_elmt);

	//Export Conductor Autonums
	QDomElement conductor_autonums = xml_document.createElement("conductors_autonums");
	conductor_autonums.setAttribute("current_autonum", m_current_conductor_autonum);
	conductor_autonums.setAttribute("freeze_new_conductors", m_freeze_new_conductors ? "true" : "false");
	conductor_autonums.setAttribute("auto_break_conductors", m_auto_break_conductor ? "true" : "false");
		//Sorted for the same reason as the xrefs above: these three
		//collections are QHash, whose key order is randomised per process,
		//so an unsorted write reorders these children on every save.
	QStringList conductor_autonum_keys = conductorAutoNum().keys();
	conductor_autonum_keys.sort();
	for (const QString &key : std::as_const(conductor_autonum_keys)) {
	QDomElement conductor_autonum = conductorAutoNum(key).toXml(xml_document, "conductor_autonum");
		if (key != "" && conductorAutoNumFormula(key) != "") {
			conductor_autonum.setAttribute("title", key);
			conductor_autonum.setAttribute("formula", conductorAutoNumFormula(key));
			conductor_autonums.appendChild(conductor_autonum);
		}
	}
	xml_element.appendChild(conductor_autonums);

	//Export Folio Autonums
	QDomElement folio_autonums = xml_document.createElement("folio_autonums");
	QStringList folio_autonum_keys = folioAutoNum().keys();
	folio_autonum_keys.sort();
	for (const QString &key : std::as_const(folio_autonum_keys)) {
	QDomElement folio_autonum = folioAutoNum(key).toXml(xml_document, "folio_autonum");
		folio_autonum.setAttribute("title", key);
		folio_autonums.appendChild(folio_autonum);
	}
	xml_element.appendChild(folio_autonums);

	//Export Element Autonums
	QDomElement element_autonums = xml_document.createElement("element_autonums");
	element_autonums.setAttribute("current_autonum", m_current_element_autonum);
	if (!elementAutoNumId(m_current_element_autonum).isNull()) {
		element_autonums.setAttribute("current_autonum_id",
									  elementAutoNumId(m_current_element_autonum).toString());
	}
	element_autonums.setAttribute("freeze_new_elements", m_freeze_new_elements ? "true" : "false");
	QStringList element_autonum_keys = elementAutoNum().keys();
	element_autonum_keys.sort();
	for (const QString &key : std::as_const(element_autonum_keys)) {
	QDomElement element_autonum = elementAutoNum(key).toXml(xml_document, "element_autonum");
		if (key != "" && elementAutoNumFormula(key) != "") {
			element_autonum.setAttribute("title", key);
			element_autonum.setAttribute("id", elementAutoNumId(key).toString());
			element_autonum.setAttribute("formula", elementAutoNumFormula(key));
			element_autonums.appendChild(element_autonum);
		}
	}
	xml_element.appendChild(element_autonums);

	//Export Cable Autonum -- one rule, but stored exactly like the others
	//so it travels with the project and survives a re-save untouched.
	//
	//Only when it holds something, though. An element which is not there
	//reads back as exactly the defaults a project which never touched
	//cable numbering already has, so writing it every time put
	//<cable_autonums> into every file the program saved -- into the
	//example projects and into projects without a single cable in them.
	//
	//That is not the same as writing it only for projects with cables,
	//which would cost data: a rule named before the first cable is
	//drawn, an axis chosen before it, and the numbering question
	//answered with no are answers only this element can carry.
	QDomElement cable_autonums = xml_document.createElement("cable_autonums");
	cable_autonums.setAttribute("current_autonum", m_current_cable_autonum);
	cable_autonums.setAttribute("axis_priority",
								m_cable_axis_x_first ? "x" : "y");
		//Only the no is written down: a project which has not answered
		//the numbering question yet asks again, so silence in the file
		//means asking rather than holding back.
	if (!m_cable_ask_numbering) {
		cable_autonums.setAttribute("ask_numbering_rule", "false");
	}
	int written_cable_rules = 0;
	QStringList cable_autonum_keys = cableAutoNum().keys();
	cable_autonum_keys.sort();
	for (const QString &key : std::as_const(cable_autonum_keys)) {
		QDomElement cable_autonum = cableAutoNum(key).toXml(xml_document, "cable_autonum");
		if (key != "" && cableAutoNumFormula(key) != "") {
			cable_autonum.setAttribute("title", key);
			cable_autonum.setAttribute("formula", cableAutoNumFormula(key));
			cable_autonums.appendChild(cable_autonum);
			++written_cable_rules;
		}
	}
		//Nothing to keep when the project never named a rule, never
		//chose an axis other than the default and has not answered the
		//numbering question with no -- then this element would say
		//nothing the reader does not assume anyway.
	if (!m_current_cable_autonum.isEmpty()
	    || !m_cable_ask_numbering
	    || !m_cable_axis_x_first
	    || written_cable_rules > 0) {
		xml_element.appendChild(cable_autonums);
	}

	// Export default guides
	QDomElement guides_elmt = xml_document.createElement("guides");
	for (const auto &g : m_default_guides) {
		QDomElement guide_elmt = xml_document.createElement("guide");
		guide_elmt.setAttribute("orientation", static_cast<int>(g.orientation));
		guide_elmt.setAttribute("position", g.position);
		guide_elmt.setAttribute("color", g.color.name());
		guides_elmt.appendChild(guide_elmt);
	}
	xml_element.appendChild(guides_elmt);
}

/**
	@brief QETProject::addDiagram
	Add a diagram in this project
	@param diagram added diagram
	@param pos position of the new diagram, by default at the end
*/
void QETProject::addDiagram(Diagram *diagram, int pos)
{
	if (!diagram) {
		return;
	}

	connect(&diagram->border_and_titleblock,
		&BorderTitleBlock::needFolioData,
		this,
		&QETProject::updateDiagramsFolioData);
	connect(diagram, &Diagram::usedTitleBlockTemplateChanged,
		this, &QETProject::usedTitleBlockTemplateChanged);

	if (pos == -1) {
		m_diagrams_list << diagram;
	} else {
		m_diagrams_list.insert(pos, diagram);
	}

	updateDiagramsFolioData();
	emit diagramAdded(this, diagram);
}

/**
	@brief QETProject::detachDiagram
	Inverse of addDiagram(): removes \p diagram from this project's diagram
	list and disconnects the signals set up by addDiagram(), without
	destroying it. Used by RemoveDiagramCommand (and removeDiagram()) so a
	removed diagram can be parked and later reinserted by undo.
	@param diagram
*/
void QETProject::detachDiagram(Diagram *diagram)
{
	if (!diagram || !m_diagrams_list.contains(diagram)) {
		return;
	}

	disconnect(&diagram->border_and_titleblock,
		&BorderTitleBlock::needFolioData,
		this,
		&QETProject::updateDiagramsFolioData);
	disconnect(diagram, &Diagram::usedTitleBlockTemplateChanged,
		this, &QETProject::usedTitleBlockTemplateChanged);

		//A folio that holds a table or a symbol is removed: tables on folios
		//not built yet (lazy folios) are built while it is still in the
		//project, as they are with every folio built (F121). A table split
		//over several folios keeps its query in its first part, and the next
		//part takes it over only if that part was built
		//(QetGraphicsTableItem::toXml()); built later, from the file, it
		//names a part that is gone and saves no query. A built table also
		//follows the project's database, so a parts list that showed this
		//folio's symbols is resized like every other. A folio with neither
		//builds no other.
	if (folioHoldsTablesOrSymbols(diagram)) {
		buildFoliosHoldingTables();
	}

	m_diagrams_list.removeAll(diagram);
	updateDiagramsFolioData();
	emit diagramRemoved(this, diagram);
}

/**
	@brief QETProject::writeBackup
	Write a backup file of this project, in the case that QET crash.
	The snapshots are written in turn to m_backup_files, so a write made
	while the project is already in a bad state only replaces the oldest
	snapshot, and the earlier ones are still there to recover from.
*/
void QETProject::writeBackup()
{
	if (!m_backup_enabled)
		return;
		//Don't launch a new backup while the previous one is still writing:
		//both could write through the same m_backup_files slot on different threads.
	if (m_backup_future.isRunning())
		return;
		//toXml() walks the whole project on the GUI thread, which freezes
		//big projects for seconds (bugtracker #273, #329). A backup of an
		//unchanged project would be identical to the last one, so skip it.
	if (!m_backup_needed)
		return;
	m_backup_needed = false;
		//Capture the document by value (implicitly shared, so cheap): the
		//Qt5-style QtConcurrent::run(function, reference-args) call did not
		//survive the Qt6 API change, a lambda behaves identically on both.
	QHash<int, QByteArray> kept_folios;
	QDomDocument xml_project(toXml(&kept_folios));
	KAutoSaveFile *target = &m_backup_files[m_next_backup_slot];
	m_next_backup_slot = (m_next_backup_slot + 1) % BackupGenerations;
	m_backup_future = QtConcurrent::run([target, xml_project, kept_folios]() mutable {
		return QET::writeTextToFile(spliceKeptFolios(xml_project.toString(4), kept_folios),
									target, nullptr);
	});
}

/**
	@return true if project options (title, project-wide properties, settings
	for new diagrams, diagrams order...) were modified, false otherwise.
*/
bool QETProject::projectOptionsWereModified()
{
	// unlike similar methods, this method does not compare the content against
	// expected values; instead, we just check whether we have been set as modified.
	return(m_modified);
}

/**
	@return the project-wide properties made available to child diagrams.
*/
DiagramContext QETProject::projectProperties()
{
	return(m_project_properties);
}

/**
	Use \a context as project-wide properties made available to child diagrams.
*/
void QETProject::setProjectProperties(const DiagramContext &context) {
	m_project_properties = context;
		//Folio and symbol texts are made from them
	markKeptFoliosStale();
	updateDiagramsFolioData();
	emit projectInformationsChanged(this);
}

/**
 * @brief QETProject::terminalStrip
 * @return a QVector who contain all terminal strip owned by this project
 */
QVector<TerminalStrip *> QETProject::terminalStrip() const {
	return m_terminal_strip_vector;
}

/**
 * @brief QETProject::newTerminalStrip
 * @param installation : installation of the terminal strip
 * @param location : location of the terminal strip
 * @param name : name of the terminal strip
 * @return Create a new terminal strip with this project as parent.
 * You can retrieve this terminal strip at any time by calling the function
 * QETProject::terminalStrip()
 */
TerminalStrip *QETProject::newTerminalStrip(QString installation, QString location, QString name)
{
	auto ts = new TerminalStrip(installation,
								location,
								name,
								this);

	m_terminal_strip_vector.append(ts);
	watchTerminalStrip(ts);
	return ts;
}

/**
 * @brief QETProject::watchTerminalStrip
 * The spare terminals of @a strip (on no folio) are lines of the
 * nomenclature : keep the database up to date with them.
 * @param strip
 * @param watch : false to stop watching @a strip
 */
void QETProject::watchTerminalStrip(TerminalStrip *strip, bool watch)
{
	if (!strip) {
		return;
	}
	disconnect(strip, nullptr, &m_data_base, nullptr);
	if (watch) {
		connect(strip, &TerminalStrip::orderChanged, &m_data_base, &projectDataBase::spareTerminalsChanged);
		connect(strip, &TerminalStrip::dataChanged, &m_data_base, &projectDataBase::spareTerminalsChanged);
	}
	m_data_base.spareTerminalsChanged();
}

/**
 * @brief QETProject::addTerminalStrip
 * Add \p strip to the terminal strip list of the project.
 * The project of \p strip must this project
 * @param strip
 * @return true if successfully added
 */
bool QETProject::addTerminalStrip(TerminalStrip *strip)
{
	if (strip->parent() != this)
		return false;

	if (m_terminal_strip_vector.contains(strip))
		return true;

	m_terminal_strip_vector.append(strip);
	watchTerminalStrip(strip);
	return true;
}

/**
 * @brief QETProject::removeTerminalStrip
 * Remove \p strip from the terminal strip list of this project.
 * Strip is removed from the list but not deleted.
 * @param strip
 * @return true if successfully removed.
 */
bool QETProject::removeTerminalStrip(TerminalStrip *strip)
{
	const bool removed_ = m_terminal_strip_vector.removeOne(strip);
	if (removed_) {
		watchTerminalStrip(strip, false);
	}
	return removed_;
}

/**
 * @brief QETProject::cables
 * @return every cable of this project
 */
QVector<Cable *> QETProject::cables() const {
	return m_cables;
}

/**
 * @brief QETProject::newCable
 * Create a cable owned by this project and watch it: whenever it changes,
 * the cable field of every conductor it feeds is worked out again.
 * @return the new cable
 */
Cable *QETProject::newCable()
{
	auto cable = new Cable(this);
	addCable(cable);
	return cable;
}

/**
 * @brief QETProject::addCable
 * @param cable
 * @return true when the cable is now held by this project
 */
bool QETProject::addCable(Cable *cable)
{
	if (!cable || cable->parent() != this)
		return false;

	if (m_cables.contains(cable))
		return true;

	m_cables.append(cable);
	connect(cable, &Cable::changed, this, [this]() {
		CableManager::refreshLabels(this);
		setModified(true);
	});
	emit cableAdded(cable);
	return true;
}

/**
 * @brief QETProject::removeCable
 * Cable is removed from the list but not deleted, so an undo command can
 * put it back.
 * @param cable
 * @return true when the cable was in the list
 */
bool QETProject::removeCable(Cable *cable) {
	if (!m_cables.removeOne(cable))
		return false;

	disconnect(cable, nullptr, this, nullptr);

		//A cable which is no longer one of ours stops writing its labels
		//into the conductors it used to feed.
	CableManager::refreshLabels(this);
	setModified(true);
	emit cableRemoved(cable);
	return true;
}

/**
 * @brief QETProject::diagramByUuid
 * @param uuid
 * @return the folio with that identity, or nullptr. A cable section says
 * which folio it is drawn on by identity, since folios are reordered and
 * renumbered freely.
 */
Diagram *QETProject::diagramByUuid(const QUuid &uuid)
{
	if (uuid.isNull()) return nullptr;
	for (Diagram *diagram : m_diagrams_list) {
		if (diagram && diagram->uuid() == uuid) return diagram;
	}
	return nullptr;
}

/**
	Cette methode sert a reperer un projet vide, c-a-d un projet identique a ce
	que l'on obtient en faisant Fichier > Nouveau.
	@return true si les schemas, la collection embarquee ou les proprietes de ce
	projet ont ete modifies.
	Concretement, le projet doit avoir un titre vide et ni ses schemas ni sa
	collection embarquee ne doivent avoir ete modifies.
	@see diagramsWereModified(), embeddedCollectionWasModified()
*/
bool QETProject::projectWasModified()
{

	if ( projectOptionsWereModified()    ||
		 !m_undo_stack -> isClean()       ||
		 m_titleblocks_collection.templates().count() )
		return(true);

	else
		return(false);
}

/**
	Indique a chaque schema du projet quel est son numero de folio et combien de
	folio le projet contient.
*/
/**
	@brief QETProject::projectWideProperties
	@return the project's properties as every folio's title block sees them:
	the user's project properties plus the project's title, path and file name.
*/
DiagramContext QETProject::projectWideProperties()
{
	DiagramContext project_wide_properties = m_project_properties;
	project_wide_properties.addValue("projecttitle", title());
	project_wide_properties.addValue("projectpath", filePath());
	project_wide_properties.addValue("projectfilename", QFileInfo(filePath()).completeBaseName());
	return project_wide_properties;
}

void QETProject::updateDiagramsFolioData()
{
	int total_folio = m_diagrams_list.count();

	const DiagramContext project_wide_properties = projectWideProperties();

	for (int i = 0 ; i < total_folio ; ++ i)
	{
		QString autopagenum = m_diagrams_list[i]->border_and_titleblock.autoPageNum();
		NumerotationContext nC = folioAutoNum(autopagenum);
		NumerotationContextCommands nCC = NumerotationContextCommands(nC);

		if ((m_diagrams_list[i]->border_and_titleblock.folio().contains("%autonum")) &&
			(!autopagenum.isNull()))
		{
			m_diagrams_list[i] -> border_and_titleblock.setFolioData(i + 1, total_folio, nCC.toRepresentedString(), project_wide_properties);
			m_diagrams_list[i]->project()->addFolioAutoNum(autopagenum,nCC.next());
		}
		else {
			m_diagrams_list[i] -> border_and_titleblock.setFolioData(i + 1, total_folio, nullptr, project_wide_properties);
		}

		if (i > 0)
		{
			m_diagrams_list.at(i)->border_and_titleblock.setPreviousFolioNum(m_diagrams_list.at(i-1)->border_and_titleblock.finalfolio());
			m_diagrams_list.at(i-1)->border_and_titleblock.setNextFolioNum(m_diagrams_list.at(i)->border_and_titleblock.finalfolio());

			if (i == total_folio-1) {
				m_diagrams_list.at(i)->border_and_titleblock.setNextFolioNum(QString());
			}
		}
		else {
			m_diagrams_list.at(i)->border_and_titleblock.setPreviousFolioNum(QString());
		}
	}

	for (const auto &diagram_ : std::as_const(m_diagrams_list)) {
		diagram_->update();
	}
}

/**
	Inform each diagram that the \a template_name title block changed.
	@param collection Title block templates collection
	@param template_name Name of the changed template
*/
void QETProject::updateDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *collection, const QString &template_name) {
	Q_UNUSED(collection)

	foreach (Diagram *diagram, m_diagrams_list) {
		diagram -> titleBlockTemplateChanged(template_name);
	}
}

/**
	Inform each diagram that the \a template_name title block is about to be removed.
	@param collection Title block templates collection
	@param template_name Name of the removed template
*/
void QETProject::removeDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *collection, const QString &template_name) {
	Q_UNUSED(collection)

	// warn diagrams that the given template is about to be removed
	foreach (Diagram *diagram, m_diagrams_list) {
		diagram -> titleBlockTemplateRemoved(template_name);
	}
}

/**
	Handles the fact that a diagram changed the title block template it uses
	@param template_name Name of the template
*/
void QETProject::usedTitleBlockTemplateChanged(const QString &template_name) {
	emit diagramUsedTemplate(embeddedTitleBlockTemplatesCollection(), template_name);
}
