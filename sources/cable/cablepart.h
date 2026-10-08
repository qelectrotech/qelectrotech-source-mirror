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
#ifndef CABLEPART_H
#define CABLEPART_H

#include "cable.h"
#include "cablemanager.h"
#include "../qetgraphicsitem/qetgraphicsitem.h"

#include <QColor>
#include <QFont>
#include <QPointer>
#include <QSet>
#include <QUuid>

class Cable;
class CablePartData;
class Conductor;
class QUndoCommand;
class QWidget;

/**
	@brief The CablePart class draws one section of a cable: the trunk
	line of one folio, the number and the type of the cable at its left,
	and a slash with the colour of a core wherever that core crosses a
	conductor.

	All of it is a view of the Cable: the geometry lives in the cable
	itself, so lengthening the trunk line is purely optical. What a core
	wires is read off where its colour label stands: dropping a label
	gives that core the wire it lands on, and letting go of a moved line
	works the entries out the same way, from where the labels have come
	to rest. A label is never sent off to look for a wire and never
	moved to reach one -- it goes on standing where the user put it --
	and no gesture at all takes a core away from its cable.
*/
class CablePart : public QetGraphicsItem
{
		Q_OBJECT

	public:
		explicit CablePart(const CablePartData &data, QGraphicsItem *parent = nullptr);
		explicit CablePart(QGraphicsItem *parent = nullptr);
		~CablePart() override;

		enum {Type = UserType + 1012};
		int type() const override {return Type;}

		void setCable(Cable *cable);
		Cable *cable() const;
		QUuid partUuid() const;
			///Bind the item to its cable once the project is loaded
		void refreshPending();

		void setLine(const QPointF &p1, const QPointF &p2);
		QLineF line() const;
		QPointF firstPoint() const {return m_p1;}
		QPointF secondPoint() const {return m_p2;}

		QRectF boundingRect() const override;
		QPainterPath shape() const override;
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
		QString name() const override;

		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
			///Shows the hand over a line which points at another folio
		void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
			///Opens the dialog editing the properties of the whole cable
		void editProperty() override;

			/**
				Give this line the mouse and wait for the next click to put
				down a core of its cable which is not on the sheet yet: the
				core follows this line wherever the mouse comes near it and
				the click leaves it standing there, snapped to the grid like
				anything else drawn.

				It is always this very section which takes it, never the
				first section of the cable: a cable running over two folios
				stands on two lines, and the one whose dialog was asked for
				is the one being worked on.
				@param core which core of the cable is being put down
			*/
		void startPlacingCore(int core);
			///Stop waiting for that click, putting nothing down at all
		void cancelPlacement();

			/**
				The line which waits for the click putting one of its cores
				down at this very moment, anywhere in the application, or
				nullptr when no line does.

				Only one line at a time may wait -- taking the core always
				belongs to the line whose dialog was asked for -- so Escape
				and the right button can simply ask here and always act on
				the right one, however many folios are open.
			*/
		static CablePart *placingPart();
			/**
				End that wait wherever it stands, without putting anything
				down anywhere: the core goes back to being a free one.

				Used by the sheet itself, which gets the right button and
				Escape before any item does (see DiagramView).
				@return true when a line was waiting for its click
			*/
		static bool cancelPlacing();

			/**
				Which core of this cable has its colour label standing at
				that very place on the sheet -- asked by the sheet itself
				when the right button comes down on a line, so that the
				menu which opens belongs to that one core rather than to
				the whole folio.
				@param scene_pos a point of the sheet, in scene coordinates
				@return the core standing there, -1 when none does
			*/
		int coreAt(const QPointF &scene_pos) const;
			/**
				Take one core off this line: it names no wire any more and
				goes back to being a free slot of its cable, listed among
				the ones which stand nowhere -- the very opposite of
				putting one down from that list.

				Kept as one undo step, so a single undo puts the label
				back exactly where it stood.
				@param core which core standing on this line is taken off
				@return true when a core was taken off
			*/
		bool takeCoreOffLine(int core);

