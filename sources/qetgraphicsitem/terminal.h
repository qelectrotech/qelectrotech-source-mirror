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
#ifndef TERMINAL_H
#define TERMINAL_H
#include <qet.h>
#include <properties/terminaldata.h>

#include <QtWidgets>
#include <QtXml>
class Conductor;
class Diagram;
class Element;


/**
	@brief The Terminal class
	This class represents a terminal of an electrical element, i.e. a possible
	plug point for conductors.
	This class handles all mouse events for connecting conductors
*/
class Terminal : public QGraphicsObject
{
	Q_OBJECT

	signals:
		void conductorWasAdded(Conductor *conductor);
		void conductorWasRemoved(Conductor *conductor);

	// constructors, destructor
	public:
		Terminal(TerminalData* data, Element *e = nullptr);
		~Terminal() override;

	private:
		Terminal(const Terminal &);

	// methods
	public:
		/**
			@brief type
			Enable the use of qgraphicsitem_cast to safely
			cast a QGraphicsItem into a Terminal
			@return the QGraphicsItem type
		*/
		int type() const override { return Type; }

		void paint(
				QPainter *painter,
				const QStyleOptionGraphicsItem *,
				QWidget *) override;
		void   drawHelpLine (bool draw = true);
		QLineF HelpLine     () const;
		QRectF boundingRect () const override;

		// methods to manage conductors attached to the terminal
		Terminal* alignedWithTerminal () const;
		bool      addConductor        (Conductor *conductor);
		void      removeConductor     (Conductor *conductor);
		int       conductorsCount     () const;
		Diagram  *diagram             () const;
		Element  *parentElement       () const;
		QUuid     uuid                () const;
		void      newUuid             ();
		QUuid     stableUuid          () const;
		QUuid     derivedUuid         () const;
		void      setPlaceRank        (int rank);
		QString   name                () const;
		QString   baseName            () const;
		TerminalData::Type terminalType() const;
		QString potential() const;
		bool useMasterLabel() const { return d->m_use_master_label; }
		void setUseMasterLabel(bool use);
		int masterLabelIndex() const { return d->m_master_label_index; }
		void setMasterLabelIndex(int index);

		QList<Conductor *> conductors() const;
		Qet::Orientation orientation() const;
		QPointF dockConductor() const;
		void updateConductor();
		bool isLinkedTo(Terminal *);
		bool canBeLinkedTo(Terminal *);
		int wireLimit() const;
		bool hasRoomForWire() const;

		// methods related to XML import/export
		static bool valideXml(QDomElement  &);
		bool fromXml (QDomElement &);
		QDomElement toXml (QDomDocument &) const;

	protected:
		// methods related to events management
		void hoverEnterEvent  (QGraphicsSceneHoverEvent *) override;
		void hoverMoveEvent   (QGraphicsSceneHoverEvent *) override;
		void hoverLeaveEvent  (QGraphicsSceneHoverEvent *) override;
		void mousePressEvent  (QGraphicsSceneMouseEvent *) override;
		void mouseMoveEvent   (QGraphicsSceneMouseEvent *) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *) override;

	// attributes
	public:
		enum { Type = UserType + 1002 };

		static const qreal terminalSize;
		static const qreal Z;
		// Various static colors used for hover effects
		/// default color
		static QColor neutralColor;
		/// color for legal actions
		static QColor allowedColor;
		/// color for allowed but fuzzy or not recommended  actions
		static QColor warningColor;
		/// color for forbidden actions
		static QColor forbiddenColor;

		static bool takesConductorColor();
		static void setTakesConductorColor(bool takes);

	private:
		bool               m_draw_help_line{false};
		QGraphicsLineItem *m_help_line{nullptr};
		QGraphicsLineItem *m_help_line_a{nullptr};


		TerminalData* d;

		/// Parent electrical element
		Element *parent_element_{nullptr};
	public:
		/// docking point for parent element
		QPointF dock_elmt_;
	private:
		/// List of conductors attached to the terminal
		QList<Conductor *> m_conductors_list;
		QRectF m_br;
			/// Last terminal seen through an attached conductor
		Terminal *m_previous_terminal = nullptr;
			/// Whether the mouse pointer is hovering the terminal
		bool m_hovered = false;
			/// How many terminals of the definition, before this one, sit at
			/// the same point with the same orientation; see derivedUuid()
		int m_place_rank = 0;
			/// Color used for the hover effect
		QColor m_hovered_color = Terminal::neutralColor;

	private:
		void init();
		void init(QPointF pf, Qet::Orientation o);
};

