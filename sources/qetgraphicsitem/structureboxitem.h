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

/**
	@brief The StructureBoxItem class
	A rectangle drawn on a diagram purely for documentation: it carries its
	own IEC 81346 plant (=) / location (+) / prefix (-) identity and no
	electrical meaning (no terminals). Elements visually contained within it
	use its identity as their reference point for %{plant}/%{location}/
	%{structure_id} instead of the diagram's own title block -- see
	discussion #649. Implemented as a QetShapeItem subclass, always using
	ShapeType::Rectangle, to reuse its resize-handle/move/selection behavior.
*/
class StructureBoxItem : public QetShapeItem
{
	Q_OBJECT

	public:
		enum { Type = UserType + 1012 };

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

		static QString xmlTagName() { return QStringLiteral("structureBox"); }

	protected:
		void paint(
				QPainter *painter,
				const QStyleOptionGraphicsItem *option,
				QWidget *widget) override;

	private:
		QString m_plant, m_location, m_prefix;
};

#endif // STRUCTUREBOXITEM_H
