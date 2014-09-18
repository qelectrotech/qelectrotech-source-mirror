/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "elementdefinition.h"
#include "xmlelementscollection.h"
#include "elementscategory.h"
#include "qetapp.h"
#include "qetdiagrameditor.h"
#include "qetresult.h"
#include "integrationmoveelementshandler.h"
#include "movetemplateshandler.h"
#include "basicmoveelementshandler.h"
#include "qetmessagebox.h"
#include "titleblocktemplate.h"
#include "ui/dialogwaiting.h"
#include "numerotationcontext.h"

QString QETProject::integration_category_name = "import";

/**
	Constructeur par defaut - cree un schema contenant une collection
	d'elements vide et un schema vide.
	@param diagrams Nombre de nouveaux schemas a ajouter a ce nouveau projet
	@param parent QObject parent
*/
QETProject::QETProject(int diagrams, QObject *parent) :
	QObject(parent),
	collection_(0),
	project_qet_version_(-1),
	modified_(false),
	read_only_(false),
	titleblocks_(this),
	folioSheetsQuantity(0)
{
	// 0 a n schema(s) vide(s)
	int diagrams_count = qMax(0, diagrams);
	for (int i = 0 ; i < diagrams_count ; ++ i) {
		addNewDiagram();
	}

	// une collection d'elements vide
	collection_ = new XmlElementsCollection();
	collection_ -> setProtocol("embed");
	collection_ -> setProject(this);
	connect(collection_, SIGNAL(written()), this, SLOT(componentWritten()));
	
	// une categorie dediee aux elements integres automatiquement
	ensureIntegrationCategoryExists();
	setupTitleBlockTemplatesCollection();

	undo_stack_ = new QUndoStack();
	connect(undo_stack_, SIGNAL(cleanChanged(bool)), this, SLOT(undoStackChanged(bool)));
}

