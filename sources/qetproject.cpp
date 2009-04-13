/*
	Copyright 2006-2009 Xavier Guerrin
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
#include "elementdefinition.h"
#include "xmlelementscollection.h"
#include "elementscategory.h"
#include "qetapp.h"
#include "qetdiagrameditor.h"
#include "integrationmoveelementshandler.h"
#include "basicmoveelementshandler.h"

QString QETProject::integration_category_name = "import";

/**
	Constructeur par defaut - cree un schema contenant une collection
	d'elements vide et un schema vide.
	@param diagrams Nombre de nouveaux schemas a ajouter a ce nouveau projet
	@param parent QObject parent
*/
QETProject::QETProject(int diagrams, QObject *parent) :
	QObject(parent),
	read_only_(false)
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
}

/**
	Construit un projet a partir du chemin d'un fichier.
	@param path Chemin du fichier
	@param parent QObject parent
*/
QETProject::QETProject(const QString &path, QObject *parent) :
	QObject(parent),
	read_only_(false)
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
	
	// passe le projet en lecture seule si le fichier l'est
	QFileInfo project_file_info(path);
	if (!project_file_info.isWritable()) {
		setReadOnly(true);
	}
}

/**
	Construit un projet a partir d'un element XML representant le projet.
	L'element XML fourni est copie et conserve dans la classe.
*/
QETProject::QETProject(const QDomElement &xml_element, QObject *parent) :
	QObject(parent),
	read_only_(false)
{
	// copie le contenu XML
	document_root_.appendChild(document_root_.importNode(xml_element, true));
	
	// et construit le projet
	readProjectXml();
}

