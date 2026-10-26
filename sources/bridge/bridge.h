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
#ifndef BRIDGE_H
#define BRIDGE_H

#include <QColor>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QUuid>

class QDomDocument;
class QDomElement;

/**
	@brief One drawn section of a bridge: the line of one folio.

	The geometry is kept as plain data rather than as a pointer to the
	graphics item, because the folio owns the item and deletes it when
	the folio is deleted, while the bridge outlives it (it may have
	another section on another folio). Same rule as a cable.
*/
struct BridgePartData
{
	QUuid uuid;
	QUuid diagram;
	QPointF p1;
	QPointF p2;
};

/**
	@brief One terminal a bridge lands on, named by identity.

	The element uuid names the terminal symbol of the drawing, the way
	the bridge is drawn; the terminal uuid tells which of the pins of
	that symbol it is -- a symbol may carry several, each of them its
	own potential, and they have no names to tell them apart.
*/
struct BridgeTerminal
{
	QUuid element;
	QString terminal;
		///True when this landing is the connection of a sheet reference
		///rather than a terminal: a bridge carried on to another sheet
		///is no terminal connection, and never counts as a Klemme
	bool reference = false;

	friend bool operator==(const BridgeTerminal &a, const BridgeTerminal &b)
	{
		return a.element == b.element && a.terminal == b.terminal;
	}
	friend bool operator!=(const BridgeTerminal &a, const BridgeTerminal &b)
	{
		return !(a == b);
	}
};

inline size_t qHash(const BridgeTerminal &terminal, size_t seed = 0)
{
	return qHash(terminal.element, seed) ^ qHash(terminal.terminal, seed);
}

/**
	@brief What a bridge crosses: the voltage/protocol of the wires it
	sits on, and nothing else. A bridge carries no function -- the
	function of a terminal is its own business and a separated function
	stays separated however many bridges sit across it.
*/
struct BridgeInformation
{
	QString tension;

	friend bool operator==(const BridgeInformation &a, const BridgeInformation &b)
	{
		return a.tension == b.tension;
	}
	friend bool operator!=(const BridgeInformation &a, const BridgeInformation &b)
	{
		return !(a == b);
	}
};

/**
	@brief The Bridge class is the project wide object a drawn bridge
	line stands for: a terminal bridge (Reihenbruecke) sitting across
	terminals in the cabinet.

	It is built like a cable: the object carries what the user fills in
	-- the colour, how many terminals the bridge may hold -- plus what
	QElectroTech works out: which terminals it lands on and on which
	folios it is drawn. Properties belong to the bridge, never to one of
	its sections: editing the colour anywhere changes it everywhere.

	What the bridge carries as information (function, potential) is
	never typed in: it is taken over from the first terminal it lands
	on, the way the bridge is built in a cabinet -- see
	BridgeManager::functionOf() and BridgeManager::tensionOf().
*/
class Bridge : public QObject
{
		Q_OBJECT

	public:
		explicit Bridge(QObject *parent = nullptr);

			///How many terminals a bridge may hold when the user says
			///nothing: as many as he draws (see maxConnections())
		static constexpr int unlimited_connections = 0;
			///How thick a bridge is drawn when the user says nothing:
			///as thick as a conductor is by default
		static constexpr qreal default_line_width = 1.0;
			///How many bridges may run side by side through one terminal,
			///the four bridge points of the drawing
		static constexpr int max_columns = 4;

		//Identity
		QUuid uuid() const;
		void setUuid(const QUuid &uuid);
		void newUuid();

		//Properties the user fills in
		QColor color() const;
		void setColor(const QColor &color);
			///How thick the lines of this bridge are drawn, in the same
			///way a conductor takes its size: 1 unless the user says
			///otherwise
		qreal lineWidth() const;
		void setLineWidth(qreal width);
			///How many terminals this bridge may hold,
			///unlimited_connections (0) when there is no limit
		int maxConnections() const;
		void setMaxConnections(int max);
			///True when @p adding more terminals would cross the limit
		bool wouldExceed(int adding) const;
			///How many Klemmen this bridge holds: a sheet reference it is
			///carried on by is no Klemme and never counts as one
		int klemmCount() const;

			///The parallel track this bridge runs in at the terminals it
			///crosses, 1 to max_columns; 0 when it was never worked out
		int column() const;
		void setColumn(int column);

			//What the bridge carries: the voltage/protocol of the wires
			//it sits on, and only that -- never a function. Taken over
			//from them, never typed in; when they do not agree the user
			//is asked which the bridge takes, the way sheet references
			//are linked.
		QString tensionText() const;
		void setTensionText(const QString &text);

			//The name the hand gives the bridge, so two of them can be
			//told apart: "24V" says what it carries, the name says which
			//one it is. Empty when nobody named it.
		QString name() const;
		void setName(const QString &name);

		//Terminals
		QList<BridgeTerminal> terminals() const;
		bool hasTerminal(const BridgeTerminal &terminal) const;
		void addTerminal(const BridgeTerminal &terminal);
		void removeTerminal(const BridgeTerminal &terminal);
			///Write a whole set of terminals in place of another: what a
			///line really crosses, worked out again after it was changed
		void setTerminals(const QList<BridgeTerminal> &terminals);
		int terminalCount() const;

		//Sections
		QList<BridgePartData> parts() const;
		bool hasPart(const QUuid &part) const;
		void addPart(const BridgePartData &part);
		void removePart(const QUuid &part);
		void setPartGeometry(const QUuid &part, const QPointF &p1, const QPointF &p2);

		//Serialisation
		QDomElement toXml(QDomDocument &document) const;
		void fromXml(const QDomElement &element);
		static QString xmlTagName() {return QStringLiteral("bridge");}

	signals:
		void changed();

	private:
		QString liveTension() const;

	private:
		QUuid m_uuid;
		QColor m_color = Qt::black;
		qreal m_line_width = 1.0;
		int m_max_connections = unlimited_connections;
		int m_column = 0;
		QString m_tension_text;
		QString m_name;
		QList<BridgeTerminal> m_terminals;
		QList<BridgePartData> m_parts;
};

#endif // BRIDGE_H
