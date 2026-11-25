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
#ifndef STRUCTUREBOXITEM_H
#define STRUCTUREBOXITEM_H

#include "diagramtextitem.h"
#include "qetshapeitem.h"

#include <QColor>
#include <QPointF>
#include <QPointer>

class Element;
class StructureBoxLabelItem;

/**
	@brief The StructureBoxItem class
	A rectangle drawn on a diagram purely for documentation: it carries its
	own IEC 81346 plant (=) / location (+) / prefix (-) identity and no
	electrical meaning (no terminals). Elements visually contained within it
	use its plant and location as their reference point for
	%{structure_id} and %{structure_id_full} instead of the diagram's own
	title block (an empty one is the next box's, then the folio's);
	%{plant} and %{location} stay the element's own. See
	discussion #649. Implemented as a QetShapeItem subclass, always using
	ShapeType::Rectangle, to reuse its resize-handle/move/selection behavior.

	Saved as a <shape> carrying a structure_box attribute and its identity,
	so every path that keeps shapes (the .qet, the project database and the
	.qetdb, folios not built, copy and paste) keeps a box too; an older
	QElectroTech reads it as a plain rectangle.
*/
class StructureBoxItem : public QetShapeItem
{
	friend class StructureBoxLabelItem;
	Q_OBJECT
	Q_PROPERTY(QString plant READ plant WRITE setPlant NOTIFY identityChanged)
	Q_PROPERTY(QString location READ location WRITE setLocation NOTIFY identityChanged)
	Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY identityChanged)
	Q_PROPERTY(QString higherPlant READ higherPlant WRITE setHigherPlant NOTIFY identityChanged)
	Q_PROPERTY(QString higherLocation READ higherLocation WRITE setHigherLocation NOTIFY identityChanged)
	Q_PROPERTY(qreal labelSize READ labelSize WRITE setLabelSize NOTIFY labelSizeChanged)

	public:
		enum { Type = UserType + 1015 };

		explicit StructureBoxItem(QPointF p1, QPointF p2 = QPointF(0, 0), QGraphicsItem *parent = nullptr);
		~StructureBoxItem() override = default;

		int type() const override { return Type; }

		QString plant() const { return m_plant; }
		void setPlant(const QString &plant);
		QString location() const { return m_location; }
		void setLocation(const QString &location);
		QString prefix() const { return m_prefix; }
		void setPrefix(const QString &prefix);
			/// The IEC 81346 level above its plant (==), over the folio's
		QString higherPlant() const { return m_higher_plant; }
		void setHigherPlant(const QString &higher_plant);
			/// The level above its location (++), over the folio's
		QString higherLocation() const { return m_higher_location; }
		void setHigherLocation(const QString &higher_location);

		QString structureId() const;

			/// Its name: a text of its own, which can stand anywhere
		StructureBoxLabelItem *label() const {return m_label;}
		QPointF labelOffset() const;
		void setLabelOffset(const QPointF &offset);
		qreal labelSize() const {return m_label_size;}
		void setLabelSize(qreal size);

		bool fromXml(const QDomElement &e) override;
		QDomElement toXml(QDomDocument &document) const override;

		void editProperty() override;
		QString name() const override;

			/// The colour new boxes are drawn in: black unless set
		static QColor newBoxColor();
		static void setNewBoxColor(const QColor &color);
			/// Whether the place is asked for as soon as a box is drawn:
			/// off unless set
		static bool askPlaceWhenDrawn();
		static void setAskPlaceWhenDrawn(bool ask);

			/// Whether @p e, a <shape>, is a structure box
		static bool isStructureBoxXml(const QDomElement &e);

			/// A plant (=) and a location (+), and the product levels (-)
			/// above the symbols there: the prefixes of the boxes around,
			/// outermost first, each with its sign ("-B2-A1"); empty
			/// when no box has one
		struct Place
		{
			QString plant;
			QString location;
			QString product;
				/// The IEC 81346 levels above the plant (==) and the
				/// location (++): the smallest box's that has one, else
				/// the folio's
			QString higher_plant;
			QString higher_location;

				/// The plant with its == level in front ("==A1=P1"), for
				/// AssignVariables::buildStructureId()
			QString plantPart() const;
				/// The location with its ++ level in front ("++S1+L1")
			QString locationPart() const;
		};

			/// The boxes @p element lies in, the smallest first. "In" means
			/// the centre of the symbol is inside the box. The one rule for
			/// every feature that asks where a symbol is.
		static QList<const StructureBoxItem *> enclosing(const Element *element);
			/// The same among @p folio_boxes, boxesOf() the element's folio:
			/// for reading many symbols of one folio, as a scene lookup per
			/// symbol is slow on a large project
		static QList<const StructureBoxItem *> enclosing(
				const Element *element,
				const QList<const StructureBoxItem *> &folio_boxes);
			/// The structure boxes drawn on @p diagram, possibly null
		static QList<const StructureBoxItem *> boxesOf(const Diagram *diagram);
			/// Where the boxes around @p element and its folio put it: field
			/// by field, an empty plant or location of a box is the next
			/// box's, then the folio's. The product is the prefixes of all
			/// the boxes around it, an empty one skipped. The element's own
			/// values are not read.
		static Place referencePlace(const Element *element);
		static Place referencePlace(const Element *element,
									const QList<const StructureBoxItem *> &folio_boxes);
			/// Where @p element is: its own plant and location, each one
			/// taken from referencePlace() when it is empty.
		static Place placeOf(const Element *element);
			/// Work out again the texts of the symbols of @p diagram which
			/// show a reference designation (%{structure_id...}) or a place
			/// (%{plant}, %{location}): what a box or the folio gives them
			/// changed, or the boxes were loaded after them
		static void refreshSymbolTexts(Diagram *diagram, bool label_fields = false);
			/// The same for the texts of @p element alone: it moved
		static void refreshTextsOf(Element *element, bool label_fields = false);

	signals:
			/// Its plant, location, prefix, == or ++ changed
		void identityChanged();
			/// The size of its name changed, by an edit or an undo
		void labelSizeChanged();


	protected:
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

	private:
		void refreshLabel();
		void placeLabel();
		QPointF cornerLabelPos() const;
		void labelPosChanged();
		static QList<const StructureBoxItem *> boxesAmong(const QList<QGraphicsItem *> &items);
		static Place placeIn(const Element *element,
							 const QList<const StructureBoxItem *> &boxes);

		QString m_plant, m_location, m_prefix;
		QString m_higher_plant, m_higher_location;
			/// The folio it was last on: the symbols it leaves are
			/// refreshed once it is gone
		QPointer<Diagram> m_folio;
		StructureBoxLabelItem *m_label = nullptr;
			/// Not at the corner: it stays where it is put. Worked out from
			/// where it stands, so undoing a move puts it back to the corner.
		bool m_label_moved = false;
		qreal m_label_size = 8;
};

/**
	@brief The StructureBoxLabelItem class
	The name of a structure box: selected and dragged on its own, wherever
	it is wanted, in one undo step. A double click edits the box.
*/
class StructureBoxLabelItem : public DiagramTextItem
{
	Q_OBJECT

	public:
		enum { Type = UserType + 1017 };
		explicit StructureBoxLabelItem(StructureBoxItem *box);
		int type() const override { return Type; }
		void fromXml(const QDomElement &) override {}

	protected:
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

	private:
		StructureBoxItem *m_box;
		QPointF m_grab;
		QPointF m_start;
		bool m_dragging = false;      ///< past the drag distance: a click is not a move
		bool m_move_box = false;      ///< part of a selection with its box: the box drives
};

#endif // STRUCTUREBOXITEM_H
