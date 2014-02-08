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
	This class represents a QET project. Typically saved as a .qet file, it
	consists in an XML document grouping 0 to n diagrams and embedding an elements
	collection. This collection enables users to export diagrams on remote
	machines without wondering whether required elements are available to them.
*/
class QETProject : public QObject {
	Q_OBJECT
	
	// constructors, destructor
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
		This enum lists possible states for a particular project.
	*/
	enum ProjectState {
		Ok                    = 0, /// no error
		FileOpenFailed        = 1, /// file opening failed
		XmlParsingFailed      = 2, /// XML parsing failed
		ProjectParsingRunning = 3, /// the XML content is currently being processed
		ProjectParsingFailed  = 4, /// the parsing of the XML content failed
		FileOpenDiscard       = 5  /// the user cancelled the file opening
	};
	
	// methods
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
	QString defaultReportProperties() const;
	void setDefaultReportProperties (const QString &properties);
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
	QUndoStack* undoStack() {return undo_stack_;}
	
	public slots:
	void componentWritten();
	Diagram *addNewDiagram();
	Diagram *addNewDiagramFolioList();
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
	void reportPropertiesChanged(QString);
	
	private slots:
	void updateDiagramsFolioData();
	void updateDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *, const QString &);
	void removeDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *, const QString &);
	void usedTitleBlockTemplateChanged(const QString &);
	void undoStackChanged (bool a) {if (!a) setModified(true);}
	
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
	
	// attributes
	private:
	/// File path this project is saved to
	QString file_path_;
	/// Current state of the project
	ProjectState state_;
	/// XML document representing the project
	QDomDocument document_root_;
	/// Diagrams carried by the project
	QList<Diagram *> diagrams_;
	/// Embedded elements collection
	XmlElementsCollection *collection_;
	/// Project title
	QString project_title_;
	/// QElectroTech version declared in the XML document at opening time
	qreal project_qet_version_;
	/// Whether options were modified
	bool modified_;
	/// Whether the project is read only
	bool read_only_;
	/// Filepath for which this project is considered read only
	QString read_only_file_path_;
	/// Name of the category used when automatically integrating elements within the embedded collection
	static QString integration_category_name;
	/// Default dimensions and properties for new diagrams created within the project
	BorderProperties default_border_properties_;
	/// Default conductor properties for new diagrams created within the project
	ConductorProperties default_conductor_properties_;
	/// Default title block properties for new diagrams created within the project
	TitleBlockProperties default_titleblock_properties_;
	/// Default report properties
	QString default_report_properties_;
	/// Embedded title block templates collection
	TitleBlockTemplatesProjectCollection titleblocks_;
	/// project-wide variables that will be made available to child diagrams
	DiagramContext project_properties_;
	/// undo stack for this project
	QUndoStack *undo_stack_;
};
Q_DECLARE_METATYPE(QETProject *)
#endif