			/**
				Which wire this line runs across at the very place a colour
				label stands -- so that the entry of a wire follows the label
				instead of the label being sent off to find a wire. Also asked
				by a copy being put down (CableCopy::wire), which must not
				move any label to reach a wire either.
				@param crossings every place this line runs across a wire
				@param core the core being worked out
				@param at where its label stands, in scene coordinates
				@return the wire to give the core, or nullptr
			*/
		Conductor *wireFor(const QList<CableCrossing> &crossings,
						   const CableCore &core, const QPointF &at) const;

			/**
				Move this line from outside: the sheet moving a whole
				selection of elements and lines at once asks every line
				of the selection to come along, one step after the other,
				exactly as if the finger had been on the line itself, so
				that one gesture moves everything which is marked instead
				of two halves of the selection going different ways.

				The calls come in order -- begin once, continue as often
				as the gesture needs, settle once for the whole
				selection, then either done or abort -- and a line which
				never moves simply has no undo step to hand over.
			*/
		void beginLineGesture();
			///Take one more step of such a move, by that much
		void continueLineGesture(const QPointF &delta);
			/**
				Where this line comes to rest, worked out the same way as
				after a drag of its own: which wire every colour label now
				stands on, asked before anything changes hands. The step
				of this line is handed over rather than pushed, so that
				the movement as a whole ends up as one single undo step.
				@param steps receives the undo step of this line, if any
				@return false when he called the gesture off
			*/
		bool settleLineGesture(QList<QUndoCommand *> &steps);
			///Put this line back where the gesture found it
		void abortLineGesture();
			///The gesture is over for this line: forget where it began
		void doneLineGesture();

			/**
				One cross reference of this section as the PDF export
				needs it: where the text of the reference stands on this
				folio, and on the folio it points at where the section it
				names stands -- so that exporting makes it a clickable
				link the way a cross reference of an element already is.

				Both rectangles are in scene coordinates, each of its own
				folio.
			*/
		struct PdfRef
		{
				///The folio the reference points at
			Diagram *diagram = nullptr;
				///Where the text of the reference stands on this folio
			QRectF here;
				///Where the section it names stands on that folio
			QRectF there;
		};
		QList<PdfRef> pdfRefs() const;

	protected:
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

	private:
			/**
				One line of the label at the left of the trunk line:
				what is written, which field of the cable it comes from
				and -- worked out from what that field asks for -- the
				font it is written with, the way it lines up and the
				rectangle it sits in.

				Every line carries its own font, because every field
				may have been given one of its own: Anlage, Ort and
				Désignation stand in one block, and their lines may
				very well no longer all be the same size once he has
				set them apart.
			*/
		struct LabelLine
		{
			QString text;
			QString key;
			QFont font;
			Qt::Alignment align = Qt::AlignRight;
			qreal width = 0.0;
			qreal height = 0.0;
			QRectF rect;
		};

			/**
				One line of the label which points at a folio this cable
				runs on besides this one: that folio, the section of the
				cable standing there, and the text this project asks for
				such a reference -- the format chosen for the type
				"Câble" under Querverweise, written out once per folio,
				in the order the folios turn.
			*/
		struct CrossRef
		{
				///The other folio, which always exists while listed
			Diagram *diagram = nullptr;
				///Which section of the cable stands on it
			QUuid part;
				///The text as it is written on the drawing
			QString text;
		};

