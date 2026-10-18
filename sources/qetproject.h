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

#include <functional>
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
#include "autoNum/assignvariables.h"
#include "container/qetcontainerdb.h"
#include "projectlock.h"
#ifdef BUILD_WITHOUT_KF
#	include "ui/nokde/kautosavefile.h"
#else
#	include <KAutoSaveFile>
#endif

#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QUuid>
#include <QVector>
#include <QFuture>

#include <array>
#include <memory>
#include <optional>

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
class Cable;


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
		QUuid renewedSharedUuid(int folio_index, int ordinal, const QUuid &uuid,
								const QUuid &folio_uuid);
		QByteArray containerPicture(const QString &file) const;
		ProjectState state() const;
		QString openError() const;
		QList<Diagram *> diagrams() const;
			//The folios whether their items are built or not
			//(lazy folios); diagrams() builds them
		QList<Diagram *> folios() const;
		bool deferringFolios() const {return m_deferring_folios;}
		int unloadedFolioCount() const;
		QList<Diagram *> builtFolios() const;
		QList<Diagram *> cableLabelFolios();
		QList<Diagram *> foliosWithWiresNotReconnected();
		void buildFolioToShow(Diagram *folio);
		void buildPartnerSymbols(const QList<Diagram *> &folios, const QSet<QUuid> &partners);
		void buildFoliosHolding(ElementData::Types kinds, bool free_only);
		void buildFoliosHoldingSymbols(const QSet<QUuid> &symbols);
		void buildFoliosHoldingTables();
		QSet<QUuid> symbolsOnUnbuiltFolios() const;
			/// While saving: the symbols on folios not built a built symbol
			/// may be saved linked to (Element::toXml())
		QSet<QUuid> waitingLinkTargets() const {return m_waiting_link_targets;}
		QDomDocument folioXml(Diagram *folio);
		QSet<Diagram *> tableChainFolios(Diagram *folio) const;
		void loadFolios() const;
		void buildFolios(const QList<Diagram *> &folios) const;
		void setFolioBuildObserver(std::function<void(int, int)> observer);
		void folioLoaded(Diagram *folio);
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

			/// Whether a folio is built only when it is needed, not all on
			/// opening (stage D6): yes, unless the
			/// environment variable QET_EAGER_FOLIOS=1 asks for every folio
			/// built on opening, as QElectroTech did before
		static bool lazyFolios();

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

			/// The cable numbering rule. Only one of them is ever kept:
			/// a cable is numbered by one rule and that is all there is,
			/// so the hash holds a single entry (see the cable tab of
			/// the automatic numbering pages).
		QHash <QString, NumerotationContext> cableAutoNum() const;
		void addCableAutoNum    (const QString& key, const NumerotationContext& context);
		void removeCableAutoNum (const QString& key);
		NumerotationContext cableAutoNum(const QString &key) const;

		QString cableAutoNumFormula(const QString& key) const;
		bool hasCableAutoNum() const;
		QString cableCurrentAutoNum() const;
		void setCurrentCableAutoNum(QString autoNum);
			/**
				Which axis the cables of the project are laid out along
				when the whole project is numbered again: true (the
				default) for the X axis, so the leftmost cable of a folio
				comes first, false for the Y axis, so the topmost does.
				It belongs to the numbering and is written into the
				project file next to the rule itself.
			*/
		bool cableXAxisFirst() const;
		void setCableXAxisFirst(bool x_axis_first);
			/**
				Whether the cable tool asks this project whether a
				numbering rule should be defined, when he is picked up
				and the project has none. Saying no in that question is
				remembered with the project rather than in the program
				settings, so one project saying no does not silence
				every other one -- each project asks again until it
				has answered for itself, and taking the rule away
				brings the question back (see the cable tab of the
				project properties and the numbering window).
			*/
		bool cableAskNumbering() const;
		void setCableAskNumbering(bool ask);
			/**
				The name the single cable numbering rule is kept under.
				Fixed and never translated: it is a key which goes into
				the settings and into the project file, so it must read
				the same whatever language the program runs in.
			*/
		static QString cableAutoNumRuleName();
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
			/// A placed symbol's numbering, whether its folio is built or
			/// not (lazy folios): what is asked of every symbol of the
			/// project when one is numbered
		struct SymbolNumbering {
			const Element *element = nullptr; ///< null on a folio not built
			QString label;
			QString formula;
			QUuid scheme;   ///< the element numbering scheme it follows
			autonum::sequentialNumbers sequence;
			bool numbered = false; ///< neither a slave nor a report
		};
		QList<SymbolNumbering> symbolNumbering() const;

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

		QDomDocument toXml(QHash<int, QByteArray> *kept_folios = nullptr,
						   QStringList *kept_parts = nullptr);
		static QString spliceKeptFolios(QString text, const QHash<int, QByteArray> &kept_folios);
		bool close();
		QETResult write(bool overwrite_changed = false);
		QString writeTarget() const;
		void rememberDiskState();
		bool changedOnDisk(QDateTime *when = nullptr) const;
		bool isReadOnly() const;
			/// Opened from a .qetz of a later format (opened read-only)
		bool isFromLaterVersion() const {return m_from_later_version;}
		void setReadOnly(bool);
			/// The lock sidecar this session holds on the project file, if any
		ProjectLock *lock() const {return m_lock.get();}
		void setLock(std::unique_ptr<ProjectLock> lock);
		void setOpenedReadOnlyByLock(bool by_lock);
		bool isEmpty() const;
		ElementsLocation importElement(ElementsLocation &location);
		QString integrateTitleBlockTemplate(const TitleBlockTemplateLocation &, MoveTitleBlockTemplatesHandler *handler);
		bool usesElement(const ElementsLocation &) const;
		QList <ElementsLocation> unusedElements() const;
		QList<ElementsLocation> unbuiltFolioElements() const;
		bool unbuiltFolioHasTables(Diagram *folio) const;
		bool folioHoldsTablesOrSymbols(Diagram *folio) const;
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

		QVector<Cable *> cables() const;
		Cable *newCable();
		bool addCable(Cable *cable);
		bool removeCable(Cable *cable);
		Diagram *diagramByUuid(const QUuid &uuid);

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
			/// A cable joined or left this project. Views over the
			/// cables -- the cable list table -- listen to both to work
			/// their rows out again; during a load the cables arrive one
			/// signal at a time, long after the folios were read.
		void cableAdded(Cable *cable);
		void cableRemoved(Cable *cable);

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
		void watchTerminalStrip(TerminalStrip *strip, bool watch = true);
		bool stampSaved();
		void readProjectXml(QDomDocument &xml_project);
		void readDiagramsXml(QDomDocument &xml_project);
		void readElementsCollectionXml(QDomDocument &xml_project);
		void readProjectPropertiesXml(QDomDocument &xml_project);
		void readDefaultPropertiesXml(QDomDocument &xml_project);
		void readTerminalStripXml(const QDomDocument &xml_project);
		void readCableXml(const QDomDocument &xml_project);
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
	public:
		static bool isContainerPath(const QString &path);
		static QString openFileFilter();
			/// Saving writes a .qetz: one read from a .qet is saved as
			/// a .qetz beside it, the .qet left as it is
			/// (stage D1)
		bool savesBesideOpenedFile() const {return m_opened_from_qet;}
		QString besideContainerPath() const;
		QETResult writeCopyForOlderVersions(const QString &path);
		static bool writeContainer(const QDomDocument &project, const QString &path,
								   QString *error = nullptr,
								   const QHash<QString, QByteArray> &kept_from = QHash<QString, QByteArray>());
		bool writeContainerFile(const QString &path, QString *error = nullptr);
	private:
		static QUuid derivedUuid(const QByteArray &content);
		void refresh();

	// attributes
	private:
			/// When false, writeBackup() is a no-op (set by the headless CLI)
		static bool m_backup_enabled;
			/// Something changed since the last backup, see writeBackup()
		bool m_backup_needed = true;
			/// The pictures of the .qetz this project was opened from, see
			/// containerPicture()
		QHash<QString, QByteArray> m_container_pictures;
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
		bool m_from_later_version = false;
			/// The lock sidecar of the project file (see ProjectLock)
		std::unique_ptr<ProjectLock> m_lock;
			/// File this project was opened read-only for, because another
			/// session holds its lock: write() does not write to it
		QString m_locked_out_path;
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
			/// Cable auto numbering: exactly one rule, kept the same way
			/// the others are so it stores and travels with the project
			/// alike, but never a list to choose from.
		QHash <QString, NumerotationContext> m_cable_autonum;
		QString m_current_cable_autonum;
			///< which axis the cables are numbered along (see cableXAxisFirst)
		bool m_cable_axis_x_first = true;
			///< whether the cable tool still asks about a numbering rule
			///< for this project (see cableAskNumbering)
		bool m_cable_ask_numbering = true;
			/// True when the loaded file had element numbering schemes
			/// saved without an id (written before ids existed)
		bool m_legacy_element_autonums = false;
		void linkElementsToElementAutoNums();
		void linkElementsToElementAutoNums(Diagram *folio);
		QHash<QString, QStringList> elementAutoNumTitlesByFormula() const;
		std::optional<QUuid> elementAutoNumIdToLink(
				const DiagramContext &information,
				const QHash<QString, QStringList> &titles_by_formula) const;
		bool m_deferring_folios = false;
			/// Opening with folios built only when needed: QET_LAZY_FOLIOS
			/// is set and the database can be filled from the document
		bool m_lazy_open = false;
			/// Saving may write a folio not built as it was read
			/// (writesKeptFolios()): the file is this version's, its
			/// database rows came from it, and nothing project-wide that
			/// a folio's saved items depend on has changed since
		bool m_kept_folios_trusted = false;
			/// Read from a .qet whose path nobody chose since: saved beside it
		bool m_opened_from_qet = false;
			/// The file the saved date and path were set for, and the undo
			/// index then: an unchanged project keeps them (stampSaved())
		QString m_stamped_path;
		int m_stamped_undo_index = 0;
			/// The project file as this QET last read or wrote it, to see
			/// whether something else changed it since (changedOnDisk())
		QString m_disk_path;
		qint64 m_disk_size = -1;
		QDateTime m_disk_mtime;
			/// Saving a .qetz may copy the part of a folio not built
			/// (toXml(), stage D4)
		bool m_kept_parts_trusted = false;
		QSet<QUuid> m_waiting_link_targets;
			/// The entries of the .qetz opened, and the part each folio
			/// was read from, in order
		QHash<QString, QByteArray> m_container_files;
		QStringList m_folio_parts;
		bool m_kept_folios_stale = false;
			/// Something was edited since the project was opened: a table
			/// filled from the database may have changed
		bool m_edited_since_open = false;
		void markKeptFoliosStale() {m_kept_folios_stale = true;}
			/// The file being read, for each folio's text (readDiagramsXml())
		QByteArray m_opened_content;
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
			/// Why the file could not be read, see openError()
		QString m_open_error;
		int m_next_backup_slot = 0;
		QUuid m_uuid = QUuid::createUuid();
		QHash<QString, int> m_derived_uuid_keys;
		QSet<QUuid> m_saved_item_uuids;
			/// While opening: per uuid carried by several symbols, the copy
			/// keeping it (folio index, ordinal on that folio), see
			/// renewedSharedUuid()
		QHash<QUuid, std::pair<int, int>> m_shared_uuid_keepers;	//symbol and wire uuids the file carries, see derivedItemUuid()
		projectDataBase m_data_base;
		QVector<TerminalStrip *> m_terminal_strip_vector;
			//Called after each folio built, see setFolioBuildObserver()
		std::function<void(int, int)> m_folio_build_observer;
		QVector<Cable *> m_cables;

		ProjectPropertiesHandler m_project_properties_handler;
};

Q_DECLARE_METATYPE(QETProject *)
#endif
