/*
		Copyright 2006-2026 QElectroTech Team
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
#ifndef PROJECTDATABASE_H
#define PROJECTDATABASE_H

#include <functional>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QPointer>
#include <QFileDialog>
#include <QHash>
#include <QMap>
#include <QDomDocument>
#include <QSet>
#include <QUuid>

#include <optional>

#include "../diagramcontext.h"
#include "../titleblockproperties.h"
#include "../borderproperties.h"
#include "../conductorproperties.h"
#include "../autoNum/assignvariables.h"
#include "../autoNum/numerotationcontext.h"

class Element;
class QETProject;
class Diagram;
class Conductor;
class Terminal;
class QGraphicsItem;
class QDomDocument;
class BorderTitleBlock;
class DiagramContext;
class QDate;

/**
	@brief The projectDataBase class
	This class wraps a sqlite data base where you can find several things
	about the content of a project.
 *
	@note this class is still in development.
*/
class projectDataBase : public QObject
{
	Q_OBJECT

	public:
		projectDataBase(QETProject *project, QObject *parent = nullptr);
		virtual ~projectDataBase() override;

		void updateDB();
			/**
				updateDB() for a project just read from @p document: the
				diagram, element, terminal and conductor tables are filled
				from the document when it carries everything they need (see
				populateFromDocument()), from the built folios otherwise.
			*/
		void updateDB(const QDomDocument &document);
			/**
				Why updateDB(document) will fill the tables from the folios
				rather than from @p document, for the reasons the document
				alone can tell; empty when it knows of none. Read before the
				folios are: a project whose tables will come from the folios
				builds every folio as it opens, even with QET_LAZY_FOLIOS.
			*/
		static QString documentRefusal(const QDomDocument &document);
			/**
				Suppress the full rebuild performed by updateDB().

				While blocked, updateDB() returns immediately instead of
				repopulating every table. Meant for bulk operations -- notably
				loading a project, where each table model re-queries the
				database as it is built and would otherwise trigger one
				complete rebuild of it. The caller unblocks and calls
				updateDB() once when done; @see QETProject::readProjectXml().
			*/
		void setUpdateBlocked(bool blocked);
		QETProject *project() const;
		QSqlQuery newQuery(const QString &query = QString(), QString *error = nullptr);
		static bool isReadOnlySelect(const QString &query, QString *error = nullptr);

			/**
				The most rows any caller reads out of one query result.

				A SELECT is not bounded by how much data the project holds:
				SQLite produces rows lazily, so a query that never stops
				producing them makes the loop that reads them never stop
				either. A recursive CTE does exactly that in one line, and
				a <graphics_table>'s <query> is stored in the .qet and run
				on load -- so the text can arrive from a file rather than
				from the person at the keyboard, and opening that file is
				the whole attack.

				100000 is far above any real result: the largest table in
				the shipped examples is 396 rows. It is a backstop, not a
				page size -- a caller that hits it has almost certainly
				been handed something it should not run to completion, and
				says so rather than truncating quietly.
			*/
		static constexpr int MaxResultRows = 100000;
		QSqlDatabase database() const {return m_data_base;}
		int excludedConductorCount() const;

			//The information of every symbol placed in the project, by
			//symbol uuid: the store each symbol's information is written
			//through. A symbol not placed
			//(a preview, the symbol editor) keeps its own.
		DiagramContext elementInformation(const QUuid &element) const;
		bool hasElementInformation(const QUuid &element) const;
			//How many placed symbols hold @p element: more than one in a
			//file whose symbols were copied by an old version (F100), or
			//for a moment while a copy's uuid is renewed
		int placedElementCount(const QUuid &element) const;
		bool storeElementInformation(const QUuid &element, const DiagramContext &information);
		void elementInformationChanged(Element *element, const DiagramContext &information);
		void forgetElementInformation(const QUuid &element);
		void elementUuidChanged(Element *element, const QUuid &old_uuid);
			//Placed symbols whose information differs from the store's,
			//each as "uuid: why"; empty when they all agree
		QStringList elementInformationMismatches() const;

			//The kind information of every symbol placed in the project, by
			//symbol uuid (NO/NC/SW/power and the like, baked into the
			//symbol's own definition, never edited after it is built): the
			//store each symbol's kind information is written through. A
			//symbol not placed keeps its own.
		DiagramContext elementKindInformation(const QUuid &element) const;
		bool storeElementKindInformation(const QUuid &element, const DiagramContext &information);
		void forgetElementKindInformation(const QUuid &element);

