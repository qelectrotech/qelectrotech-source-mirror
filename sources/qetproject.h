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
#ifndef QET_PROJECT_H
#define QET_PROJECT_H

#include "ElementsCollection/elementslocation.h"
#include "NameList/nameslist.h"
#include "project/projectpropertieshandler.h"
#include "borderproperties.h"
#include "conductorproperties.h"
#include "dataBase/projectdatabase.h"
#include "properties/reportproperties.h"
#include "properties/xrefproperties.h"
#include "titleblock/templatescollection.h"
#include "titleblockproperties.h"
#include "wirehops.h"
#include "wiringrules.h"
#include "diagram.h"
#ifdef BUILD_WITHOUT_KF
#	include "ui/nokde/kautosavefile.h"
#else
#	include <KAutoSaveFile>
#endif

#include <QHash>
#include <QSet>
#include <QUuid>
#include <QVector>
#include <QFuture>

#include <array>

class Diagram;
class Element;
class ElementsLocation;
class QETResult;
class TitleBlockTemplate;
class MoveTitleBlockTemplatesHandler;
class NumerotationContext;
class QUndoStack;
class XmlElementCollection;
class QTimer;
class TerminalStrip;


#include <QColor>

struct GuideProperties {
	int orientation; // 0 = Horizontal, 1 = Vertical
	qreal position;
	QColor color;

	bool operator==(const GuideProperties &other) const {
		return orientation == other.orientation &&
		position == other.position &&
		color == other.color;
	}
	bool operator!=(const GuideProperties &other) const {
		return !(*this == other);
	}
};

/**
	This class represents a QET project. Typically saved as a .qet file, it
	consists in an XML document grouping 0 to n diagrams and embedding an elements
	collection. This collection enables users to export diagrams on remote
	machines without wondering whether required elements are available to them.
*/
class QETProject : public QObject
{
		friend class AddDiagramCommand;
		friend class RemoveDiagramCommand;
		Q_OBJECT
	public :
		//This enum lists possible states for a particular project.
		enum ProjectState {
			Ok                    = 0, /// no error
			FileOpenFailed        = 1, /// file opening failed
			XmlParsingFailed      = 2, /// XML parsing failed
			ProjectParsingRunning = 3, /// the XML content is currently being processed
			ProjectParsingFailed  = 4, /// the parsing of the XML content failed
			FileOpenDiscard       = 5  /// the user cancelled the file opening
		};

		Q_PROPERTY(bool autoConductor READ autoConductor WRITE setAutoConductor)
	Q_PROPERTY(bool autoBreakConductor READ autoBreakConductor WRITE setAutoBreakConductor)

		// constructors, destructor
	public:
		QETProject (QObject *parent = nullptr);
		QETProject (const QString &path, QObject * = nullptr);
		QETProject (KAutoSaveFile *backup, QObject *parent=nullptr);
		~QETProject() override;

	private:
		QETProject(const QETProject &);

		// methods
	public:
		ProjectPropertiesHandler& projectPropertiesHandler();
		projectDataBase *dataBase();
		QUuid uuid() const;
		QUuid derivedItemUuid(const QString &kind, const QString &key);
		ProjectState state() const;
		QList<Diagram *> diagrams() const;
		int folioIndex(const Diagram *) const;
		XmlElementCollection *embeddedElementCollection()const;
		TitleBlockTemplatesProjectCollection *embeddedTitleBlockTemplatesCollection();
		QString filePath();
		void setFilePath(const QString &);
		QString currentDir() const;
		QString pathNameTitle() const;
		QString title() const;
		QVersionNumber declaredQElectroTechVersion();
		void setTitle(const QString &);

		/// Enable/disable the asynchronous crash-recovery backup for all
		/// projects.  Disabled by the headless CLI: the backup write runs on a
		/// background thread referencing the project, and a short-lived CLI
		/// process can destroy the project before the write finishes (crash).
		static void setBackupEnabled(bool enabled);

		/// Number of crash-recovery snapshots kept per project, written in
		/// turn by writeBackup(), so one bad write cannot replace the only copy
		static constexpr int BackupGenerations = 3;

