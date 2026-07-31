/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "qetutils.h"
#include <QString>
#include <QStringList>
#include <QGraphicsView>
#include "../qetapp.h"
#include "../qetdiagrameditor.h"

/**
	@brief QETUtils::marginsToString
	@param margins
	@return QString str "left;top;right;bottom"
*/
QString QETUtils::marginsToString(const QMargins &margins)
{
	QString str;
	str += QString::number(margins.left());
	str += ";";
	str += QString::number(margins.top());
	str += ";";
	str += QString::number(margins.right());
	str += ";";
	str += QString::number(margins.bottom());

	return str;
}

/**
	@brief QETUtils::marginsFromString
	@param string
	@return QMargins margins
*/
QMargins QETUtils::marginsFromString(const QString &string)
{
	QMargins margins;
	auto split = string.split(";");
	if (split.size() != 4)
		return  margins;

	margins.setLeft  (split.at(0).toInt());
	margins.setTop   (split.at(1).toInt());
	margins.setRight (split.at(2).toInt());
	margins.setBottom(split.at(3).toInt());

	return  margins;
}

/**
 * @brief QETUtils::graphicsHandlerSize
 * @param item
 * @return Return the handler size to use in the QGraphicsScene of @a item.
 * If size can't be found, return 10 by default.
 */
qreal QETUtils::graphicsHandlerSize(QGraphicsItem *item)
{
	if (const auto scene_ = item->scene())
	{
		if (!scene_->views().isEmpty())
		{
			if (const auto editor_ = QETApp::instance()->diagramEditorAncestorOf(scene_->views().at(0)))
			{
				const auto variant_ = editor_->property("graphics_handler_size");
					//If variant_ can't be converted to qreal, the returned qreal is 0.0
					//it's sufficient for us to check if value is set or not.
				if (const auto value_ =  variant_.toReal())
					return value_;
			}
		}
	}

		//Default value
	return 10;
}

/**
 * @brief QETUtils::sortBeginIntString
 * Sort the string @a str_a and @a str_b and take in
 * count if string begin with an int to sort it
 * as int and not as string in this case.
 * For example if we have to sort the string :
 * "3str", 10str", "100str", "2str", "20str".
 * The default behavior when sorting QString with the comparison operator will be:
 * "10str" "100str" "2str", "20str", "3str"
 * When sorting with this function, the result will be :
 * "10str", "2str", "3str", "20str", "100str"
 * @param str_a
 * @param str_b
 * @return
 */
bool QETUtils::sortBeginIntString(const QString &str_a, const QString &str_b)
{
	const QRegularExpression rx(QStringLiteral("^\\d+"));
	int int_a =-1;
	int int_b =-1;

	auto match_a = rx.match(str_a);
	if (match_a.hasMatch()) {
		int_a = match_a.captured(0).toInt();
	}

	auto match_b = rx.match(str_b);
	if (match_b.hasMatch()) {
		int_b = match_b.captured(0).toInt();
	}

		//Sort as numbers if both string
		//start at least by a digit and
		//the number of each string are different.
		//Else sort as string
	if (int_a >= 0 &&
		int_b >= 0 &&
		int_a != int_b) {
		return int_a<int_b;
	}
	else {
		return str_a<str_b;
	}
}

/**
 * @brief QETUtils::pixelSizedFont
 * Set the font size to pixelSize instead of pointSize (if needed).
 * The font used to draw diagram must be pixel sized instead of point sized (default by Qt)
 * then no matter the screen dpi used, the text in diagram keep the same size.
 * For more information see @link https://qelectrotech.org/forum/viewtopic.php?pid=6267#p6267 @endlink
 * @param font
 */
void QETUtils::pixelSizedFont(QFont &font)
{
    if (font.pixelSize() > -1) {
        return;
    }

    auto px = font.pointSizeF()/72 * QFontMetrics{font}.fontDpi();
    font.setPixelSize(qRound(px));
}

namespace
{
	/**
	 * Legacy (Qt 5) weight <- OpenType weight, closest match,
	 * same table Qt uses when parsing a 10/11 field string.
	 */
	int legacyFontWeight(const int opentype_weight)
	{
		static const int weight_map[9][2] = {
			{0, 100}, {12, 200}, {25, 300}, {50, 400}, {57, 500},
			{63, 600}, {75, 700}, {81, 800}, {87, 900}
		};
		int legacy_weight = weight_map[0][0];
		int closest_diff = qAbs(opentype_weight - weight_map[0][1]);
		for (int i = 1 ; i < 9 ; ++i)
		{
			const int diff = qAbs(opentype_weight - weight_map[i][1]);
			if (diff < closest_diff) {
				closest_diff = diff;
				legacy_weight = weight_map[i][0];
			}
		}
		return legacy_weight;
	}
}

