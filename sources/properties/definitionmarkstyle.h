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
#ifndef DEFINITIONMARKSTYLE_H
#define DEFINITIONMARKSTYLE_H

#include <QColor>
#include <QFont>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>

class QDomElement;
class QFontMetricsF;
class QPainter;
class QSettings;

/**
	@brief The DefinitionMarkStyle class
	How a conductor's definition mark (ConductorDefinitionMark) is drawn:
	the shapes that make up its mark and the texts beside it. A style is
	made of shape items (a line, a circle or a rectangle) and of text
	items whose formula gives their words (%wc, %ws... @see
	autonum::AssignVariables), both placed around the point where the
	mark stands on the conductor.

	The project carries the style its conductors are drawn with
	(QETProject::definitionMarkStyle()), so a project looks the same
	wherever it is opened; the application keeps one as the default for
	new projects. A style whose m_enabled is false draws the mark the way
	it always was: one slanted stroke with the colour above and the
	section below, and the project does not save it.
*/
class DefinitionMarkStyle
{
	public:
		enum class ShapeKind { Line, Circle, Rectangle };

		/**
			@brief The ShapeItem struct
			One shape drawn at the mark: its centre is at m_pos,
			turned by m_rotation degrees. A shape's own colour and
			pen width win over the conductor's when they are set
			(m_color valid, m_pen_width > 0).
		*/
		struct ShapeItem
		{
			ShapeKind m_kind = ShapeKind::Line;
			QPointF m_pos;
			qreal m_rotation = 0;
			qreal m_width = 8;
			qreal m_height = 8;
			qreal m_pen_width = 0;
			QColor m_color;
			bool m_filled = false;

			bool operator==(const ShapeItem &) const;
			bool operator!=(const ShapeItem &) const;
		};

		/**
			@brief The TextItem struct
			One text drawn at the mark: its place is m_pos, where
			m_alignment says the text box stands -- centered, or
			growing in one direction from it. m_formula holds the
			words, with the variables of a conductor's formula
			(%wc, %ws, %wf, %wv, %wt, %wb...). m_size and m_color
			take the conductor text's own when they say nothing
			(0, invalid). A text turns with the drawing; one that
			stays upright (m_upright) keeps its angle instead.
		*/
		struct TextItem
		{
			QString m_formula;
			QPointF m_pos;
			qreal m_rotation = 0;
			qreal m_size = 0;
			QColor m_color;
			bool m_upright = false;
			Qt::Alignment m_alignment = Qt::AlignCenter;

			bool operator==(const TextItem &) const;
			bool operator!=(const TextItem &) const;
		};

		/**
			@brief The RenderContext struct
			What the shapes and texts take from the conductor when
			their own values say nothing.
		*/
		struct RenderContext
		{
			QColor conductor_color;
			qreal conductor_width = 1;
			QColor text_color;
			QFont font;
		};

		bool m_enabled = false;
			/// The turn the whole drawing takes where the conductor
			/// runs up and down, in degrees (0, 90, 180 or 270):
			/// shapes, texts and their places turn with it
		qreal m_vertical_turn = 0;
		QList<ShapeItem> m_shapes;
		QList<TextItem> m_texts;

		static DefinitionMarkStyle standardStyle();
		bool isStandard() const;

		void toXml(QDomElement &) const;
		void fromXml(const QDomElement &);
		void toSettings(QSettings &, const QString & = QString()) const;
		void fromSettings(const QSettings &, const QString & = QString());

		QRectF boundingRect(const RenderContext &,
				    const QStringList &,
				    qreal turn) const;
		void paint(QPainter *,
			   const RenderContext &,
			   const QStringList &,
			   qreal turn) const;

		static QRectF itemRect(const ShapeItem &,
				       const RenderContext &,
				       qreal turn = 0);
		static QRectF itemRect(const TextItem &,
				       const RenderContext &,
				       const QString &,
				       qreal turn = 0);
		static QRectF textRect(const TextItem &,
				       const QFontMetricsF &,
				       const QString &);
		static void paintItem(QPainter *,
				      const ShapeItem &,
				      const RenderContext &,
				      qreal turn = 0);
		static void paintItem(QPainter *,
				      const TextItem &,
				      const RenderContext &,
				      const QString &,
				      qreal turn = 0);

		bool operator==(const DefinitionMarkStyle &) const;
		bool operator!=(const DefinitionMarkStyle &) const;
};

#endif // DEFINITIONMARKSTYLE_H
