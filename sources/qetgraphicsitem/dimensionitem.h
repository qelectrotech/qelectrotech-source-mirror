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
#ifndef DIMENSIONITEM_H
#define DIMENSIONITEM_H

#include "qetgraphicsitem.h"
#include <QFont>
#include <QPen>
#include <QUuid>

class QDomDocument;
class QDomElement;

/**
	@brief The DimensionItem class
	Represents a CAD linear or chain dimension item in a diagram.
	Scale factor: strictly 2 px = 1.0 mm (distance in mm = length / 2.0).
*/
class DimensionItem : public QetGraphicsItem
{
	Q_OBJECT

	Q_PROPERTY(QPointF p1 READ p1 WRITE setP1 NOTIFY geometryChanged)
	Q_PROPERTY(QPointF p2 READ p2 WRITE setP2 NOTIFY geometryChanged)
	Q_PROPERTY(qreal offset READ offset WRITE setOffset NOTIFY geometryChanged)
	Q_PROPERTY(bool isAutoValue READ isAutoValue WRITE setAutoValue NOTIFY propertiesChanged)
	Q_PROPERTY(QString customText READ customText WRITE setCustomText NOTIFY propertiesChanged)
	Q_PROPERTY(DimensionMode mode READ mode WRITE setMode NOTIFY propertiesChanged)
	Q_PROPERTY(TerminationStyle terminationStyle READ terminationStyle WRITE setTerminationStyle NOTIFY propertiesChanged)
	Q_PROPERTY(qreal dimensionValueMM READ dimensionValueMM NOTIFY geometryChanged)

public:
	enum { Type = UserType + 1013 };
	int type() const override { return Type; }

	enum DimensionMode {
		Linear,
		Chain
	};
	Q_ENUM(DimensionMode)

	enum TerminationStyle {
		Arrows,
		Slashes
	};
	Q_ENUM(TerminationStyle)

	DimensionItem(DimensionMode mode = Linear, QGraphicsItem *parent = nullptr);
	DimensionItem(const QPointF &p1, const QPointF &p2, qreal offset = 20.0,
	              DimensionMode mode = Linear, QGraphicsItem *parent = nullptr);
	~DimensionItem() override;

	QRectF boundingRect() const override;
	QPainterPath shape() const override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

	QPointF p1() const;
	void setP1(const QPointF &p1);

	QPointF p2() const;
	void setP2(const QPointF &p2);

	qreal offset() const;
	void setOffset(qreal offset);

	bool isAutoValue() const;
	void setAutoValue(bool auto_val);

	QString customText() const;
	void setCustomText(const QString &text);

	DimensionMode mode() const;
	void setMode(DimensionMode mode);

	TerminationStyle terminationStyle() const;
	void setTerminationStyle(TerminationStyle style);

	qreal dimensionValueMM() const;
	QString displayText() const;

	QUuid uuid() const { return m_uuid; }
	void setUuid(const QUuid &uuid) { m_uuid = uuid; emit uuidChanged(); }
	void newUuid() { setUuid(QUuid::createUuid()); }

	QDomElement toXml(QDomDocument &document) const;
	void fromXml(const QDomElement &element);

signals:
	void geometryChanged();
	void propertiesChanged();
	void uuidChanged();

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
	void calculateGeometry(QPointF &p1_dim, QPointF &p2_dim,
	                       QPointF &p1_ext_end, QPointF &p2_ext_end,
	                       QPointF &mid_dim, qreal &angle_deg,
	                       QPointF &unit_dir, QPointF &normal_dir) const;

	QPointF m_p1;
	QPointF m_p2;
	qreal m_offset = 20.0;
	bool m_auto_value = true;
	QString m_custom_text;
	DimensionMode m_mode = Linear;
	TerminationStyle m_termination_style = Arrows;
	QUuid m_uuid;
	QPen m_pen;
	QFont m_font;
};

#endif // DIMENSIONITEM_H

