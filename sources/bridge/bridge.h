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
	the bridge is drawn; the terminal name tells which of the terminals
	of that symbol it is, for symbols which have more than one.
*/
struct BridgeTerminal
{
	QUuid element;
	QString terminal;

	friend bool operator==(const BridgeTerminal &a, const BridgeTerminal &b)
	{
		return a.element == b.element && a.terminal == b.terminal;
	}
	friend bool operator!=(const BridgeTerminal &a, const BridgeTerminal &b)
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
			///How many terminals this bridge may hold,
			///unlimited_connections (0) when there is no limit
		int maxConnections() const;
		void setMaxConnections(int max);
			///True when @p adding more terminals would cross the limit
		bool wouldExceed(int adding) const;

			///The parallel track this bridge runs in at the terminals it
			///crosses, 1 to max_columns; 0 when it was never worked out
		int column() const;
		void setColumn(int column);

		//Terminals
		QList<BridgeTerminal> terminals() const;
		bool hasTerminal(const BridgeTerminal &terminal) const;
		void addTerminal(const BridgeTerminal &terminal);
		void removeTerminal(const BridgeTerminal &terminal);
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
		QUuid m_uuid;
		QColor m_color = Qt::black;
		int m_max_connections = unlimited_connections;
		int m_column = 0;
		QList<BridgeTerminal> m_terminals;
		QList<BridgePartData> m_parts;
};

#endif // BRIDGE_H