/**
 * @brief QETUtils::fontToString
 * Serialize a font to the 10/11 field description format written by Qt 5,
 * to be used instead of QFont::toString() everywhere a font description is
 * stored in a project, element or settings file.
 * The format of QFont::toString() is not stable across Qt versions : Qt 6.11
 * switched to a 19 field format carrying OpenType weights, which
 * QFont::fromString() of Qt 5.x and Qt <= 6.10 rejects, leaving a broken font.
 * The 10/11 field form is parsed correctly by every Qt version (they convert
 * the legacy weight scale as needed), so composing it ourselves keeps files
 * readable by every QET build in circulation.
 * See @link https://github.com/qelectrotech/qelectrotech-source-mirror/issues/553 @endlink
 * @param font
 * @return the font description string
 */
QString QETUtils::fontToString(const QFont &font)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	return font.toString();
#else
	const int legacy_weight = legacyFontWeight(font.weight());

	const QChar comma(QLatin1Char(','));
	QString description = font.family() + comma +
		QString::number(font.pointSizeF()) + comma +
		QString::number(font.pixelSize()) + comma +
		QString::number(int(font.styleHint())) + comma +
		QString::number(legacy_weight) + comma +
		QString::number(int(font.style())) + comma +
		QString::number(int(font.underline())) + comma +
		QString::number(int(font.strikeOut())) + comma +
		QString::number(int(font.fixedPitch())) + comma +
		QString::number(0);
	if (!font.styleName().isEmpty()) {
		description += QChar(',') + font.styleName();
	}
	return description;
#endif
}

/**
 * @brief QETUtils::fontFromString
 * Restore a font from a description string, tolerating descriptions written
 * by any Qt version. QFont::fromString() of Qt 5.x and Qt <= 6.10 rejects the
 * >= 19 field format QFont::toString() emits since Qt 6.11 (16 base fields
 * plus style name, font-feature count and variable-axis count), leaving a
 * broken font behind. When the native parser fails, salvage such a
 * description by re-composing the 10/11 field form (OpenType weight mapped
 * back to the legacy scale) and parsing that instead, so no font information
 * stored in existing files is lost.
 * See @link https://github.com/qelectrotech/qelectrotech-source-mirror/issues/553 @endlink
 * @param font : font to restore into; left untouched on failure, so the
 * caller's default keeps applying.
 * @param description : font description string
 * @return true if the description could be parsed
 */
bool QETUtils::fontFromString(QFont &font, const QString &description)
{
	QFont parsed(font);
	if (parsed.fromString(description)) {
		font = parsed;
		return true;
	}

	const QStringList l = description.split(QLatin1Char(','));
	const int count = l.size();
	const QChar comma(QLatin1Char(','));

		//16 base fields + style name + feature count + axis count; a family
		//name containing commas adds leading fields. Descriptions actually
		//using font features or variable axes append value groups whose
		//layout we cannot anchor reliably, so only salvage the plain
		//"...,0,0" case (the only one QET-edited files can contain).
	if (count >= 19
		&& l.at(count - 1).trimmed() == QLatin1String("0")
		&& l.at(count - 2).trimmed() == QLatin1String("0"))
	{
		QString legacy = QStringList(l.mid(0, count - 18)).join(comma) + comma + // family
			l.at(count - 18) + comma +                                       // pointSizeF
			l.at(count - 17) + comma +                                       // pixelSize
			l.at(count - 16) + comma +                                       // styleHint
			QString::number(legacyFontWeight(l.at(count - 15).toInt())) + comma +
			l.at(count - 14) + comma +                                       // style
			l.at(count - 13) + comma +                                       // underline
			l.at(count - 12) + comma +                                       // strikeOut
			l.at(count - 11) + comma +                                       // fixedPitch
			QString::number(0);
		const QString style_name = l.at(count - 3);
		if (!style_name.isEmpty()) {
			legacy += comma + style_name;
		}

		if (parsed.fromString(legacy)) {
			font = parsed;
			return true;
		}
		return false;
	}

		//Some historical builds double-serialized fonts, embedding a complete
		//legacy description as the family name of a second one (21 fields:
		//"Caladea,9,-1,5,75,1,0,0,0,0,Bold Italic,9,-1,0,50,0,0,0,0,0,Regular").
		//The embedded leading description carries the real font, and taking it
		//matches what the lenient parser of Qt 6.11+ resolves such strings to.
	if (count > 11
		&& parsed.fromString(QStringList(l.mid(0, 11)).join(comma))) {
		font = parsed;
		return true;
	}
	return false;
}