			//The title block properties of every folio of the project, by
			//folio uuid: the store each folio's title block is written
			//through. A folio not in a
			//project keeps its own.
		TitleBlockProperties folioTitleBlock(const QUuid &folio) const;
		const TitleBlockProperties *storedFolioTitleBlock(const QUuid &folio) const;
		Diagram *placedFolio(const QUuid &folio) const;
		void storeFolioTitleBlock(const QUuid &folio, const TitleBlockProperties &properties);
		void placeFolio(Diagram *folio);
		void folioUuidChanged(Diagram *folio, const QUuid &old_uuid);
		QStringList folioTitleBlockMismatches() const;

			//The border properties of every folio of the project, by folio
			//uuid: the store each folio's border is written through. A
			//folio not in a project keeps its own.
		const BorderProperties *storedFolioBorder(const QUuid &folio) const;
		void storeFolioBorder(const QUuid &folio, const BorderProperties &properties);

			//The folio number and total folio count of every folio of the
			//project, by folio uuid: recomputed whenever a folio is added,
			//removed or reordered (QETProject::updateDiagramsFolioData()),
			//the store each folio's number is written through.
		bool storedFolioNumber(const QUuid &folio, int *index, int *total) const;
		void storeFolioNumber(const QUuid &folio, int index, int total);

			//The properties of every wire placed in the project, by wire
			//uuid: the store each wire's properties are written through
			//A wire not placed keeps its own.
		ConductorProperties conductorProperties(const QUuid &conductor) const;
		bool hasConductorProperties(const QUuid &conductor) const;
		int placedConductorCount(const QUuid &conductor) const;
		void conductorPropertiesStored(Conductor *conductor, const ConductorProperties &properties);
		void conductorUuidChanged(Conductor *conductor, const QUuid &old_uuid);
		QStringList conductorPropertiesMismatches() const;
		QStringList linkRowMismatches();
			/// A numbering scheme of the project (QETZ-DB-FIRST stage 2b)
		struct NumberingScheme {
			QString kind;   ///< conductor, element, folio or cable
			QString title;
			QUuid uuid;     ///< element schemes only
			NumerotationContext context;
		};
			/// The project's numbering: its schemes of every kind and the
			/// settings saved with them
		struct Numbering {
			QList<NumberingScheme> schemes;
			QString current_conductor;
			QString current_element;
			QString current_cable;
			bool freeze_new_conductors = false;
			bool freeze_new_elements = false;
			bool auto_break_conductors = false;
			bool cable_x_axis_first = true;
			bool cable_ask_numbering = true;
		};
		void setNumbering(const Numbering &numbering);
		Numbering numbering() const;
		QHash<QString, NumerotationContext> numberingSchemes(const QString &kind) const;
		bool hasNumberingScheme(const QString &kind, const QString &title) const;
		NumerotationContext numberingScheme(const QString &kind, const QString &title) const;
		QUuid numberingSchemeUuid(const QString &kind, const QString &title) const;
		QString numberingSchemeTitle(const QString &kind, const QUuid &uuid) const;
		void setNumberingScheme(const QString &kind, const QString &title,
								const NumerotationContext &context,
								const QUuid &uuid = QUuid());
		void removeNumberingScheme(const QString &kind, const QString &title);
		void renameNumberingScheme(const QString &kind, const QString &old_title,
								   const QString &new_title);
		QString numberingSetting(const QString &name, const QString &default_value = QString()) const;
		bool numberingFlag(const QString &name, bool default_value) const;
		void setNumberingSetting(const QString &name, const QString &value);
		void setNumberingFlag(const QString &name, bool value);
			//2b-4: a folio's own numbering counters (unit/ten/hundred-folio
			//sequentials), one row per (folio, owner, level, scheme, ord).
			//owner is "element" or "conductor"; level is "unitfolio",
			//"tenfolio" or "hundredfolio" (kept as Diagram's own strings,
			//not renamed, so the two stay comparable in logs/greps).
		QStringList folioCounterValues(const QUuid &folio_uuid, const QString &owner,
										const QString &level, const QString &title) const;
		void setFolioCounterValues(const QUuid &folio_uuid, const QString &owner,
								   const QString &level, const QString &title,
								   const QStringList &values);
		void renameFolioCounterScheme(const QString &owner, const QString &old_title,
									  const QString &new_title);
		bool savedLinks(const Element *element, QList<std::pair<QUuid, int>> *links);
			//The link rows are written (opening filled them): a symbol
			//built links from them (Element::initLink())
		bool holdsLinks() const {return m_holds_links;}
		void forgetLinks() {m_holds_links = false;}
		QList<std::pair<QUuid, int>> partnersToLink(const QUuid &element);
		QList<std::pair<QUuid, int>> linkRowsOf(const QUuid &element) const;
		QSet<QUuid> waitingPartners(const QUuid &element, const QSet<QUuid> &waiting);
		QSet<QUuid> waitingSymbols() const;
		QSet<QUuid> unbuiltSymbols() const;

