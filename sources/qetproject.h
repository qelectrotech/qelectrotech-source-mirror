/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef QET_PROJECT_H
#define QET_PROJECT_H
#include <QtCore>
#include <QtXml>
#include "nameslist.h"
#include "elementslocation.h"
#include "borderproperties.h"
#include "conductorproperties.h"
#include "titleblockproperties.h"
#include "templatescollection.h"
class Diagram;
class ElementsCollection;
class ElementsCategory;
class ElementDefinition;
class ElementsLocation;
class QETResult;
class TitleBlockTemplate;
class XmlElementsCollection;
class MoveElementsHandler;
class MoveTitleBlockTemplatesHandler;
/**
	Cette classe represente un projet QET. Typiquement enregistre dans un
	fichier, il s'agit d'un document XML integrant des schemas ainsi qu'une
	collection d'elements embarques. Ce dernier attribut permet d'exporter des
	schema sur d'autres machines sans se soucier de la presence des elements
	sur celle-ci.
	La classe QETProject gere l'enregistrement partiel. Ainsi, enregistrer les
	modifications apportees a un element reenregistrera le fichier mais en
	mettant a jour seulement les parties du document XML concerne.
*/
class QETProject : public QObject {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	QETProject(int = 1, QObject * = 0);
	QETProject(const QString &, QObject * = 0);
	QETProject(const QDomElement &, QObject * = 0);
	virtual ~QETProject();
	
	private:
	QETProject(const QETProject &);
	
	// enums
	public:
	/**
		Represente l'etat du projet
	*/
	enum ProjectState {
		Ok                    = 0, /// Le projet n'est pas en erreur
		FileOpenFailed        = 1, /// l'ouverture d'un fichier a echoue
		XmlParsingFailed      = 2, /// l'analyse XML a echoue
		ProjectParsingRunning = 3, /// la lecture du projet est en cours
		ProjectParsingFailed  = 4, /// la lecture en tant que projet a echoue
		FileOpenDiscard       = 5  /// the user cancelled the file opening
	};
	
	// methodes
	public:
	ProjectState state() const;
	QList<Diagram *> diagrams() const;
	int folioIndex(const Diagram *) const;
	ElementsCollection *embeddedCollection() const;
	TitleBlockTemplatesProjectCollection *embeddedTitleBlockTemplatesCollection();
	QString filePath();
	void setFilePath(const QString &);
	QString currentDir() const;
	QString pathNameTitle() const;
	QString title() const;
	qreal declaredQElectroTechVersion();
	void setTitle(const QString &);
	QList<QString> embeddedTitleBlockTemplates();
	const TitleBlockTemplate *getTemplateByName(const QString &template_name);
	QDomElement getTemplateXmlDescriptionByName(const QString &);
	bool setTemplateXmlDescription(const QString &, const QDomElement &);
	void removeTemplateByName(const QString &);
	BorderProperties defaultBorderProperties() const;
	void setDefaultBorderProperties(const BorderProperties &);
	TitleBlockProperties defaultTitleBlockProperties() const;
	void setDefaultTitleBlockProperties(const TitleBlockProperties &);
	ConductorProperties defaultConductorProperties() const;
	void setDefaultConductorProperties(const ConductorProperties &);
	QDomDocument toXml();
	bool close();
	QETResult write();
	bool isReadOnly() const;
	void setReadOnly(bool);
	bool isEmpty() const;
	bool ensureIntegrationCategoryExists();
	ElementsCategory *integrationCategory() const;
	QString integrateElement(const QString &, QString &);
	QString integrateElement(const QString &, MoveElementsHandler *, QString &);
	QString integrateTitleBlockTemplate(const TitleBlockTemplateLocation &, MoveTitleBlockTemplatesHandler *handler);
	bool usesElement(const ElementsLocation &);
	bool usesTitleBlockTemplate(const TitleBlockTemplateLocation &);
	void cleanUnusedTitleBlocKTemplates();
	void cleanUnusedElements(MoveElementsHandler *);
	void cleanEmptyCategories(MoveElementsHandler *);
	bool projectWasModified();
	bool projectOptionsWereModified();
	bool embeddedCollectionWasModified();
	bool titleBlockTemplateCollectionWasModified();
	bool diagramsWereModified();
	DiagramContext projectProperties();
	void setProjectProperties(const DiagramContext &);
	
	public slots:
	void componentWritten();
	Diagram *addNewDiagram();
	void removeDiagram(Diagram *);
	void diagramOrderChanged(int, int);
	void setModified(bool);
	
	signals:
	void projectFilePathChanged(QETProject *, const QString &);
	void projectTitleChanged(QETProject *, const QString &);
	void projectInformationsChanged(QETProject *);
	void diagramAdded(QETProject *, Diagram *);
	void diagramRemoved(QETProject *, Diagram *);
	void projectModified(QETProject *, bool);
	void projectDiagramsOrderChanged(QETProject *, int, int);
	void elementIntegrated(QETProject *, const ElementsLocation &);
	void diagramUsedTemplate(TitleBlockTemplatesCollection *, const QString &);
	void readOnlyChanged(QETProject *, bool);
	
	private slots:
	void updateDiagramsFolioData();
	void updateDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *, const QString &);
	void removeDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *, const QString &);
	void usedTitleBlockTemplateChanged(const QString &);
	
	private:
	void setupTitleBlockTemplatesCollection();
	ElementsCategory *rootCategory() const;
	void readProjectXml();
	void readDiagramsXml();
	void readElementsCollectionXml();
	void readEmbeddedTemplatesXml();
	void readProjectPropertiesXml();
	void writeProjectPropertiesXml(QDomElement &);
	void readDefaultPropertiesXml();
	void writeDefaultPropertiesXml(QDomElement &);
	void addDiagram(Diagram *);
	NamesList namesListForIntegrationCategory();
	ElementsLocation copyElementWithHandler(ElementDefinition *, ElementsCategory *, MoveElementsHandler *, QString &);
	
	// attributs
	private:
	/// Chemin du fichier dans lequel ce projet est enregistre
	QString file_path_;
	/// Etat du projet
	ProjectState state_;
	/// Element XML representant le projet
	QDomDocument document_root_;
	/// Schemas contenus dans le projet
	QList<Diagram *> diagrams_;
	/// Collection d'elements embarquee
	XmlElementsCollection *collection_;
	/// Titre du projet
	QString project_title_;
	/// Version de QElectroTech declaree dans le document XML lors de son ouverture
	qreal project_qet_version_;
	/// Whether options were modified
	bool modified_;
	/// booleen indiquant si le projet est en ReadOnly ou non
	bool read_only_;
	/// Chemin du fichier pour lequel ce projet est considere comme etant en lecture seule
	QString read_only_file_path_;
	/// Nom de la categorie a utiliser pour l'integration automatique des elements
	static QString integration_category_name;
	/// Dimensions par defaut pour les nouveaux schemas dans ce projet
	BorderProperties default_border_properties_;
	/// Proprietes par defaut des conducteurs pour les nouveaux schemas dans ce projet
	ConductorProperties default_conductor_properties_;
	/// Proprietes par defaut du cartouche pour les nouveaux schemas dans ce projet
	TitleBlockProperties default_titleblock_properties_;
	/// Embedded title block templates collection
	TitleBlockTemplatesProjectCollection titleblocks_;
	/// project-wide variables that will be made available to child diagrams
	DiagramContext project_properties_;
};
Q_DECLARE_METATYPE(QETProject *)
#endif