		QFont textFont() const;
			///The font the colours of the cores are written with
		QFont coreFont() const;
			///The font one named text of this cable is written with
		QFont fontOf(const QString &key) const;
			///How one named text of this cable lines up at the line
		Qt::Alignment alignOf(const QString &key) const;
			/**
				How far the whole label at the left of the line is
				nudged on the drawing: over the line, under it, left
				or right of the end of it. A nudge only, which moves
				the texts and never the line.
				@return the distance in scene units
			*/
		QPointF textOffset() const;
			/**
				How far the row of colours is nudged on the drawing:
				again a distance on the texts only, the slashes and
				the entries behind them staying where they stand.
				@return the distance in scene units
			*/
		QPointF coreOffset() const;
			/**
				Where a text of this width starts, at the left end of
				the trunk line: before the line when it is set to end
				there (the way it always did), on the line when it is
				centred, after it when it starts there.
				@param key the field the text comes from
				@param width how wide the text is, in scene units
				@return the x of the left of the text
			*/
		qreal textLeft(const QString &key, qreal width) const;
		bool trunkIsHorizontal() const;
		QList<LabelLine> labelLines() const;
			/**
				The references to the other folios this cable runs on,
				one line each, stacked under the length of the cable:
				at the very bottom of everything the line says.
				@return those lines, in the order crossRefs() lists them
			*/
		QList<LabelLine> refLines() const;
			/**
				Every other folio this cable runs on, one entry each, in
				the order the folios turn -- empty when the cable runs on
				this folio alone, which is the usual case.
			*/
		QList<CrossRef> crossRefs() const;
			/**
				Which of those references stands at that very place, so
				that both the hand of the mouse and a double click know
				which line they are about.
				@param scene_pos a point of the sheet, in scene coordinates
				@return the index of that reference, -1 when none stands there
			*/
		int crossRefAt(const QPointF &scene_pos) const;
			/**
				Send the mouse to the section of the cable which stands
				on the folio the reference at that place points at -- the
				very leap an element's cross reference takes, straight to
				the place to look at.
				@param scene_pos a point of the sheet, in scene coordinates
				@return true when the click stood on a reference
			*/
		bool goToCrossRef(const QPointF &scene_pos);
			/**
				Keep the reference lines up to date with the project: a
				folio which is added, removed or moved changes what they
				say, and so does the format chosen under Querverweise.
			*/
		void setUpXrefHooks();
		QString typeText() const;
		QString lengthText() const;
		QRectF labelRect() const;
		qreal underTop() const;
		QRectF typeRect() const;
		QRectF lengthRect() const;
		QRectF coreLabelRect(const QPointF &at, const QString &color) const;
		QPointF coreLabelPivot(const QPointF &at, const QString &color) const;
		QPointF coreDrawPosition(const CableCore &core) const;
		bool coreDrawn(const CableCore &core) const;
		QPointF dragPosition(const QPointF &scene_pos) const;
			/**
				Where the mouse would put down a core which waits to be
				put down: the mouse projected onto the line itself, at a
				grid step, and never past either end of the line -- the
				same coordinates a dragged label would be drawn at, only
				worked out from the mouse rather than from a gesture.
				@param scene_pos where the mouse stands, in scene coordinates
				@return where the slash would be drawn
			*/
		QPointF placePosition(const QPointF &scene_pos) const;
			/**
				Snap a distance measured along the line to a grid step, in
				the coordinates of the sheet -- so a horizontal line steps
				along x and a vertical one along y. Holding Ctrl lets go of
				the grid, as everywhere else on the sheet.
				@param along how far from the first end of the line
				@return that distance at a grid step
			*/
		qreal stepAlong(qreal along) const;
		void drawLabel(QPainter *painter, const QColor &ink) const;
		void drawCores(QPainter *painter, const QColor &ink) const;
			///Draw the slash of one core and the colour written beside it
		void drawCoreMark(QPainter *painter, const QColor &ink,
						  const QPointF &at, const QString &color) const;
		void syncGeometry();
		bool labelAt(int core, const QPointF &scene_pos) const;
		///Answer to the question of taking a wire another cable holds
		enum class ClaimAnswer {
			///Take it: the other cable lets it go
			Take,
			///Leave it: nothing changes hands, the rest still happens
			Leave,
			///Cancel: this whole gesture is called off, as if undone
			Cancel
		};
			/**
				@param core which core is being let go
				@param dropped_at where its label comes to rest
				@param before every core the cable held before this one was
				added to it, so that the one step of undo takes the addition
				back as well; nullptr while a core which is already on the
				sheet is merely moved
				@return false when the user called the whole thing off
			*/
		bool rebindCore(int core, const QPointF &dropped_at,
						const QList<CableCore> *before = nullptr);
			/**
				Put a core which is not on the sheet yet onto this line: it
				stands where the mouse says, and if a wire runs across that
				very place it names that wire the same way a dragged label
				would. Called off means back to being a free core, with one
				step of undo to take it away again.
				@param core which core of the cable is being put down
				@param at where its label is to stand, in scene coordinates
				@return false when nothing was put down
			*/
		bool placeCore(int core, const QPointF &at);
			/**
				Send the mouse to this line, to the grid step the mark of
				the waiting core stands on, so that the very next click is
				the one which puts the core down -- wherever he asked for
				the core from, and whether he asks for this very one a
				second time or not. The mouse goes there only where the
				line is really in sight: a folio hidden behind another
				one, or off the edge of the view, is left alone rather
				than jumped to.
			*/
		void sendMouseToLine();
		void reconcileCores(QList<CableCore> &cores, const QPointF &delta) const;
		bool settleCores(QList<CableCore> &cores) const;
		ClaimAnswer mayClaim(Conductor *conductor) const;
		ClaimAnswer mayClaimSeveral(const QList<Conductor *> &conductors) const;
		ClaimAnswer askClaim(QWidget *parent,
							 const QString &title,
							 const QString &question) const;

