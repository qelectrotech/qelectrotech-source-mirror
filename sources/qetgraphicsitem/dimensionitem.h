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

struct DimensionProperties
{
	bool isAutoValue = true;
	QString customText;
	QString prefix;
	QString suffix;
	int precision = 1;
	QString unit = QStringLiteral("mm");
	bool showUnit = false;
	bool isFramed = true;
	bool useComma = true;
	qreal textDistance = 1.0;
	bool textCenter = true;
	bool showDualUnit = false;
	bool suppressZero = false;
	bool fractionFormat = false;
	bool isTextHorizontal = false;
	bool isTextInLine = true;
	qreal lineWidth = 1.2;
	qreal extensionOvershoot = 5.0;
	qreal extensionGap = 0.0;
	qreal offset = 20.0;
	int terminationStyle = 0; // 0 = Arrows, 1 = Slashes
	qreal arrowSize = 8.0;
	bool maskBackground = true;
	int fontSize = 8;

	bool operator==(const DimensionProperties &o) const {
		return isAutoValue == o.isAutoValue &&
		       customText == o.customText &&
		       prefix == o.prefix &&
		       suffix == o.suffix &&
		       precision == o.precision &&
		       unit == o.unit &&
		       showUnit == o.showUnit &&
		       isFramed == o.isFramed &&
		       useComma == o.useComma &&
		       qFuzzyCompare(textDistance, o.textDistance) &&
		       textCenter == o.textCenter &&
		       showDualUnit == o.showDualUnit &&
		       suppressZero == o.suppressZero &&
		       fractionFormat == o.fractionFormat &&
		       isTextHorizontal == o.isTextHorizontal &&
		       isTextInLine == o.isTextInLine &&
		       qFuzzyCompare(lineWidth, o.lineWidth) &&
		       qFuzzyCompare(extensionOvershoot, o.extensionOvershoot) &&
		       qFuzzyCompare(extensionGap, o.extensionGap) &&
		       qFuzzyCompare(offset, o.offset) &&
		       terminationStyle == o.terminationStyle &&
		       qFuzzyCompare(arrowSize, o.arrowSize) &&
		       maskBackground == o.maskBackground &&
		       fontSize == o.fontSize;
	}
	bool operator!=(const DimensionProperties &o) const {
		return !(*this == o);
	}
};

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
	Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY propertiesChanged)
	Q_PROPERTY(QString suffix READ suffix WRITE setSuffix NOTIFY propertiesChanged)
	Q_PROPERTY(int precision READ precision WRITE setPrecision NOTIFY propertiesChanged)
	Q_PROPERTY(QString unit READ unit WRITE setUnit NOTIFY propertiesChanged)
	Q_PROPERTY(bool showUnit READ showUnit WRITE setShowUnit NOTIFY propertiesChanged)
	Q_PROPERTY(bool isFramed READ isFramed WRITE setFramed NOTIFY propertiesChanged)
	Q_PROPERTY(bool useComma READ useComma WRITE setUseComma NOTIFY propertiesChanged)
	Q_PROPERTY(qreal textDistance READ textDistance WRITE setTextDistance NOTIFY propertiesChanged)
	Q_PROPERTY(bool isTextHorizontal READ isTextHorizontal WRITE setTextHorizontal NOTIFY propertiesChanged)
	Q_PROPERTY(bool isTextInLine READ isTextInLine WRITE setTextInLine NOTIFY propertiesChanged)
	Q_PROPERTY(bool suppressZero READ suppressZero WRITE setSuppressZero NOTIFY propertiesChanged)
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

	void editProperty() override;

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

	QString prefix() const;
	void setPrefix(const QString &p);

	QString suffix() const;
	void setSuffix(const QString &s);

	int precision() const;
	void setPrecision(int p);

	QString unit() const;
	void setUnit(const QString &u);

	bool showUnit() const;
	void setShowUnit(bool s);

	bool isFramed() const;
	void setFramed(bool f);

	bool useComma() const;
	void setUseComma(bool c);

	qreal textDistance() const;
	void setTextDistance(qreal d);

	bool isTextCenter() const;
	void setTextCenter(bool c);

	bool showDualUnit() const;
	void setShowDualUnit(bool d);

	bool isFractionFormat() const;
	void setFractionFormat(bool f);

	bool isTextHorizontal() const;
	void setTextHorizontal(bool h);

	bool isTextInLine() const;
	void setTextInLine(bool il);

	bool suppressZero() const;
	void setSuppressZero(bool sz);

	qreal lineWidth() const;
	void setLineWidth(qreal w);

	qreal extensionOvershoot() const;
	void setExtensionOvershoot(qreal o);

	qreal extensionGap() const;
	void setExtensionGap(qreal g);

	qreal arrowSize() const;
	void setArrowSize(qreal s);

	bool maskBackground() const;
	void setMaskBackground(bool m);

	int fontSize() const;
	void setFontSize(int s);

	DimensionMode mode() const;
	void setMode(DimensionMode mode);

	TerminationStyle terminationStyle() const;
	void setTerminationStyle(TerminationStyle style);

	DimensionProperties properties() const;
	void setProperties(const DimensionProperties &p);

	qreal dimensionValueMM() const;
	QString displayText() const;

	QList<DimensionItem *> findChainSegments() const;

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
	QString m_prefix;
	QString m_suffix;
	int m_precision = 1;
	QString m_unit = QStringLiteral("mm");
	bool m_show_unit = false;
	bool m_framed = true;
	bool m_use_comma = true;
	qreal m_text_distance = 1.0;
	bool m_text_center = true;
	bool m_show_dual_unit = false;
	bool m_fraction_format = false;
	bool m_text_horizontal = false;
	bool m_text_in_line = true;
	bool m_suppress_zero = false;
	qreal m_line_width = 1.2;
	qreal m_extension_overshoot = 5.0;
	qreal m_extension_gap = 0.0;
	qreal m_arrow_size = 8.0;
	bool m_mask_background = true;
	DimensionMode m_mode = Linear;
	TerminationStyle m_termination_style = Arrows;
	QUuid m_uuid;
	QPen m_pen;
	QFont m_font;
};

#endif // DIMENSIONITEM_H
