/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "../qet.h"

#include <QtWidgets>
#include <QtXml>
class Conductor;
class Diagram;
class Element;
class TerminalData;

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
		QString   name                () const;

		QList<Conductor *> conductors() const;
		Qet::Orientation orientation() const;
		QPointF dockConductor() const;
		void updateConductor();
		bool isLinkedTo(Terminal *);
		bool canBeLinkedTo(Terminal *);

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
						const bool all_diagram = true);

#endif
