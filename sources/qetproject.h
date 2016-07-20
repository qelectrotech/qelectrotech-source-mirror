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
#ifndef QET_PROJECT_H
#define QET_PROJECT_H

#include "nameslist.h"
#include "elementslocation.h"
#include "borderproperties.h"
#include "conductorproperties.h"
#include "titleblockproperties.h"
#include "templatescollection.h"
#include "properties/xrefproperties.h"

class Diagram;
class ElementsLocation;
class QETResult;
class TitleBlockTemplate;
class MoveTitleBlockTemplatesHandler;
class NumerotationContext;
class QUndoStack;
class XmlElementCollection;

/**
	This class represents a QET project. Typically saved as a .qet file, it
	consists in an XML document grouping 0 to n diagrams and embedding an elements
	collection. This collection enables users to export diagrams on remote
	machines without wondering whether required elements are available to them.
*/
class QETProject : public QObject
{
		Q_OBJECT

		Q_PROPERTY(bool autoConductor READ autoConductor WRITE setAutoConductor)
	
		// constructors, destructor
	public:
		QETProject (int = 1, QObject * = 0);
		QETProject (const QString &, QObject * = 0);
		virtual ~QETProject();
	
	private:
		QETProject(const QETProject &);
		void init ();
	
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
		int getFolioSheetsQuantity() const; /// get the folio sheets quantity for this project
		void setFolioSheetsQuantity(int);   /// set the folio sheets quantity for this project
		int folioIndex(const Diagram *) const;
		XmlElementCollection *embeddedElementCollection()const;
		TitleBlockTemplatesProjectCollection *embeddedTitleBlockTemplatesCollection();
		QString filePath();
		void setFilePath(const QString &);
		QString currentDir() const;
		QString pathNameTitle() const;
		QString title() const;
		qreal declaredQElectroTechVersion();
		void setTitle(const QString &);

			///DEFAULT PROPERTIES
		BorderProperties defaultBorderProperties() const;
		void             setDefaultBorderProperties(const BorderProperties &);

		TitleBlockProperties defaultTitleBlockProperties() const;
		void                 setDefaultTitleBlockProperties(const TitleBlockProperties &);

		ConductorProperties defaultConductorProperties() const;
		void                setDefaultConductorProperties(const ConductorProperties &);

		QString defaultReportProperties() const;
		void    setDefaultReportProperties (const QString &properties);

		XRefProperties					defaultXRefProperties (const QString &type) const {return m_default_xref_properties[type];}
		QHash <QString, XRefProperties> defaultXRefProperties() const					  {return m_default_xref_properties;}
		void setDefaultXRefProperties(const QString type, const XRefProperties &properties);
		void setDefaultXRefProperties(QHash <QString, XRefProperties> hash);

		QHash <QString, NumerotationContext> conductorAutoNum() const;
		QHash <QString, NumerotationContext> elementAutoNum() const;
		QHash <QString, QString>             elementAutoNumHash();
		QHash <QString, NumerotationContext> folioAutoNum() const;
		void addConductorAutoNum (QString key, NumerotationContext context);
		void addElementAutoNum (QString key, NumerotationContext context);
		void addElementAutoNumFormula (QString key, QString formula);
		void setElementAutoNumCurrentFormula (QString formula, QString title);
		void addFolioAutoNum     (QString key, NumerotationContext context);
		void removeConductorAutoNum (QString key);
		void removeElementAutoNum (QString key);
		void removeFolioAutoNum (QString key);
		NumerotationContext conductorAutoNum(const QString &key) const;
		NumerotationContext folioAutoNum(const QString &key)     const;
		NumerotationContext elementAutoNum(const QString &key)   const;
		QString elementAutoNumFormula(const QString key) const; //returns Formula
		QString elementAutoNumFormula() const;
		QString elementCurrentAutoNum () const;

		void freezeExistentElementLabel(int,int);
		void freezeNewElementLabel(int,int);
		void unfreezeExistentElementLabel(int,int);
		void unfreezeNewElementLabel(int,int);
		bool freezeNewElements();
		void setFreezeNewElements(bool);

