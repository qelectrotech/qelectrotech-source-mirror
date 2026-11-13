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

#include "qetshapeitem.h"

#include <QPointer>

class Element;

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
	Q_OBJECT
	Q_PROPERTY(QString plant READ plant WRITE setPlant NOTIFY identityChanged)
	Q_PROPERTY(QString location READ location WRITE setLocation NOTIFY identityChanged)
	Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY identityChanged)

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

		QString structureId() const;

		bool fromXml(const QDomElement &e) override;
		QDomElement toXml(QDomDocument &document) const override;

		void editProperty() override;
		QString name() const override;

			/// Whether @p e, a <shape>, is a structure box
		static bool isStructureBoxXml(const QDomElement &e);

			/// A plant (=) and a location (+)
		struct Place
		{
			QString plant;
			QString location;
		};

			/// The boxes @p element lies in, the smallest first. "In" means
			/// the centre of the symbol is inside the box. The one rule for
			/// every feature that asks where a symbol is.
		static QList<const StructureBoxItem *> enclosing(const Element *element);
			/// Where the boxes around @p element and its folio put it: field
			/// by field, an empty plant or location of a box is the next
			/// box's, then the folio's. The element's own values are not read.
		static Place referencePlace(const Element *element);
			/// Where @p element is: its own plant and location, each one
			/// taken from referencePlace() when it is empty.
		static Place placeOf(const Element *element);
			/// Work out again the texts of the symbols of @p diagram which
			/// show a reference designation (%{structure_id...}) or a place
			/// (%{plant}, %{location}): what a box or the folio gives them
			/// changed, or the boxes were loaded after them
		static void refreshSymbolTexts(Diagram *diagram);
			/// The same for the texts of @p element alone: it moved
		static void refreshTextsOf(Element *element);

	signals:
			/// Its plant, location or prefix changed
		void identityChanged();


	protected:
		void paint(
				QPainter *painter,
				const QStyleOptionGraphicsItem *option,
				QWidget *widget) override;
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

	private:
		QString m_plant, m_location, m_prefix;
			/// The folio it was last on: the symbols it leaves are
			/// refreshed once it is gone
		QPointer<Diagram> m_folio;
};

#endif // STRUCTUREBOXITEM_H
