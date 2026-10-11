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
#ifndef CABLE_H
#define CABLE_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QPointF>
#include <QStringList>
#include <QUuid>

class QDomDocument;
class QDomElement;

/**
	@brief One core of a cable which is wired to a conductor of the drawing.

	A cable owns its assignment: this is the only place in QElectroTech
	which knows that core number 3 of cable 15W1 lands on such a
	conductor. The conductor itself only carries the text the assignment
	generates (15W1:br), written back by CableManager::refreshLabels(),
	so the text can never drift away from the cable it comes from.
*/
struct CableCore
{
		/// Core number inside the cable, 0 based, from 0 to coreCount()-1
	int core = -1;
		/// The part carrying the slash drawn for this core
	QUuid part;
		/// The conductor this core lands on, null when the core is free
	QUuid conductor;
		/// Where the slash sits, in the coordinates of its own part
	QPointF position;

		///Two assignments are the same when they say the same thing, so
		///undo can tell a gesture which changed something from one which
		///merely put the slash back where it already was
	friend bool operator==(const CableCore &a, const CableCore &b)
	{
		return a.core == b.core && a.part == b.part
				&& a.conductor == b.conductor && a.position == b.position;
	}
	friend bool operator!=(const CableCore &a, const CableCore &b)
	{
		return !(a == b);
	}
};

/**
	@brief One drawn section of a cable: the trunk line of one folio.

	The geometry is kept as plain data rather than as a pointer to the
 graphics item, because the folio owns the item and deletes it when the
 folio is deleted, while the cable outlives it (it may have another
 part on another folio).
*/
struct CablePartData
{
	QUuid uuid;
	QUuid diagram;
	QPointF p1;
	QPointF p2;

	bool isHorizontal() const {return qFuzzyCompare(p1.y(), p2.y());}
};

/**
	@brief How one of the texts of a cable is written.

	Both fields may say nothing at all, and that is the normal case:
	the cable then writes that text the way the preferences offer it
	(QETApp::cableTextsFont() and QETApp::cableCoreFont()), so picking
	another default in the preferences reaches every cable which has
	not been given a font of its own.

	The texts of a cable are kept in one hash rather than one pair of
	members each, so that a new text can be added without dragging a
	new field through the whole code. They are named the way the file
	writes them:
	"designation", "type", "installation", "location", "length" for the
	five texts written next to the trunk line, and "cores" for the
	colour labels of the cores -- which have no alignment, being turned
	by a quarter along their own line.
*/
struct CableTextFormat
{
		///The font, in the stable description QETUtils::fontToString()
		///writes; empty when this cable follows the preferences
	QString font;
		///Qt::AlignLeft, Qt::AlignHCenter or Qt::AlignRight; 0 when the
		///text goes on ending where it always did, before the line
	int alignment = 0;

	bool operator==(const CableTextFormat &other) const
	{
		return font == other.font && alignment == other.alignment;
	}
	bool operator!=(const CableTextFormat &other) const
	{
		return !(*this == other);
	}
};

/**
	@brief Everything about a cable the user may type in or tick, and
	whether what he typed is written on the sheet at all.

	It is kept as a whole so that one undo step can put a whole edit
	back, and so the selection panel and the dialog always offer exactly
	the same fields in the same order.
*/
struct CableProperties
{
		///Name of the cable, e.g. 15W1 -- the head of the label
	QString designation;
		/**
			Where that name comes from: false while the program worked
			it out of the numbering rule, true from the moment it was
			typed in. Nothing shows it and no field edits it -- it
			travels with the fields so that undo puts it back with them,
			and so that numbering the whole project over again knows
			which names were never the rule's to begin with and may be
			left alone.
		*/
	bool designation_by_hand = false;
		///Kind of cable, e.g. H07RN-F 4G1,5
	QString type;
		///Installation the cable belongs to
	QString installation;
		///Place the cable runs through
	QString location;
		///Length, typed in by hand
	QString length;

		///What of that is written next to the trunk line of the drawing
	bool show_designation = true;
	bool show_type = true;
	bool show_installation = true;
	bool show_location = true;
	bool show_length = true;

		///How each text is written -- font and alignment, keyed by the
		///names listed on CableTextFormat. A text which is not in here
		///follows what the preferences say, which is where a cable
		///freshly drawn stands for all of them.
	QHash<QString, CableTextFormat> texts;

		///How far the label at the left of the trunk line is nudged from
		///where it has always stood. It is a nudge on the drawing only,
		///and it moves the whole label as one thing: installation, place
		///and designation over the line, the type and the length under
		///it -- while the line, the slashes and the wiring keep standing
		///exactly where they were.
	QPointF text_offset;
		///How far the row of colours is nudged from the slashes it names.
		///Again on the drawing only: the slashes, the entries they mark
		///and every wire behind them stay where they are, and so does the
		///row itself when the trunk line is moved.
	QPointF core_offset;

	bool operator==(const CableProperties &other) const
	{
		return designation == other.designation
				&& designation_by_hand == other.designation_by_hand
				&& type == other.type
				&& installation == other.installation
				&& location == other.location
				&& length == other.length
				&& show_designation == other.show_designation
				&& show_type == other.show_type
				&& show_installation == other.show_installation
				&& show_location == other.show_location
				&& show_length == other.show_length
				&& texts == other.texts
				&& text_offset == other.text_offset
				&& core_offset == other.core_offset;
	}
	bool operator!=(const CableProperties &other) const
	{
		return !(*this == other);
	}
};