			///DEFAULT PROPERTIES
		BorderProperties defaultBorderProperties() const;
		void             setDefaultBorderProperties(const BorderProperties &);

		QList<GuideProperties> defaultGuides() const;
		void setDefaultGuides(const QList<GuideProperties> &guides);

		TitleBlockProperties defaultTitleBlockProperties() const;
		void                 setDefaultTitleBlockProperties(const TitleBlockProperties &);

		ConductorProperties defaultConductorProperties() const;
		void                setDefaultConductorProperties(const ConductorProperties &);

		QString defaultReportProperties() const;
		void    setDefaultReportProperties (const QString &properties);

		XRefProperties					defaultXRefProperties (const QString &type) const {return m_default_xref_properties[type];}
		QHash <QString, XRefProperties> defaultXRefProperties() const					  {return m_default_xref_properties;}
		void setDefaultXRefProperties(const QString& type, const XRefProperties &properties);
		void setDefaultXRefProperties(QHash <QString, XRefProperties> hash);

		QHash <QString, NumerotationContext> conductorAutoNum() const;
		QHash <QString, NumerotationContext> elementAutoNum() const;
		QHash <QString, NumerotationContext> folioAutoNum() const;
		void addConductorAutoNum (const QString& key, const NumerotationContext& context);
		void addElementAutoNum (const QString& key, const NumerotationContext& context);
		void addFolioAutoNum     (const QString& key, const NumerotationContext& context);
		void removeConductorAutoNum (const QString& key);
		void removeElementAutoNum (const QString& key);
		void removeFolioAutoNum (const QString& key);
		NumerotationContext conductorAutoNum(const QString &key) const;
		NumerotationContext folioAutoNum(const QString &key)     const;
		NumerotationContext elementAutoNum(const QString &key);

		QString conductorAutoNumFormula(const QString& key) const; //returns Formula
		QString conductorCurrentAutoNum() const;
		void setCurrentConductorAutoNum(QString autoNum);

		QString elementAutoNumFormula(const QString& key) const;
		QString elementAutoNumCurrentFormula() const;
		QString elementCurrentAutoNum() const;
		void setCurrrentElementAutonum(QString autoNum);

			//Identity of the element numbering schemes. The title is the
			//name shown to the user and the lookup key of the API; the uuid
			//is what an element's ELMT_FORMULA_ID refers to, so a scheme can
			//be renamed or edited without its elements losing track of it.
		void addElementAutoNum(const QString &key,
							   const NumerotationContext &context,
							   const QUuid &id);
		QUuid elementAutoNumId(const QString &title) const;
		QString elementAutoNumTitle(const QUuid &id) const;
		bool renameElementAutoNum(const QString &old_title, const QString &new_title);
		QString elementAutoNumNameClash(const QString &name,
										const QString &ignored_title = QString()) const;
		static QString normalizedAutoNumName(const QString &name);
		QVector<Element *> elementsUsingElementAutoNum(const QString &title) const;

		/**
		 * @brief Renumber existing elements by element autonumbering scheme.
		 *
		 * Elements follow a scheme by its uuid (QETInformation::ELMT_FORMULA_ID),
		 * see elementsUsingElementAutoNum().
		 *
		 * If @p scheme_title is empty, all schemes are renumbered.
		 * If @p scheme_title is non-empty, only that scheme is renumbered.
		 *
		 * The operation is undoable.
		 */
		void renumberElementsBySchemeTitle(const QString &scheme_title = QString());

			//Element
		void freezeExistentElementLabel(bool freeze, int from, int to);
		void freezeNewElementLabel(bool freeze, int from, int to);
		bool isFreezeNewElements();
		void setFreezeNewElements(bool);

			//Conductor
		void freezeExistentConductorLabel(bool freeze, int from, int to);
		void freezeNewConductorLabel(bool freeze, int from, int to);
		bool isFreezeNewConductors();
		void setFreezeNewConductors(bool);