/**
	Construit un projet a partir du chemin d'un fichier.
	@param path Chemin du fichier
	@param parent QObject parent
*/
QETProject::QETProject(const QString &path, QObject *parent) :
	QObject(parent),
	collection_(0),
	project_qet_version_(-1),
	modified_(false),
	read_only_(false),
	titleblocks_(this),
	folioSheetsQuantity(0)
{
	// ouvre le fichier
	QFile project_file(path);
	if (!project_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		state_ = FileOpenFailed;
		return;
	}
	setFilePath(path);
	
	// en extrait le contenu XML
	bool xml_parsing = document_root_.setContent(&project_file);
	if (!xml_parsing) {
		state_ = XmlParsingFailed;
		return;
	}
	
	// et construit le projet
	readProjectXml();
	
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
	Construit un projet a partir d'un element XML representant le projet.
	L'element XML fourni est copie et conserve dans la classe.
*/
QETProject::QETProject(const QDomElement &xml_element, QObject *parent) :
	QObject(parent),
	collection_(0),
	project_qet_version_(-1),
	modified_(false),
	read_only_(false),
	titleblocks_(this),
	folioSheetsQuantity(0)
{
	// copie le contenu XML
	document_root_.appendChild(document_root_.importNode(xml_element, true));
	
	// et construit le projet
	readProjectXml();
	
	setupTitleBlockTemplatesCollection();

	undo_stack_ = new QUndoStack();
	connect(undo_stack_, SIGNAL(cleanChanged(bool)), this, SLOT(undoStackChanged(bool)));
}

/**
	Destructeur
*/
QETProject::~QETProject() {
	// supprime les schemas
	// qDebug() << "Suppression du projet" << ((void *)this);
	
	// supprime la collection
	// qDebug() << "Suppression de la collection du projet" << ((void *)this);
	if (collection_) {
		delete collection_;
	}
	// qDebug() << "Collection du projet" << ((void *)this) << "supprimee";
	
	// qDebug() << diagrams_;
	foreach (Diagram *diagram, diagrams_) {
		diagrams_.removeAll(diagram);
		delete diagram;
	}

	folioSheetsQuantity = 0;
	// qDebug() << diagrams_;
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
	@return la collection embarquee de ce projet
*/
ElementsCollection *QETProject::embeddedCollection() const {
	return(collection_);
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
		current_directory = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
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
				"Projet \253\240%1\240\273",
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
				"%1 [modifi\351]",
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
	@return the list of the titleblock templates embedded within this project 
*/
QList<QString> QETProject::embeddedTitleBlockTemplates() {
	return(titleblocks_.templates());
}

/**
	@param template_name Name of the requested template
	@return the requested template, or 0 if there is no valid template of this
	name within the project
*/
const TitleBlockTemplate *QETProject::getTemplateByName(const QString &template_name) {
	return(titleblocks_.getTemplate(template_name));
}

/**
	@param template_name Name of the requested template
	@return the XML description of the requested template, or a null QDomElement
	if the project does not have such an titleblock template
*/
QDomElement QETProject::getTemplateXmlDescriptionByName(const QString &template_name) {
	return(titleblocks_.getTemplateXmlDescription(template_name));
}

/**
	This methods allows adding or modifying a template embedded within the
	project.
	@param template_name Name / Identifier of the template - will be used to
	determine whether the given description will be added or will replace an
	existing one.
	@param xml_elmt An \<titleblocktemplate\> XML element describing the
	template. Its "name" attribute must equal to template_name.
	@return false if a problem occured, true otherwise
*/
bool QETProject::setTemplateXmlDescription(const QString &template_name, const QDomElement &xml_elmt) {
	return(titleblocks_.setTemplateXmlDescription(template_name, xml_elmt));
}

/**
	This methods allows removing a template embedded within the project.
	@param template_name Name of the template to be removed
*/
void QETProject::removeTemplateByName(const QString &template_name) {
	return(titleblocks_.removeTemplate(template_name));
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
	Permet de specifier le cartouche par defaut utilise lors de la creation
	d'un nouveau schema dans ce projet.
	@param titleblock Cartouche d'un schema
*/
void QETProject::setDefaultTitleBlockProperties(const TitleBlockProperties &titleblock) {
	default_titleblock_properties_ = titleblock;
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

void QETProject::setDefaultXRefProperties(QHash<QString, XRefProperties> hash) {
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
 * @brief QETProject::addConductorAutoNum
 * Add a new numerotation context. If key already exist,
 * replace old context by the new context
 * @param key
 * @param context
 */
void QETProject::addConductorAutoNum(QString key, NumerotationContext context) {
	m_conductor_autonum.insert(key, context);
}

/**
 * @brief QETProject::removeConductorAutonum
 * Remove the Numerotation Context stored with key
 * @param key
 */
void QETProject::removeConductorAutonum(QString key) {
	m_conductor_autonum.remove(key);
}

/**
 * @brief QETProject::conductorAutoNum
 * Return the numerotation context stored with @key.
 * If key is not found, return an empty numerotation context
 * @param key
 */
NumerotationContext QETProject::conductorAutoNum (const QString &key) const {
	if (m_conductor_autonum.contains(key)) return m_conductor_autonum[key];
	else return NumerotationContext();
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
			qDebug() << qPrintable(QString("QETProject::toXml() : exporting diagram \"%1\" [%2]").arg(diagram -> title()).arg(QET::pointerString(diagram)));
			QDomNode appended_diagram = project_root.appendChild(diagram -> writeXml(xml_doc));
			appended_diagram.toElement().setAttribute("order", order_num ++);
		}
	}
	
	// collection
	project_root.appendChild(collection_ -> writeXml(xml_doc));
	
	return(xml_doc);
}

/**
	Ferme le projet
*/
bool QETProject::close() {
	return(true);
}

/**
	Enregistre le projet vers un fichier.
	@see filePath()
	@see setFilePath()
	@return true si l'enregistrement a reussi, false sinon
*/
QETResult QETProject::write() {
	// this operation requires a filepath
	if (file_path_.isEmpty()) {
		return(QString("unable to save project to file: no filepath was specified"));
	}
	
	// if the project was opened read-only and the file is still non-writable, do not save the project
	if (isReadOnly() && !QFileInfo(file_path_).isWritable()) {
		return(QString("the file %1 was opened read-only and thus will not be written").arg(file_path_));
	}
	
	// realise l'export en XML du projet dans le document XML interne
	document_root_.clear();
	document_root_.appendChild(document_root_.importNode(toXml().documentElement(), true));
	
	QString error_message;
	bool writing = QET::writeXmlFile(document_root_, file_path_, &error_message);
	if (!writing) {
		return(error_message);
	}
	
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
	@param read_only true pour passer le projet (schemas et collection)
	en mode Read Only, false sinon.
*/
void QETProject::setReadOnly(bool read_only) {
	if (read_only_ != read_only) {
		// memorise le fichier pour lequel ce projet est en lecture seule
		read_only_file_path_ = file_path_;
		
		// applique le nouveau mode aux schemas
		foreach(Diagram *diagram, diagrams()) {
			diagram -> setReadOnly(read_only);
		}
		
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
	
	// si la collection du projet n'est pas vide, alors le projet n'est pas vide
	if (!collection_ -> isEmpty()) return(false);
	
	// compte le nombre de schemas non vides
	int pertinent_diagrams = 0;
	foreach(Diagram *diagram, diagrams_) {
		if (!diagram -> isEmpty()) ++ pertinent_diagrams;
	}
	
	return(pertinent_diagrams > 0);
}

/**
	Cree une categorie dediee aux elements integres automatiquement dans le
	projet si celle-ci n'existe pas deja.
	@return true si tout s'est bien passe, false sinon
*/
bool QETProject::ensureIntegrationCategoryExists() {
	ElementsCategory *root_cat = rootCategory();
	if (!root_cat) return(false);
	
	if (root_cat -> category(integration_category_name)) return(true);
	
	ElementsCategory *integration_category = root_cat -> createCategory(integration_category_name);
	if (!integration_category) return(false);
	
	integration_category -> setNames(namesListForIntegrationCategory());
	return(true);
}

/**
	@return la categorie dediee aux elements integres automatiquement dans le
	projet ou 0 si celle-ci n'a pu etre creee.
	@see ensureIntegrationCategoryExists()
*/
ElementsCategory *QETProject::integrationCategory() const {
	ElementsCategory *root_cat = rootCategory();
	if (!root_cat) return(0);
	
	return(root_cat -> category(integration_category_name));
}

/**
	Integre un element dans le projet.
	Cette methode delegue son travail a la methode
	integrateElement(const QString &, MoveElementsHandler *, QString &)
	en lui passant un MoveElementsHandler approprie.
	@param elmt_location Emplacement de l'element a integrer
	@param error_msg Reference vers une chaine de caractere qui contiendra
	eventuellement un message d'erreur
	@return L'emplacement de l'element apres integration, ou une chaine vide si
	l'integration a echoue.
*/
QString QETProject::integrateElement(const QString &elmt_location, QString &error_msg) {
	// handler dedie a l'integration d'element
	IntegrationMoveElementsHandler *integ_handler = new IntegrationMoveElementsHandler(0);
	QString integ_path = integrateElement(elmt_location, integ_handler, error_msg);
	delete integ_handler;
	
	return(integ_path);
}

/**
	Integre un element dans le projet.
	Cette methode prend en parametre l'emplacement d'un element a integrer.
	Chaque categorie mentionnee dans le chemin de cet element sera copiee de
	maniere non recursive sous la categorie dediee a l'integration si elle
	n'existe pas deja.
	L'element sera ensuite copiee dans cette copie de la hierarchie d'origine.
	En cas de probleme, error_message sera modifiee de facon a contenir un
	message decrivant l'erreur rencontree.
	@param elmt_path Emplacement de l'element a integrer
	@param handler Gestionnaire a utiliser pour gerer les copies d'elements et categories
	@param error_message Reference vers une chaine de caractere qui contiendra
	eventuellement un message d'erreur
	@return L'emplacement de l'element apres integration, ou une chaine vide si
	l'integration a echoue.
*/
QString QETProject::integrateElement(const QString &elmt_path, MoveElementsHandler *handler, QString &error_message) {
	// on s'assure que le projet a une categorie dediee aux elements importes automatiquement
	if (!ensureIntegrationCategoryExists()) {
		error_message = tr("Impossible de cr\351er la cat\351gorie pour l'int\351gration des \351l\351ments");
		return(QString());
	}
	
	// accede a la categorie d'integration
	ElementsCategory *integ_cat = integrationCategory();
	
	// accede a l'element a integrer
	ElementsCollectionItem *integ_item = QETApp::collectionItem(ElementsLocation::locationFromString(elmt_path));
	ElementDefinition *integ_elmt = integ_item ? integ_item -> toElement() : 0;
	if (!integ_item || !integ_elmt) {
		error_message = tr("Impossible d'acc\351der \340 l'\351l\351ment \340 int\351grer");
		return(QString());
	}
	
	// recopie l'arborescence de l'element de facon non recursive
	QList<ElementsCategory *> integ_par_cat = integ_elmt -> parentCategories();
	ElementsCategory *target_cat = integ_cat;
	foreach(ElementsCategory *par_cat, integ_par_cat) {
		if (par_cat -> isRootCategory()) continue;
		
		if (ElementsCategory *existing_cat = target_cat -> category(par_cat -> pathName())) {
			// la categorie cible existe deja : on continue la progression
			target_cat = existing_cat;
		} else {
			// la categorie cible n'existe pas : on la cree par recopie
			ElementsCollectionItem *result_cat = par_cat -> copy(target_cat, handler, false);
			if (!result_cat || !result_cat -> isCategory()) {
				error_message = QString(tr("Un probl\350me s'est produit pendant la copie de la cat\351gorie %1")).arg(par_cat -> location().toString());
				return(QString());
			}
			target_cat = result_cat -> toCategory();
		}
	}
	
	// recopie l'element
	ElementsLocation result;
	if (ElementDefinition *existing_elmt = target_cat -> element(integ_item -> pathName())) {
		
		// l'element existe deja - on demande au handler ce que l'on doit faire
		QET::Action action = handler -> elementAlreadyExists(integ_elmt, existing_elmt);
		
		if (action == QET::Ignore) {
			// il faut conserver et utiliser l'element deja integre
			result = existing_elmt -> location();
		} else if (action == QET::Erase) {
			// il faut ecraser l'element deja integre
			BasicMoveElementsHandler *erase_handler = new BasicMoveElementsHandler();
			result = copyElementWithHandler(integ_elmt, target_cat, erase_handler, error_message);
			delete erase_handler;
		} else if (action == QET::Rename) {
			// il faut faire cohabiter les deux elements en renommant le nouveau 
			QString integ_element_name = handler -> nameForRenamingOperation();
			BasicMoveElementsHandler *rename_handler = new BasicMoveElementsHandler();
			rename_handler -> setActionIfItemAlreadyExists(QET::Rename);
			rename_handler -> setNameForRenamingOperation(integ_element_name);
			result = copyElementWithHandler(integ_elmt, target_cat, rename_handler, error_message);
			delete rename_handler;
		} else {
			// il faut annuler la pose de l'element
			result = ElementsLocation();
		}
	} else {
		// integre l'element normalement
		result = copyElementWithHandler(integ_elmt, target_cat, handler, error_message);
	}
	
	if (!result.isNull()) emit(elementIntegrated(this, result));
	return(result.toString());
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
	while (titleblocks_.templates().contains(target_name)) {
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
	
	bool integration = setTemplateXmlDescription(
		target_name,
		src_tbt.getTemplateXmlDescription()
	);
	if (!integration) {
		handler -> errorWithATemplate(src_tbt, tr("Une erreur s'est produite durant l'int\351gration du mod\350le.", "error message"));
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
bool QETProject::usesElement(const ElementsLocation &location) {
	foreach(Diagram *diagram, diagrams()) {
		if (diagram -> usesElement(location)) {
			return(true);
		}
	}
	return(false);
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
	Delete all title block templates not used in the project
*/
void QETProject::cleanUnusedTitleBlocKTemplates() {
	titleblocks_.deleteUnusedTitleBlocKTemplates();
}

/**
	Supprime tous les elements inutilises dans le projet
	@param handler Gestionnaire d'erreur
*/
void QETProject::cleanUnusedElements(MoveElementsHandler *handler) {
	ElementsCategory *root_cat = rootCategory();
	if (!root_cat) return;
	
	root_cat -> deleteUnusedElements(handler);
}

/**
	Supprime tous les categories vides (= ne contenant aucun element ou que des
	categories vides) dans le projet
	@param handler Gestionnaire d'erreur
*/
void QETProject::cleanEmptyCategories(MoveElementsHandler *handler) {
	ElementsCategory *root_cat = rootCategory();
	if (!root_cat) return;
	
	root_cat -> deleteEmptyCategories(handler);
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
	Diagram *diagram = new Diagram();
	
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
Diagram *QETProject::addNewDiagramFolioList() {
	// do nothing if project is read only
	if (isReadOnly()) return(0);

	//create new diagram
	Diagram *diagram_folio_list = new DiagramFolioList(this);

	// setup default properties
	diagram_folio_list -> border_and_titleblock.importBorder(defaultBorderProperties());
	diagram_folio_list -> border_and_titleblock.importTitleBlock(defaultTitleBlockProperties());
	diagram_folio_list -> defaultConductorProperties = defaultConductorProperties();

	diagram_folio_list -> border_and_titleblock.setTitle(tr("Liste des Sch\351mas"));
	// no need to display rows and columns
	diagram_folio_list -> border_and_titleblock.displayRows(false);
	diagram_folio_list -> border_and_titleblock.displayColumns(false);


	addDiagram(diagram_folio_list);
	setFolioSheetsQuantity( getFolioSheetsQuantity()+1 );

	emit(diagramAdded(this, diagram_folio_list));
	return(diagram_folio_list);
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
	@return un pointeur vers la categorie racine de la collection embarquee, ou
	0 si celle-ci n'est pas accessible.
*/
ElementsCategory *QETProject::rootCategory() const {
	if (!collection_) return(0);
	
	ElementsCategory *root_cat = collection_ -> rootCategory();
	return(root_cat);
}

/**
	(Re)lit le projet depuis sa description XML
*/
void QETProject::readProjectXml() {
	QDomElement root_elmt = document_root_.documentElement();
	state_ = ProjectParsingRunning;
	
	// la racine du document XML est sensee etre un element "project"
	if (root_elmt.tagName() == "project") {


		// mode d'ouverture normal
		if (root_elmt.hasAttribute("version")) {
			bool conv_ok;
			project_qet_version_ = root_elmt.attribute("version").toDouble(&conv_ok);
			if (conv_ok && QET::version.toDouble() < project_qet_version_) {
				
				int ret = QET::MessageBox::warning(
					0,
					tr("Avertissement", "message box title"),
					tr(
						"Ce document semble avoir \351t\351 enregistr\351 avec "
						"une version ult\351rieure de QElectroTech. Il est "
						"possible que l'ouverture de tout ou partie de ce "
						"document \351choue.\n"
						"Que d\351sirez vous faire ?",
						"message box content"
					),
					QMessageBox::Open | QMessageBox::Cancel
				);
				
				if (ret == QMessageBox::Cancel) {
					state_ = FileOpenDiscard;
					return;
				}
				
			}
		}
		
		setTitle(root_elmt.attribute("title"));
	} else {
		state_ = ProjectParsingFailed;
	}
	
	// load the project-wide properties
	readProjectPropertiesXml();

	// charge les proprietes par defaut pour les nouveaux schemas
	readDefaultPropertiesXml();
	
	// if there is an attribute for folioSheetQuantity, then set it accordingly.
	// If not, then the value remains at the initial value of zero.
	if (root_elmt.hasAttribute("folioSheetQuantity"))
		addNewDiagramFolioList();

	// load the embedded titleblock templates
	readEmbeddedTemplatesXml();
	
	// charge la collection embarquee
	readElementsCollectionXml();
	
	// charge les schemas
	readDiagramsXml();

	state_ = Ok;
}

/**
	Charge les schemas depuis la description XML du projet.
	A noter qu'un projet peut parfaitement ne pas avoir de schema.
*/
void QETProject::readDiagramsXml() {
	// map destinee a accueillir les schemas
	QMultiMap<int, Diagram *> loaded_diagrams;
	
	//show DialogWaiting
	DialogWaiting* dlgWaiting = new DialogWaiting();
	dlgWaiting -> setModal(true);
	dlgWaiting -> show();
	dlgWaiting -> setTitle( tr("<b>Ouverture du projet en cours...</b>") );
	
	// recherche les schemas dans le projet
	QDomNodeList diagram_nodes = document_root_.elementsByTagName("diagram");
	dlgWaiting->setProgressBarRange(0, diagram_nodes.length());
	for (uint i = 0 ; i < diagram_nodes.length() ; ++ i) {
		dlgWaiting->setProgressBar(i+1);
		if (diagram_nodes.at(i).isElement()) {
			QDomElement diagram_xml_element = diagram_nodes.at(i).toElement();
			Diagram *diagram = new Diagram();
			diagram -> setProject(this);
			bool diagram_loading = diagram -> initFromXml(diagram_xml_element);
			if (diagram_loading) {
				dlgWaiting->setDetail( diagram->title() );
				// recupere l'attribut order du schema
				int diagram_order = -1;
				if (!QET::attributeIsAnInteger(diagram_xml_element, "order", &diagram_order)) diagram_order = 500000;
				loaded_diagrams.insert(diagram_order, diagram);
			} else {
				delete diagram;
			}
		}
	}
	
	// ajoute les schemas dans l'ordre indique par les attributs order
	foreach(Diagram *diagram, loaded_diagrams.values()) {
		addDiagram(diagram);
	}
	// Initialise links between elements in this project
	foreach (Diagram *d, diagrams()) {
		d->initElementsLinks();
	}


	//delete dialog object
	delete dlgWaiting;
}

/**
	Loads the embedded template from the XML description of the project
*/
void QETProject::readEmbeddedTemplatesXml() {
	titleblocks_.fromXml(document_root_.documentElement());
}

/**
	Charge les schemas depuis la description XML du projet
*/
void QETProject::readElementsCollectionXml() {
	// recupere la collection d'elements integreee au projet
	QDomNodeList collection_roots = document_root_.elementsByTagName("collection");
	QDomElement collection_root;
	if (!collection_roots.isEmpty()) {
		// seule la premiere collection trouvee est prise en compte
		collection_root = collection_roots.at(0).toElement();
	}
	
	if (collection_root.isNull()) {
		// s'il n'y en a pas, cree une collection vide
		collection_ = new XmlElementsCollection();
	} else {
		// sinon lit cette collection
		collection_ = new XmlElementsCollection(collection_root);
	}
	collection_ -> setProtocol("embed");
	collection_ -> setProject(this);
	connect(collection_, SIGNAL(written()), this, SLOT(componentWritten()));
}

/**
	Load project properties from the XML description of the project
*/
void QETProject::readProjectPropertiesXml() {
	foreach (QDomElement e, QET::findInDomElement(document_root_.documentElement(), "properties")) {
		project_properties_.fromXml(e);
	}
}

/**
	Export project properties under the \a xml_element XML element.
*/
void QETProject::writeProjectPropertiesXml(QDomElement &xml_element) {
	project_properties_.toXml(xml_element);
}

/**
 * @brief QETProject::readDefaultPropertiesXml
 * load default properties for new diagram, found in the xml of this project
 * or by default find in the QElectroTech global conf
 */
void QETProject::readDefaultPropertiesXml() {
	// Find xml element where is stored properties for new diagram
	QDomNodeList newdiagrams_nodes = document_root_.elementsByTagName("newdiagrams");
	if (newdiagrams_nodes.isEmpty()) return;
	
	QDomElement newdiagrams_elmt = newdiagrams_nodes.at(0).toElement();
	
	// By default, use value find in the global conf of QElectroTech
	default_border_properties_	   = QETDiagramEditor::defaultBorderProperties();
	default_titleblock_properties_ = QETDiagramEditor::defaultTitleBlockProperties();
	default_conductor_properties_  = QETDiagramEditor::defaultConductorProperties();
	default_report_properties_	   = QETDiagramEditor::defaultReportProperties();
	m_default_xref_properties	   = QETDiagramEditor::defaultXRefProperties();
	
	//Read values indicate in project
	QDomElement border_elmt, titleblock_elmt, conductors_elmt, report_elmt, xref_elmt, conds_autonums;
	
	for (QDomNode child = newdiagrams_elmt.firstChild() ; !child.isNull() ; child = child.nextSibling()) {
		QDomElement child_elmt = child.toElement();
		if (child_elmt.isNull()) continue;
		if (child_elmt.tagName() == "border") {
			border_elmt = child_elmt;
		} else if (child_elmt.tagName() == "inset") {
			titleblock_elmt = child_elmt;
		} else if (child_elmt.tagName() == "conductors") {
			conductors_elmt = child_elmt;
		} else if (child_elmt.tagName() == "report") {
			report_elmt = child_elmt;
		} else if (child_elmt.tagName() == "xrefs") {
			xref_elmt = child_elmt;
		} else if (child_elmt.tagName() == "conductors_autonums") {
			conds_autonums = child_elmt;
		}
	}
	
	// size, titleblock, conductor, report, conductor autonum
	if (!border_elmt.isNull())	   default_border_properties_.fromXml(border_elmt);
	if (!titleblock_elmt.isNull()) default_titleblock_properties_.fromXml(titleblock_elmt);
	if (!conductors_elmt.isNull()) default_conductor_properties_.fromXml(conductors_elmt);
	if (!report_elmt.isNull())	   setDefaultReportProperties(report_elmt.attribute("label"));
	if (!xref_elmt.isNull()) {
		foreach(QDomElement elmt, QET::findInDomElement(xref_elmt, "xref")) {
			XRefProperties xrp;
			xrp.fromXml(elmt);
			m_default_xref_properties.insert(elmt.attribute("type"), xrp);
		}
	}
	if (!conds_autonums.isNull()) {
		foreach (QDomElement elmt, QET::findInDomElement(conds_autonums, "conductor_autonum")) {
			NumerotationContext nc;
			nc.fromXml(elmt);
			m_conductor_autonum.insert(elmt.attribute("title"), nc);
		}
	}
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

	//Export conductors autonums
	QDomElement conds_autonums = xml_document.createElement("conductors_autonums");
	foreach (QString key, conductorAutoNum().keys()) {
		QDomElement cond_autonum = conductorAutoNum(key).toXml(xml_document, "conductor_autonum");
		cond_autonum.setAttribute("title", key);
		conds_autonums.appendChild(cond_autonum);
	}
	xml_element.appendChild(conds_autonums);
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

	names.addName("fr", "\311l\351ments import\351s");
	names.addName("en", "Imported elements");
	names.addName("de", "Importierte elemente");
	names.addName("es", "Elementos importados");
	names.addName("ru", QString(russian_data, 24));
	names.addName("cs", "Zaveden\351 prvky");
	names.addName("pl", "Elementy importowane");
	names.addName("pt", "elementos importados");
	names.addName("it", "Elementi importati");
	names.addName("el", QString(greek_data, 18));
	names.addName("nl", "Elementen ge\357mporteerd");
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
	Cette methode sert a reperer un projet vide, c-a-d un projet identique a ce
	que l'on obtient en faisant Fichier > Nouveau.
	@return true si la collection d'elements embarquee a ete modifiee.
	Concretement, cette methode retourne true si la collection embarquee
	contient 0 element et 1 categorie vide qui s'avere etre la categorie dediee
	aux elements integres automatiquement dans le projet.
*/
bool QETProject::embeddedCollectionWasModified() {
	ElementsCategory *root_cat = rootCategory();
	if (!root_cat) return(false);
	
	// la categorie racine doit comporter 0 element et 1 categorie
	if (root_cat -> categories().count() != 1) return(true);
	if (root_cat -> elements().count()   != 0) return(true);
	
	// la categorie d'integration doit exister
	ElementsCategory *integ_cat = integrationCategory();
	if (!integ_cat) return(true);
	
	// la categorie d'integration doit avoir les noms par defaut
	if (integ_cat -> categoryNames() != namesListForIntegrationCategory()) {
		return(true);
	}
	
	// the integration category must be empty
	if (integ_cat -> categories().count()) return(true);
	if (integ_cat -> elements().count()) return(true);
	
	return(false);
}

/**
	@return true if the embedded title block templates collection was modified,
	false otherwise.
*/
bool QETProject::titleBlockTemplateCollectionWasModified() {
	// we do not expect a new project to embed any title block template (this may
	// change in the future though).
	return(titleblocks_.templates().count());
}

/**
	Cette methode sert a reperer un projet vide, c-a-d un projet identique a ce
	que l'on obtient en faisant Fichier > Nouveau.
	@return true si les schemas de ce projet ont ete modifies
	Concretement, il doit y avoir exactement un schema, dont la pile
	d'annulation est "clean".
*/
bool QETProject::diagramsWereModified() {
	// il doit y avoir exactement un schema
	if (diagrams_.count() != 1) return(true);
	
	// dont la pile d'annulation est "clean"
	return(!(diagrams_[0] -> undoStack().isClean() && !diagrams_[0] -> wasWritten()));
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
	if (projectOptionsWereModified()) return(true);
	if (diagramsWereModified()) return(true);
	if (embeddedCollectionWasModified()) return(true);
	if (titleBlockTemplateCollectionWasModified()) return(true);
	
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
		diagrams_[i] -> border_and_titleblock.setFolioData(i + 1, total_folio, project_wide_properties);
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

/**
	Copie l'element integ_elmt dans la categorie target_cat en utilisant le
	gestionnaire handler ; en cas d'erreur, error_message est rempli.
	@return l'emplacement de l'element cree
*/
ElementsLocation QETProject::copyElementWithHandler(
	ElementDefinition *integ_elmt,
	ElementsCategory *target_cat,
	MoveElementsHandler *handler,
	QString &error_message
) {
	ElementsCollectionItem *result_item = integ_elmt -> copy(target_cat, handler);
	ElementDefinition *result_elmt = result_item ? result_item -> toElement() : 0;
	if (!result_item || !result_elmt) {
		error_message = QString(tr("Un probl\350me s'est produit pendant la copie de l'\351l\351ment %1")).arg(integ_elmt -> location().toString());
		return(ElementsLocation());
	}
	return(result_elmt -> location());
}