/**
	@brief Terminal::conductorsCount
	@return the number of conductors attached to the terminal.
*/
inline int Terminal::conductorsCount() const
{
	return(m_conductors_list.size());
}

QList<Terminal *> relatedPotentialTerminal (const Terminal *terminal,
						const bool all_diagram = true,
						const bool ignore_function_separation = false);

/**
	@brief The roles of a junction: which of its connections the wire comes
	in at, and which ones it goes on to.

	The roles belong to the symbol rather than to one copy of it: they are
	read from the definition, so every copy answers alike and no database
	has to hold them.
*/
struct JunctionRoles
{
	QString source;
	QStringList targets;
};

/**
	@brief junctionRolesOf
	@return the roles of @a junction, as its own data holds them: the
	symbol is read once and its roles come back with it, rather than the
	definition being read again for every wire (#1500).
	@param junction the symbol to read, which need not be one
*/
JunctionRoles junctionRolesOf(const Element *junction);

/**
	@brief junctionRolesOf
	@return the roles a definition says. The reader every caller shares,
	so the drawing and the exports always answer alike
	(@see junctionRolesOf(const Element *)).
	@param definition the symbol's definition, or a null element
*/
JunctionRoles junctionRolesOf(const QDomElement &definition);

/**
	@brief junctionSourceKey
	@return the element information a junction's source connection is
	stored against
*/
QString junctionSourceKey();

/**
	@brief junctionTargetKeys
	@return the element informations a junction's connections go on to are
	stored against, in the order the chain follows them
*/
QStringList junctionTargetKeys();

/**
	@brief junctionRoleKeys
	@return every element information a junction role is stored against
*/
QStringList junctionRoleKeys();

/**
	@brief junctionChainNeighbours
	@return the two connections of a junction's chain around @a at: the one
	the wire comes from, and the one it goes on to, either of them null when
	there is none.

	That is the chain the definition gives -- source, then target 1, target
	2, target 3 -- read at the connection @a at sits on. The chain is what
	says which way a wire runs through the junction, so a cable list can
	name its ends and a stub can say where it comes from (#1500).

	@param at a connection of a junction; anything else gives two nulls
	@return {the connection before, the connection after}, in chain order
*/
QPair<Terminal *, Terminal *> junctionChainNeighbours(Terminal *at);

/**
	@brief junctionPassThrough
	@return the connection a wire goes on to when the chain does not say:
	the one other connection of that junction which carries a wire, if
	there is exactly one.

	A junction with a single way out of it has nothing to tell apart, so
	the wire simply goes on that way even when nothing was ever set on the
	symbol -- a corner passes a wire through and needs no source for it
	(#1500). Empty when there is no such connection, or more than one,
	which is where the way on has to be told apart and there is nothing to
	go by.

	@param wired the other connections of the junction which carry a wire
	@return the one of them to go on to, empty for none or several
*/
QString junctionPassThrough(const QStringList &wired);

/**
	@brief junctionChainRoles
	@return the two connections of a chain around @a at: the one the wire
	comes from, and the one it goes on to, either of them empty when there
	is none.

	The chain itself -- source, then target 1, then target 2, then target 3
	-- read as plain uuids rather than as elements, so the drawing and the
	lists, which hold their rows as uuids, follow one rule (#1500).

	@param source the connection the wire comes in at, empty when the
	junction has none set
	@param targets where it goes on to, in the order the lists follow them
	@param at the connection to read the chain at, empty for none
	@return {the connection before, the connection after}, in chain order
*/
QPair<QString, QString> junctionChainRoles(const QString &source, const QStringList &targets,
					   const QString &at);

/**
	@brief endsPastJunctions
	Return the connections a wire ends at, past the junctions it runs
	through.

	A junction is a connection point on a wire rather than a piece of its
	own: it is bought nowhere, built nowhere, and the wire does not end
	there. The chain the junction's definition gives -- what the wire
	comes from, then where it goes on to -- says which way it runs on,
	and a junction reached that way is passed in turn, so a run through
	two of them in a row ends where it really ends (#1500).

	This is the one rule the cable list, the terminal strip and the
	wiring lists name their lands with, so the three can never disagree.

	@param end the connection the wire ends at -- one of a junction's own,
	or one of a piece which names an end
	@return the connections the wire ends at, none of them a junction's own.
	A junction with no source set is passed on when only one way out of it
	carries a wire, because there is nothing to tell apart (#1500), and
	the result is empty when there are more ways than that to choose from,
	or none at all
*/
QList<Terminal *> endsPastJunctions(Terminal *end);

#endif