		bool autoConductor () const;
		bool autoBreakConductor () const;
		bool autoElement () const;
		bool autoFolio () const;
		void setAutoConductor (bool ac);
		WireHops::Mode wireHops() const;
		void setWireHops(WireHops::Mode mode);
		bool uprightSymbolTexts() const;
		void setUprightSymbolTexts(bool upright);
		WiringRules::Settings wiringRules() const;
		WiringRules::Settings projectWiringRules() const;
		void setWiringRules(const WiringRules::Settings &rules);
		void setAutoBreakConductor (bool abc);
		void setAutoElement (bool ae);
		void autoFolioNumberingNewFolios ();
		void autoFolioNumberingSelectedFolios(int, int, const QString&);

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
		DiagramContext projectWideProperties();
		void setProjectProperties(const DiagramContext &);
		QUndoStack* undoStack() {return m_undo_stack;}

		QVector<TerminalStrip *> terminalStrip() const;
		TerminalStrip * newTerminalStrip(QString installation = QString(), QString location = QString(), QString name = QString());
		bool addTerminalStrip(TerminalStrip *strip);
		bool removeTerminalStrip(TerminalStrip *strip);

	public slots:
		Diagram *addNewDiagram(int pos = -1);
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
		void reportPropertiesChanged(const QString &old_str, const QString &new_str);
		void XRefPropertiesChanged ();
		void addAutoNumDiagram();
		void elementAutoNumAdded(QString name);
		void elementAutoNumRemoved(QString name);
		void conductorAutoNumAdded();
		void conductorAutoNumRemoved();
		void folioAutoNumAdded();
			/// A numerotation context's *values* changed -- as happens every
			/// time an element or conductor consumes the next number, not
			/// only when a rule is added or removed. Deliberately separate
			/// from the *Added/*Removed signals above, which make listeners
			/// rebuild their rule lists; this one just says "re-read me".
		void autoNumContextUpdated();
		void folioAutoNumRemoved();
		void defaultTitleBlockPropertiesChanged();
		void conductorAutoNumChanged();

