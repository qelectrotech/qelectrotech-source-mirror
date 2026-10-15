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
#ifndef TERMINALCOLOR_H
#define TERMINALCOLOR_H

#include <QColor>
#include <QCoreApplication>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QString>
#include <QVector>

/**
 * Colors of the terminals of a terminal strip.
 *
 * A color is stored as a string : "#rrggbb" for a plain color
 * or "#rrggbb/#rrggbb" for a two-tone color (the second color is drawn
 * as stripes over the first one, like the green/yellow of the earth).
 * An empty string means "no color set".
 */
namespace TerminalColor
{
	struct Entry
	{
		const char *english;	//Source text of the translation
		const char *french;		//Also accepted when typed or pasted
		const char *main;
		const char *stripe;		//nullptr if the color is plain
	};

	inline const QVector<Entry> &palette()
	{
		static const QVector<Entry> palette_ {
			{"Gray",         "gris",       "#9e9e9e", nullptr},
			{"Blue",         "bleu",       "#4a90d9", nullptr},
			{"Green/Yellow", "vert/jaune", "#ffd800", "#2e9e2e"},
			{"Brown",        "marron",     "#8b5a2b", nullptr},
			{"Black",        "noir",       "#303030", nullptr},
			{"Red",          "rouge",      "#d32f2f", nullptr},
			{"Orange",       "orange",     "#f57c00", nullptr},
			{"Yellow",       "jaune",      "#ffd800", nullptr},
			{"Green",        "vert",       "#2e9e2e", nullptr},
			{"Violet",       "violet",     "#7b3fa0", nullptr},
			{"White",        "blanc",      "#ffffff", nullptr}
		};
		return palette_;
	}

	inline QString translatedName(const Entry &entry) {
		return QCoreApplication::translate("TerminalColor", entry.english);
	}

	inline QString toSpec(const QColor &main, const QColor &stripe = QColor())
	{
		if (!main.isValid()) {
			return QString();
		}
		return stripe.isValid() ? main.name() + QLatin1Char('/') + stripe.name()
								: main.name();
	}

	inline QString specOf(const Entry &entry) {
		return toSpec(QColor(QLatin1String(entry.main)),
					  entry.stripe ? QColor(QLatin1String(entry.stripe)) : QColor());
	}

	/**
	 * @brief parse
	 * Read a color from a text : a name of the palette (in english, in french
	 * or translated), a color "#rrggbb" or "#rrggbb/#rrggbb".
	 * An empty text is valid and gives invalid colors (no color).
	 * @return true if the text was understood
	 */
	inline bool parse(const QString &text, QColor *main, QColor *stripe)
	{
		*main = QColor();
		*stripe = QColor();

		const auto text_ = text.trimmed();
		if (text_.isEmpty()) {
			return true;
		}

		for (const auto &entry : palette())
		{
			if (text_.compare(QLatin1String(entry.english), Qt::CaseInsensitive) == 0 ||
				text_.compare(QLatin1String(entry.french),  Qt::CaseInsensitive) == 0 ||
				text_.compare(translatedName(entry),        Qt::CaseInsensitive) == 0)
			{
				*main = QColor(QLatin1String(entry.main));
				if (entry.stripe) {
					*stripe = QColor(QLatin1String(entry.stripe));
				}
				return true;
			}
		}

		const auto parts_ = text_.split(QLatin1Char('/'));
		if (parts_.size() == 2)
		{
			const QColor c1(parts_.at(0).trimmed());
			const QColor c2(parts_.at(1).trimmed());
			if (c1.isValid() && c2.isValid()) {
				*main = c1;
				*stripe = c2;
				return true;
			}
			return false;
		}

		const QColor color_(text_);
		if (color_.isValid()) {
			*main = color_;
			return true;
		}
		return false;
	}

	/**
	 * @return The name to display for a color : the translated name
	 * if the color is in the palette, else the color as text.
	 */
	inline QString displayName(const QColor &main, const QColor &stripe)
	{
		const auto spec_ = toSpec(main, stripe);
		if (spec_.isEmpty()) {
			return QString();
		}
		for (const auto &entry : palette()) {
			if (specOf(entry) == spec_) {
				return translatedName(entry);
			}
		}
		return spec_;
	}

	/**
	 * @return A little pixmap of the color (the stripe color
	 * fills the upper right half for a two-tone color)
	 */
	inline QPixmap swatch(const QColor &main, const QColor &stripe, int size = 16)
	{
		QPixmap pixmap_(size, size);
		pixmap_.fill(Qt::transparent);
		if (!main.isValid()) {
			return pixmap_;
		}

		QPainter painter_(&pixmap_);
		painter_.setRenderHint(QPainter::Antialiasing);
		painter_.setPen(Qt::NoPen);
		painter_.setBrush(main);
		painter_.drawRect(0, 0, size, size);
		if (stripe.isValid())
		{
			painter_.setBrush(stripe);
			QPolygonF polygon_;
			polygon_ << QPointF(0, 0) << QPointF(size, 0) << QPointF(size, size);
			painter_.drawPolygon(polygon_);
		}
		painter_.setPen(QPen(Qt::gray, 1));
		painter_.setBrush(Qt::NoBrush);
		painter_.drawRect(0, 0, size - 1, size - 1);
		return pixmap_;
	}
}

#endif // TERMINALCOLOR_H
