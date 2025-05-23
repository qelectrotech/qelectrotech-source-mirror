﻿/*
	Copyright 2006-2025 The QElectroTech Team
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

#include "ElementsCollection/xmlelementcollection.h"
#include "autoNum/assignvariables.h"
#include "autoNum/numerotationcontext.h"
#include "autoNum/numerotationcontextcommands.h"
#include "diagram.h"
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
#include "qetversion.h"

#include <QHash>
#include <QTimer>
#include <QtConcurrentRun>
#include <QtDebug>
#include <utility>

static int BACKUP_INTERVAL = 120000; //interval in ms of backup = 2min

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

	init();
}

#ifdef BUILD_WITHOUT_KF5
#else
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
	delete  backup;

		//Set the project to read only mode if the file it.
	QFileInfo fi(m_file_path);
	setReadOnly(!fi.isWritable());

	init();
}
#endif

/**
	@brief QETProject::~QETProject
	Destructor
*/
QETProject::~QETProject()
{
		//We block database signal to avoid hundreds of unnecessary emitted signal
		//due to deletion (diagram, item, etc...) and as much update made in the not yet deleted things.
	m_data_base.blockSignals(true);

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
	@brief QETProject::init
*/
void QETProject::init()
{
	connect(&m_titleblocks_collection, &TitleBlockTemplatesCollection::changed, this, &QETProject::updateDiagramsTitleBlockTemplate);
	connect(&m_titleblocks_collection, &TitleBlockTemplatesCollection::aboutToRemove, this, &QETProject::removeDiagramsTitleBlockTemplate);

	m_undo_stack = new QUndoStack(this);
	connect(m_undo_stack, SIGNAL(cleanChanged(bool)), this, SLOT(undoStackChanged(bool)));

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
		connect(&m_autosave_timer, &QTimer::timeout, this, [=]()
		{
			if(!this->m_file_path.isEmpty())
				this->write();
		});
		m_autosave_timer.start();
	}
}