/**
	@brief Everything about a cable which its type decides, taken as one
	block so that changing the type is one single step.

	The cores go with the type rather than with the text fields: taking a
	type with fewer cores takes those cores off the drawing, and undoing
	has to bring their marks back along with the name and the colours.
*/
struct CableTypeState
{
		///Kind of cable, e.g. H07RN-F 4G1,5
	QString type;
		///How many cores that type has
	int core_count = 0;
		///The colour of each of them, in core order
	QStringList core_colors;
		///The cores the cable really holds at this moment
	QList<CableCore> cores;

	bool operator==(const CableTypeState &other) const
	{
		return type == other.type
				&& core_count == other.core_count
				&& core_colors == other.core_colors
				&& cores == other.cores;
	}
	bool operator!=(const CableTypeState &other) const
	{
		return !(*this == other);
	}
};

/**
	@brief The Cable class is the project wide object a drawn cable line
	stands for.

	It carries what the user fills in -- the type, the number, BMK,
	installation, location -- together with what QElectroTech works out:
	how many cores the type has, which colour each core has, which cores
	are wired to which conductor, and which folios the cable runs on.

	Properties belong to the cable, never to one of its parts: editing
	the type or the number anywhere changes it for every part.
*/
class Cable : public QObject
{
		Q_OBJECT

	public:
		explicit Cable(QObject *parent = nullptr);

		//Identity
		QUuid uuid() const;
		void setUuid(const QUuid &uuid);

		//Properties the user fills in
		QString designation() const;
		void setDesignation(const QString &designation);
			/**
				True when the name of this cable was typed in rather
				than worked out of the numbering rule: numbering the
				whole project over asks before it takes such a name.
			*/
		bool designationByHand() const;
		void setDesignationByHand(bool by_hand);
		QString bmk() const;
		void setBmk(const QString &bmk);
		QString installation() const;
		void setInstallation(const QString &installation);
		QString location() const;
		void setLocation(const QString &location);
		QString length() const;
		void setLength(const QString &length);

			///Every field of the cable, and what of it is drawn on the sheet
		CableProperties properties() const;
		void setProperties(const CableProperties &properties);

		//Type, copied from the cable type file when the cable is created
		QString type() const;
		void setType(const QString &type);
		int coreCount() const;
		void setCoreCount(int count);
		QStringList coreColors() const;
		void setCoreColors(const QStringList &colors);
		///Everything the type of this cable decides, taken as one block
	CableTypeState typeState() const;
		///Put back a state taken with typeState(), in one single pass
	void applyTypeState(const CableTypeState &state);

		//Cores
		QList<CableCore> usedCores() const;
		bool hasCore(int core) const;
		CableCore core(int core) const;
		void setCore(const CableCore &core);
			///Write a whole set of assignments in place of another
		void setCores(const QList<CableCore> &cores);
		void removeCore(int core);
		int usedCoreCount() const;
		int freeCoreCount() const;
		///The lowest core number which is free, -1 when none is
		int firstFreeCore() const;
		///The colour of a core, as written in the cable type file
		QString colorOfCore(int core) const;
		///The text a conductor of this core shows, e.g. "15W1:br"
		QString labelOfCore(int core) const;

		//Parts
		QList<CablePartData> parts() const;
		bool hasPart(const QUuid &part) const;
		CablePartData part(const QUuid &part) const;
		void addPart(const CablePartData &part);
		void removePart(const QUuid &part);
		void setPartGeometry(const QUuid &part, const QPointF &p1, const QPointF &p2);
		void setPartDiagram(const QUuid &part, const QUuid &diagram);
			///Carry the colour labels of one section along when that
			///section is moved as a whole (paste, duplicate, a selection
			///being moved with everything else on it)
		void movePartCores(const QUuid &part, const QPointF &delta);

		//Serialisation
		QDomElement toXml(QDomDocument &document) const;
		void fromXml(const QDomElement &element);

	signals:
		void changed();

	private:
		QUuid m_uuid;
		QString m_designation;
			///< where that name comes from (see designationByHand)
		bool m_designation_by_hand = false;
		QString m_bmk;
		QString m_installation;
		QString m_location;
		QString m_length;
		QString m_type;
		int m_core_count = 0;
		QStringList m_core_colors;
		QList<CableCore> m_cores;
		QList<CablePartData> m_parts;
			///What the label next to the trunk line is allowed to show
		bool m_show_designation = true;
		bool m_show_type = true;
		bool m_show_installation = true;
		bool m_show_location = true;
		bool m_show_length = true;
			///The texts this cable writes its own way, by the names
			///listed on CableTextFormat; everything not in here follows
			///what the preferences say
		QHash<QString, CableTextFormat> m_texts;
			///How far the label at the left of the line and the row of
			///colours are nudged on the drawing: distances nothing else
			///in the project ever looks at
		QPointF m_text_offset;
		QPointF m_core_offset;
};

/**
	@brief One core which was taken away from another cable.

	A conductor belongs to one cable only, so drawing a second cable
	across wires a first one already holds takes those cores away from
	it. That has to be remembered where it is decided, so that undoing
	the second cable gives the first one its cores back instead of
	leaving those conductors with a blank cable field forever.
*/
struct CableCoreTaken
{
		///The cable the core was taken from; null when it is gone
	QPointer<Cable> cable;
		///The core exactly as the other cable held it
	CableCore core;
};

#endif // CABLE_H