		bool autoConductor () const;
		bool autoElement () const;
		bool autoFolio () const;
		void setAutoConductor (bool ac);
		void setAutoElement (bool ae);
		void autoFolioNumberingNewFolios ();
		void autoFolioNumberingSelectedFolios(int, int, QString);

		QDomDocument toXml();
		bool close();
		QETResult write();
		bool isReadOnly() const;
		void setReadOnly(bool);
		bool isEmpty() const;
		ElementsLocation importElement(ElementsLocation &location);
		QString integrateTitleBlockTemplate(const TitleBlockTemplateLocation &, MoveTitleBlockTemplatesHandler *handler);
		bool usesElement(const ElementsLocation &) const;
		QList <ElementsLocation> unusedElements() const;
		bool usesTitleBlockTemplate(const TitleBlockTemplateLocation &);
		bool projectWasModified();
		bool projectOptionsWereModified();
		DiagramContext projectProperties();
		void setProjectProperties(const DiagramContext &);
		QUndoStack* undoStack() {return undo_stack_;}
	
	public slots:
	void componentWritten();
	Diagram *addNewDiagram();
	QList <Diagram *> addNewDiagramFolioList();
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
	void diagramUsedTemplate(TitleBlockTemplatesCollection *, const QString &);
	void readOnlyChanged(QETProject *, bool);
	void reportPropertiesChanged(QString);
	void XRefPropertiesChanged ();
	void addAutoNumDiagram();
	void elementAutoNumAdded();
	void elementAutoNumRemoved();
	void conductorAutoNumAdded();
	void conductorAutoNumRemoved();
	void folioAutoNumAdded();
	void folioAutoNumRemoved();
	void folioAutoNumChanged(QString);
	void defaultTitleBlockPropertiesChanged();
	void conductorAutoNumChanged();
	
	private slots:
	void updateDiagramsFolioData();
	void updateDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *, const QString &);
	void removeDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *, const QString &);
	void usedTitleBlockTemplateChanged(const QString &);
	void undoStackChanged (bool a) {if (!a) setModified(true);}
	
	private:
		void setupTitleBlockTemplatesCollection();

		void readProjectXml(QDomDocument &xml_project);
		void readDiagramsXml(QDomDocument &xml_project);
		void readElementsCollectionXml(QDomDocument &xml_project);
		void readProjectPropertiesXml(QDomDocument &xml_project);
		void readDefaultPropertiesXml(QDomDocument &xml_project);

		void writeProjectPropertiesXml(QDomElement &);
		void writeDefaultPropertiesXml(QDomElement &);
		void addDiagram(Diagram *);
		NamesList namesListForIntegrationCategory();
	
	// attributes
	private:
	/// File path this project is saved to
	QString file_path_;
	/// Current state of the project
	ProjectState state_;
	/// Diagrams carried by the project
	QList<Diagram *> diagrams_;
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
	/// Default dimensions and properties for new diagrams created within the project
	BorderProperties default_border_properties_;
	/// Default conductor properties for new diagrams created within the project
	ConductorProperties default_conductor_properties_;
	/// Default title block properties for new diagrams created within the project
	TitleBlockProperties default_titleblock_properties_;
	/// Default report properties
	QString default_report_properties_;
	/// Default xref properties
	QHash <QString, XRefProperties> m_default_xref_properties;
	/// Embedded title block templates collection
	TitleBlockTemplatesProjectCollection titleblocks_;
	/// project-wide variables that will be made available to child diagrams
	DiagramContext project_properties_;
	/// undo stack for this project
	QUndoStack *undo_stack_;
	/// Conductor auto numerotation
	QHash <QString, NumerotationContext> m_conductor_autonum;
	/// Folio auto numbering
	QHash <QString, NumerotationContext> m_folio_autonum;
	/// Element Auto Numbering
	QHash <QString, QString> m_element_autonum_formula; //Title and Formula hash
	QHash <QString, NumerotationContext> m_element_autonum; //Title and NumContext hash
	QString m_current_element_formula;
	QString m_current_element_autonum;
	/// Folio List Sheets quantity for this project.
	int folioSheetsQuantity;
	bool m_auto_conductor;
	XmlElementCollection *m_elements_collection;
	bool m_freeze_new_elements;
};
Q_DECLARE_METATYPE(QETProject *)
#endif