/**
	Destructeur
*/
QETProject::~QETProject() {
	// supprime les schemas
	// qDebug() << "Suppression du projet" << ((void *)this);
	
	// supprime la collection
	// qDebug() << "Suppression de la collection du projet" << ((void *)this);
	delete collection_;
	// qDebug() << "Collection du projet" << ((void *)this) << "supprimee";
	
	// qDebug() << diagrams_;
	foreach (Diagram *diagram, diagrams_) {
		diagrams_.removeAll(diagram);
		delete diagram;
	}
	// qDebug() << diagrams_;
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
	@return la liste des schemas de ce projet
*/
QList<Diagram *> QETProject::diagrams() const {
	return(diagrams_);
}

/**
	@return la collection embarquee de ce projet
*/
ElementsCollection *QETProject::embeddedCollection() const {
	return(collection_);
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
	
	return(final_title);
}

/**
	@return le titre du projet
*/
QString QETProject::title() const {
	return(project_title_);
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
InsetProperties QETProject::defaultInsetProperties() const {
	return(default_inset_properties_);
}

/**
	Permet de specifier le cartouche par defaut utilise lors de la creation
	d'un nouveau schema dans ce projet.
	@param inset Cartouche d'un schema
*/
void QETProject::setDefaultInsetProperties(const InsetProperties &inset) {
	default_inset_properties_ = inset;
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

/**
	@return un document XML representant le projet 
*/
QDomDocument QETProject::toXml() {
	// racine du projet
	QDomDocument xml_doc;
	QDomElement project_root = xml_doc.createElement("project");
	project_root.setAttribute("version", QET::version);
	project_root.setAttribute("title", project_title_);
	xml_doc.appendChild(project_root);
	
	// proprietes pour les nouveaux schemas
	QDomElement new_diagrams_properties = xml_doc.createElement("newdiagrams");
	writeDefaultPropertiesXml(new_diagrams_properties);
	project_root.appendChild(new_diagrams_properties);
	
	// schemas
	
	// qDebug() << "Export XML de" << diagrams_.count() << "schemas";
	int order_num = 1;
	foreach(Diagram *diagram, diagrams_) {
		qDebug() << qPrintable(QString("QETProject::toXml() : exporting diagram \"%1\" [%2]").arg(diagram -> title()).arg(QET::pointerString(diagram)));
		QDomNode appended_diagram = project_root.appendChild(diagram -> writeXml(xml_doc));
		appended_diagram.toElement().setAttribute("order", order_num ++);
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
bool QETProject::write() {
	// le chemin du fichier doit etre connu
	if (file_path_.isEmpty()) {
		qDebug() << qPrintable(QString("QETProject::write() : called without a known filepath [%1]").arg(QET::pointerString(this)));
		return(false);
	}
	
	// si le projet a ete ouvert en mode lecture seule et que le fichier n'est pas accessible en ecriture, on n'effectue pas l'enregistrement
	if (isReadOnly() && !QFileInfo(file_path_).isWritable()) {
		qDebug() << qPrintable(QString("QETProject::write() : the file %1 was opened read-only and thus will not be written. [%2]").arg(file_path_).arg(QET::pointerString(this)));
		return(true);
	}
	
	// ouvre le fichier en ecriture
	QFile file(file_path_);
	bool file_opening = file.open(QIODevice::WriteOnly | QIODevice::Text);
	if (!file_opening) {
		qDebug() << qPrintable(QString("QETProject::write() : unable to open %1 with write access [%2]").arg(file_path_).arg(QET::pointerString(this)));
		return(false);
	}
	
	qDebug() << qPrintable(QString("QETProject::write() : writing to file %1 [%2]").arg(file_path_).arg(QET::pointerString(this)));
	
	// realise l'export en XML du projet dans le document XML interne
	document_root_.clear();
	document_root_.appendChild(document_root_.importNode(toXml().documentElement(), true));
	
	QTextStream out(&file);
	out.setCodec("UTF-8");
	out << document_root_.toString(4);
	file.close();
	
	return(true);
}

/**
	@return true si le projet est en mode readonly, false sinon
*/
bool QETProject::isReadOnly() const {
	return(read_only_ && read_only_file_path_ == file_path_);
}

/**
	@param bool read_only true pour passer le projet (schemas et collection)
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
	@param error_message Reference vers une chaine de caractere qui contiendra
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
	
	// accede a a categorie d'integration
	ElementsCategory *integ_cat = integrationCategory();
	
	// accede a l'element a integrer
	ElementsCollectionItem *integ_item = QETApp::collectionItem(ElementsLocation::locationFromString(elmt_path));
	ElementDefinition *integ_elmt = integ_item ? integ_item -> toElement() : 0;
	if (!integ_item || !integ_elmt) {
		error_message = tr("Impossible d'acc\351der \340 l'\351l\351ment a int\351grer");
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
	if (ElementDefinition *existing_elmt = target_cat -> element(integ_item -> pathName())) {
		
		// l'element existe deja - on demande au handler ce que l'on doit faire
		QET::Action todo = handler -> elementAlreadyExists(integ_elmt, existing_elmt);
		
		if (todo == QET::Ignore) {
			// il faut conserver et utiliser l'element deja integre
			return(existing_elmt -> location().toString());
		} else if (todo == QET::Erase) {
			// il faut ecraser l'element deja integre
			BasicMoveElementsHandler *erase_handler = new BasicMoveElementsHandler();
			ElementsLocation result_loc = copyElementWithHandler(integ_elmt, target_cat, erase_handler, error_message);
			delete erase_handler;
			return(result_loc.toString());
		} else if (todo == QET::Rename) {
			// il faut faire cohabiter les deux elements en renommant le nouveau 
			QString integ_element_name = handler -> nameForRenamingOperation();
			BasicMoveElementsHandler *rename_handler = new BasicMoveElementsHandler();
			rename_handler -> setActionIfItemAlreadyExists(QET::Rename);
			rename_handler -> setNameForRenamingOperation(integ_element_name);
			ElementsLocation result_loc = copyElementWithHandler(integ_elmt, target_cat, rename_handler, error_message);
			delete rename_handler;
			return(result_loc.toString());
		} else {
			// il faut annuler la pose de l'element
			return(QString());
		}
	} else {
		// integre l'element normalement
		ElementsLocation result_loc = copyElementWithHandler(integ_elmt, target_cat, handler, error_message);
		return(result_loc.toString());
	}
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
	diagram -> border_and_inset.importBorder(defaultBorderProperties());
	diagram -> border_and_inset.importInset(defaultInsetProperties());
	diagram -> defaultConductorProperties = defaultConductorProperties();
	
	addDiagram(diagram);
	emit(diagramAdded(this, diagram));
	return(diagram);
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
	
	// la racine du document XML est sensee etre un element "project"
	if (root_elmt.tagName() == "project") {
		// mode d'ouverture normal
		if (root_elmt.hasAttribute("version")) {
			bool conv_ok;
			qreal diagram_version = root_elmt.attribute("version").toDouble(&conv_ok);
			if (conv_ok && QET::version.toDouble() < diagram_version) {
				QMessageBox::warning(
					0,
					tr("Avertissement", "message box title"),
					tr(
						"Ce document semble avoir \351t\351 enregistr\351 avec "
						"une version ult\351rieure de QElectroTech. Il est "
						"possible que l'ouverture de tout ou partie de ce "
						"document \351choue.",
						"message box content"
					)
				);
			}
		}
		
		setTitle(root_elmt.attribute("title"));
	} else if (root_elmt.tagName() == "diagram") {
		/// @todo gerer l'ouverture de fichiers <diagram />
	} else {
		state_ = ProjectParsingFailed;
	}
	
	// charge les proprietes par defaut pour les nouveaux schemas
	readDefaultPropertiesXml();
	
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
	
	// recherche les schemas dans le projet
	QDomNodeList diagram_nodes = document_root_.elementsByTagName("diagram");
	for (uint i = 0 ; i < diagram_nodes.length() ; ++ i) {
		if (diagram_nodes.at(i).isElement()) {
			QDomElement diagram_xml_element = diagram_nodes.at(i).toElement();
			Diagram *diagram = new Diagram();
			diagram -> setProject(this);
			bool diagram_loading = diagram -> fromXml(diagram_xml_element);
			if (diagram_loading) {
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
	Charge les proprietes par defaut des nouveaux schemas depuis la description
	XML du projet :
	  * dimensions
	  * contenu du cartouche
	  * conducteurs par defaut
*/
void QETProject::readDefaultPropertiesXml() {
	// repere l'element XML decrivant les proprietes des nouveaux schemas
	QDomNodeList newdiagrams_nodes = document_root_.elementsByTagName("newdiagrams");
	if (newdiagrams_nodes.isEmpty()) return;
	
	QDomElement newdiagrams_elmt = newdiagrams_nodes.at(0).toElement();
	
	// par defaut, les valeurs sont celles de la configuration QElectroTech
	default_border_properties_    = QETDiagramEditor::defaultBorderProperties();
	default_inset_properties_     = QETDiagramEditor::defaultInsetProperties();
	default_conductor_properties_ = QETDiagramEditor::defaultConductorProperties();
	
	// lecture des valeurs indiquees dans le projet
	QDomElement border_elmt, inset_elmt, conductors_elmt;
	
	// recherche des elements XML concernant les dimensions, le cartouche et les conducteurs
	for (QDomNode child = newdiagrams_elmt.firstChild() ; !child.isNull() ; child = child.nextSibling()) {
		QDomElement child_elmt = child.toElement();
		if (child_elmt.isNull()) continue;
		if (child_elmt.tagName() == "border") {
			border_elmt = child_elmt;
		} else if (child_elmt.tagName() == "inset") {
			inset_elmt = child_elmt;
		} else if (child_elmt.tagName() == "conductors") {
			conductors_elmt = child_elmt;
		}
	}
	
	// dimensions, cartouche, et conducteurs
	if (!border_elmt.isNull())     default_border_properties_.fromXml(border_elmt);
	if (!inset_elmt.isNull())      default_inset_properties_.fromXml(inset_elmt);
	if (!conductors_elmt.isNull()) default_conductor_properties_.fromXml(conductors_elmt);
}

/**
	Exporte les proprietes par defaut des nouveaux schemas dans l'element XML :
	  * dimensions
	  * contenu du cartouche
	  * conducteurs par defaut
	@param xml_element Element XML sous lequel seront exportes les proprietes
	par defaut des nouveaux schemas 
*/
void QETProject::writeDefaultPropertiesXml(QDomElement &xml_element) {
	QDomDocument xml_document = xml_element.ownerDocument();
	
	// exporte les dimensions
	QDomElement border_elmt = xml_document.createElement("border");
	default_border_properties_.toXml(border_elmt);
	xml_element.appendChild(border_elmt);
	
	// exporte le contenu du cartouche
	QDomElement inset_elmt = xml_document.createElement("inset");
	default_inset_properties_.toXml(inset_elmt);
	xml_element.appendChild(inset_elmt);
	
	// exporte le type de conducteur par defaut
	QDomElement conductor_elmt = xml_document.createElement("conductors");
	default_conductor_properties_.toXml(conductor_elmt);
	xml_element.appendChild(conductor_elmt);
}

/**
	Cette methode ajoute un schema donne au projet
	@param diagram Schema a ajouter
*/
void QETProject::addDiagram(Diagram *diagram) {
	if (!diagram) return;
	
	// s'assure que le schema connaisse son projet parent
	diagram -> setProject(this);
	
	// si le schema est ecrit, alors il faut reecrire le fichier projet
	connect(diagram, SIGNAL(written()), this, SLOT(componentWritten()));
	connect(
		&(diagram -> border_and_inset),
		SIGNAL(needFolioData()),
		this,
		SLOT(updateDiagramsFolioData())
	);
	
	// ajoute le schema au projet
	diagrams_ << diagram;
	
	updateDiagramsFolioData();
}

/**
	@return La liste des noms a utiliser pour la categorie dediee aux elements
	integres automatiquement dans le projet.
*/
NamesList QETProject::namesListForIntegrationCategory() {
	NamesList names;
	
	names.addName("fr", "\311l\351ments import\351s");
	names.addName("en", "Imported elements");
	names.addName("es", "Elementos importados");
	
	return(names);
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
	
	return(false);
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
	return(!(diagrams_[0] -> undoStack().isClean()));
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
	// il doit avoir un titre vide
	if (!title().isEmpty()) return(true);
	
	// ni ses schemas ni sa collection embarquee ne doivent avoir ete modifies
	if (diagramsWereModified()) return(true);
	if (embeddedCollectionWasModified()) return(true);
	
	return(false);
}

/**
	Indique a chaque schema du projet quel est son numero de folio et combien de
	folio le projet contient.
*/
void QETProject::updateDiagramsFolioData() {
	int total_folio = diagrams_.count();
	for (int i = 0 ; i < total_folio ; ++ i) {
		diagrams_[i] -> border_and_inset.setFolioData(i + 1, total_folio);
	}
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