	private:
			///The one line which waits for the click putting a core down,
			///null once there is none: taking the core belongs to the line
			///whose dialog was asked for, never to the first line of the
			///cable, and the sheet has to be able to cancel that wait
			///without knowing which scene it stands in
		static QPointer<CablePart> s_placing;
		QPointer<Cable> m_cable;
			///Identity of the cable this part belongs to, kept until the
			///project has loaded it and setCable() can be called
		QUuid m_pending_cable;
		QUuid m_part_uuid;
		QPointF m_p1;
		QPointF m_p2;

			///Core being dragged by its slash, -1 when none
		int m_dragged_core = -1;
			///Core waiting to be put down by the next click, -1 when none
		int m_pending_core = -1;
			///True while the mouse stands close enough to this line for
			///the waiting core to be put down where it shows
		bool m_pending_valid = false;
			///True between the click which put a core down and the second
			///click Qt counts as a double click, so that a click which was
			///there to place something does not open the dialog as well
		bool m_just_placed = false;
			///Where the dragged slash follows the mouse
		QPointF m_drag_scene;
		QPointF m_press_scene;
		QPointF m_move_p1;
		QPointF m_move_p2;
			///True while the trunk line is being dragged, so undo knows
			///there is a move to take back
		bool m_line_moved = false;
			///Which end of the trunk line is being pulled by one of its
			///blue points: 0 none, 1 the first end, 2 the second
		int m_stretch_end = 0;
			///Every core as it stands while the line is being dragged:
			///every label is carried along by however far the line has
			///come, so the line, its description and the colours of its
			///cores move as one thing and no label ever jumps, lands on
			///a neighbour's wire or disappears under his hand
		QList<CableCore> m_line_drag_cores;
			///Every core as it was when the line was picked up, so undo
			///can put the labels back with the line
		QList<CableCore> m_cores_before;
			///Another line he picked up at the same time: a drag takes
			///the whole selection with it, so each of them is carried
			///along and each of them goes back to where it stood if he
			///calls the gesture off
		struct CarriedLine
		{
			CablePart *part = nullptr;
				///Where that line ran when the gesture began
			QPointF p1;
			QPointF p2;
				///What its cable held when the gesture began
			QList<CableCore> cores;
		};
		QList<CarriedLine> m_carried;

			///True when the sheet has been told that the rest of the
			///selection comes along with this line (see carrySelection),
			///so that it is told once per gesture rather than once per
			///step -- and never again once it had nothing to move
		bool m_mover_started = false;
			///How far of this gesture the sheet has already been told
		QPointF m_told_mover;

			///The lines which keep the references to the other folios of
			///this cable up to date, taken down and taken again whenever
			///this section comes to stand in another folio
		QList<QMetaObject::Connection> m_xref_connections;

		void carryCompanions(const QPointF &delta);
			/**
				Everything else he marked at the same time comes along
				too -- the elements, the texts and the images of the same
				selection -- by the very step this line has taken, so
				that one gesture moves all of it instead of the lines
				going one way and the rest another.

				What this line measures is how far it has come all in
				altogether, from where the press found it, while the
				sheet takes a step at a time: so only the part it has not
				been told yet is handed over.
				@param total_delta how far this line has come since the press
			*/
		void carrySelection(const QPointF &total_delta);
		bool settleCompanions(QList<QUndoCommand *> &steps);
		void putCompanionsBack();
		void readyToSettle(QList<CableCore> &cores) const;
};

#endif // CABLEPART_H
