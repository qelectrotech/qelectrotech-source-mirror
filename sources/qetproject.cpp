/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "diagram.h"
#include "diagramfoliolist.h"
#include "qetapp.h"
#include "qetresult.h"
#include "movetemplateshandler.h"
#include "qetmessagebox.h"
#include "titleblocktemplate.h"
#include "ui/dialogwaiting.h"
#include "numerotationcontext.h"
#include "reportproperties.h"
#include "integrationmovetemplateshandler.h"
#include "xmlelementcollection.h"
#include "importelementdialog.h"
#include "numerotationcontextcommands.h"
#include "assignvariables.h"


#include <QStandardPaths>

/**
	Constructeur par defaut - cree un schema contenant une collection
	d'elements vide et un schema vide.
	@param diagrams Nombre de nouveaux schemas a ajouter a ce nouveau projet
	@param parent QObject parent
*/
QETProject::QETProject(int diagrams, QObject *parent) :
	QObject              (parent),
	project_qet_version_ (-1    ),
	modified_            (false ),
	read_only_           (false ),
	titleblocks_         (this  ),
	folioSheetsQuantity  (0     ),
	m_auto_conductor     (true  ),
	m_elements_collection (nullptr),
	m_freeze_new_elements (false),
	m_freeze_new_conductors (false)
{
	// 0 a n schema(s) vide(s)
	int diagrams_count = qMax(0, diagrams);
	for (int i = 0 ; i < diagrams_count ; ++ i) {
		addNewDiagram();
	}

	m_elements_collection = new XmlElementCollection(this);

	setupTitleBlockTemplatesCollection();

	undo_stack_ = new QUndoStack();
	connect(undo_stack_, SIGNAL(cleanChanged(bool)), this, SLOT(undoStackChanged(bool)));
}

/**
 * @brief QETProject::QETProject
 * Construct a project from a .qet file
 * @param path : path of the file
 * @param parent : parent QObject
 */
QETProject::QETProject(const QString &path, QObject *parent) :
	QObject              (parent),
	project_qet_version_ (-1    ),
	modified_            (false ),
	read_only_           (false ),
	titleblocks_         (this  ),
	folioSheetsQuantity  (0     ),
	m_auto_conductor     (true  ),
	m_elements_collection (nullptr)
{
		//Open the file
	QFile project_file(path);
	if (!project_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		state_ = FileOpenFailed;
		return;
	}
	setFilePath(path);
	
		//Extract the content of the xml
	QDomDocument xml_project;
	if (!xml_project.setContent(&project_file))
	{
		state_ = XmlParsingFailed;
		return;
	}

		//Build the project from the xml
	readProjectXml(xml_project);
	
	setupTitleBlockTemplatesCollection();
	
	// passe le projet en lecture seule si le fichier l'est
	QFileInfo project_file_info(path);
	if (!project_file_info.isWritable()) {
		setReadOnly(true);
	}

	undo_stack_ = new QUndoStack();
	connect(undo_stack_, SIGNAL(cleanChanged(bool)), this, SLOT(undoStackChanged(bool)));
}

/**
 * @brief QETProject::~QETProject
 * Destructor
 */
QETProject::~QETProject()
{
	qDeleteAll(diagrams_);
	delete undo_stack_;
}

/**
	Cette methode peut etre utilisee pour tester la bonne ouverture d'un projet
	@return l'etat du projet
	@see ProjectState
*/
QETProject::ProjectState QETProject::state() const {
	return(state_);
}

/**
	Get the folioSheetQuantity
	@return folio Sheets Quantity.
*/
int QETProject::getFolioSheetsQuantity() const {
	return(folioSheetsQuantity);
}

/**
	Set the folioSheetQuantity to quantity
	@param New value of quantity to be set.
*/
void QETProject::setFolioSheetsQuantity(int quantity) {
	folioSheetsQuantity = quantity;
}

/**
	@return la liste des schemas de ce projet
*/
QList<Diagram *> QETProject::diagrams() const {
	return(diagrams_);
}

/**
	@param diagram Pointer to a Diagram object
	@return the folio number of the given diagram object within the project,
	or -1 if it is not part of this project.
	Note: this returns 0 for the first diagram, not 1
*/
int QETProject::folioIndex(const Diagram *diagram) const {
	// QList::indexOf returns -1 if no item matched.
	return(diagrams_.indexOf(const_cast<Diagram *>(diagram)));
}

/**
 * @brief QETProject::embeddedCollection
 * @return The embedded collection
 */
XmlElementCollection *QETProject::embeddedElementCollection() const {
	return m_elements_collection;
}

/**
	@return the title block templates collection enbeedded within this project
*/
TitleBlockTemplatesProjectCollection *QETProject::embeddedTitleBlockTemplatesCollection() {
	return(&titleblocks_);
}

/**
	@return le chemin du fichier dans lequel ce projet est enregistre
*/
QString QETProject::filePath() {
	return(file_path_);
}

/**
	Change le chemin du fichier dans lequel ce projet est enregistre
	@param filepath Nouveau chemin de fichier
*/
void QETProject::setFilePath(const QString &filepath) {
	file_path_ = filepath;
	
	// le chemin a change : on reevalue la necessite du mode lecture seule
	QFileInfo file_path_info(file_path_);
	if (file_path_info.isWritable()) {
		setReadOnly(false);
	}
	
	emit(projectFilePathChanged(this, file_path_));
	emit(projectInformationsChanged(this));
}