			//Fill the three stores from a project's document before its
			//folios are built, then, once
			//they are, say how many of those values building agreed with.
		void prefillFromDocument(const QDomDocument &document);
		void endPrefill(bool keep_unplaced = false);
			//A folio of a project opened with lazy folios is being built:
			//its rows came from the document already, so none are written
			//and the database is not marked changed
		void setBuildingFolio(bool building);
		bool isBuildingFolio() const {return m_building_folios > 0;}
		bool rowsFromDocument() const {return m_rows_from_document;}
		QString prefillReport() const;
		QSet<QUuid> foliosWithUnjoinedWires() const;
		QSet<QUuid> folioUuidsOfElements(const QSet<QUuid> &elements) const;
		bool canEditUnbuilt(const QUuid &element) const;
		void editUnbuiltElementInformation(const QUuid &element, DiagramContext information);
		std::optional<DiagramContext> takeUnbuiltEdit(const QUuid &element);
		QSet<QUuid> foliosWithUnbuiltEdits() const;
		QHash<QUuid, DiagramContext> unbuiltEditsOnFolio(const QUuid &folio, bool *need_build = nullptr) const;
		bool setPlacedElementInformation(const QUuid &element, const DiagramContext &information);
		bool unbuiltWire(const QUuid &wire, QUuid *folio = nullptr) const;
		void editUnbuiltConductorProperties(const QUuid &wire, const ConductorProperties &properties);
		std::optional<ConductorProperties> takeUnbuiltWireEdit(const QUuid &wire);
		QHash<QUuid, ConductorProperties> unbuiltWireEditsOnFolio(const QUuid &folio) const;
		bool setPlacedConductorProperties(const QUuid &wire, const ConductorProperties &properties);
		QHash<QUuid, QString> elementTypesOnFolios(const QSet<QUuid> &folios) const;
			/// A symbol placed on a folio, as the element table has it
		struct FolioSymbol {
			QUuid uuid;
			QUuid folio;
			QString kind;       ///< ElementData::typeToString()
			QString location;   ///< its definition, ElementsLocation::path()
			autonum::sequentialNumbers sequence;
		};
		QList<FolioSymbol> symbolsOnFolios(const QList<QUuid> &folios) const;
		QList<QUuid> linkedSymbols(const QUuid &element) const;
			/// A wire placed on a folio: its text and the kinds of the
			/// symbols at its ends (ElementData::typeToString())
		struct FolioWire { QString text; QString kind1; QString kind2; };
		QList<FolioWire> wiresOnFolios(const QList<QUuid> &folios) const;
		QString elementLabel(const QUuid &element) const;
		QString terminalName(const QUuid &terminal, const QUuid &element) const;
		std::optional<QSet<QUuid>> potentialWires(const QUuid &wire) const;
		static QVariant sequenceValue(const autonum::sequentialNumbers &sequence);
		static autonum::sequentialNumbers sequenceFromValue(const QVariant &value);

		void addElement         (Element *element);
		void removeElement      (Element *element);
		void elementInfoChanged (Element *element);
		void elementInfoChanged (QList<Element *> elements);

		void addDiagram         (Diagram *diagram);
		void removeDiagram      (Diagram *diagram);
		void diagramInfoChanged (Diagram *diagram);
		void diagramOrderChanged();

		void addConductor       (Conductor *conductor);
		void removeConductor    (Conductor *conductor);
		void updateConductor    (Conductor *conductor);

			//Shapes, independent texts and images: the folio's drawing
			//furniture. Anything else passed here is ignored.
		void addDrawingItem     (QGraphicsItem *item);
		void removeDrawingItem  (QGraphicsItem *item);
		void itemGroupChanged   (QGraphicsItem *item);