	private slots:
		void updateDiagramsFolioData();
		void updateDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *, const QString &);
		void removeDiagramsTitleBlockTemplate(TitleBlockTemplatesCollection *, const QString &);
		void usedTitleBlockTemplateChanged(const QString &);
		/* Deliberately does NOT touch m_modified: m_modified /
		 * setModified() track project-OPTIONS changes only (see
		 * projectOptionsWereModified()), which have no undo
		 * entry and so must stay set until an explicit write().
		 * Diagram-content changes are tracked by the undo
		 * stack's own clean index instead, and projectWasModified()
		 * already ORs the two together -- that combined value is
		 * what actually answers "does this project have unsaved
		 * changes", so re-derive and broadcast it here on every
		 * clean/dirty transition (covering, in particular, an
		 * Undo that walks the stack back to its clean index).
		 * Latching m_modified itself to the undo stack's dirty
		 * state, the way this slot did before, is a one-way trap:
		 * cleanChanged(true) would never come back through here
		 * to un-set it, so a plain content edit stayed marked as
		 * unsaved even after being fully undone. */
		void undoStackChanged (bool /*a*/) {
			emit projectModified(this, projectWasModified());
			emit projectInformationsChanged(this);
		}

	private:
		void readProjectXml(QDomDocument &xml_project);
		void readDiagramsXml(QDomDocument &xml_project);
		void readElementsCollectionXml(QDomDocument &xml_project);
		void readProjectPropertiesXml(QDomDocument &xml_project);
		void readDefaultPropertiesXml(QDomDocument &xml_project);
		void readTerminalStripXml(const QDomDocument &xml_project);
		void readUsageXml(QDomDocument &xml_project);
		void readWireHopsXml(QDomDocument &xml_project);
		void readSymbolTextsXml(QDomDocument &xml_project);
		void readWiringRulesXml(QDomDocument &xml_project);

		void writeProjectPropertiesXml(QDomElement &);
		void writeDefaultPropertiesXml(QDomElement &);
		void writeUsageXml(QDomElement &);
		void writeWireHopsXml(QDomElement &);
		void writeSymbolTextsXml(QDomElement &);
		void writeWiringRulesXml(QDomElement &);
		void addDiagram(Diagram *diagram, int pos = -1);
		void detachDiagram(Diagram *diagram);
		void writeBackup();
		void init();
		ProjectState openFile(QFile *file);
		static QUuid derivedUuid(const QByteArray &content);
		void refresh();

	// attributes
	private:
			/// When false, writeBackup() is a no-op (set by the headless CLI)
		static bool m_backup_enabled;
			/// Something changed since the last backup, see writeBackup()
		bool m_backup_needed = true;
			/// File path this project is saved to
		QString m_file_path;
			/// Current state of the project
		ProjectState m_state;
			/// Diagrams carried by the project
		QList<Diagram *> m_diagrams_list;
			/// Project title
		QString project_title_;
			/// QElectroTech version declared in the XML document at opening time
		QVersionNumber m_project_qet_version;
			/// Whether options were modified
		bool m_modified = false;
			/// Whether the project is read only
		bool m_read_only = false;
			/// Filepath for which this project is considered read only
		QString read_only_file_path_;
			/// Default dimensions and properties for new diagrams created within the project
		BorderProperties default_border_properties_ = BorderProperties::defaultProperties();
			/// Default guides for new diagrams created within the project
		QList<GuideProperties> m_default_guides;
			/// Default conductor properties for new diagrams created within the project
		ConductorProperties default_conductor_properties_ = ConductorProperties::defaultProperties();
			/// Default title block properties for new diagrams created within the project
		TitleBlockProperties default_titleblock_properties_;
			/// Default report properties
		QString m_default_report_properties = ReportProperties::defaultProperties();
			/// Default xref properties
		QHash <QString, XRefProperties> m_default_xref_properties = XRefProperties::defaultProperties();
			/// Embedded title block templates collection
		TitleBlockTemplatesProjectCollection m_titleblocks_collection;
			/// project-wide variables that will be made available to child diagrams
		DiagramContext m_project_properties;
			/// undo stack for this project
		QUndoStack *m_undo_stack;
			/// Conductor auto numerotation
		QHash <QString, NumerotationContext> m_conductor_autonum;//Title and NumContext hash
		QString m_current_conductor_autonum;
			/// Folio auto numbering
		QHash <QString, NumerotationContext> m_folio_autonum;
			/// Element Auto Numbering
		QHash <QString, NumerotationContext> m_element_autonum; //Title and NumContext hash
			/// Title -> uuid of each element numbering scheme
		QHash <QString, QUuid> m_element_autonum_id;
		QString m_current_element_autonum;
			/// True when the loaded file had element numbering schemes
			/// saved without an id (written before ids existed)
		bool m_legacy_element_autonums = false;
		void linkElementsToElementAutoNums();
		bool m_auto_conductor = true;
		WireHops::Mode m_wire_hops = WireHops::Mode::None;
			/// Texts drawn in a turned symbol stay horizontal (on for a new
			/// project, off for one saved without it), see uprightSymbolTexts()
		bool m_upright_symbol_texts = false;
		WiringRules::Settings m_wiring_rules;
	bool m_auto_break_conductor = false;
		XmlElementCollection *m_elements_collection = nullptr;
		bool m_freeze_new_elements = false;
		bool m_freeze_new_conductors = false;
		QTimer m_save_backup_timer,
			   m_autosave_timer;
		QFuture<bool> m_backup_future;
			/// Crash-recovery snapshots, written in turn by writeBackup()
		std::array<KAutoSaveFile, BackupGenerations> m_backup_files;
		int m_next_backup_slot = 0;
		QUuid m_uuid = QUuid::createUuid();
		QHash<QString, int> m_derived_uuid_keys;
		QSet<QUuid> m_saved_item_uuids;	//symbol and wire uuids the file carries, see derivedItemUuid()
		projectDataBase m_data_base;
		QVector<TerminalStrip *> m_terminal_strip_vector;

		ProjectPropertiesHandler m_project_properties_handler;
};

Q_DECLARE_METATYPE(QETProject *)
#endif