/**
	@brief QETProject::openFile
	@param file
	@return
*/
QETProject::ProjectState QETProject::openFile(QFile *file)
{
	bool opened_here = file->isOpen() ? false : true;
	if (!file->isOpen()
			&& !file->open(QIODevice::ReadOnly
					   | QIODevice::Text)) {
		return FileOpenFailed;
	}
	QFileInfo fi(*file);
	setFilePath(fi.absoluteFilePath());

		//Extract the content of the xml
	QDomDocument xml_project;
	if (!xml_project.setContent(file))
	{
		if(opened_here) {
			file->close();
		}
		return XmlParsingFailed;
	}

		//Build the project from the xml
	readProjectXml(xml_project);

	if (!fi.isWritable()) {
		setReadOnly(true);
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

	for(const auto &diagram : diagrams())
	{
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
	@return la liste des schemas de ce projet
*/
QList<Diagram *> QETProject::diagrams() const
{
	return(m_diagrams_list);
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
#ifdef BUILD_WITHOUT_KF5
#else
	if (m_backup_file.isOpen()) {
		m_backup_file.close();
	}
	m_backup_file.setManagedFile(QUrl::fromLocalFile(filepath));
#endif
	m_file_path = filepath;

	QFileInfo fi(m_file_path);
	if (fi.isWritable()) {
		setReadOnly(false);
	}

		//title block variables should be updated after file save as dialog is confirmed, before file is saved.
	m_project_properties.addValue("saveddate",     QLocale::system().toString(QDate::currentDate(), QLocale::ShortFormat));
	m_project_properties.addValue("saveddate-eu",  QDate::currentDate().toString("dd-MM-yyyy"));
	m_project_properties.addValue("saveddate-us",  QDate::currentDate().toString("yyyy-MM-dd"));
	m_project_properties.addValue("savedtime",     QDateTime::currentDateTime().toString("HH:mm"));
	m_project_properties.addValue("savedfilename", QFileInfo(filePath()).baseName());
	m_project_properties.addValue("savedfilepath", filePath());



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
				"Projet « %1 : %2»",
				"displayed title for a ProjectView - %1 is the project title, -%2 is the project path"
			)
		).arg(project_title_, m_file_path);
	} else if (!m_file_path.isEmpty()) {
		final_title = QString(
			tr(
				"Projet %1",
				"displayed title for a title-less project - %1 is the file name"
			)
		).arg(QFileInfo(m_file_path).completeBaseName());
	} else {
		final_title = QString(
			tr(
				"Projet sans titre",
				"displayed title for a project-less, file-less project"
			)
		);
	}

	if (isReadOnly()) {
		final_title = QString(
			tr(
				"%1 [lecture seule]",
				"displayed title for a read-only project - %1 is a displayable title"
			)
		).arg(final_title);
	}
	if (m_modified) {
		final_title = QString(
			tr(
				"%1 [modifié]",
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
	m_element_autonum.insert(key, context);
	emit elementAutoNumAdded(key);
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
	@brief QETProject::freezeExistentConductorLabel
	Freeze Existent Conductors in the selected folios
	@param freeze
	@param from - first folio index to apply freeze
	@param to - last folio index to apply freeze
*/
void QETProject::freezeExistentConductorLabel(bool freeze, int from, int to) {
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
QDomDocument QETProject::toXml()
{
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
	xml_doc.appendChild(project_root);

	// titleblock templates, if any
	if (m_titleblocks_collection.templates().count()) {
		QDomElement titleblocktemplates_elmt = xml_doc.createElement("titleblocktemplates");
		foreach (QString template_name, m_titleblocks_collection.templates()) {
			QDomElement e = m_titleblocks_collection.getTemplateXmlDescription(template_name);
			titleblocktemplates_elmt.appendChild(xml_doc.importNode(e, true));
		}
		project_root.appendChild(titleblocktemplates_elmt);
	}

	// project-wide properties
	QDomElement project_properties = xml_doc.createElement("properties");
	writeProjectPropertiesXml(project_properties);
	project_root.appendChild(project_properties);

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
		QDomElement xml_diagram = diagram->toXml().documentElement();
		QDomNode xml_node = xml_doc.importNode(xml_diagram, true);

		QDomNode appended_diagram = project_root.appendChild(xml_node);
		appended_diagram.toElement().setAttribute("order", order_num ++);
	}

		//Write terminal strip to xml
	if (m_terminal_strip_vector.count())
	{
		auto xml_strip = xml_doc.createElement(QStringLiteral("terminal_strips"));
		for (auto &strip : m_terminal_strip_vector) {
			xml_strip.appendChild(strip->toXml(xml_doc));
		}
		project_root.appendChild(xml_strip);
	}

	// Write the elements collection.
	project_root.appendChild(m_elements_collection->root().cloneNode(true));

	return(xml_doc);
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
QETResult QETProject::write()
{
		// this operation requires a filepath
	if (m_file_path.isEmpty())
		return(QString("unable to save project to file: no filepath was specified"));

		// if the project was opened read-only
		// and the file is still non-writable, do not save the project
	if (isReadOnly() && !QFileInfo(m_file_path).isWritable())
		return(QString("the file %1 was opened read-only and thus will not be written").arg(m_file_path));

	QDomDocument xml_project(toXml());
	QString error_message;
	if (!QET::writeXmlFile(xml_project, m_file_path, &error_message))
		return(error_message);

		//title block variables should be updated after file save dialog is confirmed, before file is saved.
	m_project_properties.addValue("saveddate",     QLocale::system().toString(QDate::currentDate(), QLocale::ShortFormat));
	m_project_properties.addValue("saveddate-us",  QDate::currentDate().toString("yyyy-MM-dd"));
	m_project_properties.addValue("saveddate-eu",  QDate::currentDate().toString("dd-MM-yyyy"));
	m_project_properties.addValue("savedtime",     QDateTime::currentDateTime().toString("HH:mm"));
	m_project_properties.addValue("savedfilename", QFileInfo(filePath()).baseName());
	m_project_properties.addValue("savedfilepath", filePath());

	emit projectInformationsChanged(this);
	updateDiagramsFolioData();

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
		import_path = "import/" + location.collectionPath(false);
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
		handler -> errorWithATemplate(src_tbt, tr("Une erreur s'est produite durant l'intégration du modèle.", "error message"));
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
	foreach(Diagram *diagram, diagrams()) {
		if (diagram -> usesElement(location)) {
			return(true);
		}
	}
	return(false);
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

	foreach(ElementsLocation location, m_elements_collection->elementsLocation())
		if (location.isElement() && !usesElement(location))
			unused_list << location;

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

	foreach (Diagram *diagram, diagrams()) {
		if (diagram -> usesTitleBlockTemplate(location.name())) {
			return(true);
		}
	}
	return(false);
}

/**
	@brief QETProject::addNewDiagram
	Add a new diagram in project at position pos.
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

	addDiagram(diagram, pos);
	emit diagramAdded(this, diagram);
	return(diagram);
}

/**
	@brief QETProject::removeDiagram
	Remove diagram from project
	@param diagram
*/
void QETProject::removeDiagram(Diagram *diagram)
{
	if (isReadOnly() ||
		!diagram || !m_diagrams_list.contains(diagram)) {
		return;
	}

	if (m_diagrams_list.removeAll(diagram))
	{
		emit diagramRemoved(this, diagram);
		diagram->deleteLater();
	}

	updateDiagramsFolioData();
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
							tr("Avertissement",
							   "message box title"),
							tr("Ce document semble avoir été enregistré avec une version %1"
							   "\n qui est ultérieure à votre version !"
							   " \n"
							   "Vous utilisez actuellement QElectroTech en version %2")
							.arg(root_elmt.attribute(QStringLiteral("version")), QetVersion::currentVersion().toString() +
							tr(".\n Il est alors possible que l'ouverture de tout ou partie de ce "
							   "document échoue.\n"
							   "Que désirez vous faire ?"),
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
							tr("Avertissement ", "message box title"),
							tr("Le projet que vous tentez d'ouvrir est partiellement "
							   "compatible avec votre version %1 de QElectroTech.\n")
							.arg(QetVersion::currentVersion().toString()) +
							tr("Afin de le rendre totalement compatible veuillez ouvrir ce même projet "
							   "avec la version 0.8, ou 0.80 de QElectroTech et sauvegarder le projet "
							   "et l'ouvrir à  nouveau avec cette version.\n"
							   "Que désirez vous faire ?"),
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

		//Load the project-wide properties
	readProjectPropertiesXml(xml_project);

		//Load the default properties for the new diagrams
	readDefaultPropertiesXml(xml_project);

		//load the embedded titleblock templates
	m_titleblocks_collection.fromXml(xml_project.documentElement());

		//Load the embedded elements collection
	readElementsCollectionXml(xml_project);

		//Load the diagrams
	readDiagramsXml(xml_project);

		//Load the terminal strip
	readTerminalStripXml(xml_project);

		//Now that all are loaded we refresh content of the project.
	refresh();


	m_data_base.blockSignals(false);
	m_data_base.updateDB();

	m_state = Ok;
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
		dlgWaiting -> setTitle(tr("<p align=\"center\">"
					  "<b>Ouverture du projet en cours...</b><br/>"
					  "Création des folios"
					  "</p>"));
	}

	//Search the diagrams in the project
	QDomNodeList diagram_nodes = xml_project.elementsByTagName(QStringLiteral("diagram"));

	if(dlgWaiting)
		dlgWaiting->setProgressBarRange(0, diagram_nodes.length()*3);

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

			diagram->initFromXml(diagram_xml_element);
			if(dlgWaiting)
				dlgWaiting->setDetail(diagram->title());
		}
	}

	updateDiagramsFolioData();

		//Initialise links between elements in this project
		//and refresh the text of conductor
	if(dlgWaiting)
	{
		dlgWaiting->setTitle( tr("<p align=\"center\">"
								 "<b>Ouverture du projet en cours...</b><br/>"
								 "Mise en place des références croisées"
								 "</p>"));
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
	QDomElement border_elmt, titleblock_elmt, conductors_elmt, report_elmt, xref_elmt, conds_autonums, folio_autonums, element_autonums;

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
			m_element_autonum.insert(elmt.attribute(QStringLiteral("title")), nc);
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
	Export project properties under the \a xml_element XML element.
*/
void QETProject::writeProjectPropertiesXml(QDomElement &xml_element) {
	m_project_properties.toXml(xml_element);
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
	for (QString key : defaultXRefProperties().keys())
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
	foreach (QString key, conductorAutoNum().keys()) {
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
	foreach (QString key, folioAutoNum().keys()) {
	QDomElement folio_autonum = folioAutoNum(key).toXml(xml_document, "folio_autonum");
		folio_autonum.setAttribute("title", key);
		folio_autonums.appendChild(folio_autonum);
	}
	xml_element.appendChild(folio_autonums);

	//Export Element Autonums
	QDomElement element_autonums = xml_document.createElement("element_autonums");
	element_autonums.setAttribute("current_autonum", m_current_element_autonum);
	element_autonums.setAttribute("freeze_new_elements", m_freeze_new_elements ? "true" : "false");
	foreach (QString key, elementAutoNum().keys()) {
	QDomElement element_autonum = elementAutoNum(key).toXml(xml_document, "element_autonum");
		if (key != "" && elementAutoNumFormula(key) != "") {
			element_autonum.setAttribute("title", key);
			element_autonum.setAttribute("formula", elementAutoNumFormula(key));
			element_autonums.appendChild(element_autonum);
		}
	}
	xml_element.appendChild(element_autonums);
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
}

/**
	@brief QETProject::writeBackup
	Write a backup file of this project, in the case that QET crash
*/
void QETProject::writeBackup()
{
#ifdef BUILD_WITHOUT_KF5
#else
#	if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) // ### Qt 6: remove
	QDomDocument xml_project(toXml());
	QtConcurrent::run(
				QET::writeToFile,xml_project,&m_backup_file,nullptr);
#	else
#		if TODO_LIST
#			pragma message("@TODO remove code for QT 6 or later")
#		endif
	qDebug() << "Help code for QT 6 or later"
			 << "QtConcurrent::run its backwards now...function, object, args";
#	endif
#endif
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
	return ts;
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
	return true;
}

/**
 * @brief QETProject::removeTerminalStrip
 * Remove \p strip from the terminal strip list of this project.
 * Strip is removed from the list but not deleted.
 * @param strip
 * @return true if successfully removed.
 */
bool QETProject::removeTerminalStrip(TerminalStrip *strip) {
	return m_terminal_strip_vector.removeOne(strip);
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
void QETProject::updateDiagramsFolioData()
{
	int total_folio = m_diagrams_list.count();

	DiagramContext project_wide_properties = m_project_properties;
	project_wide_properties.addValue("projecttitle", title());
	project_wide_properties.addValue("projectpath", filePath());
	project_wide_properties.addValue("projectfilename", QFileInfo(filePath()).baseName());

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

	for (const auto &diagram_ : qAsConst(m_diagrams_list)) {
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