	private slots:
			//Refresh the sender()'s row after Conductor::setProperties().
		void conductorPropertiesChanged();
			//Queue the sender()'s drawing-item row for rewriting.
		void drawingItemChanged();
		void drawingItemDestroyed(QObject *object);
			//Queue the sender()'s link rows for rewriting.
		void linksChanged();
			//Queue the sender() element's folio cell for rewriting.
		void elementMoved();

	public:
		void spareTerminalsChanged();

	signals:
		void dataBaseUpdated();

	private:
		bool createDataBase();
		void createElementNomenclatureView();
		void createSummaryView();
		void createWiringListView();
		void createDrawingItemView();
		void populateDiagramTable();
		void updateFolioPositions();
		void flushElementPositions();
		void populateElementTable();
		void populateElementInfoTable();
		void populateElementInfoTableFromFolios();
		void populateSpareTerminalTable();
		void populateDiagramInfoTable();
		void populateConductorTable();
		void populateLinkTable();
		void flushLinks();
		void insertLinkRows(QSqlQuery &insert, Element *element, const std::function<bool(const QUuid &)> &waiting,
							const QList<std::pair<QUuid, int>> &old_rows);
		bool builtAlone(Element *element) const;
		bool linksLater(const QUuid &uuid) const;
		QList<std::pair<QUuid, int>> rowsToWrite(Element *element, const std::function<bool(const QUuid &)> &waiting,
												 const QList<std::pair<QUuid, int>> &old_rows) const;
		void populateDrawingItemTables();
		bool populateFromDocument(const QDomDocument &document, QString *why = nullptr);
		bool writeDrawingItem(QObject *object);
		void flushDrawingItems();
		void forgetDrawingItem(QObject *object);
		void bindConductorValues(QSqlQuery &query, Conductor *conductor, Diagram *diagram);
		void watchConductor(Conductor *conductor);
		void insertTerminal(Terminal *terminal);
		void insertTerminal(const QString &uuid, const QString &element_uuid,
							const QString &name, const QVariant &index);
		void prepareQuery();
		static QHash<QString, QString> elementInfoToString(
				Element *elmt);
		void bindDiagramInfoValues(QSqlQuery &query, Diagram *diagram);
		static void bindDiagramInfoValues(QSqlQuery &query, const QUuid &diagram_uuid,
										  const BorderTitleBlock &border);
		static void bindDiagramInfoValues(QSqlQuery &query, const QUuid &diagram_uuid,
										  const DiagramContext &infos, const QDate &date);
		static void bindElementValues(QSqlQuery &query, Element *element, Diagram *diagram);
		static void bindElementInfoValues(QSqlQuery &query, Element *element);
		static void bindElementInfoValues(QSqlQuery &query, const QString &element_uuid,
										  const DiagramContext &informations,
										  const QString &label);

	private:
		QPointer<QETProject> m_project;
		bool m_update_blocked = false;
			//Starts true : the database is empty until the first rebuild.
			//Set by every method of this class that writes rows, cleared by
			//updateDB(). Callers reach the database from outside only through
			//newQuery(), and every such call site reads.
		bool m_content_changed = true;
		QSqlDatabase m_data_base;
		QSqlQuery m_insert_elements_query,
				  m_insert_element_info_query,
				  m_remove_element_query,
				  m_remove_element_info_query,
				  m_update_element_query,
				  m_insert_diagram_query,
				  m_remove_diagram_query,
				  m_insert_diagram_info_query,
				  m_update_diagram_info_query,
				  m_diagram_order_changed,
				  m_diagram_info_order_changed,
				  m_insert_terminal_query,
				  m_insert_conductor_query,
				  m_update_conductor_query,
				  m_remove_conductor_query,
				  m_cascade_remove_element_info_query,
				  m_cascade_remove_terminal_query,
				  m_cascade_remove_conductor_query,
				  m_cascade_remove_element_query,
				  m_insert_shape_query,
				  m_insert_independent_text_query,
				  m_insert_image_query,
				  m_store_remove_query,
				  m_store_insert_query,
				  m_remove_links_query,
				  m_conductor_properties_insert_query,
				  m_conductor_property_remove_query;