/**
	@return le dossier contenant le fichier projet si celui-ci a ete
	enregistre ; dans le cas contraire, cette methode retourne l'emplacement
	du bureau de l'utilisateur.
*/
QString QETProject::currentDir() const {
	QString current_directory;
	if (file_path_.isEmpty()) {
		current_directory = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
	} else {
		current_directory = QFileInfo(file_path_).absoluteDir().absolutePath();
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
QString QETProject::pathNameTitle() const {
	QString final_title;
	
	if (!project_title_.isEmpty()) {
		final_title = QString(
			tr(
				"Projet « %1 »",
				"displayed title for a ProjectView - %1 is the project title"
			)
		).arg(project_title_);
	} else if (!file_path_.isEmpty()) {
		final_title = QString(
			tr(
				"Projet %1",
				"displayed title for a title-less project - %1 is the file name"
			)
		).arg(QFileInfo(file_path_).completeBaseName());
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
	if (modified_) {
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
QString QETProject::title() const {
	return(project_title_);
}

/**
	@return la version de QElectroTech declaree dans le fichier projet lorsque
	celui-ci a ete ouvert ; si ce projet n'a jamais ete enregistre / ouvert
	depuis un fichier, cette methode retourne -1.
*/
qreal QETProject::declaredQElectroTechVersion() {
	return(project_qet_version_);
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
	emit(projectTitleChanged(this, project_title_));
	emit(projectInformationsChanged(this));
	updateDiagramsFolioData();
}

/**
	@return les dimensions par defaut utilisees lors de la creation d'un
	nouveau schema dans ce projet.
*/
BorderProperties QETProject::defaultBorderProperties() const {
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
TitleBlockProperties QETProject::defaultTitleBlockProperties() const {
	return(default_titleblock_properties_);
}

/**
 * @brief QETProject::setDefaultTitleBlockProperties
 * Specify the title block to be used at the creation of a new diagram for this project
 * @param titleblock
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
			case QET::Custom :
				collection = QETApp::customTitleBlockTemplatesCollection();
				break;
			case QET::Embedded :
				//Titleblock is already embedded to project
				return;
			default:
				return;
		}

		QScopedPointer<IntegrationMoveTitleBlockTemplatesHandler> m(new IntegrationMoveTitleBlockTemplatesHandler);
		integrateTitleBlockTemplate(collection -> location(titleblock.template_name), m.data());
	}
	emit defaultTitleBlockPropertiesChanged();
}

/**
	@return le type de conducteur par defaut utilise lors de la creation d'un
	nouveau schema dans ce projet.
*/
ConductorProperties QETProject::defaultConductorProperties() const {
	return(default_conductor_properties_);
}

/**
	Permet de specifier e type de conducteur par defaut utilise lors de la
	creation d'un nouveau schema dans ce projet.
*/
void QETProject::setDefaultConductorProperties(const ConductorProperties &conductor) {
	default_conductor_properties_ = conductor;
}

QString QETProject::defaultReportProperties() const {
	return default_report_properties_;
}

void QETProject::setDefaultReportProperties(const QString &properties) {
	default_report_properties_ = properties;
	emit reportPropertiesChanged(properties);
}

void QETProject::setDefaultXRefProperties(const QString type, const XRefProperties &properties) {
	m_default_xref_properties.insert(type, properties);
	emit XRefPropertiesChanged();
}

void QETProject::setDefaultXRefProperties(QHash<QString, XRefProperties> hash)
{
	m_default_xref_properties.swap(hash);
	emit XRefPropertiesChanged();
}

/**
 * @brief QETProject::conductorAutoNum
 * @return All value of conductor autonum stored in project
 */
QHash <QString, NumerotationContext> QETProject::conductorAutoNum() const {
	return m_conductor_autonum;
}

/**
 * @brief QETProject::elementAutoNum
 * @return All value of element autonum stored in project
 */
QHash <QString, NumerotationContext> QETProject::elementAutoNum() const {
	return m_element_autonum;
}

/**
 * @brief QETProject::elementAutoNumFormula
 * @param element autonum title
 * @return Formula of element autonum stored in element autonum
 */
QString QETProject::elementAutoNumFormula (QString key) const
{
	if (m_element_autonum.contains(key)) {
		return autonum::numerotationContextToFormula(m_element_autonum[key]);
	}

	return QString();
}

/**
 * @brief QETProject::elementAutoNumCurrentFormula
 * @return current formula being used by project
 */
QString QETProject::elementAutoNumCurrentFormula() const {
	return elementAutoNumFormula(m_current_element_autonum);
}

/**
 * @brief QETProject::elementCurrentAutoNum
 * @return current element autonum title
 */
QString QETProject::elementCurrentAutoNum () const {
	return m_current_element_autonum;
}

/**
 * @brief QETProject::setCurrrentElementAutonum
 * @param autoNum : set the current element autonum to @autonum
 */
void QETProject::setCurrrentElementAutonum(QString autoNum) {
	m_current_element_autonum = autoNum;
}

/**
 * @brief QETProject::conductorAutoNumFormula
 * @param conductor autonum title
 * @return Formula of element autonum stored in conductor autonum
 */
QString QETProject::conductorAutoNumFormula (QString key) const
{
	if (m_conductor_autonum.contains(key))
		return autonum::numerotationContextToFormula(m_conductor_autonum.value(key));
	else
		return QString();
}

/**
 * @brief QETProject::conductorCurrentAutoNum
 * @return current conductor autonum title
 */
QString QETProject::conductorCurrentAutoNum () const {
	return m_current_conductor_autonum;
}

/**
 * @brief QETProject::setCurrentConductorAutoNum
 * @param autoNum set the current conductor autonum to @autonum
 */
void QETProject::setCurrentConductorAutoNum(QString autoNum) {
	m_current_conductor_autonum = autoNum;
}

/**
 * @brief QETProject::folioAutoNum
 * @return All value of folio autonum stored in project
 */
QHash <QString, NumerotationContext> QETProject::folioAutoNum() const {
	return  m_folio_autonum;
}

/**
 * @brief QETProject::addConductorAutoNum
 * Add a new conductor numerotation context. If key already exist,
 * replace old context by the new context
 * @param key
 * @param context
 */
void QETProject::addConductorAutoNum(QString key, NumerotationContext context) {
	m_conductor_autonum.insert(key, context);
}

/**
 * @brief QETProject::addElementAutoNum
 * Add a new element numerotation context. If key already exist,
 * replace old context by the new context
 * @param key
 * @param context
 */
void QETProject::addElementAutoNum(QString key, NumerotationContext context)
{
	m_element_autonum.insert(key, context);
	emit elementAutoNumAdded(key);
}

/**
 * @brief QETProject::addFolioAutoNum
 * Add a new folio numerotation context. If key already exist,
 * replace old context by the new context
 * @param key
 * @param context
 */
void QETProject::addFolioAutoNum(QString key, NumerotationContext context) {
	m_folio_autonum.insert(key, context);
}

/**
 * @brief QETProject::removeConductorAutoNum
 * Remove Conductor Numerotation Context stored with key
 * @param key
 */
void QETProject::removeConductorAutoNum(QString key) {
	m_conductor_autonum.remove(key);
}

/**
 * @brief QETProject::removeElementAutonum
 * Remove Element Numerotation Context stored with key
 * @param key
 */
void QETProject::removeElementAutoNum(QString key)
{
	m_element_autonum.remove(key);
	emit elementAutoNumRemoved(key);
}

/**
 * @brief QETProject::removeFolioAutonum
 * Remove Folio Numerotation Context stored with key
 * @param key
 */
void QETProject::removeFolioAutoNum(QString key) {
	m_folio_autonum.remove(key);
}

/**
 * @brief QETProject::conductorAutoNum
 * Return conductor numerotation context stored with @key.
 * If key is not found, return an empty numerotation context
 * @param key
 */
NumerotationContext QETProject::conductorAutoNum (const QString &key) const {
	if (m_conductor_autonum.contains(key)) return m_conductor_autonum[key];
	else return NumerotationContext();
}

/**
 * @brief QETProject::elementAutoNum
 * Return element numerotation context stored with @key.
 * If key is not found, return an empty numerotation context
 * @param key
 */
NumerotationContext QETProject::elementAutoNum (const QString &key) {
	if (m_element_autonum.contains(key)) return m_element_autonum[key];
	else return NumerotationContext();
}

/**
 * @brief QETProject::folioAutoNum
 * Return folio numerotation context stored with @key.
 * If key is not found, return an empty numerotation context
 * @param key
 */
NumerotationContext QETProject::folioAutoNum (const QString &key) const {
	if (m_folio_autonum.contains(key)) return m_folio_autonum[key];
	else return NumerotationContext();
}

/**
 * @brief QETProject::freezeExistentConductorLabel
 * Freeze Existent Conductors in the selected folios
 * @param from - first folio index to apply freeze
 * @param to - last folio index to apply freeze
 */
void QETProject::freezeExistentConductorLabel(bool freeze, int from, int to) {
	for (int i = from; i <= to; i++) {
		diagrams_.at(i)->freezeConductors(freeze);
	}
}

/**
 * @brief QETProject::freezeNewConductorLabel
 * Freeze New Conductors in the selected folios
 * @param from - first folio index to apply freeze
 * @param to - last folio index to apply freeze
 */
void QETProject::freezeNewConductorLabel(bool freeze, int from, int to) {
	for (int i = from; i <= to; i++) {
		diagrams_.at(i)->setFreezeNewConductors(freeze);
	}
}

/**
 * @brief QETProject::isFreezeNewConductors
 * @return freeze new conductors Project Wide status
 */
bool QETProject::isFreezeNewConductors() {
	return m_freeze_new_conductors;
}

/**
 * @brief QETProject::setfreezeNewConductors
 * Set Project Wide freeze new conductors
 */
void QETProject::setFreezeNewConductors(bool set) {
	m_freeze_new_conductors = set;
}

/**
 * @brief QETProject::freezeExistentElementLabel
 * Freeze Existent Elements in the selected folios
 * @param from - first folio index to apply freeze
 * @param to - last folio index to apply freeze
 */
void QETProject::freezeExistentElementLabel(bool freeze, int from, int to) {
	for (int i = from; i <= to; i++) {
		diagrams_.at(i)->freezeElements(freeze);
	}
}

/**
 * @brief QETProject::freezeNewElementLabel
 * Freeze New Elements in the selected folios
 * @param from - first folio index to apply freeze
 * @param to - last folio index to apply freeze
 */
void QETProject::freezeNewElementLabel(bool freeze, int from, int to) {
	for (int i = from; i <= to; i++) {
		diagrams_.at(i)->setFreezeNewElements(freeze);
	}
}

/**
 * @brief QETProject::freezeNewElements
 * @return freeze new elements Project Wide status
 */
bool QETProject::isFreezeNewElements() {
	return m_freeze_new_elements;
}

/**
 * @brief QETProject::setfreezeNewElements
 * Set Project Wide freeze new elements
 */
void QETProject::setFreezeNewElements(bool set) {
	m_freeze_new_elements = set;
}

/**
 * @brief QETProject::autoConductor
 * @return true if use of auto conductor is authorized.
 * See also Q_PROPERTY autoConductor
 */
bool QETProject::autoConductor() const
{
	return m_auto_conductor;
}

/**
 * @brief QETProject::setAutoConductor
 * @param ac
 * Enable the use of auto conductor if true
 * See also Q_PROPERTY autoConductor
 */
void QETProject::setAutoConductor(bool ac)
{
	if (ac != m_auto_conductor)
		m_auto_conductor = ac;
}

/**
 * @brief QETProject::autoFolioNumberingNewFolios
 * emit Signal to add new Diagram with autonum
 * properties
 */
void QETProject::autoFolioNumberingNewFolios(){
	emit addAutoNumDiagram();
}

/**
 * @brief QETProject::autoFolioNumberingNewFolios
 * @param autonum used, index from selected tabs "from" and "to"
 * rename folios with selected autonum
 */
void QETProject::autoFolioNumberingSelectedFolios(int from, int to, QString autonum){
	int total_folio = diagrams_.count();
	DiagramContext project_wide_properties = project_properties_;
	for (int i=from; i<=to; i++) {
		QString title = diagrams_[i] -> title();
		NumerotationContext nC = folioAutoNum(autonum);
		NumerotationContextCommands nCC = NumerotationContextCommands(nC);
		diagrams_[i] -> border_and_titleblock.setFolio("%autonum");
		diagrams_[i] -> border_and_titleblock.setFolioData(i + 1, total_folio, nCC.toRepresentedString(), project_wide_properties);
		diagrams_[i] -> project() -> addFolioAutoNum(autonum,nCC.next());
		diagrams_[i] -> update();
	}
}

/**
	@return un document XML representant le projet 
*/
QDomDocument QETProject::toXml() {
	// racine du projet
	QDomDocument xml_doc;
	QDomElement project_root = xml_doc.createElement("project");
	project_root.setAttribute("version", QET::version);
	project_root.setAttribute("title", project_title_);

	// write the present value of folioSheetsQuantity to XML.
	project_root.setAttribute("folioSheetQuantity", QString::number(folioSheetsQuantity));
	xml_doc.appendChild(project_root);
	
	// titleblock templates, if any
	if (titleblocks_.templates().count()) {
		QDomElement titleblocktemplates_elmt = xml_doc.createElement("titleblocktemplates");
		foreach (QString template_name, titleblocks_.templates()) {
			QDomElement e = titleblocks_.getTemplateXmlDescription(template_name);
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
	
	// qDebug() << "Export XML de" << diagrams_.count() << "schemas";
	int order_num = 1;
	foreach(Diagram *diagram, diagrams_) {

		// Write the diagram to XML only if it is not of type DiagramFolioList.
		DiagramFolioList *ptr = dynamic_cast<DiagramFolioList *>(diagram);
		if ( !ptr ) {
			qDebug() << qPrintable(QString("QETProject::toXml() : exporting diagram \"%1\"").arg(diagram -> title())) << "[" << diagram << "]";
			QDomNode appended_diagram = project_root.appendChild(diagram -> writeXml(xml_doc));
			appended_diagram.toElement().setAttribute("order", order_num ++);
		}
	}
	
		//Write the elements collection.
	project_root.appendChild(m_elements_collection->root().cloneNode(true));
	
	return(xml_doc);
}

/**
	Ferme le projet
*/
bool QETProject::close() {
	return(true);
}

/**
 * @brief QETProject::write
 * Save the project in a file
 * @see filePath()
 * @see setFilePath()
 * @return true if the project was successfully saved, else false
 */
QETResult QETProject::write()
{
		// this operation requires a filepath
	if (file_path_.isEmpty())
		return(QString("unable to save project to file: no filepath was specified"));

		// if the project was opened read-only and the file is still non-writable, do not save the project
	if (isReadOnly() && !QFileInfo(file_path_).isWritable())
		return(QString("the file %1 was opened read-only and thus will not be written").arg(file_path_));

		//Get the project in xml
	QDomDocument xml_project;
	xml_project.appendChild(xml_project.importNode(toXml().documentElement(), true));

	QString error_message;
	if (!QET::writeXmlFile(xml_project, file_path_, &error_message)) return(error_message);

	setModified(false);
	return(QETResult());
}

/**
	@return true si le projet est en mode readonly, false sinon
*/
bool QETProject::isReadOnly() const {
	return(read_only_ && read_only_file_path_ == file_path_);
}

/**
 * @brief QETProject::setReadOnly
 * Set this project to read only if @read_only = true
 * @param read_only
 */
void QETProject::setReadOnly(bool read_only)
{
	if (read_only_ != read_only)
	{
			//keep the file to which this project is read-only
		read_only_file_path_ = file_path_;		
		read_only_ = read_only;
		emit(readOnlyChanged(this, read_only));
	}
}

/**
	@return true si le projet peut etre considere comme vide, c'est-a-dire :
	  - soit avec une collection embarquee vide
	  - soit avec uniquement des schemas consideres comme vides
	  - soit avec un titre de projet
*/
bool QETProject::isEmpty() const {
	// si le projet a un titre, on considere qu'il n'est pas vide
	if (!project_title_.isEmpty()) return(false);
	
	//@TODO check if the embedded element collection is empty
	
	// compte le nombre de schemas non vides
	int pertinent_diagrams = 0;
	foreach(Diagram *diagram, diagrams_) {
		if (!diagram -> isEmpty()) ++ pertinent_diagrams;
	}
	
	return(pertinent_diagrams > 0);
}

/**
 * @brief QETProject::importElement
 * Import the element represented by @location to the embbeded collection of this project
 * @param location
 * @return the location of the imported element, location can be null.
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

			//@existing_location and @location have the same uuid, so it is the same element
		if (existing_location.uuid() == location.uuid()) {
			return existing_location;
		}

		ImportElementDialog ied;
		if (ied.exec() == QDialog::Accepted) {
			QET::Action action = ied.action();

				//Use the exisitng element
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
					QString new_path = parent_path + "/" + name_ + QString::number(a) + ".elmt";
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
		ElementsLocation loc(m_elements_collection->addElement(location), this);

		if (!loc.exist()) {
			qDebug() << "QETProject::importElement : failed to import location. " << location;
			return ElementsLocation();
		}
		else {
			return loc;
		}
	}

	return ElementsLocation();
}

/**
	Integrate a title block template into this project.
	@param src_tbt The location of the title block template to be integrated into this project
	@param handler 
	@return the name of the template after integration, or an empty QString if a problem occured.
*/
QString QETProject::integrateTitleBlockTemplate(const TitleBlockTemplateLocation &src_tbt, MoveTitleBlockTemplatesHandler *handler) {
	TitleBlockTemplateLocation dst_tbt(src_tbt.name(), &titleblocks_);
	
	// check whether a TBT having the same name already exists within this project
	QString target_name = dst_tbt.name();
	while (titleblocks_.templates().contains(target_name))
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
	
	if (!titleblocks_.setTemplateXmlDescription(target_name, src_tbt.getTemplateXmlDescription()))
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
 * @brief QETProject::unusedElements
 * @return the list of unused element (exactly her location)
 * An unused element, is an element present in the embedded collection but not present in a diagram of this project.
 * Be aware that an element can be not present in a diagram,
 * but managed by an undo command (delete an element), so an unused element can be used after an undo.
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
	// a diagram can only use a title block template embedded wihtin its parent project
	if (location.parentProject() != this) return(false);
	
	foreach (Diagram *diagram, diagrams()) {
		if (diagram -> usesTitleBlockTemplate(location.name())) {
			return(true);
		}
	}
	return(false);
}

/**
	Gere la reecriture du projet
*/
void QETProject::componentWritten() {
	// reecrit tout le projet
	write();
}

/**
	Ajoute un nouveau schema au projet et emet le signal diagramAdded
*/
Diagram *QETProject::addNewDiagram() {
	// ne fait rien si le projet est en lecture seule
	if (isReadOnly()) return(0);
	
	// cree un nouveau schema
	Diagram *diagram = new Diagram(this);
	
	// lui transmet les parametres par defaut
	diagram -> border_and_titleblock.importBorder(defaultBorderProperties());
	diagram -> border_and_titleblock.importTitleBlock(defaultTitleBlockProperties());
	diagram -> defaultConductorProperties = defaultConductorProperties();
	
	addDiagram(diagram);
	emit(diagramAdded(this, diagram));
	return(diagram);
}

/**
 * @brief QETProject::addNewDiagramFolioList
 * Add new diagram folio list
 * @return the created diagram
 */
QList <Diagram *> QETProject::addNewDiagramFolioList() {
	// do nothing if project is read only or folio sheet is alredy created
	QList <Diagram *> diagram_list;

	if (!isReadOnly() && getFolioSheetsQuantity() == 0) {

		//reset the number of folio sheet
		setFolioSheetsQuantity(0);

		int diagCount = diagrams().size();
		for (int i = 0; i <= diagCount/29; i++) {

			//create new diagram
			Diagram *diagram_folio_list = new DiagramFolioList(this);

			// setup default properties
			diagram_folio_list -> border_and_titleblock.importBorder(defaultBorderProperties());
			diagram_folio_list -> border_and_titleblock.importTitleBlock(defaultTitleBlockProperties());
			diagram_folio_list -> defaultConductorProperties = defaultConductorProperties();

			diagram_folio_list -> border_and_titleblock.setTitle(tr("Liste des Folios"));
			// no need to display rows and columns
			diagram_folio_list -> border_and_titleblock.displayRows(false);
			diagram_folio_list -> border_and_titleblock.displayColumns(false);

			addDiagram(diagram_folio_list);
			setFolioSheetsQuantity( getFolioSheetsQuantity()+1 );
			emit(diagramAdded(this, diagram_folio_list));
			diagram_list << diagram_folio_list;
			diagCount++;
		}
	}

	return(diagram_list);
}

/**
	Enleve un schema du projet et emet le signal diagramRemoved
	@param diagram le schema a enlever
*/
void QETProject::removeDiagram(Diagram *diagram) {
	// ne fait rien si le projet est en lecture seule
	if (isReadOnly()) return;	
	if (!diagram || !diagrams_.contains(diagram)) return;

	if (diagrams_.removeAll(diagram)) {
		emit(diagramRemoved(this, diagram));
		delete diagram;
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
	
	int diagram_max_index = diagrams_.size() - 1;
	if (old_index > diagram_max_index || new_index > diagram_max_index) return;
	
	diagrams_.move(old_index, new_index);
	updateDiagramsFolioData();
	setModified(true);
	emit(projectDiagramsOrderChanged(this, old_index, new_index));
}

/**
	Mark this project as modified and emit the projectModified() signal.
*/
void QETProject::setModified(bool modified) {
	if (modified_ != modified) {
		modified_ = modified;
		emit(projectModified(this, modified_));
		emit(projectInformationsChanged(this));
	}
}

/**
	Set up signals/slots connections related to the title block templates
	collection.
*/
void QETProject::setupTitleBlockTemplatesCollection() {
	connect(
		&titleblocks_,
		SIGNAL(changed(TitleBlockTemplatesCollection *, const QString &)),
		this,
		SLOT(updateDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *, const QString &))
	);
	connect(
		&titleblocks_,
		SIGNAL(aboutToRemove(TitleBlockTemplatesCollection *, const QString &)),
		this,
		SLOT(removeDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *, const QString &))
	);
}

/**
 * @brief QETProject::readProjectXml
 * Read and make the project from an xml description
 * @param xml_project : the description of the project from an xml
 */
void QETProject::readProjectXml(QDomDocument &xml_project)
{
	QDomElement root_elmt = xml_project.documentElement();
	state_ = ProjectParsingRunning;
	
		//The roots of the xml document must be a "project" element
	if (root_elmt.tagName() == "project")
	{
			//Normal opening mode
		if (root_elmt.hasAttribute("version"))
		{
			bool conv_ok;
			project_qet_version_ = root_elmt.attribute("version").toDouble(&conv_ok);

			if (conv_ok && QET::version.toDouble() < project_qet_version_)
			{
				int ret = QET::QetMessageBox::warning(
							  0,
							  tr("Avertissement", "message box title"),
							  tr(
								  "Ce document semble avoir été enregistré avec "
								  "une version ultérieure de QElectroTech. Il est "
								  "possible que l'ouverture de tout ou partie de ce "
								  "document échoue.\n"
								  "Que désirez vous faire ?",
								  "message box content"
								  ),
							  QMessageBox::Open | QMessageBox::Cancel
							  );
				
				if (ret == QMessageBox::Cancel)
				{
					state_ = FileOpenDiscard;
					return;
				}
			}
		}
		setTitle(root_elmt.attribute("title"));
	}
	else
	{
		state_ = ProjectParsingFailed;
	}
	
		//Load the project-wide properties
	readProjectPropertiesXml(xml_project);
		//Load the default properties for the new diagrams
	readDefaultPropertiesXml(xml_project);
		//load the embedded titleblock templates
	titleblocks_.fromXml(xml_project.documentElement());
		//Load the embedded elements collection
	readElementsCollectionXml(xml_project);
		//Load the diagrams
	readDiagramsXml(xml_project);

		// if there is an attribute for folioSheetQuantity, then set it accordingly.
		// If not, then the value remains at the initial value of zero.
	if (root_elmt.attribute("folioSheetQuantity","0").toInt())
		addNewDiagramFolioList();
	
	state_ = Ok;
}

/**
 * @brief QETProject::readDiagramsXml
 * Load the diagrams from the xml description of the project.
 * Note a project can have 0 diagram
 * @param xml_project
 */
void QETProject::readDiagramsXml(QDomDocument &xml_project)
{
	QMultiMap<int, Diagram *> loaded_diagrams;
	
	//@TODO try to solve a weird bug (dialog is black) since port to Qt5 with the DialogWaiting
	//show DialogWaiting
	DialogWaiting* dlgWaiting = new DialogWaiting();
	dlgWaiting -> setModal(true);
	dlgWaiting -> show();
	dlgWaiting -> setTitle( tr("<b>Ouverture du projet en cours...</b>") );
	
		//Search the diagrams in the project
	QDomNodeList diagram_nodes = xml_project.elementsByTagName("diagram");
	dlgWaiting->setProgressBarRange(0, diagram_nodes.length());
	for (int i = 0 ; i < diagram_nodes.length() ; ++ i)
	{
		dlgWaiting->setProgressBar(i+1);
		if (diagram_nodes.at(i).isElement())
		{
			QDomElement diagram_xml_element = diagram_nodes.at(i).toElement();
			Diagram *diagram = new Diagram(this);
			bool diagram_loading = diagram -> initFromXml(diagram_xml_element);
			if (diagram_loading)
			{
				dlgWaiting->setDetail( diagram->title() );
					//Get the attribute "order" of the diagram
				int diagram_order = -1;
				if (!QET::attributeIsAnInteger(diagram_xml_element, "order", &diagram_order)) diagram_order = 500000;
				loaded_diagrams.insert(diagram_order, diagram);
			}
			else
			{
				delete diagram;
			}
		}
	}
	
		//Add the diagrams according to there "order" attribute
	foreach(Diagram *diagram, loaded_diagrams.values())
		addDiagram(diagram);

		//Initialise links between elements in this project
		//and refresh the text of conductor
	foreach (Diagram *d, diagrams())
		d->refreshContents();

	delete dlgWaiting;
}

/**
 * @brief QETProject::readElementsCollectionXml
 * Load the diagrams from the xml description of the project
 * @param xml_project : the xml description of the project
 */
void QETProject::readElementsCollectionXml(QDomDocument &xml_project)
{
		//Get the embedded elements collection of the project
	QDomNodeList collection_roots = xml_project.elementsByTagName("collection");
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
 * @brief QETProject::readProjectPropertiesXml
 * Load project properties from the XML description of the project
 * @param xml_project : the xml description of the project
 */
void QETProject::readProjectPropertiesXml(QDomDocument &xml_project)
{
	foreach (QDomElement e, QET::findInDomElement(xml_project.documentElement(), "properties"))
		project_properties_.fromXml(e);
}

/**
 * @brief QETProject::readDefaultPropertiesXml
 * load default properties for new diagram, found in the xml of this project
 * or by default find in the QElectroTech global conf
 * @param xml_project : the xml description of the project
 */
void QETProject::readDefaultPropertiesXml(QDomDocument &xml_project)
{
		// Find xml element where is stored properties for new diagram
	QDomNodeList newdiagrams_nodes = xml_project.elementsByTagName("newdiagrams");
	if (newdiagrams_nodes.isEmpty()) return;

	QDomElement newdiagrams_elmt = newdiagrams_nodes.at(0).toElement();

		// By default, use value find in the global conf of QElectroTech
	default_border_properties_	   = BorderProperties::    defaultProperties();
	default_titleblock_properties_ = TitleBlockProperties::defaultProperties();
	default_conductor_properties_  = ConductorProperties:: defaultProperties();
	default_report_properties_	   = ReportProperties::    defaultProperties();
	m_default_xref_properties	   = XRefProperties::      defaultProperties();

		//Read values indicate in project
	QDomElement border_elmt, titleblock_elmt, conductors_elmt, report_elmt, xref_elmt, conds_autonums, folio_autonums, element_autonums;

	for (QDomNode child = newdiagrams_elmt.firstChild() ; !child.isNull() ; child = child.nextSibling())
	{
		QDomElement child_elmt = child.toElement();
		if (child_elmt.isNull()) continue;

		if (child_elmt.tagName() == "border")
			border_elmt = child_elmt;
		else if (child_elmt.tagName() == "inset")
			titleblock_elmt = child_elmt;
		else if (child_elmt.tagName() == "conductors")
			conductors_elmt = child_elmt;
		else if (child_elmt.tagName() == "report")
			report_elmt = child_elmt;
		else if (child_elmt.tagName() == "xrefs")
			xref_elmt = child_elmt;
		else if (child_elmt.tagName() == "conductors_autonums")
			conds_autonums = child_elmt;
		else if (child_elmt.tagName()== "folio_autonums")
			folio_autonums = child_elmt;
		else if (child_elmt.tagName()== "element_autonums")
			element_autonums = child_elmt;
	}

		// size, titleblock, conductor, report, conductor autonum, folio autonum, element autonum
	if (!border_elmt.isNull())	   default_border_properties_.fromXml(border_elmt);
	if (!titleblock_elmt.isNull()) default_titleblock_properties_.fromXml(titleblock_elmt);
	if (!conductors_elmt.isNull()) default_conductor_properties_.fromXml(conductors_elmt);
	if (!report_elmt.isNull())	   setDefaultReportProperties(report_elmt.attribute("label"));
	if (!xref_elmt.isNull())
	{
		foreach(QDomElement elmt, QET::findInDomElement(xref_elmt, "xref"))
		{
			XRefProperties xrp;
			xrp.fromXml(elmt);
			m_default_xref_properties.insert(elmt.attribute("type"), xrp);
		}
	}
	if (!conds_autonums.isNull())
	{
		m_current_conductor_autonum = conds_autonums.attribute("current_autonum");
		m_freeze_new_conductors = conds_autonums.attribute("freeze_new_conductors") == "true";
		foreach (QDomElement elmt, QET::findInDomElement(conds_autonums, "conductor_autonum"))
		{
			NumerotationContext nc;
			nc.fromXml(elmt);
			m_conductor_autonum.insert(elmt.attribute("title"), nc);
		}
	}
	if (!folio_autonums.isNull())
	{
		foreach (QDomElement elmt, QET::findInDomElement(folio_autonums, "folio_autonum"))
		{
			NumerotationContext nc;
			nc.fromXml(elmt);
			m_folio_autonum.insert(elmt.attribute("title"), nc);
		}
	}
	if (!element_autonums.isNull())
	{
		m_current_element_autonum = element_autonums.attribute("current_autonum");
		m_freeze_new_elements = element_autonums.attribute("freeze_new_elements") == "true";
		foreach (QDomElement elmt, QET::findInDomElement(element_autonums, "element_autonum"))
		{
			NumerotationContext nc;
			nc.fromXml(elmt);
			m_element_autonum.insert(elmt.attribute("title"), nc);
		}
	}
}

/**
	Export project properties under the \a xml_element XML element.
*/
void QETProject::writeProjectPropertiesXml(QDomElement &xml_element) {
	project_properties_.toXml(xml_element);
}

/**
 * @brief QETProject::writeDefaultPropertiesXml
 * Export all defaults properties used by a new diagram and his content
 * #size of border
 * #content of titleblock
 * #default conductor
 * #defaut folio report
 * #default Xref
 * @param xml_element xml element to use for store default propertie.
 */
void QETProject::writeDefaultPropertiesXml(QDomElement &xml_element) {
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
	foreach (QString key, defaultXRefProperties().keys()) {
		QDomElement xref_elmt = xml_document.createElement("xref");
		xref_elmt.setAttribute("type", key);
		defaultXRefProperties()[key].toXml(xref_elmt);
		xrefs_elmt.appendChild(xref_elmt);
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
	Cette methode ajoute un schema donne au projet
	@param diagram Schema a ajouter
*/
/**
 * @brief QETProject::addDiagram
 * Add a diagram in this project
 * @param diagram added diagram
 * @param position postion of the new diagram, by default at the end
 */
void QETProject::addDiagram(Diagram *diagram) {
	if (!diagram) return;
	
	// Ensure diagram know is parent project
	diagram -> setProject(this);
	
	// If diagram is write, we must rewrite the project
	connect(diagram, SIGNAL(written()), this, SLOT(componentWritten()));
	connect(
		&(diagram -> border_and_titleblock),
		SIGNAL(needFolioData()),
		this,
		SLOT(updateDiagramsFolioData())
	);
	connect(
		diagram, SIGNAL(usedTitleBlockTemplateChanged(const QString &)),
		this, SLOT(usedTitleBlockTemplateChanged(const QString &))
	);
	
	// add diagram to project
		diagrams_ << diagram;
	
	updateDiagramsFolioData();
}

/**
	@return La liste des noms a utiliser pour la categorie dediee aux elements
	integres automatiquement dans le projet.

*/
NamesList QETProject::namesListForIntegrationCategory() {
	NamesList names;

	const QChar russian_data[24] = { 0x0418, 0x043C, 0x043F, 0x043E, 0x0440, 0x0442, 0x0438, 0x0440, 0x043E, 0x0432, 0x0430, 0x043D, 0x043D, 0x044B, 0x0435, 0x0020, 0x044D, 0x043B, 0x0435, 0x043C, 0x0435, 0x043D, 0x0442, 0x044B };
	const QChar greek_data[18] = { 0x0395, 0x03b9, 0x03c3, 0x03b7, 0x03b3, 0x03bc, 0x03ad, 0x03bd, 0x03b1, 0x0020, 0x03c3, 0x03c4, 0x03bf, 0x03b9, 0x03c7, 0x03b5, 0x03af, 0x03b1 };

	names.addName("fr", "Éléments importés");
	names.addName("en", "Imported elements");
	names.addName("de", "Importierte elemente");
	names.addName("es", "Elementos importados");
	names.addName("ru", QString(russian_data, 24));
	names.addName("cs", "Zavedené prvky");
	names.addName("pl", "Elementy importowane");
	names.addName("pt", "elementos importados");
	names.addName("it", "Elementi importati");
	names.addName("el", QString(greek_data, 18));
	names.addName("nl", "Elementen geïmporteerd");
	names.addName("hr", "Uvezeni elementi");
	names.addName("ca", "Elements importats");
	names.addName("ro", "Elemente importate");

	return(names);
}

/**
	@return true if project options (title, project-wide properties, settings
	for new diagrams, diagrams order...) were modified, false otherwise.
*/
bool QETProject::projectOptionsWereModified() {
	// unlike similar methods, this method does not compare the content against
	// expected values; instead, we just check whether we have been set as modified.
	return(modified_);
}

/**
	@return the project-wide properties made available to child diagrams.
*/
DiagramContext QETProject::projectProperties() {
	return(project_properties_);
}

/**
	Use \a context as project-wide properties made available to child diagrams.
*/
void QETProject::setProjectProperties(const DiagramContext &context) {
	project_properties_ = context;
	updateDiagramsFolioData();
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
bool QETProject::projectWasModified() {

	if ( projectOptionsWereModified()    ||
		 !undo_stack_ -> isClean()       ||
		 titleblocks_.templates().count() )
		return(true);
	
	else
		return(false);
}

/**
	Indique a chaque schema du projet quel est son numero de folio et combien de
	folio le projet contient.
*/
void QETProject::updateDiagramsFolioData() {
	int total_folio = diagrams_.count();
	
	DiagramContext project_wide_properties = project_properties_;
	project_wide_properties.addValue("projecttitle", title());
	
	for (int i = 0 ; i < total_folio ; ++ i) {
		QString title = diagrams_[i] -> title();
		QString autopagenum = diagrams_[i]->border_and_titleblock.autoPageNum();
		NumerotationContext nC = folioAutoNum(autopagenum);
		NumerotationContextCommands nCC = NumerotationContextCommands(nC);
		if((diagrams_[i]->border_and_titleblock.folio().contains("%autonum"))&&(!autopagenum.isNull())){
			diagrams_[i] -> border_and_titleblock.setFolioData(i + 1, total_folio, nCC.toRepresentedString(), project_wide_properties);
			diagrams_[i]->project()->addFolioAutoNum(autopagenum,nCC.next());
		}
		else{
		diagrams_[i] -> border_and_titleblock.setFolioData(i + 1, total_folio, NULL, project_wide_properties);
		}
		diagrams_[i] -> update();
	}
}

/**
	Inform each diagram that the \a template_name title block changed.
	@param collection Title block templates collection
	@param template_name Name of the changed template
*/
void QETProject::updateDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *collection, const QString &template_name) {
	Q_UNUSED(collection)
	
	foreach (Diagram *diagram, diagrams_) {
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
	foreach (Diagram *diagram, diagrams_) {
		diagram -> titleBlockTemplateRemoved(template_name);
	}
}

/**
	Handles the fact a digram changed the title block template it used
	@param template_name Name of the template
*/
void QETProject::usedTitleBlockTemplateChanged(const QString &template_name) {
	emit(diagramUsedTemplate(embeddedTitleBlockTemplatesCollection(), template_name));
}