			//Which uuid's row each drawing item last wrote, and which item
			//wrote each row. A pasted copy is added to the folio still
			//carrying its source's uuid and renewed only afterwards, so two
			//live items can briefly share one: the row belongs to whichever
			//wrote it, and the other waits in m_dirty_drawing_items until its
			//uuid is its own. @see writeDrawingItem().
		QHash<QObject *, QUuid> m_drawing_item_row;
		QHash<QUuid, QObject *> m_drawing_row_owner;
		QSet<QObject *> m_dirty_drawing_items;
			//Elements whose links changed since their link rows were last
			//written: a link is made first and its contact group set after,
			//so the rows are written when next read, as drawing items are.
		QList<QPointer<Element>> m_dirty_link_elements;
		QSqlQuery m_saved_links_query;
		bool m_holds_links = false;
			/// waitingSymbols() as it was last, for these folios not built
			/// and this count of symbols built alone on them
		mutable QList<Diagram *> m_waiting_folios;
		mutable int m_waiting_alone = 0;
		mutable QSet<QUuid> m_waiting_symbols;
			//Elements moved since their folio cell was last written: a move
			//sends xChanged and yChanged for every step of a drag, so the
			//cell is written when next read, as drawing items are.
		QList<QPointer<Element>> m_moved_elements;
		QHash<QUuid, DiagramContext> m_element_information;
		QHash<QUuid, DiagramContext> m_element_kind_information;
			//The placed symbols holding each uuid: a paste or a folio copy
			//briefly gives a copy its original's uuid, so one uuid can have
			//two symbols until the copy's is renewed.
		QMultiHash<QUuid, QPointer<Element>> m_placed_elements;
			/// Information changed through the store for symbols on folios
			/// not built (editUnbuiltElementInformation()), with the folio
			/// each is on: building the symbol takes it (takeUnbuiltEdit())
		struct UnbuiltEdit { DiagramContext information; QUuid folio; bool had_information; };
		QHash<QUuid, UnbuiltEdit> m_unbuilt_edits;
			/// The same for wires (editUnbuiltConductorProperties())
		struct UnbuiltWireEdit { ConductorProperties properties; QUuid folio; };
		QHash<QUuid, UnbuiltWireEdit> m_unbuilt_wire_edits;
		QHash<QUuid, TitleBlockProperties> m_folio_titleblocks;
		QHash<QUuid, BorderProperties> m_folio_borders;
		QHash<QUuid, QPair<int, int>> m_folio_numbers; // index, total
		QHash<QUuid, QPointer<Diagram>> m_placed_folios;
		QHash<QUuid, ConductorProperties> m_conductor_properties;
			//The rows of conductor_properties, to write only what changes
		QHash<QUuid, QHash<QString, QString>> m_conductor_attributes;
		QSet<QUuid> m_dirty_conductor_properties;
		int m_building_folios = 0;
		QSet<QUuid> m_folios_with_unjoined_wires;
			//The folios whose rows a rebuild keeps (keepUnbuiltFolioRows())
		QSet<Diagram *> m_kept_folios;
			//The tables' rows were filled from the document: those of a
			//folio not built yet are right (lazy folios)
		bool m_rows_from_document = false;
		void keepUnbuiltFolioRows();
		void restoreKeptRows(const QString &table, Diagram *folio, const QString &where);
		void dropKeptRows();
		bool m_changed_before_folio = false, m_blocked_before_folio = false;
		QHash<QUuid, DiagramContext> m_prefilled_elements;
		QHash<QUuid, TitleBlockProperties> m_prefilled_folios;
		QHash<QUuid, ConductorProperties> m_prefilled_conductors;
		struct PrefillTally {
			int agree = 0, differ = 0, unplaced = 0;
			QMap<QString, int> fields;
		};
		PrefillTally m_prefill_tally[3];
		void flushConductorProperties();
		QMultiHash<QUuid, QPointer<Conductor>> m_placed_conductors;
		bool storeConductorProperties(const QUuid &conductor, const ConductorProperties &properties);
		void placeConductor(Conductor *conductor);
		void unplaceConductor(Conductor *conductor, const QUuid &uuid);
		Conductor *otherConductorHolder(const QUuid &uuid, const Conductor *conductor) const;
		void writeConductorRow(Conductor *conductor);
		void removeConductorRow(const QUuid &uuid);
		void unplaceFolio(Diagram *folio, const QUuid &uuid);
		void placeElement(Element *element);
		Element *otherHolder(const QUuid &uuid, const Element *element) const;
		void writeElementRows(Element *element);
		void queueLinks(Element *element);
		void removeElementRows(const QUuid &uuid);
		void unplaceElement(Element *element, const QUuid &uuid);

#ifdef QET_EXPORT_PROJECT_DB
	public:
		static void exportDb(projectDataBase *db,
				     QWidget *parent = nullptr,
				     const QString &caption = QString(),
				     const QString &dir = QString());
#endif
};

#endif // PROJECTDATABASE_H
